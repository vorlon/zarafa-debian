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
#include "CompositeMessage.h"
#include "Trace.h"
#include "ECInterfaceDefs.h"

#include <set>

HRESULT CompositeMessage::Create(IMessage *lpPrimary, IMessage *lpSecondary, ULONG ulOverrideFlags, IMessage **lppMsg)
{
	typedef mapi_object_ptr<CompositeMessage> CMPtr;

	HRESULT hr = hrSuccess;
	CMPtr ptrCM;

	if (lpPrimary == NULL || lpSecondary == NULL || (ulOverrideFlags & ~cmofAll) != 0) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	try {
		ptrCM.reset(new CompositeMessage(lpPrimary, lpSecondary, ulOverrideFlags));
	} catch (const std::bad_alloc &) {
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}

	hr = ptrCM->QueryInterface(IID_IMessage, (void**)lppMsg);

exit:
	return hr;
}

CompositeMessage::CompositeMessage(IMessage *lpPrimary, IMessage *lpSecondary, ULONG ulOverrideFlags)
: m_ptrPrimary(lpPrimary)
, m_ptrSecondary(lpSecondary)
, m_ulOverrideFlags(ulOverrideFlags)
{ }

CompositeMessage::~CompositeMessage()
{ }

HRESULT CompositeMessage::QueryInterface(REFIID refiid, void** lppInterface)
{
	REGISTER_INTERFACE(IID_ECUnknown, this);

	REGISTER_INTERFACE(IID_IMessage, &this->m_xMessage);
	REGISTER_INTERFACE(IID_IMAPIProp, &this->m_xMessage);
	REGISTER_INTERFACE(IID_IUnknown, &this->m_xMessage);

	return MAPI_E_INTERFACE_NOT_SUPPORTED;
}

HRESULT CompositeMessage::GetLastError(HRESULT hError, ULONG ulFlags, LPMAPIERROR * lppMapiError)
{
	return m_ptrPrimary->GetLastError(hError, ulFlags, lppMapiError);
}

HRESULT CompositeMessage::SaveChanges(ULONG ulFlags)
{
	return m_ptrPrimary->SaveChanges(ulFlags);
}

