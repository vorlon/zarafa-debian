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
include "mapi/mapi.util.php";
include "mapi/mapicode.php";
include "mapi/mapidefs.php";
include "mapi/mapitags.php";
include "mapi/class.recurrence.php";

// DEFAULT_SERVER is set in php-webclient/config.php, but if not, default is set here
if (!defined('DEFAULT_SERVER')) define('DEFAULT_SERVER','http://localhost:236/zarafa');

/**
* @todo Make caching for the named properties, store in the session
* @todo Caching for the stores list
* @author Robin van Duiven <r.v.duiven@aqua-it.com>
* @version 0.01
* @copyright R. v. Duiven 2004
* @package mapi
*/

class Mapi {
	
	/**
	* @var resource MAPI_Session
	*/
	var $session = false;
	
	/**
	* @var string Username
	*/
	var $username;
	
	/**
	* @var string Password
	*/
	var $password;
	
	/**
	* @var Array List of stores
	*/
	var $storeslist;
	
  /**
	* @var resource MAPI_Store
	*/
	var $defaultstore = null;
	
   /**
	* Constructor of the Mapi class
	*
	* Constructor of the Mapi class. Won't need any argument and has empty body.
	*
	* @return void
	*/
	function Mapi() {
		//Empty constructor
	}

	/**
	* Logon to the MAPI 'provider'
	*
	* Logon to the MAPI 'provider'. Uses default profile when no name and pass are
	* given; Zarafa when this isn't the case.
	* On error it will call the error function.
	*
	* @param String username
	* @param String password
	* @return void
	*/
	function logon($name = '', $passwd = '', $server = DEFAULT_SERVER) 	{
        $result = false;
        // No name and pass; log on locally.
        if(!$this->session = mapi_logon_zarafa($name, $passwd, $server))
        {
            //$this->error(1);
        } else
        {
            // Logged on so set username (local).
            $this->username = "local";
            // Put all available stores in list
            $storetable = mapi_getmsgstorestable($this->session);
            $storerows = mapi_table_queryallrows($storetable);
            foreach ($storerows as $row)
            {
                $tempstore = mapi_openmsgstore($this->session, $row[PR_ENTRYID]);
                $this->storeslist[] = $tempstore;
                if (array_key_exists(PR_DEFAULT_STORE, $row)&&($row[PR_DEFAULT_STORE]==true))
                    $this->defaultstore = $tempstore;
            }
            $result = $this->storeslist;
        }
		// Return an array of store objects.
		return $result;
	}
	
	/**
	* Function to handle error codes from different functions in this MAPI class
	*
	* Codes:<br>
	* 1 - Logon to MAPI failed<br>
	* 2 - Undefined/Not yet implemented<br>
	*
	* @param int ErrorCode
	* @return void
	* @todo Extend with the correct MAPI errorcodes (messages will be read from the language file)
	*
	*/
	function error($errorCode) {
		switch ($errorCode) {
			case 1:
				echo "Error code: 1. Logon to MAPI failed";
				break;
			case 2:
				echo "Error code: 2. Not implemented yet.";
				break;
		}
	}

   /**
	* Function to return a Array of MAPI Message Stores
	* 
	* @return Array PHP_array
	* @todo Should we implement error handling here? i.e.: Someone calls this function BEFORE loging on.
	*/
	function getAllMessageStores(){
		return $this->storeslist;
	}
	
	
   /**
	* Function to get the default messagestore. 
	* @todo check if this really works for all versions of mapi/Outlook. 
	* @return Resource messagestore
	*/
	function getDefaultMessageStore()
	{
		$result = false;
		if (empty($this->defaultstore))
		{
			$result = $this->storeslist[0];
		} else
		{
			$result = $this->defaultstore;
		}
		return $result;
	}
	
	
	/**
	* Check whether a property returns an error and if so, which
	* @param long $property Property to check for error
	* @param Array $proparray An array of properties
	* @return mixed Gives back false when there is no error, if there is, gives the error
	*/
	function propIsError($property, $proparray)
	{
	   if (array_key_exists(mapi_prop_tag(PT_ERROR, mapi_prop_id($property)),$proparray)) 
	   {
	     return $proparray[mapi_prop_tag(PT_ERROR, mapi_prop_id($property))];
	   } else {
	     return false;
	   }
	}
	
