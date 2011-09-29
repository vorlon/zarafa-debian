{include file="header.tpl" title=$display_name}
<a href="index.php?entryid={$parent_entryid}&type=table">{$parentname|escape}</a>
<div class="line"></div>
<form method="post" action="index.php?type=action&action=save">
<input type="hidden" name="type" value="IPM.Contact">
<input type="hidden" name="entryid" value="{$entryid}">
<table>
	<tr>
		<td>{_}Name{/_}:</td>
		<td><input type="text" name="display_name" size="15" value="{$display_name}"></td>
	</tr>
	<tr>
		<td>{_}Jobtitle{/_}:</td>
		<td><input type="text" name="title" size="15" value="{$title}"></td>
	</tr>
	<tr>
		<td>{_}Department{/_}:</td>
		<td><input type="text" name="department_name" size="15" value="{$department_name}"></td>
	</tr>
	<tr>
		<td>{_}Company{/_}:</td>
		<td><input type="text" name="company_name" size="15" value="{$company_name}"></td>
	</tr>
	<tr>
		<td>{_}Work Tel{/_}:</td>
		<td><input type="text" name="office_telephone_number" size="15" value="{$office_telephone_number}"></td>
	</tr>
	<tr>
		<td>{_}Work Fax{/_}:</td>
		<td><input type="text" name="business_fax_number" size="15" value="{$business_fax_number}"></td>
	</tr>
	<tr>
		<td>{_}Work Addr{/_}:</td>
		<td><textarea name="business_address" cols="15" rows="3">{$business_address}</textarea></td>
	</tr>
	<tr>
		<td>{_}IM{/_}:</td>
		<td><input type="text" name="im" size="15" value="{$im}"></td>
	</tr>
	<tr>
		<td>{_}E-mail{/_}:</td>
		<td><input type="text" name="email_address_1" size="15" value="{$email_address_1}"></td>
	</tr>
	<tr>
		<td>{_}Mobile Tel{/_}:</td>
		<td><input type="text" name="cellular_telephone_number" size="15" value="{$cellular_telephone_number}"></td>
	</tr>
	<tr>
		<td>{_}Web page{/_}:</td>
		<td><input type="text" name="webpage" size="15" value="{$webpage}"></td>
	</tr>
	<tr>
		<td>{_}Office Loc{/_}:</td>
		<td><input type="text" name="office_location" size="15" value="{$office_location}"></td>
	</tr>
	<tr>
		<td>{_}Home tel{/_}:</td>
		<td><input type="text" name="home_telephone_number" size="15" value="{$home_telephone_number}"></td>
	</tr>
	<tr>
		<td>{_}Home Addr{/_}:</td>
		<td><textarea name="home_address" cols="15" rows="3">{$home_address}</textarea></td>
	</tr>
	<tr>
		<td>{_}Other Addr{/_}:</td>
		<td><textarea name="other_address" cols="15" rows="3">{$other_address}</textarea></td>
	</tr>
	<tr>
	<td colspan="2" align="center"><input type="submit" value="{_}Modify{/_}"></td>
	</tr>
</table>
</form>
{include file="footer.tpl"}
