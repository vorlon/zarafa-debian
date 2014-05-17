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
#include "ECMAPITimezone.h"
#include "mapi_ptr.h"
#include "ECGuid.h"
#include "stringutil.h"
#include "threadutil.h"

#include <mapidefs.h>
#include <mapicode.h>
#include <mapix.h>

#include <map>

#include <boost/filesystem.hpp>
namespace bfs = boost::filesystem;
#include "boost_compat.h"

#include <pthread.h>

#include "charset/convert.h"
#include "CommonUtil.h"
#include <mapiguidext.h>
#include "namedprops.h"

#define TZ_BIN_VERSION_MAJOR 0x0002
#define TZ_BIN_VERSION_MINOR 0x0001

#define TZDEFINITION_FLAG_VALID_GUID 0x0001
#define TZDEFINITION_FLAG_VALID_KEYNAME 0x0002

#define TZRULE_FLAG_RECUR_CURRENT_TZREG 0x0001
#define TZRULE_FLAG_EFFECTIVE_TZREG 0x0002

// generated in OlsonNames.cpp
extern std::map<std::string, std::wstring> mapWindowsOlsonNames;

template<typename T>
void release_second (typename T::value_type i) {
    i.second->Release();
}

static inline FILETIME operator-(const FILETIME &ft, __int64 value) {
    ULARGE_INTEGER uli = *(ULARGE_INTEGER *)&ft;
    uli.QuadPart -= value;
    return *(FILETIME *)&uli;
}

static inline FILETIME& operator-=(FILETIME &ft, __int64 value) {
    ft = ft - value;
    return ft;
}

static inline FILETIME operator+(const FILETIME &ft, __int64 value) {
    return ft - (-value);
}

static inline FILETIME& operator+=(FILETIME &ft, __int64 value) {
    ft = ft + value;
    return ft;
}

// TZDEFINITION structure components
typedef struct {
    BYTE bMajorVersion;
    BYTE bMinorVersion;
    WORD cbHeader;
    WORD wFlags;
} TZDEF_HEADER;

// GUID if TZDEFINITION_FLAG_VALID_GUID
// WORD cchKeyName if TZDEFINITION_FLAG_VALID_KEYNAME
// WCHAR rgchKeyName if TZDEFINITION_FLAG_VALID_KEYNAME

// WORD cRules

// The following struct cRules times
struct _TZDEF_RULE {
    BYTE bMajorVersion;
    BYTE bMinorVersion;
    WORD cbRule;
    WORD wFlags;
    SYSTEMTIME stStart;
    TZREG rule;
} __attribute__((packed));

typedef struct _TZDEF_RULE TZDEF_RULE;

// TZSTRUCT in header, needs to be public


// Singleton class for Timezone database
class MAPITimezoneDB {
public:
    /**
     * Get singleton MAPITimezoneDB instance
     */
    static HRESULT getInstance(MAPITimezoneDB **lppTZDB) {
        HRESULT hr = hrSuccess;
        scoped_lock lock(mTimezoneDB);
        
        if(!lpTimezoneDB) {
            lpTimezoneDB = new MAPITimezoneDB();
            
            hr = lpTimezoneDB->HrLoadTimezones();
            if(hr != hrSuccess) {
                delete lpTimezoneDB;
                lpTimezoneDB = NULL;
                goto exit;
            }
            atexit(&destroy);
        }
        
        *lppTZDB = lpTimezoneDB;
exit:        
        return hr;
    }

public:
    /*
     * Get timezone definition by name, this is the most common.
     *
     * @param[in] strTZName Name of timezone to get (eg 'W. Europe Standard Time')
     * @param[out] lppTZ TimezoneDefinition object
     * @return result
     */
    HRESULT HrGetTZDefByName(const std::wstring &strTZName, TimezoneDefinition **lppTZ)
    {
        HRESULT hr = hrSuccess;
        std::map<std::wstring, TimezoneDefinition *>::iterator i;
        std::map<std::wstring, std::wstring>::iterator r;
        
        i = m_mapTimezones.find(strTZName);
        if(i != m_mapTimezones.end()) {
            *lppTZ = i->second;
            (*lppTZ)->AddRef();
        } else {
            r = m_mapReverse.find(strTZName);
            if (r != m_mapReverse.end()) {
                i = m_mapTimezones.find(r->second);
                if(i != m_mapTimezones.end()) {
                    *lppTZ = i->second;
                    (*lppTZ)->AddRef();
                    goto exit;
                }
            }
            hr = MAPI_E_NOT_FOUND;
        }
exit:
        return hr;
    }
    
    /*
     * Get timezone definition by looking at a timezone rule. This may match multiple timezones.
     *
     * Disambiguation is performed by:
     *
     * 1) First match with 'current' timezone rule (so: ignore DST rules for other years). This may still
     *    result in a match that is not completely correct by name, but the current rule should be ok
     * 2) If step 1 fails, then attempt to match timezone in the dst rules of timezones. The search is done
     *    simply alphabetically; there may be a more recent timezone match somewhere else, but we will ignore
     *    that.
     *
     * ulMatchLevel can be:
     *
     * 0: Exact match
     * 1: Match everything except DST switch time
     * 2: Match everything except DST switch time & dow
     * 3: Match everything except DST switch time & dow & weeknr
     * 4: Match only STD offset + DST offset
     * 5: Match only STD offset
     *
     * @param[in] timestamp Timestamp to attempt to match the rule at
     * @param[in] lpRule Rule to match
     * @param[in] ulMatchLevel Fuzzy-matching (0=full match, 5=bias only)
     * @param[out] lpStrName Name of matching TZ
     * @return result
     */
    HRESULT HrGetTZNameByRule(FILETIME timestamp, const TimezoneRule *lpRule, unsigned int ulMatchLevel, std::wstring *lpStrName)
    {
        HRESULT hr = hrSuccess;;
        std::map<std::wstring, TimezoneDefinition *>::iterator i;
        TimezoneRulePtr lpThisRule;
        bool found = false;
        
        for(i = m_mapTimezones.begin(); !found && i != m_mapTimezones.end(); i++) {
            hr = i->second->GetEffectiveRule(timestamp, false, &lpThisRule);
            if(hr != hrSuccess)
                goto exit;
            
            if(lpThisRule->Compare(lpRule, ulMatchLevel) == 0) {
                *lpStrName = i->first;
                found = true;
            }
        }

        for(i = m_mapTimezones.begin(); !found && i != m_mapTimezones.end(); i++) {
            if(i->second->ContainsRule(lpRule, ulMatchLevel) > 0) {
                *lpStrName = i->first;
                found = true;
            }
        }
        
        if(!found) {
            hr = MAPI_E_NOT_FOUND;
            goto exit;
        }
exit:        
            
        return hr;
    }
    
    /**
     * Get a list of all TZ names in the database
     *
     * @param[out] lpcValues Number of values output in lppszNames
     * @param[out] lppszNames Array of wchar_t strings. Should be freed by the caller with a single MAPIFreeBuffer() call
     * @return result
     */
    HRESULT HrGetTZNames(unsigned int *lpcValues, wchar_t ***lppszNames) {
        HRESULT hr = hrSuccess;
        std::map<std::wstring, TimezoneDefinition *>::iterator i;
        unsigned int cValues = m_mapTimezones.size();
        wchar_t **lpszNames = NULL;
        std::wstring name;
        int n = 0;
        
        hr = MAPIAllocateBuffer(cValues * sizeof(wchar_t *), (void **)&lpszNames);
        if(hr != hrSuccess)
            goto exit;
        
        for(i = m_mapTimezones.begin(); i != m_mapTimezones.end(); i++, n++) {
            hr = i->second->GetName(&name);
            if(hr != hrSuccess)
                goto exit;
                
            hr = MAPIAllocateMore((name.size()+1) * sizeof(wchar_t), lpszNames, (void **)&lpszNames[n]);
            if(hr != hrSuccess)
                goto exit;
                
            wcscpy(lpszNames[n], name.c_str());

        }
        
        *lpcValues = cValues;
        *lppszNames = lpszNames;
        
exit:
        return hr;
    }
    

private:
    static void destroy() {
        delete lpTimezoneDB;
        lpTimezoneDB = NULL;
    }
    
