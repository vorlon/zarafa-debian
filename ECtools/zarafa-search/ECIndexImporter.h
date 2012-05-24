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

#ifndef ECINDEXIMPORTER_H
#define ECINDEXIMPORTER_H

#include <mapidefs.h>
#include <edkmdb.h>

#include "ECUnknown.h"
#include "IECImportContentsChanges.h"
#include "mapi_ptr.h"

#include "ECIndexDB.h"
#include <mapidefs.h>

#include <string>

class ECLogger;
class ECConfig;
class ECThreadData;
class ECIndexDB;
class ECIndexImporterAttachment;
class ECSerializer;

class ECIndexImporter : public ECUnknown {
public:
    static HRESULT Create(ECConfig *lpConfig, ECLogger *lpLogger, ECThreadData *lpThreadData, ECIndexDB *lpIndex, GUID guidServer, ECIndexImporter **lppImporter);
    
    HRESULT QueryInterface(REFIID iid, void **lpUnk);

	HRESULT GetLastError(HRESULT hResult, ULONG ulFlags, LPMAPIERROR *lppMAPIError);
	HRESULT Config(LPSTREAM lpStream, ULONG ulFlags);
	HRESULT UpdateState(LPSTREAM lpStream);
	HRESULT ImportMessageChange(ULONG cValue, LPSPropValue lpPropArray, ULONG ulFlags, LPMESSAGE * lppMessage);
	HRESULT ImportMessageDeletion(ULONG ulFlags, LPENTRYLIST lpSourceEntryList);
	HRESULT ImportPerUserReadStateChange(ULONG cElements, LPREADSTATE lpReadState);
	HRESULT ImportMessageMove(ULONG cbSourceKeySrcFolder, BYTE FAR * pbSourceKeySrcFolder, ULONG cbSourceKeySrcMessage, BYTE FAR * pbSourceKeySrcMessage, ULONG cbPCLMessage, BYTE FAR * pbPCLMessage, ULONG cbSourceKeyDestMessage, BYTE FAR * pbSourceKeyDestMessage, ULONG cbChangeNumDestMessage, BYTE FAR * pbChangeNumDestMessage);

    HRESULT ConfigForConversionStream(LPSTREAM lpStream, ULONG ulFlags, ULONG cValuesConversion, LPSPropValue lpPropArrayConversion);
    HRESULT ImportMessageChangeAsAStream(ULONG cpvalChanges, LPSPropValue ppvalChanges, ULONG ulFlags, LPSTREAM *lppstream);
    HRESULT SetMessageInterface(REFIID refiid);

    HRESULT GetStats(ULONG *lpulCreated, ULONG *lpulChanges, ULONG *lpulDeleted, ULONG *lpulBytes);
    
private:
    // This is the stream that is currently being processed by the processing thread. If it is NULL,
    // then the thread is done processing the stream. The IDs are the IDs associated with lpStream
    
    StreamPtr m_lpStream;
    // The following are all associated with lpStream
    unsigned int m_ulFolderId;
    unsigned int m_ulDocId;
    unsigned int m_ulFlags; 		
    GUID m_guidStore;					
    std::string m_strSourceKey;
    
    pthread_mutex_t m_mutexStream; 	// Protects m_lpStream
    pthread_cond_t m_condStream;	// Signals that m_lpStream is readable again
    bool m_bExit;
    bool m_bThreadStarted;
    
    ECIndexDB *m_lpIndex;
    
    pthread_t m_thread;

    GUID m_guidServer;
    
    ECLogger *m_lpLogger;
    ECConfig *m_lpConfig;
    ECThreadData *m_lpThreadData;
    
    unsigned int m_ulCreated;
    unsigned int m_ulChanged;
    unsigned int m_ulDeleted;
    unsigned int m_ulBytes;
    
    ECIndexImporterAttachment *m_lpIndexerAttach;

    class xECImportContentsChanges : public IECImportContentsChanges {
        virtual ULONG	__stdcall AddRef();
        virtual ULONG	__stdcall Release();
        virtual HRESULT __stdcall QueryInterface(REFIID iid, void **lppUnk);
        
        virtual HRESULT __stdcall GetLastError(HRESULT hResult, ULONG ulFlags, LPMAPIERROR *lppMAPIError);
        virtual HRESULT __stdcall Config(LPSTREAM lpStream, ULONG ulFlags);
        virtual HRESULT __stdcall UpdateState(LPSTREAM lpStream);
        virtual HRESULT __stdcall ImportMessageChange(ULONG cValue, LPSPropValue lpPropArray, ULONG ulFlags, LPMESSAGE * lppMessage);
        virtual HRESULT __stdcall ImportMessageDeletion(ULONG ulFlags, LPENTRYLIST lpSourceEntryList);
        virtual HRESULT __stdcall ImportPerUserReadStateChange(ULONG cElements, LPREADSTATE lpReadState);
        virtual HRESULT __stdcall ImportMessageMove(ULONG cbSourceKeySrcFolder, BYTE FAR * pbSourceKeySrcFolder, ULONG cbSourceKeySrcMessage, BYTE FAR * pbSourceKeySrcMessage, ULONG cbPCLMessage, BYTE FAR * pbPCLMessage, ULONG cbSourceKeyDestMessage, BYTE FAR * pbSourceKeyDestMessage, ULONG cbChangeNumDestMessage, BYTE FAR * pbChangeNumDestMessage);

        virtual HRESULT __stdcall ConfigForConversionStream(LPSTREAM lpStream, ULONG ulFlags, ULONG cValuesConversion, LPSPropValue lpPropArrayConversion);
        virtual HRESULT __stdcall ImportMessageChangeAsAStream(ULONG cpvalChanges, LPSPropValue ppvalChanges, ULONG ulFlags, LPSTREAM *lppstream);
        virtual HRESULT __stdcall SetMessageInterface(REFIID refiid);
                        
    } m_xECImportContentsChanges;

    ECIndexImporter(ECConfig *lpConfig, ECLogger *lpLogger, ECThreadData *lpThreadData, ECIndexDB *lpIndex, GUID guidServer);
    ~ECIndexImporter();

    HRESULT StartThread();
    HRESULT ProcessThread();
    static void *Thread(void *lpArg);

    HRESULT ParseStream(folderid_t folder, docid_t doc, unsigned int version, ECSerializer *lpSerializer, ECIndexDB *lpIndex, BOOL bTop);
    HRESULT ParseStreamAttachments(folderid_t folder, docid_t doc, unsigned int version, ECSerializer *lpSerializer, ECIndexDB *lpIndex);

    friend class ECIndexImporterAttachment;
    
};

#endif
