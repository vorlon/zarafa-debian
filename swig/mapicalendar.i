%module(directors="1") mapicalendar

%{
    #include <platform.h>
    #include <mapi.h>
    #include <mapidefs.h>
    #include <mapicode.h>
    #include <mapiutil.h>

    #include "MAPICalendar.h"
%}

%include "wchar.i"
%include <windows.i>
%include "cstring.i"
%include "cwstring.i"
%include "std_string.i"
%include "std_wstring.i"
%include "typemap.i"
%include "typemaps.i"

//%import "mapi.i"
%import "mapitimezone.i"

%apply double *OUTPUT { time_t * }
%apply double { time_t }
%apply bool { unsigned short int }
%apply bool *OUTPUT { unsigned short int * }

%apply MAPICLASS *{PropertyPool **, IAppointment **, IOccurrence **, RecurrencePattern **}
%apply MAPISTRUCT * {LPMAPINAMEID *OUTPUT};


//////////////////////
// ULONG* + ULONG**
//////////////////////

// Output
%typemap(in,numinputs=0) (ULONG *OUTPUT, ULONG **OUTPUT) (ULONG cb, ULONG *a)
{
    cb = 0;
    a = NULL;

    $1 = &cb;
    $2 = &a;
}

%typemap(argout) (ULONG *OUTPUT, ULONG **OUTPUT)
{
    PyObject *pyList = PyList_New(0);
    ULONG *a = *($2);
    for (ULONG i = 0; i < *($1); ++i) {
        PyObject *pyElem = SWIG_From_unsigned_SS_int((a[i]));
        PyList_Append(pyList, pyElem);
        Py_DECREF(pyElem);
        //pyList = SWIG_Python_AppendOutput(pyList, );
    }
    %append_output(pyList);
}

%typemap(freearg) (ULONG *OUTPUT, ULONG **OUTPUT)
{
    if (*($2))
        MAPIFreeBuffer(*($2));
}


class PropertyPool : public ECUnknown
{
public:
    enum PropNames {
        PROP_KEYWORDS = 0,
// End of String names
        PROP_MEETINGLOCATION, PROP_GOID, PROP_ISRECURRING, PROP_CLEANID, PROP_OWNERCRITICALCHANGE, PROP_ATTENDEECRITICALCHANGE, PROP_OLDSTART, PROP_ISEXCEPTION, PROP_RECURSTARTTIME,
        PROP_RECURENDTIME, PROP_SENDASICAL, PROP_APPTSEQNR, PROP_APPTSEQTIME,
        PROP_BUSYSTATUS, PROP_APPTAUXFLAGS, PROP_LOCATION, PROP_LABEL, PROP_APPTSTARTWHOLE, PROP_APPTENDWHOLE, PROP_APPTDURATION,
        PROP_ALLDAYEVENT, PROP_RECURRENCESTATE, PROP_MEETINGSTATUS, PROP_RESPONSESTATUS, PROP_RECURRING, PROP_INTENDEDBUSYSTATUS,
        PROP_RECURRINGBASE, PROP_REQUESTSENT, PROP_APPTREPLYNAME, PROP_RECURRENCETYPE, PROP_RECURRENCEPATTERN, PROP_TIMEZONEDATA, PROP_TIMEZONE,
        PROP_RECURRENCE_START, PROP_RECURRENCE_END, PROP_ALLATTENDEESSTRING, PROP_TOATTENDEESSTRING, PROP_CCATTENDEESSTRING, PROP_NETMEETINGTYPE,
        PROP_NETMEETINGSERVER, PROP_NETMEETINGORGANIZERALIAS, PROP_NETMEETINGAUTOSTART, PROP_AUTOSTARTWHEN, PROP_CONFERENCESERVERALLOWEXTERNAL, PROP_NETMEETINGDOCPATHNAME,
        PROP_NETSHOWURL, PROP_CONVERENCESERVERPASSWORD, PROP_APPTREPLYTIME, PROP_REMINDERMINUTESBEFORESTART, PROP_REMINDERTIME, PROP_REMINDERSET, PROP_PRIVATE,
        PROP_NOAGING, PROP_SIDEEFFECT, PROP_REMOTESTATUS, PROP_COMMONSTART, PROP_COMMONEND, PROP_COMMONASSIGN,
        PROP_CONTACTS, PROP_OUTLOOKINTERNALVERSION, PROP_OUTLOOKVERSION, PROP_REMINDERNEXTTIME, PROP_HIDE_ATTACH,
        PROP_TASK_STATUS, PROP_TASK_COMPLETE, PROP_TASK_PERCENTCOMPLETE, PROP_TASK_STARTDATE, PROP_TASK_DUEDATE,
        PROP_TASK_RECURRSTATE, PROP_TASK_ISRECURRING, PROP_TASK_COMPLETED_DATE,
        PROP_APPTTZDEFSTARTDISP, PROP_APPTTZDEFENDDISP, PROP_APPTTZDEFRECUR,
// End if ID names
        SIZE_NAMEDPROPS
    };

