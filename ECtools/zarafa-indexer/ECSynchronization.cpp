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

#include <platform.h>

#include <algorithm>

#include <mapi.h>
#include <mapix.h>
#include <mapiutil.h>
#include <mapidefs.h>
#include <mapiguid.h>

#include <edkguid.h>

#include <CommonUtil.h>
#include <ECABEntryID.h>
#include <ECGuid.h>
#include <IECExportAddressbookChanges.h>
#include <restrictionutil.h>
#include <ECLuceneIndexer.h>
#include <ECChangeData.h>
#include <ECFileIndex.h>
#include <ECIndexerUtil.h>
#include <stringutil.h>

#include "ECIndexer.h"
#include "ECIndexerData.h"
#include "ECSynchronization.h"
#include "ECSynchronizationImportChanges.h"
#include "zarafa-indexer.h"

#include "mapi_ptr.h"
#include "helpers/mapiprophelper.h"

using namespace za::helpers;

ECSynchronization::ECSynchronization(ECThreadData *lpThreadData, ECIndexerData *lpIndexerData)
{
	m_lpThreadData = lpThreadData;
	m_lpIndexerData = lpIndexerData;
	m_lpChanges = NULL;

	m_bMerged = FALSE;
	m_bCompleted = FALSE;
}

ECSynchronization::~ECSynchronization()
{
	if (m_lpChanges)
		m_lpChanges->Release();
}

HRESULT ECSynchronization::Create(ECThreadData *lpThreadData, ECIndexerData *lpIndexerData, ECSynchronization **lppSynchronization)
{
	HRESULT hr = hrSuccess;
	ECSynchronization *lpSynchronization = NULL;

	try {
		lpSynchronization = new ECSynchronization(lpThreadData, lpIndexerData);
	}
	catch (...) {
		lpSynchronization = NULL;
	}
	if (!lpSynchronization) {
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}

	lpSynchronization->AddRef();

	if (lppSynchronization)
		*lppSynchronization = lpSynchronization;

exit:
	if ((hr != hrSuccess) && lpSynchronization)
		lpSynchronization->Release();

	return hr;
}

HRESULT ECSynchronization::GetAddressBookChanges(IMsgStore *lpStore, LPSTREAM lpSyncBase)
{
	HRESULT hr = hrSuccess;
	IECExportAddressbookChanges *lpExporter = NULL;
	IECImportAddressbookChanges *lpImporter = NULL;
	ULONG ulSteps = 0;
	ULONG ulStep = 0;

	hr = CreateChangeData(m_bMerged);
	if (hr != hrSuccess)
		goto exit;

	hr = lpStore->OpenProperty(PR_CONTENTS_SYNCHRONIZER, &IID_IECExportAddressbookChanges, 0, 0, (LPUNKNOWN *)&lpExporter);
	if (hr != hrSuccess)
		goto exit;

	hr = ECSynchronizationImportChanges::Create(m_lpThreadData, 0, NULL, m_lpChanges,
												IID_IECImportAddressbookChanges, (LPVOID *)&lpImporter);
	if (hr != hrSuccess)
		goto exit;

	hr = lpExporter->Config(lpSyncBase, 0, lpImporter);
	if (hr != hrSuccess)
		goto exit;

	do {
		hr = lpExporter->Synchronize(&ulSteps, &ulStep);
	} while ((hr == SYNC_W_PROGRESS) && !m_lpThreadData->bShutdown);

	if (hr != hrSuccess) {
		if (hr != SYNC_W_PROGRESS)
			goto exit;

		/*
		 * Synchronization was cancelled due to shutdown, however since we
		 * have partially synchronized this folder we cannot break off completely,
		 * continue without errors in order to clean everything up so we can
		 * start from where we left of next time.
		 */
		hr = hrSuccess;
	}

	hr = lpExporter->UpdateState(lpSyncBase);
	if (hr != hrSuccess)
		goto exit;

	if (!m_bMerged)
		m_bCompleted = TRUE;

exit:
	if (lpImporter)
		lpImporter->Release();

	if (lpExporter)
		lpExporter->Release();

	return hr;
}

