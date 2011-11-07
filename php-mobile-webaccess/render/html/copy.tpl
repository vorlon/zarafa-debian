{include file="header.tpl" title="Copy/Move"}
Copy/move<br>
{$subject|escape}<br>
<form method="POST" action="index.php?entryid={$entryid}&action=copy&type=action">
<input type="hidden" name="entryid" value="{$entryid}">
<input type="radio" name="action" value="copy" checked> Copy<br>
<input type="radio" name="action" value="move"> Move<br>
Destination: 
<select name="tofolder">
{section name=mysec loop=$folders}
	{if  $folders[mysec].entryid == $parent_entryid}
		<option value="{$folders[mysec].entryid}" selected>{$folders[mysec].name|escape}</option>
	{else}
		<option value="{$folders[mysec].entryid}">{$folders[mysec].name|escape}</option>
	{/if}
	{if $folders[mysec].subfolders[0]}
		{section name=counter loop=$folders[mysec].subfolders}
		{include file="copymsg_sub.tpl" subfolder=$folders[mysec].subfolders[counter] level=1 parent_entryid=$parent_entryid}
		{/section}
	{/if}
{/section}
</select>

<input type="submit" value="{_}Go{/_}">
</form>
{include file="footer.tpl"}