    %extend {
        PropertyPool(IMAPIProp *lpMapiProp) {
            PropertyPool *lpPropPool = NULL;
            PropertyPool::Create(lpMapiProp, &lpPropPool);
            return lpPropPool;
        };

        ~PropertyPool() {
            self->Release();
        }
    }
    static HRESULT GetPropIdAndType(PropertyPool::PropNames name, LPMAPINAMEID *OUTPUT, ULONG *lpulType);

    HRESULT GetRequiredPropTags(LPSPropTagArray lpExtraTags, LPSPropTagArray *OUTPUT) const;
    ULONG GetPropTag(PropertyPool::PropNames name) const;
};

class RecurrencePattern;

class IOccurrence : public IUnknown
{
public:
    virtual HRESULT SetStartDateTime(FILETIME ftStartDateTime, TimezoneDefinition *lpTZDef) = 0;
    virtual HRESULT GetStartDateTime(TimezoneDefinition *lpTZDef, FILETIME *lpftStartDateTime) = 0;

    virtual HRESULT SetEndDateTime(FILETIME ftEndDateTime, TimezoneDefinition *lpTZDef) = 0;
    virtual HRESULT GetEndDateTime(TimezoneDefinition *lpTZDef, FILETIME *lpftEndDateTime) = 0;

    virtual HRESULT GetDuration(ULONG *lpulDuration) = 0;

    virtual HRESULT SetBusyStatus(ULONG ulBusyStatus) = 0;
    virtual HRESULT GetBusyStatus(ULONG *lpulBusyStatus) = 0;

    virtual HRESULT SetLocation(LPTSTR lpstrLocation, ULONG ulFlags) = 0;
    %extend {
        HRESULT GetLocation(LPTSTR *OUTPUT, ULONG ulFlags) {
            return self->GetLocation(OUTPUT, NULL, ulFlags);
        }
    }

    virtual HRESULT SetReminderSet(unsigned short int fSet) = 0;
    virtual HRESULT GetReminderSet(unsigned short int *OUTPUT) = 0;

    virtual HRESULT SetReminderDelta(ULONG ulDelta) = 0;
    virtual HRESULT GetReminderDelta(ULONG *lpulDelta) = 0;

    virtual HRESULT SetSubject(LPTSTR lpstrSubject, ULONG ulFlags) = 0;
    %extend {
        HRESULT GetSubject(LPTSTR *OUTPUT, ULONG ulFlags) {
            return self->GetSubject(OUTPUT, NULL, ulFlags);
        }
    }

    virtual HRESULT SetMeetingType(ULONG ulMeetingType) = 0;
    virtual HRESULT GetMeetingType(ULONG *lpulMeetingType) = 0;

    virtual HRESULT SetSubType(unsigned short int fSubType) = 0;
    virtual HRESULT GetSubType(unsigned short int *OUTPUT) = 0;

