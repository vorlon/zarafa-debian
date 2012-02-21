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

#include "CommonUtil.h"
#include "ECGenericObjectTable.h"
#include "stringutil.h"
#include "ECSearchClient.h"
#include "ECCacheManager.h"
#include "SOAPDebug.h"
#include "ZarafaCmdUtil.h"
#include "Util.h"
#include "ECStatsCollector.h"
#include "ECIndexer.h"

#include <map>
#include <string>
#include <list>
#include <boost/algorithm/string/join.hpp>

/**
 * Returns TRUE if the restriction is always FALSE.
 *
 * Currently this is only detected for the simple case 'AND(EXIST(proptag), NOT(EXIST(proptag)), ...)', with
 * any of the first level subparts in any order.
 *
 * @param lpRestrict[in]
 * @return result
 */
 
BOOL NormalizeRestrictionIsFalse(struct restrictTable *lpRestrict)
{
    std::set<unsigned int> setExist;
    std::set<unsigned int> setNotExist;
    std::set<unsigned int> setBoth;
    bool fAlwaysFalse = false;
    
    if(lpRestrict->ulType != RES_AND)
        goto exit;
        
    for(unsigned int i = 0; i < lpRestrict->lpAnd->__size ; i++) {
        if (lpRestrict->lpAnd->__ptr[i]->ulType == RES_EXIST)
            setExist.insert(lpRestrict->lpAnd->__ptr[i]->lpExist->ulPropTag);
        else if (lpRestrict->lpAnd->__ptr[i]->ulType == RES_NOT) {
            if (lpRestrict->lpAnd->__ptr[i]->lpNot->lpNot->ulType == RES_EXIST) {
                setNotExist.insert(lpRestrict->lpAnd->__ptr[i]->lpNot->lpNot->lpExist->ulPropTag);
            }
        }
    }
    
    set_intersection(setExist.begin(), setExist.end(), setNotExist.begin(), setNotExist.end(), inserter(setBoth, setBoth.begin()));
    
    if(!setBoth.empty())
        fAlwaysFalse = true;
        
exit:
    return fAlwaysFalse;
}

/**
 * Remove always false terms from an OR restriction
 *
 * Does inline removal of terms in an OR restriction that are always false (modifies the input structure)
 *
 * @param lpRestrict[in,out] Restriction to optimize
 * @return result
 */
ECRESULT NormalizeRestrictionRemoveFalseInOr(struct restrictTable *lpRestrict)
{
    ECRESULT er = erSuccess;
    
    if(lpRestrict->ulType != RES_OR)
        goto exit;
        
    for(unsigned int i = 0; i < lpRestrict->lpOr->__size;) {
        if(NormalizeRestrictionIsFalse(lpRestrict->lpOr->__ptr[i])) {
            delete lpRestrict->lpOr->__ptr[i];
            memmove(&lpRestrict->lpOr->__ptr[i], lpRestrict->lpOr->__ptr[i+1], sizeof(struct restrictTable *) * (lpRestrict->lpOr->__size - i - 1));
            lpRestrict->lpOr->__size--;
        } else {
            i++;
        }
    }
    
exit:
    return er;
}

/**
 * Normalize nested AND clauses in a restriction
 *
 * Recursively normalize nested AND clauses:
 *
 * ((A & B) & C) => (A & B & C)
 *
 * and (recursive)
 *
 * (((A & B) & C) & D) => (A & B & C & D)
 *
 * @param lpRestrict[in,out] Restriction to normalize
 * @return result
 */
ECRESULT NormalizeRestrictionNestedAnd(struct restrictTable *lpRestrict)
{
    ECRESULT er = erSuccess;
    std::list<struct restrictTable *> lstClauses;
    bool bModified = false;
    
    if(lpRestrict->ulType != RES_AND)
        goto exit;
        
    for(unsigned int i = 0; i < lpRestrict->lpAnd->__size; i++) {
        
        if(lpRestrict->lpAnd->__ptr[i]->ulType == RES_AND) {
            // First, flatten our subchild
            er = NormalizeRestrictionNestedAnd(lpRestrict->lpAnd->__ptr[i]);
            if (er != erSuccess)
                goto exit;

            // Now, get all the clauses from the child AND-clause and push them to this AND-clause
            for(unsigned j = 0; j < lpRestrict->lpAnd->__ptr[i]->lpAnd->__size; j++) {
                lstClauses.push_back(lpRestrict->lpAnd->__ptr[i]->lpAnd->__ptr[j]);
            }

            delete [] lpRestrict->lpAnd->__ptr[i]->lpAnd->__ptr;
            delete lpRestrict->lpAnd->__ptr[i]->lpAnd;
            
            bModified = true;
        } else {
            lstClauses.push_back(lpRestrict->lpAnd->__ptr[i]);
        }
    }
        
    if(bModified) {
        // We changed something, free the previous data and create a new list of children
        delete [] lpRestrict->lpAnd->__ptr;
        
        lpRestrict->lpAnd->__ptr = s_alloc<restrictTable *>(NULL, lstClauses.size());
        
        int n = 0;
        for(std::list<struct restrictTable *>::iterator clause = lstClauses.begin(); clause != lstClauses.end(); clause++) {
            lpRestrict->lpAnd->__ptr[n++] = *clause;
        }
        
        lpRestrict->lpAnd->__size = n;
    }
    
exit:
    return er;
}

