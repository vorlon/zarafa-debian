# -*- indent-tabs-mode: nil -*-

import random

# MAPI stuff
from MAPI.Defs import *
from MAPI.Tags import *
from MAPI.Struct import *

# For backward compatibility
from AddressBook import GetUserList

# flags = 1 == EC_PROFILE_FLAGS_NO_NOTIFICATIONS
def OpenECSession(user, password, path, **keywords):
    profname = '__pr__%d' % random.randint(0,100000)
    profadmin = MAPIAdminProfiles(0)
    profadmin.CreateProfile(profname, None, 0, 0)
    admin = profadmin.AdminServices(profname, None, 0, 0)
    if keywords.has_key('providers'):
        for provider in keywords['providers']:
            admin.CreateMsgService(provider, provider, 0, 0)
    else:
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
