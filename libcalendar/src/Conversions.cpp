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
#include "Conversions.h"
#include "ConversionsInternal.h"
#include "CommonUtil.h"

namespace bg = boost::gregorian;

/**
 * Convert a timestamp represented in rtime, to a basedate, which
 * is just the timestamp for the beginning of the day in which
 * the timestamp occurrs.
 */
static inline ULONG BaseDateFromRTime(LONG rtTimestamp) {
	rtTimestamp -= (rtTimestamp % 1440);
	return rtTimestamp;
}


/////////////////
// Conversions.h
/////////////////
/**
 * Return the basedate for a timestamp represente in FILETIME.
 *
 * @param[in]	ftTimestamp		The timestamp to convert
 * @param[out]	lpulBaseDate	The returned basedate
 */
HRESULT BaseDateFromFileTime(FILETIME ftTimestamp, ULONG *lpulBaseDate)
{
	LONG rtTimestamp;

	if (lpulBaseDate == NULL)
		return MAPI_E_INVALID_PARAMETER;
	
	FileTimeToRTime(&ftTimestamp, &rtTimestamp);
	*lpulBaseDate = BaseDateFromRTime(rtTimestamp);

	return hrSuccess; 
}

/**
 * Return the basedate for a timestamp represente in unixtime.
 *
 * @param[in]	tTimestamp		The timestamp to convert
 * @param[out]	lpulBaseDate	The returned basedate
 */
HRESULT BaseDateFromUnixTime(time_t tTimestamp, ULONG *lpulBaseDate)
{
	HRESULT hr;
	LONG rtTimestamp;

	if (lpulBaseDate == NULL)
		return MAPI_E_INVALID_PARAMETER;
	
	hr = UnixTimeToRTime(tTimestamp, &rtTimestamp);
	if (hr == hrSuccess)
		*lpulBaseDate = BaseDateFromRTime(rtTimestamp);

	return hr;
}

/**
 * Create a basedate out of a year, month and day.
 *
 * @param[in]	ulYear			The year
 * @param[in]	ulMonth			The month (1..12)
 * @param[in]	ulDay			The day (1..31)
 * @param[out]	lpulBaseDate	The returned basedate
 */
HRESULT BaseDateFromYMD(ULONG ulYear, ULONG ulMonth, ULONG ulDay, ULONG *lpulBaseDate)
{
	HRESULT hr = hrSuccess;
	bg::date d;

	if (lpulBaseDate == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	try {
		d = bg::date(ulYear, ulMonth, ulDay);
	} catch (const std::out_of_range &) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}
	*lpulBaseDate = BaseDateFromBoostDate(d);

exit:
	return hr;
}

/**
 * Convert a basedate to a filetime structure. In reality this converters
 * an rtime to a filetime structure. The subtle distinction is that a
 * basedate is an rtime which is dividable by 1440 (1 day).
 * 
 * @param[in]	ulBaseDate		The basedate to convert.
 * @param[out]	lpftTimestamp	The filetime structure representing the basedate.
 */
HRESULT BaseDateToFileTime(ULONG ulBaseDate, FILETIME *lpftTimestamp)
{
	if (lpftTimestamp == NULL)
		return MAPI_E_INVALID_PARAMETER;
	
	RTimeToFileTime(ulBaseDate, lpftTimestamp);

	return hrSuccess;
}

/**
 * Convert a basedate to unixtime. In reality this converters
 * an rtime to a unixtime. The subtle distinction is that a
 * basedate is an rtime which is dividable by 1440 (1 day).
 * 
 * @param[in]	ulBaseDate		The basedate to convert.
 * @param[out]	lpftTimestamp	The filetime structure representing the basedate.
 */
HRESULT BaseDateToUnixTime(ULONG ulBaseDate, time_t *lptTimestamp)
{
	if (lptTimestamp == NULL)
		return MAPI_E_INVALID_PARAMETER;
	
	return RTimeToUnixTime(ulBaseDate, lptTimestamp);
}

/**
 * Convert a basedate to a year, month and day component. In
 * reality this converters an rtime to a year, month and day component.
 * The subtle distinction is that a basedate is an rtime which is
 * dividable by 1440 (1 day).
 * 
 * @param[in]	ulBaseDate		The basedate to convert.
 * @param[out]	lpftTimestamp	The filetime structure representing the basedate.
 */
HRESULT BaseDateToYMD(ULONG ulBaseDate, ULONG *lpulYear, ULONG *lpulMonth, ULONG *lpulDay)
{
	HRESULT hr = hrSuccess;
	bg::date::ymd_type ymd(1601, 1, 1);

	if (lpulYear == NULL || lpulMonth == NULL || lpulDay == NULL)
		return MAPI_E_INVALID_PARAMETER;

	try {
		ymd = BoostDateFromBaseDate(ulBaseDate).year_month_day();
	} catch (const std::out_of_range &) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}
	
	*lpulYear = ymd.year;
	*lpulMonth = ymd.month;
	*lpulDay = ymd.day;

exit:
	return hr;
}


/////////////////////////
// ConversionsInternal.h
/////////////////////////

static const bg::greg_weekday g_BoostWeekDays[] = {
	bg::Sunday, bg::Monday, bg::Tuesday, bg::Wednesday,
	bg::Thursday, bg::Friday, bg::Saturday
};

const bg::date g_BoostEpoch(1601, 1, 1);


/**
 * Convert a basedate (rtime) to a boost date instance.
 * 
 * @param[in]	ulBaseDate	The basedate to convert
 * @return	The boost date representing the base date
 */
bg::date BoostDateFromBaseDate(ULONG ulBaseDate) {
	return g_BoostEpoch + bg::date::duration_type(ulBaseDate / 1440);
}

/**
 * Convert a boost date instance to a basedate (rtime).
 * 
 * @param[in]	date	The boost date to convert.
 * @return	The basedate representing the boost date
 */
ULONG BaseDateFromBoostDate(const bg::date &date) {
	return (date - g_BoostEpoch).days() * 1440;
}

/**
 * Convert a MAPI weekday to a boost weekday.
 * 
 * @param[in]	ulWeekday	The MAPI weekday (0=sunday, 7=saturday)
 * @return	The boost weekday
 */
bg::greg_weekday BoostWeekdayFromMapiWeekday(ULONG ulWeekday) {
	ASSERT(ulWeekday < arraySize(g_BoostWeekDays));
	return g_BoostWeekDays[ulWeekday];
}
