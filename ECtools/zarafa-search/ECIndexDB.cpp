/*
 * Copyright 2005 - 2012  Zarafa B.V.
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
#include "ECAnalyzers.h"
#include "ECLogger.h"

#include "ECConfig.h"
#include "ECIndexerUtil.h"
#include "CommonUtil.h"
#include "stringutil.h"
#include "charset/convert.h"

#include <arpa/inet.h>
#include <unicode/coll.h>
#include <unicode/ustring.h>
#include <kchashdb.h>
#include <kcmap.h>

#include <list>
#include <string>
#include <algorithm>

#include <CLucene/util/Reader.h>

using namespace kyotocabinet;

// Key/Value types for KT_TERMS
typedef struct __attribute__((__packed__)) {
    unsigned int folder;
    unsigned short field;
    unsigned int doc;
    unsigned short version;
    unsigned char len;
} TERMENTRY;

typedef struct {
    unsigned int type; // Must be KT_TERMS
    char prefix[1]; // Actually more than 1 char
} TERMKEY;

/**
 * The format for the term entries
 *
 * The key is
 *
 * 4 bytes  | N bytes
 * KT_TERMS | Prefix
 *
 * And the value is
 *
 * 4 bytes  | 2 bytes | 4 bytes | 2 bytes   | 1 byte | len bytes | ... REPEAT for each document
 * folderid | fieldid | docid   | versionid | len    | postfix
 *
 * 
 *
 * This is the format that is used in the in-memory TinyHashMap. However, when writing to the on-disk
 * btree, the key format is
 *
 * 4 bytes  | N byes | 1 byte | 4 bytes
 * KT_TERMS | Prefix | NUL    | BLOCKID
 *
 * eg. \0\0\0\0aaa\0\1\0\0\0\0 -> contains all documents with words with prefix 'aaa'
 *
 * NOTE: the prefix isn't actually the real prefix, but an ICU sortkey of the prefix.
 *
 * The reason for this is that when we're writing to the on-disk cache, we don't want to append to the previous
 * value since this is inefficient (read / append / write). So we have a BLOCKID which is just a counter that
 * increments by one each time we flush to disk. This makes each write a write to a unique value.
 */

// Key/Value types for KT_VERSION
typedef struct {
    unsigned int type; // Must be KT_VERSION
    unsigned int folder;
    unsigned int doc;
} VERSIONKEY;

typedef struct {
    unsigned short version;
} VERSIONVALUE;

enum KEYTYPES { KT_TERMS, KT_VERSION, KT_SOURCEKEY, KT_BLOCK, KT_STATE, KT_COMPLETE, KT_DBVERSION };

#define INDEX_VERSION_VALUE 1

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

ECIndexDB::ECIndexDB(const std::string &strIndexId, ECConfig *lpConfig, ECLogger *lpLogger)
{
    UErrorCode status = U_ZERO_ERROR;
    
    m_lpConfig = lpConfig;
    m_lpLogger = lpLogger;
    
    m_lpAnalyzer = new ECAnalyzer();
    
    ParseProperties(lpConfig->GetSetting("index_exclude_properties"), m_setExcludeProps);
    
    m_lpIndex = NULL;
    m_lpCache = NULL;
    m_ulCacheSize = 0;
    m_bComplete = false;

    m_lpCollator = Collator::createInstance(status);
	m_lpCollator->setAttribute(UCOL_STRENGTH, UCOL_PRIMARY, status);
}

ECIndexDB::~ECIndexDB()
{
    FlushCache();
    
    if(m_lpCache) {
        delete m_lpCache;
    }
    if(m_lpIndex) {
        m_lpIndex->close();
        delete m_lpIndex;
    }
    if(m_lpAnalyzer)
        delete m_lpAnalyzer;
        
    if(m_lpCollator)
        delete m_lpCollator;
}

HRESULT ECIndexDB::Create(const std::string &strIndexId, ECConfig *lpConfig, ECLogger *lpLogger, bool bCreate, bool bComplete, ECIndexDB **lppIndexDB)
{
    HRESULT hr = hrSuccess;
    
    ECIndexDB *lpIndex = new ECIndexDB(strIndexId, lpConfig, lpLogger);
    
    hr = lpIndex->Open(strIndexId, bCreate, bComplete);
    if(hr != hrSuccess)
        goto exit;

    *lppIndexDB = lpIndex;
        
exit:
    if (hr != hrSuccess && lpIndex)
        delete lpIndex;
        
    return hr;
}

