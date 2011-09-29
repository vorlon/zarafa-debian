<html>
<head>
<title>{$title|escape}</title>
<link rel="stylesheet" type="text/css" href="css/style.css">
{literal}
<script>
function gotoUrl(url)
{
	location.href=("index.php?type=msg&task=IPM.Appointment&entryid="+url);
}
</script>
{/literal}
</head>
<body>
<div>
<h1><a href="index.php"><img src="images/zarafa16.gif" height="16" width="16" border="0"></a>&nbsp;{$title|escape}</h1>
