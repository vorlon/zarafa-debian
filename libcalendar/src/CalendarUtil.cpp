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

#include "platform.h"
#include "CalendarUtil.h"
#include "Conversions.h"
#include "mapi_ptr.h"

/**
 * Write a BLOB to a property on a MAPI object by opening the property as stream.
 *
 * @param[in]	lpMapiProp	The MAPI object on which to operate.
 * @param[in]	ulPropTag	The proptag of the property to write the
 *           	         	BLOB in. This must be a PT_BINARY type.
 * @param[in]	cbBlob		The size in bytes of the BLOB.
 * @param[in]	lpBlob		Pointer to the BLOB.
 */
HRESULT WriteBlob(IMAPIProp *lpMapiProp, ULONG ulPropTag, ULONG cbBlob, LPBYTE lpBlob)
{
	HRESULT hr = hrSuccess;
	StreamPtr ptrStream;

	const static ULARGE_INTEGER uliZero = {{0, 0}};

	if (lpMapiProp == NULL || PROP_TYPE(ulPropTag) != PT_BINARY || lpBlob == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	hr = lpMapiProp->OpenProperty(ulPropTag, &IID_IStream, 0, MAPI_CREATE | MAPI_MODIFY, &ptrStream);
	if (hr != hrSuccess)
		goto exit;

	hr = ptrStream->SetSize(uliZero);
	if (hr != hrSuccess)
		goto exit;

	hr = ptrStream->Write(lpBlob, cbBlob, NULL);
	if (hr != hrSuccess)
		goto exit;

	hr = ptrStream->Commit(0);
	if (hr != hrSuccess)
		goto exit;

exit:
	return hr;
}


typedef struct __attribute__((__packed__)) _GOID {
	GUID ByteArrayID;
	BYTE YH;
	BYTE YL;
	BYTE M;
	BYTE D;
	FILETIME CreationTime;
	LONGLONG X;
	LONG Size;
	BYTE Data[1];
} GOID;

#define CbNewGOID(_cb)      (offsetof(GOID, Data) + (_cb))

static const GUID GOID_BAID ={0x00000004, 0x0082, 0x00e0, {0x74, 0xc5, 0xb7, 0x10, 0x1a, 0x82, 0xe0, 0x08}};


/**
 * Create a clean global object id.
 * 
 * @param[out]	lpcbID	The size in bytes of the returned id.
 * @param[out]	lppID	The returned id.
 */
HRESULT CreateCleanGOID(ULONG *lpcbID, LPBYTE *lppID)
{
	HRESULT hr = hrSuccess;
	BytePtr ptrData;
	GOID *lpGoid = NULL;

	if (lpcbID == NULL || lppID == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	hr = MAPIAllocateBuffer(CbNewGOID(sizeof(GUID)), &ptrData);
	if (hr != hrSuccess)
		goto exit;

	lpGoid = (GOID*)ptrData.get();

	lpGoid->ByteArrayID = GOID_BAID;
	lpGoid->YH = 0;
	lpGoid->YL = 0;
	lpGoid->M = 0;
	lpGoid->D = 0;
	lpGoid->X = 0;
	lpGoid->Size = sizeof(GUID);

	GetSystemTimeAsFileTime(&lpGoid->CreationTime);

	hr = CoCreateGuid((LPGUID)&lpGoid->Data);
	if (hr != hrSuccess)
		goto exit;

	*lpcbID = CbNewGOID(sizeof(GUID));
	*lppID = ptrData.release();

exit:
	return hr;
}

/**
 * Create a global object id based on a clean global object id.
 *
 * @param[in]	cbGOID	The size in bytes of the clean global object id.
 * @param[in]	lpGOID	The clean global object id.
 * @param[in]	ulDate	The date for which to create the global object id
 *           	      	specified in rtime.
 * @param[out]	lpcbID	The size in bytes of the returned id.
 * @param[out]	lppID	The returned id.
 */
HRESULT CreateGOID(ULONG cbGOID, LPBYTE lpGOID, ULONG ulDate, ULONG *lpcbID, LPBYTE *lppID)
{
	HRESULT hr = hrSuccess;
	BytePtr ptrData;
	GOID *lpGoid = (GOID*)lpGOID;
	GOID *lpNewGoid = NULL;
	ULONG ulYear;
	ULONG ulMonth;
	ULONG ulDay;

	if (lpGOID == NULL || lpcbID == NULL || lppID == NULL || cbGOID <= CbNewGOID(0)) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (lpGoid->ByteArrayID != GOID_BAID) {
		hr = MAPI_E_NO_SUPPORT;
		goto exit;
	}

	hr = BaseDateToYMD(ulDate, &ulYear, &ulMonth, &ulDay);
	if (hr != hrSuccess)
		goto exit;

	hr = MAPIAllocateBuffer(cbGOID, &ptrData);
	if (hr != hrSuccess)
		goto exit;

	lpNewGoid = (GOID*)ptrData.get();

	memcpy(lpNewGoid, lpGOID, cbGOID);

	lpNewGoid->YH = ulYear >> 8;
	lpNewGoid->YL = ulYear & 0xff;
	lpNewGoid->M = ulMonth;
	lpNewGoid->D = ulDay;

	*lpcbID = cbGOID;
	*lppID = ptrData.release();

exit:
	return hr;
}
