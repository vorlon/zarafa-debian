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
#include "WSStreamOps.h"
#include "ECGuid.h"

#include "Mem.h"


// Utils
#include "SOAPUtils.h"
#include "WSUtil.h"
#include "stringutil.h"
#include "ZarafaUtil.h"
#include "StreamTypes.h"

#include "ECDebug.h"
#include "SOAPSock.h"


#include <algorithm>	// std::copy

#include <charset/convert.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef DEBUG
	#define STR_DEF_TIMEOUT 0
#else
	#define STR_DEF_TIMEOUT	60000
#endif

#define WSSO_MARK_EOS ((unsigned int)0)		// End Of Stream	(Contains Frames)
#define WSSO_MARK_EOF ((unsigned int)-1)	// End Of Frame		(Each frame is stored as one buffer in m_sBufferList)

#define WSSO_FLG_IGNORE_EOF	0x00000001		// Read over frame boundaries

struct WSSO_EmcasArgs {
	WSStreamOps			*lpStreamOps;
	sourceKeyPairArray	*lpsSourceKeyPairs;
	propTagArray		sPropTags;
	ULONG				ulFlags;
};

struct WSSO_ImfsArgs {
	WSStreamOps			*lpStreamOps;
	ULONG				ulFlags;
	ULONG				ulSyncId;
	entryId				sEntryId;
	entryId				sFolderEntryId;
	bool				bIsNew;
	struct propVal		*lpsConflictItems;
};

/*
 * The WSStreamOps for use with the WebServices transport
 */
WSStreamOps::WSStreamOps(ZarafaCmd *lpCmd, ECSESSIONID ecSessionId, ULONG cbFolderEntryId, LPENTRYID lpFolderEntryId, unsigned int ulServerCapabilities)
: m_threadPool(1)	// We need just 1 worker thread
{
	m_ulRef = 0;
	m_lpCmd = lpCmd;
	m_ecSessionId = ecSessionId;
	m_ulServerCapabilities = ulServerCapabilities;

	m_cbFolderEntryId = cbFolderEntryId;
	MAPIAllocateBuffer(cbFolderEntryId, (void**)&m_lpFolderEntryId);
	memcpy(m_lpFolderEntryId, lpFolderEntryId, cbFolderEntryId);
	
	pthread_mutex_init(&m_hSoapLock, NULL);
	m_bAttachValid = false;
	
	m_eMode = mNone;
	m_bDone = false;
	m_ulMaxBufferSize = 128*1024;
	m_ulMaxBufferCount = 4;

	pthread_mutex_init(&m_hBufferLock, NULL);
	pthread_cond_init(&m_hBufferCond, NULL);

	pthread_mutex_init(&m_hStreamInfoLock, NULL);
	pthread_cond_init(&m_hStreamInfoCond, NULL);
	m_bStreamInfoState = Pending;
}

WSStreamOps::~WSStreamOps()
{
	ASSERT(!m_ptrDeferredFunc.get() || m_ptrDeferredFunc->done());

	pthread_cond_destroy(&m_hStreamInfoCond);
	pthread_mutex_destroy(&m_hStreamInfoLock);
	pthread_cond_destroy(&m_hBufferCond);
	pthread_mutex_destroy(&m_hBufferLock);
	pthread_mutex_destroy(&m_hSoapLock);

	for (StreamInfoMap::iterator it = m_mapStreamInfo.begin(); it != m_mapStreamInfo.end(); ++it)
		MAPIFreeBuffer(it->second.lpsPropVals);

	if (m_cbFolderEntryId)
		MAPIFreeBuffer(m_lpFolderEntryId);

	while (!m_sBufferList.empty()) {
		delete m_sBufferList.front();
		m_sBufferList.pop_front();
	}

	DestroySoapTransport(m_lpCmd);
}

HRESULT WSStreamOps::Create(ZarafaCmd *lpCmd, ECSESSIONID ecSessionId, ULONG cbFolderEntryId, LPENTRYID lpFolderEntryId, unsigned int ulServerCapabilities, WSStreamOps **lppStreamOps)
{
	HRESULT hr = hrSuccess;
	WSStreamOps *lpStreamOps = NULL;

	lpStreamOps = new WSStreamOps(lpCmd, ecSessionId, cbFolderEntryId, lpFolderEntryId, ulServerCapabilities);

	hr = lpStreamOps->QueryInterface(IID_ECStreamOps, (void **)lppStreamOps);

	if(hr != hrSuccess)
		delete lpStreamOps;

	return hr;
}

ECThreadPool* WSStreamOps::GetThreadPool()
{
	// @todo: Implement ECDynamicThreadPool and use one of those for all WSStreamOps instances.
	return &m_threadPool;
}

