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
class message {
	var $store;
	var $eid;
	var $properties;
	var $props;
	var $parent;
	
	function message($store, $eid)
	{
		$this->store = $store;
		$this->eid = $eid;
	}
	
	function getcontents()
	{
		
		global $smarty;
		
		
		$msg = mapi_msgstore_openentry($this->store, hex2bin($this->eid));
		$msg_props = mapi_getprops($msg, $this->properties);
		
		/**
		 * set message read
		 */
		$readstatus = $this->setMessageFlag($this->store, hex2bin($this->eid), "read", $msg_props);
		
		$parent = mapi_msgstore_openentry($this->store,$msg_props[PR_PARENT_ENTRYID]);
		$parent_props = mapi_getprops($parent, array(PR_DISPLAY_NAME, PR_CONTAINER_CLASS));
		

		$smarty->assign('parentname', $parent_props[PR_DISPLAY_NAME]);
		$parentclass = isset($parent_props[PR_CONTAINER_CLASS])? $parent_props[PR_CONTAINER_CLASS] : "IPF.Note";
		$smarty->assign('parentclass', $parentclass);
		
		// Workaround: PR_BODY may be returned as an error because the body is too large. We work
		// around this problem by re-getting the body as a stream and setting it in the properties

		if(isset($msg_props[mapi_prop_tag(PT_ERROR,mapi_prop_id(PR_BODY))])) {
			$bodyStream = mapi_openpropertytostream($msg, PR_BODY);
			$stat = mapi_stream_stat($bodyStream);
			$body = mapi_stream_read($bodyStream, $stat["cb"] > MAX_BODY_SIZE ? MAX_BODY_SIZE : $stat["cb"]);

			$msg_props[PR_BODY] = $body;
		}	

		foreach ($this->properties as $key => $property)
		{
			if (isset($msg_props[$property])){
				switch ($key)
				{
					case "body":
						$this->props[$key."f"] = $msg_props[$property];
						$this->props[$key] = $msg_props[$property];
						break;
					
					case "parent_entryid":
						$this->props[$key] = bin2hex($msg_props[$property]);
						break;
					
					case "entryid":
						$this->props[$key] = bin2hex($msg_props[$property]);
						break;
						
					case "message_delivery_time":
						if (date("y-m-d",$msg_props[$property])== date('y-m-d')){
							$this->props[$key] = date("H:i:s",$msg_props[$property]);
						}
						else {
							$this->props[$key] = date("d-m-Y",$msg_props[$property]);
						}
						$this->props["date"] = date("d-m-Y H:i:s",$msg_props[$property]);
						break;
					
					case "home_address":
						$this->props[$key."f"] = $msg_props[$property];
						$this->props[$key] = $msg_props[$property];
						break;
					
					case "other_address":
						$this->props[$key."f"] = $msg_props[$property];
						$this->props[$key] = $msg_props[$property];
						break;
					
					case "business_address":
						$this->props[$key."f"] = $msg_props[$property];
						$this->props[$key] = $msg_props[$property];
						break;	
						
					case "conversation_index":
						$this->props[$key] = bin2hex($msg_props[$property]);
						break;
					
						
					default:
						$this->props[$key] = $msg_props[$property];
						break;
				}
				
			}
		}
		foreach ($this->props as $key => $prop)
		{
			$smarty->assign($key, $prop);
		}
		
		if (isset($msg_props[PR_HASATTACH]))
		{
			$attachtable = mapi_message_getattachmenttable($msg);
			$attachments = mapi_table_queryallrows($attachtable, array(PR_ATTACH_LONG_FILENAME, PR_ATTACH_FILENAME, PR_DISPLAY_NAME, PR_ATTACH_SIZE, PR_ATTACH_NUM));
			$attachlist = array();
			foreach ($attachments as $key=>$attachment)
			{
				$attachprops = array();
				if (isset($attachment[PR_ATTACH_LONG_FILENAME])) {
					$attachprops["attach_name"]=$attachment[PR_ATTACH_LONG_FILENAME];
				}
				else if (isset($attachment[PR_ATTACH_FILENAME])) {
					$attachprops["attach_name"]=$attachment[PR_ATTACH_FILENAME];
				}
				else {
					$attachprops["attach_name"]=$attachment[PR_DISPLAY_NAME];
				}
				
				$attachprops["attach_size"]=round($attachment[PR_ATTACH_SIZE]/1000);
				$attachprops["attach_num"]=$attachment[PR_ATTACH_NUM];
				array_push($attachlist, $attachprops);
			}
			$smarty->assign("attachments", $attachlist);
		}
		
	}
	
