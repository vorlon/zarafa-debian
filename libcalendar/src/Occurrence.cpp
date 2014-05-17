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
#include "Occurrence.h"
#include "OccurrenceData.h"
#include "Conversions.h"
#include "CompositeMessage.h"

#include "Trace.h"
#include "ECInterfaceDefs.h"
#include "charset/convert.h"
#include "ECRestriction.h"
#include "CommonUtil.h"

#include <mapiext.h>

typedef mapi_object_ptr<Occurrence> OccurrencePtr;

#define PROP(_name) (ptrPropertyPool->GetPropTag(PropertyPool::PROP_##_name))

/**
 * Convert a date, timeoffset and timezone definition to a UTC timestamp.
 * 
 * @param[in]	ulBaseDate			The date time, specified in rtime.
 * @param[in]	lpTZDef				Optional timezone definition.
 * @param[out]	lpftTimestampUTC	The filetime structure containing the
 * 									resulting timestamp, represented in UTC.
 */
static HRESULT CalcTimestampUTC(ULONG ulBaseDate, TimezoneDefinition *lpTZDef, FILETIME *lpftTimestampUTC)
{
	HRESULT hr = hrSuccess;
	FILETIME ftTimestamp;

	hr = BaseDateToFileTime(ulBaseDate, &ftTimestamp);
	if (hr != hrSuccess)
		goto exit;

	if (lpTZDef != NULL)
		hr = lpTZDef->ToUTC(ftTimestamp, lpftTimestampUTC);
	else
		*lpftTimestampUTC = ftTimestamp;

exit:
	return hr;
}

/**
 * Create an Occurrence instance.
 * 
 * @param[in]	ulBaseDate			The original date of the occurrence.
 * @param[in]	ulStartTimeOffset	The amount of minutes after ulBaseDate at which
 * 									the occurrence starts.
 * @param[in]	ulEndTimeOffset		The amount of minutes after ulBaseDate at which
 * 									the occurrence ends.
 * @param[in]	lpTZDefRecur		The timezone in which the recurrence pattern is
 * 									represented.
 * @param[in]	lpOccurrenceDate	The occurrence data of the master appointment on
 * 									which the data of the new occurrence will be based.
 * @param[in]	lpException			Optional exception instance that describes the
 * 									exception.
 * @param[in]	lpExtendedExcetpion	Optional extended exception instance that describes
 * 									the exception.
 * @param[out]	lppOccurrence		The new Occurrence instance. Must be deleted by the
 * 									caller.
 */