HRESULT WSStreamOps::HrStartExportMessageChangesAsStream(ULONG ulFlags, const std::vector<ICSCHANGE> &sChanges, LPSPropTagArray lpsPropTags)
{
	HRESULT				hr = hrSuccess;
	sourceKeyPairArray	*lpsSourceKeyPairs = NULL;
	WSSO_EmcasArgs		*lpArgs = NULL;
	
	if (lpsPropTags == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}
	
	if (sChanges.empty()) {
		hr = MAPI_E_UNABLE_TO_COMPLETE;
		goto exit;
	}
		
	hr = CopyICSChangeToSOAPSourceKeys((ULONG)sChanges.size(), (ICSCHANGE*)&sChanges.front(), &lpsSourceKeyPairs);
	if (hr != hrSuccess)
		goto exit;
		
	
	// Create  a new stream, that will perform the call on a seperate thread and will read the MTOM stream and writes it
	// into an internal buffer.
	// This stream will lock the current session until it's finished (or released). 
	lpArgs = new WSSO_EmcasArgs;
	lpArgs->lpStreamOps = this;
	lpArgs->lpsSourceKeyPairs = lpsSourceKeyPairs;
	lpArgs->ulFlags = ulFlags;

	if (lpsPropTags && lpsPropTags->cValues > 0) {
		lpArgs->sPropTags.__size = lpsPropTags->cValues;
		lpArgs->sPropTags.__ptr = new unsigned int[lpsPropTags->cValues];
		memcpy(lpArgs->sPropTags.__ptr, lpsPropTags->aulPropTag, lpsPropTags->cValues * sizeof *lpArgs->sPropTags.__ptr);
	} else {
		lpArgs->sPropTags.__size = 0;
		lpArgs->sPropTags.__ptr = NULL;
	}

	// Wait for existing thread to finish
	if (m_ptrDeferredFunc.get())
		m_ptrDeferredFunc->wait();

	pthread_mutex_lock(&m_hBufferLock);

	// Set the IStream mode to read
	SetMode(mRead);

	// Clean the stream info lock
	pthread_mutex_lock(&m_hStreamInfoLock);
	m_mapStreamInfo.clear();
	m_bStreamInfoState = Pending;
	pthread_mutex_unlock(&m_hStreamInfoLock);

	m_ptrDeferredFunc.reset(new DeferredFunc(FinishExportMessageChangesAsStream, lpArgs));
	if (!m_ptrDeferredFunc->dispatchOn(GetThreadPool())) {
		m_ptrDeferredFunc.reset();
		hr = MAPI_E_CALL_FAILED;
	}
	
	pthread_mutex_unlock(&m_hBufferLock);

exit:
	if (hr != hrSuccess) {
		if (lpsSourceKeyPairs)
			MAPIFreeBuffer(lpsSourceKeyPairs);
		if (lpArgs) {
			if (lpArgs->sPropTags.__ptr)
				delete[] lpArgs->sPropTags.__ptr;
			delete lpArgs;
		}
	}

	return hr;
}

HRESULT WSStreamOps::HrStartImportMessageFromStream(ULONG ulFlags, ULONG ulSyncId, ULONG cbEntryID, LPENTRYID lpEntryID, bool bIsNew, LPSPropValue lpConflictItems)
{
	HRESULT			hr = hrSuccess;
	WSSO_ImfsArgs	*lpArgs = NULL;
	
	lpArgs = new WSSO_ImfsArgs;
	memset(lpArgs, 0, sizeof *lpArgs);

	lpArgs->lpStreamOps = this;
	lpArgs->ulFlags = ulFlags;
	lpArgs->ulSyncId = ulSyncId;
	lpArgs->bIsNew = bIsNew;

	if (cbEntryID > 0 && lpEntryID != NULL) {
		hr = CopyMAPIEntryIdToSOAPEntryId(cbEntryID, lpEntryID, &lpArgs->sEntryId, false);
		if (hr != hrSuccess)
			goto exit;
	} else {
		lpArgs->sEntryId.__size = 0;
		lpArgs->sEntryId.__ptr = NULL;
	}

	if (lpConflictItems != NULL) {
		lpArgs->lpsConflictItems = new struct propVal;
		hr = CopyMAPIPropValToSOAPPropVal(lpArgs->lpsConflictItems, lpConflictItems);
		if (hr != hrSuccess)
			goto exit;
	} else
		lpArgs->lpsConflictItems = NULL;

	hr = CopyMAPIEntryIdToSOAPEntryId(m_cbFolderEntryId, m_lpFolderEntryId, &lpArgs->sFolderEntryId, false);
	if (hr != hrSuccess)
		goto exit;

	// Wait for existing thread to finish
	if (m_ptrDeferredFunc.get())
		m_ptrDeferredFunc->wait();

	pthread_mutex_lock(&m_hBufferLock);

	// Set the IStream mode to write
	SetMode(mWrite);

	m_ptrDeferredFunc.reset(new DeferredFunc(FinishImportMessageFromStream, lpArgs));
	if (!m_ptrDeferredFunc->dispatchOn(GetThreadPool())) {
		m_ptrDeferredFunc.reset();
		hr = MAPI_E_CALL_FAILED;
	}

	pthread_mutex_unlock(&m_hBufferLock);

exit:

	if (hr != hrSuccess) {
		if (lpArgs) {
			if (lpArgs->sEntryId.__ptr)
				delete[] lpArgs->sEntryId.__ptr;
			if (lpArgs->sFolderEntryId.__ptr)
				delete[] lpArgs->sFolderEntryId.__ptr;
			if (lpArgs->lpsConflictItems)
				FreePropVal(lpArgs->lpsConflictItems, true);
			delete lpArgs;
		}
	}

	return hr;
}

