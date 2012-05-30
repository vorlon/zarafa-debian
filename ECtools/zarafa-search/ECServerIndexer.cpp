/*
 * Copyright 2005 - 2012  Zarafa B.V.
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

#include "ECServerIndexer.h"
#include "ECLogger.h"
#include "ECConfig.h"
#include "ECIndexDB.h"
#include "ECIndexFactory.h"
#include "IStreamAdapter.h"
#include "ECIndexImporter.h"

#include "zarafa-search.h"

#include "pthreadutil.h"
#include "mapi_ptr.h"
#include "CommonUtil.h"

#include <mapi.h>
#include <mapicode.h>
#include <mapitags.h>
#include <edkmdb.h>

#include <stdio.h>

#define WSTR(x) (PROP_TYPE((x).ulPropTag) == PT_UNICODE ? (x).Value.lpszW : L"<Unknown>")

// Define ECImportContentsChangePtr
DEFINEMAPIPTR(ECImportContentsChanges);

ECServerIndexer::ECServerIndexer(ECConfig *lpConfig, ECLogger *lpLogger, ECThreadData *lpThreadData)
{
    m_lpConfig = lpConfig;
    m_lpLogger = lpLogger;
    m_lpThreadData = lpThreadData;
    
    pthread_mutex_init(&m_mutexExit, NULL);
    pthread_cond_init(&m_condExit, NULL);
    m_bExit = false;
    m_bThreadStarted = false;
    
    pthread_mutex_init(&m_mutexTrack, NULL);
    pthread_cond_init(&m_condTrack, NULL);
    m_ulTrack = 0;
    
    m_guidServer = GUID_NULL;
    
    m_ulIndexerState = 0;
}

ECServerIndexer::~ECServerIndexer()
{
    pthread_mutex_lock(&m_mutexExit);
    m_bExit = true;
    pthread_cond_signal(&m_condExit);
    pthread_mutex_unlock(&m_mutexExit);
    
    pthread_join(m_thread, NULL);
}

HRESULT ECServerIndexer::Create(ECConfig *lpConfig, ECLogger *lpLogger, ECThreadData *lpThreadData, ECServerIndexer **lppIndexer)
{
    HRESULT hr = hrSuccess;
    ECServerIndexer *lpIndexer = new ECServerIndexer(lpConfig, lpLogger, lpThreadData);
    
    lpIndexer->AddRef();

    hr = lpIndexer->Start();
    if(hr != hrSuccess)
        goto exit;
    
    *lppIndexer = lpIndexer;
exit:
    if(hr != hrSuccess && lpIndexer)
        lpIndexer->Release();
        
    return hr;
}

/**
 * Start the indexer thread
 */
HRESULT ECServerIndexer::Start()
{
    HRESULT hr = hrSuccess;
    
    if(pthread_create(&m_thread, NULL, ECServerIndexer::ThreadEntry, this) != 0) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to start thread: %s", strerror(errno));
        hr = MAPI_E_CALL_FAILED;
        goto exit;
    }
    
    m_bThreadStarted = true;
    
exit:
    return hr;
}

/**
 * Thread entry point delegate
 */
void *ECServerIndexer::ThreadEntry(void *lpParam)
{
    ECServerIndexer *lpIndexer = (ECServerIndexer *)lpParam;
    
    return (void *)lpIndexer->Thread();
}

/**
 * This is the main work thread that is started for the ECServerIndexer object
 *
 * Lifetime of the thread is close to that of the entire indexer process
 */
