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

#ifndef ECINDEXERUTIL_H
#define ECINDEXERUTIL_H

#include <string>
#include <set>

#include <ECSerializer.h>

/**
 * Move stream pointer from the middle of an object to the next object in stream
 *
 * @param[in]	lpSerializer
 *					Serializer containing the stream which contains the objects
 * @param[in]	ulObjType
 *					Object type
 * @param[in]	ulProps
 *					Number of properties left until next object
 * @return HRESULT
 */
HRESULT StreamToNextObject(ECSerializer *lpSerializer, ULONG ulObjType, ULONG ulProps = 0);

/**
 * Read IStream and initialize SPropValue
 *
 * @param[in]	lpSerializer
 *					Serializer containing the stream from which property data will be read
 * @param[out]	lppProp
 *					Property to which the data will be written
 * @param[out]	lppNameId
 *					Name ID if *lppProp is a named property
 * @return std::wstring
 */
HRESULT StreamToProperty(ECSerializer *lpSerializer, LPSPropValue *lppProp, LPMAPINAMEID *lppNameId);

/**
 * Convert SPropValue into std::wstring
 *
 * @param[in]	lpProp
 *					Property to convert to std::wstring
 * @return std::wstring
 */
std::wstring PropToString(LPSPropValue lpProp);

/**
 * Create IStream object for synchronization purposes
 *
 * Create a new IStream object
 *
 * @param[out]	lppStream
 *					Reference to the IStream object where the created IStream object
 *					should be stored.
 * @param[in]	ulInitData
 *					Number of entries in the lpInitData byte array
 * @param[in]	lpInitData
 *					Array of bytes containing the initial synchronization stream
 * @return HRESULT
 */
HRESULT CreateSyncStream(IStream **lppStream, ULONG ulInitData = 0, LPBYTE lpInitData = NULL);

/**
 * Parse the exclude properties setting
 *
 * Must be space-separated hex property tags (may have leading zeros)
 *
 * @param[in]		szExclude	Exclude string
 * @param[out]		setPropIDs	Property IDs to skip
 * @return result
 */
HRESULT ParseProperties(const char *szExclude, std::set<unsigned int> &setPropIDs);


#endif /* ECINDEXERUTIL_H */
