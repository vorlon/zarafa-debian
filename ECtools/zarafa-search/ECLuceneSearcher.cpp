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

#include <platform.h>

#include <algorithm>

#include <mapi.h>
#include <mapidefs.h>
#include <mapicode.h>

#include "ECIndexDB.h"
#include "charset/convert.h"

#include "ECLuceneSearcher.h"
#include "ECIndexFactory.h"

// Given two sorted lists A and B, modify A so that it is the sorted intersection of A and B
void intersect(std::list<unsigned int> &a, std::list<unsigned int> &b) {
	std::list<unsigned int>::iterator i = a.begin(), j = b.begin();
	
	while(i != a.end() && j != b.end()) {
		if(*i > *j) {
			j++;
		}
		else if(*i < *j) {
			a.erase(i++);
		}
		else {
			i++;
			j++;
		}
	}
	
	// If at end of b, but not end of a, remove rest of items from a
	if(i != a.end()) {
		a.erase(i, a.end());
	}
}

ECLuceneSearcher::ECLuceneSearcher(ECThreadData *lpThreadData, GUID *lpServerGuid, GUID *lpStoreGuid, std::list<unsigned int> &lstFolders, unsigned int ulMaxResults)
{
	m_lpThreadData = lpThreadData;
	m_lstFolders = lstFolders;
	m_ulMaxResults = ulMaxResults;
	m_strServerGuid.assign((char *)lpServerGuid, sizeof(GUID));
	m_strStoreGuid.assign((char *)lpStoreGuid, sizeof(GUID));
	m_lpIndex = NULL;
}

ECLuceneSearcher::~ECLuceneSearcher()
{
	if (m_lpIndex)
		m_lpThreadData->lpIndexFactory->ReleaseIndexDB(m_lpIndex);
}

HRESULT ECLuceneSearcher::Create(ECThreadData *lpThreadData, GUID *lpServerGuid, GUID *lpStoreGuid, std::list<unsigned int> &lstFolders, unsigned int ulMaxResults, ECLuceneSearcher **lppSearcher)
{
	HRESULT hr = hrSuccess;
	ECLuceneSearcher *lpSearcher = NULL;

	try {
		lpSearcher = new ECLuceneSearcher(lpThreadData, lpServerGuid, lpStoreGuid, lstFolders, ulMaxResults);
	}
	catch (...) {
		lpSearcher = NULL;
	}
	if (!lpSearcher) {
		hr = MAPI_E_NOT_ENOUGH_MEMORY;
		goto exit;
	}

	lpSearcher->AddRef();

	hr = lpThreadData->lpIndexFactory->GetIndexDB(lpServerGuid, lpStoreGuid, false, false, &lpSearcher->m_lpIndex);
	if(hr != hrSuccess)
		goto exit;

	if (!lpSearcher->m_lpIndex->Complete()) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	if (lppSearcher)
		*lppSearcher = lpSearcher;

exit:
	if ((hr != hrSuccess) && lpSearcher)
		lpSearcher->Release();

	return hr;
}

HRESULT ECLuceneSearcher::SearchEntries(std::list<unsigned int> *lplistResults)
{
	HRESULT hr = hrSuccess;
	std::list<unsigned int> lstMatches;
	std::list<std::wstring> lstTerms;
	bool bFirst = true;
	
	lplistResults->clear();
	
	// Search is performed by querying each term against the index database
	// and doing an intersection of all terms.
	
	for(std::list<SIndexedTerm>::iterator i = m_lstSearches.begin(); i != m_lstSearches.end(); i++) {
		std::wstring wstrTerm = convert_to<std::wstring>(i->strTerm, rawsize(i->strTerm), "utf-8");
		hr = m_lpIndex->Normalize(wstrTerm, lstTerms);
		if(hr != hrSuccess)
			goto exit;
			
		for(std::list<std::wstring>::iterator j = lstTerms.begin();j != lstTerms.end(); j++) {
			if(bFirst)
				hr = m_lpIndex->QueryTerm(m_lstFolders, i->setFields, *j, *lplistResults);
			else {
				hr = m_lpIndex->QueryTerm(m_lstFolders, i->setFields, *j, lstMatches);
				
				intersect(*lplistResults, lstMatches);
			}
			bFirst = false;
		}
	}

exit:	
	m_lstSearches.clear();

	return hr;
}

HRESULT ECLuceneSearcher::AddTerm(std::set<unsigned int> &setFields, std::string &strTerm)
{
	SIndexedTerm sIndexedTerm;
	
	sIndexedTerm.setFields = setFields;
	sIndexedTerm.strTerm = strTerm;
	
	m_lstSearches.push_back(sIndexedTerm);
	
	return hrSuccess;
}
