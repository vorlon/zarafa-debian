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

#ifndef IOccurrence_INCLUDED
#define IOccurrence_INCLUDED

#include "ECMAPITimezone.h"
#include <mapidefs.h>

class ECBaseDate;
class RecurrencePattern;


#define CALENDAR_ITEMTYPE_SINGLE			0
#define CALENDAR_ITEMTYPE_OCCURRENCE		1
#define CALENDAR_ITEMTYPE_EXCEPTION			2
#define CALENDAR_ITEMTYPE_RECURRINGMASTER	3


#define CALENDAR_MSGTYPE_DEFAULT	0
#define CALENDAR_MSGTYPE_MASTER		1
#define CALENDAR_MSGTYPE_OCCURRENCE	2
#define CALENDAR_MSGTYPE_COMPOSITE	3
#define _CALENDAR_MSGTYPE_MAX		CALENDAR_MSGTYPE_COMPOSITE


#define CALENDAR_ENSURE_OVERRIDE_ATTACHMENTS	1
#define _CALENDAR_ENSURE_OVERRIDE_MASK			(CALENDAR_ENSURE_OVERRIDE_ATTACHMENTS)


/**
 * Interface to the occurrence instance.
 */
class IOccurrence : public IUnknown
{
public:
	/**
	 * Set the start date and time of the occurrence. The provided timestamp is represented in
	 * the provided timezone or UTC.
	 *
	 * @param[in]	ftStartDateTime		The timestamp.
	 * @param[in]	lpTZDef				The timezone definition in which ftStartDateTime is
	 * 									represented or NULL for UTC.
	 */
	virtual HRESULT SetStartDateTime(FILETIME ftStartDateTime, TimezoneDefinition *lpTZDef) = 0;

	/**
	 * Get the start date and time in the specified timezone or UTC.
	 *
	 * @param[in]	lpTZDef				The timezone definition of the timezone in which to return
	 * 									the start date and time. May be NULL to request UTC.
	 * @param[out]	lpftStartDateTime	The filetime structure representing the start date and time
	 * 									in the provided timezone.
	 */
	virtual HRESULT GetStartDateTime(TimezoneDefinition *lpTZDef, FILETIME *lpftStartDateTime) = 0;

	/**
	 * Set the end date and time of the occurrence. The provided timestamp is represented in
	 * the provided timezone or UTC.
	 *
	 * @param[in]	ftEndDateTime		The timestamp.
	 * @param[in]	lpTZDef				The timezone definition in which ftEndDateTime is
	 * 									represented or NULL for UTC.
	 */
	virtual HRESULT SetEndDateTime(FILETIME ftEndDateTime, TimezoneDefinition *lpTZDef) = 0;

	/**
	 * Get the end date and time in the specified timezone or UTC.
	 *
	 * @param[in]	lpTZDef				The timezone definition of the timezone in which to return
	 * 									the end date and time. May be NULL to request UTC.
	 * @param[out]	lpftEndDateTime		The filetime structure representing the end date and time
	 * 									in the provided timezone.
	 */
	virtual HRESULT GetEndDateTime(TimezoneDefinition *lpTZDef, FILETIME *lpftEndDateTime) = 0;

	/**
	 * Get the duration of the occurrence.
	 *
	 * @param[out]	lpulDuration	The duration in minutes.
	 */
	virtual HRESULT GetDuration(ULONG *lpulDuration) = 0;

	/**
	 * Set the busy status.
	 * Valid values are:
	 * 0 - olFree
	 * 1 - olTentative
	 * 2 - olBusy
	 * 3 - olOutOfOffice
	 *
	 * @param[in]	ulBusyStatus	The new busy status.
	 */
	virtual HRESULT SetBusyStatus(ULONG ulBusyStatus) = 0;