HRESULT ECSynchronization::GetHierarchyChanges(ECEntryData *lpEntryData, IMsgStore *lpMsgStore, IMAPIFolder *lpRootFolder)
{
	HRESULT hr = hrSuccess;
	IExchangeExportChanges *lpExporter = NULL;
	IExchangeImportHierarchyChanges *lpImporter = NULL;
	ULONG ulCreate = 0;
	ULONG ulChange = 0;
	ULONG ulDelete = 0;
	ULONG ulSteps = 0;
	ULONG ulStep = 0;
	ECChanges *lpChanges = new ECChanges();
	SizedSPropTagArray(4, sptaExclude) = {
		4,
		{
			PR_LAST_MODIFICATION_TIME,
			PR_CHANGE_KEY,
			PR_PREDECESSOR_CHANGE_LIST,
			PR_DISPLAY_NAME,
		}
	};

	m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_INFO, "Indexing of store '%ls': Synchronizing Hierarchy",
								  lpEntryData->m_strUserName.c_str());

	hr = CreateChangeData(m_bMerged);
	if (hr != hrSuccess)
		goto exit;

	hr = lpRootFolder->OpenProperty(PR_HIERARCHY_SYNCHRONIZER, &IID_IExchangeExportChanges, 0, 0, (LPUNKNOWN *)&lpExporter);
	if (hr != hrSuccess)
		goto exit;

	hr = ECSynchronizationImportChanges::Create(m_lpThreadData, 0, NULL, m_lpChanges,
												IID_IExchangeImportHierarchyChanges, (LPVOID *)&lpImporter);
	if (hr != hrSuccess)
		goto exit;

	hr = lpExporter->Config(lpEntryData->m_lpHierarchySyncBase, SYNC_ASSOCIATED | SYNC_NORMAL | SYNC_NEW_MESSAGE, lpImporter, NULL,
							/*(LPSPropTagArray)&includeProps*/ NULL, (LPSPropTagArray)&sptaExclude, 0);
	if (hr != hrSuccess)
		goto exit;

	do {
		hr = lpExporter->Synchronize(&ulSteps, &ulStep);
	} while ((hr == SYNC_W_PROGRESS) && !m_lpThreadData->bShutdown);

	if (hr != hrSuccess) {
		if (hr != SYNC_W_PROGRESS)
			goto exit;

		/*
		 * Synchronization was cancelled due to shutdown, however since we
		 * have partially synchronized this folder we cannot break off completely,
		 * continue without errors in order to clean everything up so we can
		 * start from where we left of next time.
		 */
		hr = hrSuccess;
	}

	hr = lpExporter->UpdateState(lpEntryData->m_lpHierarchySyncBase);
	if (hr != hrSuccess)
		goto exit;
		
	m_lpChanges->Size(&ulCreate, &ulChange, &ulDelete);
	
	hr = m_lpChanges->ClaimChanges(lpChanges);
	if(hr != hrSuccess)
		goto exit;
	
	hr = LoadUserFolderChanges(lpChanges, lpEntryData, lpRootFolder);
	if(hr != hrSuccess)
		goto exit;
		
	if (!m_bMerged)
		m_bCompleted = TRUE;

exit:
	m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_INFO, "Indexing of store '%ls': Synchronizing Hierarchy - %d creations, %d changes, %d deletions %s",
								  lpEntryData->m_strUserName.c_str(), ulCreate, ulChange, ulDelete, (hr == hrSuccess) ? "succeeded" : "failed");

	if (lpChanges)
		delete lpChanges;

	if (lpImporter)
		lpImporter->Release();

	if (lpExporter)
		lpExporter->Release();

	return hr;
}