HRESULT WSStreamOps::GetSteps(std::set<unsigned long> *lpsetSteps)
{
	HRESULT					hr = hrSuccess;
	StreamInfoMap::iterator	it;

	if (lpsetSteps == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	pthread_mutex_lock(&m_hStreamInfoLock);
	while (m_bStreamInfoState == Pending)
		pthread_cond_wait(&m_hStreamInfoCond, &m_hStreamInfoLock);

	if (m_bStreamInfoState == Ready) {
		for (it = m_mapStreamInfo.begin(); it != m_mapStreamInfo.end(); ++it)
			lpsetSteps->insert(it->second.ulStep);
	} else
		hr = MAPI_E_NETWORK_ERROR;

	pthread_mutex_unlock(&m_hStreamInfoLock);

exit:
	return hr;
}

HRESULT WSStreamOps::GetStreamInfo(const char *lpszId, ECStreamInfo *lpsStreamInfo)
{
	HRESULT					hr = hrSuccess;
	StreamInfoMap::iterator	it;

	if (lpszId == NULL || lpsStreamInfo == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	pthread_mutex_lock(&m_hStreamInfoLock);
	while (m_bStreamInfoState == Pending)
		pthread_cond_wait(&m_hStreamInfoCond, &m_hStreamInfoLock);

	if (m_bStreamInfoState == Ready) {
		it = m_mapStreamInfo.find(lpszId);
		if (it == m_mapStreamInfo.end())
			hr = MAPI_E_NOT_FOUND;
		else
			*lpsStreamInfo = it->second;
	} else
		hr = MAPI_E_NETWORK_ERROR;

	pthread_mutex_unlock(&m_hStreamInfoLock);

exit:
	return hr;
}

HRESULT WSStreamOps::CopyFrameTo(IStream *pstm, ULARGE_INTEGER *pcbCopied)
{
	HRESULT			hr = hrSuccess;
	ULARGE_INTEGER	pcbTrans = {{0, 0}};
	unsigned long	cbNow = 0;
	unsigned long	cbRead = 0;
	ULONG			cbWritten = 0;
	char			*lpszBuffer = NULL;
	
	if (pstm == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}
	
	if (m_eMode == mNone) {
		hr = MAPI_E_UNCONFIGURED;
		goto exit;
	} else if (m_eMode != mRead) {
		hr = MAPI_E_NO_ACCESS;
		goto exit;
	}
	
	try {
		lpszBuffer = new char[128*1024];
	} catch (const std::bad_alloc &) {
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}
	
	while (true) {
		cbNow = 128*1024;
		hr = ReadBuf(lpszBuffer, cbNow, 0, &cbRead);
		if (hr != hrSuccess)
			goto exit;
			
		cbWritten = 0;
		while (cbWritten < cbRead) {
			ULONG cbw = 0;
			hr = pstm->Write(lpszBuffer + cbWritten, cbRead - cbWritten, &cbw);
			if (hr != hrSuccess)
				goto exit;
			cbWritten += cbw;
		}

		pcbTrans.QuadPart += cbRead;
		if (cbRead < cbNow)
			break;
	}
	
	if (pcbCopied)
		*pcbCopied = pcbTrans;
	
exit:
	if (lpszBuffer)
		delete[] lpszBuffer;

	return hr;
}

HRESULT WSStreamOps::FlushFrame()
{
	HRESULT			hr = hrSuccess;
	unsigned long	cbNow = 0;
	unsigned long	cbRead = 0;
	char			*lpszBuffer = NULL;
	
	if (m_eMode == mNone) {
		hr = MAPI_E_UNCONFIGURED;
		goto exit;
	} else if (m_eMode != mRead) {
		hr = MAPI_E_NO_ACCESS;
		goto exit;
	}
	
	lpszBuffer = new char[1048576];
	
	while (true) {
		cbNow = 1048576;
		hr = ReadBuf(lpszBuffer, cbNow, 0, &cbRead);
		if (hr != hrSuccess)
			goto exit;

		// Just forget about the data we just read

		if (cbRead < cbNow)
			break;
	}
	
exit:
	if (lpszBuffer)
		delete[] lpszBuffer;

	return hr;
}

// We need to handle the situation where the IStream interface has been released
// and the thread holds the only reference to this object. In that case we should
// stop writing in the FIFO as it will not be read out anymore. For this purpose
// we'll track the refcount. If the refcount of one is seen in the thread, it
// it holds the only reference and can act accordingly.

// IUnknown
ULONG WSStreamOps::AddRef()
{
	return ++m_ulRef;
}

ULONG WSStreamOps::Release()
{
	ULONG	ulRef = 0;

	pthread_mutex_lock(&m_hBufferLock);
	ulRef = --m_ulRef;

	if (ulRef == 1 && !m_ptrDeferredFunc.get()) {
		// There's only 1 reader or 1 writer left. In any case writing an
		// end of stream will cause a reader to stop reading.
		if (!m_sBufferList.empty())
			m_sBufferList.back()->Close();
		m_bDone = true; 
		pthread_cond_broadcast(&m_hBufferCond);

		pthread_mutex_unlock(&m_hBufferLock);

	} else if (ulRef == 0) {
		if (m_ptrDeferredFunc.get()) {
			// Thread might be reading or writing. make sure it's signalled to stop doing that.
			if (!m_sBufferList.empty())
				m_sBufferList.back()->Close();
			m_bDone = true; 
			pthread_cond_broadcast(&m_hBufferCond);
		}
		
		// Release the buffer lock since the thread may need the lock to continue
		pthread_mutex_unlock(&m_hBufferLock);

		if (m_ptrDeferredFunc.get())
			m_ptrDeferredFunc->wait();
			
		delete this;
	} else {
		pthread_mutex_unlock(&m_hBufferLock);
	}

	return ulRef;
}

/**
 * Stop this objects async operation and get the result. Note that
 * the operation should already be completed, but an implicit end is normally signalled through Release().
 *
 * @param[out]	lphrResult
 *					Pointer to a HRESULT value that will be set to the result of the async operation.
 *
 * @return	HRESULT
 * @retval	MAPI_E_INVALID_PARAMETER		The provided lphrResult argument is NULL.
 * @retval	MAPI_E_UNCONFIGURED				There was no async operation bound to this object.
 */
HRESULT WSStreamOps::CloseAndGetAsyncResult(HRESULT *lphrResult)
{
	HRESULT hr = hrSuccess;

	if (lphrResult == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	pthread_mutex_lock(&m_hBufferLock);

	if (!m_ptrDeferredFunc.get()) {
		pthread_mutex_unlock(&m_hBufferLock);
		hr = MAPI_E_UNCONFIGURED;
		goto exit;
	}

	// Thread might be reading. make sure it's signalled to stop doing that.
	if (!m_sBufferList.empty())
		m_sBufferList.back()->Close();
	m_bDone = true; 
	pthread_cond_broadcast(&m_hBufferCond);
	pthread_mutex_unlock(&m_hBufferLock);

	// make sure the thread is finished
	m_ptrDeferredFunc->wait();	// This can't fail
	if(lphrResult)
		*lphrResult = m_ptrDeferredFunc->result();

exit:
	return hr;
}
	
HRESULT WSStreamOps::QueryInterface(REFIID refiid, void **lppInterface)
{
	REGISTER_INTERFACE(IID_ECUnknown, this);
	REGISTER_INTERFACE(IID_ECStreamOps, this);

	REGISTER_INTERFACE(IID_IUnknown, &this->m_xUnknown);
	REGISTER_INTERFACE(IID_IStream, &this->m_xStream);

	return MAPI_E_INTERFACE_NOT_SUPPORTED;
}
	
// ISequentialStream
HRESULT WSStreamOps::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	HRESULT			hr = hrSuccess;
	unsigned long	cbRead = 0;
	
	if (pv == NULL || cb == 0 || pcbRead == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}
	
	if (m_eMode == mNone) {
		hr = MAPI_E_UNCONFIGURED;
		goto exit;
	} else if (m_eMode != mRead) {
		hr = MAPI_E_NO_ACCESS;
		goto exit;
	}
	
	hr = ReadBuf((char*)pv, cb, WSSO_FLG_IGNORE_EOF, &cbRead);
	if (hr != hrSuccess)
		goto exit;
		
	*pcbRead = cbRead;
	
exit:
	return hr;
}

HRESULT WSStreamOps::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
	HRESULT			hr = hrSuccess;
	
	if (pv == NULL || cb == 0 || pcbWritten == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}
	
	if (m_eMode == mNone) {
		hr = MAPI_E_UNCONFIGURED;
		goto exit;
	} else if (m_eMode != mWrite) {
		hr = MAPI_E_NO_ACCESS;
		goto exit;
	}
	
	hr = WriteBuf((char*)pv, cb);
	if (hr != hrSuccess)
		goto exit;
		
	*pcbWritten = cb;
	
exit:
	return hr;
}

// IStream
HRESULT WSStreamOps::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
	return MAPI_E_NO_SUPPORT;
}

