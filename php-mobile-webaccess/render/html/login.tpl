{include file="header.tpl" title="Zarafa Mobile"}
<form method="post" action="index.php">
<table>
	<tr>
		<td colspan="2">{$message|escape}</td>
	</tr>
	<tr>
		<td>{_}Username{/_}:</td>
		<td><input type="text" name="username" size="15" value="{if $username}{$username|escape}{/if}"></td>
	</tr>
	<tr>
		<td>{_}Password{/_}:</td>
		<td><input type="password" name="password" size="15"></td>
	</tr>
	<tr>
		<td colspan="2"><input type="checkbox" name="remember" id="remember"><label for="remember">{_}Store password in a cookie{/_}</label></td>
	</tr>
	<tr>
		<td><input type="submit" value="{_}Login{/_}"></td>
		<td></td>
	</tr>
</table>


</form>
{include file="footer.tpl"}
