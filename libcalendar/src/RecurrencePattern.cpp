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
#include "RecurrencePattern.h"
#include "RecurrenceState.h"
#include "CommonUtil.h"
#include "Conversions.h"
#include "ConversionsInternal.h"

#include "mapi_ptr.h"
#include "Trace.h"
#include "ECInterfaceDefs.h"

#include <mapix.h>
#include <mapicode.h>

#include <boost/date_time/gregorian/gregorian.hpp>
namespace bg = boost::gregorian;

typedef mapi_object_ptr<RecurrencePattern> RecurrencePatternPtr;
typedef mapi_array_ptr<ULONG> UlongPtr;

typedef mapi_object_ptr<IRecurrencePatternInspector, IID_IRecurrencePatternInspector> RecurrencePatternInspectorPtr;
//DEFINEMAPIPTR(RecurrencePatternInspector);

#define NO_END_CHECK	0x00000001
#define ULONG_UNSET		((ULONG)-1)

#define UNSET(var) ((var) = ULONG_UNSET)
#define ISSET(var) ((var) != ULONG_UNSET)

namespace validate {

	/**
	 * Check if a ninteger value represents a valid weekday.
	 *
	 * @param[in]	ulWeekday	The weekday number to validate
	 * @return true is valid, else false.
	 */
	bool Weekday(ULONG ulWeekday) {
		switch (ulWeekday) {
			case DOW_SUNDAY:
			case DOW_MONDAY:
			case DOW_TUESDAY:
			case DOW_WEDNESDAY:
			case DOW_THURSDAY:
			case DOW_FRIDAY:
			case DOW_SATURDAY:
				return true;
			default:
				break;
		}
		return false;
	}

	/**
	 * Check if an integer value represents a valid weekday mask.
	 *
	 * @param[in]	ulWeekdayMask	The weekday mask to validate.
	 * @return true if valid, else false
	 */
	bool Weekdays(ULONG ulWeekdayMask) {
		return (ulWeekdayMask & ~WD_MASK) == 0;
	}

} // namespace validate

namespace util {

	/**
	 * Check if a boost weekday exists in a integer value representing a bitmask
	 * of weekdays (the MAPI way).
	 *
	 * @param[in]	weekday			The boost weekday to test.
	 * @param[in]	ulWeekdayMask	The weekday mask test against.
	 * @return true is weekday exists in ulWeekdayMask, else false
	 */
	static inline bool BoostWeekDayInMask(const bg::greg_weekday& weekday, ULONG ulWeekdayMask) {
		return ((weekday == bg::Sunday) && (ulWeekdayMask & WD_SUNDAY) == WD_SUNDAY) ||
			   ((weekday == bg::Monday) && (ulWeekdayMask & WD_MONDAY) == WD_MONDAY) ||
			   ((weekday == bg::Tuesday) && (ulWeekdayMask & WD_TUESDAY) == WD_TUESDAY) ||
			   ((weekday == bg::Wednesday) && (ulWeekdayMask & WD_WEDNESDAY) == WD_WEDNESDAY) ||
			   ((weekday == bg::Thursday) && (ulWeekdayMask & WD_THURSDAY) == WD_THURSDAY) ||
			   ((weekday == bg::Friday) && (ulWeekdayMask & WD_FRIDAY) == WD_FRIDAY) ||
			   ((weekday == bg::Saturday) && (ulWeekdayMask & WD_SATURDAY) == WD_SATURDAY);
	}

	/**
	 * Get the number of set days in the bitmask with weekdays.
	 *
	 * @param[in]	ulWeekdayMask	The weekday mask to process.
	 * @return the count of set weekdays.
	 */
	static inline ULONG GetDaysInWeek(ULONG ulWeekdayMask) {
		ULONG ulDaysInWeek = 0;
		for (ULONG d = 1; d != (1 << 7); d <<= 1)
			if (ulWeekdayMask & d)
				ulDaysInWeek++;
		return ulDaysInWeek;
	}

} // namespace util


/**
 * This class updates a RecurrenceState struct based on information obtained
 * by inspecting a RecurrencePattern instance.
 */
class PatternToState : public ECUnknown
{
public:
	static HRESULT CreateInspector(RecurrencePattern *lpPattern, RecurrenceState &refRecurrenceState, IRecurrencePatternInspector **lppInspector);

	// From IUnknown
	virtual HRESULT	QueryInterface(REFIID refiid, void **lppInterface);

	// From IRecurrencePatternInspector
	virtual HRESULT SetPatternDaily(ULONG ulDailyPeriod);
	virtual HRESULT SetPatternWorkDays();
	virtual HRESULT SetPatternWeekly(ULONG ulFirstDayOfWeek, ULONG ulWeeklyPeriod, ULONG ulDayOfWeekMask);
	virtual HRESULT SetPatternAbsoluteMonthly(ULONG ulMonthlyPeriod, ULONG ulDayOfMonth);
	virtual HRESULT SetPatternRelativeMonthly(ULONG ulMonthlyPeriod, ULONG ulNum, ULONG ulDayOfWeekMask);
	virtual HRESULT SetPatternAbsoluteYearly(ULONG ulYearlyPeriod, ULONG ulMonth, ULONG ulDayOfMonth);
	virtual HRESULT SetPatternRelativeYearly(ULONG ulYearlyPeriod, ULONG ulMonth, ULONG ulNum, ULONG ulDayOfWeekMask);
	virtual HRESULT SetRangeNoEnd(ULONG ulStartDate);
	virtual HRESULT SetRangeNumbered(ULONG ulStartDate, ULONG ulOccurrences);
	virtual HRESULT SetRangeEndDate(ULONG ulStartDate, ULONG ulEndDate);

private: // methods
	PatternToState(RecurrencePattern *lpPattern, RecurrenceState &refRecurrenceState);

private: // members
	RecurrencePatternPtr m_ptrPattern;
	RecurrenceState &m_refRecurState;

private: // interfaces
	class xRecurrencePatternInspector : public IRecurrencePatternInspector {
	public:
		// From IUnknown
		virtual HRESULT __stdcall QueryInterface(REFIID refiid, void** lppInterface);
		virtual ULONG __stdcall AddRef();
		virtual ULONG __stdcall Release();

		// From IRecurrencePatternInspector
		virtual HRESULT SetPatternDaily(ULONG ulDailyPeriod);
		virtual HRESULT SetPatternWorkDays();
		virtual HRESULT SetPatternWeekly(ULONG ulFirstDayOfWeek, ULONG ulWeeklyPeriod, ULONG ulDayOfWeekMask);
		virtual HRESULT SetPatternAbsoluteMonthly(ULONG ulMonthlyPeriod, ULONG ulDayOfMonth);
		virtual HRESULT SetPatternRelativeMonthly(ULONG ulMonthlyPeriod, ULONG ulNum, ULONG ulDayOfWeekMask);
		virtual HRESULT SetPatternAbsoluteYearly(ULONG ulYearlyPeriod, ULONG ulMonth, ULONG ulDayOfMonth);
		virtual HRESULT SetPatternRelativeYearly(ULONG ulYearlyPeriod, ULONG ulMonth, ULONG ulNum, ULONG ulDayOfWeekMask);
		virtual HRESULT SetRangeNoEnd(ULONG ulStartDate);
		virtual HRESULT SetRangeNumbered(ULONG ulStartDate, ULONG ulOccurrences);
		virtual HRESULT SetRangeEndDate(ULONG ulStartDate, ULONG ulEndDate);
	} m_xRecurrencePatternInspector;
};



/**
 * Create a new RecurrencePattern instance.
 *
 * @param[out]	lppPattern	The new instance. Must be deleted by the caller.
 */