    ~MAPITimezoneDB() {
        for_each(m_mapTimezones.begin(), m_mapTimezones.end(), release_second<std::map<std::wstring, TimezoneDefinition *> >);
    }

    /**
     * Load all timezones from disk
     *
     * Normally reads from TIMEZONEDIR (/usr/share/zarafa/timezones), but can be overridden
     * by setting environment variable ZARAFA_TIMEZONEDIR. Each file represents one timezone, and
     * the name of the file is significant since it is the string identifier of that timezone.
     *
     * @return result
     */
    HRESULT HrLoadTimezones()
    {
        HRESULT hr = hrSuccess;
        bfs::path		indexdir;
        bfs::directory_iterator ilast;
        
        char *tzdir = getenv("ZARAFA_TIMEZONEDIR");
        
        if(!tzdir)
            tzdir = TIMEZONEDIR;

        try {        
            for (bfs::directory_iterator index(tzdir); index != ilast; index++) {
                std::string strFilename;
                std::string strPath;

                if (is_directory(index->status()))
                    continue;
                    
                strFilename = filename_from_path(index->path());
                strPath = path_to_string(index->path());
                
                HrLoadTimezone(strPath, convert_to<std::wstring>(strFilename, rawsize(strFilename), "UTF-8")); // Ignore errors
            }
        } catch (bfs::filesystem_error &e) {
            hr = MAPI_E_NOT_FOUND;
            goto exit;
        }
        
exit:        
        return hr;
    }
    
    /**
     * Load a single timezone from disk and add it to the in-memory database
     *
     * @param[in] strFilename Filename of timezone definition to load
     * @param[in] strName Wide-char name of the timezone
     * @return result
     */
    HRESULT HrLoadTimezone(const std::string& strFilename, const std::wstring& strName)
    {
        HRESULT hr = hrSuccess;
        TimezoneDefinition *lpTZ = NULL;
        std::wstring wstrDisplay;
        
        hr = TimezoneDefinition::FromDisk(strFilename, strName, &lpTZ);
        if(hr != hrSuccess)
            goto exit;
            
        m_mapTimezones[strName] = lpTZ;
        lpTZ->GetDisplayName(&wstrDisplay);
        m_mapReverse[wstrDisplay] = strName;
        lpTZ->AddRef();
        
exit:
        if(lpTZ)
            lpTZ->Release();
            
        return hr;
    }
        
    std::map<std::wstring, TimezoneDefinition *> m_mapTimezones;
    std::map<std::wstring, std::wstring> m_mapReverse; // displayname to windows name
    
private:
    static MAPITimezoneDB *lpTimezoneDB;
    static pthread_mutex_t mTimezoneDB;
};

// Our singleton timezone DB
MAPITimezoneDB* MAPITimezoneDB::lpTimezoneDB = NULL;
pthread_mutex_t MAPITimezoneDB::mTimezoneDB = PTHREAD_MUTEX_INITIALIZER;

/**
 * Get Year part (eg 2012) of a timestamp
 * 
 * No timezone correction is applied to the timestamp
 *
 * @param[in] tLocalTimestamp
 * @return year
 */
static unsigned int GetYear(FILETIME ftLocalTimestamp)
{
    time_t tLocalTimestamp = FileTimeToUnixTime(ftLocalTimestamp.dwHighDateTime,
                                                ftLocalTimestamp.dwLowDateTime);
    struct tm t;
    
    gmtime_r(&tLocalTimestamp, &t);
    
    return t.tm_year + 1900;
}

/**
 * Get a timezone definition based on its name
 *
 * Looks for the timezone definition in the timezone database. Will load the database
 * from disk on the first call of the process.
 *
 * @param[in] strTZName Timezone name (must exactly match - case sensitive)
 * @param[out] lppTZ Timezone
 * @return result
 */
HRESULT HrGetTZDefByName(const std::wstring &strTZName, TimezoneDefinition **lppTZ)
{
    HRESULT hr = hrSuccess;
    MAPITimezoneDB *lpTimezoneDB = NULL;
    
    hr = MAPITimezoneDB::getInstance(&lpTimezoneDB);
    if(hr != hrSuccess)
        goto exit;
    
    hr = lpTimezoneDB->HrGetTZDefByName(strTZName, lppTZ);
exit:    
    return hr;    
}

/** 
 * Returns a TimezoneDefinition class created from the timezone
 * definition found in a MAPI message.
 * 
 * @param[in] lpMessage Uses this message to get timezone information
 * @param[in] which enum of which timezone to use
 * @param[out] lppTZ Timezone
 * 
 * @return MAPI error code
 */
HRESULT HrGetTZDefFromMessage(IMessage *lpMessage, eTZ which, TimezoneDefinition **lppTZ)
{
	HRESULT hr = hrSuccess;
	TimezoneDefinitionPtr ptrTZDef;
	ULONG cValues;
	SPropArrayPtr ptrProps;
	TimezoneRulePtr ptrTZRule;
	std::wstring wstrName;
	time_t start = 0;

	PROPMAP_START
		PROPMAP_NAMED_ID(TIMEZONESTRUCT,			PT_BINARY,	PSETID_Appointment, dispidTimeZoneData)
		PROPMAP_NAMED_ID(TZDEFSTART,				PT_BINARY,	PSETID_Appointment, dispidApptTZDefStartDisplay)
		PROPMAP_NAMED_ID(TZDEFEND,					PT_BINARY,	PSETID_Appointment, dispidApptTZDefEndDisplay)
		PROPMAP_NAMED_ID(TZDEFRECUR,				PT_BINARY,	PSETID_Appointment, dispidApptTZDefRecur)
		PROPMAP_NAMED_ID(START,                     PT_SYSTIME, PSETID_Appointment, dispidApptStartWhole)
	PROPMAP_INIT(lpMessage);

	{
		SizedSPropTagArray(5, sptaTZDefPropTags) = { 5, { PROP_TIMEZONESTRUCT, PROP_TZDEFSTART, PROP_TZDEFEND, PROP_TZDEFRECUR, PROP_START } };

		hr = lpMessage->GetProps((LPSPropTagArray)&sptaTZDefPropTags, 0, &cValues, &ptrProps);
		if (FAILED(hr))
			goto exit;
	}

	if (which != TZStartDisplay || which != TZEndDisplay || which != TZRecur)
		which = TZRecur;

	// try the large timezone definition
	if (PROP_TYPE(ptrProps[which].ulPropTag) == PT_BINARY) {
		hr = TimezoneDefinition::FromBlob(ptrProps[which].Value.bin.cb, ptrProps[which].Value.bin.lpb, &ptrTZDef);
		if (hr == hrSuccess) {
			*lppTZ = ptrTZDef.release();
			goto exit;
		}
	}

	// fallback to old struct
	if (ptrProps[0].ulPropTag == PROP_TIMEZONESTRUCT) {
		hr = TimezoneRule::FromBlob(ptrProps[0].Value.bin.cb, ptrProps[0].Value.bin.lpb, &ptrTZRule);
		if (hr != hrSuccess)
			goto exit;

		// leave start at 0 if not found.
		if (PROP_TYPE(ptrProps[4].ulPropTag) == PT_SYSTIME)
			hr = FileTimeToUnixTime(ptrProps[4].Value.ft, &start); 
		if(hr != hrSuccess) 
			goto exit;

		hr = HrGetTZNameByRule(start, ptrTZRule, 0, &wstrName);
		if (hr != hrSuccess)
			goto exit;

		hr = HrGetTZDefByName(wstrName, &ptrTZDef);
		if (hr != hrSuccess)
			goto exit;

		*lppTZ = ptrTZDef.release();
		goto exit;
	}

	hr = MAPI_E_NOT_FOUND;

exit:
	return hr;
}


/**
 * Get a timezone definition based on a rule
 *
 * @param[in] timestamp Timestamp to use to match the DST rule
 * @param[in] lpRule The rule to match
 * @param[in] ulMatchLevel Fuzzy-match level (0=exact match, 5=bias only)
 * @param[out] lpStrName Name of found TZ
 * @return result
 */