	/**
	 * Get the busy Status
	 * Valid values are:
	 * 0 - olFree
	 * 1 - olTentative
	 * 2 - olBusy
	 * 3 - olOutOfOffice
	 *
	 * @param[out]	lpulBusyStatus	The stored busy status.
	 */
	virtual HRESULT GetBusyStatus(ULONG *lpulBusyStatus) = 0;

	/**
	 * Set the location.
	 *
	 * @param[in]	lpstrLocation	The new location.
	 * @param[in[	ulFlags			0 or MAPI_UNICODE if lpstrLocation is a Unicode string.
	 */
	virtual HRESULT SetLocation(LPCTSTR lpstrLocation, ULONG ulFlags) = 0;

	/**
	 * Get the location
	 *
	 * @param[out]	lppstrLocation	The stored location.
	 * @param[in]	lpBase			An optional base pointer used for MAPIAllocateMore.
	 * @param[in]	ulFlags			0 or MAPI_UNICODE if a Unicode string is expected.
	 */
	virtual HRESULT GetLocation(LPTSTR *lppstrLocation, LPVOID lpBase, ULONG ulFlags) = 0;

	/**
	 * Set the reminder set flag
	 *
	 * @param[in]	fSet	The new flag
	 */
	virtual HRESULT SetReminderSet(unsigned short int fSet) = 0;

	/**
	 * Get the reminder set flag
	 *
	 * @param[out]	lpfSet	The store flag
	 */
	virtual HRESULT GetReminderSet(unsigned short int *lpfSet) = 0;

	/**
	 * Set the reminder delta
	 *
	 * @param[in]	ulDelta		The new delta in minutes
	 */
	virtual HRESULT SetReminderDelta(ULONG ulDelta) = 0;

	/**
	 * Get the reminder delta
	 *
	 * @param[out]	lpulDelta	The stored delta in minutes
	 */
	virtual HRESULT GetReminderDelta(ULONG *lpulDelta) = 0;

	/**
	 * Set the subject
	 *
	 * @param[in]	lpstrSubject	The new subject
	 * @param[in]	ulFlags			0 or MAPI_UNICODE if lpstrSubject is a Unicode string.
	 */
	virtual HRESULT SetSubject(LPCTSTR lpstrSubject, ULONG ulFlags) = 0;

	/**
	 * Get the Subject
	 *
	 * @param[out]	lppstrSubject	The stored subject
	 * @param[in]	lpBase			An optional base pointer used for MAPIAllocateMore.
	 * @param[in]	ulFlags			0 or MAPI_UNICODE is a Unicode string is expected.
	 */
	virtual HRESULT GetSubject(LPTSTR *lppstrSubject, LPVOID lpBase, ULONG ulFlags) = 0;

	/**
	 * Set the meeting type
	 * This is a bitmask composed of the following components:
	 * bit 0: asfMeeting - The object is a meeting or meeting related object.
	 * bit 1: asfReceived - The object was received from someone else.
	 * bit 2: asfCanceled - The meeting associated with the object was canceled.
	 *
	 * @param[in]	ulMeetingType	The new meeting type.
	 *
	 * @todo: Rename to SetAppointmentState to avoid confusion.
	 */
	virtual HRESULT SetMeetingType(ULONG ulMeetingType) = 0;

	/**
	 * Get the meeting type
	 * This is a bitmask composed of the following components:
	 * bit 0: asfMeeting - The object is a meeting or meeting related object.
	 * bit 1: asfReceived - The object was received from someone else.
	 * bit 2: asfCanceled - The meeting associated with the object was canceled.
	 *
	 * @param[out]	lpulMeetingType		The stored meeting type.
	 *
	 * @todo: Rename to GetAppointmentState to avoid confusion.
	 */
	virtual HRESULT GetMeetingType(ULONG *lpulMeetingType) = 0;

	/**
	 * Set the sub type flag (all day)
	 *
	 * @param[in]	fSubType	The new flag
	 */
	virtual HRESULT SetSubType(unsigned short int fSubType) = 0;

