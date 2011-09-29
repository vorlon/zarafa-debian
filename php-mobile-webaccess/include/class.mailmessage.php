<?php
/*
 * Copyright 2005 - 2011  Zarafa B.V.
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

class mailMessage extends message {
	
	var $sendername;
	var $subject;
	var $body;
	var $deliverytime;
	var $senderemail;
	var $flags;
	var $size;
	var $parentid;
	var $parentname;
	var $attachments;
	var $eid;
	var $icon_index;
	var $icon;
	var $smarty;
	
	function mailMessage($store, $eid="")
	{
		$this->store=$store;
		$this->eid=$eid;
		
		$guid = makeguid("{00062002-0000-0000-C000-000000000046}");
  		$guid2 = makeguid("{00020329-0000-0000-C000-000000000046}");
  		$names = mapi_getIdsFromNames($this->store, array(0x8208, 0x820D, 0x820E, "Keywords"), array($guid, $guid, $guid, $guid2));

		$this->properties = array();
		$this->properties["entryid"] = PR_ENTRYID;
		$this->properties["parent_entryid"] = PR_PARENT_ENTRYID;
		$this->properties["message_class"] = PR_MESSAGE_CLASS;
		$this->properties["icon_index"] = PR_ICON_INDEX;
		$this->properties["display_to"] = PR_DISPLAY_TO;
		$this->properties["display_cc"] = PR_DISPLAY_CC;
		$this->properties["subject"] = PR_SUBJECT;
		$this->properties["normalized_subject"] = PR_NORMALIZED_SUBJECT;
		$this->properties["importance"] = PR_IMPORTANCE;
		$this->properties["sent_representing_name"] = PR_SENT_REPRESENTING_NAME;
		$this->properties["sent_representing_entryid"] = PR_SENT_REPRESENTING_ENTRYID;
		$this->properties["received_by_name"] = PR_RECEIVED_BY_NAME;
		$this->properties["received_by_email_address"] = PR_RECEIVED_BY_EMAIL_ADDRESS;
		$this->properties["message_delivery_time"] = PR_MESSAGE_DELIVERY_TIME;
		$this->properties["last_verb_executed"] = PR_LAST_VERB_EXECUTED;
		$this->properties["last_verb_execution_time"] = PR_LAST_VERB_EXECUTION_TIME;
		$this->properties["hasattach"] = PR_HASATTACH;
		$this->properties["message_size"] = PR_MESSAGE_SIZE;
		$this->properties["message_flags"] = PR_MESSAGE_FLAGS;
		$this->properties["flag_status"] = PR_FLAG_STATUS;
		$this->properties["flag_complete_time"] = PR_FLAG_COMPLETE_TIME;
		$this->properties["flag_icon"] = PR_FLAG_ICON;
		$this->properties["client_submit_time"] = PR_CLIENT_SUBMIT_TIME;
		$this->properties["sensitivity"] = PR_SENSITIVITY;
		$this->properties["read_receipt_requested"] = PR_READ_RECEIPT_REQUESTED;
		$this->properties["originator_delivery_report_requested"] = PR_ORIGINATOR_DELIVERY_REPORT_REQUESTED;
		$this->properties["location"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[0]));
		$this->properties["startdate"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[1]));
		$this->properties["duedate"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[2]));
		$this->properties["categories"] = mapi_prop_tag(PT_MV_STRING8, mapi_prop_id($names[3]));
		$this->properties["body"] = PR_BODY;
		$this->properties["conversation_index"] = PR_CONVERSATION_INDEX;
		$this->properties["conversation_topic"] = PR_CONVERSATION_TOPIC;
		
				

	}

	
	function render()
	{
		global $smarty;
		$msgstore_props = mapi_getprops($this->store);
		if (isset($_POST["send"]))
		{
			$outbox = mapi_msgstore_openentry($this->store, $msgstore_props[PR_IPM_OUTBOX_ENTRYID]);
			$parent = $msgstore_props[PR_IPM_OUTBOX_ENTRYID];
			$recipients = $_POST["recipients"];
			
			
			
			$this->props = array();
			$message = parent::render();
			$this->props[PR_MESSAGE_CLASS]="IPM.Note";
			$this->props[PR_CONVERSATION_TOPIC]=isset($this->props[PR_CONVERSATION_TOPIC])? $this->props[PR_CONVERSATION_TOPIC]:$this->props[PR_SUBJECT];
			$this->submitMessage($this->store, $this->props, $recipients, "", $output);
			$this->setMessageFlag($this->store, $output[PR_ENTRYID], "read", $output2 );
			header("Location: index.php");
		}
		else if (isset($_POST["draft"])){
			$subtree = mapi_msgstore_openentry($this->store, $msgstore_props[PR_IPM_SUBTREE_ENTRYID]);
		
			$subtreeprops = mapi_getprops($subtree, array(PR_PARENT_ENTRYID));
			
			$parententry = mapi_msgstore_openentry($this->store,$subtreeprops[PR_PARENT_ENTRYID]);
			
			$parentprops = mapi_getprops($parententry, array(PR_IPM_DRAFTS_ENTRYID));
			$parent = $parentprops[PR_IPM_DRAFTS_ENTRYID];
			
			$this->props = array();
			$message = parent::render();
			$this->props[PR_MESSAGE_CLASS]="IPM.Note";
			$recipients = $_POST["recipients"];
			$this->saveMessage($this->store, $parent, $this->props, $recipients, $checknum, $messageProps);
			header("Location: index.php");
		}
		
	}

	function createForward()
	{
		global $smarty;
		$this->props = array();
		parent::getcontents();
		$smarty->assign("action", "forward");
		$smarty->assign("subject", "FW: ".$this->props["normalized_subject"]);
		$smarty->assign("conversation_index", $this->props["conversation_index"]);
		
		$replystring = _("At [time] [name] wrote:");
		$replystring = str_replace ("[time]", $this->props["date"], $replystring );
		$replystring = str_replace ("[name]", $this->props["sent_representing_name"], $replystring );
		$smarty->assign("body", "\n\n".$replystring."\n".$this->props["body"]);
	}
	
	function createReply()
	{
		global $smarty;
		$this->props = array();
		parent::getcontents();
		$smarty->assign("subject", "RE: ".$this->props["normalized_subject"]);
		$smarty->assign("conversation_index", $this->props["conversation_index"]);
		$replystring = _("At [time] you wrote:");
		$replystring = str_replace ("[time]", $this->props["date"], $replystring );
		$smarty->assign("body", "\n\n".$replystring."\n".$this->props["body"]);
		$smarty->assign("action", "reply");
		$smarty->assign("sent_representing_smtp_address", $this->getSMTPAddressFromEntryID($this->props["sent_representing_entryid"]));
	}
	
	function getcontents()
	{
		global $smarty;
		parent::getcontents();
		
		$smarty->assign("sent_representing_smtp_address", $this->getSMTPAddressFromEntryID($this->props["sent_representing_entryid"]));
	}
	
	function getSMTPAddressFromEntryID($entryid) {
		global $mapi;
		
		$ab = mapi_openaddressbook($mapi->session);
		
		$mailuser = mapi_ab_openentry($ab, $entryid);
		
		if(!$mailuser)
			return "";
			
		$props = mapi_getprops($mailuser, array(PR_ADDRTYPE, PR_SMTP_ADDRESS, PR_EMAIL_ADDRESS));
		
		$addrtype = isset($props[PR_ADDRTYPE]) ? $props[PR_ADDRTYPE] : "SMTP";
		
		if(isset($props[PR_SMTP_ADDRESS]))
			return $props[PR_SMTP_ADDRESS];

		if($addrtype == "SMTP" && isset($props[PR_EMAIL_ADDRESS]))
			return $props[PR_EMAIL_ADDRESS];
			
		return "";
	}
}
?>
