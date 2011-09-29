{include file="header.tpl" title=$display_name}
<a href="index.php"><img src="images/icon_home.gif" alt="home"> {$parentname|escape}</a>
<div class="line"></div>
{include file="barappdaychooser.tpl" listdate=$listdate dates=$dates entryid=$entryid nextdate=$nextdate prevdate=$prevdate}

{if $allday}
	<table cellpadding="0" cellspacing="0" width="220">
	{foreach from=$allday item=appointment}
		<tr><td class="allday">{$appointment.subject|escape}</td></tr>
	{/foreach}
	</table>
{/if}
<table cellpadding="0" cellspacing="0">
{section name=bar loop=$cellsday step=1 start=0} {*running through timeblocks*}

	{if ($smarty.section.bar.index > (9*$cellshour-1)) && ($smarty.section.bar.index<(17*$cellshour)) }
		{if ($smarty.section.bar.index%$cellshour==($cellshour-1))}
			{assign var="style" value=" class=\"lastrow\""}
		{else}
			{assign var="style" value=" class=\"normalrow\""}
		{/if}
		{assign var="styletr" value="officetime"}
	{else}
		{if ($smarty.section.bar.index%$cellshour==($cellshour-1))}
			{assign var="style" value=" class=\"outsidelastrow\""}
		{else}
			{assign var="style" value=" class=\"outsidenormalrow\""}
		{/if}
		{assign var="styletr" value="nonofficetime"}
	{/if}
	
	{if $smarty.section.bar.index%$cellshour==0} {*only on full hour time show*}
		<tr class="{$styletr}">
			<td width="40"{$style}><a name="{$smarty.section.bar.index/$cellshour}" href="{$smarty.server.PHP_SELF}?type=new&item=appointment&day={$date|date_format:"%Y%m%d"}&hour={$smarty.section.bar.index/$cellshour}">{$smarty.section.bar.index/$cellshour}:00</a></td>
	{else}
		<tr>
			<td{$style}>&nbsp;</td>
	{/if}
	
	{section name=bars loop=$bars} {*running through bars*}
		{assign var="bar" value=$bars[$smarty.section.bars.index]}
		{assign var="output" value="false"}
		{foreach from=$bar item=item} {*running through appointments*}
			
				{if ($smarty.section.bar.index>=$item.startblock) && ($smarty.section.bar.index<($item.startblock+$item.length))} {*check if timeblock is inside appointment*}
					
					{if $smarty.section.bar.index == $item.startblock} {*timeblock same as appointment start so show it*}
						<td rowspan="{$item.length}" id="{$smarty.section.bars.index}" width="{$barwidth}%" class="appointment"><input type="button" value="{$item.subject|escape}" style="width: 100%; height: {$item.length*23}px; text-align:center; border: 2px solid #00f; margin-left: 2px;" onclick="gotoUrl('{$item.entryid}')" class="appointment"></td>
						{assign var="output" value="true"}
					{elseif $smarty.section.bar.index<($item.startblock+$item.length) && $output=="false"} {*timeblock is inside appointment so dont make new cell*}
						{assign var="output" value="true"}
					{else}
						<td id="{$smarty.section.bars.index}"{$style} width="{$barwidth}%">&nbsp;</td> {*outside appointment*}
						{assign var="output" value="true"}
					{/if}
					
				{/if}
						
		{/foreach}
		
		{if $output=="false"}
			<td id="{$smarty.section.bars.index}"{$style} width="{$barwidth}%">&nbsp;</td> {*outside appointments*}
		{/if}
	{/section}
	</tr>

{/section}

	
</table>
	
{include file="barappview.tpl"}
{include file='newbar.tpl'}