  /**
	* Open a specified MAPI Message Store
	* 
	* Returns
	*
	* @param String EntryID
	* @return resource MAPI_Message_Store
	*/
	function openMessageStore($entryID)
	{
		$tempstore = false;
		foreach ($this->storeslist as $store)
		{
			$storeProps = mapi_getprops($store, Array(PR_ENTRYID));
			if (strcmp($storeProps[PR_ENTRYID], $entryID)==0)
			  $tempstore = $store;
		}
		return($tempstore);
	}
	
	/**
	* Opens a messagstore of another user. If success the messastore is added 
	* to the storelist.
	*
	* @param String username
	* @return Resource MAPI_Message_Store
	*/
	function openMessageStoreOther($user)
	{
		$store = false;
	
	  if ($user_entryid = mapi_msgstore_createentryid($this->defaultstore, $user)) {
  	  if ($store = mapi_openmsgstore_zarafa_other($user_entryid, $this->username, $this->password, DEFAULT_SERVER)) {
    	  $this->storeslist[] = $store;
    	}
  	}
  	
  	return $store;
	}
	
	/**
	* Function which parses a string to message recipients.
	* This is more of a util than a MAPI helper function. Doesn't validate.
	* @author Robbert Monna
	* @version 0.1
	* @param string $inputText The string from which to create recipient array
  * @param integer $rType An optional PR_RECIPIENT_TYPE
	* @return array() Returns a table with message recipients, false if no valid recipients found
	*/
	function createRecipientList($inputText, $rType="")
	{
	  $parsedRcpts = Array();
	  // Data is separated by , or ; ; so we split by this first.
	  $splitArray = preg_split("/[;]/", $inputText);
	  $errors = 0;
	  for ($i=0; $i<count($splitArray); $i++)
	  {
			if($splitArray[$i])
			{
		    $rcptMatches = Array();
		    // Check format of the address
		    if(preg_match('/([^<]*)<([^>]*)>/', $splitArray[$i], $rcptMatches)==0)
		    {
		      // Address only
		      $parsedRcpts[$i][PR_DISPLAY_NAME] = trim($splitArray[$i]);
		      $parsedRcpts[$i][PR_EMAIL_ADDRESS] = trim($splitArray[$i]);
		    } else
		    {
		      // Address with name (a <b@c.nl> format)
		      if (trim($rcptMatches[1]))
		      {
	          // Name available, so use it as name
			      $parsedRcpts[$i][PR_DISPLAY_NAME] = trim($rcptMatches[1]);
		      } else
		      {
	          // Name not available, use mail address as display name
			      $parsedRcpts[$i][PR_DISPLAY_NAME] = trim($rcptMatches[2]);
		      }
		      $parsedRcpts[$i][PR_EMAIL_ADDRESS] = trim($rcptMatches[2]);
		    }
		    // Other properties, we only support SMTP here
		    $parsedRcpts[$i][PR_ADDRTYPE] = "SMTP";
	      if ($rType!="")
		      $parsedRcpts[$i][PR_RECIPIENT_TYPE] = $rType;
		    $parsedRcpts[$i][PR_OBJECT_TYPE] = MAPI_MAILUSER;
	      
	      // One-Off entry identifier; needed for Zarafa
	      $parsedRcpts[$i][PR_ENTRYID] = mapi_createoneoff($parsedRcpts[$i][PR_DISPLAY_NAME], $parsedRcpts[$i][PR_ADDRTYPE], $parsedRcpts[$i][PR_EMAIL_ADDRESS]);
      }
	  }
	  if (count($parsedRcpts)>0&&$errors==0)
	    return $parsedRcpts;
	  else
	    return false;
	}
	
	
	/**
	* Function which parses message recipients to a string.
	* This is more of a util than a MAPI helper function.
	* @author Robbert Monna
	* @version 0.1
	* @param string $recipients Recipients in an array
	* @param string $type MAPI_TO, MAPI_CC or MAPI_BCC (optional, take all when not given)
	* @return array() Returns a string with message recipients
	*/
	function createRecipientString($recipients, $type="default")
	{
		// First create an array with recipients of only the right type.
		$tempArray = Array();
		for ($i=0; $i<count($recipients); $i++)
		{
			if ($type=="default"||$recipients[$i][PR_RECIPIENT_TYPE]==$type)
			{
				$tempArray[] = $recipients[$i];
			}
		}
		// Then build a string out of this array.
		$tempRecipients = "";
		// Loop through all recipients in the array.
		for ($i=0; $i<count($tempArray); $i++)
		{
			if (empty($recipients[$i][PR_DISPLAY_NAME]))
			{
			  // There is no display name for this recipient.
				$tempRecipients .= $tempArray[$i][PR_EMAIL_ADDRESS];
			}
			else if (empty($tempArray[$i][PR_EMAIL_ADDRESS]))
			{
				$tempRecipients .= $tempArray[$i][PR_DISPLAY_NAME];
			}
			else
			{
			  // There is a display name set for the recipient
			  if (strcmp($tempArray[$i][PR_DISPLAY_NAME], $tempArray[$i][PR_EMAIL_ADDRESS])==0)
			  {
			    // Name and address are equal: output only the flat address.
			    $tempRecipients .= $tempArray[$i][PR_EMAIL_ADDRESS];
			  } else
			  {
			    // Name and address are not equal; use a <b@c.nl> notation.
				$tempRecipients .= $tempArray[$i][PR_DISPLAY_NAME];
				$tempRecipients .= " <".$tempArray[$i][PR_EMAIL_ADDRESS].">";
			  }
			}
			// If this wasn't the last recipient, add a comma and a space.
			if ($i!=count($tempArray)-1)
			{
				$tempRecipients .= "; ";
			}
		}
		return $tempRecipients;
	}
	
