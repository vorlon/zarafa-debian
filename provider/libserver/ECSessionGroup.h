/*
 * Copyright 2005 - 2015  Zarafa B.V. and its licensors
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation with the following
 * additional terms according to sec. 7:
 * 
 * "Zarafa" is a registered trademark of Zarafa B.V.
 * The licensing of the Program under the AGPL does not imply a trademark 
 * license. Therefore any rights, title and interest in our trademarks 
 * remain entirely with us.
 * 
 * Our trademark policy, <http://www.zarafa.com/zarafa-trademark-policy>,
 * allows you to use our trademarks in connection with Propagation and 
 * certain other acts regarding the Program. In any case, if you propagate 
 * an unmodified version of the Program you are allowed to use the term 
 * "Zarafa" to indicate that you distribute the Program. Furthermore you 
 * may use our trademarks where it is necessary to indicate the intended 
 * purpose of a product or service provided you use it in accordance with 
 * honest business practices. For questions please contact Zarafa at 
 * trademark@zarafa.com.
 *
 * The interactive user interface of the software displays an attribution 
 * notice containing the term "Zarafa" and/or the logo of Zarafa. 
 * Interactive user interfaces of unmodified and modified versions must 
 * display Appropriate Legal Notices according to sec. 5 of the GNU Affero 
 * General Public License, version 3, when you propagate unmodified or 
 * modified versions of the Program. In accordance with sec. 7 b) of the GNU 
 * Affero General Public License, version 3, these Appropriate Legal Notices 
 * must retain the logo of Zarafa or display the words "Initial Development 
 * by Zarafa" if the display of the logo is not reasonably feasible for
 * technical reasons.
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

// ECSessionGroup.h: interface for the ECSessionGroup class.
//
//////////////////////////////////////////////////////////////////////

#ifndef ECSESSIONGROUP
#define ECSESSIONGROUP

#include <list>
#include <map>
#include <set>

#include "ECKeyTable.h"
#include "ECNotification.h"
#include "ZarafaCode.h"
#include "CommonUtil.h"

class ECSession;
class ECSessionGroup;
class ECSessionManager;

struct sessionInfo {
	sessionInfo(ECSession *lpSession) : lpSession(lpSession) {}
	ECSession	 *lpSession;
};

typedef std::map<ECSESSIONID, sessionInfo> SESSIONINFOMAP;

struct subscribeItem {
	ECSESSIONID	 ulSession;		// Unique session identifier
	unsigned int ulConnection;	// Unique client identifier for notification
	unsigned int ulKey;			// database object id (also storeid) or a tableid
	unsigned int ulEventMask;
};

typedef std::list<ECNotification> NOTIFICATIONLIST;
typedef std::map<unsigned int, subscribeItem> SUBSCRIBEMAP;
typedef std::multimap<unsigned int, unsigned int> SUBSCRIBESTOREMULTIMAP;

struct changeSubscribeItem {
	ECSESSIONID		ulSession;
	unsigned int	ulConnection;
	notifySyncState	sSyncState;
};
typedef std::multimap<unsigned int, changeSubscribeItem> CHANGESUBSCRIBEMAP;	// SyncId -> changeSubscribeItem

class ECSessionGroup
{
public:
	ECSessionGroup(ECSESSIONGROUPID sessionGroupId, ECSessionManager *lpSessionManager);
	virtual ~ECSessionGroup();

	/*
	 * Thread safety handlers
	 */
	virtual void Lock();
	virtual void Unlock();
	virtual bool IsLocked();

	/*
	 * Returns the SessionGroupId
	 */
	virtual ECSESSIONGROUPID GetSessionGroupId();

	/*
	 * Add/Remove Session from group
	 */
	virtual void AddSession(ECSession *lpSession);
	virtual void ShutdownSession(ECSession *lpSession);
	virtual void ReleaseSession(ECSession *lpSession);

	/*
	 * Update session time for all attached sessions
	 */
	virtual void UpdateSessionTime();

	/*
	 * Check is SessionGroup has lost all its children
	 */
	virtual bool isOrphan();

	/*
	 * Item subscription
	 */
	virtual ECRESULT AddAdvise(ECSESSIONID ulSessionId, unsigned int ulConnection, unsigned int ulKey, unsigned int ulEventMask);
	virtual ECRESULT AddChangeAdvise(ECSESSIONID ulSessionId, unsigned int ulConnection, notifySyncState *lpSyncState);
	virtual ECRESULT DelAdvise(ECSESSIONID ulSessionId, unsigned int ulConnection);

	/*
	 * Notifications
	 */
	virtual ECRESULT AddNotification(notification *notifyItem, unsigned int ulKey, unsigned int ulStore, ECSESSIONID ulSessionId = 0);
	virtual ECRESULT AddNotificationTable(ECSESSIONID ulSessionId, unsigned int ulType, unsigned int ulObjType, unsigned int ulTableId,
										  sObjectTableKey* lpsChildRow, sObjectTableKey* lpsPrevRow, struct propValArray *lpRow);
	virtual ECRESULT AddChangeNotification(const std::set<unsigned int> &syncIds, unsigned int ulChangeId, unsigned int ulChangeType);
	virtual ECRESULT AddChangeNotification(ECSESSIONID ulSessionId, unsigned int ulConnection, unsigned int ulSyncId, unsigned long ulChangeId);
	virtual ECRESULT GetNotifyItems(struct soap *soap, ECSESSIONID ulSessionId, struct notifyResponse *notifications);

	unsigned int GetObjectSize();

private:
	ECRESULT releaseListeners();

	/* Personal SessionGroupId */
	ECSESSIONGROUPID	m_sessionGroupId;

	/* All Sessions attached to this group */
	SESSIONINFOMAP		m_mapSessions;

	/* List of all items the group is subscribed to */
	SUBSCRIBEMAP        m_mapSubscribe;
	CHANGESUBSCRIBEMAP	m_mapChangeSubscribe;
	pthread_mutex_t		m_hSessionMapLock;

	/* Notifications */
	NOTIFICATIONLIST	m_listNotification;
	double				m_dblLastQueryTime;

	/* Notifications lock/event */
	pthread_mutex_t		m_hNotificationLock;
	pthread_cond_t      m_hNewNotificationEvent;
	ECSESSIONID			m_getNotifySession;

	/* Thread safety mutex/event */
	unsigned int		m_ulRefCount;
	pthread_mutex_t		m_hThreadReleasedMutex;
	pthread_cond_t		m_hThreadReleased;

	/* Set to TRUE if no more GetNextNotifyItems() should be done on this group since the main
	 * session has exited
	 */
	bool				m_bExit;
	
	/* Reference to the session manager needed to notify changes in our queue */
	ECSessionManager *	m_lpSessionManager;
	
	/* Multimap of subscriptions that we have (key -> store id) */
	SUBSCRIBESTOREMULTIMAP	m_mapSubscribedStores;
	pthread_mutex_t		m_mutexSubscribedStores;
	
private:
	// Make ECSessionGroup non-copyable
	ECSessionGroup(const ECSessionGroup &);
	ECSessionGroup& operator=(const ECSessionGroup &);
};

#endif // #ifndef ECSESSIONGROUP