/**
 * Creates a multisearch from and substring search or an OR with substring searches
 *
 * This works for OR restrictions containing CONTENT restrictions with FL_SUBSTRING and FL_IGNORECASE
 * options enabled, or standalone CONTENT restrictions. Nested ORs are also supported as long as all the
 * CONTENT restrictions in an OR (and sub-ORs) contain the same search term. 
 *
 * eg.:
 *
 * 1. (OR (OR (OR (f1: t1), f2: t1, f3: t1 ) ) ) => f1 f2 f3 : t1
 *
 * 2. (OR f1: t1, f2: t2) => FAIL (term mismatch)
 *
 * 3. (OR f1: t1, (AND f2 : t1, f3 : t1 ) ) => FAIL (AND not supported)
 *
 * @param lpRestrict[in] Restriction to parse
 * @param sMultiSearch[out] Found search terms for the entire lpRestrict tree
 * @return result
 */
ECRESULT NormalizeGetMultiSearch(struct restrictTable *lpRestrict, std::set<unsigned int> &setExcludeProps, SIndexedTerm& sMultiSearch)
{
    ECRESULT er = erSuccess;
    
    sMultiSearch.strTerm.clear();
    sMultiSearch.setFields.clear();
    
    if(lpRestrict->ulType == RES_OR) {
        for(unsigned int i = 0; i < lpRestrict->lpOr->__size ; i++) {
            SIndexedTerm terms;
            
            if(NormalizeRestrictionIsFalse(lpRestrict->lpOr->__ptr[i]))
                continue;
                
            er = NormalizeGetMultiSearch(lpRestrict->lpOr->__ptr[i], setExcludeProps, terms);
            if (er != erSuccess)
                goto exit;
                
            if(sMultiSearch.strTerm.empty()) {
                // This is the first term, copy it
                sMultiSearch = terms;
            } else {
                if(sMultiSearch.strTerm == terms.strTerm) {
                    // Add the search fields from the subrestriction into ours
                    sMultiSearch.setFields.insert(terms.setFields.begin(), terms.setFields.end());
                } else {
                    // There are different search terms in this OR (case 2)
                    er = ZARAFA_E_INVALID_PARAMETER;
                    goto exit;
                }
            }
        }
    } else if(lpRestrict->ulType == RES_CONTENT && (lpRestrict->lpContent->ulFuzzyLevel & (FL_SUBSTRING | FL_IGNORECASE)) == (FL_SUBSTRING | FL_IGNORECASE)) {
        if(setExcludeProps.find(PROP_ID(lpRestrict->lpContent->ulPropTag)) != setExcludeProps.end()) {
            // The property cannot be searched from the indexer since it has been excluded from indexing
            er = ZARAFA_E_NOT_FOUND;
            goto exit;
        }
        // Only support looking for string-type properties
        if(PROP_TYPE(lpRestrict->lpContent->lpProp->ulPropTag) != PT_STRING8 && PROP_TYPE(lpRestrict->lpContent->lpProp->ulPropTag) != PT_UNICODE) {
            er = ZARAFA_E_INVALID_PARAMETER;
            goto exit;
        }
        sMultiSearch.strTerm = lpRestrict->lpContent->lpProp->Value.lpszA;
        sMultiSearch.setFields.insert(PROP_ID(lpRestrict->lpContent->ulPropTag));
    } else {
        // Some other restriction type, unsupported (case 3)
        er = ZARAFA_E_INVALID_PARAMETER;
        goto exit;
    }
        
exit:
    return er;
}