HRESULT Occurrence::Create(ULONG ulBaseDate, ULONG ulStartTimeOffset, ULONG ulEndTimeOffset, TimezoneDefinition *lpTZDefRecur,
                           const OccurrenceData *lpOccurrenceData, const Appointment::Exception *lpException, Appointment *lpAppointment,
                           IOccurrence **lppOccurrence)
{
	HRESULT hr = hrSuccess;
	OccurrencePtr ptrOccurrence;
	FILETIME ftStartUTC;
	FILETIME ftEndUTC;

	hr = CalcTimestampUTC(ulBaseDate + ulStartTimeOffset, lpTZDefRecur, &ftStartUTC);
	if (hr != hrSuccess)
		goto exit;

	hr = CalcTimestampUTC(ulBaseDate + ulEndTimeOffset, lpTZDefRecur, &ftEndUTC);
	if (hr != hrSuccess)
		goto exit;

	try {
		ptrOccurrence.reset(new Occurrence(ulBaseDate, lpOccurrenceData, ftStartUTC, ftEndUTC, lpAppointment));
	} catch (const std::bad_alloc &) {
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}

	// The occurrence data is now initialized with the default data so that all changes
	// that are made by processing the exception data, will show up as change so it will
	// be used as exception data when saving the exception.
	if (lpException) {
		ptrOccurrence->m_bIsException = true;
		hr = CalcTimestampUTC(lpException->basic.ulOriginalStartDate, lpTZDefRecur, &ptrOccurrence->m_ftOriginalStartDateTime);
		if (hr != hrSuccess)
			goto exit;


		// Update start and end dates
		hr = CalcTimestampUTC(lpException->basic.ulStartDateTime, lpTZDefRecur, &ftStartUTC);
		if (hr != hrSuccess)
			goto exit;
		ptrOccurrence->m_ptrOccurrenceData->ftStart.set(ftStartUTC);

		hr = CalcTimestampUTC(lpException->basic.ulEndDateTime, lpTZDefRecur, &ftEndUTC);
		if (hr != hrSuccess)
			goto exit;
		ptrOccurrence->m_ptrOccurrenceData->ftEnd.set(ftEndUTC);


		// Update optional properties
		if (lpException->basic.ulOverrideFlags & ARO_SUBJECT)
			ptrOccurrence->m_ptrOccurrenceData->strSubject.set(lpException->extended.strWideCharSubject);

		if (lpException->basic.ulOverrideFlags & ARO_MEETINGTYPE)
			ptrOccurrence->m_ptrOccurrenceData->ulMeetingType.set(lpException->basic.ulMeetingType);

		if (lpException->basic.ulOverrideFlags & ARO_REMINDERDELTA)
			ptrOccurrence->m_ptrOccurrenceData->ulReminderDelta.set(lpException->basic.ulReminderDelta);

		if (lpException->basic.ulOverrideFlags & ARO_REMINDERSET)
			ptrOccurrence->m_ptrOccurrenceData->fReminderSet.set(lpException->basic.ulReminderSet ? TRUE : FALSE);
			
		if (lpException->basic.ulOverrideFlags & ARO_LOCATION)
			ptrOccurrence->m_ptrOccurrenceData->strLocation.set(lpException->extended.strWideCharLocation);

		if (lpException->basic.ulOverrideFlags & ARO_BUSYSTATUS)
			ptrOccurrence->m_ptrOccurrenceData->ulBusyStatus.set(lpException->basic.ulBusyStatus);

		if (lpException->basic.ulOverrideFlags & ARO_ATTACHMENT)
			ptrOccurrence->m_bOverrideAttachments = true;

		if (lpException->basic.ulOverrideFlags & ARO_SUBTYPE)
			ptrOccurrence->m_ptrOccurrenceData->fSubType.set(lpException->basic.ulSubType ? TRUE : FALSE);

		if (lpException->basic.ulOverrideFlags & ARO_APPTCOLOR) {
			// This field is reserved and MUST NOT be read from or written to.
		}

		if (lpException->basic.ulOverrideFlags & ARO_EXCEPTIONAL_BODY) {
			// The Exception Embedded Message object has the PidTagRtfCompressed property...
		}
	}

	hr = ptrOccurrence->QueryInterface(IID_IOccurrence, (LPVOID*)lppOccurrence);

exit:
	return hr;
}

/**
 * Constructor
 * 
 * @param[in]	ulBaseDate			The original start date of the occurrence.
 * @param[in]	lpOccurrenceData	The occurrence data that describes this
 * 									occurrence.
 * @param[in]	ftStartUTC			The start of the occurrence in UTC.
 * @param[in]	ftEndUTC			The end of the occurrence in UTC.
 */
Occurrence::Occurrence(ULONG ulBaseDate, const OccurrenceData *lpOccurrenceData, FILETIME ftStartUTC, FILETIME ftEndUTC, Appointment *lpAppointment)
: m_ulBaseDate(ulBaseDate)
, m_bIsException(false)
, m_bOverrideAttachments(false)
, m_ftOriginalStartDateTime(ftStartUTC)
, m_ptrOccurrenceData(lpOccurrenceData->Clone(ftStartUTC, ftEndUTC))
, m_ptrAppointment(lpAppointment)
{ }

Occurrence::~Occurrence()
{ }