/**
 * Open or create the underlying TreeDB object.
 * @param[in]   strIndexId      The id of the index to open.
 * @param[in]   bCreate         If this argument is true and the 
 *                              requested TreeDB does not yet exist it
 *                              is created.
 * @param[in]   bComplete       Ignored if bCreate is false or if the
 *                              requested TreeDB already exists.
 *                              If the requested TreeDB is created, its
 *                              status is immediately set to Complete if
 *                              this argument is true.
 * 
 * The bComplete is set to false during the building state because the
 * index will only be complete once the initial scan is completed. So
 * when the initial scan is done the index will be marked as complete.
 * 
 * During the running state a new store will be detected by it's first
 * change. In this case a new index needs to be created which is
 * complete to begin with because it's empty, just like a new store.
 */
HRESULT ECIndexDB::Open(const std::string &strIndexId, bool bCreate, bool bComplete)
{
    HRESULT hr = hrSuccess;
    std::string strPath = std::string(m_lpConfig->GetSetting("index_path")) + PATH_SEPARATOR + strIndexId + ".kct";

    m_lpIndex = new TreeDB();
    m_lpCache = new TinyHashMap(1048583); // Default taken from kcdbext.h. This can be reached if each word has a key (instead of prefix)

    // Enable compression on the tree database
    m_lpIndex->tune_options(TreeDB::TCOMPRESS);

    if(bCreate) {
        if(CreatePath(m_lpConfig->GetSetting("index_path")) < 0) {
            m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Path '%s' not found or unable to create: %s", m_lpConfig->GetSetting("index_path"), strerror(errno));
        }
    }
    
    if (!m_lpIndex->open(strPath, TreeDB::OWRITER | TreeDB::OREADER)) {
        if (!bCreate) {
            hr = MAPI_E_NOT_FOUND;
            goto exit;
        }

        if (!m_lpIndex->open(strPath, TreeDB::OWRITER | TreeDB::OREADER | (bCreate ? TreeDB::OCREATE : 0))) {
            m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to open index %s: %s", strPath.c_str(), m_lpIndex->error().message());
            hr = MAPI_E_NOT_FOUND;
            goto exit;
        }
		hr = WriteToDB(KT_DBVERSION, INDEX_VERSION_VALUE);
		if (hr != hrSuccess) {
			m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to open index %s: %s", strPath.c_str(), m_lpIndex->error().message());
			goto exit;
		}

        if (bComplete)
            SetComplete();  // set m_bComplete
    } else {
		unsigned int version = QueryFromDB(KT_DBVERSION);
		if (version != INDEX_VERSION_VALUE) {
			m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Old index file %s version %d is unusable", strPath.c_str(), version);
			m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Please remove all files in %s and restart zarafa-search", m_lpConfig->GetSetting("index_path"));
			hr = MAPI_E_DISK_ERROR;
			goto exit;
		}
        m_bComplete = (QueryFromDB(KT_COMPLETE) == 1);
	}
    
exit:    
    return hr;
}

HRESULT ECIndexDB::AddTerm(folderid_t folder, docid_t doc, fieldid_t field, unsigned int version, std::wstring wstrTerm)
{
    HRESULT hr = hrSuccess;
    std::string strQuery;
    std::string strValues;
    lucene::analysis::TokenStream* stream = NULL;
    lucene::analysis::Token* token = NULL;
    unsigned int type = KT_TERMS;

    char buf[sizeof(TERMENTRY) + 256];
    char keybuf[sizeof(unsigned int) + 256 + 1];
    TERMENTRY *sTerm = (TERMENTRY *)buf;

    // Preset all the key/value parts that will not change
    sTerm->folder = folder;
    sTerm->doc = doc;
    sTerm->field = field;
    sTerm->version = version;
    
    memcpy(keybuf, (char *)&type, sizeof(type));

    // Check if the property is excluded from indexing
    if (m_setExcludeProps.find(field) == m_setExcludeProps.end())
    {   
        const wchar_t *text;
        unsigned int len;
        unsigned int keylen;
        
        lucene::util::StringReader reader(wstrTerm.c_str());
        
        stream = m_lpAnalyzer->tokenStream(L"", &reader);
        
        while((token = stream->next())) {
            text = token->termText();
            len = MIN(wcslen(text), 255);
            
            if(len == 0)
                goto next;
            
            // Generate sortkey and put it directly into our key
            keylen = GetSortKey(text, MIN(len, 3), keybuf + sizeof(unsigned int), 256);
            
            if(keylen > 256) {
                ASSERT(false);
                keylen = 256;
            }
            
            if(len <= 3) {
                // No more term info in the value
                sTerm->len = 0;
            } else {
                int klen = GetSortKey(text + 3, len - 3, buf + sizeof(TERMENTRY), 256);
                if(klen > 255) {
                    klen = 255;
                }
                sTerm->len = klen;
            }
                
            m_lpCache->append(keybuf, sizeof(unsigned int) + keylen, buf, sizeof(TERMENTRY) + sTerm->len);
            m_ulCacheSize += sizeof(unsigned int) + keylen + sizeof(TERMENTRY) + sTerm->len;

next:        
            delete token;
        }
    }

    if(m_ulCacheSize > atoui(m_lpConfig->GetSetting("term_cache_size"))) {
        hr = FlushCache();
        if(hr != hrSuccess)
            goto exit;
    }
    
exit:    
    if(stream)
        delete stream;
    
    return hr;
}

