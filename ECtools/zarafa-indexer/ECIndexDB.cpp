/*
 * Copyright 2005 - 2011  Zarafa B.V.
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
#include "ECIndexDB.h"
#include "ECDatabase.h"
#include "ECAnalyzers.h"
#include "ECLogger.h"

#include "ECConfig.h"
#include "CommonUtil.h"
#include "stringutil.h"
#include "charset/convert.h"

#include <list>
#include <string>

#include <CLucene/util/Reader.h>

ECIndexDB::ECIndexDB(ECConfig *lpConfig, ECLogger *lpLogger)
{
    m_lpConfig = lpConfig;
    m_lpLogger = lpLogger;
    m_bInTransaction = false;
    m_ulChanges = 0;
    
    m_lpDatabase = new ECDatabase(lpLogger);
    
    ECRESULT er = m_lpDatabase->Connect(lpConfig);
    if (er == ZARAFA_E_DATABASE_NOT_FOUND) {
        m_lpDatabase->GetLogger()->Log(EC_LOGLEVEL_INFO, "Database not found, creating database.");
        er = m_lpDatabase->CreateDatabase(lpConfig);
    }
    
    m_lpAnalyzer = new ECAnalyzer();
}

ECIndexDB::~ECIndexDB()
{
    if(m_bInTransaction) {
        Flush();
    }
    
    if(m_lpDatabase)
        delete m_lpDatabase;
    if(m_lpAnalyzer)
        delete m_lpAnalyzer;
}

HRESULT ECIndexDB::AddTerm(storeid_t store, folderid_t folder, docid_t doc, fieldid_t field, unsigned int version, std::wstring wstrTerm)
{
    HRESULT hr = hrSuccess;
    std::string strQuery;
    lucene::analysis::TokenStream* stream = NULL;
    lucene::analysis::Token* token = NULL;
    unsigned int ulStoreId = 0;
    bool bTerms = false;
    
    lucene::util::StringReader reader(wstrTerm.c_str());
    
    stream = m_lpAnalyzer->tokenStream(L"", &reader);
    
    hr = GetStoreId(store, &ulStoreId, true);
    if(hr != hrSuccess)
        goto exit;

    hr = EnsureTransaction();
    if(hr != hrSuccess)
        goto exit;

    strQuery = "REPLACE into words(store, folder, doc, field, term, version) VALUES";

    while((token = stream->next())) {
        strQuery += "(" + stringify(ulStoreId) + "," + stringify(folder) + "," + stringify(doc) + "," + stringify(field) + ", '" + m_lpDatabase->Escape(convert_to<std::string>("utf-8", token->termText(), rawsize(token->termText()), CHARSET_WCHAR)) + "'," + stringify(version) + "),";
    
        delete token;
        
        m_ulChanges++;
        bTerms = true;
    }
    
    if(bTerms) {
        // Remove trailing comma
        strQuery.resize(strQuery.size()-1);

        hr = m_lpDatabase->DoInsert(strQuery);
        if(hr != hrSuccess)
            goto exit;
                
        if(m_ulChanges > 10000)
            Flush();
    }

exit:
    if(stream)
        delete stream;
    
    return hr;
}

HRESULT ECIndexDB::RemoveTermsStore(storeid_t store)
{
    return hrSuccess;
}

HRESULT ECIndexDB::RemoveTermsFolder(storeid_t store, folderid_t folder)
{
    return hrSuccess;
}

/**
 * Remove terms for a document in preparation for update
 *
 * In practice, actual deletion is deferred until a later time since removal of
 * search terms is an expensive operation. We track removed or updates messages
 * by inserting them into the 'updates' table, which will remove them from any
 * future search queries.
 *
 * If this delete is called prior to a call to AddTerm(), the returned version
 * must be used in the call to AddTerm(), which ensures that the new search terms
 * will be returned in future searches.
 *
 * @param[in] store Store ID to remove document from
 * @param[in] doc Document ID to remove
 * @param[out] lpulVersion Version of new terms to be added
 * @return Result
 */
