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
 * 
 * @package mobilewebaccess
 * @author Mans Matulewicz
 * @copyright 2006
 */
session_start();

require_once("config.php");

// check if config is correct
if (CONFIG_CHECK){
	include("class.configcheck.php");
	new ConfigCheck();
}

// gzip support
ob_start("ob_gzhandler");



require_once("mapi.php");
require_once('smarty/Smarty.class.php');
require_once("include/login.php");
require_once("include/functions.php");

require_once("include/class.message.php");
require_once("include/class.hierarchy.php");
require_once("include/class.table.php");
require_once("include/class.appointmenttable.php");
require_once("include/class.appointmentmessage.php");
require_once("include/class.contactmessage.php");
require_once("include/class.contacttable.php");
require_once("include/class.mailtable.php");
require_once("include/class.mailmessage.php");
require_once("include/class.tasktable.php");
require_once("include/class.taskmessage.php");


$smarty =  new Smarty;

// register smarty template functions
require_once("include/smarty.includes.php");

$mapi = new Mapi();

$authenticated = login(); 

header("Content-Type: text/html; charset=windows-1252");

if ($authenticated) {	
	
	$store = new hierarchy($mapi->getDefaultMessageStore());
	if ((!isSet($_GET["entryid"])) && (!isSet($_GET["type"]))) {
		$store->getcontents();
		$smarty->display('hierarchy.tpl');
	}
	else {
		$_GET["type"] = isset($_GET["type"]) ? $_GET["type"] : "";
		switch ($_GET["type"])
		{
			case "table":
				require_once("table.php");
				break;
				
			case "msg":
				require_once("msg.php");
				break;
				
			case "edit":
				require_once("edit.php");
				break;
			
			case "new":
				require_once("new.php");
				break;
			
			case "action":
				require_once("action.php");
				break;
				
			case "reply":
				require_once("reply.php");
				break;
			
			case "forward":
				require_once("forward.php");
				break;
				
			case "copy":
				require_once("copy.php");
				break;
				
			case "delete":
				require_once("delete.php");
				break;
					
			case "logout":
				session_destroy(); 
				session_start();
				if (isset($_COOKIE) && isset($_COOKIE["mobile_password"])){
					setcookie("mobile_password", false, mktime(0,0,0,1,1,0));
				}
				$authenticated = false;
				break;
			
			case "attachment":
				require_once("include/attachment.php");
				break;
			
			default:
				$store->getcontents();
				$smarty->display('hierarchy.tpl');
				break;
		}
	}
}

if (!$authenticated){
	if (isset($_COOKIE["mobile_username"])){
		$smarty->assign("username", $_COOKIE["mobile_username"]);
	}
	$smarty->display('login.tpl');
}

ob_flush();
?>
