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
/**
 * for all contact handling
 * 
 * @package php-mobile-webaccess
 * @author Mans Matulewicz
 * @version 0.01
 * @todo new contact
 * @todo modify contact
 * 
 *
 */
class contactTable extends table {
	
	var $contacts;
	
	function contactTable($store, $eid)
	{
		$this->store=$store;
		$this->eid=$eid;
		$this->contacts = array();
		$this->amountitems = $GLOBALS["contactsonpage"];
		
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
		$this->properties["display_name"] = PR_DISPLAY_NAME;
		$this->properties["given_name"] = PR_GIVEN_NAME;
		$this->properties["middle_name"] = PR_MIDDLE_NAME;
		$this->properties["surname"] = PR_SURNAME;
		$this->properties["email_address_1"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[2]));
		$this->properties["fileas"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[0]));
		$this->properties["cellular_telephone_number"] = PR_CELLULAR_TELEPHONE_NUMBER;
		
	}
	
	
	function getcontents()
	{
		
		global $smarty;
		$this->sort = array();
		$this->sort[PR_SURNAME]=TABLE_SORT_ASCEND;
		$this->sort[PR_GIVEN_NAME]=TABLE_SORT_ASCEND;
		parent::getcontents();
		parent::get_parent();
	}
}
?>