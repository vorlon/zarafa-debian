<?php

$smarty->register_block("callphonenumber", "smarty_callphonenumber");
$smarty->register_block("smsphonenumber", "smarty_smsphonenumber");

function smarty_callphonenumber($params, $content, &$smarty, &$repeat)
{
	if (isset($content)) {
		$number =str_replace(" ", "", $content);
		$number =str_replace("-", "", $number);
		$url = "wtai://wp/mc;".$number;
		$href = "<a href=\"".$url."\">".$content."</a>";
		return $href;
	}
}

function smarty_smsphonenumber($params, $content, &$smarty, &$repeat)
{
	if (isset($content)) {
		$number =str_replace(" ", "", $content);
		$url = "sms:".$number;
		$href = "<a href=\"".$url."\">".$content."</a>";
		return $href;
	}
}
?>