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
#include "PropertyPool.h"
#include "CommonUtil.h"
#include "namedprops.h"

static const SizedSPropTagArray(4, g_sptaRequiredProps) = {
	4, {
			PR_SUBJECT_W,
			PR_START_DATE,
			PR_END_DATE,
			PR_HASATTACH
	}
};

static const ULONG g_aulRequiredNames[] = {
	PropertyPool::PROP_ALLDAYEVENT,
	PropertyPool::PROP_APPTENDWHOLE,
	PropertyPool::PROP_APPTSTARTWHOLE,
	PropertyPool::PROP_APPTTZDEFENDDISP,
	PropertyPool::PROP_APPTTZDEFRECUR,
	PropertyPool::PROP_APPTTZDEFSTARTDISP,
	PropertyPool::PROP_BUSYSTATUS,
	PropertyPool::PROP_COMMONEND,
	PropertyPool::PROP_COMMONSTART,
	PropertyPool::PROP_LOCATION,
	PropertyPool::PROP_MEETINGSTATUS,
	PropertyPool::PROP_REMINDERMINUTESBEFORESTART,
	PropertyPool::PROP_REMINDERSET,
	PropertyPool::PROP_RECURRENCESTATE,
	PropertyPool::PROP_RECURRING,
	PropertyPool::PROP_TIMEZONE,
	PropertyPool::PROP_TIMEZONEDATA
};


#define NAME_ENTRY(name, type, guid, id)	{ PropertyPool::PROP_##name, type, ECPropMapEntry(guid, id) }

