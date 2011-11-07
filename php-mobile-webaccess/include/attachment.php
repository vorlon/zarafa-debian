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
$message = mapi_msgstore_openentry($store->store, hex2bin($_GET["entryid"]));
if($message) {
	// Get number of attachment which should be opened.
	$attachNum = false;
	if(isset($_GET["attachnum"])) {
		$attachNum = $_GET["attachnum"];
	}
	if($attachNum) {
		// Loop through the attachNums, message in message in message ...
		for($i = 0; $i < (count($attachNum) - 1); $i++)
		{
			// Open the attachment
			$tempattach = mapi_message_openattach($message, (int) $attachNum[$i]);
				if($tempattach) {
					// Open the object in the attachment
				$message = mapi_attach_openobj($tempattach);
			}
		}
		
		// Open the attachment
		$attachment = mapi_message_openattach($message, (int) $attachNum[(count($attachNum) - 1)]);
	}
	
	// Check if the attachment is opened
	if($attachment) {
		// Get the props of the attachment
		$props = mapi_attach_getprops($attachment, array(PR_ATTACH_LONG_FILENAME, PR_ATTACH_MIME_TAG, PR_DISPLAY_NAME, PR_ATTACH_METHOD));
		// Content Type
		$contentType = "application/octet-stream";
		// Filename
		$filename = "ERROR";
		
		// Set filename						
		if(isset($props[PR_ATTACH_LONG_FILENAME])) {
			$filename = $props[PR_ATTACH_LONG_FILENAME];
		} else if(isset($props[PR_ATTACH_FILENAME])) {
			$filename = $props[PR_ATTACH_FILENAME];
		} else if(isset($props[PR_DISPLAY_NAME])) {
			$filename = $props[PR_DISPLAY_NAME];
		} 

		// Set content type
		if(isset($props[PR_ATTACH_MIME_TAG])) {
			$contentType = $props[PR_ATTACH_MIME_TAG];
		} else {
			// Parse the extension of the filename to get the content type
			if(strrpos($filename, ".") !== false) {
				$extension = strtolower(substr($filename, strrpos($filename, ".")));
				$contentType = "application/octet-stream";
				if (is_readable("include/mimetypes.dat")){
					$fh = fopen("include/mimetypes.dat","r");
					$ext_found = false;
					while (!feof($fh) && !$ext_found){
						$line = fgets($fh);
						preg_match("/(\.[a-z0-9]+)[ \t]+([^ \t\n\r]*)/i", $line, $result);
						if ($extension == $result[1]){
							$ext_found = true;
							$contentType = $result[2];
						}
					}
					fclose($fh);
				}
			}
		}
		$openType="attachment";
		// Set the headers
		header("Pragma: public");
		header("Expires: 0"); // set expiration time
		header("Cache-Control: must-revalidate, post-check=0, pre-check=0");
		header("Content-Disposition: ". $openType ."; filename=\"" . $filename . "\"");
		header("Content-Transfer-Encoding: binary");
		
		// Set content type header
		if($openType == "attachment") {
			header("Content-Type: application/octet-stream");
		} else {
			header("Content-Type: " . $contentType);
		}
		//header("Content-Type: " . $contentType);
		// Open a stream to get the attachment data
		$stream = mapi_openpropertytostream($attachment, PR_ATTACH_DATA_BIN);
		$stat = mapi_stream_stat($stream);
		// File length
		header("Content-Length: " . $stat["cb"]);
		for($i = 0; $i < $stat["cb"]; $i += BLOCK_SIZE) {
			// Print stream
			echo mapi_stream_read($stream, BLOCK_SIZE);
		}
	} 
}
?>
