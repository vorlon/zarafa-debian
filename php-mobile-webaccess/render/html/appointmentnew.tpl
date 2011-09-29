{include file="header.tpl" title="New Appointment"}
{if isset($parent_entryid)}<a href="index.php?entryid={$parent_entryid}&type=table">{$parentname|escape}</a>
<div class="line"></div>
{/if}
<form method="post" action="index.php?type=action&action=new">
<input type="hidden" name="type" value="IPM.Appointment">
<table>
	<tr>
		<td>{_}Subject{/_}:</td>
		<td><input type="text" name="subject" size="15"></td>
	</tr>
	<tr>
		<td>{_}Location{/_}:</td>
		<td><input type="text" name="location" size="15"></td>
	</tr>
	<tr>
		<td>{_}Start date{/_}:</td>
		<td>
			{if $date}
				<input type="text" name="startday" value="{$date|date_format:'%d'}" size="2">
				<input type="text" name="startmonth" value="{$date|date_format:'%m'}" size="2">
				<input type="text" name="startyear" size="4" value="{$date|date_format:'%Y'}">
			{else}
				<input type="text" name="startday" value="{$smarty.now|date_format:'%d'}" size="2">
				<input type="text" name="startmonth" value="{$smarty.now|date_format:'%m'}" size="2">
				<input type="text" name="startyear" size="4" value="{$smarty.now|date_format:'%Y'}">
			{/if}
		</td>
	</tr>
	<tr>
		<td>{_}Start time{/_}:</td>
		<td>
			{if $date}
				<input type="text" name="starttime" value="{$date|date_format:'%H:%M'}" size="5">
			{else}
				<input type="text" name="starttime" value="{$smarty.now|date_format:'%H:%M'}" size="5">
				{/if}	
		</td>
	</tr>
		<tr>
		<td>{_}End date{/_}:</td>
		<td>
			{if $date}
				<input type="text" name="endday" value="{$date|date_format:'%d'}" size="2">
				<input type="text" name="endmonth" value="{$date|date_format:'%m'}" size="2">
				<input type="text" name="endyear" size="4" value="{$date|date_format:'%Y'}">
			{else}
				<input type="text" name="endday" value="{$smarty.now|date_format:'%d'}" size="2">
				<input type="text" name="endmonth" value="{$smarty.now|date_format:'%m'}" size="2">
				<input type="text" name="endyear" size="4" value="{$smarty.now|date_format:'%Y'}">
			{/if}
		</td>
	</tr>
	<tr>
		<td>{_}End time{/_}:</td>
		<td>
			{if $date}
				<input type="text" name="endtime" value="{$date+1800|date_format:'%H:%M'}" size="5">
			{else}
				<input type="text" name="endtime" value="{$smarty.now+1800|date_format:'%H:%M'}" size="5">
				{/if}
		</td>
	</tr>
	<tr>
		<td>{_}All Day{/_}:</td>
		<td><input type="checkbox" name="allday"></td>
	</tr>
	<tr>
		<td>{_}Time As{/_}:</td>
		<td>
			<select name="busystatus">
				<option value="0">{_}Free{/_}</option>
		        <option value="1">{_}Temporarily Occupied{/_}</option>
		        <option value="2" selected>{_}Occupied{/_}</option>
		        <option value="3">{_}Not Available{/_}</option>
		   	</select>
		</td>
	</tr>
	<tr>
		<td>{_}Reminder{/_}:</td>
		<td><input type="checkbox" name="reminder"> <input type="text" size="2" name="reminder_minutes" value="15"> {_}Minutes{/_}</td>
	</tr>
	
	<tr>
		<td><input type="submit" value="{_}Create{/_}"></td>
		<td><input type="reset" value="Reset"></td>
	</tr>
</table>
</form>
{include file="footer.tpl"}
