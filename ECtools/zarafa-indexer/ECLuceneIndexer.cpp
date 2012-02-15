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
#include <fstream>

#include <mapi.h>
#include <mapiext.h>
#include <mapix.h>
#include <mapiutil.h>
#include <mapidefs.h>
#include <mapiguid.h>

#include <edkguid.h>
#include <edkmdb.h>

#include <ECGuid.h>
#include <ECSerializer.h>
#include <ECTags.h>
#include <stringutil.h>
#include <Util.h>

#include "ECFileIndex.h"
#include "ECIndexerUtil.h"
#include "ECLucene.h"
#include "ECLuceneIndexer.h"
#include "ECLuceneIndexerAttachment.h"
#include "ECIndexDB.h"
#include "zarafa-indexer.h"

#include "helpers/mapiprophelper.h"
#include "archiver-session.h"
#include "mapi_ptr.h"

using namespace za::helpers;

ECLuceneIndexer::ECLuceneIndexer(ECThreadData *lpThreadData)
{
	m_lpThreadData = lpThreadData;

	m_lpIndexDB = new ECIndexDB(lpThreadData->lpConfig, lpThreadData->lpLogger);
	m_lpIndexedProps = NULL;
}

ECLuceneIndexer::~ECLuceneIndexer()
{
	if (m_lpIndexedProps)
		MAPIFreeBuffer(m_lpIndexedProps);

	if (m_lpIndexerAttach)
		m_lpIndexerAttach->Release();

	if (m_lpMsgStore)
		m_lpMsgStore->Release();
		
	if (m_lpIndexDB)
		delete m_lpIndexDB;
}

HRESULT ECLuceneIndexer::Create(ECThreadData *lpThreadData, IMsgStore *lpMsgStore, ECLuceneIndexer **lppIndexer)
{
	HRESULT hr = hrSuccess;
	ECLuceneIndexer *lpIndexer = NULL;

	SizedSPropTagArray(1, sExtra) = {
		1, {
			PR_HASATTACH,
		},
	};

	lpIndexer = new ECLuceneIndexer(lpThreadData);
	if (!lpIndexer) {
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}

	lpIndexer->AddRef();

	hr = ECLuceneIndexerAttachment::Create(lpIndexer->m_lpThreadData, lpIndexer, &lpIndexer->m_lpIndexerAttach);
	if (hr != hrSuccess)
		goto exit;

	hr = lpMsgStore->QueryInterface(IID_IMsgStore, (LPVOID *)&lpIndexer->m_lpMsgStore);
	if (hr != hrSuccess)
		goto exit;

	if (lppIndexer)
		*lppIndexer = lpIndexer;

exit:
	if ((hr != hrSuccess) && lpIndexer)
		lpIndexer->Release();

	return hr;
}

HRESULT ECLuceneIndexer::IndexEntries(sourceid_list_t &listCreateEntries, sourceid_list_t &listChangeEntries, sourceid_list_t &listDeleteEntries, ULONG *lpcbRead)
{
	HRESULT hr = hrSuccess;

	try {
		if (!listDeleteEntries.empty()) {
			hr = IndexDeleteEntries(listDeleteEntries);
			if (hr != hrSuccess)
				goto exit;
		}

		if (!listChangeEntries.empty()) {
			hr = IndexUpdateEntries(listChangeEntries, lpcbRead);
			if (hr != hrSuccess)
				goto exit;
		}

		if (!listCreateEntries.empty()) {
			hr = IndexStreamEntries(listCreateEntries, lpcbRead);
			if (hr != hrSuccess)
				goto exit;
		}
	} catch (CLuceneError &e) {
		m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_FATAL, "CLucene error: %s", e.what());
		hr = MAPI_E_CALL_FAILED;
		goto exit;
	}
	catch (std::exception &e) {
		m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_FATAL, "STD error during index: %s", e.what());
		hr = MAPI_E_CALL_FAILED;
		goto exit;
	}
	catch (...) {
		m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_FATAL, "Unknown error during index");
		hr = MAPI_E_CALL_FAILED;
		goto exit;
	}

