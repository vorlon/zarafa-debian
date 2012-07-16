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

#include "ECIndexImporter.h"

#include "zarafa-search.h"
#include "ECLogger.h"
#include "ECConfig.h"
#include "ECIndexDB.h"
#include "ECInterfaceDefs.h"
#include "ECFifoStream.h"
#include "ECIndexImporterAttachments.h"
#include "ECSerializer.h"
#include "ECIndexerUtil.h"
#include "ECIndexFactory.h"
#include "Util.h"

#include "Trace.h"

#include <mapidefs.h>

using namespace kyotocabinet;

#include "helpers/mapiprophelper.h"
using namespace za::helpers;

/**
 * The index importer receives imports from the streaming exporter for the data received from the
 * server. To parallelize incoming streaming data and the processing of that data, a single processing
 * thread is started at construction time. The thread processes the first incoming stream, waiting for
 * data as it arrives, and then proceeds to the next stream. If data arrives faster than we can process
 * the importer will block until processing of the previous stream has completed. This means that there
 * is only ever one stream in transit, which limits memory usage to the buffersize of the stream (in
 * the order of 128K).
 *
 * Lifetime of the index importer is quite variable; during initial indexing it lives as long as the indexing
 * of an entire store. During incremental indexing, lifetime is equal to a single run (every few seconds).
 */
HRESULT ECIndexImporter::Create(ECConfig *lpConfig, ECLogger *lpLogger, IMsgStore *lpStore, ECThreadData *lpThreadData, ECIndexDB *lpIndex, GUID guidServer, ECIndexImporter **lppImporter)
{
    HRESULT hr = hrSuccess;
    ECIndexImporter *lpImporter = new ECIndexImporter(lpConfig, lpLogger, lpStore, lpThreadData, lpIndex, guidServer);
    lpImporter->AddRef();
    
    hr = lpImporter->OpenDB();
    if(hr != hrSuccess)
        goto exit;
    
    *lppImporter = lpImporter;
    
exit:
    if(hr != hrSuccess && lpImporter)
        delete lpImporter;
        
    return hr;
}

ECIndexImporter::ECIndexImporter(ECConfig *lpConfig, ECLogger *lpLogger, IMsgStore *lpStore, ECThreadData *lpThreadData, ECIndexDB *lpIndex, GUID guidServer)
{
    m_lpConfig = lpConfig;
    m_lpLogger = lpLogger;
    m_lpMsgStore = lpStore;
    m_lpThreadData = lpThreadData;
    m_lpIndex = lpIndex;
    m_guidServer = guidServer;
    
    ECIndexImporterAttachment::Create(m_lpThreadData, this, &m_lpIndexerAttach);

    m_ulDocId = 0;
    m_ulFolderId = 0;
    m_bExit = false;
    m_bThreadStarted = false;
    
    m_ulCreated = m_ulChanged = m_ulDeleted = m_ulBytes = 0;
    
    m_lpDB = NULL;
    
    pthread_cond_init(&m_condStream, NULL);
    pthread_mutex_init(&m_mutexStream, NULL);
}

ECIndexImporter::~ECIndexImporter()
{
    if(m_bThreadStarted) {
        pthread_mutex_lock(&m_mutexStream);
        m_bExit = true;
        pthread_cond_signal(&m_condStream);
        pthread_mutex_unlock(&m_mutexStream);
    
        pthread_join(m_thread, NULL);
    }
    
    if(m_lpIndexerAttach)
        m_lpIndexerAttach->Release();
        
    if(m_lpDB) {
        if(!m_lpDB->end_transaction()) {
            m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to end transaction: %s", m_lpDB->error().message());
        }
        
        m_lpDB->close();
        
        delete m_lpDB;
    }
}

