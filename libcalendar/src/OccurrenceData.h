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

#ifndef OccurrenceData_INCLUDED
#define OccurrenceData_INCLUDED

#include <string>
#include <memory>

class Appointment;
class TimezoneDefinition;


namespace details {

	/**
	 * The TrackedValue class encapsulates a value and tracks wether
	 * it's modified or not.
	 */
	template <typename _T>
	class TrackedValue {
	public:
		typedef const _T& const_ref;
		typedef _T value_type;

		TrackedValue();
		template <typename _U>
		TrackedValue(const _U &value);
		~TrackedValue();

		operator const_ref () const;
		template <typename _U>
		void set(const _U &value);
		bool dirty() const;
		void mark_clean();

	private:
		TrackedValue(const TrackedValue &);
		TrackedValue& operator=(const TrackedValue &);

	private:
		value_type	m_initialData;
		value_type	*m_lpCurrentData;
	};


	// Implementation
	template <typename _T>
	TrackedValue<_T>::TrackedValue()
	: m_lpCurrentData(NULL)
	{ }

	template <typename _T>
	template <typename _U>
	TrackedValue<_T>::TrackedValue(const _U &value)
	: m_initialData(value)
	, m_lpCurrentData(NULL)
	{ }

	template <typename _T>
	TrackedValue<_T>::~TrackedValue() {
		delete m_lpCurrentData;
	}

	template <typename _T>
	TrackedValue<_T>::operator const_ref () const {
		if (m_lpCurrentData)
			return *m_lpCurrentData;
		return m_initialData;
	}

	template <typename _T>
	template <typename _U>
	void TrackedValue<_T>::set(const _U &value) {
		if (m_lpCurrentData == NULL)
			m_lpCurrentData = new value_type(value);
		else
			*m_lpCurrentData = value;
	}

	template <typename _T>
	bool TrackedValue<_T>::dirty() const {
		return m_lpCurrentData != NULL;
	}

	template <typename _T>
	void TrackedValue<_T>::mark_clean() {
		if (dirty()) {
			m_initialData = *m_lpCurrentData;
			delete m_lpCurrentData;
			m_lpCurrentData = NULL;
		}
	}

} // namespace details


/**
 * This class contains all the data for a single occurrence.
 * When an appointment is created/loaded, there will be once
 * instance containing the default data. For each occurrence that's
 * obtained via GetOccurrence, the data is cloned to get a
 * transactional view of the data. If the requested occurrence is
 * an exception, the cloned instance is updated with the exception
 * data.
 * When the instance for a particular occurrence is updated through
 * the Occurrence interface, the OccurrenceData instance is
 * updated. The OccurrenceData will keep track of the changes in
 * order to determine what has changes when the occurrence is saved,
 * which allows us to determine what needs to be put in the exception.
 */
struct OccurrenceData {
public:
	OccurrenceData(FILETIME ftStart, FILETIME ftEnd, ULONG ulBusyStatus,
	               LPCWSTR lpstrLocation, ULONG ulReminderDelta,
	               LPCWSTR lpstrSubject, unsigned short int fReminderSet,
	               ULONG ulMeetingType, unsigned short int fSubType);

	OccurrenceData* Clone(FILETIME ftStart, FILETIME ftEnd) const;
	void MarkClean();

	details::TrackedValue<FILETIME>				ftStart;
	details::TrackedValue<FILETIME>				ftEnd;
	details::TrackedValue<ULONG>				ulBusyStatus;
	details::TrackedValue<std::wstring>			strLocation;
	details::TrackedValue<ULONG>				ulReminderDelta;
	details::TrackedValue<std::wstring> 		strSubject;
	details::TrackedValue<unsigned short int>	fReminderSet;
	details::TrackedValue<ULONG>				ulMeetingType;
	details::TrackedValue<unsigned short int>	fSubType;
};


/**
 * The OccurrenceDataHelper wraps an OccurrenceData instance and
 * exposes methods that are similar to the IOccurrence interface. This
 * allows all the magic to happen at one place instead of needing the 
 * logic in Occurrence.cpp and Appointment.cpp
 */
class OccurrenceDataHelper
{
public:
	OccurrenceDataHelper(std::auto_ptr<OccurrenceData> &rptrOccurrenceData)
	: m_lpOccurrenceData(rptrOccurrenceData.get())
	{
		ASSERT(m_lpOccurrenceData != NULL);
	}

	HRESULT SetStartDateTime(FILETIME ftStartDateTime, TimezoneDefinition *lpTZDef);
	HRESULT GetStartDateTime(Appointment *lpAppointment, TimezoneDefinition *lpTZDef, FILETIME *lpftStartDateTime);

	HRESULT SetEndDateTime(FILETIME ftEndDateTime, TimezoneDefinition *lpTZDef);
	HRESULT GetEndDateTime(Appointment *lpAppointment, TimezoneDefinition *lpTZDef, FILETIME *lpftEndDateTime);

	HRESULT GetDuration(TimezoneDefinition *lpTZDef, ULONG *lpulDuration);

	HRESULT SetBusyStatus(ULONG ulBusyStatus);
	HRESULT GetBusyStatus(ULONG *lpulBusyStatus);

	HRESULT SetLocation(LPCTSTR lpstrLocation, ULONG ulFlags);
	HRESULT GetLocation(LPTSTR *lppstrLocation, LPVOID lpBase, ULONG ulFlags);

	HRESULT SetReminderSet(unsigned short int fSet);
	HRESULT GetReminderSet(unsigned short int *lpfSet);

	HRESULT SetReminderDelta(ULONG ulDelta);
	HRESULT GetReminderDelta(ULONG *lpulDelta);

	HRESULT SetSubject(LPCTSTR lpstrSubject, ULONG ulFlags);
	HRESULT GetSubject(LPTSTR *lppstrSubject, LPVOID lpBase, ULONG ulFlags);

	HRESULT SetMeetingType(ULONG ulMeetingType);
	HRESULT GetMeetingType(ULONG *lpulMeetingType);

	HRESULT SetSubType(unsigned short int fSubType);
	HRESULT GetSubType(unsigned short int *lpfSubType);

private:
	template<typename T>
	static HRESULT GetScalar(
				const details::TrackedValue<T> &tracked_value,
				T *lpResult
	);

	static HRESULT GetString(
				const details::TrackedValue<std::wstring> &tracked_value,
				LPTSTR *lppstrResult,
				LPVOID lpBase,
				ULONG ulFlags
	);

	HRESULT GetDateTime(
				Appointment *lpAppointment,
				const details::TrackedValue<FILETIME> &tvftTimestamp,
				TimezoneDefinition *lpTZDef,
				FILETIME *lpftDateTime
	);

	HRESULT SetDateTime(
				details::TrackedValue<FILETIME> &tvftTimestamp,
				const FILETIME &ftDateTime,
				TimezoneDefinition *lpTZDef
	);

private:
	OccurrenceData *m_lpOccurrenceData;
};

#endif // ndef