exit:
	return hr;
}


HRESULT ECLuceneIndexer::IndexUpdateEntries(sourceid_list_t &listSourceId, ULONG *lpcbRead)
{
	HRESULT hr = hrSuccess;
	sourceid_list_t::iterator iter;
	storeid_t store;
	docid_t doc;
	unsigned int ulVersion = 0;
	LARGE_INTEGER lint = {{ 0, 0 }};
	ECSerializer *lpSerializer = NULL;
	ULONG ulRead = 0, ulWritten = 0;

	lpSerializer = new ECStreamSerializer(NULL);
	if (!lpSerializer) {
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}

	for (iter = listSourceId.begin(); iter != listSourceId.end(); iter++) {
		LPSPropValue lpPropHierarchyId = PpropFindProp((*iter)->lpProps, (*iter)->ulProps, PR_EC_HIERARCHYID);
		LPSPropValue lpPropStoreID = PpropFindProp((*iter)->lpProps, (*iter)->ulProps, PR_STORE_RECORD_KEY);
		LPSPropValue lpPropServerID = PpropFindProp((*iter)->lpProps, (*iter)->ulProps, PR_MAPPING_SIGNATURE);
		
		if(!lpPropHierarchyId || !lpPropStoreID || !lpPropServerID) {
			m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_ERROR, "Unable to find document id during update");
			continue;
		}

		store = storeid_t(lpPropServerID, lpPropStoreID);
		doc = lpPropHierarchyId->Value.ul;
		
		hr = m_lpIndexDB->RemoveTermsDoc(store, doc, &ulVersion);
		if(hr != hrSuccess) {
			m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_ERROR, "Error remove old fields for update");
			hr = hrSuccess;
			continue;
		}
		
		if (!(*iter)->lpStream) {
			m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_ERROR, "No data stream during message update");
			hr = hrSuccess;
			continue;
		}

		/* Read number of properties from Stream */
		hr = (*iter)->lpStream->Seek(lint, STREAM_SEEK_SET, NULL);
		if (hr != hrSuccess) {
			m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_ERROR, "Error while reading message data stream size");
			hr = hrSuccess;
			continue;
		}

		lpSerializer->SetBuffer((*iter)->lpStream);

		hr = ParseStream(storeid_t(), 0, 0, ulVersion, (*iter)->ulProps, (*iter)->lpProps, lpSerializer);
		if (hr != hrSuccess) {
			m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_ERROR, "Error while parsing message stream for update");
			hr = hrSuccess;
			continue;
		}

		hr = lpSerializer->Stat(&ulRead, &ulWritten);
		if(hr != hrSuccess)
			goto exit;
			
		if(lpcbRead)
			*lpcbRead += ulRead;
		
	}
	
exit:
	if (lpSerializer)
		delete lpSerializer;

	return hr;
}

HRESULT ECLuceneIndexer::IndexDeleteEntries(sourceid_list_t &listSourceId)
{
	HRESULT hr = hrSuccess;
	sourceid_list_t::iterator iter;
	storeid_t store;

	for (iter = listSourceId.begin(); iter != listSourceId.end(); iter++) {
		LPSPropValue lpPropSK = PpropFindProp((*iter)->lpProps, (*iter)->ulProps, PR_SOURCE_KEY);
		LPSPropValue lpPropFolderId = PpropFindProp((*iter)->lpProps, (*iter)->ulProps, PR_EC_PARENT_HIERARCHYID);
		LPSPropValue lpPropStoreID = PpropFindProp((*iter)->lpProps, (*iter)->ulProps, PR_STORE_RECORD_KEY);
		LPSPropValue lpPropServerID = PpropFindProp((*iter)->lpProps, (*iter)->ulProps, PR_MAPPING_SIGNATURE);
		
		if(!lpPropSK || !lpPropFolderId || !lpPropStoreID || !lpPropServerID) {
			m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_ERROR, "Unable to find properties during deletion");
			continue;
		}

		store = storeid_t(lpPropServerID, lpPropStoreID);
		
		m_lpIndexDB->RemoveTermsDoc(store, lpPropFolderId->Value.ul, std::string((char *)lpPropSK->Value.bin.lpb, lpPropSK->Value.bin.cb));
	}
	
	return hr;
}