HRESULT ECIndexImporter::OpenDB()
{
    HRESULT hr = hrSuccess;
    std::string strPath = std::string(m_lpConfig->GetSetting("index_path")) + PATH_SEPARATOR + bin2hex(sizeof(GUID), (unsigned char*)&m_guidServer) + ".kct";
    
    m_lpDB = new TreeDB();
    
    if(!m_lpDB->open(strPath, TreeDB::OWRITER | TreeDB::OREADER | TreeDB::OCREATE)) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to open index %s: %s", strPath.c_str(), m_lpDB->error().message());
        hr = MAPI_E_NOT_FOUND;
        goto exit;
    }

    if(!m_lpDB->begin_transaction()) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to start transaction: %s", m_lpDB->error().message());
        hr = MAPI_E_DISK_ERROR;
        goto exit;
    }

	hr = MAPIPropHelper::GetArchiverProps(MAPIPropPtr(m_lpMsgStore, true), NULL, &m_lpArchiveProps);
	if(hr != hrSuccess)
	    goto exit;

exit:
    return hr;
}

HRESULT ECIndexImporter::StartThread()
{
    HRESULT hr = hrSuccess;
    
    if(m_bThreadStarted)
        goto exit;
    
    if(pthread_create(&m_thread, NULL, ECIndexImporter::Thread, this) != 0) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to start thread: %s", strerror(errno));
        hr = MAPI_E_CALL_FAILED;
        goto exit;
    }
    
    m_bThreadStarted = true;
    
exit:
    return hr;
}

void *ECIndexImporter::Thread(void *lpArg)
{
    ECIndexImporter *lpImporter = (ECIndexImporter *)lpArg;
    
    return (void *)lpImporter->ProcessThread();
}

HRESULT ECIndexImporter::QueryInterface(REFIID refiid, void **lppInterface)
{
    REGISTER_INTERFACE(IID_ECUnknown, this);
    REGISTER_INTERFACE(IID_IExchangeImportContentsChanges, &this->m_xECImportContentsChanges);
    REGISTER_INTERFACE(IID_IECImportContentsChanges, &this->m_xECImportContentsChanges);
    REGISTER_INTERFACE(IID_IUnknown, &this->m_xECImportContentsChanges);

    return MAPI_E_INTERFACE_NOT_SUPPORTED;
}

HRESULT ECIndexImporter::GetLastError(HRESULT hResult, ULONG ulFlags, LPMAPIERROR *lppMAPIError)
{
    return MAPI_E_NO_SUPPORT;
}

HRESULT ECIndexImporter::Config(LPSTREAM lpStream, ULONG ulFlags)
{
    return hrSuccess;
}

HRESULT ECIndexImporter::UpdateState(LPSTREAM lpStream)
{
    return hrSuccess;
}

HRESULT ECIndexImporter::ImportMessageChange(ULONG cValue, LPSPropValue lpPropArray, ULONG ulFlags, LPMESSAGE * lppMessage)
{
    return MAPI_E_NO_SUPPORT; // Only streaming supported here
}

HRESULT ECIndexImporter::ImportMessageDeletion(ULONG ulFlags, LPENTRYLIST lpSourceEntryList)
{
    HRESULT hr = hrSuccess;
    ECIndexDB *lpIndex = NULL;
    ECIndexDB *lpThisIndex = NULL;
    docid_t doc;
    GUID guidStore;

	for (unsigned int i=0; i < lpSourceEntryList->cValues; i++) {
	    hr = GetDocIdFromSourceKey(std::string((char *)lpSourceEntryList->lpbin[i].lpb, lpSourceEntryList->lpbin[i].cb), &doc, &guidStore);
	    if(hr != hrSuccess) {
			// eg. document that was never indexed
	        m_lpLogger->Log(EC_LOGLEVEL_INFO, "Got deletion for unknown document: %d", doc);
	        hr = hrSuccess;
	        continue; // There is nothing to delete
	    }
	    
	    m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Remove doc %d", doc);
	    
        if(m_lpIndex == NULL) {
            // Work in multi-store mode: each change we receive may be for a different store. This means
            // we have to get the correct store for each change.
            hr = m_lpThreadData->lpIndexFactory->GetIndexDB(&m_guidServer, &guidStore, true, &lpIndex);
            if(hr != hrSuccess) {
                m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to open index database: %08X", hr);
                goto exit;
            }
            lpThisIndex = lpIndex;
        } else {
            // Work in single-store mode: use the passed index
            lpThisIndex = m_lpIndex;
        }

		hr = lpThisIndex->RemoveTermsDoc(doc, NULL);
		if(hr != hrSuccess) {
		    m_lpLogger->Log(EC_LOGLEVEL_ERROR, "Unable to remove terms for document deletion: %08X", hr);
		    goto exit;
		}
		
		m_ulDeleted++;

        if(lpIndex) {
            m_lpThreadData->lpIndexFactory->ReleaseIndexDB(lpIndex);
            lpIndex = NULL;
        }
	}

    
exit:
    return hr;
}