HRESULT WSStreamOps::SetSize(ULARGE_INTEGER libNewSize)
{
	return MAPI_E_NO_SUPPORT;
}

HRESULT WSStreamOps::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
	HRESULT			hr = hrSuccess;
	ULARGE_INTEGER	pcbTrans = {{0, 0}};
	unsigned long	cbNow = 0;
	unsigned long	cbRead = 0;
	ULONG			cbWritten = 0;
	char			*lpszBuffer = NULL;
	
	if (pstm == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}
	
	if (m_eMode == mNone) {
		hr = MAPI_E_UNCONFIGURED;
		goto exit;
	} else if (m_eMode != mRead) {
		hr = MAPI_E_NO_ACCESS;
		goto exit;
	}
	
	lpszBuffer = new char[1048576];
	
	while (pcbTrans.QuadPart < cb.QuadPart) {
		cbNow = cb.QuadPart - pcbTrans.QuadPart > 1048576 ? 1048576 : (unsigned long)(cb.QuadPart - pcbTrans.QuadPart);
		hr = ReadBuf(lpszBuffer, cbNow, WSSO_FLG_IGNORE_EOF, &cbRead);
		if (hr != hrSuccess)
			goto exit;
			
		cbWritten = 0;
		while (cbWritten < cbRead) {
			ULONG cbw = 0;
			hr = pstm->Write(lpszBuffer + cbWritten, cbRead - cbWritten, &cbw);
			if (hr != hrSuccess)
				goto exit;
			cbWritten += cbw;
		}

		pcbTrans.QuadPart += cbRead;
		if (cbRead < cbNow)
			break;
	}
	
	if (pcbRead)
		*pcbRead = pcbTrans;
	if (pcbWritten)
		*pcbWritten = pcbTrans;
	
exit:
	if (lpszBuffer)
		delete[] lpszBuffer;

	return hr;
}

