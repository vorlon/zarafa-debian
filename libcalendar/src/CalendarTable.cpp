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
#include "CalendarTable.h"
#include "mapi_ptr.h"
#include "ECRestriction.h"
#include "PropertyPool.h"
#include "ECMemTable.h"
#include "Appointment.h"
#include "RecurrencePattern.h"
#include "Util.h"
#include "CalendarUtil.h"
#include "Conversions.h"
#include "IOccurrence.h"
#include "CommonUtil.h"

#include <mapicode.h>

#include <set>

typedef mapi_object_ptr<ECMemTable> ECMemTablePtr;
typedef mapi_object_ptr<ECMemTableView> ECMemTableViewPtr;
typedef mapi_object_ptr<IAppointment> AppointmentPtr;
typedef mapi_object_ptr<RecurrencePattern> RecurrencePatternPtr;
typedef mapi_object_ptr<IOccurrence> OccurrencePtr;

typedef mapi_array_ptr<ULONG> UlongPtr;

#define PROP(_name) (m_ptrPropertyPool->GetPropTag(PropertyPool::PROP_##_name))


/**
 * This class does the actual work of building a CalendarTable.
 */
class CalendarTableBuilder
{
private:
	IMAPIFolder*	m_lpCalendar;
	FILETIME		m_ftStartDateTime;
	FILETIME		m_ftEndDateTime;
	TimezoneDefinitionPtr m_ptrClientTZ;
	LPSPropTagArray	m_lpsPropTags;
	ULONG			m_ulRowCount;

	PropertyPoolPtr	m_ptrPropertyPool;
	SRestrictionPtr	m_ptrRestriction;
	ECMemTablePtr	m_ptrMemTable;


	/**
	 * Create the restriction used to restrict the original calendar
	 * folder and to post process the individual occurrences.
	 */
	HRESULT CreateRestriction()
	{
		ECOrRestriction restriction;

		ULONG ulTagStart, ulTagEnd, ulTagRecurring;
		SPropValue sPropStart, sPropEnd, sPropRecurring;

		ulTagStart = PROP(APPTSTARTWHOLE);
		ulTagEnd = PROP(APPTENDWHOLE);
		ulTagRecurring = PROP(RECURRING);

		// Appointment is recurring
		sPropRecurring.ulPropTag = ulTagRecurring;
		sPropRecurring.Value.b = TRUE;
		restriction.append(ECPropertyRestriction(RELOP_EQ, ulTagRecurring, &sPropRecurring));

		sPropStart.ulPropTag = PROP_TAG(PT_SYSTIME, PROP_ID_NULL);
		sPropStart.Value.ft = m_ftStartDateTime;
		sPropEnd.ulPropTag = PROP_TAG(PT_SYSTIME, PROP_ID_NULL);
		sPropEnd.Value.ft = m_ftEndDateTime;

		restriction.append(
			// Or Appointment end after range start AND appointment start before range end
			ECAndRestriction(
				ECPropertyRestriction(RELOP_GT, ulTagEnd, &sPropStart) +
				ECPropertyRestriction(RELOP_LT, ulTagStart, &sPropEnd)
			) +

			// Or zero-length appointment starting (and ending) at range start
			ECAndRestriction(
				ECPropertyRestriction(RELOP_EQ, ulTagStart, &sPropStart) +
				ECPropertyRestriction(RELOP_EQ, ulTagEnd, &sPropStart)
			)
		);

		return restriction.CreateMAPIRestriction(&m_ptrRestriction);
	}


