<?php

$smarty->register_block("categories", "smarty_categories");

function smarty_categories($params, $content, &$smarty, &$repeat)
{
	if (isset($params["cat"])) {
		$cats = "";
		foreach ($params["cat"] as $key => $value)
		{
			$cats .= $value.", ";
		}
		$cats = substr ($cats, 0, -2);
		return ($cats);
	}
}
?>