HRESULT ECIndexImporter::ImportPerUserReadStateChange(ULONG cElements, LPREADSTATE lpReadState)
{
    return hrSuccess; // Dont care about read state changes
}

HRESULT ECIndexImporter::ImportMessageMove(ULONG cbSourceKeySrcFolder, BYTE FAR * pbSourceKeySrcFolder, ULONG cbSourceKeySrcMessage, BYTE FAR * pbSourceKeySrcMessage, ULONG cbPCLMessage, BYTE FAR * pbPCLMessage, ULONG cbSourceKeyDestMessage, BYTE FAR * pbSourceKeyDestMessage, ULONG cbChangeNumDestMessage, BYTE FAR * pbChangeNumDestMessage)
{
    return MAPI_E_NO_SUPPORT; // Dont support moves
}

HRESULT ECIndexImporter::ConfigForConversionStream(LPSTREAM lpStream, ULONG ulFlags, ULONG cValuesConversion, LPSPropValue lpPropArrayConversion)
{
    return MAPI_E_NO_SUPPORT;
}

HRESULT ECIndexImporter::ImportMessageChangeAsAStream(ULONG cpvalChanges, LPSPropValue ppvalChanges, ULONG ulFlags, LPSTREAM *lppstream)
{
    HRESULT hr = hrSuccess;
    LPSPropValue lpPropSK = NULL, lpPropDocId = NULL, lpPropFolderId = NULL, lpPropStoreGuid = NULL, lpPropEntryID = NULL;
    std::map<std::string, ArchiveItemId >::iterator iterArchived;

    m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Receiving incoming stream");    
    // get sourcekey
    // get hierarchyid, folderid
    lpPropSK = PpropFindProp(ppvalChanges, cpvalChanges, PR_SOURCE_KEY);
    lpPropDocId = PpropFindProp(ppvalChanges, cpvalChanges, PR_EC_HIERARCHYID);
    lpPropFolderId = PpropFindProp(ppvalChanges, cpvalChanges, PR_EC_PARENT_HIERARCHYID);
    lpPropStoreGuid = PpropFindProp(ppvalChanges, cpvalChanges, PR_STORE_RECORD_KEY);
    lpPropEntryID = PpropFindProp(ppvalChanges, cpvalChanges, PR_ENTRYID);
    
    if(!lpPropSK || !lpPropDocId || !lpPropFolderId || !lpPropEntryID || (!m_lpIndex && !lpPropStoreGuid)) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Imported document is missing identifier");
        hr = MAPI_E_NOT_FOUND;
        goto exit;
    }

    hr = StartThread();
    if(hr != hrSuccess)
        goto exit;
        
    // Check if the document we are receiving was an archive stub target. In that case we should not index the data under the document
    // id of this document, but under the original document id.
        

    // We're only ever processing one stream at a time, so we have to wait for the processing thread to finish processing the previous one
    pthread_mutex_lock(&m_mutexStream);
    while(m_lpStream) {
        // Processing thread is still busy with the previous thread, wait for it
        pthread_cond_wait(&m_condStream, &m_mutexStream);
    }

    // Record the identifiers of this message for the processing thread
    m_ulFlags = ulFlags;

    // Check if the document we are receiving was an archive stub target. In that case we should not index the data under the document
    // id of this document, but under the original document id.
    iterArchived = m_mapArchived.find(std::string((char *)lpPropEntryID->Value.bin.lpb, lpPropEntryID->Value.bin.cb));
    
    if(iterArchived != m_mapArchived.end()) {
        m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Received archived item for doc %d", iterArchived->second.ulFolderId);
        // The item was an archived message, use the original document identifiers
        m_ulFolderId = iterArchived->second.ulFolderId;
        m_ulDocId = iterArchived->second.ulDocId;
        m_guidStore = iterArchived->second.guidStore;
    } else {
        // Use the document identifiers that we received
        m_ulFolderId = lpPropFolderId->Value.ul;
        m_ulDocId = lpPropDocId->Value.ul;
        m_guidStore = *(GUID*)lpPropStoreGuid->Value.bin.lpb;
    }

    // Record the sourcekey for future deletes since we will only receive the sourcekey in that case, and we need the docid and store guid
    hr = SaveSourceKey(std::string((char *)lpPropSK->Value.bin.lpb, lpPropSK->Value.bin.cb), m_ulDocId, m_guidStore);
    if(hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to record sourcekey information for change: %08X", hr);
        pthread_mutex_unlock(&m_mutexStream);
        goto exit;
    }

    // Create a new stream for the processing thread to process. We pass the write end of the FIFO back to the exporter.
    hr = CreateFifoStreamPair(&m_lpStream, lppstream);

    // Signal that we have more data
    pthread_cond_signal(&m_condStream);
    pthread_mutex_unlock(&m_mutexStream);
    
    if(hr != hrSuccess)
        goto exit;
        
