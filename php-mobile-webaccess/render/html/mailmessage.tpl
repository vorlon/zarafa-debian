{include file="header.tpl" title=$subject}
{_}Back to{/_}: <a href="index.php?entryid={$parent_entryid}&type=table&task={$parentclass}">{$parentname|escape}</a>
<div class="line"></div>
<a href="index.php?entryid={$entryid}&type=reply&item=IPM.Note">Reply</a> <a href="index.php?entryid={$entryid}&type=forward&item=IPM.Note">Forward</a>
<a href="index.php?entryid={$entryid}&type=copy&item=IPM.Note">Copy/Move</a>
<a href="index.php?entryid={$entryid}&type=delete&item=IPM.Note">Delete</a>
<div class="line"></div>
<table>
	<tr>
		<td width="50" align="right">From: </td>
		{if $sent_representing_name}
		<td>"{$sent_representing_name|escape}"</td>
	</tr>
	<tr>
		<td></td>
		{/if}
		<td>&lt;{toemailaddress}{$sent_representing_smtp_address|escape}{/toemailaddress}&gt;</td>
	</tr>
	<tr>
		<td width="50" align="right">{_}To{/_}: </td>
		<td>{toemailaddress}{$display_to|escape}{/toemailaddress}</td>
	</tr>
	{if $display_cc}
		<tr>
			<td width="50" align="right">{_}CC{/_}: </td>
			<td>{$display_cc|escape}</td>
		</tr>
	{/if}
	<tr>
		<td align="right">{_}Subject{/_}: </td>
		<td>{$subject|escape}</td>
	</tr>
	<tr>
		<td align="right">{_}Sent{/_}: </td>
			<td>{$message_delivery_time|escape}</td>
	</tr>
	{if $hasattach}
	<tr><td></td><td><a href="#attachments">{_}Go to attachments{/_}</a></td></tr>
	{/if}
</table>
<div class="line"></div>		
{$bodyf|escape|nl2br}
<div class="line"></div>
{if $hasattach}
	<a name="attachments"></a>
	{foreach from=$attachments item=attachment}
	<a href="index.php?entryid={$entryid}&type=attachment&attachnum={$attachment.attach_num}">{$attachment.attach_name|escape} ({$attachment.attach_size}KB)</a><br>
	{/foreach}
	<div class="line"></div>
{/if}
<a href="index.php?entryid={$entryid}&type=reply&item=IPM.Note">Reply</a> <a href="index.php?entryid={$entryid}&type=forward&item=IPM.Note">Forward</a>
<a href="index.php?entryid={$entryid}&type=copy&item=IPM.Note">Copy/Move</a>
<a href="index.php?entryid={$entryid}&type=delete&item=IPM.Note">Delete</a>
{include file='newbar.tpl'}
{include file="footer.tpl"}
