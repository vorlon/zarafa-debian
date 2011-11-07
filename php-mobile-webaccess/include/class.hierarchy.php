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
 * @copyright 2006
 */

class hierarchy{
	
	/**
	 * Contains the message store
	 *
	 * @var resource MAPI_MessageStore
	 */
	var $store;
	var $folders;
	var $properties;
	
	function hierarchy($store)
	{
		$this->store = $store;
		$this->folders = array();
		$this->properties = array();
		$this->properties["entryid"] =PR_ENTRYID;
		$this->properties["name"]=PR_DISPLAY_NAME;
		$this->properties["unread"]=PR_CONTENT_UNREAD;
		$this->properties["subfolders"] = PR_SUBFOLDERS;
		$this->properties["container_class"] = PR_CONTAINER_CLASS;
		
	}

	function subfolders($entryid)
	{
		$smartyarray = array();
		$subfoldersentry = mapi_msgstore_openentry($this->store, $entryid);
		$table = mapi_folder_gethierarchytable($subfoldersentry);
		mapi_table_sort($table, array(PR_DISPLAY_NAME => TABLE_SORT_ASCEND));
		$rows = mapi_table_queryallrows($table, array(PR_ENTRYID, PR_DISPLAY_NAME, PR_CONTENT_UNREAD, PR_CONTAINER_CLASS, PR_SUBFOLDERS));
		foreach ($rows  as $row) 	
		{
			$item = array();
			foreach ($this->properties as $key => $property)
			{
				if ($property==PR_ENTRYID){
					$item[$key]=bin2hex($row[$property]);
				}
				else {
					$item[$key]=$row[$property];
				}
				
			}
			if ($item["subfolders"]){
				$item["subfolders"] = $this->subfolders(hex2bin($item["entryid"]));
			}
			array_push($smartyarray, $item);
	
		}
		return $smartyarray;
	}
	
	function getcontents()
	{
		global $smarty;
		//de properties van default messagestore
		$msgstore_props = mapi_getprops($this->store, array(PR_ENTRYID, PR_DISPLAY_NAME, PR_IPM_SUBTREE_ENTRYID, PR_IPM_OUTBOX_ENTRYID, PR_IPM_SENTMAIL_ENTRYID, PR_IPM_WASTEBASKET_ENTRYID));
		
		//Open de tree onder de default messagestore
		$folder= mapi_msgstore_openentry($this->store, $msgstore_props[PR_IPM_SUBTREE_ENTRYID]);
		
		//haal de table op
		$table = mapi_folder_gethierarchytable($folder);
		mapi_table_sort($table, array(PR_DISPLAY_NAME => TABLE_SORT_ASCEND));
		
		//mapi_folder_sort($folder, $sort);
		
		//vraag de informatie van alle mappen op
		$rows = mapi_table_queryallrows($table, array(PR_ENTRYID, PR_DISPLAY_NAME, PR_CONTENT_UNREAD, PR_CONTAINER_CLASS, PR_SUBFOLDERS));
		
		//hier wordt elke map in de array getoont
		$smartyarray = array();
		foreach ($rows  as $row) 	
		{
			$item = array();
			foreach ($this->properties as $key => $property)
			{
				if ($property==PR_ENTRYID){
					$item[$key]=bin2hex($row[$property]);
				}
				else {
					if (isset($row[$property])){
						$item[$key]=$row[$property];
					}
					
				}
			}
			if (isset($_GET["entryid"]))
			{
				if ($_GET["entryid"]== $item["entryid"]) 
				{
					$item["subfolders"]  = $this->subfolders(hex2bin($_GET["entryid"]));
				}
			}
			array_push($smartyarray, $item);
		}
		$smarty->assign('folders', $smartyarray);
		
		
	}
	
	
	function getallcontents()
	{
		global $smarty;
		//de properties van default messagestore
		$msgstore_props = mapi_getprops($this->store, array(PR_ENTRYID, PR_DISPLAY_NAME, PR_IPM_SUBTREE_ENTRYID, PR_IPM_OUTBOX_ENTRYID, PR_IPM_SENTMAIL_ENTRYID, PR_IPM_WASTEBASKET_ENTRYID));
			
		//Open de tree onder de default messagestore
		$folder= mapi_msgstore_openentry($this->store, $msgstore_props[PR_IPM_SUBTREE_ENTRYID]);
		
		//haal de table op
		$table = mapi_folder_gethierarchytable($folder);
		mapi_table_sort($table, array(PR_DISPLAY_NAME => TABLE_SORT_ASCEND));
		
		//mapi_folder_sort($folder, $sort);
		
		//vraag de informatie van alle mappen op
		$rows = mapi_table_queryallrows($table, array(PR_ENTRYID, PR_DISPLAY_NAME, PR_CONTENT_UNREAD, PR_CONTAINER_CLASS, PR_SUBFOLDERS));
			
		//hier wordt elke map in de array getoont
		$smartyarray = array();
		foreach ($rows  as $row) 	
		{
			$item = array();
			foreach ($this->properties as $key => $property)
			{
				if ($property==PR_ENTRYID){
					$item[$key]=bin2hex($row[$property]);
				}
				else {
					$item[$key]=$row[$property];
				}
			}
			if (isset($row[PR_SUBFOLDERS]))
			{
					$item["subfolders"]  = $this->subfolders($row[PR_ENTRYID]);
			}
			array_push($smartyarray, $item);
		}
		$smarty->assign('folders', $smartyarray);
		
		
	}

}
?>
