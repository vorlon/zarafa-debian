/*
 * Copyright 2005 - 2009  Zarafa B.V.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License, version 3, 
 * as published by the Free Software Foundation with the following additional 
 * term according to sec. 7:
 *  
 * According to sec. 7 of the GNU Affero General Public License, version
 * 3, the terms of the AGPL are supplemented with the following terms:
 * 
 * "Zarafa" is a registered trademark of Zarafa B.V. The licensing of
 * the Program under the AGPL does not imply a trademark license.
 * Therefore any rights, title and interest in our trademarks remain
 * entirely with us.
 * 
 * However, if you propagate an unmodified version of the Program you are
 * allowed to use the term "Zarafa" to indicate that you distribute the
 * Program. Furthermore you may use our trademarks where it is necessary
 * to indicate the intended purpose of a product or service provided you
 * use it in accordance with honest practices in industrial or commercial
 * matters.  If you want to propagate modified versions of the Program
 * under the name "Zarafa" or "Zarafa Server", you may only do so if you
 * have a written permission by Zarafa B.V. (to acquire a permission
 * please contact Zarafa at trademark@zarafa.com).
 * 
 * The interactive user interface of the software displays an attribution
 * notice containing the term "Zarafa" and/or the logo of Zarafa.
 * Interactive user interfaces of unmodified and modified versions must
 * display Appropriate Legal Notices according to sec. 5 of the GNU
 * Affero General Public License, version 3, when you propagate
 * unmodified or modified versions of the Program. In accordance with
 * sec. 7 b) of the GNU Affero General Public License, version 3, these
 * Appropriate Legal Notices must retain the logo of Zarafa or display
 * the words "Initial Development by Zarafa" if the display of the logo
 * is not reasonably feasible for technical reasons. The use of the logo
 * of Zarafa in Legal Notices is allowed for unmodified and modified
 * versions of the software.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *  
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include "platform.h"
#include "ZarafaCode.h"
#include "soapH.h"
#include "ECClientUpdate.h"
#include "ECLicenseClient.h"
#include "stringutil.h"
#include "base64.h"
#include "ECLogger.h"
#include "ECConfig.h"

#include <boost/filesystem.hpp>
namespace bfs = boost::filesystem;

#include <boost/algorithm/string/predicate.hpp>
namespace ba = boost::algorithm;

/* class and add constructor params? */
extern ECLogger *g_lpLogger;
extern ECConfig *g_lpConfig;

struct ClientVersion
{
    unsigned int nMajorVersion;
    unsigned int nMinorVersion;
    unsigned int nUpdateNumber;
    unsigned int nBuildNumber;
};

bool ConvertAndValidatePath(std::string strClientUpdatePath, std::string strMSIName, std::string *lpstrDownloadFile);
bool GetVersionFromString(char *szVersion, ClientVersion *lpClientVersion);
bool GetVersionFromMSIName(const char *szVersion, ClientVersion *lpClientVersion);
int  CompareVersions(ClientVersion Version1, ClientVersion Version2);
bool GetLatestVersionAtServer(char *szUpdatePath, ClientVersion *lpLatestVersion);
bool GetClientMSINameFromVersion(ClientVersion clientVersion, char *szMSIName, size_t strsize);


/*
 * Handles the HTTP GET command from soap, only the client update install may be downloaded.
 *
 * This function can only be called then client_update_enabled is set to yes.
 */
