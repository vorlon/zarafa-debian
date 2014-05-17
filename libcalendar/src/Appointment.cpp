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
#include "Appointment.h"
#include "Occurrence.h"
#include "OccurrenceData.h"
#include "RecurrencePattern.h"
#include "CalendarUtil.h"
#include "Conversions.h"

#include "mapi_ptr.h"
#include "ECGuid.h"
#include "CommonUtil.h"
#include "namedprops.h"
#include "Trace.h"
#include "ECInterfaceDefs.h"
#include "RecurrenceState.h"
#include "ECRestriction.h"

#include <exception>
#include <mapiext.h>

#include "PropertySet.h"

typedef mapi_object_ptr<Appointment> AppointmentPtr;
typedef mapi_array_ptr<ULONG> UlongPtr;

#define _PROP(_ppool, _name) ((_ppool)->GetPropTag(PropertyPool::PROP_##_name))
#define PROP(_name) (_PROP(m_ptrPropertyPool, _name))

/**
 * Create an Appointment instance based on an IMessage.
 * 
 * @param[in]	lpMessage		The MAPI message that represents the appointment.
 * @param[in]	lpClientTZ		The timezone definition of the client. This is used to
 * 								perform conversions that are based on the local time of
 * 								the client.
 * @param[out]	lppAppointment	The new Appointment instance. This must be released
 * 								by the caller.
 */