	/**
	 * This method creates a proptag array that can be used to set the columns on the
	 * original content table. The resulting rows will contain the user requested rows
	 * in a continuous section, so they can be used to populate the memtable.
	 * The first n rows will contains the requested properties. Then there will be a
	 * PR_NULL that is to be replaced with PR_ROWID before insertion into the memtable.
	 * After that are the properties that are required for proper processing of the data.
	 * 
	 * @param[out]	lppCombinedTags		The proptag array containing the combined tags.
	 */
	HRESULT CreatePropTagArray(LPSPropTagArray *lppCombinedTags)
	{
		HRESULT hr = hrSuccess;
		std::set<ULONG> setRequestedTags;
		SPropTagArrayPtr ptrRequiredTags;
		SPropTagArrayPtr ptrCombinedTags;

		ASSERT(m_lpsPropTags != NULL);
		ASSERT(m_ptrPropertyPool != NULL);
		ASSERT(lppCombinedTags != NULL);

		hr = m_ptrPropertyPool->GetRequiredPropTags(NULL, &ptrRequiredTags);
		if (hr != hrSuccess)
			goto exit;

		setRequestedTags.insert(m_lpsPropTags->aulPropTag, m_lpsPropTags->aulPropTag + m_lpsPropTags->cValues);

		// Reserve enough space to hold the requested and internal tags
		hr = MAPIAllocateBuffer(CbNewSPropTagArray(m_lpsPropTags->cValues + 1 + ptrRequiredTags->cValues), &ptrCombinedTags);
		if (hr != hrSuccess)
			goto exit;

		// Copy all requested proptags
		memcpy(ptrCombinedTags->aulPropTag, m_lpsPropTags->aulPropTag, m_lpsPropTags->cValues * sizeof(ULONG));
		ptrCombinedTags->cValues = m_lpsPropTags->cValues;

		// Reserve space for PR_ROWID
		ptrCombinedTags->aulPropTag[ptrCombinedTags->cValues++] = PT_NULL;

		// Append internal tags only if they're not part of the requested tags
		for (ULONG i = 0; i < ptrRequiredTags->cValues; ++i)
			if (setRequestedTags.find(ptrRequiredTags->aulPropTag[i]) == setRequestedTags.end())
				ptrCombinedTags->aulPropTag[ptrCombinedTags->cValues++] = ptrRequiredTags->aulPropTag[i];

		*lppCombinedTags = ptrCombinedTags.release();

	exit:
		return hr;
	}

	/**
	 * Create a new SRow that contains the first N properties of another SRow.
	 * 
	 * @param[in]	lpSrc		The SRow from which the propvals are to be copied.
	 * @param[in]	cValues		The number of propvals to copy.
	 * @param[out]	lppDest		The new SRow. This row must be deleted by the caller.
	 */
	static HRESULT CopyPartialSRow(SRow *lpSrc, ULONG cValues, SRow **lppDest)
	{
		HRESULT hr = hrSuccess;
		SRow sShadowRow;
		SRowPtr ptrNewRow;

		ASSERT(lpSrc != NULL);
		ASSERT(cValues <= lpSrc->cValues);
		ASSERT(lppDest != NULL);

		sShadowRow = *lpSrc;
		sShadowRow.cValues = cValues;

		hr = MAPIAllocateBuffer(sizeof(SRow), &ptrNewRow);
		if (hr != hrSuccess)
			goto exit;

		hr = Util::HrCopySRow(ptrNewRow, &sShadowRow, ptrNewRow);
		if (hr != hrSuccess)
			goto exit;

		*lppDest = ptrNewRow.release();

	exit:
		return hr;
	}