HRESULT WSStreamOps::Commit(DWORD grfCommitFlags)
{
	return MAPI_E_NO_SUPPORT;
}

HRESULT WSStreamOps::Revert(void)
{
	return MAPI_E_NO_SUPPORT;
}

HRESULT WSStreamOps::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	return MAPI_E_NO_SUPPORT;
}

HRESULT WSStreamOps::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	return MAPI_E_NO_SUPPORT;
}

HRESULT WSStreamOps::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
	return MAPI_E_NO_SUPPORT;
}

HRESULT WSStreamOps::Clone(IStream **ppstm)
{
	return MAPI_E_NO_SUPPORT;
}

HRESULT WSStreamOps::LockSoap()
{
	pthread_mutex_lock(&m_hSoapLock);
	return erSuccess;
}

HRESULT WSStreamOps::UnLockSoap()
{
	//Clean up data create with soap_malloc
	if (m_lpCmd->soap)
		soap_end(m_lpCmd->soap);

	pthread_mutex_unlock(&m_hSoapLock);
	return erSuccess;
}

inline void WSStreamOps::SetMode(eMode mode)
{
	m_eMode = mode;
}

inline WSStreamOps::eMode WSStreamOps::GetMode() const
{
	return m_eMode;
}

HRESULT WSStreamOps::WriteBuf(const char *lpsz, unsigned long cb)
{
	HRESULT			hr = hrSuccess;
	ECRESULT		er = erSuccess;
	ECFifoBuffer	*lpsFifoBuffer = NULL;
	
	pthread_mutex_lock(&m_hBufferLock);

	if (m_bDone) {
		hr = MAPI_E_CALL_FAILED;
		goto exit;
	}
	
	if (m_sBufferList.empty() || m_sBufferList.back()->IsClosed()) {
		// Wait if we would exceed the max buffercount
		while (m_sBufferList.size() >= m_ulMaxBufferCount && !m_bDone)
			pthread_cond_wait(&m_hBufferCond, &m_hBufferLock);

		if (m_bDone) {
			hr = MAPI_E_CALL_FAILED;
			goto exit;
		}

		m_sBufferList.push_back(new ECFifoBuffer(m_ulMaxBufferSize));
	}

	lpsFifoBuffer = m_sBufferList.back();

	if (lpsz == NULL) {
		if (cb == WSSO_MARK_EOF || cb == WSSO_MARK_EOS) {

			pthread_mutex_unlock(&m_hBufferLock);
			lpsFifoBuffer->Close();
			pthread_mutex_lock(&m_hBufferLock);

			if (cb == WSSO_MARK_EOS)
				m_bDone = true;
		} else
			hr = MAPI_E_INVALID_PARAMETER;

		goto exit;
	}

	pthread_mutex_unlock(&m_hBufferLock);
	er = m_sBufferList.back()->Write(lpsz, cb, STR_DEF_TIMEOUT, NULL);
	pthread_mutex_lock(&m_hBufferLock);

	if (er != erSuccess) {
		hr = ZarafaErrorToMAPIError(er, MAPI_E_CALL_FAILED);
		goto exit;
	}
	
exit:
	pthread_cond_signal(&m_hBufferCond);
	pthread_mutex_unlock(&m_hBufferLock);

	return hr;
}

HRESULT WSStreamOps::ReadBuf(char *lpsz, unsigned long cb, unsigned long ulFlags, unsigned long *lpcbRead)
{
	HRESULT					hr = hrSuccess;
	ECRESULT				er = erSuccess;
	ECFifoBuffer			*lpsFifoBuffer = NULL;
	unsigned long			cbRead = 0;
	ECFifoBuffer::size_type	cbNow = 0;
	
	if (lpsz == NULL || cb == 0 || lpcbRead == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}
	
	pthread_mutex_lock(&m_hBufferLock);

	// @todo: Fix this loop to handle timeout from ECFifoBuffer::Read and make use
	//        of the fact that ECFifoBuffer will read as much as possible without
	//        returning.
	while (cbRead < cb) {
		if (m_sBufferList.empty()) {
			if (m_bDone)
				break;
			else
				pthread_cond_wait(&m_hBufferCond, &m_hBufferLock);	// @todo: use a timeout here
		} else {
			lpsFifoBuffer = m_sBufferList.front();

			pthread_mutex_unlock(&m_hBufferLock);
			er = lpsFifoBuffer->Read(lpsz + cbRead, cb - cbRead, STR_DEF_TIMEOUT, &cbNow);
			pthread_mutex_lock(&m_hBufferLock);

			if (er != erSuccess) {
				hr = ZarafaErrorToMAPIError(er, MAPI_E_CALL_FAILED);
				goto exit_loop;
			}
			cbRead += cbNow;
			if (cbRead < cb) {
				delete m_sBufferList.front();
				m_sBufferList.pop_front();
				if ((ulFlags & WSSO_FLG_IGNORE_EOF) == 0)
					break;
			}
		}
	}

	*lpcbRead = cbRead;

exit_loop:
	pthread_cond_signal(&m_hBufferCond);
	pthread_mutex_unlock(&m_hBufferLock);

exit:
	return hr;
}