HRESULT ECServerIndexer::Thread()
{
    HRESULT hr = hrSuccess;
    MAPISessionPtr lpSession;

    m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Entering main indexer loop");

    while(!m_bExit) {
        // Open an admin session
        if(!lpSession) {
            hr = HrOpenECSession(&lpSession, L"SYSTEM", L"", m_lpConfig->GetSetting("server_socket"));
            if(hr != hrSuccess) {
                m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to contact zarafa server: %08X. Will retry.", hr);
                goto next;
            }
            
            hr = GetServerGUID(lpSession, &m_guidServer);
            if(hr != hrSuccess) {
                m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to get server GUID: %08X", hr);
                goto next;
            }

            // Load the state from disk if we don't have it yet
            hr = LoadState();
            
            if(hr == MAPI_E_NOT_FOUND)
                m_strICSState.clear();
            else if(hr == hrSuccess) {
                m_ulIndexerState = stateRunning;
                m_lpLogger->Log(EC_LOGLEVEL_INFO, "Waiting for changes");
            } else
                goto exit;
        }
            
        if(m_ulIndexerState == stateUnknown) {
            m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Initial state, getting server state");
            hr = GetServerState(lpSession, m_strICSState);
            if(hr != hrSuccess)
                goto exit;
                
            m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Server state after initial exports: %s", bin2hex(m_strICSState).c_str());
            
            // No state, start from the beginning
            m_ulIndexerState = stateBuilding;        
            m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Entering build state, starting index build");
        }
        
        if(m_ulIndexerState == stateBuilding) {
            
            hr = BuildIndexes(lpSession);
            if(hr != hrSuccess) {
                m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Store indexing failed");
                hr = hrSuccess;
                goto next;
            }
                
            m_ulIndexerState = stateRunning;
            m_lpLogger->Log(EC_LOGLEVEL_INFO, "Entering incremental state, waiting for changes");
            
            if(m_bExit)
                goto exit;
        }
        if(m_ulIndexerState == stateRunning) {

            hr = ProcessChanges(lpSession);
            
            if(hr != hrSuccess) {
                m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Change processing failed: %08X", hr);
                hr = hrSuccess;
                goto next;
            }
            
            hr = SaveState();
            if(hr != hrSuccess) {
                m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to save server state. This may cause previous changes to be re-indexed: %08X", hr);
                hr = hrSuccess;
            }
        }

next:
        pthread_mutex_lock(&m_mutexExit);
        if(!m_bExit && !m_bFast) {
		  timespec deadline = GetDeadline(5000);
		  pthread_cond_timedwait(&m_condExit, &m_mutexExit, &deadline);
		}
        pthread_mutex_unlock(&m_mutexExit);
    }
        
exit:
    
    return hr;
}

HRESULT ECServerIndexer::ProcessChanges(IMAPISession *lpSession)
{
    HRESULT hr = hrSuccess;
    MsgStorePtr lpStore;
    IStreamAdapter state(m_strICSState);
    UnknownPtr lpImportInterface;
    ECIndexImporter *lpImporter = NULL;
    ExchangeExportChangesPtr lpExporter;
    
    time_t ulStartTime = time(NULL);
    time_t ulLastTime = time(NULL);
    ULONG ulSteps = 0, ulStep = 0;
    ULONG ulTotalChange = 0, ulTotalBytes = 0;
    ULONG ulLastChange = 0, ulLastBytes = 0;
    ULONG ulCreate = 0, ulChange = 0, ulDelete = 0, ulBytes = 0;
    
    hr = HrOpenDefaultStore(lpSession, &lpStore);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to open system admin store: %08X", hr);
        goto exit;
    }
    
    hr = lpStore->OpenProperty(PR_CONTENTS_SYNCHRONIZER, &IID_IExchangeExportChanges, 0, 0, &lpExporter);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to get system exporter: %08X", hr);
        goto exit;
    }
    
    hr = ECIndexImporter::Create(m_lpConfig, m_lpLogger, m_lpThreadData, NULL, m_guidServer, &lpImporter);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to create stream importer: %08X", hr);
        goto exit;
    }
    
    hr = lpImporter->QueryInterface(IID_IUnknown, &lpImportInterface); 
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "QueryInterface failed for importer: %08X", hr);
        goto exit;
    }
    
    hr = lpExporter->Config(&state, SYNC_NORMAL, lpImportInterface, NULL, NULL, NULL, 0);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to config system exporter: %08X", hr);
        goto exit;
    }

    while(1) {
        hr = lpExporter->Synchronize(&ulSteps, &ulStep);
        
        if(time(NULL) > ulLastTime + 10) {
            m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Step %d of %d", ulStep, ulSteps);
        }
        
        if(FAILED(hr)) {
            m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Synchronize failed: %08X", hr);
            // Do not goto exit, we wish to save the sync state
            break;
        }
        
        lpImporter->GetStats(&ulCreate, &ulChange, &ulDelete, &ulBytes);
        
		ulTotalBytes += ulBytes;
		ulTotalChange += ulCreate + ulChange;
		
        if(hr == hrSuccess)
            break;
            
        if(m_bExit) {
            hr = MAPI_E_USER_CANCEL;
            break;
        }
    }

    pthread_mutex_lock(&m_mutexTrack);
    m_ulTrack++;
    pthread_cond_signal(&m_condTrack);
    pthread_mutex_unlock(&m_mutexTrack);

    if(lpExporter->UpdateState(&state) != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to update system sync state");
    }

    if(ulTotalChange)
       	m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_INFO, "Processed %d changes (%s) in %d seconds", ulTotalChange, str_storage(ulTotalBytes, false).c_str(), (int)(time(NULL) - ulStartTime));
    