HRESULT Appointment::Create(IMessage *lpMessage, TimezoneDefinition *lpClientTZ, IAppointment **lppAppointment)
{
	HRESULT hr = hrSuccess;
	AppointmentPtr ptrAppointment;

	if (lppAppointment == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	hr = Create(NULL, lpClientTZ, &ptrAppointment);
	if (hr != hrSuccess)
		goto exit;

	hr = ptrAppointment->Attach(lpMessage);
	if (hr != hrSuccess)
		goto exit;

	hr = ptrAppointment->QueryInterface(IID_IAppointment, (LPVOID*)lppAppointment);

exit:
	return hr;
}

/**
 * Create an Appointment based on a set of properties. This requires certain properties
 * to be present in the set. Use PropertyPool::GetRequiredPropTags to determine which
 * properties should be passed to SetColumns or GetProps.
 * Note that there's no good reason to get these properties through GetProps.
 * 
 * @param[in]	cValues			The number of properties in lpPropArray.
 * @param[in]	lpPropArray		The array of properties.
 * @param[in]	lpPropertyPool	The PropertyPool instance that contains the mapping of
 * 								names to proptags.
 * @param[in]	lpClientTZ		The timezone definition of the client. This is used to
 * 								perform conversions that are based on the local time of
 * 								the client.
 * @param[out]	lppAppointment	The new Appointment instance. This must be released by
 * 								the caller.
 */
HRESULT Appointment::Create(ULONG cValues, LPSPropValue lpPropArray, PropertyPool *lpPropertyPool, TimezoneDefinition *lpClientTZ, IAppointment **lppAppointment)
{
	HRESULT hr = hrSuccess;
	AppointmentPtr ptrAppointment;

	if (cValues == 0 || lpPropArray == NULL || lpPropertyPool == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	hr = Create(lpPropertyPool, lpClientTZ, &ptrAppointment);
	if (hr != hrSuccess)
		goto exit;

	hr = ptrAppointment->ProcessProps(cValues, lpPropArray);
	if (hr != hrSuccess)
		goto exit;

	hr = ptrAppointment->QueryInterface(IID_IAppointment, (LPVOID*)lppAppointment);

exit:
	return hr;
}

/**
 * Create a new appointment in lpFolder. This returns an Appointment instance
 * AND an IMessage instance. When changes are made to the appointment, they must
 * be applies by calling ApplyChanges on the appointment. This causes the changes
 * to be saved in the message instance. The changes to the message are not saved
 * to the MAPI backend untill SaveChanges is called on the message.
 * 
 * @param[in]	lpFolder		The folder in which to create the appointment.
 * @param[in]	lpClientTZ		The timezone definition of the client. This is used to
 * 								perform conversions that are based on the local time of
 * 								the client.
 * @param[out]	lppAppointment	The new Appointment instance. This must be released
 * 								by the caller.
 * @param[out[	lppMessage		The IMessage instance that is the storage for the
 * 								appointment. This must be released by the caller.
 */
HRESULT Appointment::Create(IMAPIFolder *lpFolder, TimezoneDefinition *lpClientTZ, IAppointment **lppAppointment, IMessage **lppMessage)
{
	HRESULT hr = hrSuccess;
	MessagePtr ptrMsg;

	if (lpFolder == NULL || lppAppointment == NULL || lppMessage == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	hr = lpFolder->CreateMessage(NULL, 0, &ptrMsg);
	if (hr != hrSuccess)
		goto exit;

	hr = SetDefaults(ptrMsg);
	if (hr != hrSuccess)
		goto exit;

	hr = Create(ptrMsg, lpClientTZ, lppAppointment);
	if (hr != hrSuccess)
		goto exit;

	*lppMessage = ptrMsg.release();

exit:
	return hr;
}

/**
 * Create an empty Appointment instance that is not associated with any MAPI storage.
 * 
 * @param[in]	lpPropertyPool	The PropertyPool instance that contains the mapping of
 * 								names to proptags.
 * @param[in]	lpClientTZ		The timezone definition of the client. This is used to
 * 								perform conversions that are based on the local time of
 * 								the client.
 * @param[out]	lppAppointment	The new Appointment instance. This must be released
 * 								by the caller.
 */
HRESULT Appointment::Create(PropertyPool *lpPropertyPool, TimezoneDefinition *lpClientTZ, Appointment **lppAppointment)
{
	HRESULT hr = hrSuccess;
	AppointmentPtr ptrAppointment;

	try {
		ptrAppointment.reset(new Appointment(lpPropertyPool, lpClientTZ));
	} catch (const std::bad_alloc &) {
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}

	*lppAppointment = ptrAppointment.release();

exit:
	return hr;
}

Appointment::Appointment(PropertyPool *lpPropertyPool, TimezoneDefinition *lpClientTZ)
: m_ptrPropertyPool(lpPropertyPool)
, m_ptrClientTZ(lpClientTZ)
{ }

Appointment::~Appointment()
{ }

/**
 * Attach a MAPI message to the Appointment instance. This will cause the appointment to
 * contain valid information.
 * 
 * @param[in]	lpMessage	The MAPI message representing the appointment
 */
HRESULT Appointment::Attach(IMessage *lpMessage)
{
	HRESULT hr = hrSuccess;
	PropertyPoolPtr ptrPropertyPool;
	SPropTagArrayPtr ptrPropTags;
	ULONG cValues;
	SPropValuePtr ptrProperties;

	if (lpMessage == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	hr = HrGetOneProp(lpMessage, PR_MESSAGE_CLASS_W, &ptrProperties);
	if (hr != hrSuccess)
		goto exit;

	// For now we only support appointments. Other types, like meeting requests, will be added
	// once officially supported
	if (wcsncmp(ptrProperties->Value.lpszW, L"IPM.Appointment", wcslen(L"IPM.Appointment")) != 0 ||
		(ptrProperties->Value.lpszW[wcslen(L"IPM.Appointment")] != '\0' &&
		 ptrProperties->Value.lpszW[wcslen(L"IPM.Appointment")] != '.'))
	{
		hr = MAPI_E_INVALID_TYPE;
		goto exit;
	}

	hr = PropertyPool::Create(lpMessage, &ptrPropertyPool);
	if (hr != hrSuccess)
		goto exit;

	hr = ptrPropertyPool->GetRequiredPropTags(NULL, &ptrPropTags);
	if (hr != hrSuccess)
		goto exit;

	hr = lpMessage->GetProps(ptrPropTags, 0, &cValues, &ptrProperties);
	if (FAILED(hr))
		goto exit;

	m_ptrPropertyPool = ptrPropertyPool;
	hr = ProcessProps(cValues, ptrProperties);
	if (hr != hrSuccess)
		goto exit;

	m_ptrMessage.reset(lpMessage);

exit:
	return hr;
}

/**
 * Process a set of properties to configure the Appointment instance internals.
 * The properties are either obtained from an SRow or from a GetProps call.
 * 
 * @param[in]	cValues		The number of propvals in lpPropArray
 * @param[in]	lpPropArray	The array with the propvals
 */
HRESULT Appointment::ProcessProps(ULONG cValues, LPSPropValue lpPropArray)
{
	HRESULT hr = hrSuccess;
	HRESULT hrTmp;
	LPSPropValue lpProp;

	FILETIME ftStartDateTime = {0, 0};
	FILETIME ftEndDateTime = {0, 0};
	ULONG ulBusyStatus = 0;
	std::wstring strSubject;
	std::wstring strLocation;
	ULONG ulMeetingType = 0;
	unsigned short int fSubType = FALSE;
	unsigned short int fHasAttach = FALSE;
	ULONG ulReminderDelta = 0;
	unsigned short int fReminderSet = FALSE;
	TimezoneDefinitionPtr ptrTZStartTime;
	TimezoneDefinitionPtr ptrTZDefRecur;
	RecurrencePatternPtr ptrRecurrencePattern;
	BasedateSet setDeleted;
	ExceptionMap mapModifyExceptions;
	ULONG ulStartTimeOffset = (ULONG)-1;
	ULONG ulEndTimeOffset = (ULONG)-1;
	std::auto_ptr<OccurrenceData> ptrOccurrenceData;

	ASSERT(cValues > 0);
	ASSERT(lpPropArray != NULL);
	ASSERT(m_ptrPropertyPool != NULL);

	lpProp = PpropFindProp(lpPropArray, cValues, PROP(BUSYSTATUS));
	if (lpProp)
		ulBusyStatus = lpProp->Value.ul;

	lpProp = PpropFindProp(lpPropArray, cValues, PR_SUBJECT_W);
	if (lpProp)
		strSubject.assign(lpProp->Value.lpszW);

	lpProp = PpropFindProp(lpPropArray, cValues, PROP(LOCATION));
	if (lpProp)
		strLocation.assign(lpProp->Value.lpszW);

	lpProp = PpropFindProp(lpPropArray, cValues, PROP(MEETINGSTATUS));
	if (lpProp)
		ulMeetingType = lpProp->Value.ul;

	lpProp = PpropFindProp(lpPropArray, cValues, PROP(ALLDAYEVENT));
	if (lpProp)
		fSubType = lpProp->Value.b;

	lpProp = PpropFindProp(lpPropArray, cValues, PR_HASATTACH);
	if (lpProp)
		fHasAttach = lpProp->Value.b;

	lpProp = PpropFindProp(lpPropArray, cValues, PROP(REMINDERMINUTESBEFORESTART));
	if (lpProp)
		ulReminderDelta = lpProp->Value.ul;

	lpProp = PpropFindProp(lpProp, cValues, PROP(REMINDERSET));
	if (lpProp)
		fReminderSet = lpProp->Value.b;

	lpProp = PpropFindAny(lpPropArray, cValues)
				.tag(PROP(APPTSTARTWHOLE))
				.tag(PROP(COMMONSTART))
				.tag(PR_START_DATE);
	if (lpProp)
		ftStartDateTime = lpProp->Value.ft;

	lpProp = PpropFindAny(lpPropArray, cValues)
				.tag(PROP(APPTENDWHOLE))
				.tag(PROP(COMMONEND))
				.tag(PR_END_DATE);
	if (lpProp)
		ftEndDateTime = lpProp->Value.ft;

	lpProp = PpropFindProp(lpPropArray, cValues, PROP(APPTTZDEFSTARTDISP));
	if (lpProp)
		hr = TimezoneDefinition::FromBlob(lpProp->Value.bin.cb, lpProp->Value.bin.lpb, &ptrTZStartTime);

	lpProp = PpropFindProp(lpPropArray, cValues, PROP(RECURRING));
	if (lpProp && lpProp->Value.b == TRUE) {
		lpProp = PpropFindProp(lpPropArray, cValues, PROP(RECURRENCESTATE));
		if (lpProp) {
			std::auto_ptr<RecurrenceState> ptrRecurrenceState(new RecurrenceState());

			hr = ptrRecurrenceState->ParseBlob((char*)lpProp->Value.bin.lpb, lpProp->Value.bin.cb, 0);
			if (FAILED(hr))
				goto exit;	// Or ignore

			hr = RecurrencePattern::Create(ptrRecurrenceState.get(), &ptrRecurrencePattern);
			if (hr != hrSuccess)
				goto exit;

			for (std::vector<RecurrenceState::Exception>::size_type i = 0; i < ptrRecurrenceState->lstExceptions.size(); ++i) {
				Exception exception = {
					ptrRecurrenceState->lstExceptions[i],
					ptrRecurrenceState->lstExtendedExceptions[i]
				};

				ULONG ulOriginalStartDate = exception.basic.ulOriginalStartDate; // This is really a start datetime
				mapModifyExceptions[ulOriginalStartDate - ulOriginalStartDate % 1440] = exception;
			}

			std::copy(ptrRecurrenceState->lstDeletedInstanceDates.begin(),
			          ptrRecurrenceState->lstDeletedInstanceDates.end(),
			          std::inserter(setDeleted, setDeleted.end()));

			ulStartTimeOffset = ptrRecurrenceState->ulStartTimeOffset;
			ulEndTimeOffset = ptrRecurrenceState->ulEndTimeOffset;
		}

		hrTmp = GetTZDef(cValues, lpPropArray, ftStartDateTime, &ptrTZDefRecur);
		if (hrTmp != hrSuccess && hrTmp != MAPI_E_NOT_FOUND) {
			hr = hrTmp;
			goto exit;	// Or ignore?
		}
	}

	ptrOccurrenceData.reset(new OccurrenceData(ftStartDateTime, ftEndDateTime, ulBusyStatus,
	                                           strLocation.c_str(), ulReminderDelta, strSubject.c_str(),
	                                           fReminderSet, ulMeetingType, fSubType));

	m_ptrOccurrenceData = ptrOccurrenceData;	// Transfers ownership
	m_ptrRecurrencePattern.reset(ptrRecurrencePattern);
	m_ptrTZStartTime = ptrTZStartTime;
	m_ptrTZDefRecur = ptrTZDefRecur;
	m_mapModifyExceptions.swap(mapModifyExceptions);
	m_setDeletedExceptions.swap(setDeleted);
	m_ulStartTimeOffset = ulStartTimeOffset;
	m_ulEndTimeOffset = ulEndTimeOffset;
	m_bHasAttach = fHasAttach;

	hr = UpdateTimeOffsets(ptrTZDefRecur);
	if (hr != hrSuccess)
		goto exit;

exit:
	return hr;
}

/**
 * Do a best attempt to get a valid timezone definition from the set of properties.
 * This is done by first trying ulPropTagSpecific. If that's not available the
 * APPTTZDEFRECUR property is tried. If that doesn't exist the TIMEZONE property
 * is tried in order to get the timezone by name. If that fails the TIMEZONEDATA
 * property is attempted. This will however return a timezone rule. A matching
 * timezone definition will be obtained by getting the name of a timezone that
 * contains the rule.
 * 
 * If all of the above fails, MAPI_E_NOT_FOUND is returned.
 * 
 * @param[in]	cValues				The count of properties in lpPropArray.
 * @param[in]	lpPropArray			The array of properties for the appointment.
 * @param[in]	ftTimestamp			The timestamp of the start of the appointment.
 * 									This is used during the matching of the rule.
 * @param[out]	lppTZDef			The resulting timezone definition. This must be
 * 									released by the caller.
 */
HRESULT Appointment::GetTZDef(ULONG cValues, LPSPropValue lpPropArray, FILETIME ftTimestamp, TimezoneDefinition **lppTZDef)
{
	HRESULT hr = MAPI_E_NOT_FOUND;
	LPSPropValue lpProp;

	lpProp = PpropFindProp(lpPropArray, cValues, PROP(APPTTZDEFRECUR));
	if (lpProp)
		hr = TimezoneDefinition::FromBlob(lpProp->Value.bin.cb, lpProp->Value.bin.lpb, lppTZDef);

	else {
		// First attempt to get the definition by name. This doesn't always work
		// because it can also be something like GMT +1
		lpProp = PpropFindProp(lpPropArray, cValues, PROP(TIMEZONE));
		if (lpProp)
			hr = HrGetTZDefByName(lpProp->Value.lpszW, lppTZDef);

		if (lpProp == NULL || hr == MAPI_E_NOT_FOUND) {
			lpProp = PpropFindProp(lpPropArray, cValues, PROP(TIMEZONEDATA));
			if (lpProp) {
				TimezoneRulePtr ptrTZRule;
				std::wstring wstrName;

				hr = TimezoneRule::FromBlob(lpProp->Value.bin.cb, lpProp->Value.bin.lpb, &ptrTZRule);
				if (hr != hrSuccess)
					goto exit;	// Or ignore?

				hr = HrGetTZNameByRule(ftTimestamp, ptrTZRule, 0, &wstrName);
				if (hr != hrSuccess)
					goto exit;	// Or ignore?
				
				hr = HrGetTZDefByName(wstrName, lppTZDef);
			}
		}
	}

exit:
	return hr;
}

HRESULT	Appointment::QueryInterface(REFIID refiid, void **lppInterface)
{
	REGISTER_INTERFACE(IID_Appointment, this);
	REGISTER_INTERFACE(IID_ECUnknown, this);

	REGISTER_INTERFACE(IID_IAppointment, &this->m_xAppointment);
	REGISTER_INTERFACE(IID_IOccurrence, &this->m_xAppointment);
	REGISTER_INTERFACE(IID_IUnknown, &this->m_xAppointment);

	return MAPI_E_INTERFACE_NOT_SUPPORTED;
}

HRESULT Appointment::SetRecurrence(RecurrencePattern *lpPattern)
{
	HRESULT hr = hrSuccess;
	RecurrencePatternPtr ptrCopy;
	
	if (lpPattern != NULL) {
		hr = lpPattern->Clone(&ptrCopy);
		if (hr != hrSuccess)
			goto exit;

		hr = UpdateTimeOffsets(m_ptrTZDefRecur);
		if (hr != hrSuccess)
			goto exit;
	} else {
		m_ulStartTimeOffset = (ULONG)-1;
		m_ulEndTimeOffset = (ULONG)-1;
		m_ptrTZDefRecur.reset();
	}

	m_mapModifyExceptions.clear();
	m_setDeletedExceptions.clear();

	m_ptrRecurrencePattern.reset(ptrCopy);

exit:
	return hr;
}

HRESULT Appointment::SetRecurrenceTimezone(TimezoneDefinition *lpTZDef)
{
	HRESULT hr = hrSuccess;

	hr = UpdateTimeOffsets(lpTZDef);
	if (hr != hrSuccess)
		goto exit;

	m_ptrTZDefRecur.reset(lpTZDef);

exit:
	return hr;
}

HRESULT Appointment::GetRecurrence(RecurrencePattern **lppPattern)
{
	HRESULT hr = hrSuccess;

	if (lppPattern == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (!m_ptrRecurrencePattern) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	hr = m_ptrRecurrencePattern->Clone(lppPattern);
	if (hr != hrSuccess)
		goto exit;

exit:
	return hr;
}

HRESULT Appointment::GetRecurrenceTimezone(TimezoneDefinition **lppTZDef)
{
	HRESULT hr = hrSuccess;

	if (lppTZDef == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (m_ptrTZDefRecur) {
		*lppTZDef = m_ptrTZDefRecur;
		(*lppTZDef)->AddRef();
	} else
		*lppTZDef = NULL;

exit:
	return hr;
}

HRESULT Appointment::GetBounds(ULONG *lpulFirst, ULONG *lpulLast)
{
	HRESULT hr = hrSuccess;
	ULONG ulRangeType = 0;

	if (lpulFirst == NULL || lpulLast == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (!m_ptrRecurrencePattern)
		hr = GetBoundsSingle(lpulFirst, lpulLast);
		
	else {
		hr = m_ptrRecurrencePattern->GetRangeType(&ulRangeType);
		if (hr != hrSuccess)
			goto exit;

		if (ulRangeType != ET_NEVER)
			hr = GetBoundsRecurring(lpulFirst, lpulLast);

		else {
			hr = FindFirstOccurrence(lpulFirst);
			if (hr == hrSuccess)
				*lpulLast = 0;
		}
	}


exit:
	return hr;
}

/**
 * Get the bounds of a single appointment. This will return the start date
 * of the appointment for both the first and the last occurrence.
 * 
 * @param[out]	lpulFirst	The date, specified in rtime, of the first occurrence.
 * @param[out]	lpulLast	The date, specified in rtime, of the last occurrence.
 */
HRESULT Appointment::GetBoundsSingle(ULONG *lpulFirst, ULONG *lpulLast)
{
	HRESULT hr;
	FILETIME ftStartDateTime;
	ULONG ulStartDate;

	hr = GetStartDateTime(m_ptrClientTZ, &ftStartDateTime);
	if (hr != hrSuccess)
		goto exit;

	hr = BaseDateFromFileTime(ftStartDateTime, &ulStartDate);
	if (hr != hrSuccess)
		goto exit;

	*lpulFirst = *lpulLast = ulStartDate;

exit:
	return hr;
}

/**
 * Get the bounds of a recurring appointment.
 * 
 * @param[out]	lpulFirst	The original start date, specified in rtime, of the first occurrence.
 * @param[out]	lpulLast	The original start date, specified in rtime, of the last occurrence.
 */
HRESULT Appointment::GetBoundsRecurring(ULONG *lpulFirst, ULONG *lpulLast)
{
	HRESULT hr = hrSuccess;
	ULONG ulFirst = 0;
	ULONG ulLast = 0;;
	ULONG cBaseDates = 0;
	UlongPtr ptrBaseDates;
	ULONG cDeleted = 0;
	UlongPtr ptrDeleted;
	std::vector<ULONG> vBaseDates;

	hr = m_ptrRecurrencePattern->GetBounds(&ulFirst, &ulLast);
	if (hr != hrSuccess)
		goto exit;

	hr = m_ptrRecurrencePattern->GetOccurrencesInRange(ulFirst, ulLast, &cBaseDates, &ptrBaseDates);
	if (hr != hrSuccess)
		goto exit;

	hr = GetExceptions(NULL, NULL, &cDeleted, &ptrDeleted);
	if (hr != hrSuccess)
		goto exit;

	if (cDeleted == cBaseDates) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	vBaseDates.reserve(cBaseDates - cDeleted);
	std::set_difference(ptrBaseDates.get(), ptrBaseDates.get() + cBaseDates, ptrDeleted.get(), ptrDeleted.get() + cDeleted, std::back_inserter(vBaseDates));

	*lpulFirst = vBaseDates.front();
	*lpulLast = vBaseDates.back();

exit:
	return hr;
}

/**
 * Get the date of the first occurrence of the appointment.
 * 
 * @param[out]	lpulDate	The original start date, specified in rtime, of the first occurrence.
 */
HRESULT Appointment::FindFirstOccurrence(ULONG *lpulDate)
{
	HRESULT hr = hrSuccess;;
	ULONG cModified = 0;
	UlongPtr ptrModified;
	ULONG cDeleted = 0;
	UlongPtr ptrDeleted;
	ULONG ulDate = 0;
	ULONG ulLoopCount = 0;

	hr = GetExceptions(&cModified, &ptrModified, &cDeleted, &ptrDeleted);
	if (hr != hrSuccess)
		goto exit;

	for (ulLoopCount = 0; ulLoopCount <= cDeleted; ++ulLoopCount) {
		hr = m_ptrRecurrencePattern->GetOccurrence(ulDate, &ulDate);
		if (hr != hrSuccess)
			goto exit;

		if (cModified > 0 && ulDate == ptrModified[0]) {
			// ulDate is an exception, so not deleted. Since occurrence can't be
			// reordered, this must be the first occurrence.
			break;
		}

		if (std::binary_search(ptrDeleted.get(), ptrDeleted.get() + cDeleted, ulDate) == false) {
			// ulDate is not found in the deleted list.
			break;
		}

		ulDate += 1440;
	}

	if (ulLoopCount == cDeleted + 1) {
		// If we end up here we tried the first cDeleted + 1 occurrences and still found
		// no match, which is impossible. However we don't want the possibility of an
		// endless loop, so we'll put some code here to handle potential issues with this
		// code.
		// (This is bad for coverage though)
		ASSERT(FALSE);
		hr = MAPI_E_CALL_FAILED;
		goto exit;
	}

	*lpulDate = ulDate;

exit:
	return hr;
}

HRESULT Appointment::GetOccurrence(ULONG ulBaseDate, IOccurrence **lppOccurrence)
{
	HRESULT hr = hrSuccess;
	bool bIsOccurrence;
	ExceptionMap::const_iterator iterModifyException;

	if (lppOccurrence == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (!m_ptrRecurrencePattern) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	// First check if the basedate is an occurrence
	hr = m_ptrRecurrencePattern->IsOccurrence(ulBaseDate, &bIsOccurrence);
	if (hr != hrSuccess)
		goto exit;

	if (bIsOccurrence == false) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	// Next check if there's an modify exception for the basedate
	iterModifyException = m_mapModifyExceptions.find(ulBaseDate);
	if (iterModifyException != m_mapModifyExceptions.end()) {
		hr = Occurrence::Create(ulBaseDate, m_ulStartTimeOffset, m_ulEndTimeOffset, m_ptrTZDefRecur,
		                        m_ptrOccurrenceData.get(), &iterModifyException->second, this, lppOccurrence);
		goto exit;
	}

	// Finally check if the occurrence was deleted
	if (m_setDeletedExceptions.find(ulBaseDate) != m_setDeletedExceptions.end()) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	hr = Occurrence::Create(ulBaseDate, m_ulStartTimeOffset, m_ulEndTimeOffset, m_ptrTZDefRecur,
	                        m_ptrOccurrenceData.get(), NULL, this, lppOccurrence);

exit:
	return hr;
}

HRESULT Appointment::RemoveOccurrence(ULONG ulBaseDate)
{
	HRESULT hr = hrSuccess;
	bool bIsOccurrence;

	if (!m_ptrRecurrencePattern) {
		hr = MAPI_E_NO_SUPPORT;
		goto exit;
	}

	hr = m_ptrRecurrencePattern->IsOccurrence(ulBaseDate, &bIsOccurrence);
	if (hr != hrSuccess)
		goto exit;

	if (!bIsOccurrence) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	/*
	 * We'll first try to remove the occurrence from the modified exceptions
	 * map. If that succeeds we removed an exception. All exceptions are already
	 * in the deleted occurrences set, so we're done.
	 * If it fails we'll attempt to add the occurrence to the deleted occurrences
	 * map. If that fails the item was already deleted.
	 */
	if (m_mapModifyExceptions.erase(ulBaseDate) == 0) {
		if (m_setDeletedExceptions.insert(ulBaseDate).second == false)
			hr = MAPI_E_NOT_FOUND;	// Unable to delete a deleted occurrence
	} else
		ASSERT(m_setDeletedExceptions.find(ulBaseDate) != m_setDeletedExceptions.end());

exit:
	return hr;
}

HRESULT Appointment::ResetOccurrence(ULONG ulBaseDate)
{
	HRESULT hr = hrSuccess;
	bool bIsOccurrence;
	std::vector<unsigned int>::iterator iOccurrence;

	if (!m_ptrRecurrencePattern) {
		hr = MAPI_E_NO_SUPPORT;
		goto exit;
	}

	hr = m_ptrRecurrencePattern->IsOccurrence(ulBaseDate, &bIsOccurrence);
	if (hr != hrSuccess)
		goto exit;

	if (!bIsOccurrence) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	// If ulBaseDate exists in m_mapModifiedExceptions, the exception
	// was a modified exception, which should also have an entry in
	// m_setDeletedExceptions. So also remove that.
	// If ulBaseDate does not exist in m_mapModifiedException, the exception
	// must be a delete exception. So failure to remove it from
	// m_setDeletedExceptions implies that there was no exception for
	// ulBaseDate.
	if (m_mapModifyExceptions.erase(ulBaseDate) == 1) {
		BasedateSet::size_type cnt = m_setDeletedExceptions.erase(ulBaseDate);
		ASSERT(cnt == 1);
	} else if (m_setDeletedExceptions.erase(ulBaseDate) == 0) {
		hr = MAPI_E_NOT_FOUND;
	}

exit:
	return hr;
}

HRESULT Appointment::GetExceptions(ULONG *lpcbModifyExceptions, ULONG **lpaModifyExceptions,
                                   ULONG *lpcbDeleteExceptions, ULONG **lpaDeleteExceptions)
{
	HRESULT hr = hrSuccess;
	ULONG cModified = 0;
	UlongPtr ptrModified;
	ULONG cDeleted = 0;
	UlongPtr ptrDeleted;

	hr = MAPIAllocateBuffer(sizeof(ULONG) * m_mapModifyExceptions.size(), &ptrModified);
	if (hr != hrSuccess)
		goto exit;

	for (ExceptionMap::iterator i = m_mapModifyExceptions.begin(); i != m_mapModifyExceptions.end(); ++i)
		ptrModified[cModified++] = i->first;

	hr = MAPIAllocateBuffer(sizeof(ULONG) * m_setDeletedExceptions.size(), &ptrDeleted);
	if (hr != hrSuccess)
		goto exit;

	for (BasedateSet::iterator i = m_setDeletedExceptions.begin(); i != m_setDeletedExceptions.end(); ++i) {
		if (m_mapModifyExceptions.find(*i) == m_mapModifyExceptions.end())
			ptrDeleted[cDeleted++] = *i;
	}

	if (lpcbModifyExceptions)
		*lpcbModifyExceptions = cModified;

	if (lpaModifyExceptions)
		*lpaModifyExceptions = ptrModified.release();

	if (lpcbDeleteExceptions)
		*lpcbDeleteExceptions = cDeleted;

	if (lpaDeleteExceptions)
		*lpaDeleteExceptions = ptrDeleted.release();

exit:
	return hr;
}

HRESULT Appointment::GetBaseDateForOccurrence(ULONG ulIndex, ULONG *lpulBaseDate)
{
	HRESULT hr = hrSuccess;
	ULONG ulBaseDate = 0;

	if (lpulBaseDate == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (!m_ptrRecurrencePattern) {
		hr = MAPI_E_NO_SUPPORT;
		goto exit;
	}

	hr = m_ptrRecurrencePattern->GetOccurrence(0, &ulBaseDate);
	if (hr != hrSuccess)
		goto exit;

	for (ULONG i = 0; i < ulIndex; ++i) {
		// We'll increment the last found basedate with 1 day, and 
		// RecurrencePatter::GetOccurrence will return the next valid
		// basedate for us.
		hr = m_ptrRecurrencePattern->GetOccurrence(ulBaseDate + 1440, &ulBaseDate);
		if (hr != hrSuccess)
			goto exit;
	}

	*lpulBaseDate = ulBaseDate;

exit:
	return hr;
}

HRESULT Appointment::ApplyChanges()
{
	HRESULT hr = hrSuccess;
	PropertySet properties;

	if (!m_ptrMessage) {
		hr = MAPI_E_NO_ACCESS;
		goto exit;
	}

	if (m_ptrOccurrenceData->ftStart.dirty()) {
		ULONG ulTags[] = {
			PROP(APPTSTARTWHOLE),
			PROP(COMMONSTART),
			PR_START_DATE
		};

		for (size_t i = 0; i < arraySize(ulTags); ++i)
			properties[ulTags[i]].ft = m_ptrOccurrenceData->ftStart;
	}

	if (m_ptrOccurrenceData->ftEnd.dirty()) {
		ULONG ulTags[] = {
			PROP(APPTENDWHOLE),
			PROP(COMMONEND),
			PR_END_DATE
		};

		for (size_t i = 0; i < arraySize(ulTags); ++i)
			properties[ulTags[i]].ft = m_ptrOccurrenceData->ftEnd;
	}

	if (m_ptrRecurrencePattern)
		hr = OccurrenceDataHelper(m_ptrOccurrenceData).GetDuration(m_ptrTZDefRecur, &properties[PROP(APPTDURATION)].ul);
	else
		hr = OccurrenceDataHelper(m_ptrOccurrenceData).GetDuration(m_ptrClientTZ, &properties[PROP(APPTDURATION)].ul);
	if (hr != hrSuccess)
		goto exit;

	if (m_ptrOccurrenceData->ulBusyStatus.dirty())
		properties[PROP(BUSYSTATUS)].ul = m_ptrOccurrenceData->ulBusyStatus;

	if (m_ptrOccurrenceData->strLocation.dirty()) {
		const std::wstring &strLocation = m_ptrOccurrenceData->strLocation;
		properties[PROP(LOCATION)].lpszW = (LPWSTR)strLocation.c_str();
	}

	if (m_ptrOccurrenceData->ulReminderDelta.dirty())
		properties[PROP(REMINDERMINUTESBEFORESTART)].ul = m_ptrOccurrenceData->ulReminderDelta;

	if (m_ptrOccurrenceData->strSubject.dirty()) {
		const std::wstring &strSubject = m_ptrOccurrenceData->strSubject;
		properties[PR_SUBJECT].lpszW = (LPWSTR)strSubject.c_str();
	}

	if (m_ptrOccurrenceData->fReminderSet.dirty())
		properties[PROP(REMINDERSET)].b = m_ptrOccurrenceData->fReminderSet;

	if (m_ptrOccurrenceData->ulMeetingType.dirty())
		properties[PROP(MEETINGSTATUS)].ul = m_ptrOccurrenceData->ulMeetingType;

	if (m_ptrOccurrenceData->fSubType.dirty())
		properties[PROP(ALLDAYEVENT)].b = m_ptrOccurrenceData->fSubType;

	hr = properties.SetPropsOn(m_ptrMessage);
	if (hr != hrSuccess)
		goto exit;

	m_ptrOccurrenceData->MarkClean();

	hr = UpdateRecurrence();
	if (hr != hrSuccess)
		goto exit;

exit:
	return hr;
}

/**
 * Update the recurrence blob in the message based on the stored
 * recurrence pattern. If no recurrence is set, the recurrence blob
 * will be removed from the message if present.
 */
HRESULT Appointment::UpdateRecurrence()
{
	HRESULT hr = hrSuccess;
	PropertySet properties;
	std::wstring strTZDesc;

	if (m_ptrRecurrencePattern) {
		unsigned int cbBlob = 0;
		BytePtr ptrBlob;
		unsigned char **lppucBlob = NULL;
		RecurrenceState sRecState;
		ULONG ulStart;
		ULONG ulEnd;


		hr = m_ptrRecurrencePattern->UpdateState(&sRecState);
		if (hr != hrSuccess)
			goto exit;


		// Set the start- and end-time offsets
		ASSERT(m_ulStartTimeOffset != (ULONG)-1);
		sRecState.ulStartTimeOffset = m_ulStartTimeOffset;

		ASSERT(m_ulEndTimeOffset != (ULONG)-1);
		sRecState.ulEndTimeOffset = m_ulEndTimeOffset;


		// Update the deleted exceptions
		sRecState.ulDeletedInstanceCount = m_setDeletedExceptions.size();
		sRecState.lstDeletedInstanceDates.reserve(sRecState.ulDeletedInstanceCount);
		std::copy(m_setDeletedExceptions.begin(),
		          m_setDeletedExceptions.end(),
		          std::back_inserter(sRecState.lstDeletedInstanceDates));


		// Update the modified exceptions
		sRecState.ulModifiedInstanceCount = m_mapModifyExceptions.size();
		sRecState.ulExceptionCount = sRecState.ulModifiedInstanceCount;
		sRecState.lstModifiedInstanceDates.reserve(sRecState.ulModifiedInstanceCount);
		sRecState.lstExceptions.reserve(sRecState.ulModifiedInstanceCount);
		sRecState.lstExtendedExceptions.reserve(sRecState.ulModifiedInstanceCount);
		for (ExceptionMap::const_iterator i = m_mapModifyExceptions.begin(); i != m_mapModifyExceptions.end(); ++i) {
			sRecState.lstModifiedInstanceDates.push_back(i->first);
			sRecState.lstExceptions.push_back(i->second.basic);
			sRecState.lstExtendedExceptions.push_back(i->second.extended);
		}


		// Get the recurrence blob and write it to PROP_RECURRENCESTATE
		lppucBlob = &ptrBlob;
		hr = sRecState.GetBlob((char **)lppucBlob, &cbBlob);
		if (hr != hrSuccess)
			goto exit;

		hr = WriteBlob(m_ptrMessage, PROP(RECURRENCESTATE), cbBlob, ptrBlob);
		if (hr != hrSuccess)
			goto exit;

		properties[PROP(RECURRING)].b = TRUE;
		properties[PROP(ISRECURRING)].b = TRUE;

		hr = GetBounds(&ulStart, &ulEnd);
		if (hr == hrSuccess) {
			hr = BaseDateToFileTime(ulStart, &properties[PROP(RECURRENCE_START)].ft);
			if (hr != hrSuccess)
				goto exit;

			if (m_ptrTZDefRecur) {
				const FILETIME ft = properties[PROP(RECURRENCE_START)].ft;

				hr = m_ptrTZDefRecur->ToUTC(ft, &properties[PROP(RECURRENCE_START)].ft);
				if (hr != hrSuccess)
					goto exit;
			}

			if (ulEnd != 0) {
				hr = BaseDateToFileTime(ulEnd, &properties[PROP(RECURRENCE_END)].ft);
				if (hr != hrSuccess)
					goto exit;

				if (m_ptrTZDefRecur) {
					const FILETIME ft = properties[PROP(RECURRENCE_END)].ft;

					hr = m_ptrTZDefRecur->ToUTC(ft, &properties[PROP(RECURRENCE_END)].ft);
					if (hr != hrSuccess)
						goto exit;
				}
			} else {
				hr = BaseDateToFileTime(0x5ae980df, &properties[PROP(RECURRENCE_END)].ft);
				if (hr != hrSuccess)
					goto exit;
			}
		} else if (hr != MAPI_E_NOT_FOUND)
			goto exit;
		hr = hrSuccess;

		if (m_ptrTZDefRecur) {
			static const FILETIME ftZero = {0, 0};

			hr = m_ptrTZDefRecur->ToTZDEFINITION(m_ptrOccurrenceData->ftStart, ftZero, &cbBlob, &ptrBlob);
			if (hr != hrSuccess)
				goto exit;

			hr = WriteBlob(m_ptrMessage, PROP(APPTTZDEFRECUR), cbBlob, ptrBlob);
			if (hr != hrSuccess)
				goto exit;

			hr = m_ptrTZDefRecur->ToTZSTRUCT(m_ptrOccurrenceData->ftStart, &cbBlob, &ptrBlob);
			if (hr != hrSuccess)
				goto exit;

			hr = WriteBlob(m_ptrMessage, PROP(TIMEZONEDATA), cbBlob, ptrBlob);
			if (hr != hrSuccess)
				goto exit;

			hr = m_ptrTZDefRecur->GetDisplayName(&strTZDesc);
			if (hr == hrSuccess)
				properties[PROP(TIMEZONE)].lpszW = (LPWSTR)strTZDesc.c_str();
		} else {
			SizedSPropTagArray(3, sptaDeleteProps) = {
				3, {
					PROP(APPTTZDEFRECUR),
					PROP(TIMEZONEDATA),
					PROP(TIMEZONE)
				}
			};

			hr = m_ptrMessage->DeleteProps((LPSPropTagArray)&sptaDeleteProps, NULL);
			if (hr != hrSuccess)
				goto exit;
		}

		properties[PR_ICON_INDEX].ul = 0x401;	// Recurring appointment
	} else {
		SizedSPropTagArray(4, sptaDeleteProps) = {
			4, {
				PROP(RECURRENCESTATE),
				PROP(APPTTZDEFRECUR),
				PROP(TIMEZONEDATA),
				PROP(TIMEZONE)
			}
		};

		hr = m_ptrMessage->DeleteProps((LPSPropTagArray)&sptaDeleteProps, NULL);
		if (hr != hrSuccess)
			goto exit;

		properties[PROP(RECURRING)].b = FALSE;
		properties[PROP(ISRECURRING)].b = FALSE;

		hr = GetStartDateTime(NULL, &properties[PROP(RECURRENCE_START)].ft);
		if (hr != hrSuccess)
			goto exit;

		hr = GetEndDateTime(NULL, &properties[PROP(RECURRENCE_END)].ft);
		if (hr != hrSuccess)
			goto exit;

		properties[PR_ICON_INDEX].ul = 0x400;	// Single-instance appointment
	}

	hr = properties.SetPropsOn(m_ptrMessage);
	if (hr != hrSuccess)
		goto exit;

exit:
	return hr;
}

HRESULT Appointment::UpdateTimeOffsets(TimezoneDefinition *lpTZDef)
{
	HRESULT hr = hrSuccess;
	FILETIME ft;
	LONG rtStart;
	LONG rtEnd;

	hr = OccurrenceDataHelper(m_ptrOccurrenceData).GetStartDateTime(NULL, lpTZDef, &ft);
	if (hr != hrSuccess)
		goto exit;
	FileTimeToRTime(&ft, &rtStart);

	hr = OccurrenceDataHelper(m_ptrOccurrenceData).GetEndDateTime(NULL, lpTZDef, &ft);
	if (hr != hrSuccess)
		goto exit;
	FileTimeToRTime(&ft, &rtEnd);

	m_ulStartTimeOffset = rtStart % 1440;
	m_ulEndTimeOffset = rtEnd - (rtStart - m_ulStartTimeOffset);

exit:
	return hr;
}


HRESULT Appointment::SetDefaults(IMessage *lpMessage)
{
	HRESULT hr = hrSuccess;
	PropertyPoolPtr pp;
	PropertySet properties;
	ULONG cbGoid = 0;
	BytePtr ptrGoid;

	if (lpMessage == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	hr = CreateCleanGOID(&cbGoid, &ptrGoid);
	if (hr != hrSuccess)
		goto exit;

	hr = PropertyPool::Create(lpMessage, &pp);
	if (hr != hrSuccess)
		goto exit;

	// Common properties
	properties[_PROP(pp, APPTSEQNR)].ul = 0;
	properties[_PROP(pp, BUSYSTATUS)].ul = 2;
	properties[_PROP(pp, ALLDAYEVENT)].b = FALSE;
	properties[_PROP(pp, MEETINGSTATUS)].ul = 0;
	properties[_PROP(pp, RESPONSESTATUS)].ul = 0;
	properties[PR_MESSAGE_FLAGS].ul = MSGFLAG_READ | MSGFLAG_UNMODIFIED;
	properties[_PROP(pp, GOID)].bin.cb = cbGoid;
	properties[_PROP(pp, GOID)].bin.lpb = ptrGoid;
	properties[_PROP(pp, CLEANID)].bin.cb = cbGoid;
	properties[_PROP(pp, CLEANID)].bin.lpb = ptrGoid;

	// Appointment & Meeting properties
	properties[PR_MESSAGE_CLASS].LPSZ = _T("IPM.Appointment");
	properties[_PROP(pp, SIDEEFFECT)].ul = seOpenToDelete | seOpenToCopy | seOpenToMove | seCoerceToInbox | seOpenForCtxMenu;

	hr = properties.SetPropsOn(lpMessage);
	if (hr != hrSuccess)
		goto exit;

exit:
	return hr;
}

HRESULT Appointment::SetStartDateTime(FILETIME ftStartDateTime, TimezoneDefinition *lpTZDef)
{
	HRESULT hr = hrSuccess;

	hr = OccurrenceDataHelper(m_ptrOccurrenceData).SetStartDateTime(ftStartDateTime, lpTZDef);
	if (hr != hrSuccess)
		goto exit;

	hr = UpdateTimeOffsets(m_ptrTZDefRecur);
	if (hr != hrSuccess)
		goto exit;

exit:
	return hr;
}

/**
 * Get the start datetime in the specified timezone or in UTC.
 *
 * @param[in]	lpTZDef		The TimezoneDefinion specifying in which timezone
 *							the start datetime should be returned. If NULL is
 *							passed, the start datetime will be returned in UTC.
 * @param[out]	lpftStartDateTime	The start datetime.
 * @return errorcode.
 */
HRESULT Appointment::GetStartDateTime(TimezoneDefinition *lpTZDef, FILETIME *lpftStartDateTime)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).GetStartDateTime(this, lpTZDef, lpftStartDateTime);
}

HRESULT Appointment::SetEndDateTime(FILETIME ftEndDateTime, TimezoneDefinition *lpTZDef)
{
	HRESULT hr = hrSuccess;

	hr = OccurrenceDataHelper(m_ptrOccurrenceData).SetEndDateTime(ftEndDateTime, lpTZDef);
	if (hr != hrSuccess)
		goto exit;

	hr = UpdateTimeOffsets(m_ptrTZDefRecur);
	if (hr != hrSuccess)
		goto exit;

exit:
	return hr;
}

/**
 * Get the End datetime in the specified timezone or in UTC.
 *
 * @param[in]	lpTZDef		The TimezoneDefinion specifying in which timezone
 *							the End datetime should be returned. If NULL is
 *							passed, the End datetime will be returned in UTC.
 * 							Passing a timezone for an allday floating
 * 							appointment causes the datetime to be converted in
 * 							such a way that it occupies the whole day in the
 * 							destination timezone. So the absolute start time
 * 							in the space time continuum may differ for different
 * 							timezones.
 * @param[out]	lpftEndDateTime	The End datetime.
 * @return errorcode.
 */
HRESULT Appointment::GetEndDateTime(TimezoneDefinition *lpTZDef, FILETIME *lpftEndDateTime)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).GetEndDateTime(this, lpTZDef, lpftEndDateTime);
}

HRESULT Appointment::GetDuration(ULONG *lpulDuration)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).GetDuration(m_ptrClientTZ, lpulDuration);
}

HRESULT Appointment::SetBusyStatus(ULONG ulBusyStatus)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).SetBusyStatus(ulBusyStatus);
}

HRESULT Appointment::GetBusyStatus(ULONG *lpulBusyStatus)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).GetBusyStatus(lpulBusyStatus);
}

