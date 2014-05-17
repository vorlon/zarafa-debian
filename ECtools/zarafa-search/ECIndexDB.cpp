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

#include "platform.h"
#include "ECIndexDB.h"
#include "ECAnalyzers.h"
#include "ECLogger.h"

#include "ECConfig.h"
#include "ECIndexerUtil.h"
#include "CommonUtil.h"
#include "stringutil.h"
#include "charset/convert.h"
#include "charset/utf8string.h"

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

enum KEYTYPES {
	KT_TERMS,		// N [KT_TERMS, (lucene sort key of) 3 "letter" term + \0 + block id (global)] <term0,
	KT_VERSION,		// N [KT_VESION, struct VERSIONKEY] <version>
	KT_SOURCEKEY,	// unused
	KT_BLOCK,		// 1 [KT_BLOCK] <block id>
	KT_STATE,		// 1 [KT_STATE] <syncstate>
	KT_COMPLETE,	// 1 [KT_COMPLETE] <bool>, false if the store index was aborted during the first index step
	KT_DBVERSION,	// 1 [KT_DBVERSION] <version> current vesion of the database
	KT_STUBTARGETS,	// 1 [KT_STUBTARGETS] <archiver info>
	KT_OPTIMIZETS	// 1 [KT_OPTIMIZETS] <last optimized timestamp>
};

#define INDEX_VERSION_VALUE 1

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

ECIndexDB::ECIndexDB(const std::string &strIndexId, ECConfig *lpConfig, ECLogger *lpLogger)
{
    UErrorCode status = U_ZERO_ERROR;

	m_strIndexId = strIndexId;
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

    pthread_mutex_init(&m_writeMutex, NULL);
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

    pthread_mutex_destroy(&m_writeMutex);
}