HRESULT ECLuceneIndexer::IndexStreamEntries(sourceid_list_t &listSourceId, ULONG *lpcbRead)
{
	HRESULT hr = hrSuccess;
	ECSerializer *lpSerializer = NULL;
	LARGE_INTEGER lint = {{ 0, 0 }};
	storeid_t store;
	ULONG ulWritten = 0, ulRead = 0;

	lpSerializer = new ECStreamSerializer(NULL);
	if (!lpSerializer) {
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}

	for (sourceid_list_t::iterator iter = listSourceId.begin(); iter != listSourceId.end(); iter++) {
		LPSPropValue lpPropSK = PpropFindProp((*iter)->lpProps, (*iter)->ulProps, PR_SOURCE_KEY);
		LPSPropValue lpPropFolderId = PpropFindProp((*iter)->lpProps, (*iter)->ulProps, PR_EC_PARENT_HIERARCHYID);
		LPSPropValue lpPropStoreID = PpropFindProp((*iter)->lpProps, (*iter)->ulProps, PR_STORE_RECORD_KEY);
		LPSPropValue lpPropServerID = PpropFindProp((*iter)->lpProps, (*iter)->ulProps, PR_MAPPING_SIGNATURE);
		LPSPropValue lpDocID = PpropFindProp((*iter)->lpProps, (*iter)->ulProps, PR_EC_HIERARCHYID);

		// quit without error, caller will check this flag too not to save sync point
		if (m_lpThreadData->bShutdown)
			goto exit;

		if (!lpPropSK || !lpPropFolderId || !lpPropStoreID || !lpPropServerID || !lpDocID) {
			m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_ERROR, "Unable to find properties during add");
			continue;
		}

		store = storeid_t(lpPropServerID, lpPropStoreID);

		/* No stream, will be handled by IndexCreateEntries */
		if (!(*iter)->lpStream)
			continue;

		/* Read number of properties from Stream */
		hr = (*iter)->lpStream->Seek(lint, STREAM_SEEK_SET, NULL);
		if (hr != hrSuccess)
			goto exit;

		lpSerializer->SetBuffer((*iter)->lpStream);
		
		m_lpIndexDB->AddSourcekey(store, lpPropFolderId->Value.ul, std::string((char *)lpPropSK->Value.bin.lpb, lpPropSK->Value.bin.cb), lpDocID->Value.ul);

		hr = ParseStream(storeid_t(), 0, 0, 0, (*iter)->ulProps, (*iter)->lpProps, lpSerializer);
		if (hr != hrSuccess) {
			m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_ERROR, "Error while parsing message stream");
			hr = hrSuccess;
			continue;
		}
		
		hr = lpSerializer->Stat(&ulRead, &ulWritten);
		if(hr != hrSuccess)
			goto exit;
			
		if(lpcbRead)
			*lpcbRead += ulRead;
		
	}

exit:
	if (lpSerializer)
		delete lpSerializer;

	return hr;
}

