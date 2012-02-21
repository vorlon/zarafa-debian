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

#ifndef ECINDEXDB_H
#define ECINDEXDB_H

#include <string>
#include <list>
#include <map>
#include <set>

#include <mapidefs.h>

class ECConfig;
class ECLogger;
class ECDatabaseMySQL;
class ECAnalyzer;

class storeid_t {
public:
    storeid_t() { };
    storeid_t(LPMAPIUID server, LPMAPIUID store) {
        serverGuid.assign((char *)server, sizeof(GUID));
        storeGuid.assign((char *)store, sizeof(GUID));
    }
    storeid_t(LPSPropValue lpServerGuid, LPSPropValue lpStoreGuid) {
        serverGuid.assign((char *)lpServerGuid->Value.bin.lpb, lpServerGuid->Value.bin.cb);
        storeGuid.assign((char *)lpStoreGuid->Value.bin.lpb, lpStoreGuid->Value.bin.cb);
    }
        
    std::string serverGuid;
    std::string storeGuid;
    
    bool operator < (const storeid_t &other) const {
        return std::make_pair(serverGuid, storeGuid) < std::make_pair(other.serverGuid, storeGuid);
    }
};

typedef unsigned int folderid_t;
typedef unsigned int docid_t;
typedef unsigned int fieldid_t;

class ECIndexDB {
public:
    ECIndexDB(ECConfig *lpConfig, ECLogger *lpLogger);
    ~ECIndexDB();
    
    HRESULT AddTerm(storeid_t store, folderid_t folder, docid_t doc, fieldid_t field, unsigned int ulVersion, std::wstring wstrTerm);
    HRESULT RemoveTermsStore(storeid_t store);
    HRESULT RemoveTermsFolder(storeid_t store, folderid_t folder);
    HRESULT RemoveTermsDoc(storeid_t store, docid_t doc, unsigned int *lpulVersion);
    HRESULT RemoveTermsDoc(storeid_t store, folderid_t folder, std::string strSourceKey);
    
    // We need to track the sourcekey of documents to be able to handle deletions
    HRESULT AddSourcekey(storeid_t store, folderid_t folder, std::string strSourceKey, docid_t doc);
    
    HRESULT QueryTerm(storeid_t store, std::list<unsigned int> &lstFolders, std::set<unsigned int> &setFields, std::wstring &wstrTerm, std::list<docid_t> &matches);
    
private:
    HRESULT Begin();
    HRESULT Commit();
    HRESULT Rollback();
    
    HRESULT GetStoreId(storeid_t store, unsigned int *lpulStoreId, bool bCreate);

    ECLogger *m_lpLogger;
    ECConfig *m_lpConfig;
    ECAnalyzer *m_lpAnalyzer;
    ECDatabaseMySQL *m_lpDatabase;
    
    bool m_bConnected;
    
    std::map<storeid_t, unsigned int> m_mapStores;
    std::set<unsigned int> m_setExcludeProps;
};

#endif
