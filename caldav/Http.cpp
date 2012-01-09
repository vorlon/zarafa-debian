/*
 * Copyright 2005 - 2011  Zarafa B.V.
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
#include "Http.h"

#include "ECConfig.h"

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/** 
 * Parse the incoming URL into known peices:
 *
 * /<service>/[username][/foldername][/uid.ics]
 * service: ical or caldav, mandatory
 * username: open store of this user, "public" for public folders. optional: default comes from HTTP Authenticate header.
 * foldername: folder name in store to open. multiple forms possible (normal name, prefix_guid, prefix_entryid).
 * 
 * @param[in] wstrUrl incoming url
 * @param[out] lpulFlag url flags (service type and public marker)
 * @param[out] lpwstrUrlUser owner of lpwstrFolder
 * @param[out] lpwstrFolder folder id or name
 * 
 * @return 
 */
HRESULT HrParseURL(const std::wstring &wstrUrl, ULONG *lpulFlag, std::wstring *lpwstrUrlUser, std::wstring *lpwstrFolder)
{
	std::wstring wstrService;
	std::wstring wstrFolder;
	std::wstring wstrUrlUser;
	vector<std::wstring> vcUrlTokens;
	vector<std::wstring>::iterator iterToken;
	ULONG ulFlag = 0;
	
	vcUrlTokens = tokenize(wstrUrl.substr(1,wstrUrl.length()), L'/', true);
	if (vcUrlTokens.empty())
		goto exit;

	if (vcUrlTokens.back().rfind(L".ics") != string::npos) {
		// Guid is retrieved using StripGuid().
		vcUrlTokens.pop_back();
	} else {
		// request is for folder not a calendar entry
		ulFlag |= REQ_COLLECTION;
	}
	
	if (vcUrlTokens.empty())
		goto exit;

	iterToken = vcUrlTokens.begin();
	
	wstrService = *iterToken++;
	
	//change case of Service name ICAL -> ical CALDaV ->caldav
	std::transform(wstrService.begin(), wstrService.end(), wstrService.begin(), (int(*)(int)) std::tolower);
	
	if (!wstrService.compare(L"ical"))
		ulFlag |= SERVICE_ICAL;
	else if (!wstrService.compare(L"caldav"))
		ulFlag |= SERVICE_CALDAV;
	else
		ulFlag |= SERVICE_UNKNOWN;

	if (iterToken == vcUrlTokens.end())
		goto exit;
	wstrUrlUser = *iterToken++;
	if (!wstrUrlUser.empty()) {
		//change case of folder owner USER -> user, UseR -> user
		std::transform(wstrUrlUser.begin(), wstrUrlUser.end(), wstrUrlUser.begin(), (int(*)(int)) std::tolower);
	}

	// check if the request is for public folders and set the bool flag
	// @note: request for public folder not have user's name in the url
	if (!wstrUrlUser.compare(L"public"))
		ulFlag |= REQ_PUBLIC;

	if (iterToken == vcUrlTokens.end())
		goto exit;

	// @todo subfolder/folder/ is not allowed! only subfolder/item.ics
	for ( ;iterToken != vcUrlTokens.end(); iterToken++) 
			wstrFolder = wstrFolder + *iterToken + L"/";

	wstrFolder.erase(wstrFolder.length() - 1);

	
exit:
	if (lpulFlag)
		*lpulFlag = ulFlag;
	
	if (lpwstrUrlUser)
		lpwstrUrlUser->swap(wstrUrlUser);

	if (lpwstrFolder)
		lpwstrFolder->swap(wstrFolder);

	return hrSuccess;
}

Http::Http(ECChannel *lpChannel, ECLogger *lpLogger, ECConfig *lpConfig)
{
	m_lpChannel = lpChannel;
	m_lpLogger = lpLogger;
	m_lpConfig = lpConfig;

	m_ulKeepAlive = 0;
	m_ulRetCode = 0;

	m_strRespHeader.clear();
}

/**
 * Default destructor
 */
Http::~Http()
{
}

/**
 * Reads the http headers from the channel and Parses them
 *
 * @return	HRESULT
 * @retval	MAPI_E_INVALID_PARAMETER	The http hearders are invalid
 */
