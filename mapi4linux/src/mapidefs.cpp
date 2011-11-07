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
#include "m4l.mapidefs.h"
#include "m4l.mapix.h"
#include "m4l.debug.h"
#include "m4l.mapiutil.h"

#include "ECDebug.h"
#include "Util.h"
#include "ECMemTable.h"
#include "charset/convert.h"
#include "ustringutil.h"

#include <mapi.h>
#include <mapicode.h>
#include <mapiguid.h>
#include <mapix.h>
#include <mapiutil.h>

#include "ECConfig.h"
#include "CommonUtil.h"

// ---
// IMAPIProp
// ---

M4LMAPIProp::M4LMAPIProp() {
}

M4LMAPIProp::~M4LMAPIProp() {
	list<LPSPropValue>::iterator i;
	for(i = properties.begin(); i != properties.end(); i++) {
		MAPIFreeBuffer(*i);
	}
	properties.clear();
}

HRESULT M4LMAPIProp::GetLastError(HRESULT hResult, ULONG ulFlags, LPMAPIERROR* lppMAPIError) {
	*lppMAPIError = NULL;
	return hrSuccess;
}

HRESULT M4LMAPIProp::SaveChanges(ULONG ulFlags) {
	// memory only.
	TRACE_MAPILIB(TRACE_ENTRY, "IMAPIProp::SaveChanges", "");
	TRACE_MAPILIB1(TRACE_RETURN, "IMAPIProp::SaveChanges", "0x%08x", hrSuccess);
	return hrSuccess;
}

HRESULT M4LMAPIProp::GetProps(LPSPropTagArray lpPropTagArray, ULONG ulFlags, ULONG* lpcValues, LPSPropValue* lppPropArray) {
	TRACE_MAPILIB2(TRACE_ENTRY, "IMAPIProp::GetProps", "PropTagArray=%s\nfFlags=0x%08X", PropNameFromPropTagArray(lpPropTagArray).c_str(), ulFlags);
	list<LPSPropValue>::iterator i;
	ULONG c;
	LPSPropValue props = NULL;
	HRESULT hr = hrSuccess;
	SPropValue sConvert;
	convert_context converter;
	std::wstring unicode;
	std::string ansi;
	LPSPropValue lpCopy = NULL;

	if (!lpPropTagArray) {
		// all properties are requested
		hr = MAPIAllocateBuffer(sizeof(SPropValue)*properties.size(), (void**)&props);
		if (hr != hrSuccess)
			goto exit;

		for (c = 0, i = properties.begin(); i != properties.end(); i++, c++) {
			// perform unicode conversion if required
			if ((ulFlags & MAPI_UNICODE) && (PROP_TYPE((*i)->ulPropTag) == PT_STRING8)) {
				sConvert.ulPropTag = CHANGE_PROP_TYPE((*i)->ulPropTag, PT_UNICODE);
				unicode = converter.convert_to<wstring>((*i)->Value.lpszA);
				sConvert.Value.lpszW = (WCHAR*)unicode.c_str();

				lpCopy = &sConvert;
			} else if (((ulFlags & MAPI_UNICODE) == 0) && (PROP_TYPE((*i)->ulPropTag) == PT_UNICODE)) {
				sConvert.ulPropTag = CHANGE_PROP_TYPE((*i)->ulPropTag, PT_STRING8);
				ansi = converter.convert_to<string>((*i)->Value.lpszW);
				sConvert.Value.lpszA = (char*)ansi.c_str();

				lpCopy = &sConvert;
			} else {
				lpCopy = *i;
			}

			hr = Util::HrCopyProperty(&props[c], lpCopy, (void *)props);
			if (hr != hrSuccess)
				goto exit;
		}
	} else {
		hr = MAPIAllocateBuffer(sizeof(SPropValue)*lpPropTagArray->cValues, (void**)&props);
		if (hr != hrSuccess)
			goto exit;

		for (c = 0; c < lpPropTagArray->cValues; c++) {
			for (i = properties.begin(); i != properties.end(); i++) {
				if (PROP_ID((*i)->ulPropTag) == PROP_ID(lpPropTagArray->aulPropTag[c])) {
					// perform unicode conversion if required
					if ((PROP_TYPE((*i)->ulPropTag) == PT_STRING8 && PROP_TYPE(lpPropTagArray->aulPropTag[c]) == PT_UNICODE) ||
						((ulFlags & MAPI_UNICODE) && PROP_TYPE(lpPropTagArray->aulPropTag[c]) == PT_UNSPECIFIED))
					{
						sConvert.ulPropTag = CHANGE_PROP_TYPE((*i)->ulPropTag, PT_UNICODE);
						unicode = converter.convert_to<wstring>((*i)->Value.lpszA);
						sConvert.Value.lpszW = (WCHAR*)unicode.c_str();

						lpCopy = &sConvert;
					}
					else if ((PROP_TYPE((*i)->ulPropTag) == PT_UNICODE && PROP_TYPE(lpPropTagArray->aulPropTag[c]) == PT_STRING8) ||
							 (((ulFlags & MAPI_UNICODE) == 0) && PROP_TYPE(lpPropTagArray->aulPropTag[c]) == PT_UNSPECIFIED))
					{
						sConvert.ulPropTag = CHANGE_PROP_TYPE((*i)->ulPropTag, PT_STRING8);
						ansi = converter.convert_to<string>((*i)->Value.lpszW);
						sConvert.Value.lpszA = (char*)ansi.c_str();

						lpCopy = &sConvert;
					} else {
						lpCopy = *i;
					}

					hr = Util::HrCopyProperty(&props[c], lpCopy, (void *)props);
					if (hr != hrSuccess)
						goto exit;
					break;
				}
			}
		
			if (i == properties.end()) {
				// Not found
				props[c].ulPropTag = PROP_TAG(PT_ERROR, PROP_ID(lpPropTagArray->aulPropTag[c]));
				props[c].Value.err = MAPI_E_NOT_FOUND;
        		
				hr = MAPI_W_ERRORS_RETURNED;
			}
		}
	}

	*lpcValues = c;
	*lppPropArray = props;

exit:
	if (FAILED(hr) && props)
		MAPIFreeBuffer(props);

	TRACE_MAPILIB2(TRACE_RETURN, "IMAPIProp::GetProps", "%s\n%s", GetMAPIErrorDescription(hr).c_str(), PropNameFromPropArray(*lpcValues, *lppPropArray).c_str());
	return hr;
}

