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
if (!isset($_POST["type"])) $_POST["type"] = "unknown";
switch ($_POST["type"])
{
	case "IPM.Appointment":
		switch($_GET["action"])
		{
			case "save":
				$message = new appointmentMessage($store->store, "");
				$message->save();
				break;
				
			case "new":
				$message = new appointmentMessage($store->store, "");
				$message->render();
				break;
				
			case "delete":
				/**
				 * @todo delete appointment
				 */
				break;
				
			default:
				/**
				 * @todo doSomething(tm);
				 */
				break;
		}
		break;
		
	case "IPM.Note":
		switch($_GET["action"])
		{
			case "save":
				/**
				 * @todo save mail
				 */
				break;
				
			case "new":
				$message = new mailMessage($store->store);
				$message->render();
				break;
				
			case "delete":
				/**
				 * @todo delete mail
				 */
				break;
				
			default:
				/**
				 * @todo doSomething(tm);
				 */
				break;
		}
		break;
		
	case "IPM.Contact":
		switch($_GET["action"])
		{
			case "save":
				$message = new contactMessage($store->store);
				$message->save();
				break;
				
			case "new":
				$message = new contactMessage($store->store);
				$message->render();
				break;
				
			case "delete":
				/**
				 * @todo delete contact
				 */
				break;
				
			default:
				/**
				 * @todo doSomething(tm);
				 */
				break;
		}
		break;
		
	case "IPM.Task":
		switch ($_GET["action"])
		{
			case "update":
				$message = new taskMessage($store->store);
				$message->update();
				break;
				
			case "save":
				$message = new taskMessage($store->store);
				$message->render();
				break;
				
			case "new":
				$message = new taskMessage($store->store);
				$message->render();
				break;
		}
		
	default:
		switch ($_GET["action"])
		{
			case "copy":
				$message = new message($store->store, $_GET["entryid"]);
				$message->copyMessages($_POST["tofolder"], $_POST["action"]);
				$_GET["entryid"] = bin2hex($message->parent);
				require_once("table.php");
				break;
				
			case "delete":
				if (isset($_POST["delete"]) && $_POST["delete"]=="yes"){
					$message = new message($store->store, $_GET["entryid"]);
					$message->deleteMessages();
					$_GET["entryid"] = bin2hex($message->parent);
					require_once("table.php");
				}
				else {
					$message = new message($store->store, $_GET["entryid"]);
					$msg = mapi_msgstore_openentry($store->store, hex2bin($_GET["entryid"]));
					$msg_props = mapi_getprops($msg, array(PR_PARENT_ENTRYID));
			
					$_GET["entryid"] = bin2hex($msg_props[PR_PARENT_ENTRYID]);
					require_once("table.php");
				}
				
				break;
		}
		break;
}
?>