HRESULT Http::HrReadHeaders()
{
	HRESULT hr = hrSuccess;
	std::string strBuffer;
	ULONG n = 0;
	std::map<std::string, std::string>::iterator iHeader = mapHeaders.end();

	m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Receiving headers:");
	do
	{
		hr = m_lpChannel->HrReadLine(&strBuffer);
		if (hr != hrSuccess)
			goto exit;

		if (strBuffer.empty())
			break;

		if (n == 0) {
			m_strAction = strBuffer;
			m_lpLogger->Log(EC_LOGLEVEL_ERROR, "Request URL: %s", m_strAction.c_str());
		} else {
			std::string::size_type pos = strBuffer.find(':');
			std::string::size_type start = 0;
			std::pair<std::map<std::string, std::string>::iterator, bool> r;

			if (strBuffer[0] == ' ' || strBuffer[0] == '\t') {
				if (iHeader == mapHeaders.end())
					continue;
				// continue header
				while (strBuffer[start] == ' ' || strBuffer[start] == '\t') start++;
				iHeader->second += strBuffer.substr(start);
			} else {
				// new header
				r = mapHeaders.insert(make_pair<string,string>(strBuffer.substr(0,pos), strBuffer.substr(pos+2)));
				iHeader = r.first;
			}
		}

		if (m_lpLogger->Log(EC_LOGLEVEL_DEBUG)) {
			if (strBuffer.find("Authorization") != string::npos)
				m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "< Authorization: <value hidden>");
			else if (n > 0)
				m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "< "+strBuffer);
		}
		n++;

	} while(hr == hrSuccess);

	hr = HrParseHeaders();

exit:
	return hr;
}

/**
 * Parse the http headers
 * @return	HRESULT
 * @retval	MAPI_E_INVALID_PARAMETER	The http headers are invalid
 */
// @todo this does way too much.
HRESULT Http::HrParseHeaders()
{
	HRESULT hr = hrSuccess;
	std::string strAuthdata;
	std::string strLength;
	char *lpURI = NULL;

	std::vector<std::string> items;
	std::map<std::string, std::string>::iterator iHeader = mapHeaders.end();

	items = tokenize(m_strAction, ' ', true);
	if (items.size() != 3) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}
	m_strMethod = items[0];
	m_strHttpVer = items[2];

	// converts %20 -> ' '
	lpURI = xmlURIUnescapeString((const char *)items[1].c_str(), items[1].size(), NULL);
	if (lpURI) {
		m_strPath = lpURI;
		xmlFree(lpURI);
		lpURI = NULL;
	} else {
		m_strPath = items[1];
	}

	// find the content-type
	// Content-Type: text/xml;charset=UTF-8
	hr = HrGetHeaderValue("Content-Type", &m_strCharSet);
	if (hr == hrSuccess && m_strCharSet.find("charset") != std::string::npos)
		m_strCharSet = m_strCharSet.substr(m_strCharSet.find("charset")+ strlen("charset") + 1, m_strCharSet.length());
	else
		m_strCharSet = m_lpConfig->GetSetting("default_charset"); // really should be utf-8

	// find the Authorisation data (Authorization: Basic wr8y273yr2y3r87y23ry7=)
	hr = HrGetHeaderValue("Authorization", &strAuthdata);
	if (hr != hrSuccess) {
		hr = HrGetHeaderValue("WWW-Authenticate", &strAuthdata);
		if (hr != hrSuccess) {
			hr = S_OK;	// ignore empty Authorization
			goto exit;
		}
	}

	items = tokenize(strAuthdata, ' ', true);
	// we only support basic authentication
	if (items.size() != 2 || items[0].compare("Basic") != 0) {
		hr = MAPI_E_LOGON_FAILED;
		goto exit;
	}


	items = tokenize(base64_decode(items[1]), ':');
	if (items.size() != 2) {
		hr = MAPI_E_LOGON_FAILED;
		goto exit;
	}

	m_strUser = items[0];
	m_strPass = items[1];

	m_lpLogger->Log(EC_LOGLEVEL_INFO, "Request from User : %s", m_strUser.c_str());

exit:
	return hr;
}

/**
 * Returns the user name set in the request
 * @param[in]	strUser		Return string for username in request
 * @return		HRESULT
 * @retval		MAPI_E_NOT_FOUND	No username set in the request
 */
