{include file="header.tpl" title=$subject}
<a href="index.php?entryid={$parent_entryid}&type=table&task=IPF.Task">{$parentname|escape}</a>
<div class="line"></div>
<form method="post" action="index.php?type=action&action=save">
<input type="hidden" name="type" value="IPM.Task">
<input type="hidden" name="entryid" value="{$entryid}">

<table>
	<tr>
		<td>{_}Subject{/_}</td>
		<td><input type="text" name="subject" size="15" value="{$subject|escape}"></td>
	</tr>
	<tr>
		<td>{_}Priority{/_}:</td>
		<td>{html_options name=priority options=$prioritya selected=$priority}
		</td>
	
	</tr>
	<tr>
		<td>{_}Status{/_}</td>
		<td>{html_options name=status options=$statusa selected=$status}</td>
	</tr>
	<tr>
		<td>{_}Complete (perc){/_}</td>
		<td><input type="text" name="percent_complete" size="3" value="{$percent_complete*100}"> %</td>
	</tr>
	<tr>
		<td>{_}Start date{/_}</td>
		<td><input type="checkbox" name="startdate">
			<input type="text" name="startday" value="{$startdate|date_format:'%d'}" size="2">
			<input type="text" name="startmonth" value="{$startdate|date_format:'%m'}" size="2">
			<input type="text" name="startyear" size="4" value="{$startdate|date_format:'%Y'}">
		</td>
	</tr>
	<tr>
		<td>{_}Due date{/_}</td>
		<td><input type="checkbox" name="duedate">
			<input type="text" name="dueday" value="{$duedate|date_format:'%d'}" size="2">
			<input type="text" name="duemonth" value="{$duedate|date_format:'%m'}" size="2">
			<input type="text" name="dueyear" size="4" value="{$duedate|date_format:'%Y'}">
		</td>
	</tr>
	<tr>
		<td>{_}Reminder{/_}</td>
		{if $reminder}
			<td><input type="checkbox" name="reminder" checked></td>
		{else}
			<td><input type="checkbox" name="reminder"></td>
		{/if}
	</tr>
	<tr>
		<td>{_}Remin. date{/_}:</td>
		<td>
			<input type="text" name="remindday" value="{$reminderdate|date_format:'%d'}" size="2">
			<input type="text" name="remindmonth" value="{$reminderdate|date_format:'%m'}" size="2">
			<input type="text" name="remindyear"value="{$reminderdate|date_format:'%Y'}" size="4">
		</td>
	</tr>
		<td>{_}Remin. time{/_}:</td>
		<td>
			<input type="text" name="remindhour" value="{$reminderdate|date_format:'%H'}" size="2">
			<input type="text" name="remindminute" value="{$reminderdate|date_format:'%M'}" size="2">
		</td>
			
	</tr>
	<tr>
		<td>{_}Sensitivity{/_}:</td>
		<td>{html_options name=sensitivity options=$sensitivitya selected=$sensitivity}</td>
	</tr>
	<tr>
		<td></td>
		<td><input type="submit" value="{_}Modify{/_}"></td>
	</tr>
</table>
</form>
{include file="footer.tpl"}
