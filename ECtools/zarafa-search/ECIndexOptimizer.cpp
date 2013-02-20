/*
 * Copyright 2005 - 2013  Zarafa B.V.
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
#include "ECIndexOptimizer.h"
#include "ECIndexFactory.h"
#include "Util.h"
#include "boost_compat.h"

#include <boost/filesystem.hpp>

#include "CommonUtil.h"
#include "mapi_ptr.h"
#include <set>

namespace bfs = boost::filesystem;


bool operator <(const GUID &left, const GUID &right)
{
	return memcmp(&left, &right, sizeof(GUID)) < 0;
}


ECIndexOptimizer::ECIndexOptimizer()
{
}

ECIndexOptimizer::~ECIndexOptimizer()
{
}

void* ECIndexOptimizer::Run(void* param)
{
	ECThreadData *lpThreadData = (ECThreadData*)param;
	HRESULT hr = hrSuccess;
	const char *szPath;
	ECIndexDB *lpIndex = NULL;
	GUID guidServer, guidStore;
	bfs::path		indexdir;
	bfs::directory_iterator ilast;
	time_t now;
	struct tm local;
	int until;
	std::list<std::string> listIndexFiles;
	GUID guidMBServer;
	std::set<GUID> setStoreGuid;

	// Fatal so the user knows the reason the zarafa-search is hogging the CPU
	lpThreadData->lpLogger->Log(EC_LOGLEVEL_FATAL, "Starting to cleanup and optimize index files");

	until = atoi(lpThreadData->lpConfig->GetSetting("optimize_stop"));

	szPath = lpThreadData->lpConfig->GetSetting("index_path");
	indexdir = szPath;
	if (!bfs::exists(indexdir)) {
		lpThreadData->lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to optimize indexes: path %s not found or access denied", szPath);
		return NULL;
	}

	// Get the indexed stores from disk
	for (bfs::directory_iterator index(indexdir); index != ilast; index++) {
		std::string strFilename;

		strFilename = filename_from_path(index->path());

		if (is_directory(index->status()))
			continue;

		listIndexFiles.push_back(strFilename);
	}

	// Get stores table from the server
	hr = GetServerMailboxList(lpThreadData, guidMBServer, setStoreGuid);
	if (hr != hrSuccess) {
		lpThreadData->lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to create a mailbox list, cleanup of indexes (temporarily) disabled");
		setStoreGuid.clear();
	} 

	for (std::list<std::string>::iterator iter = listIndexFiles.begin(); !lpThreadData->bShutdown && iter != listIndexFiles.end(); iter++) 
	{
		hr = lpThreadData->lpIndexFactory->GetStoreIdFromFilename(*iter, &guidServer, &guidStore);
		if (hr != hrSuccess)
			continue;

		lpThreadData->lpLogger->Log(EC_LOGLEVEL_DEBUG, "Checking file %s", (*iter).c_str());

		if (!setStoreGuid.empty() && guidServer == guidMBServer && setStoreGuid.find(guidStore) == setStoreGuid.end() ) {
			lpThreadData->lpLogger->Log(EC_LOGLEVEL_FATAL, "Detect removed store %s, start to cleanup index",  bin2hex(sizeof(GUID), (unsigned char*)&guidStore).c_str() );

			hr = lpThreadData->lpIndexFactory->RemoveIndexDB(guidServer, guidStore);
			if (hr == MAPI_E_BUSY)
				 lpThreadData->lpLogger->Log(EC_LOGLEVEL_WARNING, "Unable to cleanup index for store %s, index is use", bin2hex(sizeof(GUID), (unsigned char*)&guidStore).c_str() );
			else if (hr != hrSuccess)
				lpThreadData->lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to cleanup index for store %s: 0x%08X", bin2hex(sizeof(GUID), (unsigned char*)&guidStore).c_str(), hr);

			continue;
		}

		// open through index factory to get a shared object
		hr = lpThreadData->lpIndexFactory->GetIndexDB(&guidServer, &guidStore, false, false, &lpIndex);
		if (hr != hrSuccess)
			continue;

		lpIndex->Optimize();

		lpThreadData->lpIndexFactory->ReleaseIndexDB(lpIndex);

		now = time(NULL);
		localtime_r(&now, &local);
		if (local.tm_hour > until)
			break;
	}

	// Fatal so the user knows how we're done.
	lpThreadData->lpLogger->Log(EC_LOGLEVEL_FATAL, "Stopping index optimization");
	return NULL;
}

HRESULT ECIndexOptimizer::GetServerMailboxList(ECThreadData *lpThreadData, GUID &guidMBServer, std::set<GUID> &setStoreGuid)
{
	HRESULT hr = hrSuccess;
	ECLogger *lpLogger = lpThreadData->lpLogger;
	ECConfig *lpConfig = lpThreadData->lpConfig;
	MAPISessionPtr ptrSession;
	MsgStorePtr ptrAdminStore;
	SPropValuePtr ptrPropSign;
	ExchangeManageStorePtr ptrEMS;
	MAPITablePtr ptrTable;
	SRowSetPtr ptrRows;
	SizedSPropTagArray(1, sptaProps) = { 1, { PR_STORE_RECORD_KEY } };

	hr = HrOpenECSession(&ptrSession, L"SYSTEM", L"", lpConfig->GetSetting("server_socket"), 0, lpConfig->GetSetting("sslkey_file"),  lpConfig->GetSetting("sslkey_pass"));
	if(hr != hrSuccess) {
		lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to contact zarafa server %s: 0x%08X. Will retry.", lpConfig->GetSetting("server_socket"), hr);
		goto exit;
	}

	hr = HrOpenDefaultStore(ptrSession, &ptrAdminStore);
	if(hr != hrSuccess) {
		lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to open system admin store: 0x%08X", hr);
		goto exit;
	}

	hr = HrGetOneProp(ptrAdminStore, PR_MAPPING_SIGNATURE, &ptrPropSign);
	if(hr != hrSuccess) {
		lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to get server guid: 0x%08X", hr);
		goto exit;
	}

	hr = ptrAdminStore->QueryInterface(IID_IExchangeManageStore, (void **)&ptrEMS);
	if(hr != hrSuccess)
		goto exit;

	hr = ptrEMS->GetMailboxTable(L"", &ptrTable, 0);
	if(hr != hrSuccess) {
		lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to get mailbox table: 0x%08X", hr);
		goto exit;
	}

	hr = ptrTable->SetColumns((LPSPropTagArray)&sptaProps, 0);
	if(hr != hrSuccess) {
		lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to set columns for mailbox table: 0x%08X", hr);
		goto exit;
	}

	while(1) {
		hr = ptrTable->QueryRows(20, 0, &ptrRows);
		if(hr != hrSuccess) {
			lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to get rows from mailbox table: 0x%08X", hr);
			goto exit;
		}

		if(ptrRows.empty())
			break;

		for(unsigned int i = 0; i < ptrRows.size(); i++) {
			if (ptrRows[i].lpProps[0].Value.bin.lpb)
				setStoreGuid.insert(*((GUID*)ptrRows[i].lpProps[0].Value.bin.lpb));
		}
	}

	guidMBServer = *(GUID*)ptrPropSign->Value.bin.lpb;
exit:
	return hr;
}
