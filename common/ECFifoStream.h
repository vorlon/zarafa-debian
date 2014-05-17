/*
 * Copyright 2005 - 2014  Zarafa B.V.
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

#ifndef ECFIFOSTREAM_H
#define ECFIFOSTREAM_H

#include "ECUnknown.h"

class ECFifoBuffer;

/**
 * This is a FIFO stream that exposes two separate IStream interfaces, one
 * for reading, and one for writing. The semantics are much like the pipe()
 * system call; A reader will block while the writer has not yet sent enough
 * data, but once the writer has released it's interface, the reader will receive
 * an EOF when no more data is available. Conversely, when a reader has closed
 * its connection, the writer will receive a write error when attempting to write
 *
 * Each object supports normal reference counting using AddRef/Release.
 *
 * Although we expose the full IStream interface, the reader and writer each only
 * support the IStream::Read() or IStream::Write() calls. All other calls will
 * return MAPI_E_NO_SUPPORT.
 */

HRESULT CreateFifoStreamPair(IStream **lppReader, IStream **lppWriter);

/**
 * This class exists only to apply refcounting to ECFifoBuffer
 */
class ECFifoStream : public ECUnknown {
public:
    static HRESULT Create(ECFifoStream **lppFifoStream);
    
    ECFifoBuffer *m_lpFifoBuffer;

private:
    ECFifoStream();
    ~ECFifoStream();
};

// Class defs for reader and writer classes
class ECFifoStreamReader : public ECUnknown {
public:
    static HRESULT Create(ECFifoStream *lpBuffer, ECFifoStreamReader **lppReader);
    
    HRESULT QueryInterface(REFIID refiid, LPVOID *lppInterface);
    HRESULT Read(void *pv, ULONG cb, ULONG *pcbRead);
    
private:
    ECFifoStreamReader(ECFifoStream *lpBuffer);
    ~ECFifoStreamReader();
    
    class xStream : public IStream {
    public:
        virtual ULONG   __stdcall AddRef();
        virtual ULONG   __stdcall Release();
        virtual HRESULT __stdcall QueryInterface(REFIID refiid, LPVOID *lppInterface);

        virtual HRESULT __stdcall Read(void *pv, ULONG cb, ULONG *pcbRead);
        virtual HRESULT __stdcall Write(const void *pv, ULONG cb, ULONG *pcbWritten);
        virtual HRESULT __stdcall Seek(LARGE_INTEGER dlibmove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
        virtual HRESULT __stdcall SetSize(ULARGE_INTEGER libNewSize);
        virtual HRESULT __stdcall CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
        virtual HRESULT __stdcall Commit(DWORD grfCommitFlags);
        virtual HRESULT __stdcall Revert();
        virtual HRESULT __stdcall LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
        virtual HRESULT __stdcall UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
        virtual HRESULT __stdcall Stat(STATSTG *pstatstg, DWORD grfStatFlag);
        virtual HRESULT __stdcall Clone(IStream **ppstm);
    } m_xStream;
    
    ECFifoStream *m_lpFifoBuffer;
};

class ECFifoStreamWriter : public ECUnknown {
public:
    static HRESULT Create(ECFifoStream *lpBuffer, ECFifoStreamWriter **lppWriter);
    
    HRESULT QueryInterface(REFIID refiid, LPVOID *lppInterface);
    HRESULT Write(const void *pv, ULONG cb, ULONG *pcbWritten);
    HRESULT Commit(DWORD grfCommitFlags);
    
private:
    ECFifoStreamWriter(ECFifoStream *lpBuffer);
    ~ECFifoStreamWriter();
    
    class xStream : public IStream {
    public:
        virtual ULONG   __stdcall AddRef();
        virtual ULONG   __stdcall Release();
        virtual HRESULT __stdcall QueryInterface(REFIID refiid, LPVOID *lppInterface);

        virtual HRESULT __stdcall Read(void *pv, ULONG cb, ULONG *pcbRead);
        virtual HRESULT __stdcall Write(const void *pv, ULONG cb, ULONG *pcbWritten);
        virtual HRESULT __stdcall Seek(LARGE_INTEGER dlibmove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
        virtual HRESULT __stdcall SetSize(ULARGE_INTEGER libNewSize);
        virtual HRESULT __stdcall CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
        virtual HRESULT __stdcall Commit(DWORD grfCommitFlags);
        virtual HRESULT __stdcall Revert();
        virtual HRESULT __stdcall LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
        virtual HRESULT __stdcall UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
        virtual HRESULT __stdcall Stat(STATSTG *pstatstg, DWORD grfStatFlag);
        virtual HRESULT __stdcall Clone(IStream **ppstm);
    } m_xStream;
    
    ECFifoStream *m_lpFifoBuffer;
};

#endif
