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
 * 
 *
 */
class table {
	
	var $store;
	var $eid;
	var $parent;
	var $parentname;
	var $name;
	var $unread;
	var $amountitems;
	var $sort;
	
	function table($store, $eid)
	{
		$this->store=$store;
		$this->eid=$eid;
	}
	
	function get_parent()
	{
		global $smarty;
		// get name from the store and use this in the navigation link
		$msgstore_props = mapi_getprops($this->store, array(PR_ENTRYID, PR_DISPLAY_NAME));
		$this->parent= $msgstore_props[PR_ENTRYID];
		$smarty->assign('parent', bin2hex($msgstore_props[PR_ENTRYID]));
		$this->parentname= $msgstore_props[PR_DISPLAY_NAME];
		$smarty->assign('parentname', $msgstore_props[PR_DISPLAY_NAME]);
	}

	function getcontents($start=0)
	{
		global $smarty;
		
		$box = mapi_msgstore_openentry($this->store, hex2bin($this->eid));
		$boxprops = mapi_getprops($box, array(PR_DISPLAY_NAME));
		$smarty->assign("display_name", $boxprops[PR_DISPLAY_NAME]);
		$contents = mapi_folder_getcontentstable($box);

		// sort on subject
		
		mapi_table_sort($contents, $this->sort);
		$rows = mapi_table_queryrows($contents, $this->properties, $start, $this->amountitems);
		$smartyarray = array();	
		foreach ($rows as $row)
		{
			$item = array();
			foreach ($this->properties as $key => $property)
			{
				switch ($property)
				{
					case PR_SENT_REPRESENTING_NAME:
						if ((isset($row[$property])) && ($row[$property]!="")){
						
							if (strlen($row[$property]) > 14) {
								$item[$key]=substr($row[$property], 0, 11)."...";
							}
							else {
								$item[$key]=$row[$property];
							}
						}
						else {
							$item[$key]=NOSENDERMSG;
						}
						break;
					
					case PR_ENTRYID:
						$item[$key]=bin2hex($row[$property]);
						break;
						
					case PR_MESSAGE_SIZE:
						$item[$key]=round($row[$property]/1000);
						break;
					
					case PR_DISPLAY_TO:
						if ((isset($row[$property])) && ($row[$property]!="")){
							if (strlen($row[$property]) > 14) {
								$item[$key]=substr($row[$property], 0, 11)."...";
							}
							else {
								$item[$key]=$row[$property];
							}
						}
						else {
							$item[$key]=NOSENDERMSG;
						}
						break;
						
					case PR_MESSAGE_DELIVERY_TIME:
						if (isset($row[$property])){
							if (date("y-m-d",$row[$property])== date('y-m-d')){
								$item[$key]=date("h:i:s",$row[$property]);
							}
							else {
								$item[$key]=date('d-m-Y',$row[$property]);
							}
						}
						break;
						
					case PR_SUBJECT:
						if ((isset($row[$property])) && ($row[$property]!="")){
							$item[$key]=$row[$property];
						}
						else {
							$item[$key]=NOSUBJECTMSG;
						}
						break;
													
					default:
						$item[$key]=isset($row[$property]) ? $row[$property] : null;
						break;
							
				}		
			}
			if ((!isset($row[PR_MESSAGE_DELIVERY_TIME])) && (isset($row[PR_CLIENT_SUBMIT_TIME]))) {
				$item["message_delivery_time"]=date('d-m-Y',$row[PR_CLIENT_SUBMIT_TIME]);
			}
			array_push($smartyarray, $item);
		}
		$smarty->assign('items', $smartyarray);
		$this->get_parent();
	}
	
}
?>