	/**
	* Function which takes a table of properties and changes the index strings to real constants (long)
  * The values will be converted to the right PHP types. This function should
  * be mapi_setprops compatible. So the output from this function could directly
  * be set by mapi_setprops .
	* @author Robbert Monna
	* @version 0.1
	* @param Array $postedProperties The properties that were posted, with the names (and values) written as strings.
	* @return boolean Returns the resulting array
	*/
	function propArrayStringKeysToIDKeys($postedProperties)
	{
	  $resultArray =  null;
	  foreach ($postedProperties as $postedPropertyKey => $postedPropertyValue)
	  {
      $cstdPropKey = null;
      // Maybe should be changed to !is_numeric or something..
      if (is_string($postedPropertyKey))
        $cstdPropKey = constant($postedPropertyKey);
      else
        $cstdPropKey = $postedPropertyKey;
            switch(mapi_prop_type($cstdPropKey)) {
              case PT_LONG:
                $resultArray[$cstdPropKey] = intval($postedPropertyValue);
              break;
              case PT_DOUBLE:
                if(settype($postedPropertyValue, "double"))
                  $resultArray[$cstdPropKey] = $postedPropertyValue;
                else 
                  $resultArray[$cstdPropKey] = floatval($postedPropertyValue);
              break;
              case PT_BOOLEAN:
                if(!is_bool($postedPropertyValue))
                  $resultArray[$cstdPropKey] = $postedPropertyValue=="false"?false:true;
                else
                  $resultArray[$cstdPropKey] = $postedPropertyValue;
              break;
              default:
                $resultArray[$cstdPropKey] = $postedPropertyValue;
              break;
            }
	  
	  }
	  return $resultArray;
	}
  