HRESULT ECLuceneIndexer::ParseDocument(storeid_t store, folderid_t folder, docid_t doc, unsigned int version, ULONG cValues, LPSPropValue lpProps, IMessage *lpMessage, BOOL bDefaultField)
{
	HRESULT hr = hrSuccess;
	LPSPropValue lpEntryId = NULL;
	BOOL bAttach = FALSE;
	LPMAPINAMEID *lpNameIDs = NULL;
	ULONG cNames = 0;
	SPropTagArray spta;
	LPSPropTagArray lpta = &spta;

	for (ULONG i = 0; i < cValues; i++) {
		/* PR_HASATTACH should not be indexed */
		if (lpProps[i].ulPropTag == PR_HASATTACH) {
			bAttach = lpProps[i].Value.b;
			continue;
		}

		/* We might need the entryid later */
		if (lpProps[i].ulPropTag == PR_ENTRYID)
			lpEntryId = &lpProps[i];
			
		if(PROP_ID(lpProps[i].ulPropTag) >= 0x8000) {
			// Get named property name for this property
			spta.cValues = 1;
			spta.aulPropTag[0] = lpProps[i].ulPropTag;
			
			hr = lpMessage->GetNamesFromIDs(&lpta, NULL, 0, &cNames, &lpNameIDs);
			if (hr != hrSuccess || cNames != 1) {
				// This may happen if you SetProps() with an invalid property id, eg 0x9000
				hr = hrSuccess;
				// make sure we free if cNames was != 1
				if (lpNameIDs)
					MAPIFreeBuffer(lpNameIDs);
				lpNameIDs = NULL;
			}
		}

		/* Index property */
		if (PROP_TYPE(lpProps[i].ulPropTag) == PT_STRING8 || PROP_TYPE(lpProps[i].ulPropTag) == PT_UNICODE) {
			hr = AddPropertyToDocument(store, folder, doc, version, &lpProps[i], lpNameIDs ? lpNameIDs[0] : NULL, bDefaultField);
			if (hr != hrSuccess)
				goto exit;
		}
			
		if (lpNameIDs)
			MAPIFreeBuffer(lpNameIDs);
		lpNameIDs = NULL;
	}

	if (bAttach && (lpEntryId || lpMessage)) {
		hr = ParseAttachments(store, folder, doc, version, lpEntryId, lpMessage);
		if (hr != hrSuccess) {
			/* Ignore error, just index message without attachments */
			hr = hrSuccess;
		}
	}

exit:
	if (lpNameIDs)
		MAPIFreeBuffer(lpNameIDs);

	return hr;
}

HRESULT ECLuceneIndexer::ParseStream(storeid_t store, folderid_t folder, docid_t doc, unsigned int version, ULONG cValues, LPSPropValue lpProps, ECSerializer *lpSerializer, BOOL bDefaultField)
{
	HRESULT hr = hrSuccess;
	LPSPropValue lpProp = NULL;
	LPMAPINAMEID lpNameID = NULL;
	ULONG ulStreamVersion = 0;
	ULONG ulProps = 0;
	
	bool bIsStubbed = false;

	hr = MAPIPropHelper::IsStubbed(MAPIPropPtr(m_lpMsgStore, true), lpProps, cValues, &bIsStubbed);
	if (hr != hrSuccess)
		goto exit;

	if(!doc) {
		LPSPropValue lpPropHierarchyID = PpropFindProp(lpProps, cValues, PR_EC_HIERARCHYID);
		LPSPropValue lpPropFolderID = PpropFindProp(lpProps, cValues, PR_EC_PARENT_HIERARCHYID);
		LPSPropValue lpPropStoreID = PpropFindProp(lpProps, cValues, PR_STORE_RECORD_KEY);
		LPSPropValue lpPropServerID = PpropFindProp(lpProps, cValues, PR_MAPPING_SIGNATURE);
		
		if(!lpPropHierarchyID || !lpPropFolderID || !lpPropStoreID || !lpPropServerID) {
			m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_FATAL, "Document missing indexing id, skipping");
			goto exit;
		}
		
		doc = lpPropHierarchyID->Value.ul;
		folder = lpPropFolderID->Value.ul;
		store = storeid_t(lpPropServerID, lpPropStoreID);
	}
	
	if (bIsStubbed) {
		m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_INFO, "Processing stubbed message.");
		FlushStream(lpSerializer);
		m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_DEBUG, "Flushed stream.");

		hr = ParseStub(store, folder, doc, version, cValues, lpProps, bDefaultField);
	} else {
		/* Only the toplevel contains the stream version */
		if (!bDefaultField) {
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
			/* Make sure we start with empty variables */
			if (lpProp)
				MAPIFreeBuffer(lpProp);
			lpProp = NULL;
			if (lpNameID)
				MAPIFreeBuffer(lpNameID);
			lpNameID = NULL;

			/* Read property from stream */
			hr = StreamToProperty(lpSerializer, &lpProp, &lpNameID);
			if (hr != hrSuccess) {
				if (hr == MAPI_E_NO_SUPPORT) {
					hr = hrSuccess;
					continue;
				}
				goto exit;
			}

			/* Index property */
			if (PROP_TYPE(lpProp->ulPropTag) == PT_STRING8 || PROP_TYPE(lpProp->ulPropTag) == PT_UNICODE) {
				hr = AddPropertyToDocument(store, folder, doc, version, lpProp, lpNameID, bDefaultField);
				if (hr != hrSuccess)
					goto exit;
			}
		}

		hr = ParseStreamAttachments(store, folder, doc, version, lpSerializer);
		if (hr != hrSuccess) {
			/* Ignore error, just index message without attachments */
			hr = hrSuccess;
		}
	}

