
	<tr>
	<td></td>
	<td>
	{section name=loop start=0 loop=$level}
	&nbsp;&nbsp;&nbsp;
	{/section}
	<a href="index.php?entryid={$subfolder.entryid}&type=table&task={$subfolder.container_class}">{$subfolder.name|escape}
	{if $subfolder.unread > 0}
					<b> ({$subfolder.unread})</b>
	{/if}
	</a></td></tr>
	{if $subfolder.subfolders[0]}
		{math equation="x+1" x=$level assign=level} 
		{foreach from=$subfolder.subfolders item=subfolderz}
		   		{include file="hierarchysub.tpl" subfolder=$subfolderz level=$level}
		{/foreach}
	{/if}