  /**
	* Function which takes a flatentrylist and returns an array of recipients.
  * These flatentrylists are used in PR_REPLY_RECIPIENT_ENTRIES, remember to
  * keep this property synchronized with PR_REPLY_RECIPIENT_NAMES.
	* @author Robbert Monna
	* @version 0.1
	* @param String $flatEntryList The flatentrylist directly taken from a property
	* @return boolean Returns the resulting array with recipients
  * @todo Make this nicer
	*/
  function readFlatEntryList($flatEntryList)
  {
    // Unpack number of entries, the byte count and the entries
    $unpacked = unpack("V1cEntries/V1cbEntries/a*abEntries", $flatEntryList);
    
    $abEntries = Array();
    $stream = $unpacked['abEntries'];
    $pos = 8;
    for ($i=0; $i<$unpacked['cEntries']; $i++)
    {
      $findEntry = unpack("a".$pos."before/V1cb/a*after", $flatEntryList);
      // Go to after the unsigned int
      $pos += 4;
      $entry = unpack("a".$pos."before/a".$findEntry['cb']."abEntry/a*after", $flatEntryList);
      // Move to after the entry
      $pos += $findEntry['cb'];
      // Move to next 4-byte boundary
      $pos += $pos%4;
      // One one-off entry id
      $abEntries[] = $entry['abEntry'];
    }
    
    $recipients = Array();
    foreach ($abEntries as $abEntry)
    {
      // Unpack the one-off entry identifier
      $findID = unpack("V1version/a16mapiuid/v1flags/v1abFlags/a*abEntry", $abEntry);
      $tempArray = Array();
      // Split the entry in its three fields
			
			// Workaround (if Unicode then strip \0's)
			if ($findID['abFlags'] & 0x8000) {
        $idParts = explode("\0\0", $findID['abEntry']);
        foreach ($idParts as $idPart)
        {
          // Remove null characters from the field contents
          $tempArray[] = str_replace("\x00", "", $idPart);
				}
      } else {
				// Not Unicode. Just split by \0.
				$tempArray = explode("\0", $findID['abEntry']);
			}
      // Put data in recipient array
      $recipients[] = Array(PR_DISPLAY_NAME => $tempArray[0],
                            PR_ADDRTYPE => $tempArray[1],
                            PR_EMAIL_ADDRESS => $tempArray[2]);
    }
    return $recipients;
  }
  
  /**
	* Function which takes an array of recipients and returns a flatentrylist.
  * These flatentrylists are used in PR_REPLY_RECIPIENT_ENTRIES, remember to
  * keep this property synchronized with PR_REPLY_RECIPIENT_NAMES.
	* @author Robbert Monna
	* @version 0.1
	* @param String $recipientArray The array with recipients to convert
	* @return boolean Returns the resulting flatentrylist
	*/
  function writeFlatEntryList($recipientArray)
  {
    $oneOffs = Array();
    foreach ($recipientArray as $recipient)
    {
      // Add display name if it doesn't exist
      if (!array_key_exists(PR_DISPLAY_NAME, $recipient)||empty($recipient[PR_DISPLAY_NAME]))
        $recipient[PR_DISPLAY_NAME] = $recipient[PR_EMAIL_ADDRESS];
      /*
      // \0 between the three fields
      $oneOff = $recipient[PR_DISPLAY_NAME]."\0".$recipient[PR_ADDRTYPE]."\0".$recipient[PR_EMAIL_ADDRESS];
      $paddedOneOff = "";
      for ($i=0;$i<strlen($oneOff);$i++)
      {
        $paddedOneOff .= substr($oneOff, $i, 1);
        // Add \0 between each letter (and \0)
        if ($i<strlen($oneOff)-1)
          $paddedOneOff .= "\0";
      }
      // Those hex numbers are actually defined in mapidefs.h, I believe
      $oneOffs[] = pack("VC16vC2a*", 0, 0x81, 0x2b, 0x1f, 0xa4, 0xbe, 0xa3, 0x10, 0x19, 0x9d, 0x6e,
      0x00, 0xdd, 0x01, 0x0f, 0x54, 0x02, 0, 1, -128, $paddedOneOff);
      */
      $oneOffs[] = mapi_createoneoff($recipient[PR_DISPLAY_NAME], $recipient[PR_ADDRTYPE], $recipient[PR_EMAIL_ADDRESS]);
    }
    
    // Construct string from array with (padded) One-Off entry identifiers
    //
    // Remember, if you want to take the createOneOff part above out: that code
    // produces a padded OneOff and we add the right amount of null characters
    // below.
    //
    // So below is a wrong method for composing a flatentrylist from oneoffs and
    // above is a wrong method form composing a oneoff.
    $flatEntryString = "";
    for ($i=0; $i<count($oneOffs); $i++)
    {
      /*
      // Also add padding with x3
      $flatEntryString .= pack("Va*x3", strlen($oneOffs[$i])+3, $oneOffs[$i]);
      */
      $flatEntryString .= pack("Va*", strlen($oneOffs[$i]), $oneOffs[$i]);
      // Fill to 4-byte boundary
      $rest = strlen($oneOffs[$i])%4;
      for ($j=0;$j<$rest;$j++)
        $flatEntryString .= "\0";
    }
    // Pack the string with the number of flatentries and the stringlength
    return pack("V2a*", count($oneOffs), strlen($flatEntryString), $flatEntryString);
  }
}
?>