static struct SNameEntry {
	ULONG			ulIndex;
	ULONG			ulPropType;
	ECPropMapEntry	sPropMapEntry;
} g_asNameEntries[] = {
// String names
	NAME_ENTRY(KEYWORDS, PT_MV_TSTRING, PS_PUBLIC_STRINGS, "Keywords"),
// ID names
	NAME_ENTRY(MEETINGLOCATION, PT_TSTRING, PSETID_Meeting, dispidMeetingLocation),
	NAME_ENTRY(GOID, PT_BINARY, PSETID_Meeting, dispidGlobalObjectID),
	NAME_ENTRY(ISRECURRING, PT_BOOLEAN, PSETID_Meeting, dispidIsRecurring),
	NAME_ENTRY(CLEANID, PT_BINARY, PSETID_Meeting, dispidCleanGlobalObjectID),
	NAME_ENTRY(OWNERCRITICALCHANGE, PT_SYSTIME, PSETID_Meeting, dispidOwnerCriticalChange),
	NAME_ENTRY(ATTENDEECRITICALCHANGE, PT_SYSTIME, PSETID_Meeting, dispidAttendeeCriticalChange),
	NAME_ENTRY(OLDSTART, PT_SYSTIME, PSETID_Meeting, dispidOldWhenStartWhole),
	NAME_ENTRY(ISEXCEPTION, PT_BOOLEAN, PSETID_Meeting, dispidIsException),
	NAME_ENTRY(RECURSTARTTIME, PT_LONG, PSETID_Meeting, dispidStartRecurrenceTime),
	NAME_ENTRY(RECURENDTIME, PT_LONG, PSETID_Meeting, dispidEndRecurrenceTime),
	NAME_ENTRY(SENDASICAL, PT_BOOLEAN, PSETID_Appointment, dispidSendAsICAL),
	NAME_ENTRY(APPTSEQNR, PT_LONG, PSETID_Appointment, dispidAppointmentSequenceNumber),
	NAME_ENTRY(APPTSEQTIME, PT_SYSTIME, PSETID_Appointment, dispidApptSeqTime),
	NAME_ENTRY(BUSYSTATUS, PT_LONG, PSETID_Appointment, dispidBusyStatus),
	NAME_ENTRY(APPTAUXFLAGS, PT_LONG, PSETID_Appointment, dispidApptAuxFlags),
	NAME_ENTRY(LOCATION, PT_TSTRING, PSETID_Appointment, dispidLocation),
	NAME_ENTRY(LABEL, PT_LONG, PSETID_Appointment, dispidLabel),
	NAME_ENTRY(APPTSTARTWHOLE, PT_SYSTIME, PSETID_Appointment, dispidApptStartWhole),
	NAME_ENTRY(APPTENDWHOLE, PT_SYSTIME, PSETID_Appointment, dispidApptEndWhole),
	NAME_ENTRY(APPTDURATION, PT_LONG, PSETID_Appointment, dispidApptDuration),
	NAME_ENTRY(ALLDAYEVENT, PT_BOOLEAN, PSETID_Appointment, dispidAllDayEvent),
	NAME_ENTRY(RECURRENCESTATE, PT_BINARY, PSETID_Appointment, dispidRecurrenceState),
	NAME_ENTRY(MEETINGSTATUS, PT_LONG, PSETID_Appointment, dispidMeetingStatus),
	NAME_ENTRY(RESPONSESTATUS, PT_LONG, PSETID_Appointment, dispidResponseStatus),
	NAME_ENTRY(RECURRING, PT_BOOLEAN, PSETID_Appointment, dispidRecurring),
	NAME_ENTRY(INTENDEDBUSYSTATUS, PT_LONG, PSETID_Appointment, dispidIntendedBusyStatus),
	NAME_ENTRY(RECURRINGBASE, PT_SYSTIME, PSETID_Appointment, dispidRecurringBase),
	NAME_ENTRY(REQUESTSENT, PT_BOOLEAN, PSETID_Appointment, dispidRequestSent),		// aka PidLidFInvited
	NAME_ENTRY(APPTREPLYNAME, PT_TSTRING, PSETID_Appointment, dispidApptReplyName),
	NAME_ENTRY(RECURRENCETYPE, PT_LONG, PSETID_Appointment, dispidRecurrenceType),
	NAME_ENTRY(RECURRENCEPATTERN, PT_TSTRING, PSETID_Appointment, dispidRecurrencePattern),
	NAME_ENTRY(TIMEZONEDATA, PT_BINARY, PSETID_Appointment, dispidTimeZoneData),
	NAME_ENTRY(TIMEZONE, PT_TSTRING, PSETID_Appointment, dispidTimeZone),
	NAME_ENTRY(RECURRENCE_START, PT_SYSTIME, PSETID_Appointment, dispidClipStart),
	NAME_ENTRY(RECURRENCE_END, PT_SYSTIME, PSETID_Appointment, dispidClipEnd),
	NAME_ENTRY(ALLATTENDEESSTRING, PT_TSTRING, PSETID_Appointment, dispidAllAttendeesString),	// AllAttendees (Exluding self, ',' seperated)
	NAME_ENTRY(TOATTENDEESSTRING, PT_TSTRING, PSETID_Appointment, dispidToAttendeesString),	// RequiredAttendees (Including self)
	NAME_ENTRY(CCATTENDEESSTRING, PT_TSTRING, PSETID_Appointment, dispidCCAttendeesString),	// OptionalAttendees
	NAME_ENTRY(NETMEETINGTYPE, PT_LONG, PSETID_Appointment, dispidNetMeetingType),
	NAME_ENTRY(NETMEETINGSERVER, PT_TSTRING, PSETID_Appointment, dispidNetMeetingServer),
	NAME_ENTRY(NETMEETINGORGANIZERALIAS, PT_TSTRING, PSETID_Appointment, dispidNetMeetingOrganizerAlias),
	NAME_ENTRY(NETMEETINGAUTOSTART, PT_BOOLEAN, PSETID_Appointment, dispidNetMeetingAutoStart),
	NAME_ENTRY(AUTOSTARTWHEN, PT_LONG, PSETID_Appointment, dispidAutoStartWhen),
	NAME_ENTRY(CONFERENCESERVERALLOWEXTERNAL,PT_BOOLEAN, PSETID_Appointment, dispidConferenceServerAllowExternal),
	NAME_ENTRY(NETMEETINGDOCPATHNAME, PT_TSTRING, PSETID_Appointment, dispidNetMeetingDocPathName),
	NAME_ENTRY(NETSHOWURL, PT_TSTRING,  PSETID_Appointment, dispidNetShowURL),
	NAME_ENTRY(CONVERENCESERVERPASSWORD, PT_TSTRING, PSETID_Appointment, dispidConferenceServerPassword),
	NAME_ENTRY(APPTREPLYTIME, PT_SYSTIME, PSETID_Appointment, dispidApptReplyTime),
	NAME_ENTRY(REMINDERMINUTESBEFORESTART,PT_LONG,  PSETID_Common, dispidReminderMinutesBeforeStart),
	NAME_ENTRY(REMINDERTIME, PT_SYSTIME, PSETID_Common, dispidReminderTime),
	NAME_ENTRY(REMINDERSET, PT_BOOLEAN, PSETID_Common, dispidReminderSet),
	NAME_ENTRY(PRIVATE, PT_BOOLEAN, PSETID_Common, dispidPrivate),
	NAME_ENTRY(NOAGING, PT_BOOLEAN, PSETID_Common, dispidNoAging),
	NAME_ENTRY(SIDEEFFECT, PT_LONG, PSETID_Common, dispidSideEffect),
	NAME_ENTRY(REMOTESTATUS, PT_LONG, PSETID_Common, dispidRemoteStatus),
	NAME_ENTRY(COMMONSTART, PT_SYSTIME, PSETID_Common, dispidCommonStart),
	NAME_ENTRY(COMMONEND, PT_SYSTIME, PSETID_Common, dispidCommonEnd),
	NAME_ENTRY(COMMONASSIGN, PT_LONG, PSETID_Common, dispidCommonAssign),
	NAME_ENTRY(CONTACTS, PT_MV_UNICODE, PSETID_Common, dispidContacts),
	NAME_ENTRY(OUTLOOKINTERNALVERSION, PT_LONG, PSETID_Common, dispidOutlookInternalVersion),
	NAME_ENTRY(OUTLOOKVERSION, PT_TSTRING, PSETID_Common, dispidOutlookVersion),
	NAME_ENTRY(REMINDERNEXTTIME, PT_SYSTIME, PSETID_Common, dispidReminderNextTime),
	NAME_ENTRY(HIDE_ATTACH, PT_BOOLEAN, PSETID_Common, dispidSmartNoAttach),	
	NAME_ENTRY(TASK_STATUS, PT_LONG, PSETID_Task, dispidTaskStatus),
	NAME_ENTRY(TASK_COMPLETE, PT_BOOLEAN, PSETID_Task, dispidTaskComplete),
	NAME_ENTRY(TASK_PERCENTCOMPLETE, PT_DOUBLE, PSETID_Task, dispidTaskPercentComplete),
	NAME_ENTRY(TASK_STARTDATE, PT_SYSTIME, PSETID_Task, dispidTaskStartDate),
	NAME_ENTRY(TASK_DUEDATE, PT_SYSTIME, PSETID_Task, dispidTaskDueDate),
	NAME_ENTRY(TASK_RECURRSTATE, PT_BINARY, PSETID_Task, dispidTaskRecurrenceState),
	NAME_ENTRY(TASK_ISRECURRING, PT_BOOLEAN, PSETID_Task, dispidTaskIsRecurring),
	NAME_ENTRY(TASK_COMPLETED_DATE, PT_SYSTIME, PSETID_Task, dispidTaskDateCompleted),
	NAME_ENTRY(APPTTZDEFSTARTDISP, PT_BINARY, PSETID_Appointment, dispidApptTZDefStartDisplay),
	NAME_ENTRY(APPTTZDEFENDDISP, PT_BINARY, PSETID_Appointment, dispidApptTZDefEndDisplay),
	NAME_ENTRY(APPTTZDEFRECUR, PT_BINARY, PSETID_Appointment, dispidApptTZDefRecur)
};

