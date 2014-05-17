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

#ifndef RecurrencePattern_INCLUDED
#define RecurrencePattern_INCLUDED

#include "ECUnknown.h"
#include "RecurrenceState.h"

/**
 * Interface for receiving details from an RecurrencePattern instance.
 */
class IRecurrencePatternInspector : public IUnknown
{
public:
	// Pattern
	/**
	 * Set a daily pattern.
	 * 
	 * @param[in]	ulDailyPeriod	The period in days.
	 */
	virtual HRESULT SetPatternDaily(ULONG ulDailyPeriod) = 0;
	
	/**
	 * Set a pattern that has an occurrence on each workday.
	 */
	virtual HRESULT SetPatternWorkDays() = 0;
	
	/**
	 * Set a pattern that occurs every N weeks.
	 * 
	 * @param[in]	ulFirstDayOfWeek	Specifies which day is regarded as the first day
	 * 									of the week.
	 * @param[in]	ulWeeklyPeriod		Specifies the period in weeks.
	 * @param[in]	ulDayOfWeekMask		A bitmask specifying on which days of the week
	 * 									occurrences occur.
	 */
	virtual HRESULT SetPatternWeekly(ULONG ulFirstDayOfWeek, ULONG ulWeeklyPeriod, ULONG ulDayOfWeekMask) = 0;
	
	/**
	 * Set a pattern that has an occurrence every N months at a specific day.
	 * 
	 * @param[in]	ulMonthlyPeriod		Specifies the period in months.
	 * @param[in]	ulDayOfMonth		The day of the month on which the occurrence occurs. If
	 * 									this value is greater than the amount of days in a month,
	 * 									the last day of the month will be used.
	 */
	virtual HRESULT SetPatternAbsoluteMonthly(ULONG ulMonthlyPeriod, ULONG ulDayOfMonth) = 0;
	
	/**
	 * Set a pattern that has one or more occurrences in a specefic week every N months.
	 * 
	 * @param[in]	ulMonthlyPeriod		Specifies the period in months.
	 * @param[in]	ulNum				Specifies the week number in the month. 1 to 4 specify 
	 * 									an exact week number. 5 specifies the last week.
	 * @param[in]	ulDayOfWeekMask		A bitmask specifying on which days of the week occurrences
	 * 									occur. Although all possible combinations are possible and
	 * 									allowed by the library, MAPI only allows single days or
	 * 									0x3e for weekdays and 0x41 for weekenddays.
	 */
	virtual HRESULT SetPatternRelativeMonthly(ULONG ulMonthlyPeriod, ULONG ulNum, ULONG ulDayOfWeekMask) = 0;
	
	/**
	 * Set a pattern that has an occurrence every N years on a specific month and day.
	 * 
	 * @param[in]	ulYearlyPeriod		Specifies the period in years.
	 * @param[in]	ulMonth				Specifies the month.
	 * @param[in]	ulDayOfMonth		The day of the month on which the occurrence occurs. If
	 * 									this value is greater than the amount of days in a month,
	 * 									the last day of the month will be used.
	 */
	virtual HRESULT SetPatternAbsoluteYearly(ULONG ulYearlyPeriod, ULONG ulMonth, ULONG ulDayOfMonth) = 0;
	
	/**
	 * Set a pattern that has one or more occurrences in a specific month and week every N years.
	 * 
	 * @param[in]	ulYearlyPeriod		Specifies the period in years.
	 * @param[in]	ulMonth				Specifies the month.
	 * @param[in]	ulNum				Specifies the week number in the month. 1 to 4 specify 
	 * 									an exact week number. 5 specifies the last week.
	 * @param[in]	ulDayOfWeekMask		A bitmask specifying on which days of the week occurrences
	 * 									occur. Although all possible combinations are possible and
	 * 									allowed by the library, MAPI only allows single days or
	 * 									0x3e for weekdays and 0x41 for weekenddays.
	 */
	virtual HRESULT SetPatternRelativeYearly(ULONG ulYearlyPeriod, ULONG ulMonth, ULONG ulNum, ULONG ulDayOfWeekMask) = 0;

	// Range
	/**
	 * Set a range that has no ending.
	 * 
	 * @param[in]	ulStartDate		The start date of the range.
	 */
	virtual HRESULT SetRangeNoEnd(ULONG ulStartDate) = 0;
	
	/**
	 * Set a range that ends after N occurrences.
	 * 
	 * @param[in]	ulStartDate		The start date of the range.
	 * @param[in]	ulOccurrences	The number of occurrences in the range.
	 */
	virtual HRESULT SetRangeNumbered(ULONG ulStartDate, ULONG ulOccurrences) = 0;
	