HRESULT ECIndexDB::RemoveTermsDoc(storeid_t store, docid_t doc, unsigned int *lpulVersion)
{
    HRESULT hr = hrSuccess;
    std::string strQuery;
    unsigned int ulStoreId = 0;

    hr = GetStoreId(store, &ulStoreId, true);
    if(hr != hrSuccess)
        goto exit;
    
    strQuery = "REPLACE INTO updates (doc, store) VALUES (" + stringify(doc) + "," + stringify(ulStoreId) + ")";
    
    hr = m_lpDatabase->DoInsert(strQuery, lpulVersion);

exit:    
    return hr;
}

/**
 * Remove terms for a document
 *
 * Uses the same logic as RemoveTermsDoc() but takes a sourcekey as a parameter.
 *
 * @param[in] store Store ID to remove document from
 * @param[in[ folder Folder ID to remove document from
 * @param[in] strSourceKey Source key to remove
 * @return Result
 */
HRESULT ECIndexDB::RemoveTermsDoc(storeid_t store, folderid_t folder, std::string strSourceKey)
{
    HRESULT hr = hrSuccess;
    std::string strQuery;
    DB_ROW lpRow = NULL;
    DB_RESULT lpResult = NULL;
    unsigned int ulStoreId = 0;

    hr = GetStoreId(store, &ulStoreId, true);
    if(hr != hrSuccess)
        goto exit;
    
    strQuery = "SELECT doc FROM sourcekeys WHERE store = " + stringify(ulStoreId) + " AND folder = " + stringify(folder) + " AND sourcekey = " + m_lpDatabase->EscapeBinary(strSourceKey);
    hr = m_lpDatabase->DoSelect(strQuery, &lpResult);
    if (hr != hrSuccess)
        goto exit;
        
    lpRow = m_lpDatabase->FetchRow(lpResult);
    
    if(!lpRow || !lpRow[0]) {
        hr = MAPI_E_NOT_FOUND;
        goto exit;
    }
    
    hr = RemoveTermsDoc(store, atoui(lpRow[0]), NULL);
    if (hr != hrSuccess)
        goto exit;
        
    hr = m_lpDatabase->DoDelete("DELETE FROM sourcekeys WHERE store = " + stringify(ulStoreId) + " AND folder = " + stringify(folder) + " AND sourcekey = " + m_lpDatabase->EscapeBinary(strSourceKey));

exit:    
    if (lpResult)
        m_lpDatabase->FreeResult(lpResult);
        
    return hr;
}


HRESULT ECIndexDB::Flush()
{
    HRESULT hr = m_lpDatabase->DoInsert("COMMIT");
    if(hr != hrSuccess)
        goto exit;
        
    m_bInTransaction = false;
    m_ulChanges = 0;
    
exit:
    return hr;
}

HRESULT ECIndexDB::QueryTerm(storeid_t store, std::list<unsigned int> &lstFolders, std::set<unsigned int> &setFields, std::wstring &wstrTerm, std::list<docid_t> &lstMatches)
{
    HRESULT hr = hrSuccess;
    unsigned int ulStoreId = 0;
    DB_ROW lpRow = NULL;
    DB_RESULT lpResult = NULL;
    std::string strQuery;

    lstMatches.clear();
    
    hr = GetStoreId(store, &ulStoreId, false);
    if (hr != hrSuccess)
        goto exit;
    
    strQuery = "SELECT DISTINCT words.doc FROM words LEFT JOIN updates ON updates.doc = words.doc AND updates.store = " + stringify(ulStoreId) + " WHERE ";
    
    if(setFields.empty() || wstrTerm.empty())
        goto exit;
    
    strQuery += " words.field IN(";
    for(std::set<unsigned int>::iterator i = setFields.begin(); i != setFields.end(); i++) {
        strQuery += stringify(*i);
        strQuery += ",";
    }
    strQuery.resize(strQuery.size()-1);
    strQuery += ") ";
    
    strQuery += "AND words.term LIKE '" + m_lpDatabase->Escape(convert_to<std::string>("utf-8", wstrTerm, rawsize(wstrTerm), CHARSET_WCHAR)) + "%' ";
    
    if(!lstFolders.empty()) {
        strQuery += "AND words.folder IN (";
        for(std::list<unsigned int>::iterator i = lstFolders.begin(); i != lstFolders.end(); i++) {
            strQuery += stringify(*i);
            strQuery += ",";
        }
        strQuery.resize(strQuery.size()-1);
        strQuery += ") ";
    }
        
    strQuery += "AND words.store = " + stringify(ulStoreId) + " ";
    strQuery += "AND (updates.id IS NULL or words.version = updates.id) ";
    
    strQuery += "ORDER BY doc"; // We *must* output in sorted order since the caller expects this

    hr = m_lpDatabase->DoSelect(strQuery, &lpResult, true);
    if (hr != hrSuccess)
        goto exit;
        
    while((lpRow = m_lpDatabase->FetchRow(lpResult))) {
        if(lpRow[0] == NULL) {
            ASSERT(false);
            continue;
        }
        lstMatches.push_back(atoui(lpRow[0]));
    }
        
exit:
    if (lpResult)
        m_lpDatabase->FreeResult(lpResult);
        
    return hrSuccess;
}

