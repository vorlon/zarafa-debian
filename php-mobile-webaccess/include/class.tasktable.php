<?php
/*
 * Copyright 2005 - 2009  Zarafa B.V.
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

?>
<?php

class taskTable extends table 
{
	function taskTable($store, $eid)
	{
		$this->store=$store;
		$this->eid=$eid;
		$this->amountitems = $GLOBALS["tasksonpage"];
		
		$guid = makeguid("{00062003-0000-0000-C000-000000000046}");
		$guid2 = makeguid("{00062008-0000-0000-C000-000000000046}");
		$guid3 = makeguid("{00020329-0000-0000-C000-000000000046}");
		$names = mapi_getIdsFromNames($this->store, array(0x811C, 0x8105, 0x8101, 0x8102, 
														  0x8104, 0x811F, 0x8503, 0x8502,
														  0x810F, 0x8111, 0x8110, 0x8539,
														  0x8534, 0x8535, 0x8506, 0x853A, 
														  0x8586, "Keywords", 0x8502, 
														  0x8516, 0x8517),
													array($guid, $guid, $guid, $guid, 
														  $guid, $guid, $guid2, $guid2,
														  $guid, $guid, $guid, $guid2,
														  $guid2, $guid2, $guid2, $guid2, 
														  $guid2, $guid3, $guid2,
														  $guid2, $guid2));
														  
		$this->properties = array();
		$this->properties["entryid"] = PR_ENTRYID;
		$this->properties["parent_entryid"] = PR_PARENT_ENTRYID;
		$this->properties["subject"] = PR_SUBJECT;
		$this->properties["importance"] = PR_IMPORTANCE;
		$this->properties["complete"] = mapi_prop_tag(PT_BOOLEAN, mapi_prop_id($names[0]));
		$this->properties["duedate"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[1]));		
	}
	
	function getcontents()
	{
		global $smarty;
		$this->sort = array();
		$this->sort[$this->properties["complete"]]=TABLE_SORT_ASCEND;
		$this->sort[$this->properties["duedate"]]=TABLE_SORT_ASCEND;
		parent::getcontents();
		parent::get_parent();
	}
}
?>