int HandleClientUpdate(struct soap *soap) 
{ 
	std::string strPath;

	int nRet = 404;				// default return file not found to soap
	char *szClientUpdatePath = NULL;
	char *szCurrentVersion = NULL;
	char *szReq = NULL;
	char *szReqEnd = NULL;
	std::string strLicenseRequest;
	std::string strLicenseResponse;

	ECLicenseClient *lpLicenseClient = NULL;
	unsigned int ulLicenseResponse = 0;
	unsigned char *lpLicenseResponse = NULL;
	ECRESULT er = erSuccess;
	ClientVersion currentVersion;
	ClientVersion latestVersion;
	// TODO: rewrite with std::string
	char szClientMSIName[MAX_PATH];
	FILE *fd = NULL;


	// Get the server.cfg setting
	szClientUpdatePath = g_lpConfig->GetSetting("client_update_path");

	if (!szClientUpdatePath) {
		g_lpLogger->Log(EC_LOGLEVEL_ERROR, "Client update: The client_update_path is not found.");
		goto exit;
	}

	// if the version comes as "/autoupdate/6.20.1.1234?licreq", we need to pass the license request
	szReq = strrchr(soap->path, '?');
	if (szReq != NULL)
	{
		// since we have the ?, that's good enough
		szReq = strstr(soap->buf, "X-License: ");
		if (szReq == NULL) {
			g_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Invalid license request, header not found.");
			goto exit;
		}
		szReq += strlen("X-License: ");
		szReqEnd = strstr(szReq, "\r\n"); // TODO: can be be split over multiple lines?
		if (szReqEnd == NULL) {
			g_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Invalid license request, end of header not found.");
			goto exit;
		}
		strLicenseRequest = base64_decode(std::string(szReq, szReqEnd - szReq));

		lpLicenseClient = new ECLicenseClient(g_lpConfig->GetSetting("license_socket"),  atoui(g_lpConfig->GetSetting("license_timeout")));
		er = lpLicenseClient->Auth((unsigned char*)strLicenseRequest.c_str(), strLicenseRequest.length(), &lpLicenseResponse, &ulLicenseResponse);
		if (er != erSuccess) {
			g_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Invalid license request, error: 0x%08X.", er);
			goto exit;
		}

		strLicenseResponse = base64_encode(lpLicenseResponse, ulLicenseResponse);

		soap->http_content = "binary";
		soap_response(soap, SOAP_FILE);
		nRet = soap_send_raw(soap, strLicenseResponse.c_str(), strLicenseResponse.length());
		g_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Processing license request.");
		goto exit;
	}

	// the version comes as "/autoupdate/6.20.1.1234", convert it to "6.20.1.1234"
	szCurrentVersion = soap->path + strlen("/autoupdate");
	if (szCurrentVersion[0] == '/')
		szCurrentVersion++;

	if (szCurrentVersion[0] != '\0')
	{
		g_lpLogger->Log(EC_LOGLEVEL_INFO, "Client update: The current client version is %s.", szCurrentVersion);
		if (!GetVersionFromString(szCurrentVersion, &currentVersion))
		{
			g_lpLogger->Log(EC_LOGLEVEL_INFO, "Client update: Failed in getting version from input data.");
			goto exit;
		}
	}

	if (!GetLatestVersionAtServer(szClientUpdatePath, &latestVersion))
	{
		g_lpLogger->Log(EC_LOGLEVEL_INFO, "Client update: No updates found on server.");
		goto exit;
	}

	if (szCurrentVersion[0] != '\0')
	{
		int res = CompareVersions(currentVersion, latestVersion);
		if (res == 0) {
			g_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Client already has latest version.");
			goto exit;
		} else if (res > 0)	{
			g_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Client has newer version than server.");
			goto exit;
		}
	}

	if (!GetClientMSINameFromVersion(latestVersion, szClientMSIName, MAX_PATH))
	{
		g_lpLogger->Log(EC_LOGLEVEL_INFO, "Client update: No suitable version available.");
		goto exit;
	}

	if (ConvertAndValidatePath(szClientUpdatePath, szClientMSIName, &strPath) != true)
	{
		g_lpLogger->Log(EC_LOGLEVEL_INFO, "Client update: Error in path conversion and validation.");
		goto exit;
	}

	fd = fopen(strPath.c_str(), "rb");
	if (!fd)
	{
		g_lpLogger->Log(EC_LOGLEVEL_ERROR, "Client update: Path not found %s.", strPath.c_str());
		goto exit;
	}

	g_lpLogger->Log(EC_LOGLEVEL_ERROR, "Client update: Sending client %s new installer %s", PrettyIP(soap->ip).c_str(), szClientMSIName);

	// application/msi-installer ?
	soap->http_content = "binary";
	soap_response(soap, SOAP_FILE);

	while (true)
	{
		// FIXME: tmpbuf is only 1K, good enough?
		size_t nSize = fread(soap->tmpbuf, 1, sizeof(soap->tmpbuf), fd);

		if (!nSize)
			break;

		if (soap_send_raw(soap, soap->tmpbuf, nSize))
		{
			g_lpLogger->Log(EC_LOGLEVEL_FATAL, "Client update: Error while sending client new installer");
			goto exit;
		}
	}
	nRet = SOAP_OK;

exit:
	if (lpLicenseResponse)
		delete [] lpLicenseResponse;

	if (lpLicenseClient)
		delete lpLicenseClient;

	if (fd)
		fclose(fd);

	return nRet;
}

bool ConvertAndValidatePath(std::string strClientUpdatePath, std::string strMSIName, std::string *lpstrDownloadFile)
{
	bool bRet = false;
	size_t nTempLen = 0;
	std::string strFile;
	char cPathSeparator = '/';

	if (lpstrDownloadFile == NULL)
		goto exit;

	strFile = strClientUpdatePath;
	nTempLen = strFile.length();

	// not 100% correct, but good enough
	if (strstr(strClientUpdatePath.c_str(), "/.."))
	{
		g_lpLogger->Log(EC_LOGLEVEL_FATAL, "Update path contains invalid .. to previous path.");
		goto exit;
	}

	if (strFile[nTempLen - 1] != cPathSeparator)
		strFile += cPathSeparator;

	strFile += strMSIName;
	*lpstrDownloadFile = strFile;
	bRet = true;

exit:
	return bRet;
}