HRESULT CompositeMessage::GetProps(LPSPropTagArray lpPropTagArray, ULONG ulFlags, ULONG FAR * lpcValues, LPSPropValue FAR * lppPropArray)
{
	HRESULT hr = hrSuccess;
	ULONG cValues = 0;
	SPropArrayPtr ptrPropArray;

	if (lpcValues == NULL || lppPropArray == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	hr = m_ptrPrimary->GetProps(lpPropTagArray, ulFlags, &cValues, &ptrPropArray);
	if (FAILED(hr))
		goto exit;
		

	if (hr == MAPI_W_ERRORS_RETURNED) {
		std::map<ULONG, LPSPropValue> mapTagToPropVal;
		SPropTagArrayPtr ptrPropTagArray;
		ULONG cSecondaryValues;
		SPropArrayPtr ptrSecondaryPropArray;

		for (ULONG i = 0; i < cValues; ++i) {
			SPropValue &propVal = ptrPropArray[i];
			if (PROP_TYPE(propVal.ulPropTag) == PT_ERROR && propVal.Value.err == MAPI_E_NOT_FOUND) {
				mapTagToPropVal[lpPropTagArray->aulPropTag[i]] = &propVal;
			}
		}

		hr = MAPIAllocateBuffer(CbNewSPropTagArray(mapTagToPropVal.size()), &ptrPropTagArray);
		if (hr != hrSuccess)
			goto exit;

		ptrPropTagArray->cValues = 0;
		for (std::map<ULONG, LPSPropValue>::iterator i = mapTagToPropVal.begin(); i != mapTagToPropVal.end(); ++i)
			ptrPropTagArray->aulPropTag[ptrPropTagArray->cValues++] = i->first;

		hr = m_ptrSecondary->GetProps(ptrPropTagArray, ulFlags, &cSecondaryValues, &ptrSecondaryPropArray);
		if (FAILED(hr))
			goto exit;

		for (ULONG i = 0; i < cSecondaryValues; ++i) {
			SPropValue &propVal = ptrSecondaryPropArray[i];
			if (PROP_TYPE(propVal.ulPropTag) != PT_ERROR) {
				hr = Util::HrCopyProperty(mapTagToPropVal[propVal.ulPropTag], &propVal, ptrPropArray);
				if (hr != hrSuccess)
					goto exit;
			}
		}
	}

	*lpcValues = cValues;
	*lppPropArray = ptrPropArray.release();

exit:
	return hr;
}

HRESULT CompositeMessage::GetPropList(ULONG ulFlags, LPSPropTagArray FAR * lppPropTagArray)
{
	HRESULT hr = hrSuccess;
	SPropTagArrayPtr ptrPropTags;
	std::set<ULONG> setTags;

	if (lppPropTagArray == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	hr = m_ptrPrimary->GetPropList(ulFlags, &ptrPropTags);
	if (hr != hrSuccess)
		goto exit;

	for (ULONG i = 0; i < ptrPropTags->cValues; ++i)
		setTags.insert(ptrPropTags->aulPropTag[i]);

	hr = m_ptrSecondary->GetPropList(ulFlags, &ptrPropTags);
	if (hr != hrSuccess)
		goto exit;

	for (ULONG i = 0; i < ptrPropTags->cValues; ++i)
		setTags.insert(ptrPropTags->aulPropTag[i]);

	hr = MAPIAllocateBuffer(CbNewSPropTagArray(setTags.size()), &ptrPropTags);
	if (hr != hrSuccess)
		goto exit;

	ptrPropTags->cValues = 0;
	for (std::set<ULONG>::iterator i = setTags.begin(); i != setTags.end(); ++i)
		ptrPropTags->aulPropTag[ptrPropTags->cValues++] = *i;

	*lppPropTagArray = ptrPropTags.release();

exit:
	return hr;
}

HRESULT CompositeMessage::OpenProperty(ULONG ulPropTag, LPCIID lpiid, ULONG ulInterfaceOptions, ULONG ulFlags, LPUNKNOWN FAR * lppUnk)
{
	HRESULT hr = hrSuccess;
	UnknownPtr ptrUnknown;

	if (lppUnk == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	// We don't support deferred errors.
	ulFlags &= ~MAPI_DEFERRED_ERRORS;

	/*
	 * We need to adapt our strategy based on the MAPI_MODIFY and MAPI_CREATE flags:
	 * - 0: Try to open it on the primary. On failure try it on the secondary.
	 * - MAPI_MODIFY: Try to open it on the primary. On failure try to copy it from the
	 *                seconday to the primary and open it on success.
	 * - MAPI_CREATE: Try to open it on the primary. On failure try to copy it from the
	 *                secondary to the primary. If that fails create it on the seconday.
	 *                otherwise open the copied property on the secondary.
	 *                According to MSDN MAPI_MODIFY is required with MAPI_CREATE. We'll
	 *                implicitly set it for misbehaving clients.
	 */
	if ((ulFlags & (MAPI_CREATE | MAPI_MODIFY)) == 0) {
		hr = m_ptrPrimary->OpenProperty(ulPropTag, lpiid, ulInterfaceOptions, ulFlags, &ptrUnknown);
		if (hr == MAPI_E_NOT_FOUND)
			hr = m_ptrSecondary->OpenProperty(ulPropTag, lpiid, ulInterfaceOptions, ulFlags, &ptrUnknown);
		if (hr != hrSuccess)
			goto exit;
	}

	else if ((ulFlags & (MAPI_CREATE | MAPI_MODIFY)) == MAPI_MODIFY) {
		hr = m_ptrPrimary->OpenProperty(ulPropTag, lpiid, ulInterfaceOptions, ulFlags, &ptrUnknown);
		if (hr == MAPI_E_NOT_FOUND) {
			hr = CopyProperty(ulPropTag);
			if (hr != hrSuccess)
				goto exit;

			hr = m_ptrPrimary->OpenProperty(ulPropTag, lpiid, ulInterfaceOptions, ulFlags, &ptrUnknown);
		}
		if (hr != hrSuccess)
			goto exit;
	}

	else {
		ASSERT((ulFlags & MAPI_CREATE) == MAPI_CREATE);

		hr = m_ptrPrimary->OpenProperty(ulPropTag, lpiid, ulInterfaceOptions, (ulFlags & ~MAPI_CREATE) | MAPI_MODIFY, &ptrUnknown);
		if (hr == MAPI_E_NOT_FOUND) {
			hr = CopyProperty(ulPropTag);
			if (hr == hrSuccess)
				hr = m_ptrPrimary->OpenProperty(ulPropTag, lpiid, ulInterfaceOptions, (ulFlags & ~MAPI_CREATE) | MAPI_MODIFY, &ptrUnknown);
			else if (hr == MAPI_E_NOT_FOUND)
				hr = m_ptrPrimary->OpenProperty(ulPropTag, lpiid, ulInterfaceOptions, ulFlags | MAPI_MODIFY, &ptrUnknown);
		}
		if (hr != hrSuccess)
			goto exit;
	}

	*lppUnk = ptrUnknown.release();

exit:
	return hr;
}

HRESULT CompositeMessage::SetProps(ULONG cValues, LPSPropValue lpPropArray, LPSPropProblemArray FAR * lppProblems)
{
	return m_ptrPrimary->SetProps(cValues, lpPropArray, lppProblems);
}

HRESULT CompositeMessage::DeleteProps(LPSPropTagArray lpPropTagArray, LPSPropProblemArray FAR * lppProblems)
{
	return m_ptrPrimary->DeleteProps(lpPropTagArray, lppProblems);
}

HRESULT CompositeMessage::CopyTo(ULONG ciidExclude, LPCIID rgiidExclude, LPSPropTagArray lpExcludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray FAR * lppProblems)
{
	return MAPI_E_NO_SUPPORT;
}

HRESULT CompositeMessage::CopyProps(LPSPropTagArray lpIncludeProps, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, LPCIID lpInterface, LPVOID lpDestObj, ULONG ulFlags, LPSPropProblemArray FAR * lppProblems)
{
	return MAPI_E_NO_SUPPORT;
}

HRESULT CompositeMessage::GetNamesFromIDs(LPSPropTagArray * pptaga, LPGUID lpguid, ULONG ulFlags, ULONG * pcNames, LPMAPINAMEID ** pppNames)
{
	return m_ptrPrimary->GetNamesFromIDs(pptaga, lpguid, ulFlags, pcNames, pppNames);
}

HRESULT CompositeMessage::GetIDsFromNames(ULONG cNames, LPMAPINAMEID * ppNames, ULONG ulFlags, LPSPropTagArray * pptaga)
{
	return m_ptrPrimary->GetIDsFromNames(cNames, ppNames, ulFlags, pptaga);
}

HRESULT CompositeMessage::GetAttachmentTable(ULONG ulFlags, LPMAPITABLE *lppTable)
{
	HRESULT hr = hrSuccess;

	if (m_ulOverrideFlags & cmofAttachments)
		hr = m_ptrPrimary->GetAttachmentTable(ulFlags, lppTable);
	else
		hr = m_ptrSecondary->GetAttachmentTable(ulFlags, lppTable);

	return hr;
}

HRESULT CompositeMessage::OpenAttach(ULONG ulAttachmentNum, LPCIID lpInterface, ULONG ulFlags, LPATTACH *lppAttach)
{
	HRESULT hr = hrSuccess;
	AttachPtr ptrAttach;

	if (lppAttach == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (m_ulOverrideFlags & cmofAttachments) {
		hr = m_ptrPrimary->OpenAttach(ulAttachmentNum, lpInterface, ulFlags, &ptrAttach);
	}

	else if ((ulFlags & (MAPI_MODIFY | MAPI_BEST_ACCESS)) == 0)
		hr = m_ptrSecondary->OpenAttach(ulAttachmentNum, lpInterface, ulFlags, &ptrAttach);

	else {
		/*
		 * If we end up here, the primary message does not override the attachments, while
		 * write access is requested. We'll deny access because we don't allow the secondary
		 * message to be modified.
		 */
		hr = MAPI_E_NO_ACCESS;
		goto exit;
	}

	if (hr != hrSuccess)
		goto exit;

	*lppAttach = ptrAttach.release();

exit:
	return hr;
}

HRESULT CompositeMessage::CreateAttach(LPCIID lpInterface, ULONG ulFlags, ULONG *lpulAttachmentNum, LPATTACH *lppAttach)
{
	HRESULT hr = hrSuccess;

	if ((m_ulOverrideFlags & cmofAttachments) == 0) {
		hr = MAPI_E_NO_ACCESS;
		goto exit;
	}

	hr = m_ptrPrimary->CreateAttach(lpInterface, ulFlags, lpulAttachmentNum, lppAttach);

exit:
	return hr;
}

HRESULT CompositeMessage::DeleteAttach(ULONG ulAttachmentNum, ULONG ulUIParam, LPMAPIPROGRESS lpProgress, ULONG ulFlags)
{
	HRESULT hr = hrSuccess;

	if ((m_ulOverrideFlags & cmofAttachments) == 0) {
		hr = MAPI_E_NO_ACCESS;
		goto exit;
	}

	hr = m_ptrPrimary->DeleteAttach(ulAttachmentNum, ulUIParam, lpProgress, ulFlags);

exit:
	return hr;
}

HRESULT CompositeMessage::GetRecipientTable(ULONG ulFlags, LPMAPITABLE *lppTable)
{
	return m_ptrPrimary->GetRecipientTable(ulFlags, lppTable);
}

HRESULT CompositeMessage::ModifyRecipients(ULONG ulFlags, LPADRLIST lpMods)
{
	return m_ptrPrimary->ModifyRecipients(ulFlags, lpMods);
}

HRESULT CompositeMessage::SubmitMessage(ULONG ulFlags)
{
	return MAPI_E_NO_SUPPORT;
}

HRESULT CompositeMessage::SetReadFlag(ULONG ulFlags)
{
	return MAPI_E_NO_SUPPORT;
}


HRESULT CompositeMessage::CopyProperty(ULONG ulPropTag)
{
	HRESULT hr = hrSuccess;
	SPropValuePtr ptrProp;

	/*
	 * We perform a HrGetOneProp first, not necessarily in an attemp to
	 * increase performance, but to detect the potential non-existence
	 * of the property, which Util::DoCopyProps hides from us.
	 */
	hr = HrGetOneProp(m_ptrSecondary, ulPropTag, &ptrProp);
	if (hr == hrSuccess)
		hr = m_ptrPrimary->SetProps(1, ptrProp, NULL);

	else if (hr == MAPI_E_NOT_ENOUGH_MEMORY) {
		SizedSPropTagArray(1, sptaCopyTags) = {1, {ulPropTag}};

		hr = Util::DoCopyProps(&m_ptrSecondary.iid, m_ptrSecondary, (LPSPropTagArray)&sptaCopyTags,
		                       0, NULL, &m_ptrPrimary.iid, m_ptrPrimary, 0, NULL);
	}

	return hr;
}



DEF_ULONGMETHOD(TRACE_MAPI, CompositeMessage, Message, AddRef, (void))
DEF_ULONGMETHOD(TRACE_MAPI, CompositeMessage, Message, Release, (void))
DEF_HRMETHOD(TRACE_MAPI, CompositeMessage, Message, QueryInterface, (REFIID, refiid), (void **, lppInterface))

DEF_HRMETHOD(TRACE_MAPI, CompositeMessage, Message, GetLastError, (HRESULT, hError), (ULONG, ulFlags), (LPMAPIERROR *, lppMapiError))
DEF_HRMETHOD(TRACE_MAPI, CompositeMessage, Message, SaveChanges, (ULONG, ulFlags))
DEF_HRMETHOD(TRACE_MAPI, CompositeMessage, Message, GetProps, (LPSPropTagArray, lpPropTagArray), (ULONG, ulFlags), (ULONG FAR *, lpcValues), (LPSPropValue FAR *, lppPropArray))
DEF_HRMETHOD(TRACE_MAPI, CompositeMessage, Message, GetPropList, (ULONG, ulFlags), (LPSPropTagArray FAR *, lppPropTagArray))
DEF_HRMETHOD(TRACE_MAPI, CompositeMessage, Message, OpenProperty, (ULONG, ulPropTag), (LPCIID, lpiid), (ULONG, ulInterfaceOptions), (ULONG, ulFlags), (LPUNKNOWN FAR *, lppUnk))
DEF_HRMETHOD(TRACE_MAPI, CompositeMessage, Message, SetProps, (ULONG, cValues), (LPSPropValue, lpPropArray), (LPSPropProblemArray FAR *, lppProblems))
DEF_HRMETHOD(TRACE_MAPI, CompositeMessage, Message, DeleteProps, (LPSPropTagArray, lpPropTagArray), (LPSPropProblemArray FAR *, lppProblems))
DEF_HRMETHOD(TRACE_MAPI, CompositeMessage, Message, CopyTo, (ULONG, ciidExclude), (LPCIID, rgiidExclude), (LPSPropTagArray, lpExcludeProps), (ULONG, ulUIParam), (LPMAPIPROGRESS, lpProgress), (LPCIID, lpInterface), (LPVOID, lpDestObj), (ULONG, ulFlags), (LPSPropProblemArray FAR *, lppProblems))
DEF_HRMETHOD(TRACE_MAPI, CompositeMessage, Message, CopyProps, (LPSPropTagArray, lpIncludeProps), (ULONG, ulUIParam), (LPMAPIPROGRESS, lpProgress), (LPCIID, lpInterface), (LPVOID, lpDestObj), (ULONG, ulFlags), (LPSPropProblemArray FAR *, lppProblems))
DEF_HRMETHOD(TRACE_MAPI, CompositeMessage, Message, GetNamesFromIDs, (LPSPropTagArray *, pptaga), (LPGUID, lpguid), (ULONG, ulFlags), (ULONG *, pcNames), (LPMAPINAMEID **, pppNames))
DEF_HRMETHOD(TRACE_MAPI, CompositeMessage, Message, GetIDsFromNames, (ULONG, cNames), (LPMAPINAMEID *, ppNames), (ULONG, ulFlags), (LPSPropTagArray *, pptaga))
DEF_HRMETHOD(TRACE_MAPI, CompositeMessage, Message, GetAttachmentTable, (ULONG, ulFlags), (LPMAPITABLE *, lppTable))
DEF_HRMETHOD(TRACE_MAPI, CompositeMessage, Message, OpenAttach, (ULONG, ulAttachmentNum), (LPCIID, lpInterface), (ULONG, ulFlags), (LPATTACH *, lppAttach))
DEF_HRMETHOD(TRACE_MAPI, CompositeMessage, Message, CreateAttach, (LPCIID, lpInterface), (ULONG, ulFlags), (ULONG *, lpulAttachmentNum), (LPATTACH *, lppAttach))
DEF_HRMETHOD(TRACE_MAPI, CompositeMessage, Message, DeleteAttach, (ULONG, ulAttachmentNum), (ULONG, ulUIParam), (LPMAPIPROGRESS, lpProgress), (ULONG, ulFlags))
DEF_HRMETHOD(TRACE_MAPI, CompositeMessage, Message, GetRecipientTable, (ULONG, ulFlags), (LPMAPITABLE *, lppTable))
DEF_HRMETHOD(TRACE_MAPI, CompositeMessage, Message, ModifyRecipients, (ULONG, ulFlags), (LPADRLIST, lpMods))
DEF_HRMETHOD(TRACE_MAPI, CompositeMessage, Message, SubmitMessage, (ULONG, ulFlags))
DEF_HRMETHOD(TRACE_MAPI, CompositeMessage, Message, SetReadFlag, (ULONG, ulFlags))