	/**
	 * Code from Johnny with alight varaible name modification
	 * original code from operations.php
	 *
	 */
	function render()
	{
		$message_properties = $this->properties;
		foreach($_POST as $key => $value)
			{
				if(isset($message_properties[$key])) {
					$mapi_property = $message_properties[$key];

					switch(mapi_prop_type($mapi_property))
					{
						case PT_LONG:
							$this->props[$mapi_property] = (int) $value;
							break;
						case PT_DOUBLE:
							if(settype($value, "double")) {
								$this->props[$mapi_property] = $value;
							} else {
								$this->props[$mapi_property] = (float) $value;
							}
							break;
						case PT_BOOLEAN:
							if(!is_bool($value)) {
								$this->props[$mapi_property] = ($value=="false"||$value=="-1"?false:true);
							} else {
								$this->props[$mapi_property] = $value;
							}
							break;
						case PT_SYSTIME:
							$value = (int) $value;
							if($value > 0) {
								$this->props[$mapi_property] = $value;
							}
							break;
						case PT_MV_STRING8:
						case (PT_MV_STRING8 | MVI_FLAG):
							$mv_values = explode(";", $value);
							$values = array();
							
							foreach($mv_values as $mv_value)
							{
								if(!empty($mv_value)) {
									array_push($values, ltrim($mv_value));
								}
							}
							
							if(count($values) > 0) {
								$this->props[($mapi_property &~ MV_INSTANCE)] = $values;
							}
							break;
						default:
							switch($mapi_property)
							{
								case PR_ENTRYID:
								case PR_CONVERSATION_INDEX:
								case PR_PARENT_ENTRYID:
								case PR_STORE_ENTRYID:
									$this->props[$mapi_property] = hex2bin($value);
									break;
								default:
									$this->props[$mapi_property] = $value;
									break;
							}
							break;
					}
				} 
			}
		$this->props[PR_BODY]= get_magic_quotes_gpc()? stripslashes($this->props[PR_BODY]): $this->props[PR_BODY];	
		$this->props[PR_SUBJECT]= get_magic_quotes_gpc()? stripslashes($this->props[PR_SUBJECT]): $this->props[PR_SUBJECT];	
	}
	
	
	/**
	 * Creates new Message
	 * @author Johnny
	 * @param unknown_type $store
	 * @param unknown_type $parententryid
	 * @return unknown
	 */
	function createMessage($store, $parententryid)
	{
		$folder = mapi_msgstore_openentry($store, $parententryid); 
		return mapi_folder_createmessage($folder); 
	}
	
	/**
	 * Creates new Message
	 * @author Johnny
	 * @param unknown_type $store
	 * @param unknown_type $parententryid
	 * @return unknown
	 */
	
	function openMessage($store, $entryid)
	{
		return mapi_msgstore_openentry($store, $entryid); 
	}
	
