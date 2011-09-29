{include file="header.tpl" title="New Email"}
<a href="index.php?entryid={$parent_entryid}&type=table">{$parentname|escape}</a>
<div class="line"></div>
<form method="post" action="index.php?type=action&action=new">
<input type="hidden" name="type" value="IPM.Note">
{if $conversation_index}
<input type="hidden" name="conversation_index" value="{$conversation_index}">
{/if}
<table>
	<tr>
		<td>To:</td>
{if $smarty.get.to}
		<td><input type="text" name="recipients" size="18" value="{getemailaddresses}{$smarty.get.to|escape}{/getemailaddresses}"></td>
{elseif $sent_representing_smtp_address && $action=="reply"}
		<td><input type="text" name="recipients" size="18" value="{getemailaddresses}{$sent_representing_smtp_address|escape}{/getemailaddresses}"></td>
{else}
		<td><input type="text" name="recipients" size="18"></td>
{/if}	
	</tr>
	<tr>
		<td>Subject:</td>
{if $subject} {*if subject is set for reply/forward purpsoses, show it*}
		<td><input type="text" name="subject" size="18" value="{$subject|escape}"></td>
{else}
		<td><input type="text" name="subject" size="18"></td>
{/if}
	</tr>
	<tr>
{if $body} {*if body is set for reply/forward purpsoses, show it*}
		<td colspan="2"><textarea cols="30" rows="9" name="body">{$body|escape}</textarea></td>
{else}
		<td colspan="2"><textarea cols="30" rows="9" name="body"></textarea></td>
{/if}
	</tr>
	<tr>
		<td><input type="submit" name="send" value="{_}Send{/_}"></td>
		<td><input type="submit" name="draft" value="{_}Save draft{/_}"></td>
	</tr>
</table>
</form>
{include file="footer.tpl"}