exit:
    if(lpImporter)
        lpImporter->Release();
        
    return hr;    
}

/**
 * Build indexes from scratch
 *
 * We just scan all the stores on the server and index each of them in turn. Since we
 * track state information for each folder, if the process is interrupted, the next run
 * will carry on where it left off. As an extra optimization, an index is marked 'complete'
 * when everything has been indexed, in which case it will not be re-indexed during the next
 * run.
 */
HRESULT ECServerIndexer::BuildIndexes(IMAPISession *lpSession)
{
    HRESULT hr = hrSuccess;
    MsgStorePtr lpStore;
    ExchangeManageStorePtr lpEMS;
    MAPITablePtr lpTable;
    SRowSetPtr lpRows;
    SizedSPropTagArray(3, sptaProps) = { 3, { PR_DISPLAY_NAME_W, PR_ENTRYID, PR_EC_STORETYPE } };
    
    hr = HrOpenDefaultStore(lpSession, &lpStore);
    if(hr != hrSuccess)
        goto exit;
        
    hr = lpStore->QueryInterface(IID_IExchangeManageStore, (void **)&lpEMS);
    if(hr != hrSuccess)
        goto exit;
        
    hr = lpEMS->GetMailboxTable(L"", &lpTable, 0);
    if(hr != hrSuccess)
        goto exit;
        
    hr = lpTable->SetColumns((LPSPropTagArray)&sptaProps, 0);
    if(hr != hrSuccess)
        goto exit;
        
    while(1) {
        hr = lpTable->QueryRows(20, 0, &lpRows);
        if(hr != hrSuccess)
            goto exit;
            
        if(lpRows.empty())
            break;
            
        for(unsigned int i = 0; i < lpRows.size(); i++) {
            if(m_bExit) {
                hr = MAPI_E_USER_CANCEL;
                goto exit;
            }
                
            if(lpRows[i].lpProps[2].ulPropTag == PR_EC_STORETYPE && lpRows[i].lpProps[2].Value.ul != ECSTORE_TYPE_PRIVATE)
                continue;
            
            m_lpLogger->Log(EC_LOGLEVEL_INFO, "Starting indexing of store %ls", WSTR(lpRows[i].lpProps[0]));
            
            hr = IndexStore(lpSession, &lpRows[i].lpProps[1].Value.bin);
            if(hr != hrSuccess)
                goto exit;
        }
    }
        
exit:
    return hr;
}

/**
 * Index the entire store
 *
 * We index all the data in the store from IPM_SUBTREE and lower. This excludes associated messages and
 * folders under 'root'. Each folder is processed separately.
 */
