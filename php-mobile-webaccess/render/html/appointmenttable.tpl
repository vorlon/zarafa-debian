{include file="header.tpl" title=$display_name}
<a href="index.php"><img src="images/icon_home.gif" alt="home"> {$parentname|escape}</a>
<div class="line"></div>
{include file="barappdaychooser.tpl" listdate=$listdate dates=$dates entryid=$entryid nextdate=$nextdate prevdate=$prevdate}
<table width="100%" cellpadding="0" cellspacing="0">
{section name=mysec loop=$items}
	{if $items[mysec].subject == "No appointments."}
		<tr>				
			<td width="100%" colspan="3" class="{$items[mysec].div}">{_}{$items[mysec].subject|escape}{/_}</td>
		</tr>
		<tr>
			<td width="100%" colspan="3" class="eindeappointment"></td>
		</tr>
	{elseif $items[mysec].div== "allday"}
		<tr>				
			<td width="100%" colspan="3" class="{$items[mysec].div}"><a href="index.php?entryid={$items[mysec].entryid}&type=msg&task=IPM.Appointment">{$items[mysec].subject|escape} {$items[mysec].locationt|escape}</a></td>
		</tr>
	{else}
		<tr>				
			<td width="100%" colspan="3" class="{$items[mysec].div}"><a href="index.php?entryid={$items[mysec].entryid}&type=msg&task=IPM.Appointment">{$items[mysec].subject|escape}</a></td>
		</tr>
		<tr>	
			<td width="75" class="eindeappointment">{$items[mysec].start|date_format:"%H:%M"}-{$items[mysec].end|date_format:"%H:%M"}</td>
			<td width="100" class="eindeappointment">{if !defined($items[mysec])}&nbsp;{else}{$items[mysec].locationt|escape}{/if}</td>
			<td width="45" class="eindeappointment">&nbsp;</td>
		</tr>

	{/if}		
{/section}
</table>
{include file="barappview.tpl"}
{include file='newbar.tpl'}
