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
#include "OccurrenceData.h"
#include "Appointment.h"

#include "ECMAPITimezone.h"
#include "charset/convert.h"

namespace details {
	/**
	 * A utility function for extracting the actual
	 * value from a details::TrackedValue instance.
	 *
	 * @param[in]	tracked_value	The tracked value to extract.
	 * @return	The actual value.
	 */
	template <typename T>
	static inline T extract(const TrackedValue<T> &tracked_value) {
		T value = tracked_value;
		return value;
	}
} // namespace internal

/**
 * Constructor
 */
OccurrenceData::OccurrenceData(FILETIME _ftStart, FILETIME _ftEnd, ULONG _ulBusyStatus,
	                           LPCWSTR _lpstrLocation, ULONG _ulReminderDelta,
	                           LPCWSTR _lpstrSubject, unsigned short int _fReminderSet,
	                           ULONG _ulMeetingType, unsigned short int _fSubType)
: ftStart(_ftStart)
, ftEnd(_ftEnd)
, ulBusyStatus(_ulBusyStatus)
, strLocation(_lpstrLocation)
, ulReminderDelta(_ulReminderDelta)
, strSubject(_lpstrSubject)
, fReminderSet(_fReminderSet)
, ulMeetingType(_ulMeetingType)
, fSubType(_fSubType)
{ }

/**
 * Clone an OccurrenceData instance but update the start and end timestamps without
 * them becomming dirty.
 * 
 * @param[in]	ftStartNew	The new start timestamp.
 * @param[in]	ftEndNew	The new end timestamp.
 */
OccurrenceData* OccurrenceData::Clone(FILETIME ftStartNew, FILETIME ftEndNew) const
{
	return new OccurrenceData(
		ftStartNew,
		ftEndNew,
		ulBusyStatus,
		details::extract(strLocation).c_str(),
		ulReminderDelta,
		details::extract(strSubject).c_str(),
		fReminderSet,
		ulMeetingType,
		fSubType
	);
}

/**
 * Mark all tracked_values as clean.
 */
void OccurrenceData::MarkClean()
{
	ftStart.mark_clean();
	ftEnd.mark_clean();
	ulBusyStatus.mark_clean();
	strLocation.mark_clean();
	ulReminderDelta.mark_clean();
	strSubject.mark_clean();
	fReminderSet.mark_clean();
	ulMeetingType.mark_clean();
	fSubType.mark_clean();
}

/*
 * OccurrenceDataHelper
 */