	/**
	 * Set a range that ends after a specific date.
	 * 
	 * @param[in]	ulStartDate		The start date of the range.
	 * @param[in]	ulOccurrences	The end date of the range.
	 */
	virtual HRESULT SetRangeEndDate(ULONG ulStartDate, ULONG ulEndDate) = 0;
};


/**
 * This class contains details about a recurring appointment.
 * The details are:
 * - The pattern
 * - The end criterium
 * - The first day of week
 *
 * A client can set and get the details on an ECrecurrencePattern
 * instance and pass it to an Appointment in order for the pattern
 * to be actually applied.
 *
 * Getting the details is a bit cumbersome because to call the correct
 * GetSomePattern method one first need to know the pattern type.
 *
 * Example (no error checking):
 * RecurrencePattern *lpPattern = GetPatternSomehow();
 *
 * lpPattern->GetPatternType(&ulType);
 * switch (ulType):
 * case ptDaily:
 *   lpPattern->GetDaily(&ulDailyPeriod);
 * case ...
 *
 * After doing this the result of these calls will usualy be used
 * to take an action that is specific to a particular pattern type.
 *
 * To simplify this, a client can also use the Inspect method. This
 * will cause the instance to call back into a client specified object
 * with the details. This way a client doesn't have to write the switch
 * code each time a recurrence pattern is examined.
 *
 * Example (no error checking):
 * class MyInspector : public IRecurrencePatternInspector
 * {
 * public:
 *     HRESULT SetDaily(ULONG ulDailyPeriod);
 *     ...
 *
 *     HRESULT DoSomethingUseful();
 * };
 *
 * RecurrencePattern *lpPattern = GetPatternSomehow();
 * MyInspector inspector();
 *
 * lpPatter->Inspect(&inspector);
 * inspector.DoSomethingUseful();
 */
class RecurrencePattern : public ECUnknown
{
public:
	static HRESULT Create(RecurrencePattern **lppPattern);
	static HRESULT Create(RecurrenceState *lpRecurState, RecurrencePattern **lppPattern);

	// Setting options
	// Pattern
	HRESULT SetPatternDaily(ULONG ulDailyPeriod);
	HRESULT SetPatternWorkDays();
	HRESULT SetPatternWeekly(ULONG ulFirstDayOfWeek, ULONG ulWeeklyPeriod, ULONG ulDayOfWeekMask);
	HRESULT SetPatternAbsoluteMonthly(ULONG ulMonthlyPeriod, ULONG ulDayOfMonth);
	HRESULT SetPatternRelativeMonthly(ULONG ulMonthlyPeriod, ULONG ulNum, ULONG ulDayOfWeekMask);
	HRESULT SetPatternAbsoluteYearly(ULONG ulYearlyPeriod, ULONG ulMonth, ULONG ulDayOfMonth);
	HRESULT SetPatternRelativeYearly(ULONG ulYearlyPeriod, ULONG ulMonth, ULONG ulNum, ULONG ulDayOfWeekMask);

	// Range
	HRESULT SetRangeNoEnd(ULONG ulStartDate);
	HRESULT SetRangeNumbered(ULONG ulStartDate, ULONG ulOccurrences);
	HRESULT SetRangeEndDate(ULONG ulStartDate, ULONG ulEndDate);


	// Getting options
	// Pattern
	HRESULT GetPatternType(ULONG *lpulType);
	HRESULT GetPatternDaily(ULONG *lpulDailyPeriod);
	HRESULT GetPatternWorkDays();
	HRESULT GetPatternWeekly(ULONG *lpulFirstDayOfWeek, ULONG *lpulWeeklyPeriod, ULONG *lpulDayOfWeekMask);
	HRESULT GetPatternAbsoluteMonthly(ULONG *lpulMonthlyPeriod, ULONG *lpulDayOfMonth);
	HRESULT GetPatternRelativeMonthly(ULONG *lpulMonthlyPeriod, ULONG *lpulNum, ULONG *lpulDayOfWeekMask);
	HRESULT GetPatternAbsoluteYearly(ULONG *lpulYearlyPeriod, ULONG *lpulMonth, ULONG *lpulDayOfMonth);
	HRESULT GetPatternRelativeYearly(ULONG *lpulYearlyPeriod, ULONG *lpulMonth, ULONG *lpulNum, ULONG *lpulDayOfWeekMask);

	// Range
	HRESULT GetRangeType(ULONG *lpulType);
	HRESULT GetRangeNoEnd(ULONG *lpulStartDate);
	HRESULT GetRangeNumbered(ULONG *lpulStartDate, ULONG *lpulOccurrences);
	HRESULT GetRangeEndDate(ULONG *lpulStartDate, ULONG *lpulEndDate);


