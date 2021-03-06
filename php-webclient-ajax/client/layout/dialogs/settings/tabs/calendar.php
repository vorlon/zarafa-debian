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

function calendar_settings_title(){
	return _("Calendar");
}

function calendar_settings_order(){
	return 3;
}

function calendar_settings_html(){ ?>
	<fieldset>
		<legend><?=_("Calendar View")?></legend>

		<table class="options"> 
			<tr>
				<th><label for="calendar_workdaystart"><?=_("Start of workday")?></label></th>
				<td>
					<select id="calendar_workdaystart" onchange="calendar_checkworkhours(this)">
					<? for($i=0;$i<24*60;$i+=30) { ?>
						<option value="<?=$i?>"><?=sprintf("%d:%02d",floor($i/60),$i%60);?></option>
					<?  } ?>
					</select>
				</td>
			</tr>

			<tr>
				<th><label for="calendar_workdayend"><?=_("End of workday")?></label></th>
				<td>
					<select id="calendar_workdayend" onchange="calendar_checkworkhours(this)">
					<? for($i=0;$i<24*60;$i+=30) { ?>
						<option value="<?=$i?>"><?=sprintf("%d:%02d",floor($i/60),$i%60);?></option>
					<?  } ?>
					</select>
				</td>
			</tr>

			<tr>
				<th><label for="calendar_size"><?=_("Vertical size")?></label></th>
				<td>
					<select id="calendar_size" onchange="calendar_checkcalendarcellsize()">
						<option value="1"><?=_("Small")?></option>
						<option value="2"><?=_("Medium")?></option>
						<option value="3"><?=_("Large")?></option>
					</select>
				</td>
			</tr>
			<tr>
				<th><label for="calendar_appointment_size"><?=_("Calendar resolution")?></label></th>
				<td>
					<select id="calendar_appointment_size" onchange="calendar_checkcalendarcellsize()">
						<option value="12"><?=_("5 Minutes")?></option>
						<option value="6"><?=_("10 Minutes")?></option>
						<option value="4"><?=_("15 Minutes")?></option>
						<option value="2"><?=_("30 Minutes")?></option>
						<option value="1"><?=_("1 Hour")?></option>
					</select>
				</td>
			</tr>

			<tr>
				<th><input id="calendar_reminder" type="checkbox" class="checkbox"onclick="calendar_checkremindersetting(this)"><label for="calendar_reminder"><?=_("Default Reminder:")?></label></th>
				<td>
					<select id="calendar_reminder_minutes") >
							<option value="0">0 <?=_("minutes")?></option>
							<option value="5">5 <?=_("minutes")?></option>
							<option value="10">10 <?=_("minutes")?></option>
							<option value="15">15 <?=_("minutes")?></option>
							<option value="30">30 <?=_("minutes")?></option>
							<option value="60">1 <?=_("hour")?></option>
							<option value="120">2 <?=_("hours")?></option>
							<option value="240">4 <?=_("hours")?></option>
							<option value="480">8 <?=_("hours")?></option>
							<option value="720">0,5 <?=_("day")?></option>
							<option value="1080">18 <?=_("hours")?></option>
							<option value="1440">1 <?=_("day")?></option>
							<option value="2880">2 <?=_("days")?></option>
					</select>
				</td>
			</tr>

			<tr>
				<th>
					<input id="calendar_refresh_button" type="checkbox" class="checkbox">
					<label for="calendar_refresh_button"><?=_("Show refresh button")?></label>
				</th>
			</tr>
		</table>
	</fieldset>

	<fieldset>
		<legend><?=_("Multi User Calendar View")?></legend>

		<table class="options"> 
			<tr>
				<th><label for="calendar_mucalendar_zoomlevel"><?=_("Number of days displayed")?></label></th>
				<td>
					<select id="calendar_mucalendar_zoomlevel">
						<option value="2">2</option>
						<option value="3">3</option>
						<option value="4">4</option>
						<option value="5">5</option>
						<option value="6">6</option>
						<option value="7">7</option>
						<option value="10">10</option>
						<option value="14">14</option>
					</select>
				</td>
			</tr>

			<tr>
				<th><label for="calendar_mucalendar_numofdaysloaded"><?=_("Number of days loaded")?></label></th>
				<td>
					<select id="calendar_mucalendar_numofdaysloaded">
						<option value="2">2</option>
						<option value="3">3</option>
						<option value="4">4</option>
						<option value="5">5</option>
						<option value="6">6</option>
						<option value="7">7</option>
						<option value="10">10</option>
						<option value="14">14</option>
					</select>
				</td>
			</tr>
		</table>				
	</fieldset>
<?php } ?>