HRESULT HrGetTZNameByRule(FILETIME timestamp, const TimezoneRule * lpRule, unsigned int ulMatchLevel, std::wstring *lpStrName)
{
    HRESULT hr = hrSuccess;
    MAPITimezoneDB *lpTimezoneDB = NULL;
    
    hr = MAPITimezoneDB::getInstance(&lpTimezoneDB);
    if(hr != hrSuccess)
        goto exit;
    
    hr = lpTimezoneDB->HrGetTZNameByRule(timestamp, lpRule, ulMatchLevel, lpStrName);
exit:    
    return hr;
}

/**
 * Get a timezone definition based on a rule
 *
 * @param[in] timestamp Timestamp to use to match the DST rule
 * @param[in] lpRule The rule to match
 * @param[in] ulMatchLevel Fuzzy-match level (0=exact match, 5=bias only)
 * @param[out] lpStrName Name of found TZ
 * @return result
 */
HRESULT HrGetTZNameByRule(time_t timestamp, const TimezoneRule *lpRule, unsigned int ulMatchLevel, std::wstring *lpStrName)
{
    FILETIME ft;

    UnixTimeToFileTime(timestamp, &ft);
    return HrGetTZNameByRule(ft, lpRule, ulMatchLevel, lpStrName);
}

/**
 * Get a list of all timezone names
 *
 * @param[out] lpcValues Number of strings in lpszNames
 * @param[out] lpszNames Array of wchar_t strings
 * @return result
 */
HRESULT HrGetTZNames(unsigned int *lpcValues, wchar_t ***lpszNames)
{
    HRESULT hr = hrSuccess;
    MAPITimezoneDB *lpTimezoneDB = NULL;
    
    hr = MAPITimezoneDB::getInstance(&lpTimezoneDB);
    if(hr != hrSuccess)
        goto exit;
    
    hr = lpTimezoneDB->HrGetTZNames(lpcValues, lpszNames);
exit:    
    
    return hr;
}

HRESULT HrGetTZNameFromOlsonName(const std::string &strOlson, std::wstring *lpwstrTZName)
{
	std::map<std::string, std::wstring>::iterator i = mapWindowsOlsonNames.find(strOlson);
	if (i == mapWindowsOlsonNames.end())
		return MAPI_E_NOT_FOUND;
	*lpwstrTZName = i->second;
	return hrSuccess;
}

/** 
 * Try to autodetect the systems timezone, and return the MAPI name
 * for it.
 * 
 * @param[out] lpwstrTZName Detected name
 * 
 * @return MAPI Error code
 */
HRESULT HrDetectSystemTZName(std::wstring *lpwstrTZName)
{
	HRESULT hr = hrSuccess;
	TimezoneRulePtr ptrTZRule;

	hr = TimezoneRule::FromSystem(&ptrTZRule);
	if (hr != hrSuccess)
		goto exit;

	hr = HrGetTZNameByRule(time(NULL), ptrTZRule, 5, lpwstrTZName);

exit:
	return hr;
}


/**
 * Construct a TZ rule
 *
 * @param[in] bias Normal bias
 * @param[in] stdbias STD bias (normally 0)
 * @param[in] dstbias STD bias (normally -60 or 0)
 * @param[in] standard Moment of the start of STD time
 * @param[in] daylight Moment of the start of DST time
 */
TimezoneRule::TimezoneRule(int bias, int stdbias, int dstbias, const SYSTEMTIME &standard, const SYSTEMTIME &daylight)
{
    m_bias = bias;
    m_stdbias = stdbias;
    m_dstbias = dstbias;
    
    m_standard = standard;
    m_daylight = daylight;
}

TimezoneRule::~TimezoneRule()
{
}

/**
 * Compare with another rule
 * ulMatchLevel can be:
 *
 * 0: Exact match
 * 1: Match everything except DST switch time
 * 2: Match everything except DST switch time & dow
 * 3: Match everything except DST switch time & dow & weeknr
 * 4: Match only STD offset + DST offset
 * 5: Match only STD offset
 *
 * @param[in] lpOther Other rule to compare to
 * @param[in] ulMatchLevel Degree of matching
 * @return 0 if equal, 1 otherwise
 */
 
int TimezoneRule::Compare(const TimezoneRule *lpOther, unsigned int ulMatchLevel)
{
    if(ulMatchLevel <= 5) {
        if(m_bias != lpOther->m_bias)
            return 1;
    }
    if(ulMatchLevel <= 4) {
        if(m_dstbias != lpOther->m_dstbias)
            return 1;
    }
    if(ulMatchLevel <= 3) {
        if(m_standard.wMonth != lpOther->m_standard.wMonth || m_daylight.wMonth != lpOther->m_daylight.wMonth)
            return 1;
    }
    if(ulMatchLevel <= 2) {
        if(m_standard.wDay != lpOther->m_standard.wDay || m_daylight.wDay != lpOther->m_daylight.wDay)
            return 1;
    }
    if(ulMatchLevel <= 1) {
        if(m_standard.wDayOfWeek != lpOther->m_standard.wDayOfWeek || m_daylight.wDayOfWeek != lpOther->m_daylight.wDayOfWeek)
            return 1;
    }
    if(ulMatchLevel == 0) {
        if(m_standard.wHour != lpOther->m_standard.wHour || m_daylight.wHour != lpOther->m_daylight.wHour ||
            m_standard.wMinute != lpOther->m_standard.wMinute || m_daylight.wMinute != lpOther->m_daylight.wMinute ||
            m_standard.wSecond != lpOther->m_standard.wSecond || m_daylight.wSecond != lpOther->m_daylight.wSecond ||
            m_standard.wMilliseconds != lpOther->m_standard.wMilliseconds || m_daylight.wMilliseconds != lpOther->m_daylight.wMilliseconds)
            return 1;
            
    }
    
    return 0;
}

/**
 * Convert a local timestamp to a UTC timestamp. Note that if the local
 * timestamp is ambiguous, (because, say, 2:30 AM occurs twice due to a DST switch from 03:00 to 02:00),
 * then this function returns the first occurrence that the time is 2:30 AM. If the time specified is invalid
 * (eg. 2.30 AM when we jump from 2:00 AM to 3:00 AM), then 2:30 is interpreted as if the DST switch had not
 * happened, effectively giving the same result as 3:30 AM.
 *
 * @param[in] tLocalTimestamp Local timestamp to convert
 * @param[out] lptUTC UTC output timestamp
 * @return result
 */
HRESULT TimezoneRule::ToUTC(time_t tLocalTimestamp, time_t *lptUTC)
{
    HRESULT hr;
    FILETIME ftLocalTimestamp;
    FILETIME ftUTC;

    UnixTimeToFileTime(tLocalTimestamp, &ftLocalTimestamp);

    hr = ToUTC(ftLocalTimestamp, &ftUTC);
    if (hr == hrSuccess)
        *lptUTC = FileTimeToUnixTime(ftUTC.dwHighDateTime, ftUTC.dwLowDateTime);

    return hr;
}

/**
 * Convert a local timestamp to a UTC timestamp. Note that if the local
 * timestamp is ambiguous, (because, say, 2:30 AM occurs twice due to a DST switch from 03:00 to 02:00),
 * then this function returns the first occurrence that the time is 2:30 AM. If the time specified is invalid
 * (eg. 2.30 AM when we jump from 2:00 AM to 3:00 AM), then 2:30 is interpreted as if the DST switch had not
 * happened, effectively giving the same result as 3:30 AM.
 *
 * @param[in] tLocalTimestamp Local timestamp to convert
 * @param[out] lptUTC UTC output timestamp
 * @return result
 */
HRESULT TimezoneRule::ToUTC(FILETIME tLocalTimestamp, FILETIME *lptUTC)
{
    *lptUTC = tLocalTimestamp + 10000000ll * 60 * (m_bias + m_stdbias + (IsDST(tLocalTimestamp, false) ? m_dstbias : 0));

    return hrSuccess;
}

/**
 * Convert a UTC timestamp to a local timestamp
 *
 * @param[in] tUTC UTC timestamp to convert
 * @param[out] lptLocalTimestamp Local output timestamp
 * @return result
 */