/**
 * Normalizes the given restriction to a multi-field text search and the rest of the restriction
 *
 * Given a restriction R, modifies the restriction R and returns multi-field search terms so that the intersection
 * of the multi-field search and the new restriction R' produces the same result as restriction R
 *
 * Currently, this only works for restrictions that have the following structure (other restrictions are left untouched,
 * R' = R):
 *
 * AND(
 *  ... 
 *  OR(f1: t1, ..., fN: t1)
 *  ...
 *  OR(f1: t2, ..., fN: t2)
 *  ... other restriction parts
 * )
 *
 * in which 'f: t' represents a RES_CONTENT FL_SUBSTRING search (other match types are skipped)
 *
 * In this case the output will be:
 *
 * AND(
 *  ...
 *  ...
 * ) 
 * +
 * multisearch: f1 .. fN : t1 .. tN 
 *
 * (eg subject body from: word1 word2)
 *
 * If there are multiple OR clauses inside the initial AND clause, and the search fields DIFFER, then the FIRST 'OR'
 * field is used for the multifield search.
 */
ECRESULT NormalizeRestrictionMultiFieldSearch(struct restrictTable *lpRestrict, std::set<unsigned int> &setExcludeProps, std::list<SIndexedTerm> *lpMultiSearches)
{
    ECRESULT er = erSuccess;
    SIndexedTerm sMultiSearch;
    
    lpMultiSearches->clear();
    
    if (lpRestrict->ulType == RES_AND) {
        for(unsigned int i = 0; i < lpRestrict->lpAnd->__size;) {
            if(NormalizeGetMultiSearch(lpRestrict->lpAnd->__ptr[i], setExcludeProps, sMultiSearch) == erSuccess) {
                lpMultiSearches->push_back(sMultiSearch);

                // Remove it from the restriction since it is now handled as a multisearch
                delete lpRestrict->lpAnd->__ptr[i];
                memmove(&lpRestrict->lpAnd->__ptr[i], &lpRestrict->lpAnd->__ptr[i+1], sizeof(struct restrictTable *) * (lpRestrict->lpAnd->__size - i - 1));
                lpRestrict->lpAnd->__size--;
            } else {
                i++;
            }
        }
    } else {
        // Direct RES_CONTENT
        if(NormalizeGetMultiSearch(lpRestrict, setExcludeProps, sMultiSearch) == erSuccess) {
            lpMultiSearches->push_back(sMultiSearch);
            
            // We now have to remove the entire restriction since the top-level restriction here is
            // now obsolete. Since the above loop will generate an empty AND clause, we will do that here as well.
            
            delete lpRestrict->lpContent;
            
            lpRestrict->ulType = RES_AND;
            lpRestrict->lpAnd = new struct restrictAnd;
            lpRestrict->lpAnd->__size = 0;
        }
    }
    
    return er;
}

/**
 * Process the given restriction, and convert into multi-field searches and a restriction
 *
 * This function attempts create a multi-field search and restriction in such a way that
 * as much as possible multi-field searches are returned without changing the result of the 
 * restriction.
 *
 * This is done by applying the following transformations:
 *
 * - Flatten nested ANDs recursively
 * - Remove always-false terms in ORs in the top level
 * - Derive multi-field searches from top-level AND clause (from OR clauses with substring searches, or direct substring searches)
 */
  
ECRESULT NormalizeGetOptimalMultiFieldSearch(struct restrictTable *lpRestrict, std::set<unsigned int> &setExcludeProps, std::list<SIndexedTerm> *lpMultiSearches )
{
    ECRESULT er = erSuccess;
    
    // Normalize nested ANDs, if any
    er = NormalizeRestrictionNestedAnd(lpRestrict);
    if (er != erSuccess)
        goto exit;
        
    // Convert a series of AND's or a single text search into a new restriction and the multisearch
    // terms
    er = NormalizeRestrictionMultiFieldSearch(lpRestrict, setExcludeProps, lpMultiSearches);
    if (er != erSuccess)
        goto exit;
exit:

    return er;
}

/** 
 * Try to run the restriction using the indexer instead of slow
 * database queries. Will fail if the restriction is unable to run by
 * the indexer.
 * 
 * @param[in] lpConfig config object
 * @param[in] lpLogger log object
 * @param[in] lpCacheManager cachemanager object
 * @param[in] guidServer current server guid
 * @param[in] ulStoreId store id to search in
 * @param[in] lstFolders list of folders to search in
 * @param[in] lpRestrict restriction to search against
 * @param[out] lppNewRestrict restriction that should be applied to lppIndexerResults (part of the restriction that could not be handled by the indexer).
 *							  May be NULL if no restriction is needed (all results in lppIndexerResults match the original restriction)
 * @param[out] lppIndexerResults results found by indexer
 * 
 * @return Zarafa error code
 */