	/**
	 * Saves Message
	 * @author Johnny
	 *
	 * @param unknown_type $store
	 * @param unknown_type $parententryid
	 * @param unknown_type $props
	 * @param unknown_type $recipients
	 * @param unknown_type $checknum
	 * @param unknown_type $messageProps
	 * @return unknown
	 */
	function saveMessage($store, $parententryid, $props, $recipients, $checknum, &$messageProps)
	{
		$message = false;

		if(isset($props[PR_ENTRYID]) && !empty($props[PR_ENTRYID])) {
			$message = $this->openMessage($store, $props[PR_ENTRYID]);
		} else {
			$message = $this->createMessage($store, $parententryid);
		}

		if($message) {
			mapi_setprops($message, $props);

			if (isset($props[PR_BODY])) {
				$body = $props[PR_BODY];
				$property = PR_BODY;
				$propertyToDelete = PR_RTF_COMPRESSED;
				
				if(isset($props[PR_RTF_IN_SYNC])) {
					$body = $this->fckEditor2compressedRTF($props[PR_BODY]);
					$property = PR_RTF_COMPRESSED;
					$propertyToDelete = PR_BODY;
				}
				
				mapi_deleteprops($message, Array($propertyToDelete));
				
				$stream = mapi_openpropertytostream($message, $property);
				mapi_stream_setsize($stream, strlen($body));
				mapi_stream_write($stream, $body);
				mapi_stream_commit($stream);
			} 
			
			if($recipients) {
				$this->setRecipients($message, $recipients);
			}
			
			mapi_savechanges($message);
			$messageProps = mapi_getprops($message, array(PR_ENTRYID, PR_PARENT_ENTRYID, PR_STORE_ENTRYID));
		}
		
		return $message;
	}
	
	/**
	 * Function which modifies the recipient table of a message
	 * @param object $message MAPI Message Object
	 * @param array $recipients array of recipients
	 */
	function setRecipients($message, $recipients)
	{
		$recipients = $this->createRecipientList($recipients);
		
		$recipientTable = mapi_message_getrecipienttable($message);
		$recipientRows = mapi_table_queryallrows($recipientTable, array(PR_ROWID));
		foreach($recipientRows as $recipient)
		{
			mapi_message_modifyrecipients($message, MODRECIP_REMOVE, array($recipient));
		} 
		
		mapi_message_modifyrecipients($message, MODRECIP_ADD, $recipients);
	}
	
	
	function createRecipientList($recipients)
	{
		$pattern = "/.*?([a-zA-Z0-9\._\-]+@[a-zA-Z0-9]+[a-zA-Z0-9-\.]*\.[a-zA-Z]{2,6})/";
		preg_match_all($pattern, $recipients, $out);
		$recipients=array();
		foreach ($out[1] as $key => $email)
		{
			$recipient = array();
			$recipient[PR_RECIPIENT_TYPE] = MAPI_TO;
			$recipient[PR_DISPLAY_NAME] = $email;
			$recipient[PR_EMAIL_ADDRESS] =$email;
			$recipient[PR_ADDRTYPE] = "SMTP";
			$recipient[PR_OBJECT_TYPE] = MAPI_MAILUSER;
			$recipient[PR_ENTRYID] = mapi_createoneoff($recipient[PR_DISPLAY_NAME], $recipient[PR_ADDRTYPE], $recipient[PR_EMAIL_ADDRESS]);
			array_push($recipients, $recipient);
		}
		return $recipients;
		
	}
	
	/**
	 * Function which sends a message
	 * @param object $store MAPI Message Store Object
	 * @param array $props the properties to be saved
	 * @param array $recipients array of recipients for the recipient table
	 * @param string $checknum unique check number which checks if attachments should be added ($_SESSION)
	 * @param array $messageProps reference to an array which will be filled with entryids		 		 	 		 
	 * @return boolean true if action succeeded, false if not
	 *
	 * @todo
	 * - save message in outbox before submitting, so message is not lost if submit fails
	 * - get new parent entryid of message		 		  
	 */
	function submitMessage($store, $props, $recipients, $checknum, &$messageProps)
	{
		$result = false;
		// Get the outbox and sent mail entryid
		$storeprops = mapi_getprops($store, array(PR_IPM_OUTBOX_ENTRYID, PR_IPM_SENTMAIL_ENTRYID));

		if(isset($storeprops[PR_IPM_OUTBOX_ENTRYID])) {
			if(isset($storeprops[PR_IPM_SENTMAIL_ENTRYID])) {
				$props[PR_SENTMAIL_ENTRYID] = $storeprops[PR_IPM_SENTMAIL_ENTRYID];
			}
			
			// Save the message
			$message = $this->saveMessage($store, $storeprops[PR_IPM_OUTBOX_ENTRYID], $props, $recipients, $checknum, $messageProps);
			
			if($message) {
				// Submit the message (send)
				mapi_message_submitmessage($message);
				$messageProps[PR_PARENT_ENTRYID] = $storeprops[PR_IPM_SENTMAIL_ENTRYID];
				$result = true;
			}
		}
		
		return $result;
	}
	