HRESULT TimezoneRule::FromUTC(time_t tUTC, time_t *lptLocalTimestamp)
{
    HRESULT hr;
    FILETIME ftUTC;
    FILETIME ftLocalTimestamp;
    
    UnixTimeToFileTime( tUTC, &ftUTC);
    
    hr = FromUTC(ftUTC, &ftLocalTimestamp);
    if (hr == hrSuccess)
        *lptLocalTimestamp = FileTimeToUnixTime(ftLocalTimestamp.dwHighDateTime, ftLocalTimestamp.dwLowDateTime);

    return hr;
}

/**
 * Convert a UTC timestamp to a local timestamp
 *
 * @param[in] tUTC UTC timestamp to convert
 * @param[out] lptLocalTimestamp Local output timestamp
 * @return result
 */
HRESULT TimezoneRule::FromUTC(FILETIME tUTC, FILETIME *lptLocalTimestamp)
{
    FILETIME tSTDTimestamp;
    
    // First, do the conversion to STD time, which we can use to check if DST should be applied
    tSTDTimestamp = tUTC - 10000000ll * 60 * (m_bias + m_stdbias);

    // Now do the final conversion
    *lptLocalTimestamp = tUTC - 10000000ll * 60 * (m_bias + m_stdbias + (IsDST(tSTDTimestamp, true) ? m_dstbias : 0));

    return hrSuccess;
}

/**
 * Convert SYSTEMTIME 'Nth WEEKDAY of MONTH in year YEAR' format to local timestamp, assuming
 * times in stime are local times.
 *
 * @param[in] year Year to apply stime to
 * @param[in] stime Time of year to convert (Nth DOW of MONTH at HH:MM:ss)
 * @result local timestamp
 */
FILETIME TimezoneRule::FromSystemTime(int year, SYSTEMTIME stime)
{
    struct tm t;
    time_t ts;
    FILETIME ft;
    
    ASSERT(stime.wMonth <= 12);
    ASSERT(stime.wDayOfWeek <= 6);
    
    memset(&t, 0, sizeof(t));
    t.tm_sec = stime.wSecond;                                                                                                                            
    t.tm_min = stime.wMinute;
    t.tm_hour = stime.wHour;
    t.tm_mday = 1;
    t.tm_mon = stime.wMonth-1;
    t.tm_year = year - 1900;
    
    // Convert back-and-forth to get tm_wday, and set ts
    ts = timegm(&t);
    gmtime_r(&ts, &t);
    
    // Go to the first wday (DOW) selected in ts
    if(stime.wDayOfWeek < t.tm_wday)
        stime.wDayOfWeek += 7;
    ASSERT(t.tm_wday <= stime.wDayOfWeek);
    ts += (stime.wDayOfWeek - t.tm_wday) * 24 * 60 * 60;
    
    // Go the the Nth DOW we want (we are already at 1, so subtract 1) in ts
    ts += (stime.wDay - 1) * 7 * 24 * 60 * 60;
    
    // Make sure we can't go beyond the end of the month, if so then start
    // reversing weeks until we get back to the right month. This only happens
    // if stime.wDay > 4. Specifying another above 5 is useless but supported
    gmtime_r(&ts, &t);
    while(t.tm_mon != stime.wMonth-1) {
        ASSERT(stime.wDay > 4);
        ts -= 7 * 24 * 60 * 60;
        gmtime_r(&ts, &t);
    }
    
    UnixTimeToFileTime(ts, &ft);
    return ft;
}

/**
 * Find out if DST is applied at a certain time stamp moment. tTimestamp
 * can be either a local timestamp (isSTD = false), or an STD timestamp.
 *
 * @param[in] tTimestamp Local or DST timestamp to check if DST is to be applied
 * @param[in] isSTD TRUE if tTimestamp represents an STD timestamp
 * @return TRUE if DST is to be applied, FALSE otherwise
 */
bool TimezoneRule::IsDST(FILETIME tTimestamp, bool isSTD)
{
    FILETIME daylight;
    FILETIME standard;
    
    if(m_dstbias == 0 || m_daylight.wMonth == 0 || m_standard.wMonth == 0) {
        // No DST in this timezone rule
        return false;
    }

    /* The observant reader will see that we're always passing tTimestamp as a local
     * timestamp to GetYear, while it may be an STD timestamp. However, there are no
     * circumstances in which this would make a difference unless DST would come into effect
     * around the end of the year (eg 31-12-XXXX 23:00). Since DST changes don't happen
     * around that time, this is ignored for now.
     */
    daylight = FromSystemTime(GetYear(tTimestamp), m_daylight);
    standard = FromSystemTime(GetYear(tTimestamp), m_standard);

    /* Daylight savings time can start at, say, 02:00, at which point the clock will progress to
     * say 03:00. This means that times between 02:00 and 03:00 are invalid. Normally, the time
     * 02:30 would therefore be handled as being *after* DST, since 02:30 > 02:00. However, this actually
     * refers to the point in time (in UTC) that is equivalent to 01:30 STD, which is before the DST switch
     *
     * Most libraries, like the glibc Olson database, therefore assume 02:30 actually refers to 03:30 local time.
     *
     * More logical would be that 02:30, since it is invalid, actually refers to 03:30. To do this, we 'move'
     * the daylight starting time up, as if it starts at 03:00. There is no difference for valid timestamps since
     * the time between 02:00 and 03:00 was invalid anyway. Note that this assumes a negative DST bias (which
     * is true everywhere at the moment).
     */    
    daylight -= m_dstbias * 60 * 10000000ll;

    if(isSTD) {
        /* daylight and standard are now LOCAL timestamp. Since we want to work with STD
         * timestamps here, we need to convert them.
         *
         * The switch to daylight happens at XX:YY STD time, so this one is already at STD time (local == STD)
         * The switch to standard happens at XX:YY DST time, so this one needs to be converted to STD time
         */
        standard += m_dstbias * 60 * 10000000ll;
    }
        
    /*
     * Now we have the start and end of DST, and the timestamp we're checking all lined up. All we need
     * to do now is figure out if in a year, daylight is before or after standard time. In the northern
     * hemisphere, DST starts, say, in march, and ends in october, so DST is between march and october. In
     * the southern hemisphere, DST starts in october up to the end of the year, and from the beginning of the
     * year until march.
     */
    if(m_daylight.wMonth < m_standard.wMonth) {
        // Northern hemisphere
        if(daylight <= tTimestamp && tTimestamp < standard)
            return true;
    } else {
        // Southern hemisphere
        if(tTimestamp < standard || daylight <= tTimestamp)
            return true;
    }
    
    return false;
}

/**
 * Parse on-disk line of timezone rule information
 *
 * @param[in] line Input line to parse
 * @param[out] lppTZRule Instantiated TimezoneRule object
 * @return result
 */
HRESULT TimezoneRule::FromLine(const char *line, TimezoneRule **lppTZRule)
{
    HRESULT hr = hrSuccess;

    int bias, stdbias, dstbias;
    SYSTEMTIME standard;
    SYSTEMTIME daylight;
    
    if(sscanf(line, "%d %d %d %hu %hu %hu %hu %hu %hu %hu %hu %hu %hu %hu %hu %hu %hu %hu %hu", 
        &bias, &stdbias, &dstbias, 
        &standard.wYear, &standard.wMonth, &standard.wDayOfWeek, &standard.wDay, &standard.wHour, &standard.wMinute, &standard.wSecond, &standard.wMilliseconds,
        &daylight.wYear, &daylight.wMonth, &daylight.wDayOfWeek, &daylight.wDay, &daylight.wHour, &daylight.wMinute, &daylight.wSecond, &daylight.wMilliseconds) != 19)
    {
        ASSERT(false);
        hr = MAPI_E_CORRUPT_DATA;
        goto exit;
    }

    try {    
        *lppTZRule = new TimezoneRule(bias, stdbias, dstbias, standard, daylight);
    } catch (std::bad_alloc &) {
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto exit;
    }
    
    (*lppTZRule)->AddRef();
        
exit:
    return hr;
}

/**
 * Parse MAPI blob of timezone rule information
 *
 * @param[in] cSize Size of data in lpBlob
 * @param[in] lpBlob Byte array of data for the timezone rule in TZSTRUCT format
 * @param[out] lppTZ Instantiated TimezoneRule object
 * @return result
 */
