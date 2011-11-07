<?php

$smarty->register_block("getemailaddresses", "smarty_getmailaddresses");
$smarty->register_block("toemailaddress", "smarty_toemailaddress");
$smarty->register_block("formateditaddress", "smarty_formateditaddress");

function smarty_getmailaddresses($params, $content, &$smarty, &$repeat)
{
	if (isset($content)) {
		$pattern = "/.*?([a-zA-Z0-9\._\-]+@[a-zA-Z0-9]+[a-zA-Z0-9-\.]*\.[a-zA-Z]{2,6})/";
		preg_match_all($pattern, $content, $out);
		
		foreach ($out[1] as $key => $address)
		{
			$list .=$address."; ";
		}
		$list = substr ($list, 0, -2);
		return ($list);
	}
}

function smarty_toemailaddress($params, $content, &$smarty, &$repeat)
{
	if (isset($content)) {
		$pattern = "/.*?([a-zA-Z0-9\._\-]+@[a-zA-Z0-9]+[a-zA-Z0-9-\.]*\.[a-zA-Z]{2,6})/";
		preg_match_all($pattern, $content, $out);
		
		foreach ($out[1] as $key => $address)
		{
			$list .=$address."; ";
		}
		$list = substr ($list, 0, -2);
		if ($list)
		{
			$url = "<a href=\"index.php?type=new&item=email&to=$content\">$content</a>";
		}
		else {
			$url = $content;
		}
		return $url;
	}
}

function smarty_formateditaddress($params, $content, &$smarty, &$repeat)
{
	if (isset($content)) {
		$output = br2ln($content);
		return $output;
	}
}
?>