	/**
	 * @author johnny/michael
 	 * Function which sets the PR_MESSAGE_FLAG property of a message
	 * @param object $store MAPI Message Store Object
	 * @param string $entryid entryid of the message		 
	 * @param string $flag the flag "read" or "unread"		 
	 * @param array $messageProps reference to an array which will be filled with entryids
	 * @return boolean true if action succeeded, false if not		  
	 */
	function setMessageFlag($store, $entryid, $flag, &$messageProps)
	{
		$result = false;
		$message = $this->openMessage($store, $entryid);

		if($message) {
			switch($flag)
			{
				case "unread":
					$result = mapi_message_setreadflag($message, CLEAR_READ_FLAG);
					break;
				default:
					$result = mapi_message_setreadflag($message, MSGFLAG_READ|SUPPRESS_RECEIPT);
					break;
			}

			$messageProps = mapi_getprops($message, $this->properties);
		} 

		return $result;
	}
	
		/**
		 * @autor johhny/michael
		 * Function which copies or moves messages		 
		 * @param string $destentryid destination folder		 
		 * @param array $entryids a list of entryids which will be copied or moved
		 * @param boolean $moveMessages true - move messages, false - copy messages
		 * @return boolean true if action succeeded, false if not		  
		 */
		function copyMessages($destentryid, $action)
		{
			
			$msg = mapi_msgstore_openentry($this->store, hex2bin($this->eid));
			$msg_props = mapi_getprops($msg, array(PR_PARENT_ENTRYID));
			
			$this->parent = $msg_props[PR_PARENT_ENTRYID];
			
			$result = false;
			$sourcefolder = mapi_msgstore_openentry($this->store, $this->parent); 
			$destfolder = mapi_msgstore_openentry($this->store, hex2bin($destentryid)); 
			
		
			if ($action == "move"){
				$moveMessages =true;
			}
			else {
				$moveMessages =false;
			}
			
			if($moveMessages) {
				mapi_folder_copymessages($sourcefolder, array(hex2bin($this->eid)), $destfolder, MESSAGE_MOVE);
				$result = true;
			} else {
				mapi_folder_copymessages($sourcefolder, array(hex2bin($this->eid)), $destfolder);
				$result = true;
			}
			
			return $result;
		}
		
		function deleteMessages($hardDelete = false)
		{
			
			$msg = mapi_msgstore_openentry($this->store, hex2bin($this->eid));
			$msg_props = mapi_getprops($msg, array(PR_PARENT_ENTRYID));
			
			$this->parent = $msg_props[PR_PARENT_ENTRYID];
			
			$result = false;
			$folder = mapi_msgstore_openentry($this->store, $this->parent); 
			$msgprops = mapi_getprops($this->store, array(PR_IPM_WASTEBASKET_ENTRYID));
			
			if(isset($msgprops[PR_IPM_WASTEBASKET_ENTRYID])) {
				if($msgprops[PR_IPM_WASTEBASKET_ENTRYID] == $this->parent || $hardDelete) {
					$result = mapi_folder_deletemessages($folder, array(hex2bin($this->eid)), DELETE_HARD_DELETE);
				} else {
					$result = $this->copyMessages(bin2hex($msgprops[PR_IPM_WASTEBASKET_ENTRYID]), true);
				}
			}
			
			return $result;
		}
}
?>
