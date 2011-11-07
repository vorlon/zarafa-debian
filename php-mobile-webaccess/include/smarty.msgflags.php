<?php
$smarty->register_function("msgflags", "smarty_msgflags");

function smarty_msgflags($params, &$smarty)
{
	$msgflags	= isset($params["flags"])?$params["flags"]:0;
	$flag 		= isset($params["flag"])?$params["flag"]:"";
	$val_true	= isset($params["val_true"])?$params["val_true"]:"true";
	$val_false	= isset($params["val_false"])?$params["val_false"]:"false";
	$result = $val_false;

	$flag = constant($flag);
	
	if (($msgflags & $flag)==$flag){
		$result = $val_true;
	}else{
		$result = $val_false;
	}
	return $result;
}
?>