exit:
    return hr;    
}

HRESULT ECIndexImporter::SetMessageInterface(REFIID refiid)
{
    return MAPI_E_NO_SUPPORT;
}

/**
 * This is the main processing thread that processes incoming IStream data streams.
 *
 * Most of the time, this thread will hold the m_mutexStream lock, since we are processing a stream
 * at that time. We only release the lock when we're done processing the stream. We then wait for
 * a new stream to be ready for processing.
 */
HRESULT ECIndexImporter::ProcessThread()
{
    HRESULT hr = hrSuccess;
    ECIndexDB *lpIndex = NULL;
    ECIndexDB *lpThisIndex = NULL;
    ArchiveItem *lpArchiveItem = NULL;
    auto_ptr<ArchiveItem> lpStubTarget;
    
    m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Importer thread started");
    
    while(1) {
        pthread_mutex_lock(&m_mutexStream);
        
        if(m_bExit) {
            pthread_mutex_unlock(&m_mutexStream);
            goto exit;
        }
        
        if(!m_lpStream) {
            // Nothing there, wait for a new stream
            pthread_cond_wait(&m_condStream, &m_mutexStream);
        }
        
        if(m_lpStream) {
            unsigned int ulVersion = 0;
            unsigned int ulRead = 0, ulWritten = 0;
            ECStreamSerializer serializer(m_lpStream);
            
            if(m_lpIndex == NULL) {
                // Work in multi-store mode: each change we receive may be for a different store. This means
                // we have to get the correct store for each change.
                hr = m_lpThreadData->lpIndexFactory->GetIndexDB(&m_guidServer, &m_guidStore, true, &lpIndex);
                if(hr != hrSuccess) {
                    m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to open index database: %08X", hr);
                    goto exit;
                }
                lpThisIndex = lpIndex;
            } else {
                // Work in single-store mode: use the passed index
                lpThisIndex = m_lpIndex;
            }

            if(!(m_ulFlags & SYNC_NEW_MESSAGE)) {
                // Purge data from previous version of this document
                hr = lpThisIndex->RemoveTermsDoc(m_ulDocId, &ulVersion);
                if(hr != hrSuccess) {
                    m_lpLogger->Log(EC_LOGLEVEL_ERROR, "Error removing old fields for update: %08X, ignoring", hr);
                    hr = hrSuccess;
                }
                
                m_ulChanged++;
            } else {
                m_ulCreated++;
            }
            
            hr = ParseStream(m_ulFolderId, m_ulDocId, ulVersion, &serializer, lpThisIndex, true, &lpArchiveItem);
            lpStubTarget.reset(lpArchiveItem); // use auto_ptr for lpArchiveItem, no need for delete now
            
            if(hr != hrSuccess) {
                m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to parse stream: %08X", hr);
                // Since we want to continue to the next message, ignore the error and continue
                hr = hrSuccess;
            }

            if(lpIndex) {
                m_lpThreadData->lpIndexFactory->ReleaseIndexDB(lpIndex);
                lpIndex = NULL;
            }
            
            serializer.Stat(&ulRead, &ulWritten);
            
            m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Done processing stream. Processed %d bytes", ulRead);
            
            m_ulBytes += ulRead;
            
            // We're done with the stream, release it. This is the signal back that we're now ready
            // to receive a new stream.
            m_lpStream.reset();
            pthread_cond_signal(&m_condStream);
        }
        pthread_mutex_unlock(&m_mutexStream);
    }
    
exit:

    m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Importer thread exiting");
    
    return hr;
}
            
