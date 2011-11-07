<?php
$smarty->register_block("_", "smarty_gettext");
$smarty->register_block("smsd", "smarty_shortday");

function smarty_gettext($params, $content, &$smarty, &$repeat)
{
	
	if (isset($content)) {
//		$text = _($content);
		$text = $content; // disable gettext
		return $text;
	}

}

function smarty_shortday($params, $content, &$smarty, &$repeat)
{
	/**
	 * for some reason this function got called twice, the first time without any content so if statement
	 * this "solves" it.
	 */
	if (isset($content)) {
		$repeat = false;
		$date = $content;
		$timestamp =  mktime(0, 0, 0, substr($date, 4, 2), substr($date, 6, 2), substr($date, 0, 4));
		$day = _(date("l", $timestamp));
		$short = substr($day, 0, 1);
		return $short;
	}
	
}
?>
