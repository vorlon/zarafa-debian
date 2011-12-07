from MAPI.Defs import *
from MAPI.Tags import *
from MAPI.Struct import *

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