HRESULT ECServerIndexer::IndexStore(IMAPISession *lpSession, SBinary *lpsEntryId)
{
    HRESULT hr = hrSuccess;
    MsgStorePtr lpStore;
    SPropValuePtr lpPropSubtree;
    SPropArrayPtr lpPropStore;
    SRowSetPtr lpRows;
    MAPIFolderPtr lpSubtree;
    MAPITablePtr lpHierarchy;
    SizedSPropTagArray(2, sptaFolderProps) = { 2, { PR_DISPLAY_NAME_W, PR_ENTRYID } };
    SizedSPropTagArray(2, sptaStoreProps) = { 2, { PR_MAPPING_SIGNATURE, PR_STORE_RECORD_KEY } };
    ECIndexDB *lpIndex = NULL;
    ULONG cValues = 0;
    ULONG ulObjType = 0;
    ECIndexImporter *lpImporter = NULL;
    
    hr = lpSession->OpenMsgStore(0, lpsEntryId->cb, (LPENTRYID)lpsEntryId->lpb, NULL, 0, &lpStore);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to open store: %08X", hr);
        goto exit;
    }
    
    hr = lpStore->GetProps((LPSPropTagArray)&sptaStoreProps, 0, &cValues, &lpPropStore);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to get store properties: %08X", hr);
        goto exit;
    }
    
    hr = m_lpThreadData->lpIndexFactory->GetIndexDB(&m_guidServer, (GUID *)lpPropStore[1].Value.bin.lpb, true, &lpIndex);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to open index database: %08X", hr);
        goto exit;
    }

    if(lpIndex->Complete()) {
        m_lpLogger->Log(EC_LOGLEVEL_INFO, "Skipping store since it has already been indexed");
        goto exit;
    }

    hr = ECIndexImporter::Create(m_lpConfig, m_lpLogger, m_lpThreadData, NULL, m_guidServer, &lpImporter);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to create stream importer: %08X", hr);
        goto exit;
    }
    
    hr = HrGetOneProp(lpStore, PR_IPM_SUBTREE_ENTRYID, &lpPropSubtree);
    if(hr != hrSuccess) {
        // If you have no IPM subtree, then we're done
        hr = hrSuccess;
        goto exit;
    }
    
    hr = lpStore->OpenEntry(lpPropSubtree->Value.bin.cb, (LPENTRYID)lpPropSubtree->Value.bin.lpb, &IID_IMAPIFolder, 0, &ulObjType, (IUnknown **)&lpSubtree);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to open IPM subtree: %08X", hr);
        goto exit;
    }

    hr = lpSubtree->GetHierarchyTable(CONVENIENT_DEPTH, &lpHierarchy);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to get folder list: %08X", hr);
        goto exit;
    }
    
    hr = lpHierarchy->SetColumns((LPSPropTagArray)&sptaFolderProps, 0);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to set column set for folder list: %08X", hr);
        goto exit;
    }

    while(1) {
        hr = lpHierarchy->QueryRows(20, 0, &lpRows);
        if(hr != hrSuccess) {
            m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to get folder rows: %08X", hr);
            goto exit;
        }
        
        if(lpRows.empty())
            break;
            
        for(unsigned int i=0; i < lpRows.size(); i++) {
            m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Processing folder %ls", WSTR(lpRows[i].lpProps[0]));
            hr = IndexFolder(lpSession, lpStore, &lpRows[i].lpProps[1].Value.bin, WSTR(lpRows[i].lpProps[0]), lpImporter, lpIndex);
            if(hr != hrSuccess) {
                m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Indexing failed for folder: %08X", hr);
                goto exit;
            }
        }
    }
    
    hr = lpIndex->SetComplete();
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_INFO, "Unable to set complete flag for index");
        goto exit;
    }
    
exit:
    if (lpImporter)
        lpImporter->Release();

    if(lpIndex)
         m_lpThreadData->lpIndexFactory->ReleaseIndexDB(lpIndex);
        
    return hr;
}

/**
 * Index the data in a folder
 *
 * This is done by requesting a synchronization stream from the server for the full folder contents, or a part
 * if a previous synchronization had stopped before the folder was done. This is done by requesting the previous
 * sync state, if any, and using that if possible.
 */
 