HRESULT ECSynchronization::GetContentsChanges(ECEntryData *lpEntryData, IMsgStore *lpMsgStore, IMAPIFolder *lpRootFolder, IMAPISession *lpAdminSession)
{
	HRESULT hr = hrSuccess;
	IMAPIFolder *lpFolder = NULL;
	ULONG ulObjType = 0;

	m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_INFO, "Indexing of store '%ls': Synchronizing Content",
								  lpEntryData->m_strUserName.c_str());

	/* No folders, no contents */
	if (lpEntryData->m_lFolders.empty())
		goto exit;

	hr = StartMergedChanges();
	if (hr != hrSuccess)
		goto exit;

	for (folderdata_list_t::iterator iter = lpEntryData->m_lFolders.begin(); iter != lpEntryData->m_lFolders.end(); iter++) {
		hr = lpRootFolder->OpenEntry((*iter)->m_sFolderEntryId.cb, (LPENTRYID)(*iter)->m_sFolderEntryId.lpb,
									 &IID_IMAPIFolder, 0, &ulObjType, (LPUNKNOWN *)&lpFolder);
		if (hr != hrSuccess) {
			if (hr != MAPI_E_NOT_FOUND && hr != MAPI_E_INVALID_ENTRYID)
				goto exit;

			/* Folder has been removed, this entry will be removed during synchronization later */
			hr = hrSuccess;
			continue;
		}

		hr = GetContentsChanges(lpEntryData, (*iter), lpMsgStore, lpFolder, lpAdminSession);
		if (hr != hrSuccess) {
			m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_ERROR, "Synchronization of folder '%ls' failed",
										  (*iter)->m_strFolderName.c_str());
			goto exit;
		}
		
		/* Check if we should exit now, but make sure that all collected changes are synchronized */
		if (m_lpThreadData->bShutdown)
			break;

		if (lpFolder)
			lpFolder->Release();
		lpFolder = NULL;
	}

	hr = StopMergedChanges();
	if (hr != hrSuccess)
		goto exit;

exit:

	if (lpFolder)
		lpFolder->Release();

	return hr;
}

HRESULT ECSynchronization::GetContentsChanges(ECEntryData *lpEntryData, ECFolderData *lpFolderData, IMsgStore *lpMsgStore, IMAPIFolder *lpFolder, IMAPISession *lpAdminSession)
{
	HRESULT hr = hrSuccess;
	IExchangeExportChanges *lpExporter = NULL;
	IExchangeImportContentsChanges *lpImporter = NULL;
	ECLuceneIndexer *lpLucene = NULL;
	ECChanges *lpChanges = new ECChanges();
	ULONG ulCreate = 0;
	ULONG ulChange = 0;
	ULONG ulDelete = 0;
	
	ULONG ulSteps = 0;
	ULONG ulStep = 0;
	ULONG cValues = 0;
	ULONG ulLastStatsTime = time(NULL);
	ULONG ulStartTime = ulLastStatsTime;
	ULONG ulTotalChange = 0;
	ULONG ulTotalBytes = 0;
	
	LPSPropValue lpProps = NULL;
	
	SizedSPropTagArray(3, sptaFolderProps) = {3, { PR_RECORD_KEY, PR_STORE_RECORD_KEY, PR_MAPPING_SIGNATURE } };
	
	SPropTagArrayPtr ptrIncludeProps;

	SizedSPropTagArray(5, sptaInclude) = {
		5,
		{
			PR_EC_HIERARCHYID,
			PR_EC_PARENT_HIERARCHYID,
			PR_STORE_RECORD_KEY,
			PR_MAPPING_SIGNATURE,
			PR_ENTRYID
		}
	};

	SizedSPropTagArray(5, sptaExclude) = {
		5,
		{
			PR_LAST_MODIFICATION_TIME,
			PR_CHANGE_KEY,
			PR_MESSAGE_FLAGS,
			PR_PREDECESSOR_CHANGE_LIST,
			PR_CREATION_TIME
		}
	};
	
	hr = ECLuceneIndexer::Create(m_lpThreadData, lpMsgStore, &lpLucene);
	if (hr != hrSuccess)
		goto exit;
		
	hr = lpFolder->GetProps((LPSPropTagArray)&sptaFolderProps, 0, &cValues, &lpProps);
	if(hr != hrSuccess) // We need all properties to be ok, no warnings
		goto exit;
		
	// We want a PR_EC_PARENT_HIERARCHYID, so abuse PR_RECORD_KEY to find hierarchyid of folder ... *HACKISH*
	ASSERT(lpProps[0].Value.bin.cb == sizeof(unsigned int));
	lpProps[0].ulPropTag = PR_EC_PARENT_HIERARCHYID;
	lpProps[0].Value.ul = *(unsigned int *)lpProps[0].Value.bin.lpb;
	
	hr = MAPIPropHelper::GetArchiverProps(MAPIPropPtr(lpMsgStore, true), (LPSPropTagArray)&sptaInclude, &ptrIncludeProps);
	if (hr != hrSuccess)
		goto exit;

	hr = CreateChangeData(m_bMerged);
	if (hr != hrSuccess)
		goto exit;

	hr = lpFolder->OpenProperty(PR_CONTENTS_SYNCHRONIZER, &IID_IExchangeExportChanges, 0, 0, (LPUNKNOWN *)&lpExporter);
	if (hr != hrSuccess)
		goto exit;

	// The importer writes changes into m_lpChanges
	hr = ECSynchronizationImportChanges::Create(m_lpThreadData, cValues, lpProps, m_lpChanges,
												IID_IExchangeImportContentsChanges, (LPVOID *)&lpImporter);
	if (hr != hrSuccess)
		goto exit;

	hr = lpExporter->Config(lpFolderData->m_lpContentsSyncBase, SYNC_ASSOCIATED | SYNC_NORMAL | SYNC_NEW_MESSAGE | SYNC_LIMITED_IMESSAGE, lpImporter, NULL,
							ptrIncludeProps, (LPSPropTagArray)&sptaExclude, 0);
	if (hr != hrSuccess)
		goto exit;

	while(!m_lpThreadData->bShutdown) {
		HRESULT hrSync = lpExporter->Synchronize(&ulSteps, &ulStep);
		
		m_lpChanges->CurrentSize(&ulCreate, &ulChange, &ulDelete);
		
		m_lpChanges->ClaimChanges(lpChanges);
		
		// Synchronize has written changes to m_lpChanges, process them now
		hr = lpLucene->IndexEntries(lpChanges->lCreate, lpChanges->lChange, lpChanges->lDelete, &ulTotalBytes);
		if (hr != hrSuccess)
			goto exit;

		ulTotalChange += ulCreate + ulChange;
			
		if (hrSync != SYNC_W_PROGRESS)
			break;
			
		if (ulLastStatsTime < time(NULL) - 10) {
			unsigned int secs = time(NULL) - ulLastStatsTime;
			m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_INFO, "%.1f%% (%d of %d) of folder processed: %s in %d messages (%s/sec, %.1f messages/sec)", ((float)ulStep*100)/ulSteps,ulStep,ulSteps,str_storage(ulTotalBytes, false).c_str(), ulTotalChange, str_storage(ulTotalBytes/secs, false).c_str(), (float)ulTotalChange/secs);
			ulLastStatsTime = time(NULL);
		}
	};

	if (hr != hrSuccess) {
		if (hr != SYNC_W_PROGRESS)
			goto exit;

		/*
		 * Synchronization was cancelled due to shutdown, however since we
		 * have partially synchronized this folder we cannot break off completely,
		 * continue without errors in order to clean everything up so we can
		 * start from where we left of next time.
		 */
		hr = hrSuccess;
	}

	hr = lpExporter->UpdateState(lpFolderData->m_lpContentsSyncBase);
	if (hr != hrSuccess)
		goto exit;

	hr = UpdateFolderSyncBase(lpEntryData, lpFolderData);
	if (hr != hrSuccess)
		goto exit;

	if (!m_bMerged)
		m_bCompleted = TRUE;
		
	m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_INFO, "Processed folder with %d changes (%s) in %d seconds", ulTotalChange, str_storage(ulTotalBytes, false).c_str(), time(NULL) - ulStartTime);