	/**
	 * Update some properties based on the information from an occurrence.
	 * 
	 * This is done because the row originates from a regular table that contains
	 * information from the master appointment. So for each occurrence some data
	 * must be updated in order for the expansion to make any sense.
	 * 
	 * @param[in,out]	lpRow			The row to update.
	 * @param[in]		lpAppointment	The master appointment.
	 * @param[in]		ulBaseDate		The basedate of the occurrence to base the updates on.
	 */
	HRESULT UpdateRowForOccurrence(SRow *lpRow, IAppointment *lpAppointment, ULONG ulBaseDate)
	{
		HRESULT hr = hrSuccess;
		OccurrencePtr ptrOccurrence;
		TimezoneDefinitionPtr ptrTZDefRecur;

		ASSERT(lpRow != NULL);
		ASSERT(lpAppointment != NULL);
		ASSERT(m_ptrPropertyPool != NULL);

		hr = lpAppointment->GetOccurrence(ulBaseDate, &ptrOccurrence);
		if (hr != hrSuccess)
			goto exit;

		hr = lpAppointment->GetRecurrenceTimezone(&ptrTZDefRecur);
		if (hr != hrSuccess)
			goto exit;

		for (ULONG i = 0; i < m_lpsPropTags->cValues; ++i) {
			if (m_lpsPropTags->aulPropTag[i] == PROP(APPTSTARTWHOLE) ||
				m_lpsPropTags->aulPropTag[i] == PROP(COMMONSTART) ||
				m_lpsPropTags->aulPropTag[i] == PR_START_DATE)
			{
				// Update start datetime
				hr = ptrOccurrence->GetStartDateTime(NULL, &lpRow->lpProps[i].Value.ft);
				if (hr != hrSuccess)
					goto exit;

				ASSERT(PROP_ID(m_lpsPropTags->aulPropTag[i]) == PROP_ID(lpRow->lpProps[i].ulPropTag));
				lpRow->lpProps[i].ulPropTag = m_lpsPropTags->aulPropTag[i];
			}

			else if (m_lpsPropTags->aulPropTag[i] == PROP(APPTENDWHOLE) ||
					 m_lpsPropTags->aulPropTag[i] == PROP(COMMONEND) ||
					 m_lpsPropTags->aulPropTag[i] == PR_END_DATE)
			{
				// Update end datetime
				hr = ptrOccurrence->GetEndDateTime(NULL, &lpRow->lpProps[i].Value.ft);
				if (hr != hrSuccess)
					goto exit;
				ASSERT(PROP_ID(m_lpsPropTags->aulPropTag[i]) == PROP_ID(lpRow->lpProps[i].ulPropTag));
				lpRow->lpProps[i].ulPropTag = m_lpsPropTags->aulPropTag[i];
			}

			else if (m_lpsPropTags->aulPropTag[i] == PR_SUBJECT_W) {
				// Update subject
				hr = ptrOccurrence->GetSubject((LPTSTR*)&lpRow->lpProps[i].Value.lpszW, lpRow, MAPI_UNICODE);
				if (hr != hrSuccess)
					goto exit;

				ASSERT(PROP_ID(m_lpsPropTags->aulPropTag[i]) == PROP_ID(lpRow->lpProps[i].ulPropTag));
				lpRow->lpProps[i].ulPropTag = m_lpsPropTags->aulPropTag[i];
			}

			else if (m_lpsPropTags->aulPropTag[i] == PR_SUBJECT_A) {
				// Update subject
				hr = ptrOccurrence->GetSubject((LPTSTR*)&lpRow->lpProps[i].Value.lpszA, lpRow, 0);
				if (hr != hrSuccess)
					goto exit;

				ASSERT(PROP_ID(m_lpsPropTags->aulPropTag[i]) == PROP_ID(lpRow->lpProps[i].ulPropTag));
				lpRow->lpProps[i].ulPropTag = m_lpsPropTags->aulPropTag[i];
			}

			else if (m_lpsPropTags->aulPropTag[i] == PROP(APPTDURATION)) {
				// Update duration
				hr = ptrOccurrence->GetDuration(&lpRow->lpProps[i].Value.ul);
				if (hr != hrSuccess)
					goto exit;

				ASSERT(PROP_ID(m_lpsPropTags->aulPropTag[i]) == PROP_ID(lpRow->lpProps[i].ulPropTag));
				lpRow->lpProps[i].ulPropTag = m_lpsPropTags->aulPropTag[i];
			}

			else if (m_lpsPropTags->aulPropTag[i] == PROP(BUSYSTATUS)) {
				// Update busy status
				hr = ptrOccurrence->GetBusyStatus(&lpRow->lpProps[i].Value.ul);
				if (hr != hrSuccess)
					goto exit;

				ASSERT(PROP_ID(m_lpsPropTags->aulPropTag[i]) == PROP_ID(lpRow->lpProps[i].ulPropTag));
				lpRow->lpProps[i].ulPropTag = m_lpsPropTags->aulPropTag[i];
			}

			else if (m_lpsPropTags->aulPropTag[i] == CHANGE_PROP_TYPE(PROP(LOCATION), PT_UNICODE)) {
				// Update location
				hr = ptrOccurrence->GetLocation((LPTSTR*)&lpRow->lpProps[i].Value.lpszW, lpRow, MAPI_UNICODE);
				if (hr != hrSuccess)
					goto exit;

				ASSERT(PROP_ID(m_lpsPropTags->aulPropTag[i]) == PROP_ID(lpRow->lpProps[i].ulPropTag));
				lpRow->lpProps[i].ulPropTag = m_lpsPropTags->aulPropTag[i];
			}

			else if (m_lpsPropTags->aulPropTag[i] == CHANGE_PROP_TYPE(PROP(LOCATION), PT_STRING8)) {
				// Update location
				hr = ptrOccurrence->GetLocation((LPTSTR*)&lpRow->lpProps[i].Value.lpszA, lpRow, 0);
				if (hr != hrSuccess)
					goto exit;

				ASSERT(PROP_ID(m_lpsPropTags->aulPropTag[i]) == PROP_ID(lpRow->lpProps[i].ulPropTag));
				lpRow->lpProps[i].ulPropTag = m_lpsPropTags->aulPropTag[i];
			}

			else if (m_lpsPropTags->aulPropTag[i] == PROP(MEETINGSTATUS)) {
				// Update meetingstatus
				hr = ptrOccurrence->GetMeetingType(&lpRow->lpProps[i].Value.ul);
				if (hr != hrSuccess)
					goto exit;

				ASSERT(PROP_ID(m_lpsPropTags->aulPropTag[i]) == PROP_ID(lpRow->lpProps[i].ulPropTag));
				lpRow->lpProps[i].ulPropTag = m_lpsPropTags->aulPropTag[i];
			}

			else if (m_lpsPropTags->aulPropTag[i] == PROP(ALLDAYEVENT)) {
				// Update alldayevent
				hr = ptrOccurrence->GetSubType(&lpRow->lpProps[i].Value.b);
				if (hr != hrSuccess)
					goto exit;

				ASSERT(PROP_ID(m_lpsPropTags->aulPropTag[i]) == PROP_ID(lpRow->lpProps[i].ulPropTag));
				lpRow->lpProps[i].ulPropTag = m_lpsPropTags->aulPropTag[i];
			}

			else if (m_lpsPropTags->aulPropTag[i] == PR_HASATTACH) {
				// Update hasattach
				hr = ptrOccurrence->GetHasAttach(&lpRow->lpProps[i].Value.b);
				if (hr != hrSuccess)
					goto exit;

				ASSERT(PROP_ID(m_lpsPropTags->aulPropTag[i]) == PROP_ID(lpRow->lpProps[i].ulPropTag));
				lpRow->lpProps[i].ulPropTag = m_lpsPropTags->aulPropTag[i];
			}

			else if (m_lpsPropTags->aulPropTag[i] == PROP(REMINDERMINUTESBEFORESTART)) {
				// Update reminderminutesbeforestart
				hr = ptrOccurrence->GetReminderDelta(&lpRow->lpProps[i].Value.ul);
				if (hr != hrSuccess)
					goto exit;

				ASSERT(PROP_ID(m_lpsPropTags->aulPropTag[i]) == PROP_ID(lpRow->lpProps[i].ulPropTag));
				lpRow->lpProps[i].ulPropTag = m_lpsPropTags->aulPropTag[i];
			}

			else if (m_lpsPropTags->aulPropTag[i] == PROP(REMINDERSET)) {
				// Update reminderset
				hr = ptrOccurrence->GetReminderSet(&lpRow->lpProps[i].Value.b);
				if (hr != hrSuccess)
					goto exit;

				ASSERT(PROP_ID(m_lpsPropTags->aulPropTag[i]) == PROP_ID(lpRow->lpProps[i].ulPropTag));
				lpRow->lpProps[i].ulPropTag = m_lpsPropTags->aulPropTag[i];
			}

			else if (m_lpsPropTags->aulPropTag[i] == PROP(ISEXCEPTION)) {
				// Update isexception
				hr = ptrOccurrence->GetIsException(&lpRow->lpProps[i].Value.b);
				if (hr != hrSuccess)
					goto exit;

				ASSERT(PROP_ID(m_lpsPropTags->aulPropTag[i]) == PROP_ID(lpRow->lpProps[i].ulPropTag));
				lpRow->lpProps[i].ulPropTag = m_lpsPropTags->aulPropTag[i];
			}

			else if (m_lpsPropTags->aulPropTag[i] == PROP(OLDSTART) ||
					 m_lpsPropTags->aulPropTag[i] == PROP(RECURRINGBASE))
			{
				hr = ptrOccurrence->GetOriginalStartDateTime(NULL, &lpRow->lpProps[i].Value.ft);
				if (hr != hrSuccess)
					goto exit;

				ASSERT(PROP_ID(m_lpsPropTags->aulPropTag[i]) == PROP_ID(lpRow->lpProps[i].ulPropTag));
				lpRow->lpProps[i].ulPropTag = m_lpsPropTags->aulPropTag[i];
			}
		}

	exit:
		return hr;
	}

