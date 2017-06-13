<?php
$path_parts = pathinfo($_SERVER['SCRIPT_FILENAME']);
$file_name = preg_replace('"\.php$"', '.txt', $path_parts['basename']);
$content = date("Ymd_Hms");
$fp = fopen($file_name, "a+");
fwrite($fp, $content . "\n");
fclose($fp);
?>