exit:
	if (lpChanges)
		delete lpChanges;
		
	if (lpLucene)
		delete lpLucene;
		
	if (lpImporter)
		lpImporter->Release();

	if (lpExporter)
		lpExporter->Release();

	return hr;
}

HRESULT ECSynchronization::StartMergedChanges()
{
	m_bMerged = TRUE;
	m_bCompleted = FALSE;

	/* Force reloading of ECChangeData */
	if (m_lpChanges)
		m_lpChanges->Release();
	m_lpChanges = NULL;

	return hrSuccess;
}

HRESULT ECSynchronization::StopMergedChanges()
{
	m_bMerged = FALSE;
	m_bCompleted = TRUE;
	return hrSuccess;
}

HRESULT ECSynchronization::CreateChangeData(BOOL bMerge)
{
	HRESULT hr = hrSuccess;
	
	if (m_bCompleted) {
		if (m_lpChanges)
			m_lpChanges->Release();
		m_lpChanges = NULL;
	}
								
	if (!m_lpChanges) {
		hr = ECChangeData::Create(&m_lpChanges);
		if (hr != hrSuccess)
			goto exit;
	}
												
	m_bCompleted = FALSE;

exit:
	return hr;
}

HRESULT ECSynchronization::GetNextChanges(ECChanges **lppChanges)
{
	HRESULT hr = hrSuccess;
	ECChanges *lpChanges = NULL;

	if (!m_lpChanges) {
		if (m_bCompleted)
			hr = MAPI_E_CANCEL;
		else
			hr = MAPI_E_NOT_INITIALIZED;
		goto exit;
	}

	try {
		lpChanges = new ECChanges();
	}
	catch (...) {
		lpChanges = NULL;
	}
	if (!lpChanges) {
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}

	hr = m_lpChanges->ClaimChanges(lpChanges);
	if (hr != hrSuccess)
		goto exit;

	lpChanges->Sort();
	lpChanges->Unique();
	lpChanges->Filter();
										
	if (m_bCompleted) {
		m_lpChanges->Release();
		m_lpChanges = NULL;
	}

	if (lppChanges)
		*lppChanges = lpChanges;

exit:
	if ((hr != hrSuccess) && lpChanges)
		delete lpChanges;

	return hr;
}