/**
 * Load the the attachment and embedded message for the exception. This only
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
HRESULT Occurrence::LoadExceptionMessage()
{
	HRESULT hr = hrSuccess;

	if (!m_ptrExceptMsg)
		hr = m_ptrAppointment->GetAttachmentAndMessage(this, &m_ptrExceptAttach, &m_ptrExceptMsg);

	return hr;
}


HRESULT	Occurrence::QueryInterface(REFIID refiid, void **lppInterface)
{
	REGISTER_INTERFACE(IID_ECUnknown, this);

	REGISTER_INTERFACE(IID_IOccurrence, &this->m_xOccurrence);
	REGISTER_INTERFACE(IID_IUnknown, &this->m_xOccurrence);

	return MAPI_E_INTERFACE_NOT_SUPPORTED;
}


HRESULT Occurrence::GetStartDateTime(TimezoneDefinition *lpTZDef, FILETIME *lpftStartDateTime)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).GetStartDateTime(m_ptrAppointment, lpTZDef, lpftStartDateTime);
}

HRESULT Occurrence::SetStartDateTime(FILETIME ftStartDateTime, TimezoneDefinition *lpTZDef)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).SetStartDateTime(ftStartDateTime, lpTZDef);
}

HRESULT Occurrence::SetEndDateTime(FILETIME ftEndDateTime, TimezoneDefinition *lpTZDef)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).SetEndDateTime(ftEndDateTime, lpTZDef);
}

HRESULT Occurrence::GetEndDateTime(TimezoneDefinition *lpTZDef, FILETIME *lpftEndDateTime)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).GetEndDateTime(m_ptrAppointment, lpTZDef, lpftEndDateTime);
}

HRESULT Occurrence::GetDuration(ULONG *lpulDuration)
{
	HRESULT hr = hrSuccess;
	TimezoneDefinitionPtr ptrTZRecur;

	hr = m_ptrAppointment->GetRecurrenceTimezone(&ptrTZRecur);
	if (hr != hrSuccess)
		goto exit;

	hr = OccurrenceDataHelper(m_ptrOccurrenceData).GetDuration(ptrTZRecur, lpulDuration);
	if (hr != hrSuccess)
		goto exit;

exit:
	return hr;
}

HRESULT Occurrence::SetBusyStatus(ULONG ulBusyStatus)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).SetBusyStatus(ulBusyStatus);
}

HRESULT Occurrence::GetBusyStatus(ULONG *lpulBusyStatus)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).GetBusyStatus(lpulBusyStatus);
}

HRESULT Occurrence::SetLocation(LPCTSTR lpstrLocation, ULONG ulFlags)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).SetLocation(lpstrLocation, ulFlags);
}

HRESULT Occurrence::GetLocation(LPTSTR *lppstrLocation, LPVOID lpBase, ULONG ulFlags)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).GetLocation(lppstrLocation, lpBase, ulFlags);
}

HRESULT Occurrence::SetReminderSet(unsigned short int fSet)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).SetReminderSet(fSet);
}

HRESULT Occurrence::GetReminderSet(unsigned short int *lpfSet)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).GetReminderSet(lpfSet);
}

HRESULT Occurrence::SetReminderDelta(ULONG ulDelta)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).SetReminderDelta(ulDelta);
}

HRESULT Occurrence::GetReminderDelta(ULONG *lpulDelta)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).GetReminderDelta(lpulDelta);
}

HRESULT Occurrence::SetSubject(LPCTSTR lpstrSubject, ULONG ulFlags)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).SetSubject(lpstrSubject, ulFlags);
}

HRESULT Occurrence::GetSubject(LPTSTR *lppstrSubject, LPVOID lpBase, ULONG ulFlags)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).GetSubject(lppstrSubject, lpBase, ulFlags);
}

HRESULT Occurrence::SetMeetingType(ULONG ulMeetingType)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).SetMeetingType(ulMeetingType);
}

HRESULT Occurrence::GetMeetingType(ULONG *lpulMeetingType)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).GetMeetingType(lpulMeetingType);
}

HRESULT Occurrence::SetSubType(unsigned short int fSubType)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).SetSubType(fSubType);
}

HRESULT Occurrence::GetSubType(unsigned short int *lpfSubType)
{
	return OccurrenceDataHelper(m_ptrOccurrenceData).GetSubType(lpfSubType);
}

HRESULT Occurrence::GetIsException(unsigned short int *lpfIsException)
{
	HRESULT hr = hrSuccess;

	if (lpfIsException == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	*lpfIsException = (m_bIsException ? 1 : 0);

exit:
	return hr;
}

HRESULT Occurrence::GetHasAttach(unsigned short int *lpfHasAttach)
{
	HRESULT hr = hrSuccess;

	if (lpfHasAttach == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (m_bOverrideAttachments) {
		SPropValuePtr ptrPropVal;

		if (!m_ptrExceptMsg) {
			hr = LoadExceptionMessage();
			if (hr != hrSuccess)
				goto exit;
		}

		hr = HrGetOneProp(m_ptrExceptMsg, PR_HASATTACH, &ptrPropVal);
		if (hr == hrSuccess)
			*lpfHasAttach = ptrPropVal->Value.b;
		else if (hr == MAPI_E_NOT_FOUND) {
			*lpfHasAttach = 0;
			hr = hrSuccess;
		} else
			goto exit;
	} else
		hr = m_ptrAppointment->GetHasAttach(lpfHasAttach);

exit:
	return hr;
}

HRESULT Occurrence::GetItemType(ULONG *lpulItemType)
{
	HRESULT hr = hrSuccess;

	if (lpulItemType == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	*lpulItemType = (m_bIsException ? CALENDAR_ITEMTYPE_EXCEPTION : CALENDAR_ITEMTYPE_OCCURRENCE);

exit:
	return hr;
}

HRESULT Occurrence::GetOriginalStartDateTime(TimezoneDefinition *lpTZDef, FILETIME *lpftStartDateTime)
{
	HRESULT hr = hrSuccess;

	if (lpftStartDateTime == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (lpTZDef) {
		hr = lpTZDef->FromUTC(m_ftOriginalStartDateTime, lpftStartDateTime);
		if (hr != hrSuccess)
			goto exit;
	} else
		*lpftStartDateTime = m_ftOriginalStartDateTime;

exit:
	return hr;
}

HRESULT Occurrence::GetBaseDate(ULONG *lpulBaseDate)
{
	if (lpulBaseDate == NULL)
		return MAPI_E_INVALID_PARAMETER;

	*lpulBaseDate = m_ulBaseDate;
	return hrSuccess;
}

HRESULT Occurrence::GetMapiMessage(ULONG ulMsgType, ULONG ulOverrideFlags, IMessage **lppMessage)
{
	HRESULT hr = hrSuccess;
	MessagePtr ptrMasterMessage;

	if (lppMessage == NULL ||
	    ulMsgType > _CALENDAR_MSGTYPE_MAX ||
	    (ulOverrideFlags & ~_CALENDAR_ENSURE_OVERRIDE_MASK) != 0)
	{
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (m_bIsException || ulOverrideFlags != 0) {
		/*
		 * The occurrence already is an exception, or becomes one because of
		 * the override flag. Setting m_bIsException to true causes
		 * Appointment::GetAttachmentAndMessage to not fail instantly.
		 */
		m_bIsException = true;

		hr = LoadExceptionMessage();
		if (hr != hrSuccess)
			goto exit;
	}

	hr = m_ptrAppointment->GetMapiMessage(CALENDAR_MSGTYPE_MASTER, 0, &ptrMasterMessage);
	if (hr != hrSuccess)
		goto exit;

	if ((ulOverrideFlags & CALENDAR_ENSURE_OVERRIDE_ATTACHMENTS) &&
	    m_bOverrideAttachments == false)
	{
		SRestrictionPtr ptrRestriction;

		hr = ECOrRestriction(
				ECNotRestriction(
					ECExistRestriction(PR_ATTACH_FLAGS)
				) +
				ECBitMaskRestriction(BMR_EQZ, PR_ATTACH_FLAGS, 2)
			).CreateMAPIRestriction(&ptrRestriction, ECRestriction::Cheap);
		if (hr != hrSuccess)
			goto exit;

		hr = Util::CopyAttachments(ptrMasterMessage, m_ptrExceptMsg, ptrRestriction);
		if (hr != hrSuccess)
			goto exit;

		m_bOverrideAttachments = true;
	}

	switch (ulMsgType) {
	case CALENDAR_MSGTYPE_MASTER:
		hr = ptrMasterMessage->QueryInterface(IID_IMessage, (void**)lppMessage);
		break;

	case CALENDAR_MSGTYPE_OCCURRENCE:
		if (!m_ptrExceptMsg) {
			hr = MAPI_E_NOT_FOUND;
			goto exit;
		}
		hr = m_ptrExceptMsg->QueryInterface(IID_IMessage, (void**)lppMessage);
		break;

	case CALENDAR_MSGTYPE_DEFAULT:
	case CALENDAR_MSGTYPE_COMPOSITE:
		if (!m_ptrExceptMsg) {
			hr = MAPI_E_NOT_FOUND;
			goto exit;
		}
		hr = CompositeMessage::Create(m_ptrExceptMsg, ptrMasterMessage, (m_bOverrideAttachments ? CompositeMessage::cmofAttachments : 0), lppMessage);
		break;

	default:
		ASSERT(FALSE);
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}
	if (hr != hrSuccess)
		goto exit;