	/**
	 * Add a row to the MemTable.
	 * 
	 * @param[in]	lpRow	The row to add
	 */
	HRESULT AddRow(SRow *lpRow)
	{
		ASSERT(lpRow != NULL);
		ASSERT(lpRow->cValues >= m_lpsPropTags->cValues + 1);
		ASSERT(lpRow->lpProps[m_lpsPropTags->cValues].ulPropTag == PT_NULL);
		ASSERT(m_ptrMemTable != NULL);

		lpRow->lpProps[m_lpsPropTags->cValues].ulPropTag = PR_ROWID;
		lpRow->lpProps[m_lpsPropTags->cValues].Value.l = m_ulRowCount++;

		return m_ptrMemTable->HrModifyRow(ECKeyTable::TABLE_ROW_ADD, NULL, lpRow->lpProps, m_lpsPropTags->cValues + 1);
	}

	/**
	 * Add some or all occurrences of a recurring appointment to the MemTable based
	 * on the row of the master appointment.
	 * 
	 * @param[in]	lpRow	The row containing the properties of the master
	 * 						recurring appointment.
	 */
	HRESULT AddRecurringAppointment(SRow *lpRow)
	{
		HRESULT hr = hrSuccess;
		AppointmentPtr ptrAppointment;
		RecurrencePatternPtr ptrPattern;
		ULONG cModified = 0;
		UlongPtr ptrModified;
		ULONG cDeleted = 0;
		UlongPtr ptrDeleted;
		ULONG cBaseDates = 0;
		UlongPtr ptrBaseDates;
		ULONG ulStartDate = 0;
		ULONG ulEndDate = 0;

		ASSERT(lpRow != NULL);
		ASSERT(m_ptrPropertyPool != NULL);
		ASSERT(m_ptrMemTable != NULL);

		hr = BaseDateFromFileTime(m_ftStartDateTime, &ulStartDate);
		if (hr != hrSuccess)
			goto exit;

		hr = BaseDateFromFileTime(m_ftEndDateTime, &ulEndDate);
		if (hr != hrSuccess)
			goto exit;

		hr = Appointment::Create(lpRow->cValues, lpRow->lpProps, m_ptrPropertyPool, m_ptrClientTZ, &ptrAppointment);
		if (hr != hrSuccess)
			goto exit;

		hr = ptrAppointment->GetExceptions(&cModified, &ptrModified, &cDeleted, &ptrDeleted);
		if (hr != hrSuccess)
			goto exit;

		// Check that the deleted and modified appointment exceptions are ordered.
		for (ULONG i = 1; i < cModified; ++i)
			ASSERT(ptrModified[i] > ptrModified[i-1]);
		for (ULONG i = 1; i < cDeleted; ++i)
			ASSERT(ptrDeleted[i] > ptrDeleted[i-1]);

		hr = ptrAppointment->GetRecurrence(&ptrPattern);
		if (hr != hrSuccess)
			goto exit;

		hr = ptrPattern->GetOccurrencesInRange(ulStartDate, ulEndDate, &cBaseDates, &ptrBaseDates);
		if (hr != hrSuccess)
			goto exit;

		for (ULONG i = 0; i < cBaseDates; ++i) {
			SRowPtr ptrNewRow;

			if (std::binary_search(&ptrDeleted[0], &ptrDeleted[cDeleted], ptrBaseDates[i]))
				continue;

			hr = CopyPartialSRow(lpRow, m_lpsPropTags->cValues + 1, &ptrNewRow);
			if (hr != hrSuccess)
				goto exit;

			hr = UpdateRowForOccurrence(ptrNewRow, ptrAppointment, ptrBaseDates[i]);
			if (hr != hrSuccess)
				goto exit;

			// We don't need to recheck each occurrence here, but we do need to check each exception and the
			// first and last occurrence if they occur on the first or last day of the requested range because
			// they might occur before or after the time specified in the range.
			// For clarity it's easier to just test all occurrences.
			if (TestRestriction(m_ptrRestriction, ptrNewRow->cValues, ptrNewRow->lpProps, ECLocale()) != hrSuccess)
				continue;

			hr = AddRow(ptrNewRow);
			if (hr != hrSuccess)
				goto exit;
		}

	exit:
		return hr;
	}


public:
	/**
	 * Construct a CalendarTableBuilder
	 * 
	 * @param[in]	lpCalendar			The IMAPIFolder that represents the calendar.
	 * @param[in]	ftStartDateTime		The start timestamp of the range to be expanded.
	 * @param[in]	ftEndDateTime		The end timestamp of the tange to be expanded.
	 * @param[in]	lpClientTZ			The timezone of the client.
	 * @param[in]	lpsPropTags			A proptag array containing the property tags of the properties
	 * 									that should be present in the resulting table.
	 */
	CalendarTableBuilder(IMAPIFolder *lpCalendar, FILETIME ftStartDateTime, FILETIME ftEndDateTime, TimezoneDefinition *lpClientTZ, LPSPropTagArray lpsPropTags)
	: m_lpCalendar(lpCalendar)
	, m_ftStartDateTime(ftStartDateTime)
	, m_ftEndDateTime(ftEndDateTime)
	, m_ptrClientTZ(lpClientTZ)
	, m_lpsPropTags(lpsPropTags)
	, m_ulRowCount(0)
	{ }

