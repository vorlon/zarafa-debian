{include file="header.tpl" title="New Contact"}
<a href="index.php?entryid={$parent_entryid}&type=table">{$parentname|escape}</a>
<div class="line"></div>
<form method="post" action="index.php?type=action&action=new">
<input type="hidden" name="type" value="IPM.Contact">
<table>
	<tr>
		<td>{_}Name{/_}:</td>
		<td><input type="text" name="display_name" size="15"></td>
	</tr>
	<tr>
		<td>{_}Jobtitle{/_}:</td>
		<td><input type="text" name="title" size="15"></td>
	</tr>
	<tr>
		<td>{_}Department{/_}:</td>
		<td><input type="text" name="department_name" size="15"></td>
	</tr>
	<tr>
		<td>{_}Company{/_}:</td>
		<td><input type="text" name="company_name" size="15"></td>
	</tr>
	<tr>
		<td>{_}Work Tel{/_}:</td>
		<td><input type="text" name="office_telephone_number" size="15"></td>
	</tr>
	<tr>
		<td>{_}Work Fax{/_}:</td>
		<td><input type="text" name="business_fax_number" size="15"></td>
	</tr>
	<tr>
		<td>{_}Work Addr{/_}:</td>
		<td><input type="text" name="business_address" size="15"></td>
	</tr>
	<tr>
		<td>{_}IM{/_}:</td>
		<td><input type="text" name="im" size="15"></td>
	</tr>
	<tr>
		<td>{_}E-mail{/_}:</td>
		<td><input type="text" name="email_address_1" size="15"></td>
	</tr>
	<tr>
		<td>{_}Mobile Tel{/_}:</td>
		<td><input type="text" name="cellular_telephone_number" size="15"></td>
	</tr>
	<tr>
		<td>{_}Web page{/_}:</td>
		<td><input type="text" name="webpage" size="15"></td>
	</tr>
	<tr>
		<td>{_}Office Loc{/_}:</td>
		<td><input type="text" name="office_location" size="15"></td>
	</tr>
	<tr>
		<td>{_}Home tel{/_}:</td>
		<td><input type="text" name="home_telephone_number" size="15"></td>
	</tr>
	<tr>
		<td>{_}Home Addr{/_}:</td>
		<td><input type="text" name="home_address" size="15"></td>
	</tr>
	<tr>
		<td>{_}Other Addr{/_}:</td>
		<td><input type="text" name="other_adress" size="15"></td>
	</tr>
	<tr>
	<td colspan="2" align="center"><input type="submit" value="{_}Create{/_}"></td>
	</tr>
</table>
</form>
{include file="footer.tpl"}