	// Inspect
	HRESULT Inspect(IRecurrencePatternInspector *lpInspector);
	HRESULT Clone(RecurrencePattern **lppPattern);
	HRESULT UpdateState(RecurrenceState *lpRecurState);

	// Useful
	HRESULT IsOccurrence(ULONG ulBaseDate, bool *lpbIsOccurrence);
	HRESULT GetOccurrence(ULONG ulBaseDate, ULONG *lpulBaseDate);
	HRESULT GetOccurrencesInRange(ULONG ulStartDate, ULONG ulEndDate, ULONG *lpcbBaseDates, ULONG **lpaBaseDates);
	HRESULT GetBounds(ULONG *lpulFirst, ULONG *lpulLast);

public: // But for internal use
	HRESULT CalcDailyFirstDateTime(ULONG *lpulFirstDateTime) const;
	HRESULT CalcWeeklyFirstDateTime(ULONG *lpulFirstDateTime) const;
	HRESULT CalcMonthlyFirstDateTime(ULONG *lpulFirstDateTime) const;

protected:
	RecurrencePattern();
	~RecurrencePattern();

private:
	HRESULT GetFirstDateTime(ULONG *lpulFirstDateTime);

	// Daily helpers
	HRESULT IsOccurrenceDaily(ULONG ulBaseDate, bool *lpbIsOccurrence);
	HRESULT GetOccurrenceDaily(ULONG ulBaseDate, ULONG *lpulBaseDate);
	HRESULT GetOccurrencesInRangeDaily(ULONG ulStartDate, ULONG ulEndDate, ULONG *lpcbBaseDates, ULONG **lpaBaseDates);
	HRESULT GetLastOccurrenceDaily(ULONG *lpulEndDate);

	// Weekly helpers
	HRESULT IsOccurrenceWeekly(ULONG ulBaseDate, bool *lpbIsOccurrence);
	HRESULT GetOccurrenceWeekly(ULONG ulBaseDate, ULONG *lpulBaseDate);
	HRESULT GetOccurrencesInRangeWeekly(ULONG ulStartDate, ULONG ulEndDate, ULONG *lpcbBaseDates, ULONG **lpaBaseDates);
	HRESULT GetLastOccurrenceWeekly(ULONG *lpulEndDate);

	// Monthly helpers
	static ULONG GetMonthlyOffset(ULONG ulYear, ULONG ulMonth, ULONG ulPeriod);
	static ULONG GetMonthlyOffset(ULONG ulBaseDate, ULONG ulPeriod);
	HRESULT IsOccurrenceMonthly(ULONG ulBaseDate, bool *lpbIsOccurrence);
	HRESULT GetOccurrenceMonthly(ULONG ulBaseDate, ULONG *lpulBaseDate);
	HRESULT GetOccurrenceMonthly(ULONG ulBaseDate, ULONG ulFlags, ULONG *lpulBaseDate);
	HRESULT GetOccurrencesInRangeMonthly(ULONG ulStartDate, ULONG ulEndDate, ULONG *lpcbBaseDates, ULONG **lpaBaseDates);
	HRESULT GetLastOccurrenceMonthly(ULONG *lpulEndDate);

	// Monthly Nth helpers
	HRESULT IsOccurrenceMonthlyNth(ULONG ulBaseDate, bool *lpbIsOccurrence);
	HRESULT GetOccurrenceMonthlyNth(ULONG ulBaseDate, ULONG *lpulBaseDate);
	HRESULT GetOccurrenceMonthlyNth(ULONG ulBaseDate, ULONG ulFlags, ULONG *lpulBaseDate);
	HRESULT GetOccurrencesInRangeMonthlyNth(ULONG ulStartDate, ULONG ulEndDate, ULONG *lpcbBaseDates, ULONG **lpaBaseDates);
	HRESULT GetLastOccurrenceMonthlyNth(ULONG *lpulEndDate);


private:
	ULONG m_ulRecurFreq;
	ULONG m_ulPatternType;
	ULONG m_ulFirstDOW;
	ULONG m_ulPeriod;
	ULONG m_ulWeekDays;
	ULONG m_ulNum;
	ULONG m_ulMonth;

	ULONG m_ulStartDate;
	ULONG m_ulEndType;
	ULONG m_ulEndDate;
	ULONG m_ulOccurrenceCount;

	ULONG m_ulFirstDateTime;
};

#endif // ndef RecurrencePattern_INCLUDED
