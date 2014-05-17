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

#ifndef ECMAPITIMEZONE_H
#define ECMAPITIMEZONE_H

#include "ECUnknown.h"
#include <list>
#include <map>
#include <mapidefs.h>
#include "mapi_ptr.h"

/**
 *
 * Timezone handling functions
 *
 * This file is created specifically for handling, creating and parsing TZDEFINITION and TZREG-style
 * timezone definitions. We use a term here called 'local timestamp'. This doesn't really exist for
 * most people, so some explanation is needed:
 *
 * A normal timestamp (time_t), is *always* the seconds since 1 Jan, 1970, UTC. The timezone you are
 * in does not affect such a timestamp. When you output the same time_t value (say 123456) for different
 * timezones, you will see differing output for each timezone. DST changes the output of the timestamp
 * conversion, but it doesn't cause inconsistencies in the timestamp; if you take a timestamp N and add
 * 60, then you will have reached the point in time that is 1 minute later, irrespective of DST changes
 * or any other timezone changes.
 *
 * Now consider the situation in which you want to look at a recurring appointment, each day at 10:00 AM.
 * In this case, when we loop through a daily recurring item, we want the item to be at 10:00 AM every
 * day. One way to represent this is to say we use a 'local timestamp' which is defined as being the number
 * of seconds since 1 Jan 1970 _in the local time zone_. Therefore, the difference between a 'normal' UTC
 * timestamp and a 'local' timestamp is exactly equal to the timezone offset at that moment in seconds.
 *
 * The useful thing about local timestamps is that:
 * - You can skip to the 'next day at the same time' simply by adding 24*60*60 seconds
 * - You can use gmtime() and timegm() (yes!! gmtime!!) to decompose the timestamp into Y/M/d H/m/s 
 *
 * The bad thing about local timestamps is that:
 * - You can use gmtime() and timegm() (yes!! gmtime!!) to decompose the timestamp into Y/M/d H/m/s
 * - You can represent times that do not exist. Eg. you can make a timestamp for '28-10-12 02:30:00'
 *   which doesn't exist due to the DST change from 02:00 to 03:00 on that day.
 *
 * It is therefore of the utmost importance that you understand when you are handling a LOCAL timestamp
 * and when you are using a 'normal' timestamp! Unfortunately this is not easily enforced in the compiler.
 */

struct _TZSTRUCT {
	LONG bias;
	LONG stdbias;
	LONG dstbias;
	WORD wStdYear;
	SYSTEMTIME stStandardDate;
	WORD wDstYear;
	SYSTEMTIME stDaylightDate;
} __attribute__((packed));

typedef struct _TZSTRUCT TZSTRUCT;

class TimezoneRule : public ECUnknown
{
private:
    TimezoneRule(int bias, int stdbias, int dstbias, const SYSTEMTIME &standard, const SYSTEMTIME &daylight);
    virtual ~TimezoneRule();
    
    bool IsDST(FILETIME localtimestamp, bool std);

    FILETIME FromSystemTime(int year, SYSTEMTIME stime);
    
    int m_bias;
    int m_stdbias;
    int m_dstbias;
    
    SYSTEMTIME m_daylight;
    SYSTEMTIME m_standard;

public:
    // Parse from a TZSTRUCT data blob
    static HRESULT FromBlob(ULONG cSize, BYTE *lpBlob, TimezoneRule **lppTZ);
    // Parse from a TZREG data blob
    static HRESULT FromTZREGBlob(ULONG cSize, BYTE *lpBlob, TimezoneRule **lppTZ);
    // Parse from an on-disk timezone line
    static HRESULT FromLine(const char *line, TimezoneRule **lpTZRule);
	// "Detect" system rule
	static HRESULT FromSystem(TimezoneRule **lppTZRule);
	// From rule data
	static HRESULT FromRuleData(int bias, int stdbias, int dstbias,
	                            const SYSTEMTIME &standard, const SYSTEMTIME &daylight,
	                            TimezoneRule **lppTZRule);

    // Output the rule as a TZSTRUCT/TZREG
    HRESULT ToTZSTRUCT(ULONG *lpSize, BYTE **lpBlob);
    HRESULT ToTZSTRUCT(VOID *lpBase, ULONG *lpSize, BYTE **lpBlob);
    HRESULT ToTZREG(BYTE *lpBlob);
    
    // Compare this rule to another
    int Compare(const TimezoneRule *lpOther, unsigned int ulMatchLevel);

    // Convert the given 'local timestamp' to a UTC timestamp using the rules in this timezone
    HRESULT ToUTC(time_t tLocalTimestamp, time_t *tUTC);
    HRESULT ToUTC(FILETIME tLocalTimestamp, FILETIME *tUTC);

    // Convert the given UTC timestamp to a local timestamp using the rules in this timezone
    HRESULT FromUTC(time_t tUTC, time_t *tLocalTimeStamp);
    HRESULT FromUTC(FILETIME tUTC, FILETIME *tLocalTimeStamp);
    