/**
 * Deserialize data from the stream and send the plaintext data to the actual fulltext indexer engine
 *
 * We receive an entire serializer stream here, which is traversed completely, even if we know
 * there is no more interesting data after some point; In this case we simply flush the data, so the
 * exporter will not fail since it could not send the entire stream.
 *
 * This function may also be called recursively for embedded messages.
 *
 * @param[in] folder Folder id of message to parse
 * @param[in] doc Document id of message to parse
 * @param[in] version Version of message to parse
 * @param[in] lpSerializer Serializer to read data from
 * @param[in] lpIndex Index database to write data to
 * @param[in] bTop True if the message is a top-level message, false if it is an embedded message
 * @param[out] lppStubTarget List of stub targets detected in this stream
 * @return result
 */
HRESULT ECIndexImporter::ParseStream(folderid_t folder, docid_t doc, unsigned int version, ECSerializer *lpSerializer, ECIndexDB *lpIndex, BOOL bTop, ArchiveItem **lppStubTarget)
{
	HRESULT hr = hrSuccess;
	SPropValuePtr lpProp;
	ULONG ulStreamVersion = 0;
	ULONG ulProps = 0;
	SPropValuePtr lpArchiveProps;
	ULONG ulArchiveProps = 0;
	
	MAPIAllocateBuffer(sizeof(SPropValue) * 10, (void **)&lpArchiveProps);
	
    /* Only the toplevel contains the stream version */
    if (bTop) {
        if (lpSerializer->Read(&ulStreamVersion, sizeof(ulStreamVersion), 1) != erSuccess) {
            hr = MAPI_E_CALL_FAILED;
            goto exit;
        }

        if (ulStreamVersion != 1) {
            hr = MAPI_E_NO_SUPPORT;
            goto exit;
        }
    }

    if (lpSerializer->Read(&ulProps, sizeof(ulProps), 1) != erSuccess) {
        hr = MAPI_E_CALL_FAILED;
        goto exit;
    }

    for (unsigned int i = 0; i < ulProps; i++) {
        /* Read property from stream */
        hr = StreamToProperty(lpSerializer, &lpProp, NULL);
        if (hr != hrSuccess) {
            if (hr == MAPI_E_NO_SUPPORT) {
                hr = hrSuccess;
                continue;
            }
            goto exit;
        }

        // Remember archive properties for later
        if(Util::FindPropInArray(m_lpArchiveProps, lpProp->ulPropTag) >= 0) {
            Util::HrCopyProperty(&lpArchiveProps.get()[ulArchiveProps], lpProp, lpArchiveProps);
            ulArchiveProps++;
            ASSERT(ulArchiveProps < 10);
        }
        
        /* Put the data into the index */
        if (PROP_TYPE(lpProp->ulPropTag) == PT_STRING8 || PROP_TYPE(lpProp->ulPropTag) == PT_UNICODE || PROP_TYPE(lpProp->ulPropTag) == PT_MV_STRING8 || PROP_TYPE(lpProp->ulPropTag) == PT_MV_UNICODE) {
        	std::wstring strContents = PropToString(lpProp);
        	unsigned int ulPropId = PROP_ID(lpProp->ulPropTag);
        	
        	if(!bTop)
        	    // Submessages will be indexed under PR_BODY instead of their respective fields.
        	    ulPropId = PROP_ID(PR_BODY);
        	
        	if(!strContents.empty())
        		hr = lpIndex->AddTerm(folder, doc, ulPropId, version, strContents);
        }
    }

    hr = ParseStreamAttachments(folder, doc, version, lpSerializer, lpIndex);
    if (hr != hrSuccess) {
        m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Attachment parse error: %08X", hr);
        /* Ignore error, just index message without attachments */
        hr = hrSuccess;
    }

    if(ulArchiveProps) {
        bool bStubbed = false;
        
        MAPIPropHelper::IsStubbed(MAPIPropPtr(m_lpMsgStore, true), lpArchiveProps, ulArchiveProps, &bStubbed);
        
        if(bStubbed) {
            ObjectEntryList lstArchives;
            
            MAPIPropHelper::GetArchiveList(MAPIPropPtr(m_lpMsgStore, true), lpArchiveProps, ulArchiveProps, &lstArchives);
            
            if(!lstArchives.empty()) {
                m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Deferring stub index for doc %d", doc);
                m_lstArchived.push_back(ArchiveItem(lstArchives.front().sStoreEntryId.data(), lstArchives.front().sItemEntryId.data()));
                m_mapArchived.insert(std::make_pair(lstArchives.front().sItemEntryId.data(), ArchiveItemId(folder, doc, m_guidStore)));
            }
        }
        
    }

exit:
	return hr;
}