HRESULT Http::HrGetUser(std::wstring *strUser)
{
	HRESULT hr = hrSuccess;
	if (!m_strUser.empty())
		hr = X2W(m_strUser, strUser);
	else
		hr = MAPI_E_NOT_FOUND;

	return hr;
}

/**
 * Returns the method set in the url
 * @param[out]	strMethod	Return string for method set in request
 * @return		HRESULT
 * @retval		MAPI_E_NOT_FOUND	Empty method in request
 */
HRESULT Http::HrGetMethod(std::string *strMethod)
{
	HRESULT hr = hrSuccess;

	if (!m_strMethod.empty())
		strMethod->assign(m_strMethod);
	else
		hr = MAPI_E_NOT_FOUND;

	return hr;
}

/**
 * Returns the password sent by user
 * @param[out]	strPass		The password is returned
 * @return		HRESULT
 * @retval		MAPI_E_NOT_FOUND	Empty password in request
 */
HRESULT Http::HrGetPass(std::wstring *strPass)
{
	HRESULT hr = hrSuccess;
	if (!m_strPass.empty())
		hr = X2W(m_strPass, strPass);
	else
		hr = MAPI_E_NOT_FOUND;

	return hr;
}

/**
 * Returns the full path of request(e.g. /caldav/user/folder)
 * @param[out]	strReqPath		Return string for path
 * @return		HRESULT
 * @retval		MAPI_E_NOT_FOUND	Empty path in request
 */
HRESULT Http::HrGetUrl(std::wstring *wstrUrl)
{
	HRESULT hr = hrSuccess;

	if (!m_strPath.empty())
		hr = X2W(m_strPath, wstrUrl);
	else
		hr = MAPI_E_NOT_FOUND;

	return hr;
}

/**
 * Returns the User agent of the request
 * @param[out]	strAgent	Return user-agent
 * @return		HRESULT
 * @retval		MAPI_E_NOT_FOUND	No user-agent string present in request
 */
// @todo remove
HRESULT Http::HrGetUserAgent(std::string *strAgent)
{
	return HrGetHeaderValue("User-Agent", strAgent);
}

/**
 * Returns body of the request
 * @param[in]	strBody		Return string for  body of the request
 * @return		HRESULT
 * @retval		MAPI_E_NOT_FOUND	No body present in request
 */
HRESULT Http::HrGetBody(std::string *strBody)
{
	HRESULT hr = hrSuccess;
	if (!m_strReqBody.empty())
		strBody->assign(m_strReqBody);
	else
		hr = MAPI_E_NOT_FOUND;

	return hr;
}

/**
 * Returns the Depth set in request
 * @param[out]	ulDepth		Return string for depth set in request
 * @return		HRESULT
 * @retval		MAPI_E_NOT_FOUND	No depth value set in request
 */
HRESULT Http::HrGetDepth(ULONG *ulDepth)
{
	HRESULT hr = hrSuccess;
	std::string strDepth;

	/*
	 * Valid input: [0, 1, infinity]
	 */
	hr = HrGetHeaderValue("Depth", &strDepth);
	if (hr != hrSuccess)
		*ulDepth = 0;		// default is no subfolders. default should become a parameter .. is action dependant
	else if (strDepth.compare("infinity") == 0)
		*ulDepth = 2;
	else {
		*ulDepth = atoi(strDepth.c_str());
		if (*ulDepth > 1)
			*ulDepth = 1;
	}

	return hr;
}

/**
 * Returns Match, If-Match value of request
 * 
 * Used to compare with etag values
 *
 * @param[out]	strIfMatch	Return string for Match, If-match value set in request
 * @return		HRESULT
 * @retval		MAPI_E_NOT_FOUND	No Match, If-match value set in request
 */
