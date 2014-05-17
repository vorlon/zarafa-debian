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

#ifndef IAppointment_INCLUDED
#define IAppointment_INCLUDED

#include "IOccurrence.h"

/**
 * Interface to the base appointment instance. The base appointment behaves like
 * an occurrence but contains additional methods to handle the recurrence pattern,
 * occurrences and exceptions.
 */
class IAppointment : public IOccurrence
{
public:
	/**
	 * Set a recurrence pattern for the appointment. This will remove all existing
	 * exceptions.
	 *
	 * @param[in]	lpPattern	The RecurrencePattern describing the new recurrence pattern.
	 */
	virtual HRESULT SetRecurrence(RecurrencePattern *lpPattern) = 0;

	/**
	 * Get the currently set recurrence pattern. This will return a copy of the
	 * internal pattern, so modifying the pattern will not affect the appointment.
	 *
	 * @param[out]	lppPattern	The RecurrencePattern describing the currence recurrence
	 * 							pattern. The returned RecurrencePattern instance is
	 * 							allocated and must be deleted by the caller.
	 */
	virtual HRESULT GetRecurrence(RecurrencePattern **lppPattern) = 0;

	/**
	 * Set the timezone in which the recurrence is defined. This will remove all existing
	 * exceptions.
	 *
	 * @param[in]	lpTZDef		The TimezoneDefinition describing the timezone in which the
	 * 							recurrence pattern is defined.
	 */
	virtual HRESULT SetRecurrenceTimezone(TimezoneDefinition *lpTZDef) = 0;

	/**
	 * Get the timezone in which the recurrence is defined. This resulting
	 * object must bre released by the caller.
	 *
	 * @param[out]	lppTZDef	The TimezoneDefinition describing the timezone in which the
	 * 							recurrence pattern is defined. The returned TimezoneDefinition
	 * 							must be released by the caller.
	 */
	virtual HRESULT GetRecurrenceTimezone(TimezoneDefinition **lppTZDef) = 0;

	/**
	 * Get the bounds of the appointment. This will return the original dates of
	 * the first and the last occurrence. This method takes exceptions into account.
	 *
	 * @param[out]	lpulFirst	The original date, specified in rtime, of the
								first occurrence.
	 * @param[out]	lpulLast	The original date, specified in rtime, of the
								last occurrence. This value will be set to zero
								for recurrences without an end.
	 */
	virtual HRESULT GetBounds(ULONG *lpulFirst, ULONG *lpulLast) = 0;

	/**
	 * Get the occurrence that would occur on a specific basedate. The basedate for any
	 * occurrence is the date of the original occurrence. So exceptions that occur on
	 * a different day will need to be obtained through its original date.
	 *
	 * @param[in]	ulBaseDate		The basedate of the requested occurrence.
	 * @param[out]	lppOccurrence	The requested occurrence. The returned instance must
	 * 								be released by the caller (by calling Release()).
	 */
	virtual HRESULT GetOccurrence(ULONG ulBaseDate, IOccurrence **lppOccurrence) = 0;

	/**
	 * Remove an occurrence. This creates an exception that indicates that the occurrence
	 * for basedate is cancelled.
	 *
	 * @param[in]	ulBaseDate	The basedate of the occurrence to remove.
	 */
	virtual HRESULT RemoveOccurrence(ULONG ulBaseDate) = 0;

	/**
	 * Resets an occurrence. This can only be performed on an exception and will actually
	 * remove the exception, causing the original occurrence to appear again.
	 *
	 * @param[in]	ulBaseDate	The basedate of the occurrence to reset.
	 */
	virtual HRESULT ResetOccurrence(ULONG ulBaseDate) = 0;

	/**
	 * Get the list of modified exceptions and the list of deleted exceptions. The modified
	 * exceptions are those exceptions that modify the original occurrence in any way. The
	 * deleted exceptions are those exceptions that represent occurrences that are cancelled.
	 *
	 * @param[out]	lpcbModifiedExceptions	The number of returned modified exceptions.
	 * @param[out]	lpaModifyExceptions		The array with the basedates of the modified exceptions.
	 * 										This array must be deleted by the caller.
	 * @param[out]	lpcbDeleteExceptions	The number of returned deleted exceptions.
	 * @param[out]	lpaDeleteExceptions		The array with the basedates of the deleted exceptions.
	 * 										This array must be deleted by the caller.
	 */
	virtual HRESULT GetExceptions(ULONG *lpcbModifyExceptions, ULONG **lpaModifyExceptions,
								  ULONG *lpcbDeleteExceptions, ULONG **lpaDeleteExceptions) = 0;

	/**
	 * Return the basedate of the ulIndexth occurrence in the series.
	 *
	 * @param[in]	ulIndex			The index of the occurrence for which to
	 * 								return the basedate.
	 * @param[out]	lpulBaseDate	The returned basedate.
	 */
	virtual HRESULT GetBaseDateForOccurrence(ULONG ulIndex, ULONG *lpulBaseDate) = 0;
};

#endif // ndef IAppointment_INCLUDED
