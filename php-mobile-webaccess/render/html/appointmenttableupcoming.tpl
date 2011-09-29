{include file="header.tpl" title=$display_name}
<a href="index.php"><img src="images/icon_home.gif" alt="home"> {$parentname|escape}</a>
<div class="line"></div>
{include file="barappdaychooser.tpl" listdate=$listdate dates=$dates entryid=$entryid nextdate=$nextdate prevdate=$prevdate}

{foreach from=$days item=day}
<table width="100%" cellspacing="0">
	{if $day.appointments}
	{assign var="apps" value=0}
		<tr><td width="180" class="thinline">{$day.0|date_format:"%Y-%m-%d"}</td><td class="thinline"><a href="{$smarty.server.PHP_SELF}?type=new&item=appointment&day={$day.0|date_format:"%Y%m%d"}">+</a></td></tr>
		{foreach from=$day.appointments item=appointment}
			{math equation="x+1" x=$apps assign=apps} 
			<tr><td colspan="2">{$appointment.start|date_format:"%H:%M"}-{$appointment.end|date_format:"%H:%M"}</td></tr>
			{if $apps==$day.apps}
				<tr><td colspan="2" class="eindeappointment"><a href="index.php?entryid={$appointment.entryid}&type=msg&task=IPM.Appointment">{$appointment.subject|escape}</a></td></tr>
			{else}
				<tr><td colspan="2" class="thinline"><a href="index.php?entryid={$appointment.entryid}&type=msg&task=IPM.Appointment">{$appointment.subject|escape}</a></td></tr>
			{/if}
		{/foreach}
	{else}
		<tr><td width="180" class="eindeappointment">{$day.0|date_format:"%Y-%m-%d"}</td><td class="eindeappointment"><a href="{$smarty.server.PHP_SELF}?type=new&item=appointment&day={$day.0|date_format:"%Y%m%d"}">+</a></td></tr>
	{/if}
</table>
{/foreach}
{include file="barappview.tpl"}
{include file='newbar.tpl'}