struct findfolder_if {
	sourceid_list_t::value_type m_sValue;

	findfolder_if(sourceid_list_t::value_type sValue)
		: m_sValue(sValue) {}

	bool operator()(folderdata_list_t::value_type &entry)
	{
		return m_sValue->lpProps[0].Value.bin == entry->m_sFolderEntryId;
	}
};


HRESULT ECSynchronization::LoadUserFolderChanges(ECChanges *lpChanges, ECEntryData *lpEntry, IMAPIFolder *lpRootFolder)
{
	HRESULT hr = hrSuccess;
	IMAPITable *lpTable = NULL;
	LPSRestriction lpRestrict = NULL;
	LPSRowSet lpRows = NULL;
	sourceid_list_t::iterator iter;
	SizedSPropTagArray(3, sFolderProps) = { 3, { PR_SOURCE_KEY, PR_ENTRYID, PR_DISPLAY_NAME } };

	if (!lpChanges->lDelete.empty()) {
		for (iter = lpChanges->lDelete.begin(); iter != lpChanges->lDelete.end(); iter++) {
			folderdata_list_t::iterator entry = find_if(lpEntry->m_lFolders.begin(), lpEntry->m_lFolders.end(), findfolder_if(*iter));
			if (entry !=lpEntry->m_lFolders.end()) {
				(*entry)->Release();
				lpEntry->m_lFolders.erase(entry);
			}
		}
	}

	if (!lpChanges->lCreate.empty()) {
		hr = lpRootFolder->GetHierarchyTable(CONVENIENT_DEPTH, &lpTable);
		if (hr != hrSuccess)
			goto exit;

		CREATE_RESTRICTION(lpRestrict);
		CREATE_RES_OR(lpRestrict, lpRestrict, lpChanges->lCreate.size());
		lpRestrict->res.resOr.cRes = 0;

		for (iter = lpChanges->lCreate.begin(); iter != lpChanges->lCreate.end(); iter++) {
			/* Cheap copy, no need for allocation/copying */
			DATA_RES_PROPERTY_CHEAP(lpRestrict,
									lpRestrict->res.resOr.lpRes[lpRestrict->res.resOr.cRes],
									RELOP_EQ, (*iter)->lpProps[0].ulPropTag, &(*iter)->lpProps[0]);
			lpRestrict->res.resOr.cRes++;
		}

		hr = lpTable->Restrict(lpRestrict, TBL_BATCH);
		if (hr != hrSuccess)
			goto exit;

		hr = lpTable->SetColumns((LPSPropTagArray)&sFolderProps, TBL_BATCH);
		if (hr != hrSuccess)
			goto exit;

		while (TRUE) {
			hr = lpTable->QueryRows(25, 0, &lpRows);
			if (hr != hrSuccess)
				goto exit;

			if (lpRows->cRows == 0)
				break;

			for (ULONG i = 0; i < lpRows->cRows; i++) {
				hr = LoadUserFolder(lpEntry, lpRows->aRow[i].cValues, lpRows->aRow[i].lpProps);
				if (hr != hrSuccess)
					goto exit;
			}

			if (lpRows)
				FreeProws(lpRows);
			lpRows = NULL;
		}
	}

exit:
	if (lpRows)
		FreeProws(lpRows);

	if (lpRestrict)
		MAPIFreeBuffer(lpRestrict);

	if (lpTable)
		lpTable->Release();

	return hr;
}