HRESULT ECServerIndexer::IndexFolder(IMAPISession *lpSession, IMsgStore *lpStore, SBinary *lpsEntryId, const WCHAR *szName, ECIndexImporter *lpImporter, ECIndexDB *lpIndex)
{
    HRESULT hr = hrSuccess;
    SPropArrayPtr lpProps;
    MAPIFolderPtr lpFolder;
    std::string strFolderId((char *)lpsEntryId->lpb, lpsEntryId->cb);
    std::string strFolderState;
    ULONG ulSteps = 0, ulStep = 0;
    ULONG cValues = 0;
    ULONG ulObjType = 0;
    IStreamAdapter state(strFolderState);    
    ExchangeExportChangesPtr lpExporter;
    UnknownPtr lpImportInterface;

	time_t ulLastStatsTime = time(NULL);
	ULONG ulStartTime = ulLastStatsTime;
	ULONG ulBlockChange = 0;
	ULONG ulBlockBytes = 0;
	ULONG ulBytes = 0;
	ULONG ulTotalChange = 0;
	ULONG ulChange = 0, ulCreate = 0, ulDelete = 0;
	unsigned long long ulTotalBytes = 0;
    
    SizedSPropTagArray(3, sptaProps) = {3, { PR_DISPLAY_NAME_W, PR_MAPPING_SIGNATURE, PR_STORE_RECORD_KEY } };

    hr = lpImporter->QueryInterface(IID_IUnknown, &lpImportInterface);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to get importer interface: %08X", hr);
        goto exit;
    }
    
    hr = lpStore->GetProps((LPSPropTagArray)&sptaProps, 0, &cValues, &lpProps);
    if(FAILED(hr))
        goto exit;
        
    lpIndex->GetSyncState(strFolderId, strFolderState); // ignore error and use empty state if this fails
    
    hr = lpStore->OpenEntry(lpsEntryId->cb, (LPENTRYID)lpsEntryId->lpb, &IID_IMAPIFolder, 0, &ulObjType, (IUnknown **)&lpFolder);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to open folder %ls, for indexing: %08X", WSTR(lpProps[0]), hr);
        goto exit;
    }
    
    // Start a synchronization loop
    hr = lpFolder->OpenProperty(PR_CONTENTS_SYNCHRONIZER, &IID_IExchangeExportChanges, 0, 0, (IUnknown **)&lpExporter);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to open exporter: %08X", hr);
        goto exit;
    }

    hr = lpExporter->Config(&state, SYNC_NORMAL, lpImportInterface, NULL, NULL, NULL, 0);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable configure exporter: %08X", hr);
        goto exit;
    }
    
    while(1) {
        hr = lpExporter->Synchronize(&ulSteps, &ulStep);
        if(FAILED(hr)) {
            m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Synchronize failed: %08X", hr);
            // Do not goto exit, we wish to save the sync state
            break;
        }
        
        lpImporter->GetStats(&ulCreate, &ulChange, &ulDelete, &ulBytes);
        
		ulTotalBytes += ulBytes;
		ulTotalChange += ulCreate + ulChange;
		
		ulBlockBytes += ulBytes;
		ulBlockChange += ulCreate + ulChange;
			
        if(hr == hrSuccess)
            break;
            
        if(m_bExit) {
            hr = MAPI_E_USER_CANCEL;
            break;
        }
        
		if (ulLastStatsTime < time(NULL) - 10) {
			unsigned int secs = time(NULL) - ulLastStatsTime;
			m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_INFO, "%.1f%% (%d of %d) of folder '%ls' processed: %s in %d messages (%s/sec, %.1f messages/sec)", ((float)ulStep*100)/ulSteps,ulStep,ulSteps, szName, str_storage(ulBlockBytes, false).c_str(), ulBlockChange, str_storage(ulBlockBytes/secs, false).c_str(), (float)ulBlockChange/secs);
			ulLastStatsTime = time(NULL);
			ulBlockBytes = ulBlockChange = 0;
		}
        
    }

    if(lpExporter->UpdateState(&state) == hrSuccess) {
        if(lpIndex->SetSyncState(strFolderId, strFolderState) != hrSuccess) {
            m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to save sync state");
            hr = MAPI_E_NOT_FOUND;
            goto exit;
        }
    }

    if(ulTotalChange || m_lpLogger->Log(EC_LOGLEVEL_DEBUG))
    	m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_INFO, "Processed folder '%ls' with %d changes (%s) in %d seconds", szName, ulTotalChange, str_storage(ulTotalBytes, false).c_str(), (int)(time(NULL) - ulStartTime));

exit:
        
    return hr;
}

/**
 * Gets the current server-wide ICS state from the server
 *
 * This is used to determine from what state we need to replay server-wide
 * ICS events once we're done exporting data from all the stores. 
 */
