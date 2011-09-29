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

class contactMessage extends message 
{
	
	function contactMessage ($store, $eid="")
	{
		$this->store=$store;
		$this->eid=$eid;

		
		$guid = makeguid("{00062004-0000-0000-C000-000000000046}");
		$guid2 = makeguid("{00062008-0000-0000-C000-000000000046}");
		$guid3 = makeguid("{00020329-0000-0000-C000-000000000046}");
		$names = mapi_getIdsFromNames($this->store, array(0x8005, 0x801B, 0x8083, 0x8080, 
														  0x8093, 0x8090, 0x80A3, 0x80A0, 
														  0x801A, 0x801C, 0x8022, 0x8062, 
														  0x802B, 0x8085, 0x8095, 0x80A5, 
														  0x8028, 0x8029, 0x8054, 0x8055,
														  0x8506, 0x853A, 0x8586, "Keywords"), 
													array($guid, $guid, $guid, $guid, 
														  $guid, $guid, $guid, $guid, 
														  $guid, $guid, $guid, $guid, 
														  $guid, $guid, $guid, $guid, 
														  $guid, $guid, $guid, $guid,
														  $guid2, $guid2, $guid2, $guid3));

		$this->properties = array();
		$this->properties["entryid"] = PR_ENTRYID;
		$this->properties["parent_entryid"] = PR_PARENT_ENTRYID;
		$this->properties["icon_index"] = PR_ICON_INDEX;
		$this->properties["message_class"] = PR_MESSAGE_CLASS;
		$this->properties["display_name"] = PR_DISPLAY_NAME;
		$this->properties["given_name"] = PR_GIVEN_NAME;
		$this->properties["middle_name"] = PR_MIDDLE_NAME;
		$this->properties["surname"] = PR_SURNAME;
		$this->properties["home_telephone_number"] = PR_HOME_TELEPHONE_NUMBER;
		$this->properties["cellular_telephone_number"] = PR_CELLULAR_TELEPHONE_NUMBER;
		$this->properties["office_telephone_number"] = PR_OFFICE_TELEPHONE_NUMBER;
		$this->properties["business_fax_number"] = PR_BUSINESS_FAX_NUMBER;
		$this->properties["company_name"] = PR_COMPANY_NAME;
		$this->properties["title"] = PR_TITLE;
		$this->properties["department_name"] = PR_DEPARTMENT_NAME;
		$this->properties["office_location"] = PR_OFFICE_LOCATION;
		$this->properties["profession"] = PR_PROFESSION;
		$this->properties["manager_name"] = PR_MANAGER_NAME;
		$this->properties["assistant"] = PR_ASSISTANT;
		$this->properties["nickname"] = PR_NICKNAME;
		$this->properties["display_name_prefix"] = PR_DISPLAY_NAME_PREFIX;
		$this->properties["spouse_name"] = PR_SPOUSE_NAME;
		$this->properties["generation"] = PR_GENERATION;
		$this->properties["birthday"] = PR_BIRTHDAY;
		$this->properties["wedding_anniversary"] = PR_WEDDING_ANNIVERSARY;
		$this->properties["sensitivity"] = PR_SENSITIVITY;
		$this->properties["fileas"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[0]));
		$this->properties["business_address"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[1]));
		$this->properties["email_address_1"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[2]));
		$this->properties["email_address_display_name_1"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[3]));
		$this->properties["email_address_2"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[4]));
		$this->properties["email_address_display_name_2"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[5]));
		$this->properties["email_address_3"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[6]));
		$this->properties["email_address_display_name_3"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[7]));
		$this->properties["home_address"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[8]));
		$this->properties["other_adress"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[9]));
		$this->properties["mailing_address"] = mapi_prop_tag(PT_LONG, mapi_prop_id($names[10]));
		$this->properties["im"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[11]));
		$this->properties["webpage"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[12]));
		$this->properties["email_address_entryid_1"] = mapi_prop_tag(PT_BINARY, mapi_prop_id($names[13]));
		$this->properties["email_address_entryid_2"] = mapi_prop_tag(PT_BINARY, mapi_prop_id($names[14]));
		$this->properties["email_address_entryid_3"] = mapi_prop_tag(PT_BINARY, mapi_prop_id($names[15]));
		$this->properties["address_book_mv"] = mapi_prop_tag(PT_MV_LONG, mapi_prop_id($names[16]));
		$this->properties["address_book_long"] = mapi_prop_tag(PT_LONG, mapi_prop_id($names[17]));
		$this->properties["oneoff_members"] = mapi_prop_tag(PT_MV_BINARY, mapi_prop_id($names[18]));
		$this->properties["members"] = mapi_prop_tag(PT_MV_BINARY, mapi_prop_id($names[19]));
		$this->properties["private"] = mapi_prop_tag(PT_BOOLEAN, mapi_prop_id($names[20]));
		$this->properties["contacts"] = mapi_prop_tag(PT_MV_STRING8, mapi_prop_id($names[21]));
		$this->properties["contacts_string"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[22]));
		$this->properties["categories"] = mapi_prop_tag(PT_MV_STRING8, mapi_prop_id($names[23]));
		
	}
	
	function render()
	{
		global $smarty;
		$msgstore_props = mapi_getprops($this->store);
		$subtree = mapi_msgstore_openentry($this->store, $msgstore_props[PR_IPM_SUBTREE_ENTRYID]);
		
		$subtreeprops = mapi_getprops($subtree, array(PR_PARENT_ENTRYID));
		
		$parententry = mapi_msgstore_openentry($this->store,$subtreeprops[PR_PARENT_ENTRYID]);
		
		$parentprops = mapi_getprops($parententry, array(PR_IPM_CONTACT_ENTRYID));
		$parent = $parentprops[PR_IPM_CONTACT_ENTRYID];
		
		
		
		$this->props = array();
		$message = parent::render();
		
		
		$patternkomma = "/(([a-zA-Z0-9 \-.]*),([a-zA-Z0-9 \-.]*))/";
		$patterntussen = "/(([a-zA-Z0-9\-.]*) (([a-zA-Z0-9 \-.])*) ([a-zA-Z0-9\-.]*))/";
		$patternnorm = "/(([a-zA-Z0-9\-.]*) ([a-zA-Z0-9\-.]*))/";
		
		if (preg_match($patternkomma, $_POST["display_name"], $matches)) {
			$this->props[$this->properties["given_name"]]= ucfirst(trim($matches[3]));
			$this->props[$this->properties["surname"]]= ucfirst(trim($matches[2]));
			$this->props[$this->properties["fileas"]]= ucfirst(trim($matches[2])).", ".ucfirst(trim($matches[3]));
			$this->props[$this->properties["display_name"]]= ucfirst(trim($matches[3]))." ".ucfirst(trim($matches[2]));
		}
		else if (preg_match($patterntussen, $_POST["display_name"], $matches)) {
			$this->props[$this->properties["given_name"]]= ucfirst(trim($matches[2]));
			$this->props[$this->properties["middle_name"]]= trim($matches[3]);
			$this->props[$this->properties["surname"]]= ucfirst(trim($matches[5]));
			$this->props[$this->properties["fileas"]]= ucfirst(trim($matches[5])).", ".ucfirst(trim($matches[2]))." ".trim($matches[3]);
			$this->props[$this->properties["display_name"]]= ucfirst(trim($matches[2]))." ".trim($matches[3])." ".ucfirst(trim($matches[5]));

		}
		
		else if (preg_match($patternnorm, $_POST["display_name"], $matches)) {
			$this->props[$this->properties["given_name"]]= ucfirst(trim($matches[2]));
			$this->props[$this->properties["surname"]]= ucfirst(trim($matches[3]));
			$this->props[$this->properties["fileas"]]= ucfirst(trim($matches[3])).", ".ucfirst(trim($matches[2]));
			$this->props[$this->properties["display_name"]]= ucfirst(trim($matches[2]))." ".ucfirst(trim($matches[3]));
		}
		else {
			$this->props[$this->properties["fileas"]]= ucfirst(trim($_POST["display_name"]));
			$this->props[$this->properties["display_name"]]= ucfirst(trim($_POST["display_name"]));
		}
		$this->props[PR_MESSAGE_CLASS]="IPM.Contact";
		
		$contact = $this->saveMessage($this->store,$parent, $this->props, array(), "", $output);
		header("Location: index.php?entryid=".bin2hex($output[PR_ENTRYID])."&type=msg&task=IPM.Contact");
	}
	
	function save()
	{
		/**
		 * @todo: use parentid from entry not default contact folder
		 */
		global $smarty;
		$msgstore_props = mapi_getprops($this->store);
		$subtree = mapi_msgstore_openentry($this->store, $msgstore_props[PR_IPM_SUBTREE_ENTRYID]);
		
		$subtreeprops = mapi_getprops($subtree, array(PR_PARENT_ENTRYID));
		
		$parententry = mapi_msgstore_openentry($this->store,$subtreeprops[PR_PARENT_ENTRYID]);
		
		$parentprops = mapi_getprops($parententry, array(PR_IPM_CONTACT_ENTRYID));
		$parent = $parentprops[PR_IPM_CONTACT_ENTRYID];
		
		
		
		$this->props = array();
		$message = parent::render();
		$contact = $this->saveMessage($this->store,$parent, $this->props, array(), "", $output);
		header("Location: index.php?entryid=".bin2hex($output[PR_ENTRYID])."&type=msg&task=IPM.Contact");
	}
}
?>