HRESULT WSStreamOps::FinishExportMessageChangesAsStream(void *lpvArg)
{
	HRESULT				hr = hrSuccess;
	WSStreamOps			*lpsStreamOps = NULL;
	sourceKeyPairArray	*lpsSourceKeyPairs = NULL;
	struct soap			*lpsSoap = NULL;
	propTagArray		sPropTags = {0};
	ECStreamInfo		sStreamInfo = {0};
	ULONG				ulFlags = 0;
	exportMessageChangesAsStreamResponse	sResponse = {{0}};
	convert_context		converter;
	
	// Internal use only, so an assert will do. 
	assert(lpvArg != NULL);
		
	// Copy the arguments
	lpsStreamOps = ((WSSO_EmcasArgs*)lpvArg)->lpStreamOps;
	lpsSourceKeyPairs = ((WSSO_EmcasArgs*)lpvArg)->lpsSourceKeyPairs;
	sPropTags = ((WSSO_EmcasArgs*)lpvArg)->sPropTags;
	ulFlags = ((WSSO_EmcasArgs*)lpvArg)->ulFlags;
	lpsSoap = lpsStreamOps->m_lpCmd->soap;

	
	lpsStreamOps->LockSoap();

	// Handle the MTOM attachment ourself
	soap_post_check_mime_attachments(lpsSoap);	
	lpsSoap->fmimewriteopen = &MTOMWriteOpen;
	lpsSoap->fmimewrite = &MTOMWrite;
	lpsSoap->fmimewriteclose = &MTOMWriteClose;
	
	if (lpsStreamOps->m_lpCmd->ns__exportMessageChangesAsStream(lpsStreamOps->m_ecSessionId, ulFlags, sPropTags, *lpsSourceKeyPairs, &sResponse) != SOAP_OK) {
		pthread_mutex_lock(&lpsStreamOps->m_hStreamInfoLock);
		lpsStreamOps->m_bStreamInfoState = Failed;
		pthread_cond_broadcast(&lpsStreamOps->m_hStreamInfoCond);
		pthread_mutex_unlock(&lpsStreamOps->m_hStreamInfoLock);

		hr = MAPI_E_NETWORK_ERROR;
	} else
		hr = ZarafaErrorToMAPIError(sResponse.er, MAPI_E_NOT_FOUND);
	
	pthread_mutex_lock(&lpsStreamOps->m_hStreamInfoLock);

	// Extract the stream info and store it for later use
	for (unsigned i = 0; i < sResponse.sMsgStreams.__size; ++i) {
		//sResponse.sMsgStreams.__ptr[i].sStreamData.xop__Include.id;
		sStreamInfo.ulStep = sResponse.sMsgStreams.__ptr[i].ulStep;
		sStreamInfo.cbPropVals = sResponse.sMsgStreams.__ptr[i].sPropVals.__size;
		MAPIAllocateBuffer(sResponse.sMsgStreams.__ptr[i].sPropVals.__size * sizeof *sStreamInfo.lpsPropVals, (void**)&sStreamInfo.lpsPropVals);
		for (int j = 0; j < sResponse.sMsgStreams.__ptr[i].sPropVals.__size; ++j)
			CopySOAPPropValToMAPIPropVal(&sStreamInfo.lpsPropVals[j], &sResponse.sMsgStreams.__ptr[i].sPropVals.__ptr[j], sStreamInfo.lpsPropVals, &converter);

		lpsStreamOps->m_mapStreamInfo.insert(StreamInfoMap::value_type(sResponse.sMsgStreams.__ptr[i].sStreamData.xop__Include.id, sStreamInfo));
	}

	lpsStreamOps->m_bStreamInfoState = Ready;
	pthread_cond_broadcast(&lpsStreamOps->m_hStreamInfoCond);
	pthread_mutex_unlock(&lpsStreamOps->m_hStreamInfoLock);

	if (soap_check_mime_attachments(lpsSoap)) {    // attachments are present, channel is still open
		struct soap_multipart *content;
		while (true) {
			content = soap_get_mime_attachment(lpsSoap, (void*)lpsStreamOps);
			if (!content)
				break;
		};
        if (lpsSoap->error) {
			hr = MAPI_E_NETWORK_ERROR;
			goto exit;
		}
	} 
	
exit:
	lpsStreamOps->WriteBuf(NULL, WSSO_MARK_EOS);
	lpsStreamOps->UnLockSoap();

	if (lpsSourceKeyPairs)
		MAPIFreeBuffer(lpsSourceKeyPairs);
	
	if (((WSSO_EmcasArgs*)lpvArg)->sPropTags.__ptr)
		delete[] ((WSSO_EmcasArgs*)lpvArg)->sPropTags.__ptr;
	delete (WSSO_EmcasArgs*)lpvArg;

	return hr;
}

