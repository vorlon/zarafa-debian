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

#ifndef WSSTREAMOPS_H
#define WSSTREAMOPS_H

#include "IECUnknown.h"
#include "Zarafa.h"
#include "ZarafaCode.h"
#include "soapZarafaCmdProxy.h"
#include "ECICS.h"
#include "ECFifoBuffer.h"
#include "ECThreadPool.h"

#include <vector>
#include <map>
#include <set>
#include <string>
#include <list>
#include <memory>

#include <mapi.h>
#include <mapispi.h>
#include <pthread.h>

struct ECStreamInfo;

class WSStreamOps : public IECUnknown
{
protected:
	WSStreamOps(ZarafaCmd *lpCmd, ECSESSIONID ecSessionId, ULONG cbFolderEntryId, LPENTRYID lpFolderEntryId, unsigned int ulServerCapabilities);
	virtual ~WSStreamOps();

public:
	static HRESULT Create(ZarafaCmd *lpCmd, ECSESSIONID ecSessionId, ULONG cbFolderEntryId, LPENTRYID lpFolderEntryId, unsigned int ulServerCapabilities, WSStreamOps **lppStreamOps);
	
	// Streaming
	virtual HRESULT HrStartExportMessageChangesAsStream(ULONG ulFlags, const std::vector<ICSCHANGE> &sChanges, LPSPropTagArray lpsPropTags);
	virtual HRESULT HrStartImportMessageFromStream(ULONG ulFlags, ULONG ulSyncId, ULONG cbEntryID, LPENTRYID lpEntryID, bool bIsNew, LPSPropValue lpConflictItems);

	virtual HRESULT GetStreamInfo(const char *lpszId, ECStreamInfo *lpsStreamInfo);
	virtual HRESULT GetSteps(std::set<unsigned long> *lpsetSteps);
	virtual HRESULT CopyFrameTo(IStream *pstm, ULARGE_INTEGER *pcbCopied);
	virtual HRESULT FlushFrame();

	virtual HRESULT CloseAndGetAsyncResult(HRESULT *lphrResult);

	// IUnknown
	virtual ULONG AddRef();
	virtual ULONG Release();
	virtual HRESULT QueryInterface(REFIID refiid, void **lppInterface);
	
	// ISequentialStream
	virtual HRESULT Read(void *pv, ULONG cb, ULONG *pcbRead);
	virtual HRESULT Write(const void *pv, ULONG cb, ULONG *pcbWritten);

	// IStream
	virtual HRESULT Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
	virtual HRESULT SetSize(ULARGE_INTEGER libNewSize);
	virtual HRESULT CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
	virtual HRESULT Commit(DWORD grfCommitFlags);
	virtual HRESULT Revert(void);
	virtual HRESULT LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
	virtual HRESULT UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
	virtual HRESULT Stat(STATSTG *pstatstg, DWORD grfStatFlag);
	virtual HRESULT Clone(IStream **ppstm);

protected:
	virtual HRESULT LockSoap();
	virtual HRESULT UnLockSoap();
	
private:
	// What operation is allowed on the IStream interface
	enum eMode {mNone, mRead, mWrite};

	typedef std::map<std::string, ECStreamInfo> StreamInfoMap;

	// Internal operations
	void SetMode(eMode mode);
	eMode GetMode() const;
	HRESULT WriteBuf(const char *lpsz, unsigned long cb);
	HRESULT ReadBuf(char *lpsz, unsigned long cb, unsigned long ulFlags, unsigned long *lpcbRead);
	
	// Thread
	static HRESULT FinishExportMessageChangesAsStream(void *lpvArg);
	static HRESULT FinishImportMessageFromStream(void *lpvArg);
	ECThreadPool *GetThreadPool();
	
	// gSoap callbacks
	static void  *MTOMWriteOpen(struct soap *soap, void *handle, const char *id, const char *type, const char *description, enum soap_mime_encoding encoding);
	static int    MTOMWrite(struct soap *soap, void *handle, const char *buf, size_t len);
	static void   MTOMWriteClose(struct soap *soap, void *handle);
	static void  *MTOMReadOpen(struct soap *soap, void *handle, const char *id, const char *type, const char *description);
	static size_t MTOMRead(struct soap *soap, void *handle, char *buf, size_t len);
	static void   MTOMReadClose(struct soap *soap, void *handle);
	
private:
	class xUnknown : public IUnknown
	{
	public:
		// From IUnknown
		virtual HRESULT __stdcall QueryInterface(REFIID refiid, void** lppInterface);
		virtual ULONG __stdcall AddRef();
		virtual ULONG __stdcall Release();	
	} m_xUnknown;

	class xStream : public IStream
	{
		// IUnknown
	    virtual ULONG __stdcall AddRef();
	    virtual ULONG __stdcall Release();
	    virtual HRESULT __stdcall QueryInterface(REFIID refiid, void **lppInterface);
		
		// ISequentialStream
		virtual HRESULT __stdcall Read(void *pv, ULONG cb, ULONG *pcbRead);
		virtual HRESULT __stdcall Write(const void *pv, ULONG cb, ULONG *pcbWritten);
	
		// IStream
	    virtual HRESULT __stdcall Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
	    virtual HRESULT __stdcall SetSize(ULARGE_INTEGER libNewSize);
	    virtual HRESULT __stdcall CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
	    virtual HRESULT __stdcall Commit(DWORD grfCommitFlags);
	    virtual HRESULT __stdcall Revert(void);
	    virtual HRESULT __stdcall LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
	    virtual HRESULT __stdcall UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
	    virtual HRESULT __stdcall Stat(STATSTG *pstatstg, DWORD grfStatFlag);
	    virtual HRESULT __stdcall Clone(IStream **ppstm);
	} m_xStream;

private:
	ULONG				m_ulRef;
	ZarafaCmd*			m_lpCmd;				// command object
	ECSESSIONID			m_ecSessionId;			// Id of the session
	unsigned int		m_ulServerCapabilities;
	pthread_mutex_t		m_hSoapLock;
	
	LPENTRYID			m_lpFolderEntryId;
	ULONG				m_cbFolderEntryId;

	bool				m_bAttachValid;

	typedef ECDeferredFunc<HRESULT, HRESULT(*)(void*), void*>	DeferredFunc;
	std::auto_ptr<DeferredFunc>	m_ptrDeferredFunc;
	
	eMode				m_eMode;
	unsigned int		m_ulMaxBufferSize;
	unsigned int		m_ulMaxBufferCount;
	bool				m_bDone;
	
	std::list<ECFifoBuffer*>	m_sBufferList;
	pthread_mutex_t		m_hBufferLock;
	pthread_cond_t		m_hBufferCond;

	enum StreamInfoState { Pending, Ready, Failed };

	StreamInfoMap		m_mapStreamInfo;
	StreamInfoState		m_bStreamInfoState;
	pthread_mutex_t		m_hStreamInfoLock;
	pthread_cond_t		m_hStreamInfoCond;

	ECThreadPool		m_threadPool;
};

#endif // ndef WSSTREAMOPS_H