/**
 * Called to parse attachments from the incoming message stream
 *
 * If index_attachments is disabled, this function just flushes the rest of the stream. Otherwise
 * it will start full attachment processing, and may call back to ParseStream() if it encounters an
 * embedded message.
 */
HRESULT ECIndexImporter::ParseStreamAttachments(folderid_t folder, docid_t doc, unsigned int version, ECSerializer *lpSerializer, ECIndexDB *lpIndex)
{
	HRESULT hr = hrSuccess;

	if (!parseBool(m_lpConfig->GetSetting("index_attachments"))) {
	    // The rest of the stream is attachments only, so just flush all that data
		lpSerializer->Flush();
		goto exit;
	}

	hr = m_lpIndexerAttach->ParseAttachments(folder, doc, version, lpSerializer, lpIndex);
	if (hr != hrSuccess)
		goto exit;

exit:
	return hr;
}

/**
 * Gets some statistics about the importer
 *
 * This call also resets the returned counters
 */
HRESULT ECIndexImporter::GetStats(ULONG *lpulCreated, ULONG *lpulChanged, ULONG *lpulDeleted, ULONG *lpulBytes)
{
    HRESULT hr = hrSuccess;
    
    *lpulCreated = m_ulCreated;
    *lpulChanged = m_ulChanged;
    *lpulDeleted = m_ulDeleted;
    *lpulBytes = m_ulBytes;
    
    m_ulCreated = m_ulChanged = m_ulDeleted = m_ulBytes = 0;
    
    return hr;
}

/**
 * Save a sourcekey with document id and store GUID
 *
 * This can be retrieved later with GetDocIdFromSourceKey()
 */
HRESULT ECIndexImporter::SaveSourceKey(const std::string &strSourceKey, unsigned int doc, GUID guidStore)
{
    HRESULT hr = hrSuccess;
    std::string strKey;
    std::string strValue;
    
    strKey = "SK";
    strKey.append(strSourceKey);
    
    strValue.assign((char *)&doc, sizeof(doc));
    strValue.append((char *)&guidStore, sizeof(guidStore));
    
    if(!m_lpDB->set(strKey, strValue)) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to set sourcekey value: %s", m_lpDB->error().message());
        hr = MAPI_E_DISK_ERROR;
        goto exit;
    }
    
exit:
    return hr;
    
}

/**
 * Get document id and store GUID from sourcekey
 *
 * Since the sourcekey is globally unique, each sourcekey maps to a specific document ID and store GUID on this
 * server. We can only resolve sourcekeys that were previously passed to SaveSourceKey().
 */
HRESULT ECIndexImporter::GetDocIdFromSourceKey(const std::string &strSourceKey, unsigned int *lpdoc, GUID *lpGuidStore)
{
    HRESULT hr = hrSuccess;
    std::string strKey;
    std::string strValue;
    
    strKey = "SK";
    strKey.append(strSourceKey);
    
    if(!m_lpDB->get(strKey, &strValue)) {
        hr = MAPI_E_NOT_FOUND;
        goto exit;
    }
    
    *lpdoc = *(unsigned int *)strValue.data();
    *lpGuidStore = *(GUID *)(strValue.data()+sizeof(unsigned int));
    
exit:    
    return hr;
}

