{include file="header.tpl" title="Zarafa Mobile"}
<table>
{section name=mysec loop=$folders}

	<tr>
				{if $folders[mysec].subfolders}
					{if $folders[mysec].subfolders[0]}
						<td><a href="index.php" class="subfolder">[-]</a></td>
					{else}
						<td><a href="index.php?entryid={$folders[mysec].entryid}" class="subfolder">[+]</a></td>
					{/if}
				{else}
					<td></td>
				{/if}
		<td>
			<a href="index.php?entryid={$folders[mysec].entryid}&type=table&task={$folders[mysec].container_class}">{$folders[mysec].name|escape}
			{if $folders[mysec].unread > 0}
				<b> ({$folders[mysec].unread})</b>
			{/if}
			</a>
		</td>
		
	</tr>
			{if $folders[mysec].subfolders[0]}
				{section name=counter loop=$folders[mysec].subfolders}
				{include file="hierarchysub.tpl" subfolder=$folders[mysec].subfolders[counter] level=1}
				{/section}
			
		{/if}

{/section}
</table>
<div class="line"></div>
<a href="index.php?type=logout&entryid=">{_}Log out{/_}</a>

{include file='newbar.tpl'}
{include file="footer.tpl"}