HRESULT TimezoneRule::FromBlob(ULONG cSize, BYTE *lpBlob, TimezoneRule **lppTZRule)
{
    HRESULT hr = hrSuccess;
    TZSTRUCT *lpTZReg = (TZSTRUCT *)lpBlob;
    
    if(cSize != sizeof(TZSTRUCT)) {
        hr = MAPI_E_CORRUPT_DATA;
        goto exit;
    }
    
    try {
        *lppTZRule = new TimezoneRule(lpTZReg->bias, lpTZReg->stdbias, lpTZReg->dstbias, lpTZReg->stStandardDate, lpTZReg->stDaylightDate);
    } catch (std::bad_alloc &) {
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto exit;
    }
    (*lppTZRule)->AddRef();

exit:                                 
    return hr;
}

/**
 * Parse TZREG blob of timezone rule information
 *
 * @param[in] cSize Size of data in lpBlob
 * @param[in] lpBlob Byte array of data for the timezone rule in TZREG format
 * @param[out] lppTZ Instantiated TimezoneRule object
 * @return result
 */
HRESULT TimezoneRule::FromTZREGBlob(ULONG cSize, BYTE *lpBlob, TimezoneRule **lppTZRule)
{
    HRESULT hr = hrSuccess;
    TZREG *lpTZReg = (TZREG *)lpBlob;
    
    if(cSize != sizeof(TZREG)) {
        hr = MAPI_E_CORRUPT_DATA;
        goto exit;
    }

    try {   
        *lppTZRule = new TimezoneRule(lpTZReg->bias, lpTZReg->stdbias, lpTZReg->dstbias, lpTZReg->stStandardDate, lpTZReg->stDaylightDate);
    } catch (std::bad_alloc &) {
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto exit;
    }
    (*lppTZRule)->AddRef();

exit:                                 
    return hr;
}

HRESULT TimezoneRule::FromSystem(TimezoneRule **lppTZRule)
{
	HRESULT hr = hrSuccess;
	SYSTEMTIME stNULL = {0};

	// find time samples in the current year to determain which offset we have in standard and daylight time
	struct tm tmnow;
	struct tm tmjan1 = { 0, 0, 12, 1, 1, 0, 0, 0, -1 };
	struct tm tmjul1 = { 0, 0, 12, 1, 7, 0, 0, 0, -1 };
	time_t tjan1, tjul1, now;
	int base, dst;

	now = time(NULL);
	localtime_r(&now, &tmnow);

	// find offset januari 1st
	tmjan1.tm_year = tmnow.tm_year;
	tjan1 = mktime(&tmjan1);
	localtime_r(&tjan1, &tmjan1);
	base = tmjan1.tm_gmtoff;

	// same for july 1st
	tmjul1.tm_year = tmnow.tm_year;
	tjul1 = mktime(&tmjul1);
	localtime_r(&tjul1, &tmjul1);
	dst = tmjul1.tm_gmtoff;

	// swap base and dst for southern hemisphere
	if (dst < base)
		std::swap(base, dst);

	try {
		*lppTZRule = new TimezoneRule(-(base/60), 0, -((dst-base)/60), stNULL, stNULL);
	} catch (std::bad_alloc &) {
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}
	(*lppTZRule)->AddRef();

exit:
	return hr;
}

HRESULT TimezoneRule::FromRuleData(int bias, int stdbias, int dstbias,
                                   const SYSTEMTIME &standard, const SYSTEMTIME &daylight,
                                   TimezoneRule **lppTZRule)
{
    HRESULT hr = hrSuccess;
    
    if (lppTZRule == NULL) {
        hr = MAPI_E_INVALID_PARAMETER;
        goto exit;
    }
    
    try {
        *lppTZRule = new TimezoneRule(bias, stdbias, dstbias, standard, daylight);
    } catch (const std::bad_alloc &) {
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto exit;
    }
    (*lppTZRule)->AddRef();
    
exit:
    return hr;
}
                                    

/**
 * Generation MAPI blob of timezone rule information
 *
 * @param[out] lpSize Size of data in lppBlob
 * @param[out] lppBlob TZSTRUCT representation of this rule
 * @return result
 */
HRESULT TimezoneRule::ToTZSTRUCT(ULONG *lpSize, BYTE **lppBlob)
{
	return ToTZSTRUCT(NULL, lpSize, lppBlob);
}

/**
 * Generation MAPI blob of timezone rule information
 *
 * @param[in] lpBase Base pointer that causes MAPIAllocateMore to be used when not NULL.
 * @param[out] lpSize Size of data in lppBlob
 * @param[out] lppBlob TZSTRUCT representation of this rule
 * @return result
 */
HRESULT TimezoneRule::ToTZSTRUCT(VOID *lpBase, ULONG *lpSize, BYTE **lppBlob)
{
    HRESULT hr = hrSuccess;
    mapi_memory_ptr<TZSTRUCT> lpTZStruct;

    if (lpBase == NULL)
        hr = MAPIAllocateBuffer(sizeof(TZSTRUCT), (void **)&lpTZStruct);
    else
        hr = MAPIAllocateMore(sizeof(TZSTRUCT), lpBase, (void **)&lpTZStruct);
    if(hr != hrSuccess)
        goto exit;
        
    memset(lpTZStruct, 0, sizeof(TZSTRUCT));

    lpTZStruct->bias = m_bias;
    lpTZStruct->stdbias = m_stdbias;
    lpTZStruct->dstbias = m_dstbias;
    
    lpTZStruct->stStandardDate = m_standard;
    lpTZStruct->stDaylightDate = m_daylight;

    *lppBlob = (BYTE *)lpTZStruct.release();
    *lpSize = sizeof(TZSTRUCT);
        
exit:
    return hr;
}

/**
 * Generate MAPI blob of timezone rule information
 *
 * @param[in] lpBlob Buffer to receive TZREG structure, must be at least sizeof(TZREG) bytes large
 * @return result
 */
HRESULT TimezoneRule::ToTZREG(BYTE *lpBlob)
{
    TZREG *lpTZReg = (TZREG *)lpBlob;
    
    lpTZReg->bias = m_bias;
    lpTZReg->stdbias = m_stdbias;
    lpTZReg->dstbias = m_dstbias;
    
    lpTZReg->stStandardDate = m_standard;
    lpTZReg->stDaylightDate = m_daylight;
    
    return hrSuccess;
}

HRESULT TimezoneRule::GetRuleData(int *lpBias, int *lpStdbias, int *lpDstbias,
								  SYSTEMTIME *lpStandard, SYSTEMTIME *lpDaylight)
{
	if (lpBias)
		*lpBias = m_bias;
	
	if (lpStdbias)
		*lpStdbias = m_stdbias;
	
	if (lpDstbias)
		*lpDstbias = m_dstbias;
	
	if (lpStandard)
		*lpStandard = m_standard;
	
	if (lpDaylight)
		*lpDaylight = m_daylight;
	
	return hrSuccess;
}

/**
 * Construct a TZ definition
 *
 * @param guid GUID of this TZ definition (or GUID_NULL if none)
 * @param strName Name of the TZ definition
 * @param strDisplay Display name of the TZ definition
 * @param mapDST Map of year -> TimezoneRule rule definition
 */
TimezoneDefinition::TimezoneDefinition(const GUID& guid, const std::wstring &strName, const std::wstring &strDisplay, std::map<unsigned int, TimezoneRule *>& mapDST) : m_wstrName(strName), m_wstrDisplay(strDisplay), m_guid(guid), m_mapRules(mapDST)
{
}

TimezoneDefinition::~TimezoneDefinition()
{
    for_each(m_mapRules.begin(), m_mapRules.end(), release_second<std::map<unsigned int, TimezoneRule *> >);
}


/**
 * Parse MAPI blob of timezone definition
 *
 * @param[in] cSize Size of data in lpBlob
 * @param[out] lppTZ TimezoneDefinition instantiated object
 * @return result
 */
