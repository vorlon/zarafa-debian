<?php
/*
 * Copyright 2005 - 2015  Zarafa B.V. and its licensors
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License, version 3,
 * as published by the Free Software Foundation with the following
 * additional terms according to sec. 7:
 * 
 * "Zarafa" is a registered trademark of Zarafa B.V.
 * The licensing of the Program under the AGPL does not imply a trademark 
 * license. Therefore any rights, title and interest in our trademarks 
 * remain entirely with us.
 * 
 * Our trademark policy, <http://www.zarafa.com/zarafa-trademark-policy>,
 * allows you to use our trademarks in connection with Propagation and 
 * certain other acts regarding the Program. In any case, if you propagate 
 * an unmodified version of the Program you are allowed to use the term 
 * "Zarafa" to indicate that you distribute the Program. Furthermore you 
 * may use our trademarks where it is necessary to indicate the intended 
 * purpose of a product or service provided you use it in accordance with 
 * honest business practices. For questions please contact Zarafa at 
 * trademark@zarafa.com.
 *
 * The interactive user interface of the software displays an attribution 
 * notice containing the term "Zarafa" and/or the logo of Zarafa. 
 * Interactive user interfaces of unmodified and modified versions must 
 * display Appropriate Legal Notices according to sec. 5 of the GNU Affero 
 * General Public License, version 3, when you propagate unmodified or 
 * modified versions of the Program. In accordance with sec. 7 b) of the GNU 
 * Affero General Public License, version 3, these Appropriate Legal Notices 
 * must retain the logo of Zarafa or display the words "Initial Development 
 * by Zarafa" if the display of the logo is not reasonably feasible for
 * technical reasons.
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

define("SETTINGS_TABDIR", "client/layout/dialogs/settings/tabs");
require("client/layout/tabbar.class.php");

function initWindow(){
	global $tabbar, $tabs;

	$tab_files = array();
	$dh = opendir(SETTINGS_TABDIR);
	while($file = readdir($dh)){
		if (substr($file,0,1)!='.' && substr($file,0,1)!="_" && substr($file,-4)=='.php'){
			include(SETTINGS_TABDIR.'/'.$file);
			$name = substr($file,0,-4);
			if (function_exists($name."_settings_order")){
				$order = call_user_func($name."_settings_order");
			}else{
				$order = count($tab_files);
			}
			
			while (isset($tab_files[$order])){
				$order++; // make sure every tabpage is visible
			}
			
			$tab_files[$order] = $name;
		}
	}
	ksort($tab_files);	
	$tabs = array();
	foreach($tab_files as $k=>$v){
		if (function_exists($v."_settings_title")){
			$tabs[$v] = call_user_func($v."_settings_title");
		}else{
			$tabs[$v] = $v;
		}
	}
	reset($tabs);
	$tabbar = new TabBar($tabs, key($tabs));
}


function getIncludes(){
	global $tabs;
	$files = array(
					"client/layout/css/tabbar.css",
					"client/layout/js/tabbar.js",
					"client/layout/css/settings.css"
			);
	foreach($tabs as $k=>$v){
		$files[] = SETTINGS_TABDIR."/".$k.".js";
	}
	return $files;
}

function getDialogTitle(){
	return _("Settings");
}

function getJavaScript_onresize(){ 
	?>
		resizeBody();
	<?
} // getJavaSctipt_onresize	

function getJavaScript_onload(){ 
	global $tabbar, $tabs;
	
	$tabbar->initJavascript("tabbar", "\t\t\t\t\t");
	
	echo "\t\t\t\t\ttabbar.change(parentWebclient.settings.get(\"global/last_settings_tab\", tabbar.getSelectedTab()));\n\n";
	echo "\t\t\t\t\twebclient.tabbar = tabbar;\n";

	foreach($tabs as $name=>$title){
		echo "\t\t\t\t\t".$name."_loadSettings(parentWebclient.settings);\n";
	}
	?>
			webclient.menu.showMenu();
			resizeBody();
	<?
} // getJavaSctipt_onload	

function getJavaScript_other(){
	global $tabs;
?>	
			var reloadNeeded = false;

			function save_settings(){
				var settings = parentWebclient.settings;
<?	foreach($tabs as $name=>$title){
?>				<?=$name?>_saveSettings(settings);
<?	} ?>
				settings.set("global/last_settings_tab", webclient.tabbar.getSelectedTab());
				
				if (reloadNeeded){
					parentWebclient.requestReload();
				}

				window.close();
			}

			function close_settings(){
				var settings = parentWebclient.settings;
				settings.set("global/last_settings_tab", webclient.tabbar.getSelectedTab());
				window.close();
			}

			function resizeBody() {
				// Get preferred tab height.
				var titleElement = dhtml.getElementById("windowtitle");
				var menubarElement = dhtml.getElementById("menubar");
				var tabbarElement = dhtml.getElementById("tabbar");
				var tabHeight = document.documentElement.offsetHeight - titleElement.offsetHeight - menubarElement.offsetHeight - tabbarElement.offsetHeight - 30;

			// Set height of all the tabs to make scrolling work.
			<?	foreach($tabs as $name=>$title){
			?>				
				var tabElement = dhtml.getElementById('<?=$name?>_tab');
				tabElement.style.height = tabHeight + 'px';
			<?	} ?>
			}
<?	
}


function getMenuButtons(){
	return array(
			"save"=>array(
				'id'=>"save",
				'name'=>_("Save"),
				'title'=>_("Save settings"),
				'callback'=>'save_settings'
			),
			
			"cancel"=>array(
				'id'=>'close',
				'name'=>_("Close"),
				'title'=>_("Don't save settings"),
				'callback'=>'close_settings'
			),
			
		);
}

function getBody() {
	global $tabbar, $tabs;

	$tabbar->createTabs(); 

	foreach($tabs as $name=>$title){
		$tabbar->beginTab($name);
		
		if (function_exists($name."_settings_html")){
			call_user_func($name."_settings_html");
		}else{
			echo "<p>tabpage \"".$name."\" not found</p>";
		}
		$tabbar->endTab();
	}
} // getBody


?>
