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
	/**
	* Generic event bus with subscribe/notify architecture
	*
	* Every module registers itselfs by this class so that it can receive events. The events represent changes in the underlying
	* MAPI store like changes to folders, e-mails, calendar items, etc.
	*
	* A module can register itself for the following events:
	* - OBJECT_SAVE
	* - OBJECT_DELETE
	*
	* - TABLE_SAVE
	* - TABLE_DELETE
	*
	* - REQUEST_START
	* - REQUEST_END
	*
	* - DUMMY_EVENT
	*
	* To use the REQUEST/DUMMY events, make sure you register your module with REQUEST_ENTRYID 
	* as the dummy entryid.
	*
	* Events:
	*
	* (The parameters that are discussed are passed to notify() and sent to the update() methods of the modules)
	*
	* <b>OBJECT_SAVE</b>
	* This event is triggered when a folder object is created or modified. The entryid parameter is the entryid
	* of the object that has been created or modified. The data parameter contains an array of properties of the
	* object, with the following properties: PR_ENTRYID, PR_STORE_ENTRYID
	*
	* <b>OBJECT_DELETE</b>
	* This event is triggered when a folder is deleted. Teh entryid parameter is the entryid of the object
    * that has been deleted. The data parameter contains an array of properties of the deleted object, with
    * the following properties: PR_ENTRYID, PR_STORE_ENTRYID
    *
    * <b>TABLE_SAVE</b>
    * This event is triggered when a message item is created or modified. The entryid parameter is the entryid of the modified
    * object. The data parameter contains the following properties of the modified message: PR_ENTRYID, PR_PARENT_ENTRYID, 
    * PR_STORE_ENTRYID
    *
    * <b>TABLE_DELETE</b>
    * This event is triggered when a message item is deleted. The entryid parameter is the entryid of the deleted object. The
    * data parameter contains the following properties of the deleted message: PR_ENTRYID, PR_PARENT_ENTRYID, PR_STORE_ENTRYID
    *
    * <b>REQUEST_START</b>
    * This event is triggered for EVERY XML request that is done to the backend, before processing of the XML or other events.
    * The entryid parameter must be the fixed value REQUEST_ENTRYID. The $data parameter is not passed.
    *
    * <b>REQUEST_END</b>
    * This event is triggereed for EVERY XML request that is done to the backend, after all modules processing and will be the last
    * event triggered. The entryid parameter must be that fixed value REQUEST_ENTRYID. The $data parameter is not passed.
    *
    * <b>DUMMY_EVENT</b>
    * If your module receives no events, it must still be registered. This is done via the DUMMY_EVENT which will register the module
    * but not register the module for any event.
    *
    * @todo The event names are rather misleading and should be renamed to FOLDER_CHANGE, FOLDER_DELETE, MESSAGE_CHANGE, MESSAGE_DELETE
    * @todo The entryid is passed in $entryid AND in $data in almost all cases, which is rather wasteful
    * @todo DUMMY_EVENT is a rather strange way to register a module with no events, why not pass an empty events array ?
    *
    * @package core
	*/
	class Bus
	{
		/**
		* @var array data which is sent back to the client.
		*/
		var $responseData;
		
		/**
		* @var array all registered modules.
		*/
		var $registeredModules;
		
		/**
		* @var array all module objects
		*/
		var $modules;
		
		/**
		* Constructor
		*/
		function Bus()
		{
			$this->responseData = array();
			$this->responseData["module"] = array();
			
			$this->registeredModules = array();
			$this->modules = array();
		} 
	
		/**
		* Register a module on the bus.
		* 
		* After a module is registered on the bus, it will receive updates to objects generated by this module
		* or by other modules. For example, a module registered on the inbox can receive changes of the item count
		* of the inbox by specifying the entryid of the inbox here and the event
		*
		* Valid values for $events are OBJECT_SAVE, OBJECT_DELETE, TABLE_SAVE, TABLE_DELETE, REQUEST_START, REQUEST_END and DUMMY_EVENT
		*
		* @access public
		* @param object $moduleObject Reference to the module object to be registered
		* @param string $entryid EntryID of an object coupled to the module
		* @param array $events Array of events that the module should receive
		* @param boolean $recursive optional If true all events on children of the object referenced by $entryid will go to this module object.
		*/
		function register($module, $entryid, $events, $recursive = false)
		{
			$this->modules[$module->getId()] = $module;
			
			if(!array_key_exists($entryid, $this->registeredModules)) {
				$this->registeredModules[$entryid] = array();
			}
			
			foreach($events as $event)
			{
				if(!array_key_exists($event, $this->registeredModules[$entryid])) {
					$this->registeredModules[$entryid][$event] = array();
				}
				array_push($this->registeredModules[$entryid][$event], array("id" => $module->getId(), "isRecursive" => $recursive));
			}
		} 
		
		/**
		* Broadcast an update to modules attached to bus.
		*
		* This function is used to send an update to other modules that have been attached to the bus via register().
		*
		* @access public
		* @param string $entryid Entryid of the object for which the event has happened
		* @param int $event Event which should be fired. Can be any of OBJECT_SAVE, OBJECT_DELETE, TABLE_SAVE, TABLE_DELETE, REQUEST_START, REQUEST_END and DUMMY_EVENT.
		* @param array $data The data which is used to execute the event, which differs for each event type.
		*/
		function notify($entryID, $event, $data=null)
		{
			// Get recursive modules
			$recursiveModules = $this->getRecursiveModules();
			
			// Modules must be get only ONE update
			$updatedModules = array();

			// Check the recursive modules and check if the event is a child of the recursive module
			if ($entryID!=REQUEST_ENTRYID){
				if (is_array($data)&& isset($data[PR_STORE_ENTRYID])){
					$store = $GLOBALS["mapisession"]->openMessageStore($data[PR_STORE_ENTRYID]);
				}else{
					// Get default store
					$store = $GLOBALS["mapisession"]->getDefaultMessageStore();
				}

				foreach($recursiveModules as $rootentryid => $module)
				{
					if($this->isChild($store, hex2bin($rootentryid), hex2bin($entryID))) {
						if(isset($this->modules[$module["id"]]) && is_object($this->modules[$module["id"]])) {
							$this->modules[$module["id"]]->update($event, $entryID, $data);
							$updatedModules[$module["id"]] = true;
						}
					}
				}
			}
			
			// Update the module
			if(array_key_exists($entryID, $this->registeredModules)) {
				if(array_key_exists($event, $this->registeredModules[$entryID])) {
					foreach($this->registeredModules[$entryID][$event] as $module)
					{
						if (!isset($updatedModules[$module["id"]])){
							if(isset($this->modules[$module["id"]]) && is_object($this->modules[$module["id"]])) {
								$this->modules[$module["id"]]->update($event, $entryID, $data);
								$updatedModules[$module["id"]] = true;
							}
						}
					}
				}
			}
		}
		
		/**
		* Add reponse data to collected XML response
		*
		* This function is called by all modules when they want to output some XML response data
		* to the client. It simply copies the XML response data to an internal structure. The data
        * is later collected via getData() when all modules are done processing, after which the
        * data is sent to the client.
        *
        * The data added is not checked for validity, and is therefore completely free-form. However,
        * 
		* @access public
		* @param array $data data to be added.
		*/
		function addData($data)
		{
			array_push($this->responseData["module"], $data);
		}
		
		/**
		* Function which returns the data stored via addData()
		* @access public
		* @return array Response data.
		*/
		function getData()
		{
			return $this->responseData;
		}
		
		/**
		* Reset the bus state and response data, and send resets to all modules.
		*
		* Since the bus is a global, persistent object, the reset() function is called before or after
		* processing. This makes sure that the on-disk serialized representation of the bus object is as
		* small as possible.
		* @access public
		*/
		function reset()
		{
			$this->responseData = array();
			$this->responseData["module"] = array();
			
			foreach($this->modules as $key => $module) {
				$this->modules[$key]->reset();
			}
		}
		
		/**
		* Check if the specified module ID is already registered via register()
		* @param integer $moduleID Module number.
		* @return object module object if moduleID exists, false if not.
		* @access public
		*/
		function moduleExists($moduleID)
		{
			if(isset($this->modules[$moduleID]) && is_object($this->modules[$moduleID])) {
				return $this->modules[$moduleID];
			}
			
			return false;
		}
		
		/**
        * Update an existing module with a new module state.
		*
		* Since the modules inside the bus are not referenced, but copied, and since modules should
		* retain state as persistent objects, this function is used after a module has finished processing
		* to save the module's state inside the bus object. This simply makes sure that the state of the
		* module is saved over multiple XML calls.
		*
   		* @access public
		* @param object $module Module object		 		 
		*/		 		
		function setModule($module)
		{
			if(isset($this->modules[$module->getId()]) && is_object($this->modules[$module->getId()])) {
				$this->modules[$module->getId()] = $module;
			}
		}
		
		/**
		* Returns a list of all modules that were registered as recursive
		*
   		* @access public
		* @return array Associative array with $entryid -> $module of all modules which are recursive
		*/
		function getRecursiveModules()
		{
			$modules = array();
			
			foreach($this->registeredModules as $rootentryid => $entryID)
			{
				foreach($entryID as $event)
				{
					foreach($event as $module)
					{
						if($module["isRecursive"]) {
							$modules[$rootentryid] = $module;
						}
					}
				}
			}
			
			return $modules;
		}
		
		/**
		* Check if an entryid is the child of another (parent) entryid
		*
		* <B>WARNING</B>This function is recursive, and can be quite slow
		*
   		* @access private
		* @param object $store Mapi Message Store in which both entryid's reside
		* @param string $rootentryid The parent entryid
		* @param string $entryid The child entryid
		* @return boolean true if $entryid is child of $rootentryid.
		*/
		function isChild($store, $rootentryid, $entryid)
		{
			$isChild = false;
			$parententryid = $this->getParentEntryID($store, $entryid);

			if($parententryid) {
				if($rootentryid != $parententryid) {
					$isChild = $this->isChild($store, $rootentryid, $parententryid);
				} else {
					$isChild = true;
				}
			}
			
			return $isChild;
		}
		
		/**
		* Get the parent entryid of another entryid
		*
		* This function needs to be able to open the MAPI message of the passed entryid to get the parent information.
		*
   		* @access private
		* @param object $store Mapi Message Store.
		* @param string $entryid The entryid.
		* @return string the parententryid on success, false on failure.
		*/
		function getParentEntryID($store, $entryid)
		{
			$parententryid = false;

			$mapiObject = mapi_msgstore_openentry($store, $entryid);
			if($mapiObject) {
				$props = mapi_getprops($mapiObject, array(PR_DISPLAY_NAME, PR_ENTRYID, PR_PARENT_ENTRYID, PR_FOLDER_TYPE));
				if(isset($props[PR_ENTRYID]) && isset($props[PR_PARENT_ENTRYID])) {
					if(($props[PR_ENTRYID] != $props[PR_PARENT_ENTRYID]) && isset($props[PR_FOLDER_TYPE]) && $props[PR_FOLDER_TYPE]!=FOLDER_ROOT) {
						$parententryid = $props[PR_PARENT_ENTRYID];
					}
				}
			}

			return $parententryid;
		}
		
		/**
		* Delete all registered modules.
		*
		* This function is normally only called when the client does a logout or re-login.
   		* @access public
		*/		 		
		function deleteAllRegisteredModules()
		{
			unset($this->registeredModules);
			unset($this->modules);
			$this->registeredModules = array();
			$this->modules = array();
		}
		
		/**
		* Delete a specific registered module
		*
		* When a client no longer needs a module, it can be delete from the bus. After being deleted, the module
		* will no longer receive events and all state information for the module will be removed.
		*
   		* @access public
		* @param int $moduleID ID of the module.
		*/
		function deleteRegisteredModule($moduleID)
		{
			unset($this->modules[$moduleID]);
		
			$registeredModules = $this->registeredModules;
			$this->registeredModules = array();
		
			foreach($registeredModules as $entryID => $entryIDs)
			{
				foreach($entryIDs as $event => $events)
				{
					foreach($events as $moduleNumber => $module)
					{
						if(isset($this->modules[$module["id"]]) && is_object($this->modules[$module["id"]])) {
							if($this->modules[$module["id"]]->getId() != $moduleID) {
								if(!isset($this->registeredModules[$entryID])) {
									$this->registeredModules[$entryID] = array();
								}
								
								if(!isset($this->registeredModules[$entryID][$event])) {
									$this->registeredModules[$entryID][$event] = array();
								}
								
								array_push($this->registeredModules[$entryID][$event], array("id" => $module["id"], "isRecursive" => $module["isRecursive"]));
							}
						}
					}
				}
			}
		}
	} 	
?>