HRESULT Appointment::SetLocation(LPCTSTR lpstrLocation, ULONG ulFlags)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).SetLocation(lpstrLocation, ulFlags);
}

HRESULT Appointment::GetLocation(LPTSTR *lppstrLocation, LPVOID lpBase, ULONG ulFlags)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).GetLocation(lppstrLocation, lpBase, ulFlags);
}

HRESULT Appointment::SetReminderSet(unsigned short int fSet)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).SetReminderSet(fSet);
}

HRESULT Appointment::GetReminderSet(unsigned short int *lpfSet)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).GetReminderSet(lpfSet);
}

HRESULT Appointment::SetReminderDelta(ULONG ulDelta)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).SetReminderDelta(ulDelta);
}

HRESULT Appointment::GetReminderDelta(ULONG *lpulDelta)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).GetReminderDelta(lpulDelta);
}

HRESULT Appointment::SetSubject(LPCTSTR lpstrSubject, ULONG ulFlags)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).SetSubject(lpstrSubject, ulFlags);
}

HRESULT Appointment::GetSubject(LPTSTR *lppstrSubject, LPVOID lpBase, ULONG ulFlags)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).GetSubject(lppstrSubject, lpBase, ulFlags);
}

HRESULT Appointment::SetMeetingType(ULONG ulMeetingType)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).SetMeetingType(ulMeetingType);
}

