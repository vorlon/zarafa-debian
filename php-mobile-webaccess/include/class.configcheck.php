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
 * This class will check the server configuration and it stops the 
 * webaccess from working until this is fixed
 *
 *@author Michael Erkens <m.erkens@connectux.com>
 */

class ConfigCheck 
{
	var $result;

	function ConfigCheck()
	{
		$this->result = true;	

		// here we check our settings, changes to the config and
		// additonal checks must be added/changed here
		$this->checkPHP("4.3", "You must upgrade PHP");
		$this->checkExtension("mapi", "5.0-4688", "If you have upgraded zarafa, please restart Apache");
		$this->checkPHPsetting("register_globals", "off", "Modify this setting in '%s'");
		$this->checkPHPsetting("magic_quotes_gpc", "off", "Modify this setting in '%s'");
		$this->checkPHPsetting("magic_quotes_runtime", "off", "Modify this setting in '%s'");
		$this->checkPHPsetting("short_open_tag", "on", "Modify this setting in '%s'");
		$this->checkPHPsetting("session.auto_start", "off", "Modify this setting in '%s'");
		$this->checkPHPsetting("output_handler", "", "With this option set, it is unsure if the webaccess will work correctly");
		$this->checkPHPsetting("zlib.output_handler", "", "With this option set, it is unsure if the webaccess will work correctly");
		$this->checkPHPsetting("zlib.output_compression", "off", "With this option set, it could occure that XMLHTTP-requests will fail");
		$this->checkDirectory(SMARTY_CACHE_DIR, "rw", "Please make sure this directory exists and is writable for PHP/Apache");
		$this->checkDirectory(SMARTY_COMPILE_DIR, "rw", "Please make sure this directory exists and is writable for PHP/Apache");
		$this->checkFunction("iconv", "Install the 'iconv' module for PHP, or else you don't have euro-sign support.");
		$this->checkFunction("gzencode", "You don't have zlib support: <a href=\"http://php.net/manual/en/ref.zlib.php#zlib.installation\">http://php.net/manual/en/ref.zlib.php#zlib.installation</a>");

		// check if there were *any* errors, then break with E_USER_ERROR
		if (!$this->result){
			?>
				<p style="font-weight: bold;">Zarafa WebAccess can't start because of incompatible configuration.</p>
				<p>Please correct above errors, a good start is by checking your '<tt><?php echo $this->get_php_ini(); ?></tt>' file.</p>
				<p>Or if you wish, you can disable this config check by editing the file '<tt><?php echo dirname($_SERVER["SCRIPT_FILENAME"]) ?>/config.php</tt>', but this is not recommend.</p>
			<?php
			exit;
		}

	}
		

	/**
	 * This function throws all the errors, make sure that the check-function 
	 * will call this in case of an error.
	 */
	function error($string, $help)
	{
		printf("<div style=\"color: #f00;\">%s</div><div style=\"font-size: smaller; margin-left: 20px;\">%s</div>\n",$string, $help);
		$this->result = false;
	}

	/**
	 * See error()
	 */
	function error_version($name, $needed, $found, $help)
	{
		$this->error(sprintf("<strong>Version error:</strong> %s %s found, but %s needed.\n",$name, $needed, $found), $help);
	}

	/**
	 * See error()
	 */
	function error_notfound($name, $help)
	{
		$this->error(sprintf("<strong>Not Found:</strong> %s not found", $name), $help);
	}

	/**
	 * See error()
	 */
	function error_config($name, $needed, $help)
	{
		$help = sprintf($help, "<tt>".$this->get_php_ini()."</tt>");
		$this->error(sprintf("<strong>PHP Config error:</strong> %s must be '%s'", $name, $needed), $help);
	}

	/**
	 * See error()
	 */
	function error_directory($dir, $msg, $help)
	{
		$this->error(sprintf("<strong>Directory Error:</strong> %s %s", $dir, $msg), $help);
	}

	/**
	 * Retrieves the location of php.ini
	 */
	function get_php_ini()
	{
		if (isset($this->phpini)){
			return $this->phpini;
		}

		$result = "php.ini";
	
		ob_start();
		phpinfo(INFO_GENERAL);
		$phpinfo = ob_get_contents();
		ob_end_clean();

		preg_match("/<td class=\"v\">(.*php[45]?\.ini)/i", $phpinfo, $matches);
		if (isset($matches[0])){
			$result = $matches[0];
		}
		$this->phpini = $result;
		return $result;
	}

	/**********************************************************\
	*  Check functions                                         *
	\**********************************************************/


	/**
	 * Checks for the PHP version
	 */
	function checkPHP($version, $help_msg="")
	{
		$result = true;
		if (version_compare(phpversion(), $version) == -1){
			$this->error_version("PHP",phpversion(), $version, $help_msg);
			$result = false;
		}
		return $result;
	}

	/**
	 * Check if extension is loaded and if the version is what we need
	 */
	function checkExtension($name, $version="", $help_msg="")
	{
		$result = true;
		if (extension_loaded($name)){
			if (version_compare(phpversion($name), $version) == -1){
				$this->error_version("PHP ".$name." extension",phpversion($name), $version, $help_msg);
				$result = false;
			}
		}else{
			$this->error_notfound("PHP ".$name." extension", $help_msg);
			$result = false;
		}
		return $result;
	}

	/**
	 * Check if a function exists
	 */
	function checkFunction($name, $help_msg="")
	{
		$result = true;
		if (!function_exists($name)){
			$this->error_notfound("PHP function '".$name."' ", $help_msg);
			$result = false;
		}
		return $result;
	}
	
	/**
	 * This function checks if a specific php setting (php.ini) is set to a 
	 * value we need, for example register_globals
	 */
	function checkPHPsetting($setting,$value_needed, $help_msg=""){
		$result = true;
		// convert $value_needed
		switch($value_needed){
			case "on":
			case "yes":
			case "true":
				$value = 1;
				break;

			case "off":
			case "no":
			case "false":
				$value = 0;
				break;

			default:
				$value = $value_needed;
		}

		if (ini_get($setting) != $value){
			$this->error_config($setting, $value_needed, $help_msg);
			$result = false;
		}
		return $result;
	}

	/**
	 * This functions checks if a directory exists and if requested also if
	 * this directory is readable/writable specified with the $states parameter
	 *
	 * $states is a string which can contain these chars:
	 *      r  - check if directory is readable
	 *      w  - check if directory is writable
	 */
	function checkDirectory($dir, $states="r", $help_msg="")
	{
		$result = true;
		if (is_dir($dir)){
			$states = strtolower($states);
			if (strpos($states,"r")!==FALSE){
				if (!is_readable($dir)){
					$this->error_directory($dir, "isn't readable", $help_msg);
					$result = false;
				}
			}
			if (strpos($states,"w")!==FALSE){
				if (!is_writable($dir)){
					$this->error_directory($dir, "isn't writable", $help_msg);
					$result = false;
				}
			}
		}else{
			$this->error_directory($dir, "doesn't exists", $help_msg);
			$result = false;
		}
		return $result;
	}
}

?>