//<Major>.<Minor>.<Update>.<Build No.>
bool GetVersionFromString(char *szVersion, ClientVersion *lpClientVersion)
{
	ClientVersion cv;
	std::vector<std::string> vParts;

	if (szVersion == NULL)
		return false;

	vParts = tokenize(szVersion, '.');
	if (vParts.size() != 4)
		return false;

	cv.nMajorVersion = atoi(vParts[0].c_str());
	cv.nMinorVersion = atoi(vParts[1].c_str());
	cv.nUpdateNumber = atoi(vParts[2].c_str());
	cv.nBuildNumber = atoi(vParts[3].c_str());

	*lpClientVersion = cv;

	return true;
}

//<Major>.<Minor>.<Update>-<Build No.>.msi
bool GetVersionFromMSIName(const char *szVersion, ClientVersion *lpClientVersion)
{
	ClientVersion cv;
	std::vector<std::string> vParts;

	if (NULL == szVersion)
		return false;

	vParts = tokenize(szVersion, ".-");
	if (vParts.size() != 5)		// 5 because of the .msi at the end
		return false;

	cv.nMajorVersion = atoi(vParts[0].c_str());
	cv.nMinorVersion = atoi(vParts[1].c_str());
	cv.nUpdateNumber = atoi(vParts[2].c_str());
	cv.nBuildNumber = atoi(vParts[3].c_str());

	*lpClientVersion = cv;

	return true;
}

/*
  The return values are following:

  -n - Version1 <  Version2 (client should upgrade)
   0 - Version1 == Version2
   n - Version1 >  Version2
*/
int CompareVersions(ClientVersion Version1, ClientVersion Version2)
{
	if (Version1.nMajorVersion != Version2.nMajorVersion)
		return Version1.nMajorVersion - Version2.nMajorVersion;

	if (Version1.nMinorVersion != Version2.nMinorVersion)
		return Version1.nMinorVersion - Version2.nMinorVersion;

	if (Version1.nUpdateNumber != Version2.nUpdateNumber)
		return Version1.nUpdateNumber - Version2.nUpdateNumber;

	if (Version1.nBuildNumber != Version2.nBuildNumber)
		return Version1.nBuildNumber - Version2.nBuildNumber;

	return 0;
}

//zarafaclient-6.20-1234.msi
//zarafaclient-*.*-*.msi
bool GetLatestVersionAtServer(char *szUpdatePath, ClientVersion *lpLatestVersion)
{
	ClientVersion tempVersion = {0};
	ClientVersion latestVersion = {0};
	std::string strFileStart = "zarafaclient-";

	bool bRet = false;

	if (szUpdatePath == NULL)
		goto exit;

	try {
		bfs::path updatesdir = szUpdatePath;
		if (!bfs::exists(updatesdir)) {
			g_lpLogger->Log(EC_LOGLEVEL_ERROR, "Unable to open client_update_path directory");
			goto exit;
		}

		bfs::directory_iterator update_last;
		for (bfs::directory_iterator update(updatesdir); update != update_last; update++) {
			std::string strFilename = update->path().leaf();

			if (!bfs::is_regular(*update) && !bfs::is_symlink(*update)) {
				g_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Invalid entry in %s: %s", szUpdatePath, strFilename.c_str());
				continue;
			}

			if (!ba::starts_with(update->path().leaf(), strFileStart)) {
				g_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Ignoring file %s for client update", strFilename.c_str());
				continue;
			}

			g_lpLogger->Log(EC_LOGLEVEL_INFO, "Update Name: %s", strFilename.c_str());

			const char *pTemp = strFilename.c_str() + strFileStart.length();
			if (!GetVersionFromMSIName(pTemp, &tempVersion))
			{
				g_lpLogger->Log(EC_LOGLEVEL_WARNING, "Failed in getting version from string '%s'", pTemp);
				continue;
			}

			// first time, latestVersion will be 0, so always older
			if (CompareVersions(latestVersion, tempVersion) < 0) {
				bRet = true;
				latestVersion = tempVersion;
			}
		}
	} catch (const bfs::filesystem_error &e) {
		g_lpLogger->Log(EC_LOGLEVEL_INFO, "Boost exception during certificate validation: %s", e.what());
	} catch (const std::exception &e) {
		g_lpLogger->Log(EC_LOGLEVEL_INFO, "STD exception during certificate validation: %s", e.what());
	}

	if (bRet)
		*lpLatestVersion = latestVersion;

exit:
	return bRet;
}

bool GetClientMSINameFromVersion(ClientVersion clientVersion, char *szMSIName, size_t strsize)
{
	if (szMSIName == NULL || strsize == 0)
		return false;

	snprintf(szMSIName, strsize, "zarafaclient-%d.%d.%d-%d.msi", clientVersion.nMajorVersion, clientVersion.nMinorVersion, clientVersion.nUpdateNumber, clientVersion.nBuildNumber);
	return true;
}
