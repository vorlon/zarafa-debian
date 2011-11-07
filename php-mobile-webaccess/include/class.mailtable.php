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
/**
 * @package php-mobile-webaccess
 * @author Mans Matulewicz
 * @version 0.01
 */
class mailTable extends table {
	
	var $mails;
	var $tablestart;
	var $unread;
	
	function mailTable($store, $eid)
	{
		$this->store=$store;
		$this->eid=$eid;
		$this->mails = array();
		$this->amountitems = $GLOBALS["emailsonpage"];
		
		$this->properties = array();
		$this->properties["entryid"] = PR_ENTRYID;
		$this->properties["display_name"] = PR_DISPLAY_NAME;
		$this->properties["sent_representing_name"] = PR_SENT_REPRESENTING_NAME;
		$this->properties["subject"] = PR_SUBJECT;
		$this->properties["message_delivery_time"] = PR_MESSAGE_DELIVERY_TIME;
		$this->properties["unread"] = PR_CONTENT_UNREAD;
		$this->properties["message_size"] = PR_MESSAGE_SIZE;
		$this->properties["client_submit_time"] = PR_CLIENT_SUBMIT_TIME;
		$this->properties["hasattach"] = PR_HASATTACH;
		$this->properties["message_flags"] = PR_MESSAGE_FLAGS;
		$this->properties["display_to"] = PR_DISPLAY_TO;
		
		
		$this->tablestart = 0;
		if (isSet($_GET["start"])){
			$this->tablestart = $_GET["start"];
		}

	}
	
	function settabletype()
	{
		global $smarty;
		$msgstore_props = mapi_getprops($this->store, array(PR_IPM_OUTBOX_ENTRYID, PR_IPM_SENTMAIL_ENTRYID, PR_IPM_SUBTREE_ENTRYID));
		if ($msgstore_props[PR_IPM_OUTBOX_ENTRYID] == hex2bin($this->eid))
		{
			$result ="out";
		}
		else if ($msgstore_props[PR_IPM_SENTMAIL_ENTRYID] == hex2bin($this->eid))
		{
			$result ="out";
		}
		else
		{
			$subtree = mapi_msgstore_openentry($this->store, $msgstore_props[PR_IPM_SUBTREE_ENTRYID]);
			$subtreeprops = mapi_getprops($subtree, array(PR_PARENT_ENTRYID));
			$parententry = mapi_msgstore_openentry($this->store,$subtreeprops[PR_PARENT_ENTRYID]);
			$parentprops = mapi_getprops($parententry, array(PR_IPM_DRAFTS_ENTRYID));
			if ($parentprops[PR_IPM_DRAFTS_ENTRYID]==hex2bin($this->eid))
			{
				$result="drafts";
			}
			else {
				$result="in";
			}
		}

		$smarty->assign("tabletype", $result);
		
		
	}
	
	function getcontents()
	{
		global $smarty;
		$start = isset ($_GET["start"]) ? (int) $_GET["start"] : 0;
		$this->sort = array();
		$this->sort[PR_MESSAGE_DELIVERY_TIME]=TABLE_SORT_DESCEND;
		parent::getcontents($start);
		$this->settabletype();
		
		
	}
}
?>