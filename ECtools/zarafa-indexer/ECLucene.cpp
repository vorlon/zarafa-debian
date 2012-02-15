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

#include <platform.h>

#include <algorithm>

#include <mapi.h>
#include <mapix.h>
#include <mapiutil.h>
#include <mapidefs.h>
#include <mapiguid.h>

#include <edkguid.h>

#include <ECGuid.h>
#include <ECTags.h>
#include <IECMultiStoreTable.h>
#include <stringutil.h>
#include <Util.h>

#include "ECFileIndex.h"
#include "ECLucene.h"
#include "ECLuceneIndexer.h"
#include "ECLuceneSearcher.h"
#include "zarafa-indexer.h"

#include "charset/convert.h"

ECLucene::ECLucene(ECThreadData *lpThreadData)
{
	const char *lpszCacheTimeout = NULL;

	m_lpThreadData = lpThreadData;

	pthread_mutex_init(&m_hInstanceLock, NULL);

	lpszCacheTimeout = m_lpThreadData->lpConfig->GetSetting("index_cache_timeout");
	m_ulCacheTimeout = (lpszCacheTimeout ? atoui(lpszCacheTimeout) : 0);
}

ECLucene::~ECLucene()
{
	pthread_mutex_destroy(&m_hInstanceLock);
}

HRESULT ECLucene::GetAttachmentCache(std::string &strInstanceId, std::wstring *lpstrAttachData)
{
	HRESULT hr = hrSuccess;
	instance_map_t::iterator iter;

	/* Caching is disabled */
	if (!m_ulCacheTimeout)
		return MAPI_E_NOT_FOUND;

	pthread_mutex_lock(&m_hInstanceLock);

	iter = m_mInstanceCache.find(strInstanceId);
	if (iter == m_mInstanceCache.end()) {
		hr = MAPI_E_NOT_FOUND;
		goto exit;
	}

	iter->second.ulTimestamp = time(NULL);
	if (lpstrAttachData)
		lpstrAttachData->assign(iter->second.strAttachData);

exit:
	pthread_mutex_unlock(&m_hInstanceLock);

	return hr;
}

HRESULT ECLucene::UpdateAttachmentCache(std::string &strInstanceId, std::wstring &strAttachData)
{
	HRESULT hr = hrSuccess;

	/* Caching is disabled */
	if (!m_ulCacheTimeout)
		goto exit;

	pthread_mutex_lock(&m_hInstanceLock);
	m_mInstanceCache.insert(instance_map_t::value_type(strInstanceId, InstanceCacheEntry_t(strAttachData)));
	pthread_mutex_unlock(&m_hInstanceLock);

exit:
	return hr;
}
