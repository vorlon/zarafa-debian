<div class="line"></div>
<table>
	<tr>
		<td>{_}Change view{/_}: </td>
		<td><a href="{$smarty.server.PHP_SELF}?entryid={$smarty.get.entryid}&type=table&task=IPF.Appointment&appview=normal">{_}Agenda{/_}</a></td>
		<td><a href="{$smarty.server.PHP_SELF}?entryid={$smarty.get.entryid}&type=table&task=IPF.Appointment&appview=upcoming">{_}Upcoming{/_}</a></td>
		<td><a href="{$smarty.server.PHP_SELF}?entryid={$smarty.get.entryid}&type=table&task=IPF.Appointment&appview=day">{_}Day{/_}</a></td>
	</tr>
</table>