/**
 * Get a list of stub targets that were encountered in imports
 * 
 * The call is responsible for requesting the streams of these items and streaming them back
 * to *this instance* of the importer. The importer remembers the mapping of the archived items
 * back to the document id of the stub, so it is important not to free the importer and pass the
 * archived items back to a new instance, because the items would then be indexed under their own
 * document id and store, instead of the original stub's document id and store.
 *
 * @param[out] lppArchived The list of archive stub targets that should be retrieved
 * @return success (cannot fail)
 */
HRESULT ECIndexImporter::GetStubTargets(std::list<ArchiveItem> **lppArchived)
{
    std::list<ArchiveItem> *lpArchived = new std::list<ArchiveItem>(m_lstArchived);
    
    *lppArchived = lpArchived;
    
    m_lstArchived.clear();
    
    return hrSuccess;
}

// Interface forwarders

ULONG ECIndexImporter::xECImportContentsChanges::AddRef()
{
    METHOD_PROLOGUE_(ECIndexImporter, ECImportContentsChanges);
    TRACE_MAPI(TRACE_ENTRY, "IECImportContentsChanges::AddRef");
    return pThis->AddRef();
}

ULONG ECIndexImporter::xECImportContentsChanges::Release()
{
    METHOD_PROLOGUE_(ECIndexImporter, ECImportContentsChanges);
    TRACE_MAPI(TRACE_ENTRY, "IECImportContentsChanges::Release");
    return pThis->Release();
}

DEF_HRMETHOD(TRACE_MAPI, ECIndexImporter, ECImportContentsChanges, QueryInterface, (REFIID, refiid), (void **, lppInterface))
DEF_HRMETHOD(TRACE_MAPI, ECIndexImporter, ECImportContentsChanges, GetLastError, (HRESULT, hResult), (ULONG, ulFlags), (LPMAPIERROR *, lppMAPIError))
DEF_HRMETHOD(TRACE_MAPI, ECIndexImporter, ECImportContentsChanges, Config, (LPSTREAM, lpStream), (ULONG, ulFlags))
DEF_HRMETHOD(TRACE_MAPI, ECIndexImporter, ECImportContentsChanges, UpdateState, (LPSTREAM, lpStream))
DEF_HRMETHOD(TRACE_MAPI, ECIndexImporter, ECImportContentsChanges, ImportMessageChange, (ULONG, cValue, LPSPropValue, lpPropArray), (ULONG, ulFlags), (LPMESSAGE *, lppMessage))
DEF_HRMETHOD(TRACE_MAPI, ECIndexImporter, ECImportContentsChanges, ImportMessageDeletion, (ULONG, ulFlags), (LPENTRYLIST, lpSourceEntryList))
DEF_HRMETHOD(TRACE_MAPI, ECIndexImporter, ECImportContentsChanges, ImportPerUserReadStateChange, (ULONG, cElements), (LPREADSTATE, lpReadState))
DEF_HRMETHOD(TRACE_MAPI, ECIndexImporter, ECImportContentsChanges, ImportMessageMove, (ULONG, cbSourceKeySrcFolder, BYTE FAR *, pbSourceKeySrcFolder), (ULONG, cbSourceKeySrcMessage, BYTE FAR *, pbSourceKeySrcMessage), (ULONG, cbPCLMessage, BYTE FAR *, pbPCLMessage), (ULONG, cbSourceKeyDestMessage, BYTE FAR *, pbSourceKeyDestMessage), (ULONG, cbChangeNumDestMessage, BYTE FAR *, pbChangeNumDestMessage))
DEF_HRMETHOD(TRACE_MAPI, ECIndexImporter, ECImportContentsChanges, ConfigForConversionStream, (LPSTREAM, lpStream), (ULONG, ulFlags), (ULONG, cValuesConversion), (LPSPropValue, lpPropArrayConversion))
DEF_HRMETHOD(TRACE_MAPI, ECIndexImporter, ECImportContentsChanges, ImportMessageChangeAsAStream, (ULONG, cpvalChanges), (LPSPropValue, ppvalChanges), (ULONG, ulFlags), (LPSTREAM *,lppstream))
DEF_HRMETHOD(TRACE_MAPI, ECIndexImporter, ECImportContentsChanges, SetMessageInterface, (REFIID, refiid))
