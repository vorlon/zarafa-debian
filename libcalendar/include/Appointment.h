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

#ifndef Appointment_INCLUDED
#define Appointment_INCLUDED

#include "IAppointment.h"
#include "PropertyPool.h"
#include "RecurrenceState.h"

#include <mapidefs.h>
#include "mapi_ptr.h"

#include <map>
#include <set>
#include <memory>

struct OccurrenceData;
class Occurrence;

class RecurrencePattern;

class Appointment : public ECUnknown
{
public: // types
	typedef RecurrenceState::Exception BasicException;
	typedef RecurrenceState::ExtendedException ExtendedException;

	typedef struct _Exception {
		BasicException basic;
		ExtendedException extended;
	} Exception;


private: // types
	typedef mapi_object_ptr<RecurrencePattern> RecurrencePatternPtr;
	typedef std::map<ULONG, Exception> ExceptionMap;
	typedef std::set<ULONG> BasedateSet;


public: // methods
	static HRESULT Create(IMessage *lpMessage, TimezoneDefinition *lpClientTZ, IAppointment **lppAppointment);
	static HRESULT Create(ULONG cValues, LPSPropValue lpPropArray, PropertyPool *lpPropertyPool, TimezoneDefinition *lpClientTZ, IAppointment **lppAppointment);

	static HRESULT Create(IMAPIFolder *lpFolder, TimezoneDefinition *lpClientTZ, IAppointment **lppAppointment, IMessage **lppMessage);

	// From IUnknown
	virtual HRESULT	QueryInterface(REFIID refiid, void **lppInterface);

	// From IAppointment
	virtual HRESULT SetRecurrence(RecurrencePattern *lpPattern);
	virtual HRESULT GetRecurrence(RecurrencePattern **lppPattern);
	virtual HRESULT SetRecurrenceTimezone(TimezoneDefinition *lpTZDef);
	virtual HRESULT GetRecurrenceTimezone(TimezoneDefinition **lppTZDef);
	virtual HRESULT GetBounds(ULONG *lpulFirst, ULONG *lpulLast);
	virtual HRESULT GetOccurrence(ULONG ulBaseDate, IOccurrence **lppOccurrence);
	virtual HRESULT RemoveOccurrence(ULONG ulBaseDate);
	virtual HRESULT ResetOccurrence(ULONG ulBaseDate);
	virtual HRESULT GetExceptions(ULONG *lpcbModifyExceptions, ULONG **lpaModifyExceptions,
	                              ULONG *lpcbDeleteExceptions, ULONG **lpaDeleteExceptions);
	virtual HRESULT GetBaseDateForOccurrence(ULONG ulIndex, ULONG *lpulBaseDate);

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

	// Internal use
	HRESULT GetAttachmentAndMessage(Occurrence *lpOccurence, IAttach **lppAttach, IMessage **lppMessage);
	HRESULT GetPropertyPool(PropertyPool **lppPropertyPool);
	HRESULT GetExceptionData(ULONG ulBaseDate, Exception *lpException);
	HRESULT SetExceptionData(ULONG ulBaseDate, const Exception &exception);
	HRESULT GetStartTimezone(TimezoneDefinition **lppTZDef);

private: // methods
	static HRESULT Create(PropertyPool *lpPropertyPool, TimezoneDefinition *lpClientTZ, Appointment **lppAppointment);

	Appointment(PropertyPool *lpPropertyPool, TimezoneDefinition *lpClientTZ);
	virtual ~Appointment();

	HRESULT Attach(IMessage *lpMessage);
	HRESULT ProcessProps(ULONG cValues, LPSPropValue lpPropArray);
	
	HRESULT GetTZDef(ULONG cValues, LPSPropValue lpPropArray, FILETIME ftTimestamp, TimezoneDefinition **lppTZDef);

	HRESULT GetBoundsSingle(ULONG *lpulFirst, ULONG *lpulLast);
	HRESULT GetBoundsRecurring(ULONG *lpulFirst, ULONG *lpulLast);
	HRESULT FindFirstOccurrence(ULONG *lpulDate);
	HRESULT UpdateRecurrence();
	HRESULT UpdateTimeOffsets(TimezoneDefinition *lpTZDef);

	static HRESULT SetDefaults(IMessage *lpMessage);

private: // members
	PropertyPoolPtr m_ptrPropertyPool;
	TimezoneDefinitionPtr m_ptrClientTZ;

	MessagePtr m_ptrMessage;
	std::auto_ptr<OccurrenceData> m_ptrOccurrenceData;

	ULONG m_ulStartTimeOffset;
	ULONG m_ulEndTimeOffset;
	bool m_bHasAttach;
	TimezoneDefinitionPtr m_ptrTZStartTime;
	TimezoneDefinitionPtr m_ptrTZDefRecur;
	ExceptionMap m_mapModifyExceptions;
	BasedateSet m_setDeletedExceptions;

	RecurrencePatternPtr m_ptrRecurrencePattern;

private: // interfaces
	class xAppointment : public IAppointment {
	public:
		// From IUnknown
		virtual HRESULT __stdcall QueryInterface(REFIID refiid, void** lppInterface);
		virtual ULONG __stdcall AddRef();
		virtual ULONG __stdcall Release();

		// From IAppointment
		virtual HRESULT SetRecurrence(RecurrencePattern *lpPattern);
		virtual HRESULT GetRecurrence(RecurrencePattern **lppPattern);
		virtual HRESULT SetRecurrenceTimezone(TimezoneDefinition *lpTZDef);
		virtual HRESULT GetRecurrenceTimezone(TimezoneDefinition **lppTZDef);
		virtual HRESULT GetBounds(ULONG *lpulFirst, ULONG *lpulLast);
		virtual HRESULT GetOccurrence(ULONG ulBaseDate, IOccurrence **lppOccurrence);
		virtual HRESULT RemoveOccurrence(ULONG ulBaseDate);
		virtual HRESULT ResetOccurrence(ULONG ulBaseDate);
		virtual HRESULT GetExceptions(ULONG *lpcbModifyExceptions, ULONG **lpaModifyExceptions,
									  ULONG *lpcbDeleteExceptions, ULONG **lpaDeleteExceptions);
		virtual HRESULT GetBaseDateForOccurrence(ULONG ulIndex, ULONG *lpulBaseDate);

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
	} m_xAppointment;
};

#endif // ndef Appointment_INCLUDED