HRESULT TimezoneDefinition::FromBlob(ULONG cSize, BYTE *lpBlob, TimezoneDefinition **lppTZ)
{
    HRESULT hr = hrSuccess;
    TZDEF_HEADER *lpHeader = (TZDEF_HEADER *)lpBlob;
    TZDEF_RULE *lpRule = NULL;
    BYTE *lpHere = NULL;
    GUID guid = GUID_NULL;
    std::wstring strKeyname;
    int cRules;
    TimezoneRule *lpTZRule = NULL;
    std::map<unsigned int, TimezoneRule *> mapTZRule;
    
    if(cSize < sizeof(TZDEF_HEADER)) {
        hr = MAPI_E_CORRUPT_DATA;
        goto exit;
    }
    
    if(lpHeader->bMajorVersion != TZ_BIN_VERSION_MAJOR) {
        hr = MAPI_E_VERSION;
        goto exit;
    }
    
    lpHere = lpBlob + sizeof(TZDEF_HEADER);
    cSize -= sizeof(TZDEF_HEADER);
    
    if(lpHeader->wFlags & TZDEFINITION_FLAG_VALID_GUID) {
        if(cSize < sizeof(GUID)) {
            hr = MAPI_E_CORRUPT_DATA;
            goto exit;
        }
        guid = *(GUID *)lpHere;
        lpHere += sizeof(GUID);
        cSize -= sizeof(GUID);
    }
    
    if(lpHeader->wFlags & TZDEFINITION_FLAG_VALID_KEYNAME) {
        if(cSize < sizeof(WORD)) {
            hr = MAPI_E_CORRUPT_DATA;
            goto exit;
        }
        WORD cchKeyName = *(WORD *)lpHere;
        lpHere += sizeof(WORD);
        cSize -= sizeof(WORD);
        
        if(cSize <  cchKeyName * sizeof(short int)) {
            hr = MAPI_E_CORRUPT_DATA;
            goto exit;
        }
        strKeyname = convert_to<std::wstring>((char *)lpHere, cchKeyName * sizeof(short int), "UCS-2LE");
        lpHere += cchKeyName * sizeof(short int);
        cSize -= cchKeyName * sizeof(short int);
    }
    
    if(cSize < sizeof(WORD)) {
        hr = MAPI_E_CORRUPT_DATA;
        goto exit;
    }
    cRules = *(WORD *)lpHere;
    lpHere += sizeof(WORD);
    cSize -= sizeof(WORD);
        
    lpRule = (TZDEF_RULE *)(lpBlob + lpHeader->cbHeader + offsetof(TZDEF_HEADER, wFlags));

    // Normally, currently lpHere == lpRule, but new versions may add or delete extra fields in which
    // case we are skipping that and going to the rules in lpRule. We have not seen that yet so let's
    // assert that they are equal at the moment.
    ASSERT((void *)lpHere == (void *)lpRule);
    
    for(int i = 0; i < cRules; i++) {
        if(cSize < sizeof(TZDEF_RULE)) {
            hr = MAPI_E_CORRUPT_DATA;
            goto exit;
        }
        
        if(lpRule->bMajorVersion == TZ_BIN_VERSION_MAJOR) {
            hr = TimezoneRule::FromTZREGBlob(sizeof(TZREG), (BYTE *)&lpRule->rule, &lpTZRule);
            if(hr != hrSuccess)
                goto exit;
                
            mapTZRule.insert(std::make_pair(lpRule->stStart.wYear, lpTZRule));
        }
        
        cSize -= lpRule->cbRule + offsetof(TZDEF_RULE, wFlags);
        lpRule = (TZDEF_RULE *)((BYTE *)lpRule + lpRule->cbRule + offsetof(TZDEF_RULE, wFlags)); 
    }

    ASSERT(cSize == 0);
    
    try {
        *lppTZ = new TimezoneDefinition(guid, strKeyname, L"", mapTZRule);
    } catch (std::bad_alloc &) {
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto exit;
    }
    (*lppTZ)->AddRef();
    
    mapTZRule.clear();
    
exit:
    for_each(mapTZRule.begin(), mapTZRule.end(), release_second<std::map<unsigned int, TimezoneRule *> >);

    return hr;
}

/**
 * Generation MAPI blob of timezone definition
 *
 * @param[in] effective UTC timestamp of period that should be marked as 'effective' DST rule
 * @param[in] recur UTC timestamp of period that should be marked as 'recur' DST rule
 * @param[out] lpSize Size of output blob
 * @param[out] lppBlob MAPI allocated blob
 * @return result
 */
HRESULT TimezoneDefinition::ToTZDEFINITION(time_t effective, time_t recur, ULONG *lpSize, BYTE **lppBlob)
{
    return ToTZDEFINITION(effective, recur, NULL, lpSize, lppBlob);
}

/**
 * Generation MAPI blob of timezone definition
 *
 * @param[in] effective UTC timestamp of period that should be marked as 'effective' DST rule
 * @param[in] recur UTC timestamp of period that should be marked as 'recur' DST rule
 * @param[in] lpBase Base pointer that causes MAPIAllocateMore to be used when not NULL.
 * @param[out] lpSize Size of output blob
 * @param[out] lppBlob MAPI allocated blob
 * @return result
 */
HRESULT TimezoneDefinition::ToTZDEFINITION(time_t effective, time_t recur, VOID *lpBase, ULONG *lpSize, BYTE **lppBlob)
{
    FILETIME ftEffective;
    FILETIME ftRecur;
    
    UnixTimeToFileTime(effective, &ftEffective);
    UnixTimeToFileTime(recur, &ftRecur);
    
    return ToTZDEFINITION(ftEffective, ftRecur, lpBase, lpSize, lppBlob);
}

/**
 * Generation MAPI blob of timezone definition
 *
 * @param[in] effective UTC timestamp of period that should be marked as 'effective' DST rule
 * @param[in] recur UTC timestamp of period that should be marked as 'recur' DST rule
 * @param[out] lpSize Size of output blob
 * @param[out] lppBlob MAPI allocated blob
 * @return result
 */
HRESULT TimezoneDefinition::ToTZDEFINITION(FILETIME effective, FILETIME recur, ULONG *lpSize, BYTE **lppBlob)
{
	return ToTZDEFINITION(effective, recur, NULL, lpSize, lppBlob);
}

/**
 * Generation MAPI blob of timezone definition
 *
 * @param[in] effective UTC timestamp of period that should be marked as 'effective' DST rule
 * @param[in] recur UTC timestamp of period that should be marked as 'recur' DST rule
 * @param[in] lpBase Base pointer that causes MAPIAllocateMore to be used when not NULL.
 * @param[out] lpSize Size of output blob
 * @param[out] lppBlob MAPI allocated blob
 * @return result
 */