ECRESULT GetIndexerResults(ECDatabase *lpDatabase, ECConfig *lpConfig, ECLogger *lpLogger, ECCacheManager *lpCacheManager, GUID *guidServer, GUID *guidStore, ECListInt &lstFolders, struct restrictTable *lpRestrict, struct restrictTable **lppNewRestrict, std::list<unsigned int> &lstMatches)
{
    ECRESULT er = erSuccess;
	ECSearchClient *lpSearchClient = NULL;
	std::set<unsigned int> setExcludePropTags;
	struct timeval tstart, tend;
	LONGLONG	llelapsedtime;
	struct restrictTable *lpOptimizedRestrict = NULL;
	std::list<SIndexedTerm> lstMultiSearches;

	if (!lpDatabase) {
		er = ZARAFA_E_DATABASE_ERROR;
		goto exit;
	}
	
	lstMatches.clear();

	if(parseBool(lpConfig->GetSetting("index_services_enabled")) == true &&
		strlen(lpConfig->GetSetting("index_services_path")) > 0)
	{
		lpSearchClient = new ECSearchClient(lpConfig->GetSetting("index_services_path"), atoui(lpConfig->GetSetting("index_services_search_timeout")) );
		if (!lpSearchClient) {
			er = ZARAFA_E_NOT_ENOUGH_MEMORY;
			goto exit;
		}

		if(lpCacheManager->GetExcludedIndexProperties(setExcludePropTags) != erSuccess) {
            er = lpSearchClient->GetProperties(setExcludePropTags);
            if (er == ZARAFA_E_NETWORK_ERROR)
                lpLogger->Log(EC_LOGLEVEL_ERROR, "Error while connecting to indexer on %s", lpConfig->GetSetting("index_services_path"));
            else if (er != erSuccess)
                lpLogger->Log(EC_LOGLEVEL_ERROR, "Error while querying indexer on %s, 0x%08x", lpConfig->GetSetting("index_services_path"), er);

            if (er != erSuccess)
                goto exit;
                
            er = lpCacheManager->SetExcludedIndexProperties(setExcludePropTags);
            
            if (er != erSuccess)
                goto exit;
        }  

        er = CopyRestrictTable(NULL, lpRestrict, &lpOptimizedRestrict);
        if (er != erSuccess)
            goto exit;
        
        er = NormalizeGetOptimalMultiFieldSearch(lpOptimizedRestrict, setExcludePropTags, &lstMultiSearches);
        if (er != erSuccess)
            goto exit; // Note this will happen if the restriction cannot be handled by the indexer
            
        if (lstMultiSearches.empty()) {
            // Although the restriction was strictly speaking indexer-compatible, no index queries could
            // be found, so bail out
            er = ZARAFA_E_NOT_FOUND;
            goto exit;
        }

		lpLogger->Log(EC_LOGLEVEL_DEBUG, "Using index, %d index queries", lstMultiSearches.size());
        
		gettimeofday(&tstart, NULL);

        er = lpSearchClient->Query(guidServer, guidStore, lstFolders, lstMultiSearches, lstMatches);
		
		gettimeofday(&tend, NULL);
		llelapsedtime = difftimeval(&tstart,&tend);
		g_lpStatsCollector->Max(SCN_INDEXER_SEARCH_MAX, llelapsedtime);
		g_lpStatsCollector->Avg(SCN_INDEXER_SEARCH_AVG, llelapsedtime);

        if (er != erSuccess) {
			g_lpStatsCollector->Increment(SCN_INDEXER_SEARCH_ERRORS);
            lpLogger->Log(EC_LOGLEVEL_ERROR, "Error while querying indexer on %s, 0x%08x", lpConfig->GetSetting("index_services_path"), er);
		} else
			lpLogger->Log(EC_LOGLEVEL_DEBUG, "Indexed query results found in %.4f ms", llelapsedtime/1000.0);
			if(lpLogger->Log(EC_LOGLEVEL_DEBUG))
    			lpLogger->Log(EC_LOGLEVEL_DEBUG, "%d indexed matches found", lstMatches.size());
	} else {
	    er = ZARAFA_E_NOT_FOUND;
	    goto exit;
    }
    
    *lppNewRestrict = lpOptimizedRestrict;
    lpOptimizedRestrict = NULL;
    
exit:
    if (lpOptimizedRestrict)
        FreeRestrictTable(lpOptimizedRestrict);
        
	if (lpSearchClient)
		delete lpSearchClient;

	if (er != erSuccess)
		g_lpStatsCollector->Increment(SCN_DATABASE_SEARCHES);
	else
		g_lpStatsCollector->Increment(SCN_INDEXED_SEARCHES);
    
    return er;
}
