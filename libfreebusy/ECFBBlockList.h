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

/**
 * @file
 * Free/Busy block list
 *
 * @addtogroup libfreebusy
 * @{
 */

#ifndef ECFBBLOCKLIST_H
#define ECFBBLOCKLIST_H

#include "freebusy.h"
#include <map>

typedef std::map<LONG, FBBlock_1>mapFB;

class ECFBBlockList
{
public:
	ECFBBlockList(void);
	~ECFBBlockList(void);

	void Copy(ECFBBlockList *lpfbBlkList);

	HRESULT Add(FBBlock_1* lpFBBlock);
	HRESULT Next(FBBlock_1* pblk);
	HRESULT Reset();
	HRESULT Skip(LONG items);
	HRESULT Restrict(LONG tmStart, LONG tmEnd);
	void Clear();
	ULONG Size();

	HRESULT Merge(FBBlock_1* lpFBBlock);
	HRESULT GetEndTime(LONG *rtmEnd);
private:
	mapFB			m_FBMap;
	mapFB::iterator	m_FBIter;
	LONG			m_tmRestictStart;
	LONG			m_tmRestictEnd;
	bool			m_bInitIter;
};

#endif // ECFBBLOCKLIST_H
/** @} */
