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

class taskMessage extends message 
{
	var $props;
	
	function taskMessage($store, $eid="")
	{
		$this->store=$store;
		$this->eid=$eid;
		
		$guid = makeguid("{00062003-0000-0000-C000-000000000046}");
		$guid2 = makeguid("{00062008-0000-0000-C000-000000000046}");
		$guid3 = makeguid("{00020329-0000-0000-C000-000000000046}");
		$names = mapi_getIdsFromNames($this->store, array(0x811C, 0x8105, 0x8101, 0x8102, 
														  0x8104, 0x811F, 0x8503, 0x8502,
														  0x810F, 0x8111, 0x8110, 0x8539,
														  0x8534, 0x8535, 0x8506, 0x853A, 
														  0x8586, "Keywords", 0x8502, 
														  0x8516, 0x8517, 0x8518),
													array($guid, $guid, $guid, $guid, 
														  $guid, $guid, $guid2, $guid2,
														  $guid, $guid, $guid, $guid2,
														  $guid2, $guid2, $guid2, $guid2, 
														  $guid2, $guid3, $guid2,
														  $guid2, $guid2, $guid2));
														  
		$this->properties = array();
		$this->properties["entryid"] = PR_ENTRYID;
		$this->properties["parent_entryid"] = PR_PARENT_ENTRYID;
		$this->properties["icon_index"] = PR_ICON_INDEX;
		$this->properties["message_class"] = PR_MESSAGE_CLASS;
		$this->properties["subject"] = PR_SUBJECT;
		$this->properties["importance"] = PR_IMPORTANCE;
		$this->properties["sensitivity"] = PR_SENSITIVITY;
		$this->properties["last_modification_time"] = PR_LAST_MODIFICATION_TIME;
		$this->properties["complete"] = mapi_prop_tag(PT_BOOLEAN, mapi_prop_id($names[0]));
		$this->properties["duedate"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[1]));
		$this->properties["status"] = mapi_prop_tag(PT_LONG, mapi_prop_id($names[2]));
		$this->properties["percent_complete"] = mapi_prop_tag(PT_DOUBLE, mapi_prop_id($names[3]));
		$this->properties["startdate"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[4]));
		$this->properties["owner"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[5]));
		$this->properties["reminder"] = mapi_prop_tag(PT_BOOLEAN, mapi_prop_id($names[6]));
		$this->properties["reminderdate"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[7]));
		$this->properties["datecompleted"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[8]));
		$this->properties["totalwork"] = mapi_prop_tag(PT_LONG, mapi_prop_id($names[9]));
		$this->properties["actualwork"] = mapi_prop_tag(PT_LONG, mapi_prop_id($names[10]));
		$this->properties["companies"] = mapi_prop_tag(PT_MV_STRING8, mapi_prop_id($names[11]));
		$this->properties["mileage"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[12]));
		$this->properties["billinginformation"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[13]));
		$this->properties["private"] = mapi_prop_tag(PT_BOOLEAN, mapi_prop_id($names[14]));
		$this->properties["contacts"] = mapi_prop_tag(PT_MV_STRING8, mapi_prop_id($names[15]));
		$this->properties["contacts_string"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[16]));
		$this->properties["categories"] = mapi_prop_tag(PT_MV_STRING8, mapi_prop_id($names[17]));
		$this->properties["reminder_time"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[18]));
		$this->properties["commonstart"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[19]));
		$this->properties["commonend"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[20]));
		$this->properties["commonassign"] = mapi_prop_tag(PT_LONG, mapi_prop_id($names[21]));
		$this->amountitems=10;
		
	}
	
	function render()
	{
		global $smarty;
		if (isset($_POST["entryid"]))
		{
			$parententry = mapi_msgstore_openentry($this->store,hex2bin($_POST["entryid"]));
			$parentprops = mapi_getprops($parententry, array(PR_PARENT_ENTRYID));
			$parent = $parentprops[PR_PARENT_ENTRYID];
		}
		else {
			$msgstore_props = mapi_getprops($this->store);
			$subtree = mapi_msgstore_openentry($this->store, $msgstore_props[PR_IPM_SUBTREE_ENTRYID]);
			
			$subtreeprops = mapi_getprops($subtree, array(PR_PARENT_ENTRYID));
			
			$parententry = mapi_msgstore_openentry($this->store,$subtreeprops[PR_PARENT_ENTRYID]);
			
			$parentprops = mapi_getprops($parententry, array(PR_IPM_TASK_ENTRYID));
			$parent = $parentprops[PR_IPM_TASK_ENTRYID];
		}
		
		
		
		
		$this->props = array();
		$message = parent::render();
		$this->props[PR_MESSAGE_CLASS]="IPM.Task";
		$this->props[$this->properties["complete"]] = false;
		if ($_POST["startdate"]=="on"){
			$this->props[$this->properties["startdate"]] = gmmktime(0, 0, 0, $_POST["startmonth"], $_POST["startday"], $_POST["startyear"]);
		}
		
		if ($_POST["duedate"]=="on"){
			$this->props[$this->properties["duedate"]]= gmmktime(0, 0, 0, $_POST["duemonth"], $_POST["dueday"], $_POST["dueyear"]);
		}
		
		if ((isset($_POST["reminder"]))&& ($_POST["reminder"]=="on")){
			$this->props[$this->properties["reminder"]] = true;
			$this->props[$this->properties["reminderdate"]] =  gmmktime($_POST["remindhour"], $_POST["remindminute"], 0, $_POST["remindmonth"], $_POST["remindday"], $_POST["remindyear"]);
		}
		else {
			$this->props[$this->properties["reminder"]] = false;
		}
		
		if ((($_POST["percent_complete"]<100) && ($_POST["percent_complete"]>0))&&(($_POST["status"]=="2") || ($_POST["status"]==0))) {
			$this->props[$this->properties["status"]] = 1;
			$this->props[$this->properties["complete"]] = false;
			$this->props[$this->properties["percent_complete"]] = $_POST["percent_complete"]/100;
		} 
		else if (($_POST["status"]==2)||($_POST["percent_complete"]==100)){
			$this->props[$this->properties["percent_complete"]] = 1.0;
			$this->props[$this->properties["complete"]] = true;
			$this->props[$this->properties["status"]] = 2;
		}
		else if ($_POST["status"]==0){
			$this->props[$this->properties["percent_complete"]] = 0.0;
			$this->props[$this->properties["complete"]] = false;
		}
		else if ($_POST["percent_complete"]>1)
		{
			$this->props[$this->properties["percent_complete"]] = $_POST["percent_complete"]/100;
		}
		$this->props[$this->properties["commonassign"]] = 0;
		
		$task = $this->saveMessage($this->store,$parent, $this->props, array(), "", $output);
		$table = new taskTable($this->store, bin2hex($parent));
		$table->getcontents();
		$smarty->display('tasktable.tpl');
	}
	
	function update()
	{
		global $smarty;
		$msgstore_props = mapi_getprops($this->store);
		$subtree = mapi_msgstore_openentry($this->store, $msgstore_props[PR_IPM_SUBTREE_ENTRYID]);
		
		$subtreeprops = mapi_getprops($subtree, array(PR_PARENT_ENTRYID));
		
		$parententry = mapi_msgstore_openentry($this->store,$subtreeprops[PR_PARENT_ENTRYID]);
		
		$parentprops = mapi_getprops($parententry, array(PR_IPM_TASK_ENTRYID));
		$parent = $parentprops[PR_IPM_TASK_ENTRYID];
		
		
		
		$this->props = array();
		$message = parent::render();
		if (isset ($_POST["complete"]) && $_POST["complete"]=="on") {
			$this->props[$this->properties["complete"]] = true;
			$this->props[$this->properties["percent_complete"]] = 1.0;
			$this->props[$this->properties["status"]] = 2;
		}
		else{
			$this->props[$this->properties["complete"]] = false;
			$this->props[$this->properties["percent_complete"]] = 0.0;
			$this->props[$this->properties["status"]] = 0;
			
		}
		$this->props[$this->properties["message_class"]]="IPM.Task";
		$this->props[$this->properties["entryid"]]=hex2bin($_POST["entryid"]);
		$this->props[$this->properties["commonassign"]]=0;
		
			
		$contact = $this->saveMessage($this->store,$parent, $this->props, array(), "", $output);

		
		$table = new taskTable($this->store, bin2hex($parent));
		$table->getcontents();
		$smarty->display('tasktable.tpl');
	}
}
?>
