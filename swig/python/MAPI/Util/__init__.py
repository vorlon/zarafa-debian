# -*- indent-tabs-mode: nil -*-

from MAPI.Defs import *
from MAPI.Tags import *
from MAPI.Struct import *
import random

# flags = 1 == EC_PROFILE_FLAGS_NO_NOTIFICATIONS
def OpenECSession(user, password, path, **keywords):
    profname = '__pr__%d' % random.randint(0,100000)
    profadmin = MAPIAdminProfiles(0)
    profadmin.CreateProfile(profname, None, 0, 0)
    admin = profadmin.AdminServices(profname, None, 0, 0)
    admin.CreateMsgService("ZARAFA6", "Zarafa", 0, 0)
    table = admin.GetMsgServiceTable(0)
    rows = table.QueryRows(1,0)
    prop = PpropFindProp(rows[0], PR_SERVICE_UID)
    uid = prop.Value
    profprops = [ SPropValue(PR_EC_PATH, path) ]
    if user.__class__.__name__ == 'unicode':
        profprops += [ SPropValue(PR_EC_USERNAME_W, user) ]
    else:
        profprops += [ SPropValue(PR_EC_USERNAME_A, user) ]
    if password.__class__.__name__ == 'unicode':
        profprops += [ SPropValue(PR_EC_USERPASSWORD_W, password) ]
    else:
        profprops += [ SPropValue(PR_EC_USERPASSWORD_A, password) ]

    if keywords.has_key('sslkey_file') and keywords['sslkey_file']:
        profprops += [ SPropValue(PR_EC_SSLKEY_FILE, keywords['sslkey_file']) ]
    if keywords.has_key('sslkey_pass') and keywords['sslkey_pass']:
        profprops += [ SPropValue(PR_EC_SSLKEY_PASS, keywords['sslkey_pass']) ]

    flags = 1
    if keywords.has_key('flags'):
        flags = keywords['flags']
    profprops += [ SPropValue(PR_EC_FLAGS, flags) ]
    
    admin.ConfigureMsgService(uid, 0, 0, profprops)
        
    session = MAPILogonEx(0,profname,None,0)
    
    profadmin.DeleteProfile(profname, 0)
    return session
    
def GetDefaultStore(session):
    table = session.GetMsgStoresTable(0)
    
    table.SetColumns([PR_DEFAULT_STORE, PR_ENTRYID], 0)
    rows = table.QueryRows(25,0)
    
    for row in rows:
        if(row[0].ulPropTag == PR_DEFAULT_STORE and row[0].Value):
            return session.OpenMsgStore(0, row[1].Value, None, MDB_WRITE)
            
    return None

def GetPublicStore(session):
    table = session.GetMsgStoresTable(0)

    table.SetColumns([PR_MDB_PROVIDER, PR_ENTRYID], 0)
    rows = table.QueryRows(25,0)

    for row in rows:
        if(row[0].ulPropTag == PR_MDB_PROVIDER and row[0].Value == ZARAFA_STORE_PUBLIC_GUID):
            return session.OpenMsgStore(0, row[1].Value, None, MDB_WRITE)


def GetCompanyList(session):
    companies = []
    ab = session.OpenAddressBook(0, None, 0)
    gab = ab.OpenEntry(ab.GetDefaultDir(), None, 0)
    table = gab.GetHierarchyTable(0)
    table.SetColumns([PR_DISPLAY_NAME], TBL_BATCH)
    while True:
        rows = table.QueryRows(50, 0)
        if len(rows) == 0:
            break
        companies.extend((row[0].Value for row in rows))
    return companies

def _GetAbObjectList(container, restriction):
    users = []
    table = container.GetContentsTable(0)
    table.SetColumns([MAPI.Tags.PR_EMAIL_ADDRESS], MAPI.TBL_BATCH)
    if restriction:
        table.Restrict(restriction, TBL_BATCH)
    while True:
        rows = table.QueryRows(50, 0)
        if len(rows) == 0:
            break
        users.extend((row[0].Value for row in rows))
    return users

def GetAbObjectList(session, restriction = None, company = None):
    ab = session.OpenAddressBook(0, None, 0)
    gab = ab.OpenEntry(ab.GetDefaultDir(), None, 0)
    table = gab.GetHierarchyTable(0)
    if company is None or len(company) == 0:
        companyCount = table.GetRowCount(0)
        if companyCount == 0:
            users = _GetAbObjectList(gab, restriction)
        else:
            table.SetColumns([PR_ENTRYID], TBL_BATCH)
            users = []
            for row in table.QueryRows(companyCount, 0):
                company = gab.OpenEntry(row[0].Value, None, 0)
                companyUsers = _GetAbObjectList(company, restriction)
                users.extend(companyUsers)
    else:
        table.FindRow(SContentRestriction(FL_FULLSTRING|FL_IGNORECASE, PR_DISPLAY_NAME, SPropValue(PR_DISPLAY_NAME, company)), BOOKMARK_BEGINNING, 0)
        table.SetColumns([PR_ENTRYID], TBL_BATCH)
        row = table.QueryRows(1, 0)[0]
        users = _GetAbObjectList(ab.OpenEntry(row[0].Value, None, 0), restriction)
        
    return users

def GetUserList(session, company = None):
    restriction = SAndRestriction([SPropertyRestriction(RELOP_EQ, PR_OBJECT_TYPE, SPropValue(PR_OBJECT_TYPE, MAPI_MAILUSER)),
                                   SPropertyRestriction(RELOP_NE, PR_DISPLAY_TYPE, SPropValue(PR_DISPLAY_TYPE, DT_REMOTE_MAILUSER))])
    return GetAbObjectList(session, restriction, company)

def GetGroupList(session, company = None):
    restriction = SPropertyRestriction(RELOP_EQ, PR_OBJECT_TYPE, SPropValue(PR_OBJECT_TYPE, MAPI_DISTLIST))
    return GetAbObjectList(session, restriction, company)