    // Get the data that defines the rule.
    HRESULT GetRuleData(int *lpBias, int *lpStdbias, int *lpDstbias,
						SYSTEMTIME *lpStandard, SYSTEMTIME *lpDaylight);
};

class TimezoneDefinition : public ECUnknown
{
public:
    TimezoneDefinition(const GUID& guid, const std::wstring &strName, const std::wstring &strDisplay, std::map<unsigned int, TimezoneRule *>& mapDST);
    virtual ~TimezoneDefinition();

private:
    unsigned int FindDSTYear(FILETIME timestamp);
        
    const std::wstring m_wstrName;	// Eg 'W. Europe Standard Time'
    const std::wstring m_wstrDisplay; // Eg '(UTC+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna'
    const GUID m_guid;
    
    const std::map<unsigned int, TimezoneRule *> m_mapRules;

public:
    // Read a TZ definition from disk
    static HRESULT FromDisk(const std::string& strPath, const std::wstring &strName, TimezoneDefinition **lppTZ);
    // Read a TZ definition from MAPI
    static HRESULT FromBlob(ULONG cSize, BYTE *lpBlob, TimezoneDefinition **lppTZ);

    // Get the name
    HRESULT GetName(std::wstring *lpwstrName);
    // Get the display name
    HRESULT GetDisplayName(std::wstring *lpwstrDisplay);
    // Get the GUID
    HRESULT GetGUID(GUID *lpGUID);
    
    // Output the entire definition as a TZDEFINITION
    HRESULT ToTZDEFINITION(time_t effective, time_t recur, ULONG *lpSize, BYTE **lpBlob);
    HRESULT ToTZDEFINITION(time_t effective, time_t recur, VOID *lpBase, ULONG *lpSize, BYTE **lpBlob);
    HRESULT ToTZDEFINITION(FILETIME effective, FILETIME recur, ULONG *lpSize, BYTE **lpBlob);
    HRESULT ToTZDEFINITION(FILETIME effective, FILETIME recur, VOID *lpBase, ULONG *lpSize, BYTE **lpBlob);

    // Output the rule for the specified year as a TZREG
    HRESULT ToTZSTRUCT(time_t timestamp, ULONG *lpSize, BYTE **lpBlob);
    HRESULT ToTZSTRUCT(time_t timestamp, VOID *lpBase, ULONG *lpSize, BYTE **lpBlob);
    HRESULT ToTZSTRUCT(FILETIME timestamp, ULONG *lpSize, BYTE **lpBlob);
    HRESULT ToTZSTRUCT(FILETIME timestamp, VOID *lpBase, ULONG *lpSize, BYTE **lpBlob);
    
    // Convert the given 'local timestamp' to a UTC timestamp using the rules in this timezone
    HRESULT ToUTC(time_t tLocalTimestamp, time_t *tUTC);
    HRESULT ToUTC(FILETIME tLocalTimestamp, FILETIME *tUTC);

    // Convert the given UTC timestamp to a local timestamp using the rules in this timezone
    HRESULT FromUTC(time_t tUTC, time_t *tLocalTimeStamp);
    HRESULT FromUTC(FILETIME tUTC, FILETIME *tLocalTimeStamp);

    // Get a rule effective at a certain moment in time
    HRESULT GetEffectiveRule(time_t timestamp, bool bLocal, TimezoneRule **lppRule);
    HRESULT GetEffectiveRule(FILETIME timestamp, bool bLocal, TimezoneRule **lppRule);

    // Match a rule with any of the DST rules for this timezone
    unsigned int ContainsRule(const TimezoneRule *lpRule, unsigned int ulMatchLevel);
};

// Database functions
HRESULT HrGetTZDefByName(const std::wstring &strTZName, TimezoneDefinition **lppTZ);
HRESULT HrGetTZNameByRule(time_t timestamp, const TimezoneRule *lpRule, unsigned int ulMatchLevel, std::wstring *lpStrName);
HRESULT HrGetTZNameByRule(FILETIME timestamp, const TimezoneRule *lpRule, unsigned int ulMatchLevel, std::wstring *lpStrName);
HRESULT HrGetTZNames(unsigned int *lpcValues, wchar_t ***lpszNames);
HRESULT HrGetTZNameFromOlsonName(const std::string &strOlson, std::wstring *lpwstrTZName);
HRESULT HrDetectSystemTZName(std::wstring *lpwstrTZName);

enum eTZ { TZStartDisplay = 1, TZEndDisplay, TZRecur };
HRESULT HrGetTZDefFromMessage(IMessage *lpMessage, eTZ which, TimezoneDefinition **lppTZ);

typedef mapi_object_ptr<TimezoneRule> TimezoneRulePtr;
typedef mapi_object_ptr<TimezoneDefinition> TimezoneDefinitionPtr;


#endif // ECMAPITIMEZONE_H
