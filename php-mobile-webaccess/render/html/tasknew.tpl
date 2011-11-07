{include file="header.tpl" title="New Task"}
<a href="index.php?entryid={$parent_entryid}&type=table">{$parentname|escape}</a>
<div class="line"></div>
<form method="post" action="index.php?type=action&action=new">
<input type="hidden" name="type" value="IPM.Task">

<table>
	<tr>
		<td>{_}Subject{/_}</td>
		<td><input type="text" name="subject" size="15"></td>
	</tr>
	<tr>
		<td>{_}Priority{/_}:</td>
		<td>
			<select name="priority">
				<option value="-1">{_}Non urgent{/_}</option>
				<option value="0" selected>{_}Normal{/_}</option>
				<option value="1">{_}Urgent{/_}</option>
			</select>
		</td>
	
	</tr>
	<tr>
		<td>{_}Status{/_}</td>
		<td>{html_options name=status options=$statusa selected=0}</td>
	</tr>
	<tr>
		<td>{_}Start date{/_}</td>
		<td><input type="checkbox" name="startdate">
			<input type="text" name="startday" value="{$smarty.now|date_format:'%d'}" size="2">
			<input type="text" name="startmonth" value="{$smarty.now|date_format:'%m'}" size="2">
			<input type="text" name="startyear" size="4" value="{$smarty.now|date_format:'%Y'}">
		</td>
	</tr>
	<tr>
		<td>{_}Due date{/_}</td>
		<td><input type="checkbox" name="duedate">
			<input type="text" name="dueday" value="{$smarty.now|date_format:'%d'}" size="2">
			<input type="text" name="duemonth" value="{$smarty.now|date_format:'%m'}" size="2">
			<input type="text" name="dueyear" size="4" value="{$smarty.now|date_format:'%Y'}">
		</td>
	</tr>
	<tr>
		<td>{_}Reminder{/_}</td>
		<td><input type="checkbox" name="reminder"></td>
	</tr>
	<tr>
		<td>{_}Remin. date{/_}:</td>
		<td>
			<input type="text" name="remindday" value="{$smarty.now|date_format:'%d'}" size="2">
			<input type="text" name="remindmonth" value="{$smarty.now|date_format:'%m'}" size="2">
			<input type="text" name="remindyear"value="{$smarty.now|date_format:'%Y'}" size="4">
		</td>
	</tr>
		<td>{_}Remin. time{/_}:</td>
		<td>
			<input type="text" name="remindhour" value="{$smarty.now|date_format:'%H'}" size="2">
			<input type="text" name="remindminute" value="{$smarty.now|date_format:'%M'}" size="2">
		</td>
			
	</tr>
	<tr>
		<td>{_}Sensitivity{/_}:</td>
		<td>{html_options name=sensitivity options=$sensitivitya selected=0}</td>
	</tr>
	<tr>
		<td></td>
		<td><input type="submit" value="{_}Create{/_}"></td>
	</tr>
</table>
</form>
{include file="footer.tpl"}
