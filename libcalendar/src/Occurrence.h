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

#ifndef Occurrence_INCLUDED
#define Occurrence_INCLUDED

#include "ECUnknown.h"
#include "IOccurrence.h"
#include "RecurrenceState.h"
#include "Appointment.h"
#include "mapi_ptr.h"

struct OccurrenceData;
class TimezoneDefinition;

typedef mapi_object_ptr<Appointment> AppointmentPtr;

class Occurrence : public ECUnknown
{
public:
	static HRESULT Create(ULONG ulBaseDate, ULONG ulStartTimeOffset, ULONG ulEndTimeOffset, TimezoneDefinition *lpTZDefRecur,
	                      const OccurrenceData *lpOccurrenceData, const Appointment::Exception *lpException, Appointment *lpAppointment,
	                      IOccurrence **lppOccurrence);

	// From IUnknown
	virtual HRESULT	QueryInterface(REFIID refiid, void **lppInterface);

	// From IOccurrence
	virtual HRESULT SetStartDateTime(FILETIME ftStartDateTime, TimezoneDefinition *lpTZDef);
	virtual HRESULT GetStartDateTime(TimezoneDefinition *lppTZDef, FILETIME *lpftStartDateTime);

	virtual HRESULT SetEndDateTime(FILETIME ftEndDateTime, TimezoneDefinition *lpTZDef);
	virtual HRESULT GetEndDateTime(TimezoneDefinition *lppTZDef, FILETIME *lpftEndDateTime);

	virtual HRESULT GetDuration(ULONG *lpulDuration);
	
	virtual HRESULT SetBusyStatus(ULONG ulBusyStatus);
	virtual HRESULT GetBusyStatus(ULONG *lpulBusyStatus);

	virtual HRESULT SetLocation(LPCTSTR lpstrLocation, ULONG ulFlags);
	virtual HRESULT GetLocation(LPTSTR *lppstrLocation, LPVOID lpBase, ULONG ulFlags);

	virtual HRESULT SetReminderSet(unsigned short int fSet);
	virtual HRESULT GetReminderSet(unsigned short int *lpfSet);

	virtual HRESULT SetReminderDelta(ULONG ulDelta);
	virtual HRESULT GetReminderDelta(ULONG *lpulDelta);

	virtual HRESULT SetSubject(LPCTSTR lpstrSubject, ULONG ulFlags);
	virtual HRESULT GetSubject(LPTSTR *lppstrSubject, LPVOID lpBase, ULONG ulFlags);

	virtual HRESULT SetMeetingType(ULONG ulMeetingType);
	virtual HRESULT GetMeetingType(ULONG *lpulMeetingType);

	virtual HRESULT SetSubType(unsigned short int fSubType);
	virtual HRESULT GetSubType(unsigned short int *lpfSubType);

	virtual HRESULT GetIsException(unsigned short int *lpfIsException);
	virtual HRESULT GetHasAttach(unsigned short int *lpfHasAttach);
	virtual HRESULT GetItemType(ULONG *lpulItemType);

	virtual HRESULT GetOriginalStartDateTime(TimezoneDefinition *lpTZDef, FILETIME *lpftStartDateTime);
	virtual HRESULT GetBaseDate(ULONG *lpulBaseDate);
	virtual HRESULT GetMapiMessage(ULONG ulMsgType, ULONG ulOverrideFlags, IMessage **lppMessage);
	virtual HRESULT ApplyChanges();


private: // methods
	Occurrence(ULONG ulBaseDate, const OccurrenceData *lpOccurrenceDate, FILETIME ftStartUTC, FILETIME ftEndUTC, Appointment *lpAppointment);
	virtual ~Occurrence();

	HRESULT LoadExceptionMessage();


private: // members
	ULONG m_ulBaseDate;
	bool  m_bIsException;
	bool  m_bOverrideAttachments;
	FILETIME m_ftOriginalStartDateTime;
	std::auto_ptr<OccurrenceData> m_ptrOccurrenceData;

	AppointmentPtr m_ptrAppointment;
	
	AttachPtr m_ptrExceptAttach;
	MessagePtr m_ptrExceptMsg;


private: // interfaces
	class xOccurrence : public IOccurrence {
	public:
		// From IUnknown
		virtual HRESULT __stdcall QueryInterface(REFIID refiid, void** lppInterface);
		virtual ULONG __stdcall AddRef();
		virtual ULONG __stdcall Release();

		// From IOccurrence
		virtual HRESULT SetStartDateTime(FILETIME ftStartDateTime, TimezoneDefinition *lpTZDef);
		virtual HRESULT GetStartDateTime(TimezoneDefinition *lpTZDef, FILETIME *lpftStartDateTime);
		virtual HRESULT SetEndDateTime(FILETIME ftEndDateTime, TimezoneDefinition *lpTZDef);
		virtual HRESULT GetEndDateTime(TimezoneDefinition *lpTZDef, FILETIME *lpftEndDateTime);
		virtual HRESULT GetDuration(ULONG *lpulDuration);
		virtual HRESULT SetBusyStatus(ULONG ulBusyStatus);
		virtual HRESULT GetBusyStatus(ULONG *lpulBusyStatus);
		virtual HRESULT SetLocation(LPCTSTR lpstrLocation, ULONG ulFlags);
		virtual HRESULT GetLocation(LPTSTR *lppstrLocation, LPVOID lpBase, ULONG ulFlags);
		virtual HRESULT SetReminderSet(unsigned short int fSet);
		virtual HRESULT GetReminderSet(unsigned short int *lpfSet);
		virtual HRESULT SetReminderDelta(ULONG ulDelta);
		virtual HRESULT GetReminderDelta(ULONG *lpulDelta);
		virtual HRESULT SetSubject(LPCTSTR lpstrSubject, ULONG ulFlags);
		virtual HRESULT GetSubject(LPTSTR *lppstrSubject, LPVOID lpBase, ULONG ulFlags);
		virtual HRESULT SetMeetingType(ULONG ulMeetingType);
		virtual HRESULT GetMeetingType(ULONG *lpulMeetingType);
		virtual HRESULT SetSubType(unsigned short int fSubType);
		virtual HRESULT GetSubType(unsigned short int *lpfSubType);
		virtual HRESULT GetIsException(unsigned short int *lpfIsException);
		virtual HRESULT GetHasAttach(unsigned short int *lpfHasAttach);
		virtual HRESULT GetItemType(ULONG *lpulItemType);
		virtual HRESULT GetOriginalStartDateTime(TimezoneDefinition *lpTZDef, FILETIME *lpftStartDateTime);
		virtual HRESULT GetBaseDate(ULONG *lpulBaseDate);
		virtual HRESULT GetMapiMessage(ULONG ulMsgType, ULONG ulOverrideFlags, IMessage **lppMessage);
		virtual HRESULT ApplyChanges();

	} m_xOccurrence;
};

#endif // ndef Occurrence_INCLUDED