	/**
	 * Build the actual table.
	 * 
	 * @param[in]	ulFlags				Flag. Can be 0 or MAPI_UNICODE to specify if the resulting table
	 * 									should contain strings represented in UNICODE.
	 * @param[out]	lppTable			The IMAPITable that contains the expanded calendar. This table
	 * 									must be released by the caller.
	 */
	HRESULT BuildTable(ULONG ulFlags, IMAPITable **lppTable)
	{
		HRESULT hr = hrSuccess;
		MAPITablePtr ptrContentTable;
		ECMemTableViewPtr ptrMemTableView;
		SPropTagArrayPtr ptrPropTagArray;

		if (m_lpCalendar == NULL || m_lpsPropTags == NULL || lppTable == NULL) {
			hr = MAPI_E_INVALID_PARAMETER;
			goto exit;
		}

		hr = PropertyPool::Create(m_lpCalendar, &m_ptrPropertyPool);
		if (hr != hrSuccess)
			goto exit;

		hr = CreateRestriction();
		if (hr != hrSuccess)
			goto exit;

		hr = m_lpCalendar->GetContentsTable(0, &ptrContentTable);
		if (hr != hrSuccess)
			goto exit;

		hr = ptrContentTable->Restrict(m_ptrRestriction, TBL_BATCH);
		if (hr != hrSuccess)
			goto exit;

		hr = CreatePropTagArray(&ptrPropTagArray);
		if (hr != hrSuccess)
			goto exit;

		hr = ptrContentTable->SetColumns(ptrPropTagArray, TBL_BATCH);
		if (hr != hrSuccess)
			goto exit;

		hr = ECMemTable::Create(m_lpsPropTags, PR_ROWID, &m_ptrMemTable);
		if (hr != hrSuccess)
			goto exit;

		while (true) {
			SRowSetPtr ptrRows;

			hr = ptrContentTable->QueryRows(100, 0, &ptrRows);
			if (hr != hrSuccess)
				goto exit;

			if (ptrRows.empty())
				break;

			for (SRowSetPtr::size_type i = 0; i < ptrRows.size(); ++i) {
				LPSPropValue lpPropRecurring = PpropFindProp(ptrRows[i].lpProps, ptrRows[i].cValues, PROP(RECURRING));
				if (lpPropRecurring == NULL || lpPropRecurring->Value.b == FALSE) {
					hr = AddRow(&ptrRows[i]);
					if (hr != hrSuccess)
						goto exit;
				} else {
					hr = AddRecurringAppointment(&ptrRows[i]);
					if (hr != hrSuccess)
						goto exit;
				}
			}
		}

		hr = m_ptrMemTable->HrGetView(ECLocale(), ulFlags & MAPI_UNICODE, &ptrMemTableView);
		if (hr != hrSuccess)
			goto exit;

		hr = ptrMemTableView->SetColumns(m_lpsPropTags, TBL_BATCH);
		if (hr != hrSuccess)
			goto exit;

		hr = ptrMemTableView->QueryInterface(IID_IMAPITable, (LPVOID*)lppTable);

	exit:
		return hr;
	}
};


