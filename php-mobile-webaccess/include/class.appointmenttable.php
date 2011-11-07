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
 * @package php-mobile-webaccess
 * @author Mans Matulewicz
 * @version 0.01
 * 
 *
 */

class appointmentTable extends table
{
	var $properties;
	
	/**
	 * contructor
	 *
	 * @param resource Mapi_Msgstore
	 * @param reoource EntryID
	 * @return appointmentFolder
	 */
	function appointmentTable($store, $eid)
	{
		$this->store=$store;
		$this->eid=$eid;
		$this->amountitems = $GLOBALS["appointmentsonpage"];
		
        $guid = makeguid("{00062002-0000-0000-C000-000000000046}");
        $guid2 = makeguid("{00062008-0000-0000-C000-000000000046}");
        $guid3 = makeguid("{00020329-0000-0000-C000-000000000046}");
        $names = mapi_getIdsFromNames($this->store, array(0x820D, 0x820E, 0x8223, 0x8216, 
                                                          0x8205, 0x8214, 0x8215, 0x8506, 
                                                          0x8217, 0x8235, 0x8236, 0x8208, 
                                                          0x8213, 0x8218, 0x8503, 0x8501,
                                                          0x8506, 0x853A, 0x8586, "Keywords",
                                                          0x8502, 0x8516, 0x8517, 0x8228, 
                                                          0x8233), 
                                                    array($guid, $guid, $guid, $guid, 
                                                          $guid, $guid, $guid, $guid2, 
                                                          $guid, $guid, $guid, $guid, 
                                                          $guid, $guid, $guid2, $guid2,
                                                          $guid2, $guid2, $guid2, $guid3,
                                                          $guid2, $guid2, $guid2, $guid,
                                                          $guid));

        $properties = array();
        $properties["entryid"] = PR_ENTRYID;
        $properties["parent_entryid"] = PR_PARENT_ENTRYID;
        $properties["message_class"] = PR_MESSAGE_CLASS;
        $properties["icon_index"] = PR_ICON_INDEX;
        $properties["subject"] = PR_SUBJECT;
        $properties["display_to"] = PR_DISPLAY_TO;
        $properties["importance"] = PR_IMPORTANCE;
        $properties["sensitivity"] = PR_SENSITIVITY;
        $properties["startdate"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[0]));
        $properties["duedate"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[1]));
        $properties["recurring"] = mapi_prop_tag(PT_BOOLEAN, mapi_prop_id($names[2]));
        $properties["recurring_data"] = mapi_prop_tag(PT_BINARY, mapi_prop_id($names[3]));
        $properties["busystatus"] = mapi_prop_tag(PT_LONG, mapi_prop_id($names[4]));
        $properties["label"] = mapi_prop_tag(PT_LONG, mapi_prop_id($names[5]));
        $properties["alldayevent"] = mapi_prop_tag(PT_BOOLEAN, mapi_prop_id($names[6]));
        $properties["private"] = mapi_prop_tag(PT_BOOLEAN, mapi_prop_id($names[7]));
        $properties["meeting"] = mapi_prop_tag(PT_LONG, mapi_prop_id($names[8]));
        $properties["startdate_recurring"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[9]));
        $properties["enddate_recurring"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[10]));
        $properties["location"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[11]));
        $properties["duration"] = mapi_prop_tag(PT_LONG, mapi_prop_id($names[12]));
        $properties["responsestatus"] = mapi_prop_tag(PT_LONG, mapi_prop_id($names[13]));
        $properties["reminder"] = mapi_prop_tag(PT_BOOLEAN, mapi_prop_id($names[14]));
        $properties["reminder_minutes"] = mapi_prop_tag(PT_LONG, mapi_prop_id($names[15]));
        $properties["private"] = mapi_prop_tag(PT_BOOLEAN, mapi_prop_id($names[16]));
        $properties["contacts"] = mapi_prop_tag(PT_MV_STRING8, mapi_prop_id($names[17]));
        $properties["contacts_string"] = mapi_prop_tag(PT_STRING8, mapi_prop_id($names[18]));
        $properties["categories"] = mapi_prop_tag(PT_MV_STRING8, mapi_prop_id($names[19]));
        $properties["reminder_time"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[20]));
        $properties["commonstart"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[21]));
        $properties["commonend"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[22]));
        $properties["basedate"] = mapi_prop_tag(PT_SYSTIME, mapi_prop_id($names[23]));
        $properties["timezone_data"] = mapi_prop_tag(PT_BINARY, mapi_prop_id($names[24]));
        
        $this->properties = $properties;
	}
	
	/**
	 * @todo Show appointments
	 *
	 */
	
	function getcontents($days=1, $type="normal")
	{
        global $smarty;
        $calender = mapi_msgstore_openentry($this->store, hex2bin($this->eid));
        
        
        if (isset($_GET["day"])){
            $datestart=mktime(0, 0, 0, substr($_GET["day"], 4, 2), substr($_GET["day"], 6, 2), substr($_GET["day"], 0, 4));
            $dateend=mktime(0, 0, 0, substr($_GET["day"], 4, 2), substr($_GET["day"], 6, 2)+$days, substr($_GET["day"], 0, 4));
            $prevdate= mktime(0, 0, 0, substr($_GET["day"], 4, 2), substr($_GET["day"], 6, 2)-7, substr($_GET["day"], 0, 4));
            $nextdate= mktime(0, 0, 0, substr($_GET["day"], 4, 2), substr($_GET["day"], 6, 2)+7, substr($_GET["day"], 0, 4));
        }
        else{
            $datestart=mktime(0, 0, 0, date("m"), date("d"), date("y"));
            $dateend=mktime(0, 0, 0, date("m"), date("d")+$days, date("y"));
            $prevdate=mktime(0, 0, 0, date("m"), date("d")-7, date("y"));
            $nextdate=mktime(0, 0, 0, date("m"), date("d")+7, date("y"));
        }

        $appointments = $this->getCalendarItems($this->store, $calender, $datestart, $dateend);
        
        switch ($type)
        {
            
            
        
            case "upcoming":
                
                // Obtain a list of columns
                foreach ($appointments as $key => $row) {
                    $start[$key]  = $row['start'];
                    $end[$key] = $row['end'];
                }
        
                // Sort the appointments with startdate ascending and after that enddate ascending
                if (isset($start))
                array_multisort($start, SORT_ASC, $end, SORT_ASC, $appointments);
                $daysa = array();
                $date=$datestart;
                $daycounter=0;
                //echo $date;
                while ($date <$dateend){
                    array_push($daysa, $daycounter);
                    $daysa[$daycounter]=array();
                    array_push($daysa[$daycounter], $date);
                    $daysa[$daycounter]["appointments"]=array();
                    $appcounter=0;
                    foreach ($appointments as $key => $appointment)
                    {
                        if ($date== mktime(0, 0, 0, date("m", $appointment["start"]), date("d", $appointment["start"]), date("y", $appointment["start"]))){
                            $item = array();
                            $item["entryid"]= bin2hex($appointment["entryid"]);
                            $item["display_to"]=$appointment["display_to"];
                            $item["start"]=$appointment["start"];
                            $item["end"]=$appointment["end"];
                            $item["subject"]=$appointment["subject"];
                            $item["location"]=$appointment["location"];
                            array_push($daysa[$daycounter]["appointments"], $item);
                            $appcounter++;
                        }
                    }
                    $daysa[$daycounter]["apps"]=$appcounter;
                    $date = mktime(0, 0, 0, date("m", $date), date("d", $date)+1, date("y", $date));
                    $daycounter++;
                }
                $smarty->assign('days', $daysa);
                break;
            
            
            case "day":
                foreach ($appointments as $key => $row) {
                    $start[$key]  = $row['start'];
                    $end[$key] = $row['end'];
                }
        
                // Sort the appointments with startdate ascending and after that enddate ascending
                if (isset($start))
                array_multisort($start, SORT_ASC, $end, SORT_ASC, $appointments);
                

                $bars[0] = array();
                $allday=array();
                foreach ($appointments as $appointment)
                {
                    $appointment["entryid"] = bin2hex ($appointment["entryid"]);
                    $appointment["startblock"] = (int)getstart($appointment["start"]);
                    $appointment["length"] = round(getlength($appointment["start"], $appointment["end"]));
                    if (!$appointment["alldayevent"])
                    {
                        if (empty($bars[0]))
                        {				
                            array_push($bars[0], $appointment);
                        }
                        else {
                            
                            $counter=0;
                            $result=false;
                            $done = false;
                            while(in_bar($appointment, $bars[$counter])){
                                $counter++;
                                if (!isset($bars[$counter])) array_push($bars, array());
                            }
                                
                            array_push($bars[$counter], $appointment);		
                            
                        }
                    }
                    else{
                        array_push($allday, $appointment);
                    }
                    
                }
                $smarty->assign('bars', $bars);
                $smarty->assign('allday', $allday);
                $smarty->assign('date', $datestart);
                $smarty->assign("cellshour", CELLSHOUR);
                $smarty->assign("cellsday", CELLSHOUR*24);
                $smarty->assign("amountbars", count($bars));
                $smarty->assign("barwidth", (int) 100/(count($bars)));
                
        
                
                break;
                    
            default:
        
                if (empty($appointments))
                {
                    $smartyarray = array();
                    $item["subject"]="No appointments.";
                    array_push($smartyarray, $item);
                    $smarty->assign('items', $smartyarray);
                }
                else {
                    
                    
                    // Obtain a list of columns
                    foreach ($appointments as $key => $row) {
                        $start[$key]  = $row['start'];
                        $end[$key] = $row['end'];
                    }
            
                    // Sort the appointments with startdate ascending and after that enddate ascending
                    array_multisort($start, SORT_ASC, $end, SORT_ASC, $appointments);
                    $smartyarray = array();
                    foreach ($appointments as $key => $appointment)
                    {
                        $item = array();
                        $item["entryid"]= bin2hex($appointment["entryid"]);
                        $item["display_to"]=$appointment["display_to"];
                        $item["start"]=$appointment["start"];
                        $item["end"]=$appointment["end"];
                        $item["subject"]=$appointment["subject"];
                        $item["location"]=$appointment["location"];
                        /**
                         * in table we dont want () to showup when there is an empty location
                         */
                        if ($appointment["location"] != "") {
                            $item["locationt"]="(".$appointment["location"].")";
                        }
                        else {
                            $item["locationt"]="";
                        }
                        
                        if ($appointment["alldayevent"]){
                            $item["div"]="allday";
                        }
                        else {
                            $item["div"]="partday";
                        }
                        
                        array_push($smartyarray, $item);
                    }
                    $smarty->assign('items', $smartyarray);
                }
                break;
        }
        
        
        /**
         * next is needed to get the dates right on top of the calender view
         */
        $date = array();
        for($x=0; $x<7; $x++)
        {
            $tmpdate = date("j", $datestart) - date("w", $datestart) + $x; //day x of month - day y of week + counter
            
            $date[$x]["date"] = date("Ymd", mktime(0, 0, 0, date("m", $datestart), $tmpdate, date("Y", $datestart)));
            if ((mktime(0, 0, 0, date("m", $datestart), $tmpdate, date("Y", $datestart))) == $datestart)	{
                $date[$x]["backtoday"] = "today";
            }
            else{
                $date[$x]["backtoday"] = "otherday";
            }
        }
        $smarty->assign('dates', $date);
        $smarty->assign('listdate', $datestart);
        $smarty->assign('prevdate', $prevdate);
        $smarty->assign('nextdate', $nextdate);
        $smarty->assign('entryid', $this->eid);
        
		$box = mapi_msgstore_openentry($this->store, hex2bin($this->eid));
		$boxprops = mapi_getprops($box, array(PR_DISPLAY_NAME));
		$smarty->assign("display_name", $boxprops[PR_DISPLAY_NAME]);

        $this->get_parent();
    }

    function makedays($date)
    {
        $days= array();
        $days[0] ;
    }

    /**
     * Function to return all Calendar items in a given timeframe. This
     * function also takes recurring items into account.
     * @param object $store message store
     * @param object $calendar folder
     * @param date $start startdate of the interval
     * @param date $end enddate of the interval
     */
    function getCalendarItems($store, $calendar, $start, $end)
    {
        $items = Array();
    
        /***
        * Restriction on calendar:
        * Get normal items which starts, ends, in between or over the current start-end range.
        * Get recurring items which ends after the start-end range or which start before the start-end range.
        *
        * The restriction is shown below in a 'if'-statement.
        *
        * if(
        *		(item[start] >= start && item[start] <= end) ||
        *		(item[end]   >= start && item[end]   <= end) ||
        *		(item[start] <  start && item[end]   >  end) ||
        *		
        *		(
        *			(EXIST(recurrence_enddate_property) && item[isRecurring] == true AND item[end] >= start) ||
        *			(!EXIST(recurrence_enddate_property) && item[isRecurring] == true AND item[start] <= end) 
        *		)
        *	)
        */
        $restriction = Array(RES_OR,
                             Array(
                                   // OR
                                   // (item[start] >= start && item[start] <= end)
                                   Array(RES_AND,
                                         Array(
                                               Array(RES_PROPERTY,
                                                     Array(RELOP => RELOP_GE,
                                                           ULPROPTAG => $this->properties["startdate"],
                                                           VALUE => $start
                                                           )
                                                     ),
                                               Array(RES_PROPERTY,
                                                     Array(RELOP => RELOP_LE,
                                                           ULPROPTAG => $this->properties["startdate"],
                                                           VALUE => $end
                                                           )
                                                     )
                                               )
                                         ),
                                   // OR
                                   // (item[end]   >= start && item[end]   <= end)
                                   Array(RES_AND,
                                         Array(
                                               Array(RES_PROPERTY,
                                                     Array(RELOP => RELOP_GE,
                                                           ULPROPTAG => $this->properties["duedate"],
                                                           VALUE => $start
                                                           )
                                                     ),
                                               Array(RES_PROPERTY,
                                                     Array(RELOP => RELOP_LE,
                                                           ULPROPTAG => $this->properties["duedate"],
                                                           VALUE => $end
                                                           )
                                                     )
                                               )
                                         ),
                                   // OR
                                   // (item[start] <  start && item[end]   >  end)
                                   Array(RES_AND,
                                         Array(
                                               Array(RES_PROPERTY,
                                                     Array(RELOP => RELOP_LT,
                                                           ULPROPTAG => $this->properties["startdate"],
                                                           VALUE => $start
                                                           )
                                                     ),
                                               Array(RES_PROPERTY,
                                                     Array(RELOP => RELOP_GT,
                                                           ULPROPTAG => $this->properties["duedate"],
                                                           VALUE => $end
                                                           )
                                                     )
                                               )
                                         ),
                                   // OR
                                   Array(RES_OR,
                                         Array(
                                               // OR
                                               // (EXIST(recurrence_enddate_property) && item[isRecurring] == true && item[end] >= start)
                                               Array(RES_AND,
                                                     Array(
                                                           Array(RES_EXIST,
                                                                 Array(ULPROPTAG => $this->properties["enddate_recurring"],
                                                                       )
                                                                 ),
                                                           Array(RES_PROPERTY,
                                                                 Array(RELOP => RELOP_EQ,
                                                                       ULPROPTAG => $this->properties["recurring"],
                                                                       VALUE => true
                                                                       )
                                                                 ),
                                                           Array(RES_PROPERTY,
                                                                 Array(RELOP => RELOP_GE,
                                                                       ULPROPTAG => $this->properties["enddate_recurring"],
                                                                       VALUE => $start
                                                                       )
                                                                 )
                                                           )
                                                     ),
                                               // OR
                                               // (!EXIST(recurrence_enddate_property) && item[isRecurring] == true && item[start] <= end)
                                               Array(RES_AND,
                                                     Array(
                                                           Array(RES_NOT,
                                                                 Array(
                                                                       Array(RES_EXIST,
                                                                             Array(ULPROPTAG => $this->properties["enddate_recurring"]
                                                                                   )
                                                                             )
                                                                       )
                                                                 ),
                                                           Array(RES_PROPERTY,
                                                                 Array(RELOP => RELOP_LE,
                                                                       ULPROPTAG => $this->properties["startdate"],
                                                                       VALUE => $end
                                                                       )
                                                                 ),
                                                           Array(RES_PROPERTY,
                                                                 Array(RELOP => RELOP_EQ,
                                                                       ULPROPTAG => $this->properties["recurring"],
                                                                       VALUE => true
                                                                       )
                                                                 )
                                                           )
                                                     )
                                               )
                                         ) // EXISTS OR
                                   )
                             );		// global OR

        $table = mapi_folder_getcontentstable($calendar);
        $calendaritems = mapi_table_queryallrows($table, $this->properties, $restriction);
    
        foreach($calendaritems as $calendaritem)
        {
            if (isset($calendaritem[$this->properties["recurring"]]) && $calendaritem[$this->properties["recurring"]]) {
                $recurrence = new Recurrence($store, $calendaritem);
                $recuritems = $recurrence->getItems($start, $end);
                
                foreach($recuritems as $recuritem)
                {
                    $item = Array();
                    $item['start'] = $recuritem[$this->properties['startdate']];
                    $item['end'] = $recuritem[$this->properties['duedate']];
                    $item['subject'] = $recuritem[$this->properties['subject']];
                    $item['display_to'] = $recuritem[PR_DISPLAY_TO];
                    $item['location'] = $recuritem[$this->properties['location']];
                    $item['alldayevent'] = $recuritem[$this->properties['alldayevent']];
                    $item['entryid'] = $recuritem[PR_ENTRYID];

                    if(isset($recuritem["exception"])) {
                        $item["exception"] = true;
                    }

                    if(isset($recuritem["basedate"])) {
                        $item["basedate"] = array();
                        $item["basedate"]["attributes"] = array();
                        $item["basedate"]["attributes"]["unixtime"] = $recuritem["basedate"];
                        $item["basedate"]["_content"] = strftime("%a %d-%m-%Y %H:%M", $recuritem["basedate"]);
                    }
                    
                    array_push($items, $item);
                }
            } else {
                $item['start'] = $calendaritem[$this->properties['startdate']];
                $item['end'] = $calendaritem[$this->properties['duedate']];
                $item['subject'] = $calendaritem[$this->properties['subject']];
                $item['display_to'] = $calendaritem[PR_DISPLAY_TO];
                $item['location'] = $calendaritem[$this->properties['location']];
                $item['alldayevent'] = $calendaritem[$this->properties['alldayevent']];
                $item['entryid'] = $calendaritem[PR_ENTRYID];
                
                array_push($items, $item);
            } 
        } 
        usort($items, array("appointmentTable","compareCalendarItems"));

        return $items;
    } 

    /**
     * Function will sort items for the month view
     * small startdate->attributes->unixtime on top	 
     */		 		
    function compareCalendarItems($a, $b)
    {
        $start_a = $a["start"];
        $start_b = $b["start"];
    
       if ($start_a == $start_b) {
           return 0;
       }
       return ($start_a < $start_b) ? -1 : 1;
    }


}
?>
