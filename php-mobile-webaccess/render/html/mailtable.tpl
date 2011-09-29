{include file="header.tpl" title=$display_name}
<a href="index.php"><img src="images/icon_home.gif" alt="home" border="0"> {$parentname|escape}</a>
<div class="line"></div><br>
{if $items}
	<table width="100%">
	
	{section name=mysec loop=$items}
		<tr class="{msgflags flags=$items[mysec].message_flags flag="MSGFLAG_READ" val_true="msg_read" val_false="msg_unread"}">
			<td rowspan="2" width="15"><img src="images/{icon flags=$items[mysec].message_flags attach=$items[mysec].hasattach}"></td>
			{if $tabletype=="in"}
			<td width="50%">{$items[mysec].sent_representing_name|escape}</td>
			{else}
			<td width="50%">{$items[mysec].display_to|escape}</td>
			{/if}
			<td width="40%">{$items[mysec].message_delivery_time|escape}</td>
			<td width="10%">{$items[mysec].message_size}KB</td>
		</tr>
		<tr class="{msgflags flags=$items[mysec].message_flags flag="MSGFLAG_READ" val_true="msg_read" val_false="msg_unread"}">
			{if $tabletype=="drafts"}
				<td colspan="3"><a href="index.php?entryid={$items[mysec].entryid}&item=IPM.Note&type=edit">{$items[mysec].subject|escape}</a></td>		
			{else}
				<td colspan="3"><a href="index.php?entryid={$items[mysec].entryid}&type=msg">{$items[mysec].subject|escape}</a></td>
			{/if}
		</tr>
		<tr>
			<td colspan="4"><div class="line"></div></td>			
		</tr>			
	{/section}
	
	
	</table>
{else}
{_}No Items{/_}
{/if}
{if $smarty.get.start > 0 && $items}
<a href="{$smarty.server.PHP_SELF}?entryid={$smarty.get.entryid}&type=table&start={$smarty.get.start-10}">Prev</a>
{/if}
{if $items}
<a href="{$smarty.server.PHP_SELF}?entryid={$smarty.get.entryid}&type=table&start={$smarty.get.start+10}">Next</a>
{/if}
{include file='newbar.tpl'}
{include file="footer.tpl"}