	/**
	 * Get the sub type flag (all day)
	 *
	 * @param[out]	lpfSubType	The stored flag
	 */
	virtual HRESULT GetSubType(unsigned short int *lpfSubType) = 0;

	/**
	 * See if an occurrence is an exception.
	 *
	 * @param[out]	lpfIsException	non-zero if the occurrence is an exception.
	 */
	virtual HRESULT GetIsException(unsigned short int *lpfIsException) = 0;

	/**
	 * See if the occurrence message has one or more attachments.
	 *
	 * @param[out]	lpfHasAttach	non-zero if one or more attachments are present.
	 */
	virtual HRESULT GetHasAttach(unsigned short int *lpfHasAttach) = 0;

	/**
	 * Get the type of the occurrence.
	 * Valid values are:
	 * - CALENDAR_ITEMTYPE_SINGLE
	 * - CALENDAR_ITEMTYPE_OCCURRENCE
	 * - CALENDAR_ITEMTYPE_EXCEPTION
	 * - CALENDAR_ITEMTYPE_RECURRINGMASTER
	 *
	 * @param[out]	lpulItemType	The type of the occurrence
	 */
	virtual HRESULT GetItemType(ULONG *lpulItemType) = 0;

	/**
	 * Get the original start date and time (in UTC).
	 *
	 * @param[in]	lpTZDef				The timezone definition of the timezone in which to return
	 * 									the date and time. May be NULL to request UTC.
	 * @param[out]	lpftStartDateTime	The filetime structure representing the original
	 * 									start date and time.
	 */
	virtual HRESULT GetOriginalStartDateTime(TimezoneDefinition *lpTZDef, FILETIME *lpftStartDateTime) = 0;

	/**
	 * Get the basedate of the occurrence.
	 *
	 * @param[out]	lpulBaseDate	The basedate of the occurrence.
	 */
	virtual HRESULT GetBaseDate(ULONG *lpulBaseDate) = 0;

	/**
	 * Get the MAPI message associated with the occurrence. This will always return
	 * MAPI_E_NOT_FOUND for occurrences of a recurring series that are not an exception.
	 * If the occurrence was created from table data this call will always return
	 * MAPI_E_NO_SUPPORT
	 * @param[in]	ulMsgType		The required message type.
	 * @param[in]	ulOverrideFlags	The flags that ensure override behaviour of the occurrence.
	 * @param[out]	lppMessage		The MAPI message associated with the occurrence.
	 *
	 * Valid values for ulMsgType are:
	 * - CALENDAR_MSGTYPE_DEFAULT
	 * - CALENDAR_MSGTYPE_MASTER
	 * - CALENDAR_MSGTYPE_OCCURRENCE
	 * - CALENDAR_MSGTYPE_COMPOSITE
	 *
	 * It's up to the implementation of the IOccurrence interface which flags are supported and
	 * which flag CALENDAR_MSGTYPE_DEFAULT defaults to.
	 *
	 * Currently an Appointment instance will only support CALENDAR_MSGTYPE_MASTER, which
	 * CALENDAR_MSGTYPE_DEFAULT will map to.
	 * Occurrence instances will support all types and CALENDAR_MSGTYPE_DEFAULT will map to
	 * CALENDAR_MSGTYPE_COMPOSITE.
	 *
	 * Valid values for ulOverrideFlags are:
	 * - CALENDAR_ENSURE_OVERRIDE_ATTACHMENTS
	 *
	 * The purpose of these flags is to ensure that an exception will override the attachments and/or
	 * the recipients. If this is already the case, this is a no-op.
	 */
	virtual HRESULT GetMapiMessage(ULONG ulMsgType, ULONG ulOverrideFlags, IMessage **lppMessage) = 0;

	/**
	 * Apply all changes that were made to the occurrence. This will cause the underlying
	 * storage to be updated but NOT saved. The caller should take care of persisting the
	 * data stored in the underlying storage.
	 */
	virtual HRESULT ApplyChanges() = 0;
};

#endif // ndef IOccurrence_INCLUDED