/**
 * Create a PropertyPool instance that uses lpMapiProp as the MAPI object to
 * do the conversion of names to tags on.
 * 
 * @param[in]	lpMapiProp			An IMAPIProp derived object.
 * @param[out]	lppPropertyPool		The new PropertyPool instance. Must be deleted by the caller.
 */
HRESULT PropertyPool::Create(IMAPIProp *lpMapiProp, PropertyPool **lppPropertyPool)
{
	HRESULT hr = hrSuccess;
	PropertyPoolPtr ptrPropPool;

	if (lpMapiProp == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	try {
		ptrPropPool.reset(new PropertyPool());
	} catch (const std::bad_alloc &) {
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}

	hr = ptrPropPool->LookupNames(lpMapiProp);
	if (hr != hrSuccess)
		goto exit;	

	*lppPropertyPool = ptrPropPool.release();

exit:
	return hr;
}

/**
 * Get the MAPINAMEID structure and the PROP_TYPE for a specific property name.
 * 
 * @param[in]	name		The property name for which to obtain the MAPINAMEID and PROP_TYPE.
 * @param[out]	lppsNameId	The MAPINAMEID for name. This must be freed by the caller.
 * @param[out]	lpulType	The PROP_TYPE for name.
 */
HRESULT PropertyPool::GetPropIdAndType(PropertyPool::PropNames name, MAPINAMEID **lppsNameId, ULONG *lpulType)
{
	HRESULT hr = hrSuccess;

	if (name < 0 || name >= SIZE_NAMEDPROPS) {
		hr = MAPI_E_BAD_VALUE;
		goto exit;
	}

	if (lppsNameId == NULL && lpulType == NULL) {
		hr = MAPI_E_INVALID_PARAMETER;
		goto exit;
	}

	if (lppsNameId != NULL) {
		MAPINameIdPtr ptrNameId;
		MAPINAMEID *lpNameId = g_asNameEntries[name].sPropMapEntry.GetMAPINameId();

		hr = MAPIAllocateBuffer(sizeof(MAPINAMEID), &ptrNameId);
		if (hr != hrSuccess)
			goto exit;

		hr = MAPIAllocateMore(sizeof(GUID), ptrNameId, (LPVOID*)&ptrNameId->lpguid);
		if (hr != hrSuccess)
			goto exit;

		memcpy(ptrNameId->lpguid, lpNameId->lpguid, sizeof(GUID));
		ptrNameId->ulKind = lpNameId->ulKind;

		if (lpNameId->ulKind == MNID_ID)
			ptrNameId->Kind.lID = lpNameId->Kind.lID;
		else {
			hr = MAPIAllocateMore((wcslen(lpNameId->Kind.lpwstrName) + 1) * sizeof(WCHAR), ptrNameId, (LPVOID*)&ptrNameId->Kind.lpwstrName);
			if (hr != hrSuccess)
				goto exit;

			wcscpy(ptrNameId->Kind.lpwstrName, lpNameId->Kind.lpwstrName);
		}

		*lppsNameId = ptrNameId.release();
	}

	if (lpulType != NULL)
		*lpulType = g_asNameEntries[name].ulPropType;

exit:
	return hr;
}

/**
 * Get an array of property tags that are required to construct an Appointment instance.
 * 
 * @param[in]	lpExtraTags		An SPropTagArray containing additional tags that should be
 * 								returned. This is for conveniance because a caller might have
 * 								a set of properties that is requires as well.
 * @param[out]	lppPropTagArray	The SPropTagArray that contains the required property tags and
 * 								the extra tags.
 */
HRESULT PropertyPool::GetRequiredPropTags(SPropTagArray *lpExtraTags, SPropTagArray **lppPropTagArray) const
{
	HRESULT hr = hrSuccess;
	SPropTagArrayPtr ptrPropTagArray;

	ULONG cPropTags = g_sptaRequiredProps.cValues + arraySize(g_aulRequiredNames);
	if (lpExtraTags != NULL)
		cPropTags += lpExtraTags->cValues;

	hr = MAPIAllocateBuffer(CbNewSPropTagArray(cPropTags), &ptrPropTagArray);
	if (hr != hrSuccess)
		goto exit;

	memcpy(ptrPropTagArray->aulPropTag, g_sptaRequiredProps.aulPropTag, g_sptaRequiredProps.cValues * sizeof(ULONG));
	for (size_t i = 0; i < arraySize(g_aulRequiredNames); ++i)
		ptrPropTagArray->aulPropTag[g_sptaRequiredProps.cValues + i] = m_aulPropTag[g_aulRequiredNames[i]];
	if (lpExtraTags != NULL)
		memcpy(ptrPropTagArray->aulPropTag + g_sptaRequiredProps.cValues + arraySize(g_aulRequiredNames), lpExtraTags->aulPropTag, lpExtraTags->cValues * sizeof(ULONG));
	ptrPropTagArray->cValues = cPropTags;

	*lppPropTagArray = ptrPropTagArray.release();

exit:
	return hr;
}

/**
 * Get the proptag for a propname.
 * 
 * @param[in]	name	The property name for which to obtain the proptag.
 * @return	The requested proptag.
 */
ULONG PropertyPool::GetPropTag(PropertyPool::PropNames name) const
{
	if (name < 0 || name >= SIZE_NAMEDPROPS)
		return PT_NULL;

	return m_aulPropTag[name];
}

PropertyPool::PropertyPool()
{ }

PropertyPool::~PropertyPool()
{ }

/**
 * Convert the property names to property tags.
 * 
 * @param[in]	lpMapiProp	The IMAPIProp derived instance used to
 * 							perform the conversion on.
 */
HRESULT PropertyPool::LookupNames(IMAPIProp *lpMapiProp)
{
	HRESULT hr = hrSuccess;

	ASSERT(lpMapiProp != NULL);

	PROPMAP_START;
	for (size_t i = 0; i < arraySize(g_asNameEntries); ++i)
		__propmap.AddProp(&m_aulPropTag[g_asNameEntries[i].ulIndex], g_asNameEntries[i].ulPropType, g_asNameEntries[i].sPropMapEntry);
	PROPMAP_INIT(lpMapiProp);

exit:
	return hr;
}
