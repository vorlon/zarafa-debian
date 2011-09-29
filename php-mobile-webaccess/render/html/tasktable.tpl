{include file="header.tpl" title=$display_name}
<a href="index.php"><img src="images/icon_home.gif" alt="home"> {$parentname|escape}</a>
<div class="line"></div>
<table width="100%">
<tr><td width="15">[c]</td><td>Subject</td><td>Duedate</td></tr>
{section name=task loop=$items}
	<tr>
	
			
				<form method="post" action="index.php?entryid={$items[task].entryid}&action=update&type=action">
					<td width="15">
						<input type="hidden" name="entryid" value="{$items[task].entryid}">
						<input type="hidden" name="type" value="IPM.Task">
		
						<input type="checkbox" name="complete" onclick="this.form.submit();" {if $items[task].complete}checked{/if}>
					</td>
				</form>
			{if $items[task].complete}
				<td><div class="completedtask"><a href="index.php?entryid={$items[task].entryid}&type=edit&item=IPM.Task">{$items[task].subject|escape}</a><div></td>
				<td><div class="completedtask">{$items[task].duedate|date_format:"%d/%m/%Y"}</div></td>
			{else}				
				<td><a href="index.php?entryid={$items[task].entryid}&type=edit&item=IPM.Task">{$items[task].subject|escape}</a></td>
				<td>{$items[task].duedate|date_format:"%d/%m/%Y"}</td>
			{/if}
		
		
	</tr>
{/section}
</table>
<div class="line"></div>
<form method="post" action="index.php?type=action&action=new">
<input type="hidden" name="type" value="IPM.Task">
<input type="text" size="15" name="subject"><input type="submit" value="{_}Create{/_}">
</form>
{include file="newbar.tpl}
{include file="footer.tpl"}
