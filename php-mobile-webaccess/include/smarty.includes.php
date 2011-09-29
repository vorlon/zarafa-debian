<?php

// set smarty settings
$smarty->template_dir = SMARTY_TEMPLATE_DIR;
$smarty->config_dir = SMARTY_CONFIG_DIR;
$smarty->cache_dir = SMARTY_CACHE_DIR;
$smarty->compile_dir = SMARTY_COMPILE_DIR;

require_once("include/smarty.gettext.php");
require_once("include/smarty.address.php");
require_once("include/smarty.phonenumber.php");
require_once("include/smarty.weblink.php");
require_once("include/smarty.categories.php");
require_once("include/smarty.icon.php");
require_once("include/smarty.msgflags.php");
require_once("include/smarty.arrays.php");

?>