HRESULT WSStreamOps::FinishImportMessageFromStream(void *lpvArg)
{
	HRESULT			hr = hrSuccess;
	WSStreamOps		*lpsStreamOps = NULL;
	WSSO_ImfsArgs	*lpArgs = NULL;
	struct soap		*lpsSoap = NULL;
	unsigned int	ulResult = 0;
	xsd__Binary		sStreamData = {{0}};
	
	if (lpvArg == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}
		
	// Copy the arguments
	lpArgs = (WSSO_ImfsArgs*)lpvArg;
	lpsStreamOps = lpArgs->lpStreamOps;
	lpsSoap = lpsStreamOps->m_lpCmd->soap;

	sStreamData.xop__Include.__ptr = (unsigned char*)lpsStreamOps;
	sStreamData.xop__Include.type = s_strcpy(lpsSoap, "application/binary");
	
	lpsStreamOps->LockSoap();

	soap_set_omode(lpsSoap, SOAP_ENC_MTOM | SOAP_IO_CHUNK);	
	lpsSoap->fmimereadopen = &MTOMReadOpen;
	lpsSoap->fmimeread = &MTOMRead;
	lpsSoap->fmimereadclose = &MTOMReadClose;
	
	if (lpsStreamOps->m_lpCmd->ns__importMessageFromStream(lpsStreamOps->m_ecSessionId, lpArgs->ulFlags, lpArgs->ulSyncId, lpArgs->sFolderEntryId, lpArgs->sEntryId, lpArgs->bIsNew, lpArgs->lpsConflictItems, sStreamData, &ulResult) != SOAP_OK)
		hr = MAPI_E_NETWORK_ERROR;
	else
		hr = ZarafaErrorToMAPIError(ulResult, MAPI_E_NOT_FOUND);
	
exit:
	lpsStreamOps->UnLockSoap();
	
	if (lpArgs->sEntryId.__ptr)
		delete[] lpArgs->sEntryId.__ptr;
	if (lpArgs->sFolderEntryId.__ptr)
		delete[] lpArgs->sFolderEntryId.__ptr;
	if (lpArgs->lpsConflictItems)
		FreePropVal(lpArgs->lpsConflictItems, true);
	
	delete lpArgs;

	return hr;
}

void *WSStreamOps::MTOMWriteOpen(struct soap* /*soap*/, void *handle, const char *id, const char* /*type*/, const char* /*description*/, enum soap_mime_encoding encoding)
{
	WSStreamOps		*lpsStreamOps = (WSStreamOps*)handle;
	unsigned char	cbIdLen = 0;
	
	if (strncmp(id, "emcas-", 6) == 0 && encoding == SOAP_MIME_BINARY) {
		// Write the length of the id and the id in the stream as the other end will
		// need that to extract vital information.
		cbIdLen = strlen(id);
		lpsStreamOps->WriteBuf((char*)&cbIdLen, sizeof(cbIdLen));
		lpsStreamOps->WriteBuf(id, cbIdLen);
		lpsStreamOps->m_bAttachValid = true;
	} else {
		lpsStreamOps->m_bAttachValid = false;
	}
	
	return handle;
}

int WSStreamOps::MTOMWrite(struct soap* /*soap*/, void *handle, const char *buf, size_t len)
{
	HRESULT		hr;
	WSStreamOps	*lpsStreamOps = (WSStreamOps*)handle;
	
	if (lpsStreamOps->GetMode() == mRead && lpsStreamOps->m_bAttachValid) {
		hr = lpsStreamOps->WriteBuf(buf, len);
		if (hr != hrSuccess)
			lpsStreamOps->m_bAttachValid = false;
	}
	
	return SOAP_OK;
}

void WSStreamOps::MTOMWriteClose(struct soap* /*soap*/, void *handle)
{
	WSStreamOps	*lpsStreamOps = (WSStreamOps*)handle;
	
	if (lpsStreamOps->GetMode() == mRead && lpsStreamOps->m_bAttachValid) {
		lpsStreamOps->WriteBuf(NULL, WSSO_MARK_EOF);
		lpsStreamOps->m_bAttachValid = false;
	}
}

void *WSStreamOps::MTOMReadOpen(struct soap* /*soap*/, void *handle, const char* /*id*/, const char* /*type*/, const char* /*description*/)
{
	return handle;
}

size_t WSStreamOps::MTOMRead(struct soap* /*soap*/, void *handle, char *buf, size_t len)
{
	WSStreamOps		*lpsStreamOps = NULL;
	unsigned long	cbRead = 0;

	if (handle == NULL)
		return 0;

	lpsStreamOps = (WSStreamOps*)handle;
	lpsStreamOps->ReadBuf(buf, len, WSSO_FLG_IGNORE_EOF, &cbRead);

	return cbRead;
}

void WSStreamOps::MTOMReadClose(struct soap* /*soap*/, void * /*handle*/)
{
}


// IUnknown
HRESULT __stdcall WSStreamOps::xUnknown::QueryInterface(REFIID refiid, void **lppInterface)
{
	METHOD_PROLOGUE_(WSStreamOps , Unknown);
	HRESULT hr = pThis->QueryInterface(refiid, lppInterface);
	return hr;
}

ULONG __stdcall WSStreamOps::xUnknown::AddRef()
{
	METHOD_PROLOGUE_(WSStreamOps , Unknown);
	return pThis->AddRef();
}

ULONG __stdcall WSStreamOps::xUnknown::Release()
{
	METHOD_PROLOGUE_(WSStreamOps , Unknown);
	return pThis->Release();
}


// IUnknown
ULONG WSStreamOps::xStream::AddRef()
{
	TRACE_MAPI(TRACE_ENTRY, "IStream::AddRef", "");
	METHOD_PROLOGUE_(WSStreamOps, Stream);
	ULONG ref = pThis->AddRef();
	TRACE_MAPI(TRACE_RETURN, "IStream::AddRef", "%u", ref);
	return ref;
}

ULONG WSStreamOps::xStream::Release()
{
	TRACE_MAPI(TRACE_ENTRY, "IStream::Release", "");
	METHOD_PROLOGUE_(WSStreamOps, Stream);
	ULONG ref = pThis->Release();
	TRACE_MAPI(TRACE_RETURN, "IStream::Release", "%u", ref);
	return ref;
}