exit:
	return hr;
}

HRESULT Occurrence::ApplyChanges()
{
	HRESULT hr = hrSuccess;
	bool bModified = false;
	PropertyPoolPtr ptrPropertyPool;
	TimezoneDefinitionPtr ptrTZDefRecur;
	Appointment::Exception exception;
	FILETIME ft;

	SPropValue aAttProps[2];
	SPropValue aMsgProps[13];
	ULONG cMsgProps = 0;

	hr = m_ptrAppointment->GetPropertyPool(&ptrPropertyPool);
	if (hr != hrSuccess)
		goto exit;

	hr = m_ptrAppointment->GetExceptionData(m_ulBaseDate, &exception);
	if (hr != hrSuccess)
		goto exit;

	hr = m_ptrAppointment->GetRecurrenceTimezone(&ptrTZDefRecur);
	if (hr != hrSuccess)
		goto exit;

	// Set the attachment properties
	hr = GetStartDateTime(ptrTZDefRecur, &aAttProps[0].Value.ft);
	if (hr != hrSuccess)
		goto exit;
	aAttProps[0].ulPropTag = PR_EXCEPTION_STARTTIME;

	hr = GetEndDateTime(ptrTZDefRecur, &aAttProps[1].Value.ft);
	if (hr != hrSuccess)
		goto exit;
	aAttProps[1].ulPropTag = PR_EXCEPTION_ENDTIME;


	// Always set the start-, end- and original datetime.
	if (ptrTZDefRecur) {
		hr = ptrTZDefRecur->FromUTC(m_ptrOccurrenceData->ftStart, &ft);
		if (hr != hrSuccess)
			goto exit;
		FileTimeToRTime(&ft, (LONG*)&exception.basic.ulStartDateTime);

		hr = ptrTZDefRecur->FromUTC(m_ptrOccurrenceData->ftEnd, &ft);
		if (hr != hrSuccess)
			goto exit;
		FileTimeToRTime(&ft, (LONG*)&exception.basic.ulEndDateTime);

		hr = ptrTZDefRecur->FromUTC(m_ftOriginalStartDateTime, &ft);
		if (hr != hrSuccess)
			goto exit;
		FileTimeToRTime(&ft, (LONG*)&exception.basic.ulOriginalStartDate);
	} else {
		ft = m_ptrOccurrenceData->ftStart;
		FileTimeToRTime(&ft, (LONG*)&exception.basic.ulStartDateTime);

		ft = m_ptrOccurrenceData->ftEnd;
		FileTimeToRTime(&ft, (LONG*)&exception.basic.ulEndDateTime);

		FileTimeToRTime(&m_ftOriginalStartDateTime, (LONG*)&exception.basic.ulOriginalStartDate);
	}

	exception.extended.ulStartDateTime = exception.basic.ulStartDateTime;
	exception.extended.ulEndDateTime = exception.basic.ulEndDateTime;
	exception.extended.ulOriginalStartDate = exception.basic.ulOriginalStartDate;

	if (m_ptrOccurrenceData->ftStart.dirty()) {
		ULONG ulTags[] = {
			PROP(APPTSTARTWHOLE),
			PROP(COMMONSTART),
			PR_START_DATE
		};

		for (size_t i = 0; i < arraySize(ulTags); ++i) {
			aMsgProps[cMsgProps].ulPropTag = ulTags[i];
			aMsgProps[cMsgProps++].Value.ft = m_ptrOccurrenceData->ftStart;
		}

		bModified = true;
	}

	if (m_ptrOccurrenceData->ftEnd.dirty()) {
		ULONG ulTags[] = {
			PROP(APPTENDWHOLE),
			PROP(COMMONEND),
			PR_END_DATE
		};

		for (size_t i = 0; i < arraySize(ulTags); ++i) {
			aMsgProps[cMsgProps].ulPropTag = ulTags[i];
			aMsgProps[cMsgProps++].Value.ft = m_ptrOccurrenceData->ftEnd;
		}

		bModified = true;
	}

	if (m_ptrOccurrenceData->ulBusyStatus.dirty()) {
		aMsgProps[cMsgProps].ulPropTag = PROP(BUSYSTATUS);
		aMsgProps[cMsgProps++].Value.ul = m_ptrOccurrenceData->ulBusyStatus;

		exception.basic.ulBusyStatus = m_ptrOccurrenceData->ulBusyStatus;
		exception.basic.ulOverrideFlags |= ARO_BUSYSTATUS;

		bModified = true;
	}

	if (m_ptrOccurrenceData->strLocation.dirty()) {
		const std::wstring &strLocation = m_ptrOccurrenceData->strLocation;
		aMsgProps[cMsgProps].ulPropTag = PROP(LOCATION);
		aMsgProps[cMsgProps++].Value.lpszW = (LPWSTR)strLocation.c_str();

		exception.basic.strLocation = convert_to<std::string>("WINDOWS-1252", strLocation, rawsize(strLocation), CHARSET_WCHAR);
		exception.extended.strWideCharLocation = strLocation;
		exception.basic.ulOverrideFlags |= ARO_LOCATION;

		bModified = true;
	}

	if (m_ptrOccurrenceData->ulReminderDelta.dirty()) {
		aMsgProps[cMsgProps].ulPropTag = PROP(REMINDERMINUTESBEFORESTART);
		aMsgProps[cMsgProps++].Value.ul = m_ptrOccurrenceData->ulReminderDelta;

		exception.basic.ulReminderDelta = m_ptrOccurrenceData->ulReminderDelta;
		exception.basic.ulOverrideFlags |= ARO_REMINDERDELTA;

		bModified = true;
	}

	if (m_ptrOccurrenceData->strSubject.dirty()) {
		const std::wstring &strSubject = m_ptrOccurrenceData->strSubject;
		aMsgProps[cMsgProps].ulPropTag = PR_SUBJECT;
		aMsgProps[cMsgProps++].Value.lpszW = (LPWSTR)strSubject.c_str();

		exception.basic.strSubject = convert_to<std::string>("WINDOWS-1252", strSubject, rawsize(strSubject), CHARSET_WCHAR);
		exception.extended.strWideCharSubject = strSubject;
		exception.basic.ulOverrideFlags |= ARO_SUBJECT;

		bModified = true;
	}

	if (m_ptrOccurrenceData->fReminderSet.dirty()) {
		aMsgProps[cMsgProps].ulPropTag = PROP(REMINDERSET);
		aMsgProps[cMsgProps++].Value.b = m_ptrOccurrenceData->fReminderSet;

		exception.basic.ulReminderSet = m_ptrOccurrenceData->fReminderSet;
		exception.basic.ulOverrideFlags |= ARO_REMINDERSET;

		bModified = true;
	}

	if (m_ptrOccurrenceData->ulMeetingType.dirty()) {
		aMsgProps[cMsgProps].ulPropTag = PROP(MEETINGSTATUS);
		aMsgProps[cMsgProps++].Value.ul = m_ptrOccurrenceData->ulMeetingType;

		exception.basic.ulMeetingType = m_ptrOccurrenceData->ulMeetingType;
		exception.basic.ulOverrideFlags |= ARO_MEETINGTYPE;

		bModified = true;
	}

	if (m_ptrOccurrenceData->fSubType.dirty()) {
		aMsgProps[cMsgProps].ulPropTag = PROP(ALLDAYEVENT);
		aMsgProps[cMsgProps++].Value.b = m_ptrOccurrenceData->fSubType;

		exception.basic.ulSubType = m_ptrOccurrenceData->fSubType;
		exception.basic.ulOverrideFlags |= ARO_SUBTYPE;

		bModified = true;
	}

	if (m_bOverrideAttachments) {
		unsigned short int fHasAttach;

		hr = GetHasAttach(&fHasAttach);
		if (hr != hrSuccess)
			goto exit;

		exception.basic.ulOverrideFlags |= ARO_ATTACHMENT;
		exception.basic.ulAttachment = fHasAttach;
	} else
		exception.basic.ulOverrideFlags &= ~ARO_ATTACHMENT;

	// TODO: Research what to do with ARO_APPTCOLOR and ARO_EXCEPTIONAL_BODY.

	if (bModified == true || m_bOverrideAttachments) {
		m_bIsException = true;

		hr = LoadExceptionMessage();
		if (hr != hrSuccess)
			goto exit;

		if (cMsgProps > 0) {
			hr = m_ptrExceptMsg->SetProps(cMsgProps, aMsgProps, NULL);
			if (FAILED(hr))
				goto exit;

			hr = m_ptrExceptMsg->SaveChanges(KEEP_OPEN_READWRITE);
			if (hr != hrSuccess)
				goto exit;
		}

		hr = m_ptrExceptAttach->SetProps(2, aAttProps, NULL);
		if (FAILED(hr))
			goto exit;

		hr = m_ptrExceptAttach->SaveChanges(KEEP_OPEN_READWRITE);
		if (hr != hrSuccess)
			goto exit;

		hr = m_ptrAppointment->SetExceptionData(m_ulBaseDate, exception);
		if (hr != hrSuccess)
			goto exit;

		m_ptrOccurrenceData->MarkClean();
	}

exit:
	return hr;
}