HRESULT ECIndexDB::Create(const std::string &strIndexId, ECConfig *lpConfig, ECLogger *lpLogger, bool bCreate, bool bComplete, ECIndexDB **lppIndexDB)
{
    HRESULT hr = hrSuccess;
    
    ECIndexDB *lpIndex = new ECIndexDB(strIndexId, lpConfig, lpLogger);
    
    hr = lpIndex->Open(bCreate, bComplete);
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
HRESULT ECIndexDB::Open(bool bCreate, bool bComplete)
{
    HRESULT hr = hrSuccess;
    std::string strPath = std::string(m_lpConfig->GetSetting("index_path")) + PATH_SEPARATOR + m_strIndexId + ".kct";
	int rv = 0;

    m_lpIndex = new TreeDB();
    m_lpCache = new TinyHashMap(1048583); // Default taken from kcdbext.h. This can be reached if each word has a key (instead of prefix)

    // Enable compression on the tree database
    m_lpIndex->tune_options(TreeDB::TCOMPRESS);

    if(bCreate) {
        if(CreatePath(m_lpConfig->GetSetting("index_path")) < 0) {
            m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Path '%s' not found or unable to create: %s", m_lpConfig->GetSetting("index_path"), strerror(errno));
        }
    }

	try {
		rv = m_lpIndex->open(strPath, TreeDB::OWRITER | TreeDB::OREADER);
	}
	catch (std::exception &e) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to open index %s: %s", strPath.c_str(), m_lpIndex->error().message());
		hr = MAPI_E_DISK_ERROR;
		goto exit;
	}
    if (!rv) {
        if (!bCreate) {
            hr = MAPI_E_NOT_FOUND;
            goto exit;
        }
		// no need to try/catch, since there is no data to read yet
        if (!m_lpIndex->open(strPath, TreeDB::OWRITER | TreeDB::OREADER | TreeDB::OCREATE)) {
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

    pthread_mutex_lock(&m_writeMutex);

    m_lpLogger->Log(EC_LOGLEVEL_DEBUG|EC_LOGLEVEL_SEARCH, "add term 0x%08X: '%s'", field, convert_to<utf8string>(wstrTerm).c_str());

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
                
            m_lpLogger->Log(EC_LOGLEVEL_DEBUG|EC_LOGLEVEL_SEARCH, "token: '%s' type %s, key: '%s'", convert_to<utf8string>(text).c_str(), convert_to<utf8string>(token->type()).c_str(), keybuf+sizeof(unsigned int));
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

    pthread_mutex_unlock(&m_writeMutex);
    
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
    
    if(!value) {
		if (m_lpIndex->error() == kyotocabinet::BasicDB::Error::NOREC)
			sValue.version = 1;
		else {
			m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to get version from index %s: %s", m_lpIndex->path().c_str(), m_lpIndex->error().message());
			hr = MAPI_E_DISK_ERROR;
			goto exit;
		}
    } else {
		if (cb != sizeof(VERSIONVALUE))
			sValue.version = 1;
		else {
			sValue = *(VERSIONVALUE *)value;
			sValue.version++;
		}
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
				if(!sVersion && m_lpIndex->error() != kyotocabinet::BasicDB::Error::NOREC) {
					m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to get term version from index %s: %s", m_lpIndex->path().c_str(), m_lpIndex->error().message());
					hr = MAPI_E_DISK_ERROR;
					goto exit;
				} else if(!sVersion || cb != sizeof(VERSIONVALUE) || sVersion->version == p->version)
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
HRESULT ECIndexDB::FlushCache(kyotocabinet::TinyHashMap *lpCache, bool bTransaction)
{
    HRESULT hr = hrSuccess;
    char k[1024];
    const char* kbuf, *vbuf;
    size_t ksize, vsize;
    unsigned int ulBlock = 0;
    unsigned int key = KT_BLOCK;
    size_t cb;
    double dblStart = GetTimeOfDay();

	if (lpCache == NULL)
		lpCache = m_lpCache;

    TinyHashMap::Sorter sorter(lpCache);

    if(lpCache->count() == 0)
        goto exit; // Nothing to flush

    m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Flushing the cache");
    
    if(bTransaction && !m_lpIndex->begin_transaction()) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to start transaction: %s", m_lpIndex->error().message());
        hr = MAPI_E_DISK_ERROR;
        goto exit;
    }

	// get current block id
    vbuf = m_lpIndex->get((char *)&key, sizeof(key), &cb);
    if(!vbuf) {
		if (m_lpIndex->error() == kyotocabinet::BasicDB::Error::NOREC)
			ulBlock = 0;
		else if (m_lpIndex->error()) {
			m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to fetch block id: %s", m_lpIndex->error().message());
			hr = MAPI_E_DISK_ERROR;
			goto exit;
		}
    } else {
        ulBlock = *(unsigned int *)vbuf;
    }
    
    if(vbuf)
        delete [] vbuf;
    
    ulBlock++;
    
    m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Flushing %u values to block %d", (unsigned)lpCache->count(), ulBlock);

	// set new current block id
    if(!m_lpIndex->set((char *)&key, sizeof(key), (char *)&ulBlock, sizeof(ulBlock))) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to write block id: %s", m_lpIndex->error().message());
        hr = MAPI_E_DISK_ERROR;
        goto exit;
    }
    
    while ((kbuf = sorter.get(&ksize, &vbuf, &vsize)) != NULL) {
        ksize = MIN(ksize, sizeof(k)-sizeof(int)-1);
        memcpy(k, kbuf, ksize);
        // Value in the DB is <sortkey>\0<blockid>
        *(k+ksize)=0;
        *(unsigned int *)(k+ksize+1) = htonl(ulBlock);
        ASSERT(m_lpIndex->get(k, ksize+sizeof(ulBlock)+1, &cb) == NULL);
        if (!m_lpIndex->set(k, ksize+sizeof(ulBlock)+1, vbuf, vsize)) {
            m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to flush index data: %s", m_lpIndex->error().message());
            hr = MAPI_E_DISK_ERROR;
            goto exit;
        }
        sorter.step();
    }

    if(bTransaction && !m_lpIndex->end_transaction()) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to start transaction: %s", m_lpIndex->error().message());
        hr = MAPI_E_DISK_ERROR;
        goto exit;
    }
    
    m_ulCacheSize = 0;
    lpCache->clear();

    m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Flushing the cache took %.2f seconds", GetTimeOfDay() - dblStart);
    
exit:
    return hr;        
}

/**
 * Set the sync state for a folder. This can be read back from GetSyncState()
 * 
 * The stub targets will be saved first because if that fails, the sync
 * state won't be saved and the folder will be resynchronized from the
 * previous sync state.
 * If saving of the stub targets passes but saving of the sync state fails,
 * we end up with the new stub targets and the previous sync state, which
 * means that some stubs will be processed multiple times, which is fine.
 * 
 * The stub targets will contain the previous targets plus the new targets
 * minus the processed targets. So no targets will be lost.

 * @param[in] strFolder Folder ID
 * @param[in] strState State blob
 * @param[in] strStubTargets Stub targets blob
 * @return result
 */
HRESULT ECIndexDB::SetSyncState(const std::string& strFolder, const std::string& strState, const std::string& strStubTargets)
{
    HRESULT hr = hrSuccess;
    unsigned int ulBlock;
    std::string strKey;
    
    ulBlock = KT_STUBTARGETS;
    strKey.assign((const char *)&ulBlock, sizeof(ulBlock));
    strKey += strFolder;
    
    if(strStubTargets.empty()) {
        m_lpIndex->remove(strKey);  // No error check.
    } else {
        if(!m_lpIndex->set(strKey, strStubTargets)) {
            m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to set stub targets for folder: %s", m_lpIndex->error().message());
            hr = MAPI_E_DISK_ERROR;
            goto exit;
        }
    }
    
    ulBlock = KT_STATE;
    strKey.assign((const char *)&ulBlock, sizeof(ulBlock));
    strKey += strFolder;
    
    if(!m_lpIndex->set(strKey, strState)) {
        m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to set state for folder: %s", m_lpIndex->error().message());
        hr = MAPI_E_DISK_ERROR;
        goto exit;
    }
    
exit:
    return hr;
}

/**
 * Get the sync state for a folder.
 * 
 * @param[out] strFolder Folder ID
 * @param[out] strState State blob
 * @param[out] strStubTargets Stub targets blob
 * @return result
 */
HRESULT ECIndexDB::GetSyncState(const std::string& strFolder, std::string& strState, std::string& strStubTargets)
{
    HRESULT hr = hrSuccess;
    unsigned int ulBlock;
    std::string strKey;

    ulBlock = KT_STATE;
    strKey.assign((const char *)&ulBlock, sizeof(ulBlock));
    strKey += strFolder;
    
    if(!m_lpIndex->get(strKey, &strState)) {
		if (m_lpIndex->error() == kyotocabinet::BasicDB::Error::NOREC)
			hr = MAPI_E_NOT_FOUND;
		else {
			m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to read sync state for folder: %s", m_lpIndex->error().message());
			hr = MAPI_E_DISK_ERROR;
		}
        goto exit;
    }
    
    ulBlock = KT_STUBTARGETS;
    strKey.assign((const char *)&ulBlock, sizeof(ulBlock));
    strKey += strFolder;
    
    if(!m_lpIndex->get(strKey, &strStubTargets)) {
        // If the key is absent, there were no targets, not an error
		if (m_lpIndex->error() == kyotocabinet::BasicDB::Error::NOREC)
			strStubTargets.clear();
		else if (m_lpIndex->error()) {
			m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to read sync state for stubtargets: %s", m_lpIndex->error().message());
			hr = MAPI_E_DISK_ERROR;
			goto exit;
		}
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
 * Write a simple integer value under an integer key in the database
 */
HRESULT ECIndexDB::WriteToDB(unsigned int key, unsigned int value)
{
	HRESULT hr = hrSuccess;
	if (!m_lpIndex->set((char*)&key, sizeof(key),
						(char*)&value, sizeof(value))) {
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to write key %u value %u to index: %s", key, value, m_lpIndex->error().message());
		hr = MAPI_E_DISK_ERROR;
	}
	return hr;
}

/** 
 * Read a simple integer value under an integer key from the database
 */
unsigned int ECIndexDB::QueryFromDB(unsigned int key)
{
    size_t len = 0;
    unsigned char *lpValue = NULL;
	unsigned int rValue = 0;

    lpValue = (unsigned char*)m_lpIndex->get((char*)&key, sizeof(key), &len);
	if (lpValue && len == sizeof(unsigned int))
		rValue = *(unsigned int*)lpValue;
	else if (!lpValue && m_lpIndex->error() != kyotocabinet::BasicDB::Error::NOREC)
		m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to read key %u from index: %s", key, m_lpIndex->error().message());
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

/** 
 * Walks through all KT_TERMS keys in the database, and removes all
 * old versions of a document, and combines keys if possible.
 * 
 * @return MAPI Error code
 */
HRESULT ECIndexDB::Optimize()
{
	HRESULT hr = hrSuccess;
	kyotocabinet::DB::Cursor *cursor = NULL;
	char *firstkey = NULL;
	size_t firstlen = 0;
	char *currkey = NULL;
	size_t currlen = 0;
	char *value = NULL;
	size_t len = 0;
	size_t offset = 0;
	size_t steps = 0;
	size_t removes = 0;
	kyotocabinet::TinyHashMap *lpCache = new kyotocabinet::TinyHashMap();
	time_t tsLast = 0, tsEnd = 0;
	int iLast = 0;
	const char *szAge = NULL;
	std::string strValues;
	unsigned int ulLastBlock = 0;

    pthread_mutex_lock(&m_writeMutex);

	if (!m_bComplete)
		goto exit;

	iLast = QueryFromDB(KT_OPTIMIZETS);
	tsLast = time(NULL);
	szAge = m_lpConfig->GetSetting("optimize_age");
	if (iLast + (::atoi(szAge)*24*60*60) > tsLast) {
		m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Ignoring optimized index %s", m_lpIndex->path().c_str());
		goto exit;
	}

	m_lpLogger->Log(EC_LOGLEVEL_NOTICE, "Optimizing index %s", m_lpIndex->path().c_str());

	hr = FlushCache();
	if (hr != hrSuccess)
		goto exit;

	cursor = m_lpIndex->cursor();

	if (!cursor->jump())
		goto exit;

	while (true) {

		currkey = cursor->get_key(&currlen, false);
		if (!currkey)
			break;
		if (currlen < 4) {
			cursor->step();
			goto cleanup;
		}
		if(*(unsigned int*)currkey != KT_TERMS) {
			cursor->step();
			goto cleanup;
		}

		if (firstkey == NULL) {
			// mark start of block
			firstkey = currkey;
			firstlen = currlen;
		} else if (currlen != firstlen || memcmp(firstkey, currkey, currlen - sizeof(unsigned int)) != 0) {
			// new block, flush previous processed blocks
			goto cleanup;
		}
		ulLastBlock = ntohl(*(currkey + currlen - sizeof(unsigned int)));

		// KT_TERMS <sortkeydata N bytes> blockid

		value = cursor->get_value(&len);
		if (!value) {
			cursor->step();
			goto cleanup;
		}

		offset = 0;

		strValues.reserve(len);
		// find all current values in the block, and save them in the cache
		while(value && offset < len) {
			TERMENTRY *entry = (TERMENTRY *) (value + offset);
			VERSIONKEY sKey;
			VERSIONVALUE *sVersion;
			size_t cb = 0;

			sKey.type = KT_VERSION;
			sKey.folder = entry->folder;
			sKey.doc = entry->doc;

			sVersion = (VERSIONVALUE *)m_lpIndex->get((char *)&sKey, sizeof(sKey), &cb);

			if(!sVersion && m_lpIndex->error() != kyotocabinet::BasicDB::Error::NOREC) {
				m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to get term version from index %s: %s", m_lpIndex->path().c_str(), m_lpIndex->error().message());
				hr = MAPI_E_DISK_ERROR;
				delete [] value;
				goto exit;
			} else if(!sVersion || (cb == sizeof(VERSIONVALUE) && sVersion->version == entry->version)) {
				strValues.append((char *)entry, sizeof(TERMENTRY) + entry->len);
			} else {
				// skip this entry so it's removed
				removes++;
			}

			delete [] sVersion;

			offset += sizeof(TERMENTRY) + entry->len;
		}
		// keep entry, strip block id from key. another -1 because of terminating 0 added in sorter loop in FlushCache.
		lpCache->append(currkey, currlen - sizeof(unsigned int) -1, strValues.data(), strValues.length());
		strValues.clear();

		delete [] value;
		if (firstkey != currkey) {
			delete [] currkey;
			currkey = NULL;
			currlen = 0;
		}

		// find next key
		if (cursor->step()) {
			steps++;
			continue;
		} else {
			// incomplete index, since a KT_TERMS entry can't be last
			if (m_lpIndex->error() != kyotocabinet::BasicDB::Error::NOREC)
				m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Index is faulty: %s", m_lpIndex->error().message());
			else
				m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Index is incomplete");
			
			hr = MAPI_E_DISK_ERROR;
			goto exit;
		}

	cleanup:
		if (firstkey && (removes || steps > 1)) {
			if(!m_lpIndex->begin_transaction()) {
				m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to start transaction: %s", m_lpIndex->error().message());
				hr = MAPI_E_DISK_ERROR;
				goto exit;
			}

			m_lpLogger->Log(EC_LOGLEVEL_DEBUG, "Merging %lu blocks and removing %lu out of date entries", steps, removes);

			// remove old data from file
			if (!cursor->jump(firstkey, firstlen)) {
				m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to jump back to first key to combine in index %s: %s", m_lpIndex->path().c_str(), m_lpIndex->error().message());
				hr = MAPI_E_DISK_ERROR;
				goto exit;
			}
			// since the indexer can place new data in the index while we're optimizing, 
			// remove only blocks we've actually seen by testing the block id
			{
				char *cmpkey = NULL;
				size_t cmplen = 0;
				do {
					cmpkey = cursor->get_key(&cmplen);
					if (!cmpkey || cmplen < 4 || (*(unsigned int*)cmpkey != KT_TERMS))
						break;
					if (cmplen != firstlen || memcmp(firstkey, cmpkey, cmplen - sizeof(unsigned int)) != 0)
						break;
					if (ntohl(*(cmpkey + cmplen - sizeof(unsigned int))) <= ulLastBlock)
						cursor->remove();
					else
						break;
					delete [] cmpkey;
				} while (true);
				delete [] cmpkey;
			}

			// write new data to file
			FlushCache(lpCache, false);

			if(!m_lpIndex->end_transaction()) {
				m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to start transaction: %s", m_lpIndex->error().message());
				hr = MAPI_E_DISK_ERROR;
				goto exit;
			}

			// jump back were we left off, or stop processing
			if (currkey) {
				if (!cursor->jump(currkey, currlen)) {
					m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Unable to jump to next key to combine in index %s: %s", m_lpIndex->path().c_str(), m_lpIndex->error().message());
					hr = MAPI_E_DISK_ERROR;
					goto exit;
				}
			} else
				break;
		}
		// reset function
		steps = 0;
		removes = 0;
		delete [] currkey;
		currlen = 0;
		currkey = NULL;
		delete [] firstkey;
		firstlen = 0;
		firstkey = NULL;
		lpCache->clear();
	}

	// write new timestamp
	tsEnd = time(NULL);
	hr = WriteToDB(KT_OPTIMIZETS, (unsigned int)tsEnd);

	m_lpLogger->Log(EC_LOGLEVEL_FATAL, "Optimized index in %lu seconds", tsEnd - tsLast);

exit:
	// print stats, processed %d terms
	delete lpCache;
	// may not have cleaned this if we jumped
	if (firstkey && firstkey != currkey)
		delete [] firstkey;
	delete [] currkey;

	if (cursor && hr == hrSuccess)
		m_lpIndex->defrag();

	delete cursor;

	pthread_mutex_unlock(&m_writeMutex);

	return hr;
}

HRESULT ECIndexDB::Remove()
{
    std::string strPath = std::string(m_lpConfig->GetSetting("index_path")) + PATH_SEPARATOR + m_strIndexId + ".kct";
	m_lpLogger->Log(EC_LOGLEVEL_INFO, "Removing %s", strPath.c_str());
	if (unlink(strPath.c_str()) < 0) {
		switch (errno) {
		case EACCES:
			return MAPI_E_NO_ACCESS;
		case ENOENT:
			return MAPI_E_NOT_FOUND;
		default:
			return MAPI_E_CALL_FAILED;
		};
	}
	return hrSuccess;
}
