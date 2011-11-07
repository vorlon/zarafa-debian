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
 * @todo create appointment
 * @todo edit appointment
 * @todo delete appointment
 * @todo show appointment
 *
 */
include_once('mapi/class.freebusypublish.php');

class appointmentMessage extends message
{
	var $props;
	
	function appointmentMessage($store, $eid)
	{
		$this->store=$store;
		$this->eid=$eid;
		
		
		$guid = makeguid("{00062002-0000-0000-C000-000000000046}");
		$guid2 = makeguid("{00062008-0000-0000-C000-000000000046}");
		$guid3 = makeguid("{00020329-0000-0000-C000-000000000046}");
		$names = mapi_getIdsFromNames($this->store, array(0x820D, 0x820E, 0x8223, 0x8216, 
														  0x8205, 0x8214, 0x8215, 0x8506, 
														  0x8217, 0x8235, 0x8236, 0x8208, 
														  0x8213, 0x8218, 0x8503, 0x8501,
														  0x8506, 0x853A, 0x8586, "Keywords",
														  0x8213, 0x8502, 0x8516, 0x8517,
														  0x8228, 0x8518), 
													array($guid, $guid, $guid, $guid, 
														  $guid, $guid, $guid, $guid2, 
														  $guid, $guid, $guid, $guid, 
														  $guid, $guid, $guid2, $guid2,
														  $guid2, $guid2, $guid2, $guid3,
														  $guid, $guid2, $guid2, $guid2,
														  $guid, $guid2));

		$this->properties = array();
  		$this->properties["entryid"] = PR_ENTRYID;
  		$this->properties["parent_entryid"] = PR_PARENT_ENTRYID;
  		$this->properties["message_class"] = PR_MESSAGE_CLASS;
  		$this->properties["icon_index"] = PR_ICON_INDEX;
  		$this->properties["subject"] = PR_SUBJECT;
  		$this->properties["display_to"] = PR_DISPLAY_TO;
  		$this->properties["importance"] = PR_IMPORTANCE;
  		$this->properties["sensitivity"] = PR_SENSITIVITY;
  		$this->properties["body"] = PR_BODY;
  		$this->properties["hasattach"] = PR_HASATTACH;
  		$this->properties["startdate"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[0]));
  		$this->properties["duedate"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[1]));
  		$this->properties["recurring"] = mapi_prop_tag(PT_BOOLEAN, mapi_prop_id($names[2]));
  		$this->properties["recurring_data"] = mapi_prop_tag(PT_BINARY, mapi_prop_id($names[3]));
  		$this->properties["busystatus"] = mapi_prop_tag(PT_LONG, mapi_prop_id($names[4]));
  		$this->properties["label"] = mapi_prop_tag(PT_LONG, mapi_prop_id($names[5]));
  		$this->properties["alldayevent"] = mapi_prop_tag(PT_BOOLEAN, mapi_prop_id($names[6]));
  		$this->properties["private"] = mapi_prop_tag(PT_BOOLEAN, mapi_prop_id($names[7]));
  		$this->properties["meeting"] = mapi_prop_tag(PT_LONG, mapi_prop_id($names[8]));
  		$this->properties["startdate_recurring"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[9]));
  		$this->properties["enddate_recurring"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[10]));
  		$this->properties["location"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[11]));
  		$this->properties["duration"] = mapi_prop_tag(PT_LONG, mapi_prop_id($names[12]));
  		$this->properties["responsestatus"] = mapi_prop_tag(PT_LONG, mapi_prop_id($names[13]));
  		$this->properties["reminder"] = mapi_prop_tag(PT_BOOLEAN, mapi_prop_id($names[14]));
  		$this->properties["reminder_minutes"] = mapi_prop_tag(PT_LONG, mapi_prop_id($names[15]));
  		$this->properties["private"] = mapi_prop_tag(PT_BOOLEAN, mapi_prop_id($names[16]));
		$this->properties["contacts"] = mapi_prop_tag(PT_MV_STRING8, mapi_prop_id($names[17]));
		$this->properties["contacts_string"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[18]));
		$this->properties["categories"] = mapi_prop_tag(PT_MV_STRING8, mapi_prop_id($names[19]));
		$this->properties["duration"] = mapi_prop_tag(PT_LONG, mapi_prop_id($names[20]));
		$this->properties["reminder_time"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[21]));
		$this->properties["commonstart"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[22]));
		$this->properties["commonend"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[23]));
		$this->properties["basedate"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[24]));
		$this->properties["commonassign"] = mapi_prop_tag(PT_LONG, mapi_prop_id($names[25]));
	}
	
	function getnew()
	{
		global $smarty;
		$parent = mapi_msgstore_openentry($this->store,hex2bin($this->eid));
		$parent_props = mapi_getprops($parent, array(PR_DISPLAY_NAME));
		
		$smarty->assign('parentname', $parent_props[PR_DISPLAY_NAME]);
		$smarty->assign('parent', $this->eid);
		
	}
	/**
	 * use input of new appointment form to fill all props and save it to a new message
	 * 
	 *
	 */
	function render()
	{
		global $smarty;
		$msgstore_props = mapi_getprops($this->store);
		$subtree = mapi_msgstore_openentry($this->store, $msgstore_props[PR_IPM_SUBTREE_ENTRYID]);
		
		$subtreeprops = mapi_getprops($subtree, array(PR_PARENT_ENTRYID));
		
		$parententry = mapi_msgstore_openentry($this->store,$subtreeprops[PR_PARENT_ENTRYID]);
		
		$parentprops = mapi_getprops($parententry, array(PR_IPM_APPOINTMENT_ENTRYID));
		$parent = $parentprops[PR_IPM_APPOINTMENT_ENTRYID];
		$this->props = array();
		parent::render();
		$this->props[$this->properties["parent_entryid"]] = $parent;
		$this->props[$this->properties["message_class"]] = "IPM.Appointment";
		$this->props[$this->properties["icon_index"]] = 1024;

		
		
		/**
		 * @internal When all day is checked its needed to set starttime to 00:00 on startdate and 00:00 on enddate (startdate+1)
		 */
		if (isset($_POST["allday"]) && ($_POST["allday"]=="on")){
			$this->props[$this->properties["alldayevent"]] = true;
			$this->props[$this->properties["startdate"]] = mktime(0, 0, 0, $_POST["startmonth"], $_POST["startday"], $_POST["startyear"]);
			$this->props[$this->properties["duedate"]] = mktime(0, 0, 0, $_POST["startmonth"], $_POST["startday"]+1, $_POST["startyear"]);
			$this->props[$this->properties["commonstart"]] = mktime(0, 0, 0, $_POST["startmonth"], $_POST["startday"], $_POST["startyear"]);
			$this->props[$this->properties["commonend"]] = mktime(0, 0, 0, $_POST["startmonth"], $_POST["startday"]+1, $_POST["startyear"]);
			$this->props[$this->properties["duration"]] = 1440;
		}
		else{
			$startdate = mktime(substr($_POST["starttime"], 0, 2), substr($_POST["starttime"], 3,2), 0, $_POST["startmonth"], $_POST["startday"], $_POST["startyear"]);
			$enddate = mktime(substr($_POST["endtime"], 0, 2), substr($_POST["endtime"], 3,2), 0, $_POST["endmonth"], $_POST["endday"], $_POST["endyear"]);
			if ($enddate < $startdate){
				$enddate = $startdate + 1800;
			}
			$this->props[$this->properties["alldayevent"]] = false;
			$this->props[$this->properties["startdate"]] = $startdate;
			$this->props[$this->properties["duedate"]] = $enddate;
			$this->props[$this->properties["commonstart"]] = $startdate;
			$this->props[$this->properties["commonend"]] = $enddate;
			$this->props[$this->properties["duration"]] = ($enddate - $startdate)/60;
		}
		
		/**
		 * make sure reminder is set
		 */
		if ((isset($_POST["reminder"]))&& ($_POST["reminder"]=="on")){
			$this->props[$this->properties["reminder"]] = true;
			$this->props[$this->properties["reminder_minutes"]] = $_POST["reminder_minutes"];
		}
		else {
			$this->props[$this->properties["reminder"]] = false;
			$this->props[$this->properties["reminder_minutes"]] = 0;
		}
		
		$this->props[$this->properties["commonassign"]] = 0;

		$this->props[$this->properties["recurring"]] = false;
		
		$this->saveMessage($this->store,$parent, $this->props, array(), "", $output);
		/**
		 * after message is put in the user will be shown the appointment view
		 */
		header("Location: index.php?entryid=".bin2hex($output[PR_ENTRYID])."&type=msg&task=IPM.Appointment");
	}
	
	function save()
	{
		global $smarty, $mapi;

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
			
			$parentprops = mapi_getprops($parententry, array(PR_IPM_APPOINTMENT_ENTRYID));
			$parent = $parentprops[PR_IPM_APPOINTMENT_ENTRYID];
		}

		$this->render();
		$this->props[$this->properties["message_class"]] = "IPM.Appointment";
		$this->props[$this->properties["parent_entryid"]] = $parent;
		$this->props[$this->properties["commonassign"]] = 0;

		$this->props[$this->properties["recurring"]] = false;
		
		$this->saveMessage($this->store, $parent, $this->props, array(), "", $output);

		// Publish updated free/busy information
		if($parent) {
			$calendar = mapi_msgstore_openentry($this->store, $parent);
			$props = mapi_getprops($this->store, Array(PR_MAILBOX_OWNER_ENTRYID));
			if($props[PR_MAILBOX_OWNER_ENTRYID]) {
				$pub = new FreeBusyPublish($mapi->session, $this->store, $calendar, $props[PR_MAILBOX_OWNER_ENTRYID]);
				$pub->publishFB(time() - (7 * 24 * 60 * 60), 6 * 30 * 24 * 60 * 60); // publish from one week ago, 6 months ahead
			}
		}
	}
}
?>