HRESULT Appointment::GetMeetingType(ULONG *lpulMeetingType)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).GetMeetingType(lpulMeetingType);
}

HRESULT Appointment::SetSubType(unsigned short int fSubType)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).SetSubType(fSubType);
}

HRESULT Appointment::GetSubType(unsigned short int *lpfSubType)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).GetSubType(lpfSubType);
}

HRESULT Appointment::GetIsException(unsigned short int *lpfIsException)
{
	HRESULT hr = hrSuccess;

	if (lpfIsException == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	*lpfIsException = 0;

exit:
	return hr;
}

HRESULT Appointment::GetHasAttach(unsigned short int *lpfHasAttach)
{
	HRESULT hr = hrSuccess;

	if (lpfHasAttach == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (!m_ptrMessage)
		*lpfHasAttach = (m_bHasAttach ? 1 : 0);
	else {
		SPropValuePtr ptrPropVal;

		hr = HrGetOneProp(m_ptrMessage, PR_HASATTACH, &ptrPropVal);
		if (hr == hrSuccess)
			*lpfHasAttach = ptrPropVal->Value.b;
		else if (hr == MAPI_E_NOT_FOUND) {
			*lpfHasAttach = 0;
			hr = hrSuccess;
		} else
			goto exit;
	}

exit:
	return hr;
}

HRESULT Appointment::GetItemType(ULONG *lpulItemType)
{
	HRESULT hr = hrSuccess;

	if (lpulItemType == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	*lpulItemType = (m_ptrRecurrencePattern == NULL ? CALENDAR_ITEMTYPE_SINGLE : CALENDAR_ITEMTYPE_RECURRINGMASTER);

exit:
	return hr;
}

HRESULT Appointment::GetOriginalStartDateTime(TimezoneDefinition *lpTZDef, FILETIME *lpftStartDateTime)
{
	return GetStartDateTime(lpTZDef, lpftStartDateTime);
}

HRESULT Appointment::GetBaseDate(ULONG *lpulBaseDate)
{
	HRESULT hr = hrSuccess;
	FILETIME ftStartDateTime;

	hr = GetStartDateTime(m_ptrRecurrencePattern ? m_ptrTZDefRecur : m_ptrClientTZ, &ftStartDateTime);
	if (hr == hrSuccess)
		hr = BaseDateFromFileTime(ftStartDateTime, lpulBaseDate);
		
	return hr;
}

HRESULT Appointment::GetMapiMessage(ULONG ulMsgType, ULONG /*ulOverrideFlags*/, IMessage **lppMessage)
{
	HRESULT hr = hrSuccess;

	if (ulMsgType == CALENDAR_MSGTYPE_OCCURRENCE || ulMsgType == CALENDAR_MSGTYPE_COMPOSITE) {
		hr = MAPI_E_NO_SUPPORT;
		goto exit;
	}

	hr = m_ptrMessage->QueryInterface(IID_IMessage, (LPVOID*)lppMessage);
	if (hr != hrSuccess)
		goto exit;

exit:
	return hr;
}

/**
 * Open the the attachment and embedded message for the exception. This only
 * works for exceptions, regular occurrences will always return MAPI_E_NOT_FOUND.
 * If the occurrence was created based on table data this method will return
 * MAPI_E_NO_SUPPORT.
 *
 * The strategy to find the correct message is:
 * 1. Find the correct attachment by restricting on PidTagExceptionReplaceTime,
 *    which is the original start date and time of the ocurrence in UTC. This
 *    property is not always present though.
 * 2. Find the correct attachment by restricting on the PidTagExceptionStartTime,
 *    which is the new start time of the occurrence specified in local time. This
 *    property is not 100% reliable according to the documentation. So an additional
 *    check will be performed on the embedded message for the found attachment.
 *    This check compares the PidLidAppointmentStartWhole with the expected
 *    start date and time of the ocurrence in UTC.
 * 3. If attempt one and two fail, a brute force attempt is performed to find the
 *    the correct attachment by comparing the PidLidAppointmentStartWhole of all
 *    attachments.
 */
HRESULT Appointment::GetAttachmentAndMessage(Occurrence *lpOccurrence, IAttach **lppAttach, IMessage **lppMessage)
{
	HRESULT hr = hrSuccess;
	unsigned short int fIsException;
	MAPITablePtr ptrAttTable;
	SPropValue sPropValue;
	SRowSetPtr ptrRowSet;
	SPropValuePtr ptrPropValue;
	TimezoneDefinitionPtr ptrTZDef;
	FILETIME ftStartUTC;
	FILETIME ftOriginalStart;
	ULONG ulAttachNum;
	PropertySet props;
	ULONG ulBaseDate = 0;
	ULONG cbGoid = 0;
	BytePtr ptrGoid;

	AttachPtr ptrAttach;
	MessagePtr ptrMessage;

	SizedSPropTagArray(1, sptaAttTableProps) = {1, {PR_ATTACH_NUM}};

	if (lpOccurrence == NULL || lppAttach == NULL || lppMessage == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (m_ptrMessage == NULL) {
		hr = MAPI_E_NO_SUPPORT;
		goto exit;
	}

	hr = lpOccurrence->GetIsException(&fIsException);
	if (hr != hrSuccess)
		goto exit;

	if (fIsException == FALSE) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	hr = m_ptrMessage->GetAttachmentTable(0, &ptrAttTable);
	if (hr != hrSuccess)
		goto exit;

	hr = ptrAttTable->SetColumns((SPropTagArray*)&sptaAttTableProps, TBL_BATCH);
	if (hr != hrSuccess)
		goto exit;


	// Attempt 1
	hr = lpOccurrence->GetOriginalStartDateTime(m_ptrTZDefRecur, &sPropValue.Value.ft);
	if (hr != hrSuccess)
		goto exit;

	sPropValue.ulPropTag = PR_EXCEPTION_REPLACETIME;

	hr = ECPropertyRestriction(RELOP_EQ, sPropValue.ulPropTag, &sPropValue, ECRestriction::Cheap)
			.FindRowIn(ptrAttTable, BOOKMARK_BEGINNING, 0);
	if (hr == hrSuccess) {
		hr = ptrAttTable->QueryRows(1, 0, &ptrRowSet);
		if (hr != hrSuccess)
			goto exit;

		hr = m_ptrMessage->OpenAttach(ptrRowSet[0].lpProps[0].Value.ul, NULL, MAPI_MODIFY, &ptrAttach);
		if (hr != hrSuccess)
			goto exit;

		hr = ptrAttach->OpenProperty(PR_ATTACH_DATA_OBJ, &ptrMessage.iid, 0, MAPI_MODIFY, &ptrMessage);
		if (hr != hrSuccess)
			goto exit;

		goto done;
	}


	// Attempt 2
	hr = lpOccurrence->GetStartDateTime(m_ptrTZDefRecur, &sPropValue.Value.ft);
	if (hr != hrSuccess)
		goto exit;

	sPropValue.ulPropTag = PR_EXCEPTION_STARTTIME;

	hr = ECPropertyRestriction(RELOP_EQ, sPropValue.ulPropTag, &sPropValue, ECRestriction::Cheap)
			.FindRowIn(ptrAttTable, BOOKMARK_BEGINNING, 0);
	if (hr == hrSuccess) {
		hr = ptrAttTable->QueryRows(1, 0, &ptrRowSet);
		if (hr != hrSuccess)
			goto exit;

		hr = m_ptrMessage->OpenAttach(ptrRowSet[0].lpProps[0].Value.ul, NULL, MAPI_MODIFY, &ptrAttach);
		if (hr != hrSuccess)
			goto exit;

		hr = ptrAttach->OpenProperty(PR_ATTACH_DATA_OBJ, &ptrMessage.iid, 0, MAPI_MODIFY, &ptrMessage);
		if (hr != hrSuccess)
			goto exit;

		hr = HrGetOneProp(ptrMessage, PROP(APPTSTARTWHOLE), &ptrPropValue);
		if (hr == hrSuccess) {
			if (ptrTZDef) {
				hr = ptrTZDef->ToUTC(sPropValue.Value.ft, &ftStartUTC);
				if (hr != hrSuccess)
					goto exit;
			} else
				ftStartUTC = sPropValue.Value.ft;

			if (ptrPropValue->Value.ft == ftStartUTC)
				goto done;
		}
	}


	// Attempt 3
	if (m_ptrTZDefRecur) {
		hr = m_ptrTZDefRecur->ToUTC(sPropValue.Value.ft, &ftStartUTC);
		if (hr != hrSuccess)
			goto exit;
	} else
		ftStartUTC = sPropValue.Value.ft;

	hr = ptrAttTable->SeekRow(BOOKMARK_BEGINNING, 0, NULL);
	if (hr != hrSuccess)
		goto exit;

	while (true) {
		hr = ptrAttTable->QueryRows(1, 0, &ptrRowSet);
		if (hr != hrSuccess)
			goto exit;

		if (ptrRowSet.empty())
			break;

		hr = m_ptrMessage->OpenAttach(ptrRowSet[0].lpProps[0].Value.ul, NULL, MAPI_MODIFY, &ptrAttach);
		if (hr != hrSuccess)
			goto exit;

		hr = ptrAttach->OpenProperty(PR_ATTACH_DATA_OBJ, &ptrMessage.iid, 0, MAPI_MODIFY, &ptrMessage);
		if (hr == MAPI_E_INTERFACE_NOT_SUPPORTED)
			continue;
		if (hr != hrSuccess)
			goto exit;

		hr = HrGetOneProp(ptrMessage, PROP(APPTSTARTWHOLE), &ptrPropValue);
		if (hr == hrSuccess && ptrPropValue->Value.ft == ftStartUTC)
			goto done;
	}


	// Not found, create them
	hr = m_ptrMessage->CreateAttach(&ptrAttach.iid, 0, &ulAttachNum, &ptrAttach);
	if (hr != hrSuccess)
		goto exit;

	hr = lpOccurrence->GetOriginalStartDateTime(m_ptrTZDefRecur, &ftOriginalStart);
	if (hr != hrSuccess)
		goto exit;

	props[PR_EXCEPTION_REPLACETIME].ft = ftOriginalStart;
	props[PR_DISPLAY_NAME].LPSZ = _T("Untitled");
	props[PR_RENDERING_POSITION].l = -1;
	props[PR_ATTACH_METHOD].ul = ATTACH_EMBEDDED_MSG;
	props[PR_ATTACHMENT_HIDDEN].b = TRUE;
	props[PR_ATTACHMENT_FLAGS].ul = 2;
	props[PR_ATTACHMENT_CONTACTPHOTO].b = FALSE;
	props[PR_ATTACHMENT_LINKID].ul = 0;

	hr = props.SetPropsOn(ptrAttach);
	if (FAILED(hr))
		goto exit;

	hr = ptrAttach->OpenProperty(PR_ATTACH_DATA_OBJ, &ptrMessage.iid, 0, MAPI_CREATE | MAPI_MODIFY, &ptrMessage);
	if (hr != hrSuccess)
		goto exit;

	props.clear();
	props[PROP(RECURRINGBASE)].ft = ftOriginalStart;
	props[PR_MESSAGE_CLASS].LPSZ = _T("IPM.OLE.CLASS.{00061055-0000-0000-C000-000000000046}");

	hr = HrGetOneProp(m_ptrMessage, PROP(CLEANID), &ptrPropValue);
	if (hr == MAPI_E_NOT_FOUND)
		hr = HrGetOneProp(m_ptrMessage, PROP(GOID), &ptrPropValue);
	if (hr != hrSuccess)
		goto exit;

	hr = lpOccurrence->GetBaseDate(&ulBaseDate);
	if (hr != hrSuccess)
		goto exit;

	props[PROP(CLEANID)].bin = ptrPropValue->Value.bin;

	hr = CreateGOID(ptrPropValue->Value.bin.cb,
	                ptrPropValue->Value.bin.lpb,
	                ulBaseDate, &cbGoid, &ptrGoid);

	if (hr == hrSuccess) {
		props[PROP(GOID)].bin.cb = cbGoid;
		props[PROP(GOID)].bin.lpb = ptrGoid;
	} else if (hr != MAPI_E_INVALID_PARAMETER && hr != MAPI_E_NO_SUPPORT)
		goto exit;

	hr = props.SetPropsOn(ptrMessage);
	if (FAILED(hr))
		goto exit;

done:
	*lppAttach = ptrAttach.release();
	*lppMessage = ptrMessage.release();

exit:
	return hr;
}

/**
 * Get a reference to the property pool used by the Appointment instance.
 *
 * @param[out]	lppPropertyPool		A pointer to the property pool. This
 *									must be released by the caller.
 */
HRESULT Appointment::GetPropertyPool(PropertyPool **lppPropertyPool)
{
	HRESULT hr = hrSuccess;

	if (lppPropertyPool == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	*lppPropertyPool = m_ptrPropertyPool;
	(*lppPropertyPool)->AddRef();

exit:
	return hr;
}

HRESULT Appointment::GetExceptionData(ULONG ulBaseDate, Exception *lpException)
{
	HRESULT hr = hrSuccess;
	bool bIsOccurrence;
	ExceptionMap::iterator iEntry;

	if (lpException == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (!m_ptrRecurrencePattern) {
		hr = MAPI_E_NO_SUPPORT;
		goto exit;
	}

	hr = m_ptrRecurrencePattern->IsOccurrence(ulBaseDate, &bIsOccurrence);
	if (hr != hrSuccess)
		goto exit;

	if (!bIsOccurrence) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	iEntry = m_mapModifyExceptions.find(ulBaseDate);
	if (iEntry == m_mapModifyExceptions.end()) {
		lpException->basic.ulStartDateTime = 0;
		lpException->basic.ulEndDateTime = 0;
		lpException->basic.ulOriginalStartDate = 0;
		lpException->basic.ulOverrideFlags = 0;

		lpException->extended.ulStartDateTime = 0;
		lpException->extended.ulEndDateTime = 0;
		lpException->extended.ulOriginalStartDate = 0;
	} else {
		*lpException = iEntry->second;
	}

exit:
	return hr;
}

HRESULT Appointment::SetExceptionData(ULONG ulBaseDate, const Exception &exception)
{
	HRESULT hr = hrSuccess;
	bool bIsOccurrence;
	std::pair<ExceptionMap::iterator, bool> insert_result;

	if (!m_ptrRecurrencePattern) {
		hr = MAPI_E_NO_SUPPORT;
		goto exit;
	}

	hr = m_ptrRecurrencePattern->IsOccurrence(ulBaseDate, &bIsOccurrence);
	if (hr != hrSuccess)
		goto exit;

	if (!bIsOccurrence) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	insert_result = m_mapModifyExceptions.insert(ExceptionMap::value_type(ulBaseDate, Exception()));
	if (insert_result.second == true) {
		// New insert, so check if it's not deleted
		
		if (m_setDeletedExceptions.insert(ulBaseDate).second == false) {
			m_mapModifyExceptions.erase(insert_result.first);
			hr = MAPI_E_NOT_FOUND;	// Can't make a modified exception out of
			goto exit;            	// a deleted exception.
		}
	}

	insert_result.first->second = exception;

exit:
	return hr;
}

HRESULT Appointment::GetStartTimezone(TimezoneDefinition **lppTZDef)
{
	HRESULT hr = hrSuccess;

	if (lppTZDef == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (!m_ptrTZStartTime) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	*lppTZDef = m_ptrTZStartTime;
	(*lppTZDef)->AddRef();

exit:
	return hr;
}




DEF_ULONGMETHOD(TRACE_MAPI, Appointment, Appointment, AddRef, (void))
DEF_ULONGMETHOD(TRACE_MAPI, Appointment, Appointment, Release, (void))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, QueryInterface, (REFIID, refiid), (void **, lppInterface))

DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, SetRecurrence, (RecurrencePattern *, lpPattern))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, SetRecurrenceTimezone, (TimezoneDefinition *, lpTZDef))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetRecurrence, (RecurrencePattern **, lppPattern))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetRecurrenceTimezone, (TimezoneDefinition **, lppTZDef))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetBounds, (ULONG *, lpulFirst), (ULONG *, lpulLast))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetOccurrence, (ULONG, ulBaseDate), (IOccurrence **, lppOccurrence))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, RemoveOccurrence, (ULONG, ulBaseDate))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, ResetOccurrence, (ULONG, ulBaseDate))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetExceptions, (ULONG *, lpcbModifyExceptions), (ULONG **, lpaModifyExceptions), (ULONG *, lpcbDeleteExceptions), (ULONG **, lpaDeleteExceptions))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetBaseDateForOccurrence, (ULONG, ulIndex), (ULONG *, lpulBaseDate))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, SetStartDateTime, (FILETIME, ftStartDateTime), (TimezoneDefinition *, lpTZDef))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetStartDateTime, (TimezoneDefinition *, lpTZDef), (FILETIME *, lpftStartDateTime))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, SetEndDateTime, (FILETIME, ftEndDateTime), (TimezoneDefinition *, lpTZDef))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetEndDateTime, (TimezoneDefinition *, lpTZDef), (FILETIME *, lpftEndDateTime))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetDuration, (ULONG *, lpulDuration))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, SetBusyStatus, (ULONG, ulBusyStatus))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetBusyStatus, (ULONG *, lpulBusyStatus))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, SetLocation, (LPCTSTR, lpstrLocation), (ULONG, ulFlags))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetLocation, (LPTSTR *, lppstrLocation), (LPVOID, lpBase), (ULONG, ulFlags))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, SetReminderSet, (unsigned short int, fSet))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetReminderSet, (unsigned short int *, lpfSet))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, SetReminderDelta, (ULONG, ulDelta))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetReminderDelta, (ULONG *, lpulDelta))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, SetSubject, (LPCTSTR, lpstrSubject), (ULONG, ulFlags))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetSubject, (LPTSTR *, lppstrSubject), (LPVOID, lpBase), (ULONG, ulFlags))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, SetMeetingType, (ULONG, ulMeetingType))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetMeetingType, (ULONG *, lpulMeetingType))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, SetSubType, (unsigned short int, fSubType))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetSubType, (unsigned short int *, lpfSubType))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetIsException, (unsigned short int *, lpfIsException))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetHasAttach, (unsigned short int *, lpfHasAttach))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetItemType, (ULONG *, lpulItemType))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetOriginalStartDateTime, (TimezoneDefinition *, lpTZDef), (FILETIME *, lpftStartDateTime))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetBaseDate, (ULONG *, lpulBaseDate))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, GetMapiMessage, (ULONG, ulMsgType), (ULONG, ulOverrideFlags), (IMessage **, lppMessage))
DEF_HRMETHOD(TRACE_MAPI, Appointment, Appointment, ApplyChanges, (VOID))
