<?php
$smarty->register_function("icon", "smarty_icon");
function smarty_icon($params, &$smarty)
{

	$content = $params["flags"];
	if (isset($content)) {
		if (($content &MSGFLAG_READ)==MSGFLAG_READ){
			if (($content &MSGFLAG_HASATTACH)==MSGFLAG_HASATTACH){
				$result = "icon_mailreadattach.gif";
			}
			else {
				$result = "icon_mailread.gif";
			}
		}
		else {
			if (($content &MSGFLAG_HASATTACH)==MSGFLAG_HASATTACH){
				$result = "icon_mailunreadattach.gif";
			}
			else {
				$result = "icon_mailunread.gif";
			}
		}
		return $result;
	}
}
?>