    virtual HRESULT GetIsException(unsigned short int *OUTPUT) = 0;
    virtual HRESULT GetHasAttach(unsigned short int *OUTPUT) = 0;
    virtual HRESULT GetItemType(ULONG *lpulItemType) = 0;

    virtual HRESULT GetOriginalStartDateTime(TimezoneDefinition *lpTZDef, FILETIME *lpftStartDateTime) = 0;
    virtual HRESULT GetBaseDate(ULONG *lpulBaseDate) = 0;
    virtual HRESULT GetMapiMessage(ULONG ulMsgType, ULONG ulOverrideFlags, IMessage **lppMessage) = 0;
    virtual HRESULT ApplyChanges() = 0;

    %extend {
        ~IOccurrence() { self->Release(); }
    }
};


class IAppointment : public IOccurrence
{
public:
    virtual HRESULT SetRecurrence(RecurrencePattern *lpPattern) = 0;
    virtual HRESULT GetRecurrence(RecurrencePattern **lppPattern) = 0;
    virtual HRESULT SetRecurrenceTimezone(TimezoneDefinition *lpTZDef) = 0;
    virtual HRESULT GetRecurrenceTimezone(TimezoneDefinition **lppTZDef) = 0;
    virtual HRESULT GetBounds(ULONG *lpulFirst, ULONG *lpulLast) = 0;
    
    virtual HRESULT GetOccurrence(ULONG ulBaseDate, IOccurrence **lppOccurrence) = 0;
    virtual HRESULT RemoveOccurrence(ULONG ulBaseDate) = 0;
    virtual HRESULT ResetOccurrence(ULONG ulBaseDate) = 0;
    virtual HRESULT GetExceptions(ULONG *OUTPUT, ULONG **OUTPUT,
                                  ULONG *OUTPUT, ULONG **OUTPUT) = 0;
    virtual HRESULT GetBaseDateForOccurrence(ULONG ulIndex, ULONG *lpulBaseDate) = 0;
    %extend {
        ~IAppointment() { self->Release(); }
    }
};

%inline %{
    HRESULT CreateAppointment(IMessage *lpMessage, TimezoneDefinition *lpClientTZ, IAppointment **lppAppointment) {
        return Appointment::Create(lpMessage, lpClientTZ, lppAppointment);
    }

    HRESULT CreateAppointment(ULONG cValues, LPSPropValue lpProps, PropertyPool *lpPropertyPool, TimezoneDefinition *lpClientTZ, IAppointment **lppAppointment) {
        return Appointment::Create(cValues, lpProps, lpPropertyPool, lpClientTZ, lppAppointment);
    }

    HRESULT CreateAppointment(IMAPIFolder *lpFolder, TimezoneDefinition *lpClientTZ, IAppointment **lppAppointment, IMessage **lppMessage) {
        return Appointment::Create(lpFolder, lpClientTZ, lppAppointment, lppMessage);
    }
%}

#define ITEMTYPE_SINGLE            0
#define ITEMTYPE_OCCURRENCE        1
#define ITEMTYPE_EXCEPTION         2
#define ITEMTYPE_RECURRINGMASTER   3

#define MSGTYPE_DEFAULT	0
#define MSGTYPE_MASTER		1
#define MSGTYPE_OCCURRENCE	2
#define MSGTYPE_COMPOSITE	3

#define ENSURE_OVERRIDE_ATTACHMENTS	1


class IRecurrencePatternInspector : public IUnknown
{
public:
    // Pattern
    virtual HRESULT SetPatternDaily(ULONG ulDailyPeriod) = 0;
    virtual HRESULT SetPatternWorkDays() = 0;
    virtual HRESULT SetPatternWeekly(ULONG ulFirstDayOfWeek, ULONG ulWeeklyPeriod, ULONG ulDayOfWeekMask) = 0;
    virtual HRESULT SetPatternAbsoluteMonthly(ULONG ulMonthlyPeriod, ULONG ulDayOfMonth) = 0;
    virtual HRESULT SetPatternRelativeMonthly(ULONG ulMonthlyPeriod, ULONG ulWhich, ULONG ulDayOfWeekMask) = 0;
    virtual HRESULT SetPatternAbsoluteYearly(ULONG ulYearlyPeriod, ULONG ulMonth, ULONG ulDayOfMonth) = 0;
    virtual HRESULT SetPatternRelativeYearly(ULONG ulYearlyPeriod, ULONG ulMonth, ULONG ulWhich, ULONG ulDayOfWeekMask) = 0;

