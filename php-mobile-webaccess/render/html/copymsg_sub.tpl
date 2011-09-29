{section name=loop start=0 loop=$level}
{/section}
{if $parent_entryid==$subfolder.entryid} 
	<option value="{$subfolder.entryid}" selected>&nbsp;&nbsp;{$subfolder.name|escape}</option>
{else}
	<option value="{$subfolder.entryid}">&nbsp;&nbsp;{$subfolder.name|escape}</option>
{/if}
{if $subfolder.subfolders[0]}
	{math equation="x+1" x=$level assign=level} 
	{foreach from=$subfolder.subfolders item=subfolderz}
	   		{include file="copymsg_sub.tpl" subfolder=$subfolderz level=$level parent_entryid=$parent_entryid}
	{/foreach}
{/if}