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

#ifndef ECLUCENE_H
#define ECLUCENE_H

#include <map>
#include <string>
#include <vector>

#include <pthread.h>

#include "zarafa-indexer.h"

/**
 * Cache entry for Single Instance Attachment data
 *
 * When Single Instance attachment data is encountered it will only be parsed to
 * plain-text once and stored in a cache in case the same attachment is found later
 * again.
 */
struct InstanceCacheEntry_t {
	/**
	 * Constructor
	 *
	 * @param[in] strAttachData
	 */
	InstanceCacheEntry_t(std::wstring &strAttachData)
		: strAttachData(strAttachData), ulTimestamp(time(NULL)) {}

	/**
	 * Parsed attachment data (plain-text)
	 */
	const std::wstring strAttachData;

	/**
	 * Last access time for cache entry
	 */
	time_t ulTimestamp;
};

typedef std::map<std::string, InstanceCacheEntry_t> instance_map_t;

/**
 * Wrapper to the Lucene classes access
 * 
 * This class contains a cache for ECLuceneAccess objects for all previously
 * opened stores. This also contains a cache for all parsed Single Instance attachments,
 * the Single Instance cache is shared between all indexed stores.
 */
class ECLucene {
public:
	/**
	 * Constructor
	 *
	 * @param[in]	lpThreadData
	 */
	ECLucene(ECThreadData *lpThreadData);

	/**
	 * Destructor
	 */
	~ECLucene();

	/**
	 * Search Single Instance Attachment cache for parsed attachment data
	 *
	 * @param[in]	strInstanceId
	 *					Single Instance ID which is requested.
	 * @param[out]	lpstrAttachData
	 *					The parsed to plain-text Single Instance attachment data.
	 * @return HRESULT
	 */
	HRESULT GetAttachmentCache(std::string &strInstanceId, std::wstring *lpstrAttachData);

	/**
	 * Store parsed atachment data for a particular Single Instance Attachment
	 *
	 * @param[in]	strInstanceId
	 *					Single Instance ID which is requested.
	 * @param[out]	strAttachData
	 *					The parsed to plain-text Single Instance attachment data.
	 * @return HRESULT
	 */
	HRESULT UpdateAttachmentCache(std::string &strInstanceId, std::wstring &strAttachData);

private:
	ECThreadData *m_lpThreadData;

	pthread_mutex_t m_hInstanceLock;
	instance_map_t m_mInstanceCache;

	ULONG m_ulCacheTimeout;
};

#endif /* ECLUCENE_H */