HRESULT ECIndexDB::EnsureTransaction()
{
    HRESULT hr = hrSuccess;
    
    if(!m_bInTransaction) {
        hr = m_lpDatabase->DoInsert("BEGIN");
        if(hr != hrSuccess)
            goto exit;
            
        m_bInTransaction = true;
    }
exit:
    return hr;
}

/**
 * Map storeid (serverguid/storeguid) to an integer store ID
 *
 * We keep a small cache to make sure that we don't query the database all the time. If the store ID
 * was not found in the database, a store ID is generated. The function will therefore only fail
 * if a database error occurs.
 *
 * @param[in] store Store to map
 * @param[out] lpulStoreId Output of integer store id
 * @param[in] bCreate TRUE if we should create the id if needed
 * @return result
 */
HRESULT ECIndexDB::GetStoreId(storeid_t store, unsigned int *lpulStoreId, bool bCreate)
{
    HRESULT hr = hrSuccess;
    
    unsigned int ulStoreId = 0;
    std::map<storeid_t, unsigned int>::iterator i;
    std::string strQuery;
    DB_RESULT lpResult = NULL;
    DB_ROW lpRow = NULL;

    i = m_mapStores.find(store);
    if(i != m_mapStores.end()) {
        // Found in cache
        *lpulStoreId = i->second;
        goto exit;
    }
    
    strQuery = "SELECT id FROM stores WHERE serverguid = " + m_lpDatabase->EscapeBinary(store.serverGuid) + " AND storeguid = " + m_lpDatabase->EscapeBinary(store.storeGuid);
    
    hr = m_lpDatabase->DoSelect(strQuery, &lpResult);
    if(hr != hrSuccess)
        goto exit;
        
    lpRow = m_lpDatabase->FetchRow(lpResult);
    
    if(!lpRow || !lpRow[0]) {
        if (bCreate) {
            strQuery = "INSERT INTO stores (serverguid, storeguid) VALUES(" + m_lpDatabase->EscapeBinary(store.serverGuid) + "," + m_lpDatabase->EscapeBinary(store.storeGuid) + ")";
            hr = m_lpDatabase->DoInsert(strQuery, &ulStoreId);
            if (hr != hrSuccess)
                goto exit;
        } else {
            hr = MAPI_E_NOT_FOUND;
            goto exit;
        }
    } else {
        ulStoreId = atoui(lpRow[0]);
    }

    m_mapStores.insert(std::make_pair(store, ulStoreId));    
    *lpulStoreId = ulStoreId;
    
exit:
    if(lpResult)
        m_lpDatabase->FreeResult(lpResult);

    return hr;        
}

/**
 * Add document sourcekey
 *
 * @param[in] store Store ID of document
 * @param[in] folder Folder ID of document (hierarchyid)
 * @param[in] strSourceKey Source key of document
 * @param[in] doc Document ID of document (hierarchyid)
 * @return result
 */
HRESULT ECIndexDB::AddSourcekey(storeid_t store, folderid_t folder, std::string strSourceKey, docid_t doc)
{
    HRESULT hr = hrSuccess;
    std::string strQuery;
    unsigned int ulStoreId = 0;

    hr = GetStoreId(store, &ulStoreId, true);
    if(hr != hrSuccess)
        goto exit;
    
    strQuery = "REPLACE INTO sourcekeys(store, folder, sourcekey, doc) VALUES(" + stringify(ulStoreId) + "," + stringify(folder) + "," + m_lpDatabase->EscapeBinary(strSourceKey) + "," + stringify(doc) + ")";
    
    hr = m_lpDatabase->DoInsert(strQuery);
    
    if(hr != hrSuccess)
        goto exit;
        
exit:
    return hr;
}

