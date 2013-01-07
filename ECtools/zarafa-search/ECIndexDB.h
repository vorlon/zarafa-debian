/*
 * Copyright 2005 - 2013  Zarafa B.V.
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

#include <unicode/coll.h>
#include <mapidefs.h>
#include <kchashdb.h>

class ECConfig;
class ECLogger;
class ECAnalyzer;
class ECIndexFactory;

typedef unsigned int folderid_t;
typedef unsigned int docid_t;
typedef unsigned int fieldid_t;

class ECIndexDB {
public:
    HRESULT AddTerm(folderid_t folder, docid_t doc, fieldid_t field, unsigned int ulVersion, std::wstring wstrTerm);
    HRESULT RemoveTermsFolder(folderid_t folder);
    HRESULT RemoveTermsDoc(folderid_t folder, docid_t doc, unsigned int *lpulVersion);
    
    HRESULT StartTransaction();
    HRESULT EndTransaction();

    HRESULT Normalize(const std::wstring &strInput, std::list<std::wstring> &lstOutput);
    HRESULT QueryTerm(std::list<unsigned int> &lstFolders, std::set<unsigned int> &setFields, std::wstring &wstrTerm, std::list<docid_t> &matches);

    HRESULT	SetSyncState(const std::string& strFolder, const std::string& strState, const std::string& strStubTargets);
    HRESULT	GetSyncState(const std::string& strFolder, std::string& strState, std::string& strStubTargets);

    bool Complete();
    HRESULT SetComplete();

	HRESULT Optimize();

private:
    static HRESULT Create(const std::string& strIndexId, ECConfig *lpConfig, ECLogger *lpLogger, bool bCreate, bool bComplete, ECIndexDB **lppIndexDB);

    HRESULT FlushCache(kyotocabinet::TinyHashMap *lpCache = NULL, bool bTransaction = true);
    unsigned int QueryFromDB(unsigned int key);
	HRESULT WriteToDB(unsigned int key, unsigned int value);
    
    ECIndexDB(const std::string& strIndexId, ECConfig *lpConfig, ECLogger *lpLogger);
    ~ECIndexDB();

    size_t GetSortKey(const wchar_t *wszInput, size_t len, char *szOutput, size_t outLen);
    
    HRESULT Open(const std::string &strIndexId, bool bCreate, bool bComplete);
    
    ECLogger *m_lpLogger;
    ECConfig *m_lpConfig;
    ECAnalyzer *m_lpAnalyzer;
    
    std::set<unsigned int> m_setExcludeProps;

    kyotocabinet::TreeDB *m_lpIndex;
    kyotocabinet::TinyHashMap *m_lpCache;
    
    Collator *m_lpCollator;

    friend class ECIndexFactory; // Create / Delete of ECIndexDB only allowed from factory
    
    unsigned int m_ulCacheSize;
    bool m_bComplete;
};

#endif