HRESULT ECIndexDB::RemoveTermsFolder(folderid_t folder)
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
 * @param[in] doc Document ID to remove
 * @param[out] lpulVersion Version of new terms to be added
 * @return Result
 */
HRESULT ECIndexDB::RemoveTermsDoc(folderid_t folder, docid_t doc, unsigned int *lpulVersion)
{
    HRESULT hr = hrSuccess;
    VERSIONKEY sKey;
    VERSIONVALUE sValue;
    char *value = NULL;
    size_t cb = 0;
    
    sKey.type = KT_VERSION;
	sKey.folder = folder;
    sKey.doc = doc;
    
    value = m_lpIndex->get((char *)&sKey, sizeof(sKey), &cb);
    
    if(!value || cb != sizeof(VERSIONVALUE)) {
        sValue.version = 1;
    } else {
        sValue = *(VERSIONVALUE *)value;
        sValue.version++;
    }
    
    if(!m_lpIndex->set((char *)&sKey, sizeof(sKey), (char *)&sValue, sizeof(sValue))) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to set version for document: %s", m_lpIndex->error().message());
        hr = MAPI_E_NOT_FOUND;
        goto exit;
    }
    
    if(lpulVersion)
        *lpulVersion = sValue.version;
     
exit:   
    if (value)
        delete [] value;
        
    return hr;
}

/**
 * Normalize a string into lower-case form
 *
 * This normalizes a string into possibly multiple query strings, in such a way that they
 * can be passed to QueryTerm()
 * @param[in] strInput String to normalize
 * @param[out] lstOutput List of normalized strings
 */
HRESULT ECIndexDB::Normalize(const std::wstring &strInput, std::list<std::wstring> &lstOutput)
{
    HRESULT hr = hrSuccess;
    lucene::analysis::TokenStream* stream = NULL;
    lucene::analysis::Token* token = NULL;
    
    lucene::util::StringReader reader(strInput.c_str());
    
    stream = m_lpAnalyzer->tokenStream(L"", &reader);
    
    lstOutput.clear();
    while((token = stream->next())) {
        lstOutput.push_back(token->termText());
        delete token;
    }

	delete stream;
    
    return hr;
}

