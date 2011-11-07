{include file="header.tpl" title=$display_name}
<a href="index.php?entryid={$parent_entryid}&type=table&task={$parentclass}">{$parentname|escape}</a>
<div class="line"></div>
<b>{$display_name|escape}</b><br>
{if $title}
	{$title|escape}<br>
{/if}
{if $company_name}
	{$company_name|escape}<br>
{/if}
<div class="line"></div>
{if $cellular_telephone_number}
	<table width="100%">
		<tr>
			<td rowspan="2" width="20px" align="center"><img src="images/icon_phone.gif"></td>
			<td><b>{_}Mobile Tel{/_}</b></td>
		</tr>
		<tr>
			<td>{callphonenumber}{$cellular_telephone_number|escape}{/callphonenumber}</td>
		</tr>
	</table>
	<div class="line"></div>
{/if}
{if $office_telephone_number}
	<table width="100%">
		<tr>
			<td rowspan="2" width="20px" align="center"><img src="images/icon_worktel.gif"></td>
			<td><b>{_}Work Tel{/_}</b></td>
		</tr>
		<tr>
			<td>{callphonenumber}{$office_telephone_number|escape}{/callphonenumber}</td>
		</tr>
	</table>
	<div class="line"></div>
{/if}
{if $home_telephone_number}
	<table width="100%">
		<tr>
			<td rowspan="2" width="20px" align="center"><img src="images/icon_hometel.gif"></td>
			<td><b>{_}Home Tel{/_}</b></td>
		</tr>
		<tr>
			<td>{callphonenumber}{$home_telephone_number|escape}{/callphonenumber}</td>
		</tr>
	</table>
	<div class="line"></div>
{/if}
{if $cellular_telephone_number}
	<table width="100%">
		<tr>
			<td rowspan="2" width="20px" align="center"><img src="images/icon_sms.gif"></td>
			<td><b>{_}Send SMS{/_}</b></td>
		</tr>
		<tr>
			<td>{smsphonenumber}{$cellular_telephone_number|escape}{/smsphonenumber}</td>
		</tr>
	</table>
	<div class="line"></div>
{/if}
{if $email_address_1}
	<table width="100%">
		<tr>
			<td rowspan="2" width="20px" align="center"><img src="images/icon_mailread.gif"></td>
			<td><b>{_}Send E-mail{/_}</b></td>
		</tr>
		<tr>
			<td>{toemailaddress}{$email_address_1|escape}{/toemailaddress}</td>
		</tr>
	</table>
	<div class="line"></div>
{/if}
{if $webpage}
	<table width="100%">
		<tr>
			<td rowspan="2" width="20px" align="center"><img src="images/icon_globe.gif"></td>
			<td><b>{_}Browse Web Page{/_}</b></td>
		</tr>
		<tr>
			<td>{weblink}{$webpage}{/weblink}</td>
		</tr>
	</table>
	<div class="line"></div>
{/if}

<table>
{if $business_address}
	<tr>
			<td width="100" valign="top">{_}Work Address{/_}:</td>
			<td>{$business_addressf|escape|nl2br}</td>
	</tr>
{/if}
{if $office_location}
	<tr>
			<td width="100" valign="top">{_}Office Location{/_}:</td>
			<td>{$office_location|escape|nl2br}</td>
	</tr>
{/if}
{if $home_address}
	<tr>
			<td width="100" valign="top">{_}Home Address{/_}:</td>
			<td>{$home_addressf|escape|nl2br}</td>
	</tr>
{/if}
{if $business_fax_number}
	<tr>
			<td width="100" valign="top">{_}Work Fax{/_}:</td>
			<td>{$business_fax_number|escape}</td>
	</tr>
{/if}
{if $department_name}
	<tr>
			<td width="100" valign="top">{_}Department{/_}:</td>
			<td>{$department_name|escape}</td>
	</tr>
{/if}
{if $categories}
	<tr>
			<td width="100" valign="top">{_}Categories{/_}:</td>
			<td>{categories cat=$categories|escape}{/categories}</td>
	</tr>
{/if}
</table>
<a href="index.php?entryid={$entryid}&item=IPM.Contact&type=edit">{_}Modify{/_}</a>
{include file="footer.tpl"}