HRESULT Http::HrGetIfMatch(std::string *strIfMatch)
{
	HRESULT hr = hrSuccess;
	std::string strData;
	size_t szLength = 0;
	ULONG ulStart = 0;
	ULONG ulEnd = 0;

	hr = HrGetHeaderValue("If-Match", &strData);
	if (hr == hrSuccess) {
		szLength = strData.size();
		if ( (strData.at(0) == '\"' && strData.at (szLength - 1) == '\"') ||
			 (strData.at(0) == '\'' && strData.at (szLength - 1) == '\'')) {
			strIfMatch->assign(strData.substr( 1, szLength - 2));
		}

		goto exit;
	}

	hr = HrGetHeaderValue("If", &strData);
	if (hr == hrSuccess) {
		ulStart = strData.find("[") + 1;
		ulEnd = strData.find("]");
		strIfMatch->assign(strData.substr(ulStart, ulEnd - ulStart));

		goto exit;
	}

exit:
	return hr;
}

/**
 * Returns Charset of the request
 * @param[out]	strCharset	Return string Charset of the request
 *
 * @return		HRESULT
 * @retval		MAPI_E_NOT_FOUND	No charset set in request
 */
HRESULT Http::HrGetCharSet(std::string *strCharset)
{
	HRESULT hr = hrSuccess;

	if(!m_strCharSet.empty())
		strCharset->assign(m_strCharSet);
	else
		hr = MAPI_E_NOT_FOUND;

	return hr;
}

/**
 * Returns the Destination value found in the header
 * 
 * Specifies the destination of entry in MOVE request,
 * to move mapi message from one folder to another
 * for eg.
 *
 * Destination: https://zarafa.com:8080/caldav/USER/FOLDER-ID/ENTRY-GUID.ics
 *
 * @param[out]	strDestination	Return string destination of the request
 *
 * @return		HRESULT
 * @retval		MAPI_E_NOT_FOUND	No destination set in request
 */
HRESULT Http::HrGetDestination(std::wstring *wstrDestination)
{
	HRESULT hr = hrSuccess;
	std::string strHost;
	std::string strDest;

	// @todo what if destination host is different than this host?
	hr = HrGetHeaderValue("Host", &strHost);
	if(hr != hrSuccess)
		goto exit;
	
	hr = HrGetHeaderValue("Destination", &strDest);
	if (hr != hrSuccess)
		goto exit;

	strDest.substr(strHost.length(), strDest.length() - strHost.length());

	if (!strDest.empty())
		X2W(strDest, wstrDestination);
	else
		hr = MAPI_E_NOT_FOUND;
exit:
	return hr;
}

/**
 * Reads request body from the channel
 * @return		HRESULT
 * @retval		MAPI_E_NOT_FOUND	Empty body
 */
HRESULT Http::HrReadBody()
{
	HRESULT hr = hrSuccess;
	int ulContLength;
	std::string strLength;

	// find the Content-Length
	if (HrGetHeaderValue("Content-Length", &strLength) != hrSuccess)
		return MAPI_E_NOT_FOUND;

	ulContLength = atoi((char*)strLength.c_str());
	if (ulContLength <= 0)
		return MAPI_E_NOT_FOUND;

	hr = m_lpChannel->HrReadBytes(&m_strReqBody, ulContLength);
	if (m_lpLogger->Log(EC_LOGLEVEL_DEBUG) && !m_strUser.empty())
		m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Request body:\n%s\n", m_strReqBody.c_str());
	
	return hr;
}

/**
 * Check for errors in the http request
 * @return		HRESULT
 * @retval		MAPI_E_INVALID_PARAMETER	Unsupported http request
 */
HRESULT Http::HrValidateReq()
{
	HRESULT hr = hrSuccess;
	char *lpszMethods[] = {"ACL","GET","HEAD","POST","PUT","DELETE","OPTIONS","PROPFIND","REPORT","MKCALENDAR" ,"PROPPATCH" ,"MOVE", NULL};
	bool bFound = false;
	int i = 0;

	if (m_strMethod.empty()) {
		hr = MAPI_E_INVALID_PARAMETER;
		m_lpLogger->Log(EC_LOGLEVEL_ERROR, "HTTP request method is empty: %08X", hr);
		goto exit;
	}
	
	if (!parseBool(m_lpConfig->GetSetting("enable_ical_get")) && m_strMethod == "GET") {
		hr = MAPI_E_NO_ACCESS;
		m_lpLogger->Log(EC_LOGLEVEL_INFO, "Denying iCalendar GET since it is disabled");
		goto exit;
	}

	for (i = 0; lpszMethods[i] != NULL; i++) {
		if (m_strMethod.compare(lpszMethods[i]) == 0) {
			bFound = true;
			break;
		}
	}

	if (bFound == false) {
		hr = MAPI_E_INVALID_PARAMETER;
		m_lpLogger->Log(EC_LOGLEVEL_ERROR, "HTTP request '%s' not implemented: %08X", m_strMethod.c_str(), hr);
		goto exit;
	}

	// validate authentication data
	if (m_strUser.empty() || m_strPass.empty())
	{
		// hr still success, since http request is valid
		m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Request missing authorization data");
	}

exit:
	return hr;
}

