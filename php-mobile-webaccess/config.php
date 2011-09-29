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
/* 
	config.php
	
	The config file for the mobile webaccess.
*/

	// Set FALSE disable the config check
	define("CONFIG_CHECK", true);

	// Default Zarafa server to connect to.
	define("SERVER", "file:///var/run/zarafa");
	#define("SERVER", "http://localhost:236/zarafa");

	// Defines the base path on the server, terminated by a slash
	define('BASE_PATH', dirname($_SERVER['SCRIPT_FILENAME']) . "/");

	// Define the include paths
	set_include_path(BASE_PATH."include/" . PATH_SEPARATOR . 
					 BASE_PATH . PATH_SEPARATOR . 
					 "." . PATH_SEPARATOR . 
					 "/usr/share/php/");

	define("COOKIE_EXPIRE", 60*60*24*365); // one year

	/**************************************\
	* Template Settings / Smarty           *
	\**************************************/


	// smarty
	define("SMARTY_TEMPLATE_DIR", "render/html");
	define("SMARTY_CONFIG_DIR", "/var/lib/zarafa-webaccess-mobile/config");
	define("SMARTY_CACHE_DIR",	"/var/lib/zarafa-webaccess-mobile/cache"); // must be writable
	define("SMARTY_COMPILE_DIR", "/var/lib/zarafa-webaccess-mobile/templates_c"); // must be writable

	// renderer type (currently only "html")
	$config["renderer"]="html";
	
	// textmessages for when the subject or the sender not exists
	// needed to click on the item
	define('NOSENDERMSG', _("No sender"));
	define('NOSUBJECTMSG', _("No subject"));

	define('CELLSHOUR', 1);
	define('SCREENWIDTH', 240);
	
	// number of items per page
	$GLOBALS["emailsonpage"]=10;
	$GLOBALS["appointmentsonpage"]=10;
	$GLOBALS["contactsonpage"]=1000;
	$GLOBALS["tasksonpage"]=100;

	/**************************************\
	* Memory usage and timeouts            *
	\**************************************/
	
	// The maximum POST limit. To upload large files, this value must be larger than upload_max_filesize.
	ini_set('post_max_size', "31M");
	ini_set('upload_max_filesize', "30M");

	// This sets the maximum amount of memory in bytes that we are allowed to allocate. 
	ini_set('memory_limit', -1);  // -1 = no limit

	// This sets the maximum time in seconds that is allowed to run before it is terminated by the parser.	
	ini_set('max_execution_time', 300); // 5 minutes

	// BLOCK_SIZE (in bytes) is used for attachments by mapi_stream_read/mapi_stream_write
	define('BLOCK_SIZE', 1048576);

	// Maximum body size that will be sent to the client (100k by default)
	define('MAX_BODY_SIZE', 100 * 1024);	

	// Debugging
	$debug = false;

	if ($debug){
		error_reporting(E_ALL);
		ini_set("display_errors", true);
		if (file_exists("debug.php")){
			include ("debug.php");
		}else{
			// define empty dump function in case we still use it somewhere
			function dump(){}
		}	
	}else{
		ini_set("display_errors", false);
	}

?>
