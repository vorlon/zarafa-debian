{include file="header.tpl" title=$subject}
<a href="index.php?entryid={$parent_entryid}&type=table&task={$parentclass}">{$parentname|escape}</a>
<div class="line"></div>
<table width="220">
	<tr>
		<td><b>{$subject|escape}</b></td>
		<td>
			<div style="text-align: right;">
			{if $reminder}
				<img src="images/icon_reminder.gif" width="19" height="14">
			{/if}
			{if $private}
				<img src="images/icon_private.gif" width="15" height="14">
			{/if}
			</div>
		</td>
	</tr>
	<tr><td>&nbsp;</td><td></td></tr>
	<tr>
		<td colspan="2">
			{if $alldayevent}
			{_}All Day Event{/_} - {$startdate|date_format:"%d-%m-%Y"}
			{else}
			{$startdate|date_format:"%H:%M"} - {$duedate|date_format:"%H:%M"} {_}{$startdate|date_format:"%a"}{/_}, {$startdate|date_format:"%d/%m/%Y"}
			{/if}
		</td>
	</tr>
	<tr>
		<td colspan="2">
		{if $location}
			{$location|escape}
		{/if}
		</td>
	</tr>
</table>
<table>
	{if $contacts_string}
		<tr>
			<td>{_}Attendees{/_}:</td>
			<td>{$contacts_string|escape}</td>
		</tr>
	{/if}
	{if $categories}
		<tr>
			<td>{_}Categories{/_}:</td>
			<td>{categories cat=$categories|escape}{/categories}</td>
		</tr>
	{/if}
	{if $busystatus >= 0 && $busystatus!=2}
	<tr>
		<td>{_}Busy Status:{/_}</td>
		{if $busystatus == 0}
			<td>Free</td>
		{elseif $busystatus == 1}
			<td>{_}Tentative{/_}</td>
		{elseif $busystatus == 2}
			<td>{_}Busy{/_}</td>
		{elseif $busystatus == 3}
			<td>{_}Out of Office{/_}</td>
		{/if}	
	</tr>
	{/if}
</table>
<div class="line"></div>
{if $body}
{$body|escape}
<div class="line"></div>
{/if}
{if $hasattach}
	<a name="attachments"></a>
	{foreach from=$attachments item=attachment}
	<a href="index.php?entryid={$entryid}&type=attachment&attachnum={$attachment.attach_num}">{$attachment.attach_name|escape} ({$attachment.attach_size}KB)</a><br>
	{/foreach}
	<div class="line"></div>
{/if}
<a href="index.php?entryid={$entryid}&item=IPM.Appointment&type=edit">{_}Modify{/_}</a>
{include file="footer.tpl"}