// FIXME lstFolders should be setFolders in the first place
HRESULT ECIndexDB::QueryTerm(std::list<unsigned int> &lstFolders, std::set<unsigned int> &setFields, std::wstring &wstrTerm, std::list<docid_t> &lstMatches)
{
    HRESULT hr = hrSuccess;
    std::set<unsigned int> setFolders(lstFolders.begin(), lstFolders.end());
    std::set<unsigned int> setMatches;
    std::set<unsigned int>::iterator j;
    std::list<unsigned int>::iterator i;
    size_t len = 0;
    char *thiskey = NULL;
    size_t thislen = 0;
    char *value = NULL;
    unsigned int offset = 0;
    unsigned int type = KT_TERMS;
    TERMENTRY *p = NULL;
    char keybuf[sizeof(unsigned int) + 256];
    unsigned int keylen = 0;
    char valbuf[256];
    unsigned int vallen = 0;
    kyotocabinet::DB::Cursor *cursor = NULL;
    
    lstMatches.clear();
    
    // Make the key that we will search    
    memcpy(keybuf, (char *)&type, sizeof(type));
    keylen = GetSortKey(wstrTerm.c_str(), MIN(wstrTerm.size(), 3), keybuf + sizeof(unsigned int), 256);
    keylen = MIN(256, keylen);
    
    if(wstrTerm.size() > 3) {
        vallen = GetSortKey(wstrTerm.c_str() + 3, wstrTerm.size() - 3, valbuf, 256);
        vallen = MIN(256, vallen);
    } else {
        vallen = 0;
    }
    
    cursor = m_lpIndex->cursor();
    
    if(!cursor->jump(keybuf, sizeof(unsigned int) + keylen))
        goto exit; // No matches;
        
    while(1) {
        thiskey = cursor->get_key(&thislen, false);
        
        if(!thiskey)
            break;

        if(thislen < 4)
            break; // Invalid key in the database
            
        if(*(unsigned int*)thiskey != KT_TERMS)
            break; // Key is not a term key
        
        if(thislen - (4+1) < sizeof(unsigned int) + keylen)
            break; // Found key is shorter than the prefix we're looking for, so there are no more matches after this (since shorter keys must be after the key we're looking for)
            
        if(memcmp(thiskey, keybuf, sizeof(unsigned int) + keylen) > 0)
            break; // Found key is beyond the prefix we're looking for
            
        value = cursor->get_value(&len, true);
    
        offset = 0;

        if(!value)
            goto nextkey; // No matches
        
        while(offset < len) {
            p = (TERMENTRY *) (value + offset);
            
            if (setFolders.count(p->folder) == 0)
                goto nextterm;
            if (setFields.count(p->field) == 0)
                goto nextterm;
            if (p->len < vallen)
                goto nextterm;
                
            if (vallen == 0 || strncmp(valbuf, value + offset + sizeof(TERMENTRY), vallen) == 0) {
                VERSIONKEY sKey;
                VERSIONVALUE *sVersion;
                size_t cb = 0;
                sKey.type = KT_VERSION;
				sKey.folder = p->folder;
                sKey.doc = p->doc;
                
                // Check if the version is ok
                sVersion = (VERSIONVALUE *)m_lpIndex->get((char *)&sKey, sizeof(sKey), &cb);
                
                if(!sVersion || cb != sizeof(VERSIONVALUE) || sVersion->version == p->version)
                    setMatches.insert(p->doc);
                    
                delete [] sVersion;
            }
nextterm:
            offset += sizeof(TERMENTRY) + p->len;            
        }
nextkey:
        delete [] thiskey;
        thiskey = NULL;
        delete [] value;
        value = NULL;
    }
    
    std::copy(setMatches.begin(), setMatches.end(), std::back_inserter(lstMatches));
    
exit:
    if (cursor)
        delete cursor;
    if (thiskey)
        delete [] thiskey;
    if (value)
        delete [] value;
        
    return hr;
}

/**
 * Create a sortkey from the wchar_t input passed
 *
 * @param wszInput Input string
 * @param len Length of wszInput in characters
 * @param szOutput Output buffer
 * @param outLen Output buffer size in bytes
 * @return Length of data written to szOutput
 */
size_t ECIndexDB::GetSortKey(const wchar_t *wszInput, size_t len, char *szOutput, size_t outLen)
{
    UChar in[1024];
    int32_t inlen;
    int32_t keylen;
    UErrorCode error = U_ZERO_ERROR;
    
    u_strFromWCS(in, arraySize(in), &inlen, wszInput, len, &error);
    
    keylen = m_lpCollator->getSortKey(in, inlen, (uint8_t *)szOutput, (int32_t)outLen);
    
    return keylen - 1;
}

/**
 * Flush cache from in-memory cache by merging the contents into the
 * tree
 */