HRESULT ECServerIndexer::GetServerState(IMAPISession *lpSession, std::string &strServerState)
{
    HRESULT hr = hrSuccess;
    MsgStorePtr lpStore;
    std::string strState;
    IStreamAdapter state(strState);
    ExchangeExportChangesPtr lpExporter;
    ULONG ulSteps = 0, ulStep = 0;
    
    hr = HrOpenDefaultStore(lpSession, &lpStore);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to open default system store: %08X", hr);
        goto exit;
    }
    
    hr = lpStore->OpenProperty(PR_CONTENTS_SYNCHRONIZER, &IID_IExchangeExportChanges, 0, 0, &lpExporter);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to open systemwide exporter: %08X", hr);
        goto exit;
    }
    
    hr = lpExporter->Config(&state, SYNC_NORMAL | SYNC_CATCHUP, NULL, NULL, NULL, NULL, 0);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to config systemwide exporter: %08X", hr);
        goto exit;
    }
    
    while(1) {
        hr = lpExporter->Synchronize(&ulSteps, &ulStep);
        
        if(FAILED(hr)) {
            m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Synchronize failed on systemwide exporter: %08X", hr);
            goto exit;
        }
        
        if(hr == hrSuccess)
            break;
    }

    lpExporter->UpdateState(&state);
    
    strServerState = strState;
    
exit:    
    return hr;
}

/**
 * Load server-wide state from disk
 */
HRESULT ECServerIndexer::LoadState()
{
    HRESULT hr = hrSuccess;
    std::string strFn = (std::string)m_lpConfig->GetSetting("index_path") + PATH_SEPARATOR + "state-" + bin2hex(sizeof(GUID), (unsigned char *)&m_guidServer);
    int c = 0;
    
    FILE *fp = fopen(strFn.c_str(), "rb");
    
    if(!fp) {
        if(errno != ENOENT) {
            m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to open '%s' for reading: %s", strFn.c_str(), strerror(errno));
            hr = MAPI_E_DISK_ERROR;
        } else {
            hr = MAPI_E_NOT_FOUND;
        }
        goto exit;
    }
    
    m_strICSState.clear();
    
    while((c = fgetc(fp)) != EOF)
        m_strICSState.append(1, c);

exit:
    if(fp)
        fclose(fp);        
    
    return hr;
}

/**
 * Save server-wide state to disk
 */
HRESULT ECServerIndexer::SaveState()
{
    HRESULT hr = hrSuccess;
    std::string strFn = (std::string)m_lpConfig->GetSetting("index_path") + PATH_SEPARATOR + "state-" + bin2hex(sizeof(GUID), (unsigned char *)&m_guidServer);
    
    FILE *fp = fopen(strFn.c_str(), "wb");
    
    if(!fp) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to open '%s' for writing: %s", strFn.c_str(), strerror(errno));
        hr = MAPI_E_DISK_ERROR;
        goto exit;
    }
    
    if(fwrite(m_strICSState.data(), 1, m_strICSState.size(), fp) != m_strICSState.size()) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to write state to '%s': %s", strFn.c_str(), strerror(errno));
        hr = MAPI_E_DISK_ERROR;
        goto exit;
    }
    
exit:
    if(fp)
        fclose(fp);

    return hr;
}

/**
 * Get GUID for the server we're connected to in
 */
HRESULT ECServerIndexer::GetServerGUID(IMAPISession *lpSession, GUID *lpGuid)
{
    HRESULT hr = hrSuccess;
    MsgStorePtr lpStore;
    SPropValuePtr lpProp;
    
    hr = HrOpenDefaultStore(lpSession, &lpStore);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to open default system store: %08X", hr);
        goto exit;
    }
    
    hr = HrGetOneProp(lpStore, PR_MAPPING_SIGNATURE, &lpProp);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to get server guid: %08X", hr);
        goto exit;
    }
    
    *lpGuid = *(GUID*)lpProp->Value.bin.lpb;
    
exit:    
    return hr;
}

/**
 * Run synchronization at least once.
 *
 */
HRESULT ECServerIndexer::RunSynchronization()
{
    HRESULT hr = hrSuccess;

    unsigned int ulTrack = m_ulTrack;
    
    pthread_mutex_lock(&m_mutexExit);
    m_bFast = true;
    pthread_cond_signal(&m_condExit);
    pthread_mutex_unlock(&m_mutexExit);
    
    pthread_mutex_lock(&m_mutexTrack);
    while(m_ulTrack < ulTrack + 2) 			// Wait for two whole sync loops, since 1 may just have missed an update
        pthread_cond_wait(&m_condTrack, &m_mutexTrack);
    pthread_mutex_unlock(&m_mutexTrack);
    
    // No need to signal now
    m_bFast = false;
    
    return hr;
}
