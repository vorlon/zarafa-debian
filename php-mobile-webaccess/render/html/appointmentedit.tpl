{include file="header.tpl" title="Edit Appointment"}
<a href="index.php?entryid={$parent_entryid}&type=table&task=IPF.Appointment">{$parentname|escape}</a>
<div class="line"></div>
<form method="post" action="index.php?type=action&action=save">
<input type="hidden" name="type" value="IPM.Appointment">
<input type="hidden" name="entryid" value="{$entryid}">
<table>
	<tr>
		<td>{_}Subject{/_}:</td>
		<td><input type="text" name="subject" size="15" value="{$subject|escape}"></td>
	</tr>
	<tr>
		<td>{_}Location{/_}:</td>
		<td><input type="text" name="location" size="15" value="{$location|escape}"></td>
	</tr>
	<tr>
		<td>{_}Start date{/_}:</td>
		<td>
			{if $startdate}
				<input type="text" name="startday" value="{$startdate|date_format:'%d'}" size="2">
				<input type="text" name="startmonth" value="{$startdate|date_format:'%m'}" size="2">
				<input type="text" name="startyear" size="4" value="{$startdate|date_format:'%Y'}">
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
			{if $startdate}
				<input type="text" name="starttime" value="{$startdate|date_format:'%H:%M'}" size="5">
			{else}
				<input type="text" name="starttime" value="{$smarty.now|date_format:'%H:%M'}" size="5">
				{/if}	
		</td>
	</tr>
		<tr>
		<td>{_}End date{/_}:</td>
		<td>
			{if $duedate}
				<input type="text" name="endday" value="{$duedate|date_format:'%d'}" size="2">
				<input type="text" name="endmonth" value="{$duedate|date_format:'%m'}" size="2">
				<input type="text" name="endyear" size="4" value="{$duedate|date_format:'%Y'}">
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
			{if $duedate}
				<input type="text" name="endtime" value="{$duedate|date_format:'%H:%M'}" size="5">
			{else}
				<input type="text" name="endtime" value="{$smarty.now+1800|date_format:'%H:%M'}" size="5">
			{/if}
		</td>
	</tr>
	<tr>
		<td>{_}All Day{/_}:</td>
		{if $allday}
			<td><input type="checkbox" name="allday" checked></td>
		{else}
			<td><input type="checkbox" name="allday"></td>
		{/if}
		
	</tr>
	<tr>
		<td>{_}Time As{/_}:</td>
		<td>
			{html_options name=busystatus options=$busystatusa selected=$busystatus}
		</td>
	</tr>
	<tr>
		<td>{_}Reminder{/_}:</td>
		{if $reminder}
			<td><input type="checkbox" name="reminder" checked> 
		{else}
			<td><input type="checkbox" name="reminder"> 
		{/if}
		{if $reminder_minutes}
			<input type="text" size="2" name="reminder_minutes" value="{$reminder_minutes}"> {_}Minutes{/_}</td>
		{else}
			<input type="text" size="2" name="reminder_minutes" value="15"> {_}Minutes{/_}</td>
		{/if}
	</tr>
	
	<tr>
		<td><input type="submit" value="{_}Modify{/_}"></td>
		<td><input type="reset" value="Reset"></td>
	</tr>
</table>
</form>
{include file="footer.tpl"}
