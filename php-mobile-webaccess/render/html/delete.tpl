{include file="header.tpl" title="Delete message"}

<p>{_}Are you sure you want to delete the following message{/_}:</p>
<p>{$subject|escape}</p>

<table>
	<tr>
		<td>
			<form method="POST" action="index.php?entryid={$entryid}&action=delete&type=action">
				<input type="hidden" name="entryid" value="{$entryid}">
				<input type="hidden" name="message_class" value="{$message_class}">
				<input type="hidden" name="delete" value="yes">
				<input type="submit" value="{_}Delete{/_}">
			</form>
		</td>
		<td>
			<form method="POST" action="index.php?entryid={$entryid}&action=delete&type=action">
				<input type="hidden" name="entryid" value="{$entryid}">
				<input type="hidden" name="message_class" value="{$message_class}">
				<input type="submit" value="{_}Cancel{/_}">
			</form>
		</td>
	</tr>
</table>
{include file="footer.tpl"}
