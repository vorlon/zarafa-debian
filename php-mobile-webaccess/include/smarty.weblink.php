<?php

$smarty->register_block("weblink", "smarty_weblink");


function smarty_weblink($params, $content, &$smarty, &$repeat)
{
	if (isset($content)) {
		$href="<a href=\"$content\">$content</a>";
		return $href;
	}
}
?>