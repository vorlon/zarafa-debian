<div class="line"></div>
<table>
	<tr>
		<td>{_}New{/_}: </td>
		<td><a href="{$smarty.server.PHP_SELF}?type=new&item=email"><img src="images/icon_createmail.gif" width="16" height="15" alt="{_}New Email{/_}" border="0"></a></td>
		{if $smarty.get.day}
			<td><a href="{$smarty.server.PHP_SELF}?type=new&item=appointment&day={$smarty.get.day}"><img src="images/icon_calendar.gif" width="16" height="15" alt="{_}New Appointment{/_}" border="0"></a></td>
		{else}
			<td><a href="{$smarty.server.PHP_SELF}?type=new&item=appointment"><img src="images/icon_calendar.gif" width="16" height="15" alt="{_}New Appointment{/_}" border="0"></a></td>
		{/if}
		<td><a href="{$smarty.server.PHP_SELF}?type=new&item=contact"><img src="images/icon_contact.gif" width="16" height="15" alt="{_}New Contact{/_}" border="0"></a></td>
		<td><a href="{$smarty.server.PHP_SELF}?type=new&item=task"><img src="images/icon_task.gif" width="16" height="15" alt="{_}New Task{/_}" border="0"></a></td>
	</tr>
</table>