HRESULT TimezoneDefinition::ToTZDEFINITION(FILETIME effective, FILETIME recur, VOID *lpBase, ULONG *lpSize, BYTE **lppBlob)
{
    HRESULT hr = hrSuccess;
    std::string strName;
    ULONG cSize = 0;
    BYTE *lpBlob = NULL;
    BYTE *lpHere = NULL;
    unsigned int effectiveyear = 0;
    unsigned int recuryear = 0;
    TZDEF_HEADER *lpHeader = NULL;
    TimezoneRule *lpEffectiveRule = NULL;
    TimezoneRule *lpRecurRule = NULL;
    /*
     * Since FindDSTYear accepts local timestamp, we have to convert from our passed UTC timestamps to local
     * timestamps first
     */
    if(effective.dwHighDateTime != 0 && effective.dwLowDateTime != 0) {
        hr = GetEffectiveRule(effective, false, &lpEffectiveRule);
        if(hr != hrSuccess)
            goto exit;
            
        effectiveyear = ContainsRule(lpEffectiveRule, 0);
        if(!effectiveyear) {
            hr = MAPI_E_NOT_FOUND;
            ASSERT(false); // Should not happen since we just got the rule
            goto exit;
        }
    }
    
    if(recur.dwHighDateTime != 0 && recur.dwLowDateTime != 0) {
        hr = GetEffectiveRule(recur, false, &lpRecurRule);
        if(hr != hrSuccess)
            goto exit;

        recuryear = ContainsRule(lpEffectiveRule, 0);
        if(!recuryear) {
            hr = MAPI_E_NOT_FOUND;
            ASSERT(false); // Should not happen since we just got the rule
            goto exit;
        }
    }
    
    strName = convert_to<std::string>("UCS-2LE", m_wstrName.data(), rawsize(m_wstrName), CHARSET_WCHAR);
    
    cSize = sizeof(TZDEF_HEADER) + sizeof(m_guid) + sizeof(WORD) + strName.size() + sizeof(WORD) + m_mapRules.size() * sizeof(TZDEF_RULE);

    if (lpBase == NULL)
        hr = MAPIAllocateBuffer(cSize, (void **)&lpBlob);
    else
        hr = MAPIAllocateMore(cSize, lpBase, (void **)&lpBlob);
    if(hr != hrSuccess)
        goto exit;
    
    memset(lpBlob, 0, cSize);
        
    lpHeader = (TZDEF_HEADER *)lpBlob;
    lpHeader->bMajorVersion = TZ_BIN_VERSION_MAJOR;
    lpHeader->bMinorVersion = TZ_BIN_VERSION_MINOR;
    if(m_guid != GUID_NULL)
        lpHeader->wFlags |= TZDEFINITION_FLAG_VALID_GUID;
    if(!m_wstrName.empty())
        lpHeader->wFlags |= TZDEFINITION_FLAG_VALID_KEYNAME;
        
    lpHere = lpBlob + sizeof(TZDEF_HEADER);
        
    if(m_guid != GUID_NULL) {
        *(GUID *)lpHere = m_guid;
        lpHere += sizeof(GUID);
    } else {
        cSize -= sizeof(GUID);
    }
    
    if(!m_wstrName.empty()) {
        *(WORD *)lpHere = m_wstrName.size();
        lpHere += sizeof(WORD);
        memcpy(lpHere, strName.data(), strName.size());
        lpHere += strName.size();
    } else {
        cSize -= sizeof(WORD);
    }
    
    *(WORD *)lpHere = m_mapRules.size();
    lpHere += sizeof(WORD);

    lpHeader->cbHeader = lpHere - lpBlob - offsetof(TZDEF_HEADER, wFlags); // Calculate offset between here and the 'wFlags' member of the header
    
    for(std::map<unsigned int, TimezoneRule *>::const_iterator i = m_mapRules.begin(); i != m_mapRules.end(); i++) {
        TZDEF_RULE *lpRule = (TZDEF_RULE *)lpHere;
        
        lpRule->bMajorVersion = TZ_BIN_VERSION_MAJOR;
        lpRule->bMinorVersion = TZ_BIN_VERSION_MINOR;
        lpRule->cbRule = sizeof(TZDEF_RULE) - offsetof(TZDEF_RULE, wFlags);
        lpRule->wFlags = 0;
        
        if(effectiveyear && i->first == effectiveyear)
            lpRule->wFlags |= TZRULE_FLAG_EFFECTIVE_TZREG;
        
        if(recuryear && i->first == recuryear)
            lpRule->wFlags |= TZRULE_FLAG_RECUR_CURRENT_TZREG;
            
        lpRule->stStart.wYear = i->first;
        lpRule->stStart.wMonth = 1;
        lpRule->stStart.wDay = 1;
        
        i->second->ToTZREG((BYTE *)&lpRule->rule);
        
        lpHere += sizeof(TZDEF_RULE);
    }
    
    ASSERT(lpHere - lpBlob == (int)cSize);
    
    *lppBlob = lpBlob;
    *lpSize = cSize;
    
exit:
    if(lpEffectiveRule)
        lpEffectiveRule->Release();
    if(lpRecurRule)
        lpRecurRule->Release();
        
    return hr;  
}

/**
 * Get the rule that is in effect at the given UTC timestamp.
 *
 * @param[in] timestamp Timestamp to get effective rule for
 * @param[in] local True if timestamp is a local time, false if it is a UTC timestamp
 * @return result
 */
HRESULT TimezoneDefinition::GetEffectiveRule(time_t timestamp, bool local, TimezoneRule **lppRule)
{
    FILETIME ftTimestamp;
    
    UnixTimeToFileTime(timestamp, &ftTimestamp);
    return GetEffectiveRule(ftTimestamp, local, lppRule);
}

/**
 * Get the rule that is in effect at the given UTC timestamp.
 *
 * @param[in] timestamp Timestamp to get effective rule for
 * @param[in] local True if timestamp is a local time, false if it is a UTC timestamp
 * @return result
 */
HRESULT TimezoneDefinition::GetEffectiveRule(FILETIME timestamp, bool local, TimezoneRule **lppRule)
{
    HRESULT hr = hrSuccess;
    FILETIME tLocal;
    std::map<unsigned int, TimezoneRule *>::const_iterator i;
    
    int effectiveyear = FindDSTYear(timestamp);
    
    if(!local) {
        /* FindDSTYear takes a local timestamp, while we have given a UTC timestamp. Since we don't know
         * what rule to apply to get to the local timestamp, we first use the approximate year from
         * the UTC timestamp, convert to local using that, and then use that to get the local year,
         * then reapply the effective year search. This is needed for timezones that change their
         * definition on Jan 1, like Samoa did on 1 Jan 2012.
         */

        std::map<unsigned int, TimezoneRule *>::const_iterator i = m_mapRules.find(effectiveyear);

        ASSERT(i != m_mapRules.end());

        hr = i->second->FromUTC(timestamp, &tLocal);
        if (hr != hrSuccess)
            goto exit;

        effectiveyear = FindDSTYear(tLocal);
    }
    
    i = m_mapRules.find(effectiveyear);
    ASSERT(i != m_mapRules.end());

    *lppRule = i->second;
    (*lppRule)->AddRef();
    
exit:
    return hr;
}    

/**
 * Output a TZSTRUCT structure that is in effect at UTC timestamp
 *
 * @param[in] timestamp Timestamp to output TZSTRUCT for
 * @param[out] lpSize Size of bytes in lpBlob
 * @param[out] lpBlob TZSTRUCT data
 * @return result
 */
HRESULT TimezoneDefinition::ToTZSTRUCT(time_t timestamp, ULONG *lpSize, BYTE **lpBlob)
{
	return ToTZSTRUCT(timestamp, NULL, lpSize, lpBlob);
}

/**
 * Output a TZSTRUCT structure that is in effect at UTC timestamp
 *
 * @param[in] timestamp Timestamp to output TZSTRUCT for
 * @param[in] lpBase Base pointer that causes MAPIAllocateMore to be used when not NULL.
 * @param[out] lpSize Size of bytes in lpBlob
 * @param[out] lpBlob TZSTRUCT data
 * @return result
 */
HRESULT TimezoneDefinition::ToTZSTRUCT(time_t timestamp, VOID *lpBase, ULONG *lpSize, BYTE **lpBlob)
{
    FILETIME ftTimestamp;
    
    UnixTimeToFileTime(timestamp, &ftTimestamp);
    return ToTZSTRUCT(ftTimestamp, lpSize, lpBlob);
}

/**
 * Output a TZSTRUCT structure that is in effect at UTC timestamp
 *
 * @param[in] timestamp Timestamp to output TZSTRUCT for
 * @param[out] lpSize Size of bytes in lpBlob
 * @param[out] lpBlob TZSTRUCT data
 * @return result
 */
HRESULT TimezoneDefinition::ToTZSTRUCT(FILETIME timestamp, ULONG *lpSize, BYTE **lpBlob)
{
	return ToTZSTRUCT(timestamp, NULL, lpSize, lpBlob);
}

/**
 * Output a TZSTRUCT structure that is in effect at UTC timestamp
 *
 * @param[in] timestamp Timestamp to output TZSTRUCT for
 * @param[in] lpBase Base pointer that causes MAPIAllocateMore to be used when not NULL.
 * @param[out] lpSize Size of bytes in lpBlob
 * @param[out] lpBlob TZSTRUCT data
 * @return result
 */
HRESULT TimezoneDefinition::ToTZSTRUCT(FILETIME timestamp, VOID *lpBase, ULONG *lpSize, BYTE **lpBlob)
{
    TimezoneRulePtr lpRule;
    HRESULT hr = hrSuccess;

    hr = GetEffectiveRule(timestamp, false, &lpRule);
    if(hr != hrSuccess)
        goto exit;    
    
    hr = lpRule->ToTZSTRUCT(lpBase, lpSize, lpBlob);
    if(hr != hrSuccess)
        goto exit;

exit:
    return hr;
}