    // Range
    virtual HRESULT SetRangeNoEnd(ULONG ulStartDate) = 0;
    virtual HRESULT SetRangeNumbered(ULONG ulStartDate, ULONG ulOccurrences) = 0;
    virtual HRESULT SetRangeEndDate(ULONG ulStartDate, ULONG ulEndDate) = 0;

    %extend {
        virtual ~IRecurrencePatternInspector() { self->Release(); }
    }
};

class RecurrencePattern : public IRecurrencePatternInspector
{
public:
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
    HRESULT IsOccurrence(ULONG ulBaseDate, bool *OUTPUT);
    HRESULT GetOccurrence(ULONG ulBaseDate, ULONG *lpulBaseDate);
    HRESULT GetOccurrencesInRange(ULONG ulStartDate, ULONG ulEndDate, ULONG *OUTPUT, ULONG **OUTPUT);
    HRESULT GetBounds(ULONG *lpulFirst, ULONG *lpulLast);

    %extend {
        RecurrencePattern() {
            RecurrencePattern *lpPattern = NULL;
            RecurrencePattern::Create(&lpPattern);
            return lpPattern;
        }

        RecurrencePattern(RecurrenceState *lpRecurState) {
            RecurrencePattern *lpPattern = NULL;
            RecurrencePattern::Create(lpRecurState, &lpPattern);
            return lpPattern;
        }

        ~RecurrencePattern() {
            self->Release();
        }
    }
};


#if SWIGPYTHON

%{
#include "swig_iunknown.h"

typedef IUnknownImplementor<IRecurrencePatternInspector> RecurrencePatternInspector;
%}

%feature("director") RecurrencePatternInspector;
%feature("nodirector") RecurrencePatternInspector::QueryInterface;
class RecurrencePatternInspector : public IRecurrencePatternInspector {
public:
    RecurrencePatternInspector(ULONG cInterfaces, LPCIID lpInterfaces);
    %extend {
        virtual ~RecurrencePatternInspector() { self->Release(); }
    }
};

#endif // SWIGPYTHON


HRESULT GetExpandedCalendarTable(IMAPIFolder *lpCalendar, FILETIME ftStartDateTime, FILETIME ftEndDateTime, TimezoneDefinition *lpClientTZ, LPSPropTagArray lpsPropTags, ULONG ulFlags, IMAPITable **lppTable);


%rename(ToBaseDate) BaseDateFromFileTime;
HRESULT BaseDateFromFileTime(FILETIME ftTimestamp, ULONG *lpulBaseDate);

%rename(ToBaseDate) BaseDateFromUnixTime;
HRESULT BaseDateFromUnixTime(time_t tTimestamp, ULONG *lpulBaseDate);

%rename(ToBaseDate) BaseDateFromYMD;
HRESULT BaseDateFromYMD(ULONG ulYear, ULONG ulMonth, ULONG ulDay, ULONG *lpulBaseDate);

%rename(ToFileTime) BaseDateToFileTime;
HRESULT BaseDateToFileTime(ULONG ulBaseDate, FILETIME *lpftTimestamp);

%rename(ToUnixTime) BaseDateToUnixTime;
HRESULT BaseDateToUnixTime(ULONG ulBaseDate, time_t *lptTimestamp);

%rename (ToYMD) BaseDateToYMD;
HRESULT BaseDateToYMD(ULONG ulBaseDate, ULONG *lpulYear, ULONG *lpulMonth, ULONG *lpulDay);
