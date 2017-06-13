<?php
$plugin = trim(isset($_GET["plugin"]) ? $_GET["plugin"] : "");
$version = trim(isset($_GET["version"]) ? $_GET["version"] : "");
if (strlen($plugin) > 0 && strlen($version) > 0) {
savestats($plugin, $version);
}
function savestats($plugin, $version) {
// File name/format:
// titan_eclipse_designer.txt titan_eclipse_logviewer.txt titan_eclipse_executor.txt
// version1 counter1
// version2 counter2
// ...
$filename = $plugin . ".txt";
$tmpfilename = $filename . ".tmp";
$tmp = fopen($tmpfilename, "w");
if (file_exists($filename)) {
$oldstats = file_get_contents($filename);
$lines = explode("\n", $oldstats);
foreach ($lines as $line) {
$data = explode(" ", $line);
if ($data[0] == $version) {
fputs($tmp, $version . " " . ((int)$data[1] + 1) . "\n");
} else {
fputs($tmp, $line);
}
}
} else {
fputs($version . "1\n");
}
fclose($tmp);
rename($tmpfilename, $filename);
}
?>
