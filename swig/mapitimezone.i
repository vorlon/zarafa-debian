%module mapitimezone

%{
    #include <platform.h>
    #include <mapi.h>
    #include <mapidefs.h>
    #include <mapicode.h>
    #include <mapiutil.h>

    #include "ECMAPITimezone.h"
%}

%include "wchar.i"
%include <windows.i>
%include "cstring.i"
%include "cwstring.i"
%include "std_string.i"
%include "std_wstring.i"
%include "typemap.i"
%include "typemaps.i"

%apply MAPICLASS *{TimezoneDefinition **, TimezoneRule **}
%apply (ULONG *OUTPUT, LPENTRYID *OUTPUT) {(ULONG* lpSize, BYTE **lpBlob)}
%apply unsigned int *OUTPUT { unsigned int *}
%apply double *OUTPUT { time_t * }
%apply double { time_t }
%apply (ULONG cbEntryID, LPENTRYID lpEntryID) {(ULONG cSize, BYTE *lpBlob)}
%apply std::wstring *OUTPUT { std::wstring * }
%apply LPMAPIUID OUTPUT {GUID *}
%apply (ULONG *, MAPIARRAY *) {(unsigned int *lpcValues, wchar_t ***lppszNames)}

class TimezoneRule
{
private:
	TimezoneRule();
public:
    // Parse from a TZREG data blob
    static HRESULT FromBlob(ULONG cSize, BYTE *lpBlob, TimezoneRule **lppTZ);
    static HRESULT FromTZREGBlob(ULONG cSize, BYTE *lpBlob, TimezoneRule **lppTZ);
    static HRESULT FromRuleData(int bias, int stdbias, int dstbias,
                                SYSTEMTIME standard, SYSTEMTIME daylight,
                                TimezoneRule **lppTZRule);

    // Output the rule as a TZREG
    HRESULT ToTZSTRUCT(ULONG *lpSize, BYTE **lpBlob);
    HRESULT ToTZREG(BYTE *lpBlob);
    
    // Compare this rule to another
    int Compare(const TimezoneRule *lpOther, unsigned int ulMatchLevel);

    // Convert the given 'local timestamp' to a UTC timestamp using the rules in this timezone
    HRESULT ToUTC(time_t tLocalTimestamp, time_t *tUTC);
    HRESULT ToUTC(FILETIME tLocalTimestamp, FILETIME *tUTC);

    // Convert the given UTC timestamp to a local timestamp using the rules in this timezone
    HRESULT FromUTC(time_t tUTC, time_t *tLocalTimeStamp);
    HRESULT FromUTC(FILETIME tUTC, FILETIME *tLocalTimeStamp);
 
	HRESULT TimezoneRule::GetRuleData(int *OUTPUT, int *OUTPUT, int *OUTPUT,
								      SYSTEMTIME *OUTPUT, SYSTEMTIME *OUTPUT);

	%extend {
		~TimezoneRule() { self->Release(); };
	}

};

class TimezoneDefinition
{
private:
	TimezoneDefinition();
	~TimezoneDefinition();
public:
    // Read a TZ definition from MAPI
    static HRESULT FromBlob(ULONG cSize, BYTE *lpBlob, TimezoneDefinition **lppTZ);

    // Output the entire definition as a TZDEFINITION
    HRESULT ToTZDEFINITION(time_t effective, time_t recur, ULONG *lpSize, BYTE **lpBlob);
    HRESULT ToTZDEFINITION(FILETIME effective, FILETIME recur, ULONG *lpSize, BYTE **lpBlob);

    // Output the rule for the specified year as a TZREG
    HRESULT ToTZSTRUCT(time_t timestamp, ULONG *lpSize, BYTE **lpBlob);
    HRESULT ToTZSTRUCT(FILETIME timestamp, ULONG *lpSize, BYTE **lpBlob);
    
    // Convert the given 'local timestamp' to a UTC timestamp using the rules in this timezone
    HRESULT ToUTC(time_t tLocalTimestamp, time_t *tUTC);
    HRESULT ToUTC(FILETIME tLocalTimestamp, FILETIME *tUTC);

    // Convert the given UTC timestamp to a local timestamp using the rules in this timezone
    HRESULT FromUTC(time_t tUTC, time_t *tLocalTimeStamp);
    HRESULT FromUTC(FILETIME tUTC, FILETIME *tLocalTimeStamp);

    // Get a rule effective at a certain moment in time
    HRESULT GetEffectiveRule(time_t timestamp, bool local, TimezoneRule **lppRule);
    HRESULT GetEffectiveRule(FILETIME timestamp, bool local, TimezoneRule **lppRule);

    // Match a rule with any of the DST rules for this timezone
    unsigned int ContainsRule(const TimezoneRule *lpRule, unsigned int ulMatchLevel);

	// Get the name of the timezone
    HRESULT GetName(std::wstring *lpwstrName);
    // Get the display name
    HRESULT GetDisplayName(std::wstring *lpwstrDisplay);
    // Get the GUID
    HRESULT GetGUID(GUID *lpGUID);

	%extend {
		~TimezoneDefinition() { self->Release(); };
	}
};

// Database functions
HRESULT HrGetTZDefByName(const std::wstring &strTZName, TimezoneDefinition **OUTPUT);
HRESULT HrGetTZNameByRule(time_t timestamp, const TimezoneRule *lpRule, unsigned int ulMatchLevel, std::wstring *OUTPUT);
HRESULT HrGetTZNameByRule(FILETIME timestamp, const TimezoneRule *lpRule, unsigned int ulMatchLevel, std::wstring *OUTPUT);
HRESULT HrGetTZNames(unsigned int *lpcValues, wchar_t ***lppszNames);
HRESULT HrGetTZNameFromOlsonName(const std::string &strOlson, std::wstring *OUTPUT);
HRESULT HrDetectSystemTZName(std::wstring *OUTPUT);
