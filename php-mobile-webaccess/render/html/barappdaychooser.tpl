<table>
	<tr class="eindeappointment">
		<td width="75px">{$listdate|date_format:"%d-%m-%Y"}</td>
		{section name=days loop=$dates}
			<td width="10px" class="{$dates[days].backtoday}"><a class="{$dates[days].backtoday}" href="{$smarty.server.PHP_SELF}?entryid={$entryid}&type=table&day={$dates[days].date}&task=IPF.Appointment">{smsd}{$dates[days].date}{/smsd}</a></td>
		{/section}
		<td width="10px"><a href="{$smarty.server.PHP_SELF}?entryid={$entryid}&type=table&day={$smarty.now|date_format:"%Y%m%d"}&task=IPF.Appointment">[T]</td>
		<td width="10px"><a href="{$smarty.server.PHP_SELF}?entryid={$entryid}&type=table&day={$prevdate|date_format:"%Y%m%d"}&task=IPF.Appointment">&lt;</a></td>
		<td width="8px" style="align:center;">|</td>
		<td width="10px"><a href="{$smarty.server.PHP_SELF}?entryid={$entryid}&type=table&day={$nextdate|date_format:"%Y%m%d"}&task=IPF.Appointment">&gt;</a></td>
	</tr>
</table>
<div class="line"></div>