template<typename T>
HRESULT OccurrenceDataHelper::GetScalar(
			const details::TrackedValue<T> &tracked_value,
			T *lpResult
)
{
	HRESULT hr = hrSuccess;

	if (lpResult == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	*lpResult = tracked_value;

exit:
	return hr;
}

HRESULT OccurrenceDataHelper::GetString(
			const details::TrackedValue<std::wstring> &tracked_value,
			LPTSTR *lppstrResult,
			LPVOID lpBase,
			ULONG ulFlags
)
{
	HRESULT hr = hrSuccess;
	std::wstring wstrValue;

	if (lppstrResult == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	wstrValue = tracked_value;

	if (ulFlags & MAPI_UNICODE) {
		if (lpBase)
			hr = MAPIAllocateMore((wstrValue.length() + 1) * sizeof(WCHAR), lpBase, (LPVOID*)lppstrResult);
		else
			hr = MAPIAllocateBuffer((wstrValue.length() + 1) * sizeof(WCHAR), (LPVOID*)lppstrResult);
		if (hr != hrSuccess)
			goto exit;

		wcscpy((LPWSTR)*lppstrResult, wstrValue.c_str());
	} else {
		const std::string strValue = convert_to<std::string>(wstrValue);

		if (lpBase)
			hr = MAPIAllocateMore((strValue.length() + 1), lpBase, (LPVOID*)lppstrResult);
		else
			hr = MAPIAllocateBuffer((strValue.length() + 1), (LPVOID*)lppstrResult);
		if (hr != hrSuccess)
			goto exit;

		strcpy((LPSTR)*lppstrResult, strValue.c_str());
	}

exit:
	return hr;
}

HRESULT OccurrenceDataHelper::GetDateTime(
			Appointment *lpAppointment,
			const details::TrackedValue<FILETIME> &tvftTimestamp,
			TimezoneDefinition *lpTZDef,
			FILETIME *lpftDateTime
)
{
	HRESULT hr = hrSuccess;
	TimezoneDefinitionPtr ptrStartTZ;

	if (lpftDateTime == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (lpAppointment &&
	    lpTZDef != NULL &&
	    m_lpOccurrenceData->fSubType == 1 &&
	    (m_lpOccurrenceData->ulMeetingType & 1) == 0)
	{
		// Both start- and end-time must be converted with the start timezone for
		// floating appointments.
		hr = lpAppointment->GetStartTimezone(&ptrStartTZ);
		if (hr == hrSuccess || hr == MAPI_E_NOT_FOUND) {
			lpTZDef = ptrStartTZ;
			hr = hrSuccess;
		} else
			goto exit;
	}

	if (lpTZDef) {
		hr = lpTZDef->FromUTC(tvftTimestamp, lpftDateTime);
		if (hr != hrSuccess)
			goto exit;
	} else
		*lpftDateTime = tvftTimestamp;

exit:
	return hr;
}

HRESULT OccurrenceDataHelper::SetDateTime(
				details::TrackedValue<FILETIME> &tvftTimestamp,
				const FILETIME &ftDateTime,
				TimezoneDefinition *lpTZDef
	)
{
	HRESULT hr = hrSuccess;
	FILETIME ftDateTimeUTC;

	if (lpTZDef) {
		hr = lpTZDef->ToUTC(ftDateTime, &ftDateTimeUTC);
		if (hr != hrSuccess)
			goto exit;
	} else
		ftDateTimeUTC = ftDateTime;

	tvftTimestamp.set(ftDateTimeUTC);

exit:
	return hr;
}

HRESULT OccurrenceDataHelper::SetStartDateTime(FILETIME ftStartDateTime, TimezoneDefinition *lpTZDef)
{
	return SetDateTime(m_lpOccurrenceData->ftStart, ftStartDateTime, lpTZDef);
}

HRESULT OccurrenceDataHelper::GetStartDateTime(Appointment *lpAppointment, TimezoneDefinition *lpTZDef, FILETIME *lpftStartDateTime)
{
	return GetDateTime(lpAppointment, m_lpOccurrenceData->ftStart, lpTZDef, lpftStartDateTime);
}

HRESULT OccurrenceDataHelper::SetEndDateTime(FILETIME ftEndDateTime, TimezoneDefinition *lpTZDef)
{
	return SetDateTime(m_lpOccurrenceData->ftEnd, ftEndDateTime, lpTZDef);
}

HRESULT OccurrenceDataHelper::GetEndDateTime(Appointment *lpAppointment, TimezoneDefinition *lpTZDef, FILETIME *lpftEndDateTime)
{
	return GetDateTime(lpAppointment, m_lpOccurrenceData->ftEnd, lpTZDef, lpftEndDateTime);
}

HRESULT OccurrenceDataHelper::GetDuration(TimezoneDefinition *lpTZDef, ULONG *lpulDuration)
{
	/*
	 * It seems logical to calculate the duration by substracting the start datetime
	 * from the end datatime using the UTC values. This would yield the correct duration
	 * if one would clock the duration, also when a DST change happens during the
	 * occurrence.
	 * However, it seems Outlook/MAPI make the calculation using the local times. This
	 * will return the same value, most of the time. But not always.
	 */
	HRESULT hr = hrSuccess;
	FILETIME ftStart;
	FILETIME ftEnd;
	time_t duration;	// in seconds

	if (lpulDuration == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	/*
	 * There's no correction required for floating appointments, so we
	 * can pass NULL as the Appointment argument for GetStartDateTime and
	 * GetEndDateTime.
	 */
	hr = GetStartDateTime(NULL, lpTZDef, &ftStart);
	if (hr == hrSuccess)
		hr = GetEndDateTime(NULL, lpTZDef, &ftEnd);
	if (hr != hrSuccess)
		goto exit;

	duration = ftEnd - ftStart;
	*lpulDuration = (duration / 60);

exit:
	return hr;
}

HRESULT OccurrenceDataHelper::SetBusyStatus(ULONG ulBusyStatus)
{
	m_lpOccurrenceData->ulBusyStatus.set(ulBusyStatus);
	return hrSuccess;
}

HRESULT OccurrenceDataHelper::GetBusyStatus(ULONG *lpulBusyStatus)
{
	return GetScalar(m_lpOccurrenceData->ulBusyStatus, lpulBusyStatus);
}

HRESULT OccurrenceDataHelper::SetLocation(LPCTSTR lpstrLocation, ULONG ulFlags)
{
	HRESULT hr = hrSuccess;

	if (lpstrLocation == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (ulFlags & MAPI_UNICODE)
		m_lpOccurrenceData->strLocation.set(reinterpret_cast<LPCWSTR>(lpstrLocation));
	else
		m_lpOccurrenceData->strLocation.set(convert_to<std::wstring>(reinterpret_cast<LPCSTR>(lpstrLocation)));

exit:
	return hr;
}

HRESULT OccurrenceDataHelper::GetLocation(LPTSTR *lppstrLocation, LPVOID lpBase, ULONG ulFlags)
{
	return GetString(m_lpOccurrenceData->strLocation, lppstrLocation, lpBase, ulFlags);
}

HRESULT OccurrenceDataHelper::SetReminderSet(unsigned short int fSet)
{
	m_lpOccurrenceData->fReminderSet.set(fSet);
	return hrSuccess;
}

HRESULT OccurrenceDataHelper::GetReminderSet(unsigned short int *lpfSet)
{
	return GetScalar(m_lpOccurrenceData->fReminderSet, lpfSet);
}

HRESULT OccurrenceDataHelper::SetReminderDelta(ULONG ulDelta)
{
	m_lpOccurrenceData->ulReminderDelta.set(ulDelta);
	return hrSuccess;
}

HRESULT OccurrenceDataHelper::GetReminderDelta(ULONG *lpulDelta)
{
	return GetScalar(m_lpOccurrenceData->ulReminderDelta, lpulDelta);
}

HRESULT OccurrenceDataHelper::SetSubject(LPCTSTR lpstrSubject, ULONG ulFlags)
{
	HRESULT hr = hrSuccess;

	if (lpstrSubject == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (ulFlags & MAPI_UNICODE)
		m_lpOccurrenceData->strSubject.set(reinterpret_cast<LPCWSTR>(lpstrSubject));
	else
		m_lpOccurrenceData->strSubject.set(convert_to<std::wstring>(reinterpret_cast<LPCSTR>(lpstrSubject)));

exit:
	return hr;
}

HRESULT OccurrenceDataHelper::GetSubject(LPTSTR *lppstrSubject, LPVOID lpBase, ULONG ulFlags)
{
	return GetString(m_lpOccurrenceData->strSubject, lppstrSubject, lpBase, ulFlags);
}

HRESULT OccurrenceDataHelper::SetMeetingType(ULONG ulMeetingType)
{
	m_lpOccurrenceData->ulMeetingType.set(ulMeetingType);
	return hrSuccess;
}

HRESULT OccurrenceDataHelper::GetMeetingType(ULONG *lpulMeetingType)
{
	return GetScalar(m_lpOccurrenceData->ulMeetingType, lpulMeetingType);
}

HRESULT OccurrenceDataHelper::SetSubType(unsigned short int fSubType)
{
	m_lpOccurrenceData->fSubType.set(fSubType);
	return hrSuccess;
}

HRESULT OccurrenceDataHelper::GetSubType(unsigned short int *lpfSubType)
{
	return GetScalar(m_lpOccurrenceData->fSubType, lpfSubType);
}
