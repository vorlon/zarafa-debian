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

#ifndef TYPECONVERSION_H
#define TYPECONVERSION_H

/*
 * These functions convert from MAPI types (structs) to PHP arrays and types and vice versa
 */

#include "globals.h"
ZEND_EXTERN_MODULE_GLOBALS(mapi)

#include "charset/convert.h"
#include "options.h"

/*
 * PHP -> MAPI
 *
 * All functions return a newly allocation MAPI structure which must be MAPIFreeBuffer()'ed by
 * the caller.
 */

// These allocate the structure and copy the data into it, then returns the entire allocated structure, allocation
// via lpBase if non null, otherwise just as MAPIAllocateBuffer

HRESULT			PHPArraytoSBinaryArray(zval * entryid_array, void *lpBase, LPENTRYLIST *lppEntryList TSRMLS_DC);
HRESULT			PHPArraytoSortOrderSet(zval * sortorder_array, void *lpBase, LPSSortOrderSet *lppSortOrderSet TSRMLS_DC);
HRESULT			PHPArraytoPropTagArray(zval * prop_value_array, void *lpBase, LPSPropTagArray *lppPropTagArray TSRMLS_DC);
HRESULT			PHPArraytoPropValueArray(zval* phpArray, void *lpBase, ULONG *lpcValues, LPSPropValue *lppPropValues TSRMLS_DC);
HRESULT			PHPArraytoAdrList(zval *phpArray, void *lpBase, LPADRLIST *lppAdrList TSRMLS_DC);
HRESULT			PHPArraytoRowList(zval *phpArray, void *lpBase, LPROWLIST *lppRowList TSRMLS_DC);
HRESULT			PHPArraytoSRestriction(zval *phpVal, void *lpBase, LPSRestriction *lppRestriction TSRMLS_DC);
HRESULT			PHPArraytoReadStateArray(zval *phpVal, void *lpBase, ULONG *lpcValues, LPREADSTATE *lppReadStates TSRMLS_DC);
HRESULT			PHPArraytoGUIDArray(zval *phpVal, void *lpBase, ULONG *lpcValues, LPGUID *lppGUIDs TSRMLS_DC);

// These functions fill a pre-allocated structure, possibly allocating more memory via lpBase

HRESULT		 	PHPArraytoSBinaryArray(zval * entryid_array, void *lpBase, LPENTRYLIST lpEntryList TSRMLS_DC);
HRESULT 		PHPArraytoSRestriction(zval *phpVal, void *lpBase, LPSRestriction lpRestriction TSRMLS_DC);

/* imtoinet, imtomapi options */
HRESULT			PHPArraytoSendingOptions(zval *phpArray, sending_options *lpSOPT);
HRESULT			PHPArraytoDeliveryOptions(zval *phpArray, delivery_options *lpDOPT);

/*
 * MAPI -> PHP
 *
 * All functions return a newly allocated ZVAL structure which must be FREE_ZVAL()'ed by the caller.
 */
 
HRESULT			SBinaryArraytoPHPArray(SBinaryArray *lpBinaryArray, zval **ret TSRMLS_DC);
HRESULT			PropTagArraytoPHPArray(ULONG cValues, LPSPropTagArray lpPropTagArray, zval **ret TSRMLS_DC);
HRESULT			PropValueArraytoPHPArray(ULONG cValues, LPSPropValue pPropValueArray, zval **ret TSRMLS_DC);
HRESULT 		SRestrictiontoPHPArray(LPSRestriction lpRes, int level, zval **ret TSRMLS_DC);
HRESULT			RowSettoPHPArray(LPSRowSet lpRowSet, zval **ret TSRMLS_DC);
HRESULT 		ReadStateArraytoPHPArray(ULONG cValues, LPREADSTATE lpReadStates, zval **ret TSRMLS_DC);
HRESULT			NotificationstoPHPArray(ULONG cNotifs, LPNOTIFICATION lpNotifs, zval **ret TSRMLS_DC);

#endif