HRESULT RecurrencePattern::Create(RecurrencePattern **lppPattern)
{
	HRESULT hr = hrSuccess;
	RecurrencePatternPtr ptrPattern;

	if (lppPattern == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	try {
		ptrPattern.reset(new RecurrencePattern());
	} catch (const std::bad_alloc&) {
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}

	*lppPattern = ptrPattern.release();

exit:
	return hr;
}

/**
 * Create a new RecurrencePattern instance and configure it based on an existing
 * RecurrenceState structure.
 *
 * @param[in]	lpRecurState	The RecurrenceState structure which will configure the
 * 								new instance.
 * @param[out]	lppPattern		The new RecurrencePattern instance. Must be deleted by the caller.
 */
HRESULT RecurrencePattern::Create(RecurrenceState *lpRecurState, RecurrencePattern **lppPattern)
{
#define RP_TYPE(_rfreq, _ptype) (((unsigned short)(_rfreq) << 16) | (unsigned short)(_ptype))

	HRESULT hr = hrSuccess;
	RecurrencePatternPtr ptrPattern;

	if (lpRecurState == NULL || lppPattern == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	hr = Create(&ptrPattern);
	if (hr != hrSuccess)
		goto exit;

	switch (RP_TYPE(lpRecurState->ulRecurFrequency, lpRecurState->ulPatternType)) {
	case RP_TYPE(RF_DAILY, PT_DAY):
		hr = ptrPattern->SetPatternDaily(lpRecurState->ulPeriod / (60*24));
		break;
	case RP_TYPE(RF_DAILY, PT_WEEK):
		hr = ptrPattern->SetPatternWorkDays();
		break;
	case RP_TYPE(RF_WEEKLY, PT_WEEK):
		hr = ptrPattern->SetPatternWeekly(lpRecurState->ulFirstDOW, lpRecurState->ulPeriod, lpRecurState->ulWeekDays);
		break;
	case RP_TYPE(RF_MONTHLY, PT_MONTH):
		hr = ptrPattern->SetPatternAbsoluteMonthly(lpRecurState->ulPeriod, lpRecurState->ulDayOfMonth);
		break;
	case RP_TYPE(RF_MONTHLY, PT_MONTH_NTH):
		hr = ptrPattern->SetPatternRelativeMonthly(lpRecurState->ulPeriod, lpRecurState->ulWeekNumber, lpRecurState->ulWeekDays);
		break;
	case RP_TYPE(RF_YEARLY, PT_MONTH):
		hr = ptrPattern->SetPatternAbsoluteYearly(lpRecurState->ulPeriod / 12, 0, lpRecurState->ulDayOfMonth);
		break;
	case RP_TYPE(RF_YEARLY, PT_MONTH_NTH):
		hr = ptrPattern->SetPatternRelativeYearly(lpRecurState->ulPeriod / 12, 0, lpRecurState->ulWeekNumber, lpRecurState->ulWeekDays);
		break;
	default:
		hr = MAPI_E_CORRUPT_DATA;
		break;
	}
	if (hr != hrSuccess)
		goto exit;

	switch (lpRecurState->ulEndType) {
	case ET_NEVER:
		hr = ptrPattern->SetRangeNoEnd(lpRecurState->ulStartDate);
		break;
	case ET_NUMBER:
		hr = ptrPattern->SetRangeNumbered(lpRecurState->ulStartDate, lpRecurState->ulOccurrenceCount);
		break;
	case ET_DATE:
		hr = ptrPattern->SetRangeEndDate(lpRecurState->ulStartDate, lpRecurState->ulEndDate);
		break;
	default:
		hr = MAPI_E_CORRUPT_DATA;
		break;
	}

	*lppPattern = ptrPattern.release();

exit:
	return hr;
}

RecurrencePattern::RecurrencePattern()
: m_ulRecurFreq(ULONG_UNSET)
, m_ulPatternType(ULONG_UNSET)
, m_ulFirstDOW(ULONG_UNSET)
, m_ulPeriod(ULONG_UNSET)
, m_ulWeekDays(ULONG_UNSET)
, m_ulNum(ULONG_UNSET)
, m_ulMonth(ULONG_UNSET)
, m_ulStartDate(ULONG_UNSET)
, m_ulEndType(ULONG_UNSET)
, m_ulEndDate(ULONG_UNSET)
, m_ulOccurrenceCount(ULONG_UNSET)
, m_ulFirstDateTime(ULONG_UNSET)
{ }

RecurrencePattern::~RecurrencePattern()
{ }


/**
 * Set a daily pattern.
 *
 * @param[in]	ulDailyPeriod	The period in days.
 */
HRESULT RecurrencePattern::SetPatternDaily(ULONG ulDailyPeriod)
{
	m_ulRecurFreq = RF_DAILY;
	m_ulPatternType = PT_DAY;
	m_ulPeriod = ulDailyPeriod;

	UNSET(m_ulFirstDOW);
	UNSET(m_ulWeekDays);
	UNSET(m_ulNum);
	UNSET(m_ulMonth);

	UNSET(m_ulFirstDateTime);

	return hrSuccess;
}

/**
 * Set a pattern that has an occurrence on each workday.
 */
HRESULT RecurrencePattern::SetPatternWorkDays()
{
	m_ulRecurFreq = RF_DAILY;
	m_ulPatternType = PT_WEEK;
	m_ulFirstDOW = DOW_MONDAY;
	m_ulPeriod = 1;
	m_ulWeekDays = (WD_MONDAY | WD_TUESDAY | WD_WEDNESDAY | WD_THURSDAY | WD_FRIDAY);

	UNSET(m_ulNum);
	UNSET(m_ulMonth);

	UNSET(m_ulFirstDateTime);

	return hrSuccess;
}

/**
 * Set a pattern that occurs every N weeks.
 *
 * @param[in]	ulFirstDayOfWeek	Specifies which day is regarded as the first day
 * 									of the week.
 * @param[in]	ulWeeklyPeriod		Specifies the period in weeks.
 * @param[in]	ulDayOfWeekMask		A bitmask specifying on which days of the week
 * 									occurrences occur.
 */
HRESULT RecurrencePattern::SetPatternWeekly( ULONG ulFirstDayOfWeek, ULONG ulWeeklyPeriod, ULONG ulDayOfWeekMask)
{
	HRESULT hr = hrSuccess;

	if (!validate::Weekday(ulFirstDayOfWeek) || !validate::Weekdays(ulDayOfWeekMask)) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	m_ulRecurFreq = RF_WEEKLY;
	m_ulPatternType = PT_WEEK;
	m_ulFirstDOW = ulFirstDayOfWeek;
	m_ulPeriod = ulWeeklyPeriod;
	m_ulWeekDays = ulDayOfWeekMask;

	UNSET(m_ulNum);
	UNSET(m_ulMonth);

	UNSET(m_ulFirstDateTime);

exit:
	return hr;
}

/**
 * Set a pattern that has an occurrence every N months at a specific day.
 *
 * @param[in]	ulMonthlyPeriod		Specifies the period in months.
 * @param[in]	ulDayOfMonth		The day of the month on which the occurrence occurs. If
 * 									this value is greater than the amount of days in a month,
 * 									the last day of the month will be used.
 */
HRESULT RecurrencePattern::SetPatternAbsoluteMonthly(ULONG ulMonthlyPeriod, ULONG ulDayOfMonth)
{
	HRESULT hr = hrSuccess;

	if (ulDayOfMonth > 31) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	m_ulRecurFreq = RF_MONTHLY;
	m_ulPatternType = PT_MONTH;
	m_ulPeriod = ulMonthlyPeriod;
	m_ulNum = ulDayOfMonth;

	UNSET(m_ulFirstDOW);
	UNSET(m_ulWeekDays);
	UNSET(m_ulMonth);

	UNSET(m_ulFirstDateTime);

exit:
	return hr;
}

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
HRESULT RecurrencePattern::SetPatternRelativeMonthly(ULONG ulMonthlyPeriod, ULONG ulNum, ULONG ulDayOfWeekMask)
{
	HRESULT hr = hrSuccess;

	if (ulNum< 1 || ulNum > 5 || !validate::Weekdays(ulDayOfWeekMask)) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	m_ulRecurFreq = RF_MONTHLY;
	m_ulPatternType = PT_MONTH_NTH;
	m_ulPeriod = ulMonthlyPeriod;
	m_ulWeekDays = ulDayOfWeekMask;
	m_ulNum = ulNum;

	UNSET(m_ulFirstDOW);
	UNSET(m_ulMonth);

	UNSET(m_ulFirstDateTime);

exit:
	return hr;
}

/**
 * Set a pattern that has an occurrence every N years on a specific month and day.
 *
 * @param[in]	ulYearlyPeriod		Specifies the period in years.
 * @param[in]	ulMonth				Specifies the month.
 * @param[in]	ulDayOfMonth		The day of the month on which the occurrence occurs. If
 * 									this value is greater than the amount of days in a month,
 * 									the last day of the month will be used.
 */
HRESULT RecurrencePattern::SetPatternAbsoluteYearly(ULONG ulYearlyPeriod, ULONG ulMonth, ULONG ulDayOfMonth)
{
	HRESULT hr = hrSuccess;

	if (ulMonth > 12 || ulDayOfMonth < 1 || ulDayOfMonth > 31) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	m_ulRecurFreq = RF_YEARLY;
	m_ulPatternType = PT_MONTH;
	m_ulPeriod = ulYearlyPeriod * 12;
	m_ulNum = ulDayOfMonth;
	m_ulMonth = ulMonth;

	UNSET(m_ulFirstDOW);
	UNSET(m_ulWeekDays);

	UNSET(m_ulFirstDateTime);

exit:
	return hr;
}

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
HRESULT RecurrencePattern::SetPatternRelativeYearly(ULONG ulYearlyPeriod, ULONG ulMonth, ULONG ulNum, ULONG ulDayOfWeekMask)
{
	HRESULT hr = hrSuccess;

	if (ulMonth > 12 || ulNum < 1 || ulNum > 5 || !validate::Weekdays(ulDayOfWeekMask)) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	m_ulRecurFreq = RF_YEARLY;
	m_ulPatternType = PT_MONTH_NTH;
	m_ulPeriod = ulYearlyPeriod * 12;
	m_ulWeekDays = ulDayOfWeekMask;
	m_ulNum = ulNum;
	m_ulMonth = ulMonth;

	UNSET(m_ulFirstDOW);

	UNSET(m_ulFirstDateTime);

exit:
	return hr;
}


/**
 * Set a range that has no ending.
 *
 * @param[in]	ulStartDate		The start date of the range.
 */
HRESULT RecurrencePattern::SetRangeNoEnd(ULONG ulStartDate)
{
	m_ulEndType = ET_NEVER;
	m_ulStartDate = ulStartDate;

	UNSET(m_ulEndDate);
	UNSET(m_ulOccurrenceCount);

	UNSET(m_ulFirstDateTime);

	return hrSuccess;
}

/**
 * Set a range that ends after N occurrences.
 *
 * @param[in]	ulStartDate		The start date of the range.
 * @param[in]	ulOccurrences	The number of occurrences in the range.
 */
HRESULT RecurrencePattern::SetRangeNumbered(ULONG ulStartDate, ULONG ulOccurrences)
{
	m_ulEndType = ET_NUMBER;
	m_ulStartDate = ulStartDate;
	m_ulOccurrenceCount = ulOccurrences;

	UNSET(m_ulEndDate);

	UNSET(m_ulFirstDateTime);

	return hrSuccess;
}

/**
 * Set a range that ends after a specific date.
 *
 * @param[in]	ulStartDate		The start date of the range.
 * @param[in]	ulEndDate		The end date of the range.
 */
HRESULT RecurrencePattern::SetRangeEndDate(ULONG ulStartDate, ULONG ulEndDate)
{
	HRESULT hr = hrSuccess;

	if (ulStartDate > ulEndDate) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	m_ulEndType = ET_DATE;
	m_ulStartDate = ulStartDate;
	m_ulEndDate = ulEndDate;

	UNSET(m_ulOccurrenceCount);

	UNSET(m_ulFirstDateTime);

exit:
	return hr;
}



/**
 * Set a range that ends after a specific date.
 *
 * @param[in]	ulStartDate		The start date of the range.
 * @param[in]	ulOccurrences	The end date of the range.
 */
HRESULT RecurrencePattern::GetPatternType(ULONG *lpulType)
{
	if (lpulType == NULL)
		return MAPI_E_INVALID_PARAMETER;

	*lpulType = m_ulPatternType;

	return hrSuccess;
}

/**
 * Get the parameters of a daily pattern. This returns MAPI_E_NO_SUPPORT
 * if the RecurrencePattern isn't configured for a daily pattern
 *
 * @param[out]	lpulDailyPeriod		The period in days.
 */
HRESULT RecurrencePattern::GetPatternDaily(ULONG *lpulDailyPeriod)
{
	if (lpulDailyPeriod == NULL)
		return MAPI_E_INVALID_PARAMETER;

	if (m_ulPatternType != PT_DAY || m_ulRecurFreq != RF_DAILY)
		return MAPI_E_NO_SUPPORT;

	*lpulDailyPeriod = m_ulPeriod;

	return hrSuccess;
}

/**
 * Would get the parameters of a workdays pattern. Since there are no
 * parameters for such a pattern, this will only return MAPI_E_NO_SUPPORT
 * if the RecurrencePattern is not configured for a workdays pattern.
 */
HRESULT RecurrencePattern::GetPatternWorkDays()
{
	if (m_ulPatternType != PT_WEEK || m_ulRecurFreq != RF_DAILY)
		return MAPI_E_NO_SUPPORT;

	return hrSuccess;
}

/**
 * Get the parameters for a weekly pattern. This returns MAPI_E_NO_SUPPORT
 * if the RecurrencePattern isn't configured for a weekly pattern.
 *
 * @param[out]	lpulFirstDayOfWeek	Specifies which day is regarded as the first day
 * 									of the week.
 * @param[out]	lpulWeeklyPeriod	Specifies the period in weeks.
 * @param[out]	lpulDayOfWeekMask	A bitmask specifying on which days of the week
 * 									occurrences occur.
 */
HRESULT RecurrencePattern::GetPatternWeekly(ULONG *lpulFirstDayOfWeek, ULONG *lpulWeeklyPeriod, ULONG *lpulDayOfWeekMask)
{
	if (lpulFirstDayOfWeek == NULL || lpulWeeklyPeriod == NULL || lpulDayOfWeekMask == NULL)
		return MAPI_E_INVALID_PARAMETER;

	if (m_ulPatternType != PT_WEEK || m_ulRecurFreq != RF_WEEKLY)
		return MAPI_E_NO_SUPPORT;

	*lpulFirstDayOfWeek = m_ulFirstDOW;
	*lpulWeeklyPeriod = m_ulPeriod;
	*lpulDayOfWeekMask = m_ulWeekDays;

	return hrSuccess;
}

/**
 * Get the parameters for a monthly pattern that has an occurrence on an exact
 * day in the month. This returns MAPI_E_NO_SUPPORT if the RecurrencePattern isn't
 * configured for a absolute monthly pattern.
 *
 * @param[out]	lpulMonthlyPeriod	Specifies the period in months.
 * @param[out]	lpulDayOfMonth		The day of the month on which the occurrence occurs.
 */
HRESULT RecurrencePattern::GetPatternAbsoluteMonthly(ULONG *lpulMonthlyPeriod, ULONG *lpulDayOfMonth)
{
	if (lpulMonthlyPeriod == NULL || lpulDayOfMonth == NULL)
		return MAPI_E_INVALID_PARAMETER;

	if (m_ulPatternType != PT_MONTH || m_ulRecurFreq != RF_MONTHLY)
		return MAPI_E_NO_SUPPORT;

	*lpulMonthlyPeriod = m_ulPeriod;
	*lpulDayOfMonth = m_ulNum;

	return hrSuccess;
}

/**
 * Get the parameters for a monthly pattern that occurs on one or more days
 * in a certain week every N monghts. This returns MAPI_E_NO_SUPPORT if the
 * RecurrencePattern isn't configured for a relative monthly pattern.
 *
 * @param[out]	lpulMonthlyPeriod		Specifies the period in months.
 * @param[out]	lpulNum					Specifies the week number in the month. 1 to 4 specify
 * 										an exact week number. 5 specifies the last week.
 * @param[out]	lpulDayOfWeekMask		A bitmask specifying on which days of the week occurrences
 * 										occur. Although all possible combinations are possible and
 * 										allowed by the library, MAPI only allows single days or
 * 										0x3e for weekdays and 0x41 for weekenddays.
 */
HRESULT RecurrencePattern::GetPatternRelativeMonthly(ULONG *lpulMonthlyPeriod, ULONG *lpulWeekNum, ULONG *lpulDayOfWeek)
{
	if (lpulMonthlyPeriod == NULL || lpulWeekNum == NULL || lpulDayOfWeek == NULL)
		return MAPI_E_INVALID_PARAMETER;

	if (m_ulPatternType != PT_MONTH_NTH || m_ulRecurFreq != RF_MONTHLY)
		return MAPI_E_NO_SUPPORT;

	*lpulMonthlyPeriod = m_ulPeriod;
	*lpulWeekNum = m_ulNum;
	*lpulDayOfWeek = m_ulWeekDays;

	return hrSuccess;
}

/**
 * Get the parameters for a yearly pattern that occurs on an exact day
 * in a certain month every N years. This returns MAPI_E_NO_SUPPORT if the
 * RecurrencePattern isn't configured for an absolute yearly pattern.
 *
 * @param[out]	lpulYearlyPeriod		Specifies the period in years.
 * @param[out]	lpulMonth				Specifies the month.
 * @param[out]	lpulDayOfMonth			The day of the month on which the occurrence occurs.
 */
HRESULT RecurrencePattern::GetPatternAbsoluteYearly(ULONG *lpulYearlyPeriod, ULONG *lpulMonth, ULONG *lpulDayOfMonth)
{
	HRESULT hr = hrSuccess;
	ULONG ulYear, ulMonth, ulDate;

	if (lpulYearlyPeriod == NULL || lpulMonth == NULL || lpulDayOfMonth == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (m_ulPatternType != PT_MONTH || m_ulRecurFreq != RF_YEARLY) {
		hr = MAPI_E_NO_SUPPORT;
		goto exit;
	}

	hr = BaseDateToYMD(m_ulStartDate, &ulYear, &ulMonth, &ulDate);
	if (hr != hrSuccess)
		goto exit;

	if (ISSET(m_ulMonth) && m_ulMonth != 0)
		ulMonth = m_ulMonth;

	*lpulYearlyPeriod = m_ulPeriod / 12;
	*lpulMonth = ulMonth;
	*lpulDayOfMonth = m_ulNum;

exit:
	return hr;
}

/**
 * Get the parameters for a yearly pattern that occurs on one of more days
 * in a particular week in a certain month every N years. This returns
 * MAPI_E_NO_SUPPORT if the RecurrencePattern isn't configured for a
 * relative yearly pattern.
 *
 * @param[out]	lpulYearlyPeriod		Specifies the period in years.
 * @param[out]	lpulMonth				Specifies the month.
 * @param[out]	lpulNum					Specifies the week number in the month. 1 to 4 specify
 * 										an exact week number. 5 specifies the last week.
 * @param[out]	lpulDayOfWeekMask		A bitmask specifying on which days of the week occurrences
 * 										occur. Although all possible combinations are possible and
 * 										allowed by the library, MAPI only allows single days or
 * 										0x3e for weekdays and 0x41 for weekenddays.
 */
HRESULT RecurrencePattern::GetPatternRelativeYearly(ULONG *lpulYearlyPeriod, ULONG *lpulMonth, ULONG *lpulWeekNum, ULONG *lpulDayOfWeek)
{
	HRESULT hr = hrSuccess;
	ULONG ulYear, ulMonth, ulDate;

	if (lpulYearlyPeriod == NULL || lpulMonth == NULL || lpulWeekNum == NULL || lpulDayOfWeek == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (m_ulPatternType != PT_MONTH_NTH || m_ulRecurFreq != RF_YEARLY) {
		hr = MAPI_E_NO_SUPPORT;
		goto exit;
	}

	hr = BaseDateToYMD(m_ulStartDate, &ulYear, &ulMonth, &ulDate);
	if (hr != hrSuccess)
		goto exit;

	if (ISSET(m_ulMonth) && m_ulMonth != 0)
		ulMonth = m_ulMonth;

	*lpulYearlyPeriod = m_ulPeriod / 12;
	*lpulMonth = ulMonth;
	*lpulWeekNum = m_ulNum;
	*lpulDayOfWeek = m_ulWeekDays;

exit:
	return hr;
}

/**
 * Get the range type of the pattern.
 *
 * @param[out]	lpulType	The range type. Valid values are ET_NEVER,
 * 							ET_NUMBER and ET_DATE.
 */
HRESULT RecurrencePattern::GetRangeType(ULONG *lpulType)
{
	if (lpulType == NULL)
		return MAPI_E_INVALID_PARAMETER;

	*lpulType = m_ulEndType;

	return hrSuccess;
}

/**
 * Get the parameters of a no end range.
 *
 * @param[out]	lpulStartDate	The start date of the pattern.
 */
HRESULT RecurrencePattern::GetRangeNoEnd(ULONG *lpulStartDate)
{
	if (lpulStartDate == NULL)
		return MAPI_E_INVALID_PARAMETER;

	if (m_ulEndType != ET_NEVER)
		return MAPI_E_NO_SUPPORT;

	*lpulStartDate = m_ulStartDate;

	return hrSuccess;
}

/**
 * Get the parameters of a numbered range.
 *
 * @param[out]	lpulStartDate	The start date of the pattern.
 * @param[out]	lpulOccurrences	The number of occurrences in the pattern.
 */
HRESULT RecurrencePattern::GetRangeNumbered(ULONG *lpulStartDate, ULONG *lpulOccurrences)
{
	if (lpulStartDate == NULL || lpulOccurrences == NULL)
		return MAPI_E_INVALID_PARAMETER;

	if (m_ulEndType != ET_NUMBER)
		return MAPI_E_NO_SUPPORT;

	*lpulStartDate = m_ulStartDate;
	*lpulOccurrences = m_ulOccurrenceCount;

	return hrSuccess;
}

/**
 * Get the parameters of a end date range.
 *
 * @param[out]	lpulStartDate	The start date of the pattern.
 * @param[out]	lpulEndDate		The end date of the pattern.
 */
HRESULT RecurrencePattern::GetRangeEndDate(ULONG *lpulStartDate, ULONG *lpulEndDate)
{
	if (lpulStartDate == NULL || lpulEndDate == NULL)
		return MAPI_E_INVALID_PARAMETER;

	if (m_ulEndType != ET_DATE)
		return MAPI_E_NO_SUPPORT;

	*lpulStartDate = m_ulStartDate;
	*lpulEndDate = m_ulEndDate;

	return hrSuccess;
}


/**
 * Get the parameters of a RecurrencePattern by providing an instance of
 * a class that implements IRecurrencePatternInspector interface.
 * The proper methods will be called on that instance, which allows that
 * instance to take the appropriate actions.
 *
 * @param[in]	lpInspector		The object that will be inspecting this
 * 								instance.
 */
HRESULT RecurrencePattern::Inspect(IRecurrencePatternInspector *lpInspector)
{
	HRESULT hr = hrSuccess;
	ULONG ulMonth;

	if (lpInspector == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (m_ulRecurFreq == RF_YEARLY) {
		if (ISSET(m_ulMonth) && m_ulMonth != 0)
			ulMonth = m_ulMonth;

		else {
			ULONG ulYear, ulDay;

			hr = BaseDateToYMD(m_ulStartDate, &ulYear, &ulMonth, &ulDay);
			if (hr != hrSuccess)
				goto exit;
		}
	}

	switch (m_ulEndType) {
	case ET_NEVER:
		hr = lpInspector->SetRangeNoEnd(m_ulStartDate);
		break;
	case ET_NUMBER:
		hr = lpInspector->SetRangeNumbered(m_ulStartDate, m_ulOccurrenceCount);
		break;
	case ET_DATE:
		hr = lpInspector->SetRangeEndDate(m_ulStartDate, m_ulEndDate);
		break;
	default:
		hr = MAPI_E_CORRUPT_DATA;
		break;
	}

	switch (m_ulPatternType) {
	case PT_DAY:
		if (m_ulRecurFreq != RF_DAILY)
			hr = MAPI_E_CORRUPT_DATA;
		hr = lpInspector->SetPatternDaily(m_ulPeriod);
		break;
	case PT_WEEK:
		if (m_ulRecurFreq == RF_DAILY)
			hr = lpInspector->SetPatternWorkDays();
		else if (m_ulRecurFreq == RF_WEEKLY)
			hr = lpInspector->SetPatternWeekly(m_ulFirstDOW, m_ulPeriod, m_ulWeekDays);
		else
			hr = MAPI_E_CORRUPT_DATA;
		break;
	case PT_MONTH:
		if (m_ulRecurFreq == RF_MONTHLY)
			hr = lpInspector->SetPatternAbsoluteMonthly(m_ulPeriod, m_ulNum);
		else if (m_ulRecurFreq == RF_YEARLY)
			hr = lpInspector->SetPatternAbsoluteYearly(m_ulPeriod / 12, ulMonth, m_ulNum);
		else
			hr = MAPI_E_CORRUPT_DATA;
		break;
	case PT_MONTH_NTH:
		if (m_ulRecurFreq == RF_MONTHLY)
			hr = lpInspector->SetPatternRelativeMonthly(m_ulPeriod, m_ulNum, m_ulWeekDays);
		else if (m_ulRecurFreq == RF_YEARLY)
			hr = lpInspector->SetPatternRelativeYearly(m_ulPeriod / 12, ulMonth, m_ulNum, m_ulWeekDays);
		else
			hr = MAPI_E_CORRUPT_DATA;
		break;
	default:
		hr = MAPI_E_CORRUPT_DATA;
		break;
	}
	if (hr != hrSuccess)
		goto exit;

exit:
	return hr;
}

/**
 * Clone a RecurrencePattern.
 *
 * @param[out]	lppPattern	The new RecurrencePattern instance that is
 * 							an exact copy of the currence.
 */
HRESULT RecurrencePattern::Clone(RecurrencePattern **lppPattern)
{
	HRESULT hr = hrSuccess;
	RecurrencePatternPtr ptrClone;

	if (lppPattern == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	hr = Create(&ptrClone);
	if (hr != hrSuccess)
		goto exit;

	ptrClone->m_ulRecurFreq = m_ulRecurFreq;
	ptrClone->m_ulPatternType = m_ulPatternType;
	ptrClone->m_ulStartDate = m_ulStartDate;
	ptrClone->m_ulFirstDOW = m_ulFirstDOW;
	ptrClone->m_ulPeriod = m_ulPeriod;
	ptrClone->m_ulWeekDays = m_ulWeekDays;
	ptrClone->m_ulNum = m_ulNum;
	ptrClone->m_ulMonth = m_ulMonth;
	ptrClone->m_ulEndType = m_ulEndType;
	ptrClone->m_ulEndDate = m_ulEndDate;
	ptrClone->m_ulOccurrenceCount = m_ulOccurrenceCount;

	*lppPattern = ptrClone.release();

exit:
	return hr;
}


HRESULT RecurrencePattern::UpdateState(RecurrenceState *lpRecurState)
{
	HRESULT hr = hrSuccess;
	RecurrencePatternInspectorPtr ptrInspector;

	if (lpRecurState == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	hr = PatternToState::CreateInspector(this, *lpRecurState, &ptrInspector);
	if (hr != hrSuccess)
		goto exit;

	hr = Inspect(ptrInspector);

exit:
	return hr;
}


/**
 * Check if a date is part of the recurrence.
 *
 * @param[in]	ulBAseDate			The date, in rtime, to test.
 * @param[out]	lpbIsOccurrence		A boolean value that will be set based
 * 									on ulBaseDate being part of the pattern.
 */
HRESULT RecurrencePattern::IsOccurrence(ULONG ulBaseDate, bool *lpbIsOccurrence)
{
	HRESULT hr = hrSuccess;

	if (lpbIsOccurrence == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	switch (m_ulPatternType) {
	case PT_DAY:
		hr = IsOccurrenceDaily(ulBaseDate, lpbIsOccurrence);
		break;
	case PT_WEEK:
		hr = IsOccurrenceWeekly(ulBaseDate, lpbIsOccurrence);
		break;
	case PT_MONTH:
		hr = IsOccurrenceMonthly(ulBaseDate, lpbIsOccurrence);
		break;
	case PT_MONTH_NTH:
		hr = IsOccurrenceMonthlyNth(ulBaseDate, lpbIsOccurrence);
		break;
	default:
		hr = MAPI_E_NO_SUPPORT;
		goto exit;
	}

exit:
	return hr;
}

/**
 * This method returns the provided date if that is an occurrence of the current
 * recurrence pattern. Otherwise it will return the first to come.
 *
 * @param[in]	ulBaseDate		The basedate from where to look.
 * @param[out]	lpulBaseDate	The first date which is part of the pattern.
 */
HRESULT RecurrencePattern::GetOccurrence(ULONG ulBaseDate, ULONG *lpulBaseDate)
{
	HRESULT hr = hrSuccess;

	if (lpulBaseDate == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	switch (m_ulPatternType) {
	case PT_DAY:
		hr = GetOccurrenceDaily(ulBaseDate, lpulBaseDate);
		break;
	case PT_WEEK:
		hr = GetOccurrenceWeekly(ulBaseDate, lpulBaseDate);
		break;
	case PT_MONTH:
		hr = GetOccurrenceMonthly(ulBaseDate, lpulBaseDate);
		break;
	case PT_MONTH_NTH:
		hr = GetOccurrenceMonthlyNth(ulBaseDate, lpulBaseDate);
		break;
	default:
		hr = MAPI_E_NO_SUPPORT;
		goto exit;
	}

exit:
	return hr;
}

/**
 * Get all the dates that are part of the pattern within a certain date range.
 *
 * @param[in]	ulStartDate		The start of the range.
 * @param[in]	ulEndDate		The end of the range.
 * @param[out]	lpcbBaseDates	The amount of occurrences returned.
 * @param[out]	lpaBaseDates	The array with dates.
 */
HRESULT RecurrencePattern::GetOccurrencesInRange(ULONG ulStartDate, ULONG ulEndDate, ULONG *lpcbBaseDates, ULONG **lpaBaseDates)
{
	HRESULT hr = hrSuccess;

	if (lpcbBaseDates == NULL || lpaBaseDates == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	switch (m_ulPatternType) {
	case PT_DAY:
		hr = GetOccurrencesInRangeDaily(ulStartDate, ulEndDate, lpcbBaseDates, lpaBaseDates);
		break;
	case PT_WEEK:
		hr = GetOccurrencesInRangeWeekly(ulStartDate, ulEndDate, lpcbBaseDates, lpaBaseDates);
		break;
	case PT_MONTH:
		hr = GetOccurrencesInRangeMonthly(ulStartDate, ulEndDate, lpcbBaseDates, lpaBaseDates);
		break;
	case PT_MONTH_NTH:
		hr = GetOccurrencesInRangeMonthlyNth(ulStartDate, ulEndDate, lpcbBaseDates, lpaBaseDates);
		break;
	default:
		hr = MAPI_E_NO_SUPPORT;
		goto exit;
	}

exit:
	return hr;
}

/**
 * Get the first and last occurrence in the pattern.
 *
 * @param[out]	lpulFirst	The date of the first occurrence.
 * @param[out]	lpulLast	The date of the last occurrence.
 */
HRESULT RecurrencePattern::GetBounds(ULONG *lpulFirst, ULONG *lpulLast)
{
	HRESULT hr = hrSuccess;
	ULONG ulFirst = 0;
	ULONG ulLast = 0;

	if (lpulFirst == NULL || lpulLast == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	hr = GetOccurrence(0, &ulFirst);
	if (hr != hrSuccess)
		goto exit;

	if (m_ulEndType != ET_NEVER) {
		switch (m_ulPatternType) {
		case PT_DAY:
			hr = GetLastOccurrenceDaily(&ulLast);
			break;
		case PT_WEEK:
			hr = GetLastOccurrenceWeekly(&ulLast);
			break;
		case PT_MONTH:
			hr = GetLastOccurrenceMonthly(&ulLast);
			break;
		case PT_MONTH_NTH:
			hr = GetLastOccurrenceMonthlyNth(&ulLast);
			break;
		default:
			hr = MAPI_E_NO_SUPPORT;
			goto exit;
		}
		if (hr != hrSuccess)
			goto exit;
	}

	*lpulFirst = ulFirst;
	*lpulLast = ulLast;

exit:
	return hr;
}


HRESULT RecurrencePattern::GetFirstDateTime(ULONG *lpulFirstDateTime)
{
	ASSERT(lpulFirstDateTime != NULL);

	HRESULT hr = hrSuccess;

	if (m_ulFirstDateTime == ULONG_UNSET) {
		switch (m_ulPatternType) {
		case PT_DAY:
			hr = CalcDailyFirstDateTime(&m_ulFirstDateTime);
			break;
		case PT_WEEK:
			hr = CalcWeeklyFirstDateTime(&m_ulFirstDateTime);
			break;
		case PT_MONTH:
		case PT_MONTH_NTH:
			hr = CalcMonthlyFirstDateTime(&m_ulFirstDateTime);
			break;
		default:
			hr = MAPI_E_NO_SUPPORT;
			goto exit;
		}
	}

	*lpulFirstDateTime = m_ulFirstDateTime;

exit:
	return hr;
}


HRESULT RecurrencePattern::CalcDailyFirstDateTime(ULONG *lpulFirstDateTime) const
{
	ASSERT(lpulFirstDateTime != NULL);
	*lpulFirstDateTime = m_ulStartDate % (m_ulPeriod * 1440);

	return hrSuccess;
}

HRESULT RecurrencePattern::IsOccurrenceDaily(ULONG ulBaseDate, bool *lpbIsOccurrence)
{
	HRESULT hr = hrSuccess;
	ULONG ulFirstDateTime;

	ASSERT(lpbIsOccurrence != NULL);

	if (ulBaseDate < m_ulStartDate) {
		*lpbIsOccurrence = false;
		goto exit;
	} else {
		ULONG ulRecurEnd;

		hr = GetLastOccurrenceDaily(&ulRecurEnd);
		if (hr != hrSuccess)
			goto exit;

		if (ulBaseDate > ulRecurEnd) {
			*lpbIsOccurrence = false;
			goto exit;
		}
	}

	hr = GetFirstDateTime(&ulFirstDateTime);
	if (hr != hrSuccess)
		goto exit;

	if (ulBaseDate % (m_ulPeriod * 1440) != ulFirstDateTime) {
		*lpbIsOccurrence = false;
		goto exit;
	}

	*lpbIsOccurrence = true;

exit:
	return hr;
}

HRESULT RecurrencePattern::GetOccurrenceDaily(ULONG ulBaseDate, ULONG *lpulBaseDate)
{
	HRESULT hr = hrSuccess;
	ULONG ulFirstDateTime;
	ULONG ulOffsetInPeriod;
	ULONG ulMinutesPerPeriod;

	ASSERT(lpulBaseDate != NULL);

	if (ulBaseDate < m_ulStartDate)
		ulBaseDate = m_ulStartDate;
	else {
		ULONG ulRecurEnd;

		hr = GetLastOccurrenceDaily(&ulRecurEnd);
		if (hr != hrSuccess)
			goto exit;

		if (ulBaseDate > ulRecurEnd) {
			hr = MAPI_E_NOT_FOUND;
			goto exit;
		}
	}

	hr = GetFirstDateTime(&ulFirstDateTime);
	if (hr != hrSuccess)
		goto exit;

	ulMinutesPerPeriod = m_ulPeriod * 1440;
	ulOffsetInPeriod = ulBaseDate % ulMinutesPerPeriod;

	*lpulBaseDate = ulBaseDate + (ulMinutesPerPeriod + ulFirstDateTime - ulOffsetInPeriod) % ulMinutesPerPeriod;

exit:
	return hr;
}

HRESULT RecurrencePattern::GetOccurrencesInRangeDaily(ULONG ulStartDate, ULONG ulEndDate, ULONG *lpcbBaseDates, ULONG **lpaBaseDates)
{
	HRESULT hr = hrSuccess;
	ULONG ulFirstDate;
	ULONG ulRecurEnd;
	ULONG ulMinutesPerPeriod;
	std::list<ULONG> lstBaseDates;
	ULONG cbBaseDates = 0;
	mapi_array_ptr<ULONG> ptrBaseDates;

	ASSERT(lpcbBaseDates != NULL && lpaBaseDates != NULL);

	hr = GetOccurrenceDaily(ulStartDate, &ulFirstDate);
	if (hr == MAPI_E_NOT_FOUND) {
		*lpcbBaseDates = 0;
		*lpaBaseDates = NULL;
		hr = hrSuccess;
		goto exit;
	}
	if (hr != hrSuccess)
		goto exit;

	hr = GetLastOccurrenceDaily(&ulRecurEnd);
	if (hr != hrSuccess)
		goto exit;

	if (ulRecurEnd < ulEndDate)
		ulEndDate = ulRecurEnd;

	ulMinutesPerPeriod = m_ulPeriod * 1440;
	for (ULONG ulBaseDate = ulFirstDate; ulBaseDate <= ulEndDate; ulBaseDate += ulMinutesPerPeriod) {
		lstBaseDates.push_back(ulBaseDate);
		cbBaseDates++;
	}

	hr = MAPIAllocateBuffer(cbBaseDates * sizeof(ULONG), &ptrBaseDates);
	if (hr != hrSuccess)
		goto exit;

	std::copy(lstBaseDates.begin(), lstBaseDates.end(), ptrBaseDates.get());

	*lpcbBaseDates = cbBaseDates;
	*lpaBaseDates = ptrBaseDates.release();

exit:
	return hr;
}

HRESULT RecurrencePattern::GetLastOccurrenceDaily(ULONG *lpulEndDate)
{
	ASSERT(lpulEndDate != NULL);

	if (m_ulEndType == ET_NEVER)
		*lpulEndDate = 0x5ae980df;		// 'Fri Dec 31 23:59:00 4500'
	else if (m_ulEndType == ET_DATE)
		*lpulEndDate = m_ulEndDate;
	else {
		ASSERT(m_ulEndType == ET_NUMBER);

		*lpulEndDate = m_ulStartDate + (m_ulOccurrenceCount - 1) * m_ulPeriod * 1440;
	}

	return hrSuccess;
}



HRESULT RecurrencePattern::CalcWeeklyFirstDateTime(ULONG *lpulFirstDateTime) const
{
	// Internal, so we can assert
	ASSERT(lpulFirstDateTime != NULL);

	// conversions
	const bg::date date = BoostDateFromBaseDate(m_ulStartDate);
	const bg::greg_weekday weekday = BoostWeekdayFromMapiWeekday(m_ulFirstDOW);

	// calculation
	bg::date clipStart = date;
	if (date.day_of_week() != weekday)
		clipStart = bg::first_day_of_the_week_before(weekday).get_date(date);

	ULONG ulMinutesSinceEpoch = (clipStart - g_BoostEpoch).days() * 1440;
	ULONG ulMinutesPerPeriod = m_ulPeriod * 10080;

	*lpulFirstDateTime = ulMinutesSinceEpoch % ulMinutesPerPeriod;

	return hrSuccess;
}

HRESULT RecurrencePattern::IsOccurrenceWeekly(ULONG ulBaseDate, bool *lpbIsOccurrence)
{
	HRESULT hr = hrSuccess;
	ULONG ulFirstDateTime;
	bg::date clipStart;
	ULONG ulMinutesSinceEpoch;
	ULONG ulMinutesPerPeriod;
	ULONG ulOffsetInPeriod;

	const bg::date date(BoostDateFromBaseDate(ulBaseDate));
	const bg::greg_weekday weekday(BoostWeekdayFromMapiWeekday(m_ulFirstDOW));

	// Internal, so we can assert
	ASSERT(lpbIsOccurrence != NULL);

	if (ulBaseDate < m_ulStartDate) {
		*lpbIsOccurrence = false;
		goto exit;
	} else {
		ULONG ulRecurEnd;

		hr = GetLastOccurrenceWeekly(&ulRecurEnd);
		if (hr != hrSuccess)
			goto exit;

		if (ulBaseDate > ulRecurEnd) {
			*lpbIsOccurrence = false;
			goto exit;
		}
	}

	hr = GetFirstDateTime(&ulFirstDateTime);
	if (hr != hrSuccess)
		goto exit;

	// calculation
	clipStart = date;
	if (date.day_of_week() != weekday)
		clipStart = bg::first_day_of_the_week_before(weekday).get_date(date);

	ulMinutesSinceEpoch = (clipStart - g_BoostEpoch).days() * 1440;
	ulMinutesPerPeriod = m_ulPeriod * 10080;

	ulOffsetInPeriod = ulMinutesSinceEpoch % ulMinutesPerPeriod;

	*lpbIsOccurrence = (ulOffsetInPeriod == ulFirstDateTime && util::BoostWeekDayInMask(date.day_of_week(), m_ulWeekDays));

exit:
	return hr;
}

HRESULT RecurrencePattern::GetOccurrenceWeekly(ULONG ulBaseDate, ULONG *lpulBaseDate)
{
	HRESULT hr = hrSuccess;
	ULONG ulFirstDateTime;
	ULONG ulRecurEnd = 0;
	bg::date date;
	bg::date clipStart;
	ULONG ulMinutesSinceEpoch;
	ULONG ulMinutesPerPeriod;
	ULONG ulOffsetInPeriod;
	ULONG ulOffsetToFirst;

	const bg::greg_weekday weekday(BoostWeekdayFromMapiWeekday(m_ulFirstDOW));

	// Internal, so we can assert
	ASSERT(lpulBaseDate != NULL);

	hr = GetLastOccurrenceWeekly(&ulRecurEnd);
	if (hr != hrSuccess)
		goto exit;

	if (ulBaseDate < m_ulStartDate)
		ulBaseDate = m_ulStartDate;
	else if (ulBaseDate > ulRecurEnd) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	hr = GetFirstDateTime(&ulFirstDateTime);
	if (hr != hrSuccess)
		goto exit;

	// calculation
	date = BoostDateFromBaseDate(ulBaseDate);
	clipStart = date;
	if (date.day_of_week() != weekday)
		clipStart = bg::first_day_of_the_week_before(weekday).get_date(date);

	ulMinutesSinceEpoch = (clipStart - g_BoostEpoch).days() * 1440;
	ulMinutesPerPeriod = m_ulPeriod * 10080;

	ulOffsetInPeriod = ulMinutesSinceEpoch % ulMinutesPerPeriod;
	ulOffsetToFirst = (ulMinutesPerPeriod + ulFirstDateTime - ulOffsetInPeriod) % ulMinutesPerPeriod;

	if (ulOffsetToFirst == 0) {
		// Specified date falls in correct week. Now find a day that matches the pattern
		// If nothing can be found in the current week we'll skip to the next valid week
		for (bg::date d(date); d == date || d.day_of_week() != weekday; d += bg::date_duration(1)) {
			if (util::BoostWeekDayInMask(d.day_of_week(), m_ulWeekDays)) {
				ulBaseDate = BaseDateFromBoostDate(d);
				goto done;
			}
		}

		date = clipStart + bg::date_duration(m_ulPeriod * 7);
		for (bg::date d(date); d == date || d.day_of_week() != weekday; d += bg::date_duration(1)) {
			if (util::BoostWeekDayInMask(d.day_of_week(), m_ulWeekDays)) {
				ulBaseDate = BaseDateFromBoostDate(d);
				goto done;
			}
		}

		ASSERT(false);
	} else {
		date = clipStart + bg::date_duration(ulOffsetToFirst / 1440);
		for (bg::date d(date); d == date || d.day_of_week() != weekday; d += bg::date_duration(1)) {
			if (util::BoostWeekDayInMask(d.day_of_week(), m_ulWeekDays)) {
				ulBaseDate = BaseDateFromBoostDate(d);;
				goto done;
			}
		}

		ASSERT(false);
	}

done:
	if (ulBaseDate > ulRecurEnd) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	*lpulBaseDate = ulBaseDate;

exit:
	return hr;
}

HRESULT RecurrencePattern::GetOccurrencesInRangeWeekly(ULONG ulStartDate, ULONG ulEndDate, ULONG *lpcbBaseDates, ULONG **lpaBaseDates)
{
	HRESULT hr = hrSuccess;
	ULONG ulFirstDate;
	ULONG ulRecurEnd;
	bg::date first;
	bg::date weekstart;
	bg::date end;
	std::list<ULONG> lstBaseDates;
	ULONG cbBaseDates = 0;
	mapi_array_ptr<ULONG> ptrBaseDates;

	const bg::greg_weekday weekday(BoostWeekdayFromMapiWeekday(m_ulFirstDOW));

	ASSERT(lpcbBaseDates != NULL && lpaBaseDates != NULL);

	hr = GetOccurrenceWeekly(ulStartDate, &ulFirstDate);
	if (hr == MAPI_E_NOT_FOUND) {
		*lpcbBaseDates = 0;
		*lpaBaseDates = NULL;
		hr = hrSuccess;
		goto exit;
	}
	if (hr != hrSuccess)
		goto exit;

	hr = GetLastOccurrenceWeekly(&ulRecurEnd);
	if (hr != hrSuccess)
		goto exit;

	end = BoostDateFromBaseDate(std::min(ulEndDate, ulRecurEnd));

	// First week, can be partial
	first = BoostDateFromBaseDate(ulFirstDate);
	for (bg::date d(first); (d == first || d.day_of_week() != weekday) && d <= end; d += bg::date_duration(1)) {
		if (util::BoostWeekDayInMask(d.day_of_week(), m_ulWeekDays)) {
			lstBaseDates.push_back(BaseDateFromBoostDate(d));
			cbBaseDates++;
		}
	}

	weekstart = first;
	if (first.day_of_week() != weekday)
		weekstart = bg::first_day_of_the_week_before(weekday).get_date(first);

	while(true) {
		// Increment week to next period
		weekstart += bg::date_duration(m_ulPeriod * 7);
		if (weekstart > end)
			break;

		for (bg::date d(weekstart); (d == weekstart || d.day_of_week() != weekday) && d <= end; d += bg::date_duration(1)) {
			if (util::BoostWeekDayInMask(d.day_of_week(), m_ulWeekDays)) {
				lstBaseDates.push_back(BaseDateFromBoostDate(d));
				cbBaseDates++;
			}
		}
	}

	hr = MAPIAllocateBuffer(cbBaseDates * sizeof(ULONG), &ptrBaseDates);
	if (hr != hrSuccess)
		goto exit;

	std::copy(lstBaseDates.begin(), lstBaseDates.end(), ptrBaseDates.get());

	*lpcbBaseDates = cbBaseDates;
	*lpaBaseDates = ptrBaseDates.release();

exit:
	return hr;
}

HRESULT RecurrencePattern::GetLastOccurrenceWeekly(ULONG *lpulEndDate)
{
	HRESULT hr = hrSuccess;

	ASSERT(lpulEndDate != NULL);

	if (m_ulEndType == ET_NEVER)
		*lpulEndDate = 0x5ae980df;		// 'Fri Dec 31 23:59:00 4500'
	else if (m_ulEndType == ET_DATE)
		*lpulEndDate = m_ulEndDate;
	else {
		ASSERT(m_ulEndType == ET_NUMBER);

		bg::date date;
		bg::date last;
		ULONG ulCount = 0;

		ULONG ulDaysInWeek;
		ULONG ulPeriods;

		const bg::greg_weekday weekday(BoostWeekdayFromMapiWeekday(m_ulFirstDOW));

		// First count the number of days until the end of the
		date = BoostDateFromBaseDate(m_ulStartDate);
		for (bg::date d(date); (d == date || d.day_of_week() != weekday) && ulCount < m_ulOccurrenceCount; d += bg::date_duration(1)) {
			if (util::BoostWeekDayInMask(d.day_of_week(), m_ulWeekDays)) {
				last = d;
				ulCount++;
			}
		}

		if (ulCount == m_ulOccurrenceCount) {
			*lpulEndDate = BaseDateFromBoostDate(last);
			goto exit;
		}

		// We need more, advance to next period
		if (date.day_of_week() != weekday)
			date = bg::first_day_of_the_week_before(weekday).get_date(date);
		date += bg::date_duration(m_ulPeriod * 7);

		// We can now simply advance the number of required days devided by the number of
		// days per week amount of periods.
		ulDaysInWeek = util::GetDaysInWeek(m_ulWeekDays);
		ulPeriods = (m_ulOccurrenceCount - ulCount) / ulDaysInWeek;
		date += bg::date_duration(m_ulPeriod * 7 * ulPeriods);
		ulCount += ulDaysInWeek * ulPeriods;

		// If we're done now, we'll return the date of the day before the start of the
		// current period. This is to make sure that if the current day is part of the
		// recurrence, it won't be included.
		if (ulCount == m_ulOccurrenceCount) {
			*lpulEndDate = BaseDateFromBoostDate(date - bg::date_duration(1));
			goto exit;
		}

		// Last thing to do is to add the remainder from the current week.
		ASSERT(m_ulOccurrenceCount - ulCount < ulDaysInWeek);
		for (bg::date d(date); (d == date || d.day_of_week() != weekday) && ulCount < m_ulOccurrenceCount; d += bg::date_duration(1)) {
			if (util::BoostWeekDayInMask(d.day_of_week(), m_ulWeekDays)) {
				last = d;
				ulCount++;
			}
		}

		// We should be done here in any case.
		ASSERT(m_ulOccurrenceCount == ulCount);
		*lpulEndDate = BaseDateFromBoostDate(last);
	}

exit:
	return hr;
}



ULONG RecurrencePattern::GetMonthlyOffset(ULONG ulYear, ULONG ulMonth, ULONG ulPeriod)
{
	ULONG ulMonthsSinceEpoch;

	ulMonthsSinceEpoch = 12 * (ulYear - 1601) + ulMonth - 1;
	return ulMonthsSinceEpoch % ulPeriod;
}

ULONG RecurrencePattern::GetMonthlyOffset(ULONG ulBaseDate, ULONG ulPeriod)
{
	bg::date::ymd_type ymd(1601, 1, 1);
	bg::date firstMonth;

	ymd = BoostDateFromBaseDate(ulBaseDate).year_month_day();
	return GetMonthlyOffset(ymd.year, ymd.month, ulPeriod);
}

HRESULT RecurrencePattern::CalcMonthlyFirstDateTime(ULONG *lpulFirstDateTime) const
{
	ASSERT(lpulFirstDateTime != NULL);
	ASSERT(m_ulRecurFreq != RF_YEARLY || ISSET(m_ulMonth));
	ASSERT(m_ulRecurFreq == RF_YEARLY || !ISSET(m_ulMonth));

	bg::date::ymd_type ymd = BoostDateFromBaseDate(m_ulStartDate).year_month_day();

	if (ISSET(m_ulMonth) && m_ulMonth != 0 && m_ulMonth != ymd.month) {
		if (m_ulMonth < ymd.month)
			ymd.year = ymd.year + 1;
		ymd.month = m_ulMonth;
	}

	*lpulFirstDateTime = GetMonthlyOffset(ymd.year, ymd.month, m_ulPeriod);

	return hrSuccess;
}

HRESULT RecurrencePattern::IsOccurrenceMonthly(ULONG ulBaseDate, bool *lpbIsOccurrence)
{
	HRESULT hr = hrSuccess;
	ULONG ulFirstDateTime;
	ULONG ulOffsetInPeriod;
	bg::date date;
	bg::date::ymd_type ymd(1601, 1, 1);

	ASSERT(lpbIsOccurrence != NULL);

	if (ulBaseDate < m_ulStartDate) {
		*lpbIsOccurrence = false;
		goto exit;
	} else {
		ULONG ulRecurEnd;

		hr = GetLastOccurrenceMonthly(&ulRecurEnd);
		if (hr != hrSuccess)
			goto exit;

		if (ulBaseDate > ulRecurEnd) {
			*lpbIsOccurrence = false;
			goto exit;
		}
	}

	hr = GetFirstDateTime(&ulFirstDateTime);
	if (hr != hrSuccess)
		goto exit;

	ulOffsetInPeriod = GetMonthlyOffset(ulBaseDate, m_ulPeriod);
	if (ulOffsetInPeriod != ulFirstDateTime) {
		// wrong month
		*lpbIsOccurrence = false;
		goto exit;
	}

	// Correct month. Now check day
	date = BoostDateFromBaseDate(ulBaseDate);
	ymd = date.year_month_day();

	if (ymd.day == m_ulNum) {
		// Exact same day
		*lpbIsOccurrence = true;
		goto exit;
	}

	if (m_ulNum > ymd.day && date == date.end_of_month()) {
		// Last day of shorter month
		*lpbIsOccurrence = true;
		goto exit;
	}

	*lpbIsOccurrence = false;

exit:
	return hr;
}

HRESULT RecurrencePattern::GetOccurrenceMonthly(ULONG ulBaseDate, ULONG *lpulBaseDate)
{
	return GetOccurrenceMonthly(ulBaseDate, 0, lpulBaseDate);
}

HRESULT RecurrencePattern::GetOccurrenceMonthly(ULONG ulBaseDate, ULONG ulFlags, ULONG *lpulBaseDate)
{
	HRESULT hr = hrSuccess;
	ULONG ulFirstDateTime;
	ULONG ulRecurEnd = 0;
	ULONG ulOffsetInPeriod;
	bg::date date;
	bg::date::ymd_type ymd(1601, 1, 1);
	bg::date::day_type last_day(1);

	ASSERT(lpulBaseDate != NULL);

	if ((ulFlags & NO_END_CHECK) == 0) {
		hr = GetLastOccurrenceMonthly(&ulRecurEnd);
		if (hr != hrSuccess)
			goto exit;
	}

	if (ulBaseDate < m_ulStartDate)
		ulBaseDate = m_ulStartDate;
	else if ((ulFlags & NO_END_CHECK) == 0 && ulBaseDate > ulRecurEnd) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	hr = GetFirstDateTime(&ulFirstDateTime);
	if (hr != hrSuccess)
		goto exit;

	ulOffsetInPeriod = GetMonthlyOffset(ulBaseDate, m_ulPeriod);

	date = BoostDateFromBaseDate(ulBaseDate);
	ymd = date.year_month_day();

	// If this is the correct month, we need to check if the occurrence this month happens
	// before or after ulBaseDate. In case it's before ulBaseDate, we'll get the next correct
	// month (which we will also do of this is not the correct month).
	if (ulOffsetInPeriod == ulFirstDateTime) {
		if (m_ulNum >= ymd.day) {
			last_day = date.end_of_month().day();
			hr = BaseDateFromYMD(ymd.year, ymd.month, std::min(ULONG(last_day), m_ulNum), lpulBaseDate);
			goto exit;
		} else {
			date = bg::date(ymd.year, ymd.month, 1);
			date += bg::months(m_ulPeriod);
		}
	} else {
		date = bg::date(ymd.year, ymd.month, 1);
		date += bg::months((m_ulPeriod + ulFirstDateTime - ulOffsetInPeriod) % m_ulPeriod);
	}

	ymd = date.year_month_day();
	last_day = date.end_of_month().day();

	hr = BaseDateFromYMD(ymd.year, ymd.month, std::min(ULONG(last_day), m_ulNum), &ulBaseDate);
	if (hr != hrSuccess)
		goto exit;

	if ((ulFlags & NO_END_CHECK) == 0 && ulBaseDate > ulRecurEnd) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	*lpulBaseDate = ulBaseDate;

exit:
	return hr;
}

HRESULT RecurrencePattern::GetOccurrencesInRangeMonthly(ULONG ulStartDate, ULONG ulEndDate, ULONG *lpcbBaseDates, ULONG **lpaBaseDates)
{
	HRESULT hr = hrSuccess;
	ULONG ulFirstDate;
	ULONG ulRecurEnd;
	std::list<ULONG> lstBaseDates;
	ULONG cbBaseDates = 0;
	mapi_array_ptr<ULONG> ptrBaseDates;

	ASSERT(lpcbBaseDates != NULL && lpaBaseDates != NULL);

	hr = GetOccurrenceMonthly(ulStartDate, &ulFirstDate);
	if (hr == MAPI_E_NOT_FOUND) {
		*lpcbBaseDates = 0;
		*lpaBaseDates = NULL;
		hr = hrSuccess;
		goto exit;
	}
	if (hr != hrSuccess)
		goto exit;

	hr = GetLastOccurrenceMonthly(&ulRecurEnd);
	if (hr != hrSuccess)
		goto exit;

	if (ulRecurEnd < ulEndDate)
		ulEndDate = ulRecurEnd;

	for (bg::month_iterator iter(BoostDateFromBaseDate(ulFirstDate), m_ulPeriod); /* No end condition */; ++iter) {
		/*
		 * The Boost date-time lib has this 'nice' feature, which is snap-to-end. This makes sure that
		 * if a certain date is at the end of the month, adding a month will cause the new date to also
		 * be at the end of the month, which is not the same day of the month. This has some nice
		 * benefits in some situations, for example when the next month is shorter, but it makes the
		 * month_iterator behave unexpected when the last day of a less than 31 day month is the selected
		 * day in for occurrence.
		 *
		 * For example:
		 * If we choose day 30 in each month and start in april, the dates selected by the month_iterator
		 * will be april 30, may 31, june 30, july 31 etc...
		 * But we would expect april 30, may 30, june 30, july 30 etc...
		 *
		 * The other way around, If choose day 31 and start in april we would get
		 * april 31, may 31, june 30, july 31 etc... Which is what we expect.
		 *
		 * So the strategy is to take the date from the month_iterator but make sure the day of month is
		 * never bigger than set by the recurrence.
		 *
		 * We can also only perform the endDate check after the correction
		 */
		ULONG ulBaseDate;

		bg::date::ymd_type ymd = iter->year_month_day();
		hr = BaseDateFromYMD(ymd.year, ymd.month, std::min((ULONG)ymd.day, m_ulNum), &ulBaseDate);
		if (hr != hrSuccess)
			goto exit;

		if (ulBaseDate > ulEndDate)
			break;

		lstBaseDates.push_back(ulBaseDate);
		cbBaseDates++;
	}

	hr = MAPIAllocateBuffer(cbBaseDates * sizeof(ULONG), &ptrBaseDates);
	if (hr != hrSuccess)
		goto exit;

	std::copy(lstBaseDates.begin(), lstBaseDates.end(), ptrBaseDates.get());

	*lpcbBaseDates = cbBaseDates;
	*lpaBaseDates = ptrBaseDates.release();

exit:
	return hr;
}

HRESULT RecurrencePattern::GetLastOccurrenceMonthly(ULONG *lpulEndDate)
{
	HRESULT hr = hrSuccess;

	ASSERT(lpulEndDate != NULL);

	if (m_ulEndType == ET_NEVER)
		*lpulEndDate = 0x5ae980df;		// 'Fri Dec 31 23:59:00 4500'
	else if (m_ulEndType == ET_DATE)
		*lpulEndDate = m_ulEndDate;
	else {
		ULONG ulFirstDate;
		bg::date date;
		bg::date::ymd_type ymd(1601, 1, 1);

		ASSERT(m_ulEndType == ET_NUMBER);

		hr = GetOccurrenceMonthly(m_ulStartDate, NO_END_CHECK, &ulFirstDate);
		if (hr != hrSuccess)
			goto exit;

		date = BoostDateFromBaseDate(ulFirstDate) + bg::months((m_ulOccurrenceCount - 1) * m_ulPeriod);
		ymd = date.year_month_day();
		hr = BaseDateFromYMD(ymd.year, ymd.month, std::min((ULONG)ymd.day, m_ulNum), lpulEndDate);
	}

exit:
	return hr;
}

// This method is intentionally not a static member because that would force the
// inclusion of <boost/date_time/gregorian/gregorian.hpp> into all consuming modules.
static bg::date NthDayFromMaskInMonth(ULONG ulNum, ULONG ulWeekDays, ULONG ulMonth, ULONG ulYear)
{
	ASSERT(ulNum >= 1 && ulNum <= 5);
	ASSERT(validate::Weekdays(ulWeekDays));
	ASSERT(ulMonth >= 1 && ulMonth <= 12);

	const bg::date month_start(ulYear, ulMonth, 1);
	const bg::date month_end(month_start.end_of_month());

	if (ulNum < 5) {
		for (bg::date d(month_start); d <= month_end; d += bg::date_duration(1)) {
			if (util::BoostWeekDayInMask(d.day_of_week(), ulWeekDays)) {
				if (--ulNum == 0)
					return d;
			}
		}
		ASSERT(false);
	}

	else {
		for (bg::date d(month_end); d >= month_start; d-= bg::date_duration(1)) {
			if (util::BoostWeekDayInMask(d.day_of_week(), ulWeekDays))
				return d;
		}
		ASSERT(false);
	}

	return bg::date();
}

HRESULT RecurrencePattern::IsOccurrenceMonthlyNth(ULONG ulBaseDate, bool *lpbIsOccurrence)
{
	HRESULT hr = hrSuccess;
	ULONG ulOffsetInPeriod;
	ULONG ulFirstDateTime;
	bg::date basedate;
	bg::date refdate;
	bg::date::ymd_type ymd(1601, 1, 1);

	ASSERT(lpbIsOccurrence != NULL);

	hr = GetFirstDateTime(&ulFirstDateTime);
	if (hr != hrSuccess)
		goto exit;

	ulOffsetInPeriod = GetMonthlyOffset(ulBaseDate, m_ulPeriod);
	if (ulOffsetInPeriod != ulFirstDateTime) {
		// wrong month
		*lpbIsOccurrence = false;
		goto exit;
	}

	// Correct month. Now check day
	basedate = BoostDateFromBaseDate(ulBaseDate);
	ymd = basedate.year_month_day();

	refdate = NthDayFromMaskInMonth(m_ulNum, m_ulWeekDays, ymd.month, ymd.year);

	*lpbIsOccurrence = (basedate == refdate);

exit:
	return hr;
}

HRESULT RecurrencePattern::GetOccurrenceMonthlyNth(ULONG ulBaseDate, ULONG *lpulBaseDate)
{
	return GetOccurrenceMonthlyNth(ulBaseDate, 0, lpulBaseDate);
}

HRESULT RecurrencePattern::GetOccurrenceMonthlyNth(ULONG ulBaseDate, ULONG ulFlags, ULONG *lpulBaseDate)
{
	HRESULT hr = hrSuccess;
	ULONG ulFirstDateTime;
	ULONG ulRecurEnd = 0;
	ULONG ulOffsetInPeriod;
	bg::date basedate;
	bg::date refdate;
	bg::date::ymd_type ymd(1601, 1, 1);

	ASSERT(lpulBaseDate != NULL);

	if ((ulFlags & NO_END_CHECK) == 0) {
		hr = GetLastOccurrenceMonthlyNth(&ulRecurEnd);
		if (hr != hrSuccess)
			goto exit;
	}

	if (ulBaseDate < m_ulStartDate)
		ulBaseDate = m_ulStartDate;
	else if ((ulFlags & NO_END_CHECK) == 0 && ulBaseDate > ulRecurEnd) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	hr = GetFirstDateTime(&ulFirstDateTime);
	if (hr != hrSuccess)
		goto exit;

	ulOffsetInPeriod = GetMonthlyOffset(ulBaseDate, m_ulPeriod);

	basedate = BoostDateFromBaseDate(ulBaseDate);
	ymd = basedate.year_month_day();

	// If this is the correct month, we need to check if the occurrence this month happens
	// before or after ulBaseDate. In case it's before ulBaseDate, we'll get the next correct
	// month (which we will also do of this is not the correct month).
	if (ulOffsetInPeriod == ulFirstDateTime) {
		refdate = NthDayFromMaskInMonth(m_ulNum, m_ulWeekDays, ymd.month, ymd.year);

		if (refdate < basedate) {
			// Goto next period and find correct day there.
			ymd = (basedate + bg::months(m_ulPeriod)).year_month_day();
			refdate = NthDayFromMaskInMonth(m_ulNum, m_ulWeekDays, ymd.month, ymd.year);
		}
	} else {
		ymd = (basedate + bg::months((m_ulPeriod + ulFirstDateTime - ulOffsetInPeriod) % m_ulPeriod)).year_month_day();
		refdate = NthDayFromMaskInMonth(m_ulNum, m_ulWeekDays, ymd.month, ymd.year);
	}

	ulBaseDate = BaseDateFromBoostDate(refdate);

	if ((ulFlags & NO_END_CHECK) == 0 && ulBaseDate > ulRecurEnd) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	*lpulBaseDate = ulBaseDate;

exit:
	return hr;
}

HRESULT RecurrencePattern::GetOccurrencesInRangeMonthlyNth(ULONG ulStartDate, ULONG ulEndDate, ULONG *lpcbBaseDates, ULONG **lpaBaseDates)
{
	HRESULT hr = hrSuccess;
	ULONG ulFirstDate;
	ULONG ulRecurEnd;
	bg::date date;
	bg::date::ymd_type ymd(1601, 1, 1);
	std::list<ULONG> lstBaseDates;
	ULONG cbBaseDates = 0;
	mapi_array_ptr<ULONG> ptrBaseDates;

	ASSERT(lpcbBaseDates != NULL && lpaBaseDates != NULL);

	hr = GetOccurrenceMonthlyNth(ulStartDate, &ulFirstDate);
	if (hr == MAPI_E_NOT_FOUND) {
		*lpcbBaseDates = 0;
		*lpaBaseDates = NULL;
		hr = hrSuccess;
		goto exit;
	}
	if (hr != hrSuccess)
		goto exit;

	hr = GetLastOccurrenceMonthlyNth(&ulRecurEnd);
	if (hr != hrSuccess)
		goto exit;

	if (ulRecurEnd < ulEndDate)
		ulEndDate = ulRecurEnd;

	lstBaseDates.push_back(ulFirstDate);
	cbBaseDates = 1;

	date = BoostDateFromBaseDate(ulFirstDate);

	while (true) {
		ULONG ulBaseDate;

		date += bg::months(m_ulPeriod);
		ymd = date.year_month_day();

		date = NthDayFromMaskInMonth(m_ulNum, m_ulWeekDays, ymd.month, ymd.year);
		ulBaseDate = BaseDateFromBoostDate(date);
		if (ulBaseDate > ulEndDate)
			break;

		lstBaseDates.push_back(ulBaseDate);
		cbBaseDates++;
	}

	hr = MAPIAllocateBuffer(cbBaseDates * sizeof(ULONG), &ptrBaseDates);
	if (hr != hrSuccess)
		goto exit;

	std::copy(lstBaseDates.begin(), lstBaseDates.end(), ptrBaseDates.get());

	*lpcbBaseDates = cbBaseDates;
	*lpaBaseDates = ptrBaseDates.release();

exit:
	return hr;
}

HRESULT RecurrencePattern::GetLastOccurrenceMonthlyNth(ULONG *lpulEndDate)
{
	HRESULT hr = hrSuccess;

	ASSERT(lpulEndDate != NULL);

	if (m_ulEndType == ET_NEVER)
		*lpulEndDate = 0x5ae980df;		// 'Fri Dec 31 23:59:00 4500'
	else if (m_ulEndType == ET_DATE)
		*lpulEndDate = m_ulEndDate;
	else {
		ULONG ulFirstDate;
		bg::date date;
		bg::date::ymd_type ymd(1601, 1, 1);

		ASSERT(m_ulEndType == ET_NUMBER);

		hr = GetOccurrenceMonthlyNth(m_ulStartDate, NO_END_CHECK, &ulFirstDate);
		if (hr != hrSuccess)
			goto exit;

		date = BoostDateFromBaseDate(ulFirstDate) + bg::months((m_ulOccurrenceCount - 1) * m_ulPeriod);
		ymd = date.year_month_day();
		date = NthDayFromMaskInMonth(m_ulNum, m_ulWeekDays, ymd.month, ymd.year);

		*lpulEndDate = BaseDateFromBoostDate(date);
	}

exit:
	return hr;
}



HRESULT PatternToState::CreateInspector(RecurrencePattern *lpPattern, RecurrenceState &refRecurrenceState, IRecurrencePatternInspector **lppInspector)
{
	HRESULT hr = hrSuccess;
	mapi_object_ptr<PatternToState> ptrPatternToState;

	if (lpPattern == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	try {
		ptrPatternToState.reset(new PatternToState(lpPattern, refRecurrenceState));
	} catch (const std::bad_alloc &) {
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}

	hr = ptrPatternToState->QueryInterface(IID_IRecurrencePatternInspector, (LPVOID*)lppInspector);

exit:
	return hr;
}

PatternToState::PatternToState(RecurrencePattern *lpPattern, RecurrenceState &refRecurrenceState)
: m_ptrPattern(lpPattern)
, m_refRecurState(refRecurrenceState)
{ }

HRESULT	PatternToState::QueryInterface(REFIID refiid, void **lppInterface)
{
	REGISTER_INTERFACE(IID_ECUnknown, this);

	REGISTER_INTERFACE(IID_IRecurrencePatternInspector, &this->m_xRecurrencePatternInspector);
	REGISTER_INTERFACE(IID_IUnknown, &this->m_xRecurrencePatternInspector);

	return MAPI_E_INTERFACE_NOT_SUPPORTED;
}

HRESULT PatternToState::SetPatternDaily(ULONG ulDailyPeriod)
{
	m_refRecurState.ulRecurFrequency = RF_DAILY;
	m_refRecurState.ulPatternType = PT_DAY;
	m_refRecurState.ulCalendarType = 0; // TODO: Verify
	m_refRecurState.ulPeriod = ulDailyPeriod * 24*60;
	m_refRecurState.ulSlidingFlag = 0;
	m_refRecurState.ulWeekDays = 0;
	m_refRecurState.ulDayOfMonth = 0;
	m_refRecurState.ulWeekNumber = 0;
	m_refRecurState.ulFirstDOW = 0;

	m_refRecurState.ulFirstDateTime = (m_refRecurState.ulStartDate % m_refRecurState.ulPeriod);

	return hrSuccess;
}

HRESULT PatternToState::SetPatternWorkDays()
{
	HRESULT hr = hrSuccess;

	m_refRecurState.ulRecurFrequency = RF_DAILY;
	m_refRecurState.ulPatternType = PT_WEEK;
	m_refRecurState.ulCalendarType = 0; // TODO: Verify
	m_refRecurState.ulPeriod = 24*60;
	m_refRecurState.ulSlidingFlag = 0;
	m_refRecurState.ulWeekDays = (WD_MONDAY | WD_TUESDAY | WD_WEDNESDAY | WD_THURSDAY | WD_FRIDAY);
	m_refRecurState.ulDayOfMonth = 0;
	m_refRecurState.ulWeekNumber = 0;
	m_refRecurState.ulFirstDOW = 0;

	hr = m_ptrPattern->CalcWeeklyFirstDateTime(&m_refRecurState.ulFirstDateTime);
	if (hr != hrSuccess)
		goto exit;

exit:
	return hr;
}

HRESULT PatternToState::SetPatternWeekly(ULONG ulFirstDayOfWeek, ULONG ulWeeklyPeriod, ULONG ulDayOfWeekMask)
{
	HRESULT hr = hrSuccess;

	m_refRecurState.ulRecurFrequency = RF_WEEKLY;
	m_refRecurState.ulPatternType = PT_WEEK;
	m_refRecurState.ulCalendarType = 0; // TODO: Verify
	m_refRecurState.ulPeriod = ulWeeklyPeriod;
	m_refRecurState.ulSlidingFlag = 0;
	m_refRecurState.ulWeekDays = ulDayOfWeekMask;
	m_refRecurState.ulDayOfMonth = 0;
	m_refRecurState.ulWeekNumber = 0;
	m_refRecurState.ulFirstDOW = ulFirstDayOfWeek;

	hr = m_ptrPattern->CalcWeeklyFirstDateTime(&m_refRecurState.ulFirstDateTime);
	if (hr != hrSuccess)
		goto exit;

exit:
	return hr;
}

HRESULT PatternToState::SetPatternAbsoluteMonthly(ULONG ulMonthlyPeriod, ULONG ulDayOfMonth)
{
	HRESULT hr = hrSuccess;
	ULONG ulFirstDateTimeMonths;
	ULONG ulDays;

	m_refRecurState.ulRecurFrequency = RF_MONTHLY;
	m_refRecurState.ulPatternType = PT_MONTH;
	m_refRecurState.ulCalendarType = 0; // TODO: Verify
	m_refRecurState.ulPeriod = ulMonthlyPeriod;
	m_refRecurState.ulSlidingFlag = 0;
	m_refRecurState.ulWeekDays = 0;
	m_refRecurState.ulDayOfMonth = ulDayOfMonth;
	m_refRecurState.ulWeekNumber = 0;
	m_refRecurState.ulFirstDOW = 0;

	hr = m_ptrPattern->CalcMonthlyFirstDateTime(&ulFirstDateTimeMonths);
	if (hr != hrSuccess)
		goto exit;

	ulDays = bg::date_duration((g_BoostEpoch + bg::months(ulFirstDateTimeMonths)) - g_BoostEpoch).days();
	m_refRecurState.ulFirstDateTime = ulDays * 24*60;

exit:
	return hr;
}

HRESULT PatternToState::SetPatternRelativeMonthly(ULONG ulMonthlyPeriod, ULONG ulNum, ULONG ulDayOfWeekMask)
{
	HRESULT hr = hrSuccess;
	ULONG ulFirstDateTimeMonths;
	ULONG ulDays;

	m_refRecurState.ulRecurFrequency = RF_MONTHLY;
	m_refRecurState.ulPatternType = PT_MONTH_NTH;
	m_refRecurState.ulCalendarType = 0; // TODO: Verify
	m_refRecurState.ulPeriod = ulMonthlyPeriod;
	m_refRecurState.ulSlidingFlag = 0;
	m_refRecurState.ulWeekDays = ulDayOfWeekMask;
	m_refRecurState.ulDayOfMonth = 0;
	m_refRecurState.ulWeekNumber = ulNum;
	m_refRecurState.ulFirstDOW = 0;

	hr = m_ptrPattern->CalcMonthlyFirstDateTime(&ulFirstDateTimeMonths);
	if (hr != hrSuccess)
		goto exit;

	ulDays = bg::date_duration((g_BoostEpoch + bg::months(ulFirstDateTimeMonths)) - g_BoostEpoch).days();
	m_refRecurState.ulFirstDateTime = ulDays * 24*60;

exit:
	return hr;
}

HRESULT PatternToState::SetPatternAbsoluteYearly(ULONG ulYearlyPeriod, ULONG ulMonth, ULONG ulDayOfMonth)
{
	HRESULT hr = hrSuccess;
	ULONG ulFirstDateTimeMonths;
	ULONG ulDays;

	m_refRecurState.ulRecurFrequency = RF_YEARLY;
	m_refRecurState.ulPatternType = PT_MONTH;
	m_refRecurState.ulCalendarType = 0; // TODO: Verify
	m_refRecurState.ulPeriod = ulYearlyPeriod * 12;
	m_refRecurState.ulSlidingFlag = 0;
	m_refRecurState.ulWeekDays = 0;
	m_refRecurState.ulDayOfMonth = ulDayOfMonth;
	m_refRecurState.ulWeekNumber = 0;
	m_refRecurState.ulFirstDOW = 0;

	hr = m_ptrPattern->CalcMonthlyFirstDateTime(&ulFirstDateTimeMonths);
	if (hr != hrSuccess)
		goto exit;

	ulDays = bg::date_duration((g_BoostEpoch + bg::months(ulFirstDateTimeMonths)) - g_BoostEpoch).days();
	m_refRecurState.ulFirstDateTime = ulDays * 24*60;

exit:
	return hr;
}

HRESULT PatternToState::SetPatternRelativeYearly(ULONG ulYearlyPeriod, ULONG ulMonth, ULONG ulNum, ULONG ulDayOfWeekMask)
{
	HRESULT hr = hrSuccess;
	ULONG ulFirstDateTimeMonths;
	ULONG ulDays;

	m_refRecurState.ulRecurFrequency = RF_YEARLY;
	m_refRecurState.ulPatternType = PT_MONTH_NTH;
	m_refRecurState.ulCalendarType = 0; // TODO: Verify
	m_refRecurState.ulPeriod = ulYearlyPeriod * 12;
	m_refRecurState.ulSlidingFlag = 0;
	m_refRecurState.ulWeekDays = ulDayOfWeekMask;
	m_refRecurState.ulDayOfMonth = 0;
	m_refRecurState.ulWeekNumber = ulNum;
	m_refRecurState.ulFirstDOW = 0;

	hr = m_ptrPattern->CalcMonthlyFirstDateTime(&ulFirstDateTimeMonths);
	if (hr != hrSuccess)
		goto exit;

	ulDays = bg::date_duration((g_BoostEpoch + bg::months(ulFirstDateTimeMonths)) - g_BoostEpoch).days();
	m_refRecurState.ulFirstDateTime = ulDays * 24*60;

exit:
	return hr;
}

HRESULT PatternToState::SetRangeNoEnd(ULONG ulStartDate)
{
	HRESULT hr = hrSuccess;

	hr = m_ptrPattern->GetOccurrence(ulStartDate, &m_refRecurState.ulStartDate);
	if (hr != hrSuccess)
		goto exit;

	m_refRecurState.ulEndType = ET_NEVER;
	m_refRecurState.ulOccurrenceCount = 0;
	m_refRecurState.ulEndDate = 0x5ae980df;		// 'Fri Dec 31 23:59:00 4500'

exit:
	return hr;
}

HRESULT PatternToState::SetRangeNumbered(ULONG ulStartDate, ULONG ulOccurrences)
{
	HRESULT hr = hrSuccess;

	hr = m_ptrPattern->GetOccurrence(ulStartDate, &m_refRecurState.ulStartDate);
	if (hr != hrSuccess)
		goto exit;

	hr = m_ptrPattern->GetBounds(&m_refRecurState.ulStartDate,
	                             &m_refRecurState.ulEndDate);

	m_refRecurState.ulEndType = ET_NUMBER;
	m_refRecurState.ulOccurrenceCount = ulOccurrences;

exit:
	return hr;
}

HRESULT PatternToState::SetRangeEndDate(ULONG ulStartDate, ULONG ulEndDate)
{
	HRESULT hr = hrSuccess;
	UlongPtr ptrOccurrences;

	hr = m_ptrPattern->GetBounds(&m_refRecurState.ulStartDate,
	                             &m_refRecurState.ulEndDate);

	hr = m_ptrPattern->GetOccurrencesInRange(m_refRecurState.ulStartDate,
	                                         m_refRecurState.ulEndDate,
	                                         &m_refRecurState.ulOccurrenceCount,
	                                         &ptrOccurrences);
	if (hr != hrSuccess)
		goto exit;

	m_refRecurState.ulEndType = ET_DATE;

exit:
	return hr;
}


DEF_ULONGMETHOD(TRACE_MAPI, PatternToState, RecurrencePatternInspector, AddRef, (void))
DEF_ULONGMETHOD(TRACE_MAPI, PatternToState, RecurrencePatternInspector, Release, (void))
DEF_HRMETHOD(TRACE_MAPI, PatternToState, RecurrencePatternInspector, QueryInterface, (REFIID, refiid), (void **, lppInterface))

DEF_HRMETHOD(TRACE_MAPI, PatternToState, RecurrencePatternInspector, SetPatternDaily, (ULONG, ulDailyPeriod))
DEF_HRMETHOD(TRACE_MAPI, PatternToState, RecurrencePatternInspector, SetPatternWorkDays, (void))
DEF_HRMETHOD(TRACE_MAPI, PatternToState, RecurrencePatternInspector, SetPatternWeekly, (ULONG, ulFirstDayOfWeek), (ULONG, ulWeeklyPeriod), (ULONG, ulDayOfWeekMask))
DEF_HRMETHOD(TRACE_MAPI, PatternToState, RecurrencePatternInspector, SetPatternAbsoluteMonthly, (ULONG, ulMonthlyPeriod), (ULONG, ulDayOfMonth))
DEF_HRMETHOD(TRACE_MAPI, PatternToState, RecurrencePatternInspector, SetPatternRelativeMonthly, (ULONG, ulMonthlyPeriod), (ULONG, ulNum), (ULONG, ulDayOfWeekMask))
DEF_HRMETHOD(TRACE_MAPI, PatternToState, RecurrencePatternInspector, SetPatternAbsoluteYearly, (ULONG, ulYearlyPeriod), (ULONG, ulMonth), (ULONG, ulDayOfMonth))
DEF_HRMETHOD(TRACE_MAPI, PatternToState, RecurrencePatternInspector, SetPatternRelativeYearly, (ULONG, ulYearlyPeriod), (ULONG, ulMonth), (ULONG, ulNum), (ULONG, ulDayOfWeekMask))
DEF_HRMETHOD(TRACE_MAPI, PatternToState, RecurrencePatternInspector, SetRangeNoEnd, (ULONG, ulStartDate))
DEF_HRMETHOD(TRACE_MAPI, PatternToState, RecurrencePatternInspector, SetRangeNumbered, (ULONG, ulStartDate), (ULONG, ulOccurrences))
DEF_HRMETHOD(TRACE_MAPI, PatternToState, RecurrencePatternInspector, SetRangeEndDate, (ULONG, ulStartDate), (ULONG, ulEndDate))

