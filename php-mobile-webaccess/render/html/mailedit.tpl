{include file="header.tpl" title="Edit Email"}
<a href="index.php?entryid={$parent_entryid}&type=table">{$parentname|escape}</a>
<div class="line"></div>
<form method="post" action="index.php?type=action&action=new">
<input type="hidden" name="type" value="IPM.Note">
<input type="hidden" name="entryid" value="{$entryid}">
<table>
	<tr>
	<td rowspan="2"><input type="submit" name="send" value="{_}Send{/_}"></td>
	<td>To:</td>
	<td><input type="text" name="recipients" size="12"></td>
	
</tr>
<tr>
	<td>Subject:</td>
	{if $subject} {*if subject is set for reply/forward purpsoses, show it*}
		<td><input type="text" name="subject" size="12" value="{$subject|escape}"></td>
	{else}
		<td><input type="text" name="subject" size="12"></td>
	{/if}
</tr>
<tr>
	{if $body} {*if body is set for reply/forward purpsoses, show it*}
		<td colspan="3" align="center"><textarea cols="25" rows="9" name="body">{$body|escape}</textarea></td>
	{else}
		<td colspan="3" align="center"><textarea cols="25" rows="9" name="body"></textarea></td>
	{/if}

</tr>
<tr>
<td colspan="3" align="center"><input type="submit" name="draft" value="{_}Save draft{/_}">

</tr>

</table>
</form>
{include file="footer.tpl"}