exit:
	if (lpProp)
		MAPIFreeBuffer(lpProp);

	if (lpNameID)
		MAPIFreeBuffer(lpNameID);

	return hr;
}

HRESULT ECLuceneIndexer::ParseAttachments(storeid_t store, folderid_t folder, docid_t doc, unsigned int version, LPSPropValue lpEntryId, IMessage *lpOrigMessage)
{
	HRESULT hr = hrSuccess;
	IMessage *lpMessage = NULL;
	ULONG ulObjType = 0;

	if (!parseBool(m_lpThreadData->lpConfig->GetSetting("index_attachments"))) {
		hr = MAPI_E_NO_SUPPORT;
		goto exit;
	}

	if (lpOrigMessage) {
		lpMessage = lpOrigMessage;
		lpMessage->AddRef();
	} else if (lpEntryId && lpEntryId->Value.bin.cb) {
		hr = m_lpMsgStore->OpenEntry(lpEntryId->Value.bin.cb, (LPENTRYID)lpEntryId->Value.bin.lpb, &IID_IMessage, 0, &ulObjType, (LPUNKNOWN *)&lpMessage);
		if (hr != hrSuccess)
			goto exit;
	} else {
		hr = MAPI_E_NO_SUPPORT;
		goto exit;
	}

	hr = m_lpIndexerAttach->ParseAttachments(store, folder, doc, version, lpMessage);
	if (hr != hrSuccess)
		goto exit;

exit:
	if (lpMessage)
		lpMessage->Release();

	return hr;
}

HRESULT ECLuceneIndexer::ParseStreamAttachments(storeid_t store, folderid_t folder, docid_t doc, unsigned int version, ECSerializer *lpSerializer)
{
	HRESULT hr = hrSuccess;

	if (!parseBool(m_lpThreadData->lpConfig->GetSetting("index_attachments"))) {
		hr = MAPI_E_NO_SUPPORT;
		goto exit;
	}

	hr = m_lpIndexerAttach->ParseAttachments(store, folder, doc, version, lpSerializer);
	if (hr != hrSuccess)
		goto exit;

exit:
	return hr;
}