/**
 * Convert from local timestamp to UTC timestamp
 *
 * @param[in] tLocalTimestamp Local timestamp to convert
 * @param[out] lptUTC UTC converted timestamp
 * @return result
 */
HRESULT TimezoneDefinition::ToUTC(time_t tLocalTimestamp, time_t *lptUTC)
{
    HRESULT hr;
    FILETIME ftLocalTimestamp;
    FILETIME ftUTC;
    
    UnixTimeToFileTime(tLocalTimestamp, &ftLocalTimestamp);
    hr = ToUTC(ftLocalTimestamp, &ftUTC);
    if (hr == hrSuccess)
        *lptUTC = FileTimeToUnixTime(ftUTC.dwHighDateTime, ftUTC.dwLowDateTime);
    
    return hr;
}

/**
 * Convert from local timestamp to UTC timestamp
 *
 * @param[in] tLocalTimestamp Local timestamp to convert
 * @param[out] lptUTC UTC converted timestamp
 * @return result
 */
HRESULT TimezoneDefinition::ToUTC(FILETIME tLocalTimestamp, FILETIME *lptUTC)
{
    TimezoneRulePtr lpRule;
    HRESULT hr = hrSuccess;

    hr = GetEffectiveRule(tLocalTimestamp, true, &lpRule);
    if(hr != hrSuccess)
        goto exit;    
    
    hr = lpRule->ToUTC(tLocalTimestamp, lptUTC);
    if(hr != hrSuccess)
        goto exit;

exit:
    return hr;
}

/**
 * Convert from UTC timestamp to local timestamp
 *
 * @param[in] tUTC UTC input timestamp
 * @param[out] lptLocalTimeStamp Local output timestamp
 * @return result
 */
HRESULT TimezoneDefinition::FromUTC(time_t tUTC, time_t *lptLocalTimeStamp)
{
    HRESULT hr;
    FILETIME ftUTC;
    FILETIME ftLocalTimeStamp;
    
    UnixTimeToFileTime(tUTC, &ftUTC);
    hr = FromUTC(ftUTC, &ftLocalTimeStamp);
    if (hr == hrSuccess)
        *lptLocalTimeStamp = FileTimeToUnixTime(ftLocalTimeStamp.dwHighDateTime, ftLocalTimeStamp.dwLowDateTime);
    
    return hr;
}

/**
 * Convert from UTC timestamp to local timestamp
 *
 * @param[in] tUTC UTC input timestamp
 * @param[out] lptLocalTimeStamp Local output timestamp
 * @return result
 */
HRESULT TimezoneDefinition::FromUTC(FILETIME tUTC, FILETIME *lptLocalTimeStamp)
{
    TimezoneRulePtr lpRule;
    HRESULT hr = hrSuccess;

    hr = GetEffectiveRule(tUTC, false, &lpRule);
    if(hr != hrSuccess)
        goto exit;    
    
    hr = lpRule->FromUTC(tUTC, lptLocalTimeStamp);
    if(hr != hrSuccess)
        goto exit;

exit:
    return hr;
}


/**
 * Read a timezone definition from disk
 *
 * eg.
 *   (UTC-04:00) Cuiaba
 *   2006 240 0 -60 0 2 0 2 2 0 0 0 0 11 0 1 0 0 0 0
 *   2007 240 0 -60 0 2 0 5 0 0 0 0 0 10 0 2 0 0 0 0
 *   2008 240 0 -60 0 2 0 3 0 0 0 0 0 10 6 3 23 59 59 999
 *   2009 240 0 -60 0 2 6 2 23 59 59 999 0 10 6 3 23 59 59 999
 *   ...
 *
 * @param[in] strFilename Filename to read
 * @param[in] strName Name of the timezone
 * @param[out] lppTZ Instantiated TimezoneDefinition object
 * @return result
 */
HRESULT TimezoneDefinition::FromDisk(const std::string &strFilename, const std::wstring &strName, TimezoneDefinition **lppTZ)
{
    HRESULT hr = hrSuccess;
    FILE *fp = NULL;
    char buf[4096];
    std::string strDisplay;
    TimezoneRulePtr lpDSTRule;
    std::map<unsigned int, TimezoneRule *> mapDSTRules;
    int n = 0;
    int year = 0;
    
    // First line contains display name
    fp = fopen(strFilename.c_str(), "rt");
    if(!fp) {
        hr = MAPI_E_NOT_FOUND;
        goto exit;
    }
    
    if(!fgets(buf, sizeof(buf), fp)) {
        hr = MAPI_E_CORRUPT_DATA;
        goto exit;
    }

    strDisplay = trim(buf, "\r\n");
    
    if(strDisplay.empty()) { 
        hr = MAPI_E_CORRUPT_DATA;
        goto exit;
    }

    while(fgets(buf, sizeof(buf), fp)) {
        if(sscanf(buf, "%d %n", &year, &n) != 1) {
            ASSERT(false);
            hr = MAPI_E_CORRUPT_DATA;
            goto exit;
        }
        
        hr = TimezoneRule::FromLine(buf + n, &lpDSTRule);
        if(hr != hrSuccess)
            goto exit;
            
        mapDSTRules.insert(std::make_pair(year, lpDSTRule.release()));
    }
    
    ASSERT(!mapDSTRules.empty());
    
    try {
        *lppTZ = new TimezoneDefinition(GUID_NULL, strName, convert_to<std::wstring>(strDisplay, rawsize(strDisplay), "UTF-8"), mapDSTRules);
    } catch (std::bad_alloc &) {
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto exit;
    }
    (*lppTZ)->AddRef();

    // We no longer own these pointers
    mapDSTRules.clear();
    
exit:
    if(fp)
        fclose(fp);
        
    for_each(mapDSTRules.begin(), mapDSTRules.end(), release_second<std::map<unsigned int, TimezoneRule *> >);
    
    return hr;
}

/**
 * Find the year for the rule that would be in effect for the local timestamp passed
 *
 * @param[in] timestamp
 * @return Year
 */
unsigned int TimezoneDefinition::FindDSTYear(FILETIME timestamp)
{
    std::map<unsigned int, TimezoneRule *>::const_iterator i;
    unsigned int year = GetYear(timestamp);
    
    i = m_mapRules.lower_bound(year);
    
    // I will point at the exact year, or the next year
    if(i == m_mapRules.end() || (i != m_mapRules.begin() && i->first != year)) {
        i--;
    }

    return i->first;    
}

/**
 * Checks to see if this TZ contains the passed TZ rule
 *
 * We iterate through rules in reverse order, which is important if you are using
 * this function to get the effective year (lpulYear).
 *
 * @param[in] lpRule Rule to match
 * @param[in] ulMatchLevel Fuzzy match level (0=exact, 5=bias only)
 * @return year
 */
unsigned int TimezoneDefinition::ContainsRule(const TimezoneRule *lpRule, unsigned int ulMatchLevel)
{
    std::map<unsigned int, TimezoneRule *>::const_reverse_iterator i;
    
    for(i = m_mapRules.rbegin(); i != m_mapRules.rend(); i++) {
        if(i->second->Compare(lpRule, ulMatchLevel) == 0) {
            return i->first;
        }
    }
    
    return 0;
}

/**
 * Get TZ name. This is important since timezones are mostly stored under their exact name
 *
 * @param[out] lpwstrName Name of this TZ definition
 * @return result
 */

HRESULT TimezoneDefinition::GetName(std::wstring *lpwstrName)
{
    *lpwstrName = m_wstrName;
    return hrSuccess;
}

/**
 * Get TZ display name.
 *
 * @param[out] lpwstrDisplay Display name of this TZ definition
 * @return result
 */
HRESULT TimezoneDefinition::GetDisplayName(std::wstring *lpwstrDisplay)
{
    *lpwstrDisplay = m_wstrDisplay;
    return hrSuccess;
}

/**
 * Get GUID for this timezone
 * @param[out] lpGUID GUID of this TZ definition
 * @return result
 */
HRESULT TimezoneDefinition::GetGUID(GUID *lpGUID)
{
    if(m_guid == GUID_NULL)
        return MAPI_E_NOT_FOUND;
        
    *lpGUID = m_guid;
    return hrSuccess;
}