HRESULT ECSynchronization::LoadUserFolder(ECEntryData *lpEntry, ULONG ulProps, LPSPropValue lpProps)
{
	HRESULT hr = hrSuccess;
	ECFolderData *lpFolderData = NULL;
	LPSPropValue lpFolderName = NULL;
	LPSPropValue lpSourceKey = NULL;
	LPSPropValue lpEntryId = NULL;
	LPBYTE lpStreamData = NULL;
	ULONG ulStreamSize = 0;
	std::string strTmp;

	hr = ECFolderData::Create(lpEntry, &lpFolderData);
	if (hr != hrSuccess)
		goto exit;

	lpFolderName = PpropFindProp(lpProps, ulProps, PR_DISPLAY_NAME);
	if (!lpFolderName) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	lpFolderData->m_strFolderName = lpFolderName->Value.LPSZ;

	lpSourceKey = PpropFindProp(lpProps, ulProps, PR_SOURCE_KEY);
	if (!lpSourceKey) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}
				
	hr = HrAddEntryData(&lpFolderData->m_sFolderSourceKey, lpEntry, &lpSourceKey->Value.bin);
	if (hr != hrSuccess)
		goto exit;

	lpEntryId = PpropFindProp(lpProps, ulProps, PR_ENTRYID);
	if (!lpEntryId) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	hr = HrAddEntryData(&lpFolderData->m_sFolderEntryId, lpEntry, &lpEntryId->Value.bin);
	if (hr != hrSuccess)
		goto exit;

	strTmp = bin2hex(lpEntryId->Value.bin.cb, lpEntryId->Value.bin.lpb);
	hr = m_lpThreadData->lpFileIndex->GetStoreFolderInfoPath(lpEntry->m_strStorePath, strTmp, &lpFolderData->m_strFolderPath);
	if (hr != hrSuccess)
		goto exit;

	hr = m_lpThreadData->lpFileIndex->GetStoreFolderSyncBase(lpFolderData->m_strFolderPath, &ulStreamSize, &lpStreamData);
	if (hr != hrSuccess)
		goto exit;

	hr = CreateSyncStream(&lpFolderData->m_lpContentsSyncBase, ulStreamSize, lpStreamData);
	if (hr != hrSuccess)
		goto exit;

exit:
	if (lpStreamData)
		MAPIFreeBuffer(lpStreamData);

	if ((hr != hrSuccess) && lpFolderData) {
		lpEntry->m_lFolders.remove(lpFolderData);
		lpFolderData->Release();
	}

	return hr;
}

HRESULT ECSynchronization::UpdateFolderSyncBase(ECEntryData *lpEntryData, ECFolderData *lpFolderData)
{
	HRESULT hr = hrSuccess;
	STATSTG sStat;
	LARGE_INTEGER liPos = {{0, 0}};
	ULONG ulSize = 0;
	LPBYTE lpBuff = NULL;
	ULONG ulBuff = 0;

	hr = CreatePath(lpEntryData->m_strStorePath.c_str());
	if (hr != hrSuccess) {
		m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_INFO, "Unable to create folder '%s' for sync state information: %s", lpEntryData->m_strStorePath.c_str(), strerror(errno));
		goto exit;
	}

	hr = lpFolderData->m_lpContentsSyncBase->Stat(&sStat, STATFLAG_DEFAULT);
	if (hr != hrSuccess)
		goto exit;

	ulSize = sStat.cbSize.LowPart;
	if (sStat.cbSize.HighPart) {
		hr = MAPI_E_TOO_BIG;
		goto exit;
	}

	hr = MAPIAllocateBuffer(ulSize, (LPVOID *)&lpBuff);
	if (hr != hrSuccess)
		goto exit;

	hr = lpFolderData->m_lpContentsSyncBase->Seek(liPos, STREAM_SEEK_SET, NULL);
	if (hr != hrSuccess)
		goto exit;

	hr = lpFolderData->m_lpContentsSyncBase->Read(lpBuff, ulSize, &ulBuff);
	if (hr != hrSuccess)
		goto exit;

	if (ulSize != ulBuff) {
		hr = MAPI_E_CALL_FAILED;
		goto exit;
	}

	m_lpThreadData->lpFileIndex->SetStoreFolderSyncBase(lpFolderData->m_strFolderPath, ulBuff, lpBuff);
	/* Ignore return code from SetStoreFolderSyncBase, we don't care... */

exit:
	if (lpBuff)
		MAPIFreeBuffer(lpBuff);

	return hr;
}

