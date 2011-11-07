{include file="header.tpl" title=$display_name}
<a href="index.php"><img src="images/icon_home.gif" alt="home"> {$parentname|escape}</a>
<table width="100%">
<tr><td><div class="line"></div></td></tr>
{section name=contact loop=$items}
	<tr><td><b><a href="index.php?entryid={$items[contact].entryid}&type=msg&task=IPM.Contact">{$items[contact].fileas|escape}</a></b></td></tr>
	<tr>
		{if $items[contact].cellular_telephone_number}
			<td>{callphonenumber|escape}{$items[contact].cellular_telephone_number}{/callphonenumber}</td>
		{elseif $items[contact].email_address_1}
			<td>{toemailaddress}{$items[contact].email_address_1|escape}{/toemailaddress}</td>
		{/if}	
	</tr>
	<tr><td><div class="line"></div></td></tr>
{/section}
</table>
{include file='newbar.tpl'}
{include file="footer.tpl"}