DEF_ULONGMETHOD(TRACE_MAPI, Occurrence, Occurrence, AddRef, (void))
DEF_ULONGMETHOD(TRACE_MAPI, Occurrence, Occurrence, Release, (void))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, QueryInterface, (REFIID, refiid), (void **, lppInterface))

DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, SetStartDateTime, (FILETIME, ftStartDateTime), (TimezoneDefinition *, lpTZDef))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, GetStartDateTime, (TimezoneDefinition *, lpTZDef), (FILETIME *, lpftStartDateTime))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, SetEndDateTime, (FILETIME, ftEndDateTime), (TimezoneDefinition *, lpTZDef))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, GetEndDateTime, (TimezoneDefinition *, lpTZDef), (FILETIME *, lpftEndDateTime))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, GetDuration, (ULONG *, lpulDuration))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, SetBusyStatus, (ULONG, ulBusyStatus))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, GetBusyStatus, (ULONG *, lpulBusyStatus))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, SetLocation, (LPCTSTR, lpstrLocation), (ULONG, ulFlags))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, GetLocation, (LPTSTR *, lppstrLocation), (LPVOID, lpBase), (ULONG, ulFlags))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, SetReminderSet, (unsigned short int, fSet))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, GetReminderSet, (unsigned short int *, lpfSet))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, SetReminderDelta, (ULONG, ulDelta))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, GetReminderDelta, (ULONG *, lpulDelta))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, SetSubject, (LPCTSTR, lpstrSubject), (ULONG, ulFlags))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, GetSubject, (LPTSTR *, lppstrSubject), (LPVOID, lpBase), (ULONG, ulFlags))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, SetMeetingType, (ULONG, ulMeetingType))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, GetMeetingType, (ULONG *, lpulMeetingType))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, SetSubType, (unsigned short int, fSubType))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, GetSubType, (unsigned short int *, lpfSubType))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, GetIsException, (unsigned short int *, lpfIsException))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, GetHasAttach, (unsigned short int *, lpfHasAttach))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, GetItemType, (ULONG *, lpulItemType))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, GetOriginalStartDateTime, (TimezoneDefinition *, lpTZDef), (FILETIME *, lpftStartDateTime))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, GetBaseDate, (ULONG *, lpulBaseDate))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, GetMapiMessage, (ULONG, ulMsgType), (ULONG, ulOverrideFlags), (IMessage **, lppMessage))
DEF_HRMETHOD(TRACE_MAPI, Occurrence, Occurrence, ApplyChanges, (VOID))