HRESULT M4LMAPIProp::GetPropList(ULONG ulFlags, LPSPropTagArray* lppPropTagArray) {
	TRACE_MAPILIB(TRACE_ENTRY, "IMAPIProp::GetPropList", "");
	TRACE_MAPILIB1(TRACE_RETURN, "IMAPIProp::GetPropList", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPIProp::OpenProperty(ULONG ulPropTag, LPCIID lpiid, ULONG ulInterfaceOptions, ULONG ulFlags, LPUNKNOWN* lppUnk) {
	TRACE_MAPILIB(TRACE_ENTRY, "IMAPIProp::OpenProperty", "");
	TRACE_MAPILIB1(TRACE_RETURN, "IMAPIProp::OpenProperty", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPIProp::SetProps(ULONG cValues, LPSPropValue lpPropArray, LPSPropProblemArray* lppProblems) {
	TRACE_MAPILIB1(TRACE_ENTRY, "IMAPIProp::SetProps", "%s", PropNameFromPropArray(cValues, lpPropArray).c_str());
	list<LPSPropValue>::iterator i, del;
	ULONG c;
	LPSPropValue pv = NULL;
	HRESULT hr = hrSuccess;

	// Validate input
	if (lpPropArray == NULL || cValues == 0) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	for (c=0; c<cValues; c++) {
		// TODO: return MAPI_E_INVALID_PARAMETER, if multivalued property in 
		//       the array and its cValues member is set to zero.		
		if (PROP_TYPE(lpPropArray[c].ulPropTag) == PT_OBJECT) {
			hr = MAPI_E_INVALID_PARAMETER;
			goto exit;
		}		
	}

    // remove possible old properties
	for (c=0; c<cValues; c++) {
		for(i = properties.begin(); i != properties.end(); ) {
			if ( PROP_ID((*i)->ulPropTag) == PROP_ID(lpPropArray[c].ulPropTag) && 
				(*i)->ulPropTag != PR_NULL && 
				PROP_TYPE((*i)->ulPropTag) != PT_ERROR)
			{
				del = i++;
				MAPIFreeBuffer(*del);
				properties.erase(del);
			} else {
				i++;
			}
		}
	}

    // set new properties
	for (c=0; c<cValues; c++) {
		// Ignore PR_NULL property tag and all properties with a type of PT_ERROR
		if(PROP_TYPE(lpPropArray[c].ulPropTag) == PT_ERROR || 
			lpPropArray[c].ulPropTag == PR_NULL)
			continue;
		
		hr = MAPIAllocateBuffer(sizeof(SPropValue), (void**)&pv);
		if (hr != hrSuccess)
			goto exit;

		memset(pv, 0, sizeof(pv));
		hr = Util::HrCopyProperty(pv, &lpPropArray[c], (void *)pv);
		if (hr != hrSuccess) {
			MAPIFreeBuffer(pv);
			goto exit;
		}
		properties.push_back(pv);
	}

exit:
	TRACE_MAPILIB1(TRACE_RETURN, "IMAPIProp::SetProps", "0x%08x", hr);
	return hr;
}

HRESULT M4LMAPIProp::DeleteProps(LPSPropTagArray lpPropTagArray, LPSPropProblemArray* lppProblems) {
	TRACE_MAPILIB(TRACE_ENTRY, "IMAPIProp::DeleteProps", "");
	TRACE_MAPILIB1(TRACE_RETURN, "IMAPIProp::DeleteProps", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPIProp::CopyTo(ULONG ciidExclude, LPCIID rgiidExclude, LPSPropTagArray lpExcludeProps, ULONG ulUIParam,
			    LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags,
			    LPSPropProblemArray* lppProblems) {
	TRACE_MAPILIB(TRACE_ENTRY, "IMAPIProp::CopyTo", "");
	TRACE_MAPILIB1(TRACE_RETURN, "IMAPIProp::CopyTo", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPIProp::CopyProps(LPSPropTagArray lpIncludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface,
			       LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray* lppProblems) {
	TRACE_MAPILIB(TRACE_ENTRY, "IMAPIProp::CopyProps", "");
	TRACE_MAPILIB1(TRACE_RETURN, "IMAPIProp::CopyProps", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPIProp::GetNamesFromIDs(LPSPropTagArray* lppPropTags, LPGUID lpPropSetGuid, ULONG ulFlags, ULONG* lpcPropNames,
				     LPMAPINAMEID** lpppPropNames) {
	TRACE_MAPILIB(TRACE_ENTRY, "IMAPIProp::GetNamesFromIDs", "");
	TRACE_MAPILIB1(TRACE_RETURN, "IMAPIProp::GetNamesFromIDs", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPIProp::GetIDsFromNames(ULONG cPropNames, LPMAPINAMEID* lppPropNames, ULONG ulFlags, LPSPropTagArray* lppPropTags) {
	TRACE_MAPILIB(TRACE_ENTRY, "IMAPIProp::GetIDsFromNames", "");
	TRACE_MAPILIB1(TRACE_RETURN, "IMAPIProp::GetIDsFromNames", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

// iunknown passthru
ULONG M4LMAPIProp::AddRef() {
	return M4LUnknown::AddRef();
}
ULONG M4LMAPIProp::Release() {
	return M4LUnknown::Release();
}
HRESULT M4LMAPIProp::QueryInterface(REFIID refiid, void **lpvoid) {
	TRACE_MAPILIB(TRACE_ENTRY, "IMAPIProp::QueryInterface", "");
	HRESULT hr = hrSuccess;
	if ((refiid == IID_IMAPIProp) || (refiid == IID_IUnknown)) {
		AddRef();
		*lpvoid = (IMAPIProp *)this;
		hr = hrSuccess;
	} else
		hr = MAPI_E_INTERFACE_NOT_SUPPORTED;

	TRACE_MAPILIB1(TRACE_RETURN, "IMAPIProp::QueryInterface", "0x%08x", hr);
	return hr;
}


// ---
// IProfSect
// ---

M4LProfSect::M4LProfSect(BOOL bGlobalProf) {
	this->bGlobalProf = bGlobalProf;
}

M4LProfSect::~M4LProfSect() {
}

HRESULT M4LProfSect::ValidateState(ULONG ulUIParam, ULONG ulFlags) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LProfSect::ValidateState", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LProfSect::ValidateState", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LProfSect::SettingsDialog(ULONG ulUIParam, ULONG ulFlags) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LProfSect::SettingsDialog", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LProfSect::SettingsDialog", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LProfSect::ChangePassword(LPTSTR lpOldPass, LPTSTR lpNewPass, ULONG ulFlags) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LProfSect::ChangePassword", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LProfSect::ChangePassword", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LProfSect::FlushQueues(ULONG ulUIParam, ULONG cbTargetTransport, LPENTRYID lpTargetTransport, ULONG ulFlags) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LProfSect::FlushQueues", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LProfSect::FlushQueues", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

// IMAPIProp passthru
HRESULT M4LProfSect::GetLastError(HRESULT hResult, ULONG ulFlags, LPMAPIERROR* lppMAPIError) {
	return M4LMAPIProp::GetLastError(hResult, ulFlags, lppMAPIError);
}

HRESULT M4LProfSect::SaveChanges(ULONG ulFlags) {
	return M4LMAPIProp::SaveChanges(ulFlags);
}

HRESULT M4LProfSect::GetProps(LPSPropTagArray lpPropTagArray, ULONG ulFlags, ULONG* lpcValues, LPSPropValue* lppPropArray) {
	return M4LMAPIProp::GetProps(lpPropTagArray, ulFlags, lpcValues, lppPropArray);
}

HRESULT M4LProfSect::GetPropList(ULONG ulFlags, LPSPropTagArray* lppPropTagArray) {
	return M4LMAPIProp::GetPropList(ulFlags, lppPropTagArray);
}

HRESULT M4LProfSect::OpenProperty(ULONG ulPropTag, LPCIID lpiid, ULONG ulInterfaceOptions, ULONG ulFlags, LPUNKNOWN* lppUnk) {
	return M4LMAPIProp::OpenProperty(ulPropTag, lpiid, ulInterfaceOptions, ulFlags, lppUnk);
}

HRESULT M4LProfSect::SetProps(ULONG cValues, LPSPropValue lpPropArray, LPSPropProblemArray* lppProblems) {
	HRESULT hr = hrSuccess;

	hr = M4LMAPIProp::SetProps(cValues, lpPropArray, lppProblems);


	return hr;
}

HRESULT M4LProfSect::DeleteProps(LPSPropTagArray lpPropTagArray, LPSPropProblemArray* lppProblems) {
	return M4LMAPIProp::DeleteProps(lpPropTagArray, lppProblems);
}

HRESULT M4LProfSect::CopyTo(ULONG ciidExclude, LPCIID rgiidExclude, LPSPropTagArray lpExcludeProps, ULONG ulUIParam,
			    LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray* lppProblems) {
	return M4LMAPIProp::CopyTo(ciidExclude, rgiidExclude, lpExcludeProps, ulUIParam,
							   lpProgress, lpInterface, lpDestObj, ulFlags, lppProblems);
}

HRESULT M4LProfSect::CopyProps(LPSPropTagArray lpIncludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface,
							   LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray* lppProblems) {
	return M4LMAPIProp::CopyProps(lpIncludeProps, ulUIParam, lpProgress, lpInterface, lpDestObj, ulFlags, lppProblems);
}

HRESULT M4LProfSect::GetNamesFromIDs(LPSPropTagArray* lppPropTags, LPGUID lpPropSetGuid, ULONG ulFlags, ULONG* lpcPropNames,
									 LPMAPINAMEID** lpppPropNames) {
	return M4LMAPIProp::GetNamesFromIDs(lppPropTags, lpPropSetGuid, ulFlags, lpcPropNames, lpppPropNames);
}

HRESULT M4LProfSect::GetIDsFromNames(ULONG cPropNames, LPMAPINAMEID* lppPropNames, ULONG ulFlags, LPSPropTagArray* lppPropTags) {
	return M4LMAPIProp::GetIDsFromNames(cPropNames, lppPropNames, ulFlags, lppPropTags);
}

// iunknown passthru
ULONG M4LProfSect::AddRef() {
	return M4LUnknown::AddRef();
}
ULONG M4LProfSect::Release() {
	return M4LUnknown::Release();
}
HRESULT M4LProfSect::QueryInterface(REFIID refiid, void **lpvoid) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LProfSect::QueryInterface", "");
	HRESULT hr = hrSuccess;

	if ((refiid == IID_IProfSect) || (refiid == IID_IMAPIProp) || (refiid == IID_IUnknown)) {
		AddRef();
		*lpvoid = (IProfSect *)this;
		hr = hrSuccess;
    } else
		hr = MAPI_E_INTERFACE_NOT_SUPPORTED;

	TRACE_MAPILIB1(TRACE_RETURN, "M4LProfSect::QueryInterface", "0x%08x", hr);
	return hr;
}


// ---
// IMAPITable
// ---
M4LMAPITable::M4LMAPITable() {
}

M4LMAPITable::~M4LMAPITable() {
}

HRESULT M4LMAPITable::GetLastError(HRESULT hResult, ULONG ulFlags, LPMAPIERROR *lppMAPIError) {
	*lppMAPIError = NULL;
	return hrSuccess;
}

HRESULT M4LMAPITable::Advise(ULONG ulEventMask, LPMAPIADVISESINK lpAdviseSink, ULONG * lpulConnection) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::Advise", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::Advise", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::Unadvise(ULONG ulConnection) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::Unadvise", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::Unadvise", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::GetStatus(ULONG *lpulTableStatus, ULONG *lpulTableType) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::GetStatus", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::GetStatus", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::SetColumns(LPSPropTagArray lpPropTagArray, ULONG ulFlags) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::SetColumns", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::SetColumns", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::QueryColumns(ULONG ulFlags, LPSPropTagArray *lpPropTagArray) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::QueryColumns", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::QueryColumns", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::GetRowCount(ULONG ulFlags, ULONG *lpulCount) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::GetRowCount", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::GetRowCount", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::SeekRow(BOOKMARK bkOrigin, LONG lRowCount, LONG *lplRowsSought) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::SeekRow", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::SeekRow", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::SeekRowApprox(ULONG ulNumerator, ULONG ulDenominator) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::SeekRowApprox", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::SeekRowApprox", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::QueryPosition(ULONG *lpulRow, ULONG *lpulNumerator, ULONG *lpulDenominator) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::QueryPosition", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::QueryPosition", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::FindRow(LPSRestriction lpRestriction, BOOKMARK bkOrigin, ULONG ulFlags) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::FindRow", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::FindRow", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::Restrict(LPSRestriction lpRestriction, ULONG ulFlags) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::Restrict", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::Restrict", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::CreateBookmark(BOOKMARK* lpbkPosition) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::CreateBookmark", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::CreateBookmark", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::FreeBookmark(BOOKMARK bkPosition) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::FreeBookmark", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::FreeBookmark", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::SortTable(LPSSortOrderSet lpSortCriteria, ULONG ulFlags) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::SortTable", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::SortTable", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::QuerySortOrder(LPSSortOrderSet *lppSortCriteria) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::QuerySortOrder", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::QuerySortOrder", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::QueryRows(LONG lRowCount, ULONG ulFlags, LPSRowSet *lppRows) {
    // TODO
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::QueryRows", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::QueryRows", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::Abort() {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::Abort", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::Abort", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::ExpandRow(ULONG cbInstanceKey, LPBYTE pbInstanceKey, ULONG ulRowCount,
								ULONG ulFlags, LPSRowSet * lppRows, ULONG *lpulMoreRows) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::ExpandRow", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::ExpandRow", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::CollapseRow(ULONG cbInstanceKey, LPBYTE pbInstanceKey, ULONG ulFlags, ULONG *lpulRowCount) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::CollapseRow", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::CollapseRow", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::WaitForCompletion(ULONG ulFlags, ULONG ulTimeout, ULONG *lpulTableStatus) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::WaitForCompletion", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::WaitForCompletion", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::GetCollapseState(ULONG ulFlags, ULONG cbInstanceKey, LPBYTE lpbInstanceKey, ULONG *lpcbCollapseState,
			 LPBYTE *lppbCollapseState) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::GetCollapseState", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::GetCollapseState", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

HRESULT M4LMAPITable::SetCollapseState(ULONG ulFlags, ULONG cbCollapseState, LPBYTE pbCollapseState, BOOKMARK *lpbkLocation) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::SetCollapseState", "");
	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::SetCollapseState", "0x%08x", MAPI_E_NO_SUPPORT);
	return MAPI_E_NO_SUPPORT;
}

// iunknown passthru
ULONG M4LMAPITable::AddRef() {
	return M4LUnknown::AddRef();
}
ULONG M4LMAPITable::Release() {
	return M4LUnknown::Release();
}
HRESULT M4LMAPITable::QueryInterface(REFIID refiid, void **lpvoid) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPITable::QueryInterface", "");
	HRESULT hr = hrSuccess;

	if ((refiid == IID_IMAPITable) || (refiid == IID_IUnknown)) {
		AddRef();
		*lpvoid = (IMAPITable *)this;
		hr = hrSuccess;
	} else
		hr = MAPI_E_INTERFACE_NOT_SUPPORTED;

	TRACE_MAPILIB1(TRACE_RETURN, "M4LMAPITable::QueryInterface", "0x%08x", hr);
	return hr;
}

// ---
// IProviderAdmin
// ---
M4LProviderAdmin::M4LProviderAdmin(M4LMsgServiceAdmin* new_msa, char *szService) {
	msa = new_msa;
	if(szService)
		this->szService = strdup(szService);
	else
		this->szService = NULL;
}

M4LProviderAdmin::~M4LProviderAdmin() {
	if(szService)
		free(szService);
}

HRESULT M4LProviderAdmin::GetLastError(HRESULT hResult, ULONG ulFlags, LPMAPIERROR* lppMAPIError) {
    *lppMAPIError = NULL;
    return hrSuccess;
}

HRESULT M4LProviderAdmin::GetProviderTable(ULONG ulFlags, LPMAPITABLE* lppTable) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LProviderAdmin::GetProviderTable", "");
	HRESULT hr = hrSuccess;
	ULONG cValues = 0;
	LPSPropValue lpsProps = NULL;
	list<providerEntry *>::iterator i;
	ECMemTable *lpTable = NULL;
	ECMemTableView *lpTableView = NULL;
	LPSPropValue lpDest = NULL;
	ULONG cValuesDest = 0;
	SPropValue sPropID;
	int n = 0;
	LPSPropTagArray lpPropTagArray = NULL;

	SizedSPropTagArray(8, sptaProviderCols) = {8, {PR_MDB_PROVIDER, PR_INSTANCE_KEY, PR_RECORD_KEY, PR_ENTRYID, PR_DISPLAY_NAME_A, PR_OBJECT_TYPE, PR_RESOURCE_TYPE, PR_PROVIDER_UID} };
	
	pthread_mutex_lock(&msa->m_mutexserviceadmin);

	hr = Util::HrCopyUnicodePropTagArray(ulFlags, (LPSPropTagArray)&sptaProviderCols, &lpPropTagArray);
	if(hr != hrSuccess)
		goto exit;

	hr = ECMemTable::Create(lpPropTagArray, PR_ROWID, &lpTable);
	if(hr != hrSuccess)
		goto exit;
	
	// Loop through all providers, add each to the table
	for(i=msa->providers.begin(); i != msa->providers.end(); i++) {
		if(szService) {
			if(strcmp(szService, (*i)->servicename.c_str()) != 0)
				continue;
		}
		
		hr = (*i)->profilesection->GetProps((LPSPropTagArray)&sptaProviderCols, 0, &cValues, &lpsProps);
		if (FAILED(hr))
			goto exit;
		
		sPropID.ulPropTag = PR_ROWID;
		sPropID.Value.ul = n++;
		
		hr = Util::HrAddToPropertyArray(lpsProps, cValues, &sPropID, &lpDest, &cValuesDest);
		if(hr != hrSuccess)
			goto exit;
		
		lpTable->HrModifyRow(ECKeyTable::TABLE_ROW_ADD, NULL, lpDest, cValuesDest);
		
		MAPIFreeBuffer(lpDest);
		MAPIFreeBuffer(lpsProps);
		lpDest = NULL;
		lpsProps = NULL;
	}
	
	hr = lpTable->HrGetView(createLocaleFromName(""), ulFlags, &lpTableView);
	if(hr != hrSuccess)
		goto exit;
		
	hr = lpTableView->QueryInterface(IID_IMAPITable, (void **)lppTable);
	
exit:
	pthread_mutex_unlock(&msa->m_mutexserviceadmin);

	if (lpPropTagArray)
		MAPIFreeBuffer(lpPropTagArray);

	if (lpTableView)
		lpTableView->Release();

	if (lpTable)
		lpTable->Release();

	if (lpDest)
		MAPIFreeBuffer(lpDest);
	
	if (lpsProps)
		MAPIFreeBuffer(lpsProps);
	
	TRACE_MAPILIB1(TRACE_RETURN, "M4LProviderAdmin::GetProviderTable", "0x%08x", hr);
	return hr;
}

HRESULT M4LProviderAdmin::CreateProvider(LPTSTR lpszProvider, ULONG cValues, LPSPropValue lpProps, ULONG ulUIParam,
										 ULONG ulFlags, MAPIUID* lpUID) {
    TRACE_MAPILIB(TRACE_ENTRY, "M4LProviderAdmin::CreateProvider", "");
	SPropValue sProps[10];
	// from client.iss file, should be read from a mapisvc.inf file
	unsigned char privateGuid[] =   { 0xCA, 0x3D, 0x25, 0x3C, 0x27, 0xD2, 0x3C, 0x44, 0x94, 0xFE, 0x42, 0x5F, 0xAB, 0x95, 0x8C, 0x19 };
	unsigned char publicGuid[] = 	{ 0x09, 0x4A, 0x7F, 0xD4, 0xBD, 0xD3, 0x3C, 0x49, 0xB2, 0xFC, 0x3C, 0x90, 0xBB, 0xCB, 0x48, 0xD4 };
	unsigned char abGuid[] =		{ 0xAC, 0x21, 0xA9, 0x50, 0x40, 0xD3, 0xEE, 0x48, 0xB3, 0x19, 0xFB, 0xA7, 0x53, 0x30, 0x44, 0x25 };

	LPSPropValue lpsPropValProfileName = NULL;
	providerEntry *entry = NULL;
	serviceEntry* lpService = NULL;
	HRESULT hr = hrSuccess;

	pthread_mutex_lock(&msa->m_mutexserviceadmin);

	if(szService == NULL) {
		hr = MAPI_E_NO_ACCESS;
		goto exit;
	}

	lpService = msa->findServiceAdmin((LPTSTR)szService);

	// @todo According to MSDN, ulFlags may contain MAPI_UNICODE, but I don't think we'll ever see and/or use this.
	if(stricmp((char *)lpszProvider, "ZARAFA6_MSMDB_private") != 0 &&
	   stricmp((char *)lpszProvider, "ZARAFA6_MSMDB_public") != 0 &&
	   stricmp((char *)lpszProvider, "ZARAFA6_ABP") != 0) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	entry = new providerEntry;

	entry->profilesection = new M4LProfSect();
	if(!entry->profilesection) {
		delete entry;
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}
	
	// Set the default profilename
	hr = HrGetOneProp((IProfSect*)msa->profilesection, PR_PROFILE_NAME_A, &lpsPropValProfileName);
	if (hr != hrSuccess)
		goto exit;

	hr = entry->profilesection->SetProps(1, lpsPropValProfileName, NULL);
	if (hr != hrSuccess)
		goto exit;

	CoCreateGuid((LPGUID)&entry->uid);
	entry->profilesection->AddRef();

	// This is actually the mapisvc.ini file
	if(stricmp((char *)lpszProvider,"ZARAFA6_MSMDB_private") == 0) {
		sProps[0].ulPropTag = PR_MDB_PROVIDER;
		sProps[0].Value.bin.lpb = (BYTE *)&privateGuid;
		sProps[0].Value.bin.cb = sizeof(GUID);

		sProps[1].ulPropTag = PR_DISPLAY_NAME_A;
		sProps[1].Value.lpszA = "Zarafa private store";
		
		sProps[2].ulPropTag = PR_RESOURCE_TYPE;
		sProps[2].Value.ul = MAPI_STORE_PROVIDER;
		
		sProps[3].ulPropTag = PR_INSTANCE_KEY;
		sProps[3].Value.bin.lpb = (BYTE *)&entry->uid;
		sProps[3].Value.bin.cb = sizeof(GUID);

		sProps[4].ulPropTag = PR_PROVIDER_UID;
		sProps[4].Value.bin.lpb = (BYTE *)&entry->uid;
		sProps[4].Value.bin.cb = sizeof(GUID);
		
		sProps[5].ulPropTag = PR_RESOURCE_FLAGS;
		sProps[5].Value.ul = STATUS_PRIMARY_STORE | STATUS_DEFAULT_STORE;

		sProps[6].ulPropTag = PR_DEFAULT_STORE;
		sProps[6].Value.b = TRUE;

		sProps[7].ulPropTag = PR_OBJECT_TYPE;
		sProps[7].Value.ul = MAPI_STORE;

		sProps[8].ulPropTag = PR_PROVIDER_DISPLAY_A;
		sProps[8].Value.lpszA = "Zarafa private store";

		sProps[9].ulPropTag = PR_SERVICE_UID;
		sProps[9].Value.bin.lpb = (BYTE *)&lpService->muid;
		sProps[9].Value.bin.cb = sizeof(GUID);

		entry->profilesection->SetProps(10, sProps, NULL);
		
	} else if(stricmp((char *)lpszProvider,"ZARAFA6_MSMDB_public") == 0) {
		sProps[0].ulPropTag = PR_MDB_PROVIDER;
		sProps[0].Value.bin.lpb = (BYTE *)&publicGuid;
		sProps[0].Value.bin.cb = sizeof(GUID);

		sProps[1].ulPropTag = PR_DISPLAY_NAME_A;
		sProps[1].Value.lpszA = "Zarafa public store";
		
		sProps[2].ulPropTag = PR_RESOURCE_TYPE;
		sProps[2].Value.ul = MAPI_STORE_PROVIDER;

		sProps[3].ulPropTag = PR_INSTANCE_KEY;
		sProps[3].Value.bin.lpb = (BYTE *)&entry->uid;
		sProps[3].Value.bin.cb = sizeof(GUID);

		sProps[4].ulPropTag = PR_PROVIDER_UID;
		sProps[4].Value.bin.lpb = (BYTE *)&entry->uid;
		sProps[4].Value.bin.cb = sizeof(GUID);

		sProps[5].ulPropTag = PR_RESOURCE_FLAGS;
		sProps[5].Value.ul = STATUS_NO_DEFAULT_STORE;

		sProps[6].ulPropTag = PR_DEFAULT_STORE;
		sProps[6].Value.b = FALSE;

		sProps[7].ulPropTag = PR_OBJECT_TYPE;
		sProps[7].Value.ul = MAPI_STORE;
		
		sProps[8].ulPropTag = PR_PROVIDER_DISPLAY_A;
		sProps[8].Value.lpszA = "Zarafa public store";

		sProps[9].ulPropTag = PR_SERVICE_UID;
		sProps[9].Value.bin.lpb = (BYTE *)&lpService->muid;
		sProps[9].Value.bin.cb = sizeof(GUID);

		entry->profilesection->SetProps(10, sProps, NULL);

	} else if(stricmp((char *)lpszProvider,"ZARAFA6_ABP") == 0) {
 		sProps[0].ulPropTag = PR_AB_PROVIDER_ID;
 		sProps[0].Value.bin.lpb = (BYTE *)&abGuid;
 		sProps[0].Value.bin.cb = sizeof(GUID);

		sProps[1].ulPropTag = PR_DISPLAY_NAME_A;
		sProps[1].Value.lpszA = "Zarafa Addressbook";
		
		sProps[2].ulPropTag = PR_RESOURCE_TYPE;
		sProps[2].Value.ul = MAPI_AB_PROVIDER;

		sProps[3].ulPropTag = PR_INSTANCE_KEY;
		sProps[3].Value.bin.lpb = (BYTE *)&entry->uid;
		sProps[3].Value.bin.cb = sizeof(GUID);

		sProps[4].ulPropTag = PR_PROVIDER_UID;
		sProps[4].Value.bin.lpb = (BYTE *)&entry->uid;
		sProps[4].Value.bin.cb = sizeof(GUID);

		sProps[5].ulPropTag = PR_OBJECT_TYPE;
		sProps[5].Value.ul = MAPI_ADDRBOOK;
		
		sProps[6].ulPropTag = PR_PROVIDER_DISPLAY_A;
		sProps[6].Value.lpszA = "Zarafa Addressbook";

		sProps[7].ulPropTag = PR_SERVICE_UID;
		sProps[7].Value.bin.lpb = (BYTE *)&lpService->muid;
		sProps[7].Value.bin.cb = sizeof(GUID);

		entry->profilesection->SetProps(8, sProps, NULL);
	}
	
	entry->servicename = szService;
		
	msa->providers.push_back(entry);

	if(lpUID)
		*lpUID = entry->uid;
	
	// We should really call the MSGServiceEntry with MAPI_CREATE_PROVIDER, but there
	// isn't much use at the moment.
exit:
	pthread_mutex_unlock(&msa->m_mutexserviceadmin);

	if (lpsPropValProfileName)
		MAPIFreeBuffer(lpsPropValProfileName);

	TRACE_MAPILIB1(TRACE_RETURN, "M4LProviderAdmin::CreateProvider", "0x%08x", hr);
    return hr;
}

HRESULT M4LProviderAdmin::DeleteProvider(LPMAPIUID lpUID) {
	HRESULT hr = MAPI_E_NOT_FOUND;	
	TRACE_MAPILIB(TRACE_ENTRY, "M4LProviderAdmin::DeleteProvider", "");
	list<providerEntry*>::iterator i;
	
	for(i = msa->providers.begin(); i != msa->providers.end(); i++) {
		if(memcmp(&(*i)->uid, lpUID, sizeof(MAPIUID)) == 0) {
			msa->providers.erase(i);
			hr = hrSuccess;
			break;
		}
	}
	
	TRACE_MAPILIB1(TRACE_RETURN, "M4LProviderAdmin::DeleteProvider", "0x%08x", MAPI_E_NO_SUPPORT);
    return hr;
}

HRESULT M4LProviderAdmin::OpenProfileSection(LPMAPIUID lpUID, LPCIID lpInterface, ULONG ulFlags, LPPROFSECT* lppProfSect) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LProviderAdmin::OpenProfileSection", "");
	HRESULT hr = hrSuccess;
	providerEntry *provider = NULL;
	// See provider/client/guid.h
	unsigned char globalGuid[] =    { 0x13,0xDB,0xB0,0xC8,0xAA,0x05,0x10,0x1A,0x9B,0xB0,0x00,0xAA,0x00,0x2F,0xC4,0x5A };
	
	pthread_mutex_lock(&msa->m_mutexserviceadmin);

	// Special ID: the global guid opens the profile's global profile section instead of a local profile
	if(memcmp(lpUID,&globalGuid,sizeof(MAPIUID)) == 0) {
		hr = msa->OpenProfileSection(lpUID, lpInterface, ulFlags, lppProfSect);
		goto exit;
	}
	
	provider = msa->findProvider(lpUID);
	if(provider == NULL) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	hr = provider->profilesection->QueryInterface(lpInterface ? (*lpInterface) : IID_IProfSect, (void**)lppProfSect);

exit:
	pthread_mutex_unlock(&msa->m_mutexserviceadmin);

	TRACE_MAPILIB1(TRACE_RETURN, "M4LProviderAdmin::OpenProfileSection", "0x%08x", hr);
	return hr;
}

// iunknown passthru
ULONG M4LProviderAdmin::AddRef() {
	return M4LUnknown::AddRef();
}
ULONG M4LProviderAdmin::Release() {
	return M4LUnknown::Release();
}
HRESULT M4LProviderAdmin::QueryInterface(REFIID refiid, void **lpvoid) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LProviderAdmin::QueryInterface", "");
	HRESULT hr = hrSuccess;

	if ((refiid == IID_IProviderAdmin) || (refiid == IID_IUnknown)) {
		AddRef();
		*lpvoid = (IProviderAdmin *)this;
		hr = hrSuccess;
	} else
		hr = MAPI_E_INTERFACE_NOT_SUPPORTED;

	TRACE_MAPILIB1(TRACE_RETURN, "M4LProviderAdmin::QueryInterface", "0x%08x", hr);
	return hr;
}


// 
// IMAPIAdviseSink
// 

M4LMAPIAdviseSink::M4LMAPIAdviseSink(LPNOTIFCALLBACK lpFn, void *lpContext) {
	this->lpContext = lpContext;
	this->lpFn = lpFn;
}

M4LMAPIAdviseSink::~M4LMAPIAdviseSink() {
}

ULONG M4LMAPIAdviseSink::OnNotify(ULONG cNotif, LPNOTIFICATION lpNotifications) {
	return this->lpFn(this->lpContext, cNotif, lpNotifications);
}


// iunknown passthru
ULONG M4LMAPIAdviseSink::AddRef() {
	return M4LUnknown::AddRef();
}
ULONG M4LMAPIAdviseSink::Release() {
	return M4LUnknown::Release();
}
HRESULT M4LMAPIAdviseSink::QueryInterface(REFIID refiid, void **lpvoid) {
	TRACE_MAPILIB(TRACE_ENTRY, "M4LMAPIAdviseSink::QueryInterface", "");
	HRESULT hr = hrSuccess;
	if ((refiid == IID_IMAPIAdviseSink) || (refiid == IID_IUnknown)) {
		AddRef();
		*lpvoid = (IMAPIAdviseSink *)this;
		hr = hrSuccess;
	} else
		hr = MAPI_E_INTERFACE_NOT_SUPPORTED;

	TRACE_MAPILIB1(TRACE_RETURN, "M4LProviderAdmin::QueryInterface", "0x%08x", hr);
	return hr;
}