/**
 * Sets keep-alive time for the http response
 *
 * @param[in]	ulKeepAlive		Numerical value set as keep-alive time of http connection
 * @return		HRESULT			Always set as hrSuccess
 */
HRESULT Http::HrSetKeepAlive(int ulKeepAlive)
{
	m_ulKeepAlive = ulKeepAlive;
	return hrSuccess;
}

/**
 * Flush all headers and body to client(i.e Send all data to client)
 *
 * Sends the data in chunked http if the data is large and client uses http 1.1
 * Example of Chunked http
 * 1A[CRLF]					- size in hex
 * xxxxxxxxxxxx..[CRLF]		- data
 * 1A[CRLF]					- size in hex
 * xxxxxxxxxxxx..[CRLF]		- data
 * 0[CRLF]					- end of response
 * [CRLF]
 *
 * @return	HRESULT
 * @retval	MAPI_E_END_OF_SESSION	States that client as set connection type as closed
 */
HRESULT Http::HrFinalize()
{
	HRESULT hr = hrSuccess;

	HrResponseHeader("Content-Length", stringify(m_strRespBody.length()));

	// force chunked http for long size response, should check version >= 1.1 to disable chunking
	if (m_strRespBody.size() < HTTP_CHUNK_SIZE || m_strHttpVer.compare("1.1") != 0)
	{
		hr = HrFlushHeaders();
		if (hr != hrSuccess && hr != MAPI_E_END_OF_SESSION)
			goto exit;
		
		if (!m_strRespBody.empty()) {
			m_lpChannel->HrWriteString(m_strRespBody);
			if (m_lpLogger->Log(EC_LOGLEVEL_DEBUG))
				m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Response body:\n%s", m_strRespBody.c_str());
		}
	}
	else 
	{
		const char *lpstrBody = m_strRespBody.data();
		char lpstrLen[10];
		std::string::size_type szBodyLen = m_strRespBody.size();	// length of data to be sent to the client
		std::string::size_type szBodyWritten = 0;					// length of data sent to client
		unsigned int szPart = HTTP_CHUNK_SIZE;						// default lenght of chunk data to be written

		HrResponseHeader("Transfer-Encoding", "chunked");

		hr = HrFlushHeaders();
		if (hr != hrSuccess && hr != MAPI_E_END_OF_SESSION)
			goto exit;

		while (szBodyWritten < szBodyLen)
		{
			if ((szBodyWritten + HTTP_CHUNK_SIZE) > szBodyLen)
				szPart = szBodyLen - szBodyWritten;				// change length of data for last chunk

			// send hex length of data and data part
			snprintf(lpstrLen, sizeof(lpstrLen), "%X", szPart);
			m_lpChannel->HrWriteLine(lpstrLen);
			m_lpChannel->HrWriteLine((char*)lpstrBody, szPart);

			szBodyWritten += szPart;
			lpstrBody += szPart;
		}

		// end of response
		snprintf(lpstrLen, 10, "0\r\n");
		m_lpChannel->HrWriteLine(lpstrLen);
		// just the first part of the body in the log. header shows it's chunked.
		m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "%s", m_strRespBody.c_str());
	}

exit:
	return hr;
}

/**
 * Sets http response headers
 * @param[in]	ulCode			Http header status code
 * @param[in]	strResponse		Http header status string
 * 
 * @return		HRESULT
 * @retval		MAPI_E_CALL_FAILED	The status is already set
 * 
 */