HRESULT ECIndexDB::FlushCache()
{
    HRESULT hr = hrSuccess;
    char k[1024];
    const char* kbuf, *vbuf;
    size_t ksiz, vsiz;
    unsigned int ulBlock = 0;
    unsigned int key = KT_BLOCK;
    size_t cb;
    double dblStart = GetTimeOfDay();
    TinyHashMap::Sorter sorter(m_lpCache);

    if(m_lpCache->count() == 0)
        goto exit; // Nothing to flush

    m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Flushing the cache");
    
    if(!m_lpIndex->begin_transaction()) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to start transaction: %s", m_lpIndex->error().message());
        hr = MAPI_E_DISK_ERROR;
        goto exit;
    }
    
    vbuf = m_lpIndex->get((char *)&key, sizeof(key), &cb);
    if(cb != sizeof(ulBlock) || !vbuf) {
        ulBlock = 0;
    } else {
        ulBlock = *(unsigned int *)vbuf;
    }
    
    if(vbuf)
        delete [] vbuf;
    
    ulBlock++;
    
    m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Flushing %u values to block %d", (unsigned)m_lpCache->count(), ulBlock);
    
    if(!m_lpIndex->set((char *)&key, sizeof(key), (char *)&ulBlock, sizeof(ulBlock))) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to write block id: %s", m_lpIndex->error().message());
        hr = MAPI_E_DISK_ERROR;
        goto exit;
    }
    
    while ((kbuf = sorter.get(&ksiz, &vbuf, &vsiz)) != NULL) {
        ksiz = MIN(ksiz, sizeof(k)-sizeof(int)-1);
        memcpy(k, kbuf, ksiz);
        // Value in the DB is <sortkey>\0<blockid>
        *(k+ksiz)=0;
        *(unsigned int *)(k+ksiz+1) = htonl(ulBlock);
        ASSERT(m_lpIndex->get(k, ksiz+sizeof(ulBlock)+1, &cb) == NULL);
        if (!m_lpIndex->set(k, ksiz+sizeof(ulBlock)+1, vbuf, vsiz)) {
            m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to flush index data: %s", m_lpIndex->error().message());
            hr = MAPI_E_DISK_ERROR;
            goto exit;
        }
        sorter.step();
    }

    if(!m_lpIndex->end_transaction()) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to start transaction: %s", m_lpIndex->error().message());
        hr = MAPI_E_DISK_ERROR;
        goto exit;
    }
    
    m_ulCacheSize = 0;
    m_lpCache->clear();

    m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Flushing the cache took %.2f seconds", GetTimeOfDay() - dblStart);
    
exit:
    return hr;        
}

/**
 * Set the sync state for a folder. This can be read back from GetSyncState()
 * 
 * @param[in] strFolder Folder ID
 * @param[in] strState State blob
 * @return result
 */
HRESULT ECIndexDB::SetSyncState(const std::string& strFolder, const std::string& strState)
{
    HRESULT hr = hrSuccess;
    unsigned int ulBlock = KT_STATE;
    
    std::string strKey((const char *)&ulBlock, sizeof(ulBlock));
    strKey += strFolder;
    
    if(!m_lpIndex->set(strKey, strState)) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to set state for folder");
        hr = MAPI_E_DISK_ERROR;
        goto exit;
    }
    
exit:
    return hr;
}

/**
 * Get the sync state for a folder.
 * 
 * @param[in] strFolder Folder ID
 * @param[in] strState State blob
 * @return result
 */
HRESULT ECIndexDB::GetSyncState(const std::string& strFolder, std::string& strState)
{
    HRESULT hr = hrSuccess;
    unsigned int ulBlock = KT_STATE;
    
    std::string strKey((const char *)&ulBlock, sizeof(ulBlock));
    strKey += strFolder;
    
    if(!m_lpIndex->get(strKey, &strState)) {
        hr = MAPI_E_NOT_FOUND;
        goto exit;
    }
    
exit:
    return hr;
}

/**
 * Return true if the index is marked as complete. This indicates that
 * the initial indexing is performed and the index is now incrementally
 * updated.
 */
bool ECIndexDB::Complete()
{
    return m_bComplete;
}

/** 
 * Write a simple integer value under an interger key in the database
 */
HRESULT ECIndexDB::WriteToDB(unsigned int key, unsigned int value)
{
	HRESULT hr = hrSuccess;
	if (!m_lpIndex->set((char*)&key, sizeof(key),
						(char*)&value, sizeof(value))) {
		hr = MAPI_E_DISK_ERROR;
	}
	return hr;
}

/** 
 * Read a simple integer value under an interger key from the database
 */
unsigned int ECIndexDB::QueryFromDB(unsigned int key)
{
    size_t len = 0;
    unsigned char *lpValue = NULL;
	unsigned int rValue = 0;

    lpValue = (unsigned char*)m_lpIndex->get((char*)&key, sizeof(key), &len);
	if (lpValue && len == sizeof(unsigned int))
		rValue = *(unsigned int*)lpValue;
    
    delete[] lpValue;
    return rValue;
}

/**
 * Mark the index as complete. This indicates that the initial indexing
 * is performed and the index is now incrementally updated.
 */
HRESULT ECIndexDB::SetComplete()
{
    HRESULT hr = hrSuccess;
    
    if (!m_bComplete) {
		hr = WriteToDB(KT_COMPLETE, 1);
		if (hr != hrSuccess)
            goto exit;
        
        m_bComplete = true;
    }
    
exit:
    return hr;
}