HRESULT ECLuceneIndexer::ParseStub(storeid_t store, folderid_t folder, docid_t doc, unsigned int version, ULONG cValues, LPSPropValue lpProps, BOOL bDefaultField)
{
	HRESULT			hr = hrSuccess;
	LPSPropValue	lpEntryID = NULL;
	ULONG			ulType = 0;
	MessagePtr		ptrMessage;

	SPropTagArrayPtr	ptrPropTagArray;
	ULONG				*lpPropTag = NULL;
	ULONG 				ulProps = 0;
	SPropArrayPtr		ptrProps;

	lpEntryID = PpropFindProp(lpProps, cValues, PR_ENTRYID);
	if (lpEntryID == NULL) {
		m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_FATAL, "No entryid found for stubbed message.");
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	hr = m_lpMsgStore->OpenEntry(lpEntryID->Value.bin.cb, (LPENTRYID)lpEntryID->Value.bin.lpb, &ptrMessage.iid, 0, &ulType, &ptrMessage);
	if (hr != hrSuccess) {
		m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to open stubbed message.");
		goto exit;
	}

	// Process the stub
	hr = ptrMessage->GetPropList(fMapiUnicode, &ptrPropTagArray);
	if (FAILED(hr)) {
		m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to get archive property list. hr=0x%08x", hr);
		goto exit;
	}

	// Replace any non PR_BODY body type with PR_BODY
	for (lpPropTag = ptrPropTagArray->aulPropTag; lpPropTag < ptrPropTagArray->aulPropTag + ptrPropTagArray->cValues; ++lpPropTag) {
		if (*lpPropTag == PR_RTF_COMPRESSED || PROP_ID(*lpPropTag) == PROP_ID(PR_BODY_HTML)) {
			*lpPropTag = PR_BODY;
			break;
		}
	}
	// Replace all other non PR_BODY body type with PR_NULL
	for (; lpPropTag < ptrPropTagArray->aulPropTag + ptrPropTagArray->cValues; ++lpPropTag) {
		if (*lpPropTag == PR_RTF_COMPRESSED || PROP_ID(*lpPropTag) == PROP_ID(PR_BODY_HTML))
			*lpPropTag = PR_NULL;
	}

	hr = ptrMessage->GetProps(ptrPropTagArray, 0, &ulProps, &ptrProps);
	if (FAILED(hr)) {
		m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to load archive properties. hr=0x%08x", hr);
		goto exit;
	}

	// Open all MAPI_E_NOT_ENOUGH_MEMORY properties and update the properties
	for (ULONG i = 0; i < ulProps; ++i) {
		if (PROP_TYPE(ptrPropTagArray->aulPropTag[i]) == PT_TSTRING &&
			PROP_TYPE(ptrProps[i].ulPropTag) == PT_ERROR &&
			ptrProps[i].Value.err == MAPI_E_NOT_ENOUGH_MEMORY)
		{
			hr = OpenProperty(ptrMessage, ptrPropTagArray->aulPropTag[i], ptrProps, &ptrProps[i]);
			if (hr != hrSuccess) {
				m_lpThreadData->lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to load large archive property. tag=0x%04x, hr=0x%08x", ptrPropTagArray->aulPropTag[i], hr);
				goto exit;
			}
		}
	}
	
	hr = ParseDocument(store, folder, doc, version, ulProps, ptrProps.get(), ptrMessage, bDefaultField);

exit:
	return hr;
}

HRESULT ECLuceneIndexer::AddPropertyToDocument(storeid_t store, folderid_t folder, docid_t doc, unsigned int version, LPSPropValue lpProp, LPMAPINAMEID lpNameID, BOOL bDefaultField)
{
	HRESULT hr = hrSuccess;
	std::wstring strContents = PropToString(lpProp);
	
	// FIXME do string tokenization here instead of in AddTerm()

	if(!strContents.empty())
		hr = m_lpIndexDB->AddTerm(store, folder, doc, PROP_ID(lpProp->ulPropTag), version, strContents);
	
	return hr;
}

std::wstring ECLuceneIndexer::GetDynamicFieldName(LPSPropValue lpProp, LPMAPINAMEID lpNameID)
{
	std::wstring strName;
	
	if(lpProp->ulPropTag < 0x8500 || lpNameID == NULL) {
		strName = wstringify_int64(PROP_ID(lpProp->ulPropTag), true);
	} else {
		strName = bin2hexw(sizeof(GUID), (unsigned char *)lpNameID->lpguid);
		strName += '.';
		if(lpNameID->ulKind == MNID_ID) {
			strName += wstringify_int64(PROP_ID(lpNameID->Kind.lID)); // using int64 because it outputs the same format as stringify_int64 (used in server)
		} else {
			strName += lpNameID->Kind.lpwstrName;
		}
	}
	
	return strName;
}