HRESULT Http::HrResponseHeader(unsigned int ulCode, std::string strResponse)
{
	HRESULT hr = hrSuccess;

	m_ulRetCode = ulCode;
	
	// do not set headers if once set
	if (m_strRespHeader.empty()) 
		m_strRespHeader = "HTTP/1.1 " + stringify(ulCode) + " " + strResponse;
	else
		hr = MAPI_E_CALL_FAILED;

	return hr;
}

/**
 * Adds response header to the list of headers
 * @param[in]	strHeader	Name of the header eg. Connection, Host, Date
 * @param[in]	strValue	Value of the header to be set
 * @return		HRESULT		Always set to hrSuccess
 */
HRESULT Http::HrResponseHeader(std::string strHeader, std::string strValue)
{
	HRESULT hr = hrSuccess;
	std::string header;

	header = strHeader + ": " + strValue;
	m_lstHeaders.push_back(header);

	return hr;
}

/**
 * Add string to http response body
 * @param[in]	strResponse		The string to be added to http response body
 * return		HRESULT			Always set to hrSuccess
 */
HRESULT Http::HrResponseBody(std::string strResponse)
{
	HRESULT hr = hrSuccess;

	m_strRespBody += strResponse;
	// data send in HrFinalize()

	return hr;
}

/**
 * Request authorization information from the client
 *
 * @param[in]	strMsg	Message to be shown to the client
 * @return		HRESULT 
 */
HRESULT Http::HrRequestAuth(std::string strMsg)
{
	HRESULT hr = hrSuccess;

	hr = HrResponseHeader(401, "Unauthorized");
	if (hr != hrSuccess)
		goto exit;

	strMsg = "Basic realm=\"" + strMsg + "\"";
	hr = HrResponseHeader("WWW-Authenticate", strMsg);
	if (hr != hrSuccess)
		goto exit;

exit:
	return hr;
}

/**
 * Write all http headers to ECChannel
 * @retrun	HRESULT
 * @retval	MAPI_E_END_OF_SESSION	If Connection type is set to close then the mapi session is ended
 */
HRESULT Http::HrFlushHeaders()
{
	HRESULT hr = hrSuccess;
	std::list<std::string>::iterator h;
	std::string strOutput;
	char lpszChar[128];
	time_t tmCurrenttime = time(NULL);
	std::string strConnection;

	HrGetHeaderValue("Connection", &strConnection);

	// Add misc. headers
	HrResponseHeader("Server","Zarafa");
	strftime(lpszChar, 127, "%a, %d %b %Y %H:%M:%S GMT", gmtime(&tmCurrenttime));
	HrResponseHeader("Date", lpszChar);
	if (m_ulKeepAlive != 0 && stricmp(strConnection.c_str(), "keep-alive") == 0) {
		HrResponseHeader("Connection", "Keep-Alive");
		HrResponseHeader("Keep-Alive", stringify(m_ulKeepAlive, false));
	}
	else
	{
		HrResponseHeader("Connection", "close");
		hr = MAPI_E_END_OF_SESSION;
	}

	// create headers packet
	ASSERT(m_ulRetCode != 0);
	if (m_ulRetCode == 0)
		HrResponseHeader(500, "Request handled incorrectly");

	m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "> " + m_strRespHeader);
	strOutput += m_strRespHeader + "\r\n";
	m_strRespHeader.clear();
	m_ulRetCode = 0;

	for (h = m_lstHeaders.begin(); h != m_lstHeaders.end(); h++) {
		m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "> " + *h);
		strOutput += *h + "\r\n";
	}
	m_lstHeaders.clear();

	//as last line has a CRLF. The HrWriteLine adds one more CRLF.
	//this means the End of headder.
	m_lpChannel->HrWriteLine(strOutput);

	return hr;
}

HRESULT Http::X2W(const std::string &strIn, std::wstring *lpstrOut)
{
	const char *lpszCharset = (m_strCharSet.empty() ? "UTF-8" : m_strCharSet.c_str());
	return TryConvert(m_converter, strIn, rawsize(strIn), lpszCharset, *lpstrOut);
}

HRESULT Http::HrGetHeaderValue(const std::string &strHeader, std::string *strValue)
{
	std::map<std::string, std::string>::iterator iHeader = mapHeaders.find(strHeader);
	if (iHeader == mapHeaders.end())
		return MAPI_E_NOT_FOUND;
	*strValue = iHeader->second;
	return hrSuccess;
}

