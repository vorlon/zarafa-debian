<?php
$sensitivity = array();
$sensitivity[SENSITIVITY_NONE]=_("Sensitivity none");
$sensitivity[SENSITIVITY_PERSONAL]=_("Personal");
$sensitivity[SENSITIVITY_PRIVATE]=_("Private");
$sensitivity[SENSITIVITY_COMPANY_CONFIDENTIAL]=_("Company Confidential");

$smarty->assign("sensitivitya", $sensitivity);


$status = array();
$status[olTaskNotStarted]=_("Task Not Started");
$status[olTaskInProgress]=_("Task In Progress");
$status[olTaskComplete]=_("Task Complete");
$status[olTaskWaiting]=_("Task Waiting");
$status[olTaskDeferred]=_("Task Deffered");

$smarty->assign("statusa", $status);

$priority = array();
$priority[PRIO_NONURGENT] = _("Non urgent");
$priority[PRIO_NORMAL] = _("Normal");
$priority[PRIO_URGENT] = _("Urgent");

$smarty->assign("prioritya", $priority);

$busystatus = array();
$busystatus[0] = _("Free");
$busystatus[1] = _("Tentative");
$busystatus[2] = _("Busy");
$busystatus[3] = _("Out of Office");

$smarty->assign("busystatusa", $busystatus);
?>