HRESULT WSStreamOps::xStream::QueryInterface(REFIID refiid, void **lppInterface)
{
	TRACE_MAPI(TRACE_ENTRY, "IStream::QueryInterface", "%s", DBGGUIDToString(refiid).c_str());
	METHOD_PROLOGUE_(WSStreamOps , Stream);
	HRESULT hr = pThis->QueryInterface(refiid, lppInterface);
	TRACE_MAPI(TRACE_RETURN, "IStream::QueryInterface", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

// ISequentialStream
HRESULT WSStreamOps::xStream::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	TRACE_MAPI(TRACE_ENTRY, "IStream::Read", "%p, %u", pv, cb);
	METHOD_PROLOGUE_(WSStreamOps, Stream);
	HRESULT hr = pThis->Read(pv, cb, pcbRead);
	TRACE_MAPI(TRACE_RETURN, "IStream::Read", "%u, %s", pcbRead ? *pcbRead : 0, GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT WSStreamOps::xStream::Write(const void *pv, ULONG cb, ULONG *pcbWritten)
{
	TRACE_MAPI(TRACE_ENTRY, "IStream::Write", "%p, %u", pv, cb);
	METHOD_PROLOGUE_(WSStreamOps, Stream);
	HRESULT hr = pThis->Write(pv, cb, pcbWritten);
	TRACE_MAPI(TRACE_RETURN, "IStream::Write", "%u, %s", pcbWritten ? *pcbWritten : 0, GetMAPIErrorDescription(hr).c_str());
	return hr;
}

// IStream
HRESULT WSStreamOps::xStream::Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition)
{
	TRACE_MAPI(TRACE_ENTRY, "IStream::Seek", "");
	METHOD_PROLOGUE_(WSStreamOps, Stream);
	HRESULT hr = pThis->Seek(dlibMove, dwOrigin, plibNewPosition);
	TRACE_MAPI(TRACE_RETURN, "IStream::Seek", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT WSStreamOps::xStream::SetSize(ULARGE_INTEGER libNewSize)
{
	TRACE_MAPI(TRACE_ENTRY, "IStream::SetSize", "");
	METHOD_PROLOGUE_(WSStreamOps, Stream);
	HRESULT hr = pThis->SetSize(libNewSize);
	TRACE_MAPI(TRACE_RETURN, "IStream::SetSize", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT WSStreamOps::xStream::CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
	TRACE_MAPI(TRACE_ENTRY, "IStream::CopyTo", "%p, %s", pstm, stringify_int64(cb.QuadPart).c_str());
	METHOD_PROLOGUE_(WSStreamOps, Stream);
	HRESULT hr = pThis->CopyTo(pstm, cb, pcbRead, pcbWritten);
	TRACE_MAPI(TRACE_RETURN, "IStream::CopyTo", "%s, %s, %s", 
		pcbRead ? stringify_int64(pcbRead->QuadPart).c_str() : "0",
		pcbWritten ? stringify_int64(pcbWritten->QuadPart).c_str() : "0",
		GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT WSStreamOps::xStream::Commit(DWORD grfCommitFlags)
{
	TRACE_MAPI(TRACE_ENTRY, "IStream::Commit", "");
	METHOD_PROLOGUE_(WSStreamOps, Stream);
	HRESULT hr = pThis->Commit(grfCommitFlags);
	TRACE_MAPI(TRACE_RETURN, "IStream::Commit", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT WSStreamOps::xStream::Revert(void)
{
	TRACE_MAPI(TRACE_ENTRY, "IStream::Revert", "");
	METHOD_PROLOGUE_(WSStreamOps, Stream);
	HRESULT hr = pThis->Revert();
	TRACE_MAPI(TRACE_RETURN, "IStream::Revert", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT WSStreamOps::xStream::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	TRACE_MAPI(TRACE_ENTRY, "IStream::LockRegion", "");
	METHOD_PROLOGUE_(WSStreamOps, Stream);
	HRESULT hr = pThis->LockRegion(libOffset, cb, dwLockType);
	TRACE_MAPI(TRACE_RETURN, "IStream::LockRegion", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT WSStreamOps::xStream::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
	TRACE_MAPI(TRACE_ENTRY, "IStream::UnlockRegion", "");
	METHOD_PROLOGUE_(WSStreamOps, Stream);
	HRESULT hr = pThis->UnlockRegion(libOffset, cb, dwLockType);
	TRACE_MAPI(TRACE_RETURN, "IStream::UnlockRegion", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT WSStreamOps::xStream::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
	TRACE_MAPI(TRACE_ENTRY, "IStream::Stat", "");
	METHOD_PROLOGUE_(WSStreamOps, Stream);
	HRESULT hr = pThis->Stat(pstatstg, grfStatFlag);
	TRACE_MAPI(TRACE_RETURN, "IStream::Stat", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}

HRESULT WSStreamOps::xStream::Clone(IStream **ppstm)
{
	TRACE_MAPI(TRACE_ENTRY, "IStream::Clone", "");
	METHOD_PROLOGUE_(WSStreamOps, Stream);
	HRESULT hr = pThis->Clone(ppstm);
	TRACE_MAPI(TRACE_RETURN, "IStream::Clone", "%s", GetMAPIErrorDescription(hr).c_str());
	return hr;
}