/**
 * Get an IMAPITable instance containing an expanded view of the calendar. This means all recurring
 * items in the specified range will be expanded and represented as multiple single occurrences.
 * 
 * @param[in]	lpCalendar			The IMAPIFolder that represents the calendar.
 * @param[in]	ftStartDateTime		The start timestamp (in UTC) of the range to be expanded.
 * @param[in]	ftEndDateTime		The end timestamp (in UTC) of the tange to be expanded.
 * @param[in]	lpClientTZ			The timezone of the client.
 * @param[in]	lpsPropTags			A proptag array containing the property tags of the properties
 * 									that should be present in the resulting table.
 * @param[in]	ulFlags				Flag. Can be 0 or MAPI_UNICODE to specify if the resulting table
 * 									should contain strings represented in UNICODE.
 * @param[out]	lppTable			The IMAPITable that contains the expanded calendar. This table
 * 									must be released by the caller.
 */
HRESULT GetExpandedCalendarTable(IMAPIFolder *lpCalendar, FILETIME ftStartDateTime, FILETIME ftEndDateTime, TimezoneDefinition *lpClientTZ, LPSPropTagArray lpsPropTags, ULONG ulFlags, IMAPITable **lppTable)
{
	return CalendarTableBuilder(lpCalendar, ftStartDateTime, ftEndDateTime, lpClientTZ, lpsPropTags).BuildTable(ulFlags, lppTable);
}
