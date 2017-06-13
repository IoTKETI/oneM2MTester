<html>
<head>
<link href="titan_builder.css" rel="stylesheet" type="text/css" />
<title>
Web interface of the automatic Titan test builder...
</title>
<script>
function validate_selections()
{
  var platforms_selection = document.getElementById('platforms[]');
  var recipients_selection = document.getElementById('selected_recipients[]');
  var platforms = false;
  var recipients = false;
  for (var i = 0; i < platforms_selection.options.length; i++) {
    if (platforms_selection.options[i].selected && platforms_selection.options[i].text.length > 0) {
      platforms = true;
      break;
    }
  }
  if (recipients_selection.options.length > 0 && recipients_selection.options[0].text.length > 0) {
    for (var i = 0; i < platforms_selection.options.length; i++) {
      recipients_selection.options[i].selected = true;
    }
    recipients = true;
  }
  if (!platforms && !recipients) {
    alert('At least one platform and one recipient needs to be selected.');
  } else if (!platforms) {
    alert('At least one platform needs to be selected.');
  } else if (!recipients) {
    alert('At least one recipient needs to be selected.');
  }
  return platforms && recipients;
}
function addtolist(sourceID, targetID)
{
  source = document.getElementById(sourceID);
  target = document.getElementById(targetID);
  numberOfItems = source.options.length;
  insertPt = target.options.length;
  if (target.options[0].text === "") { insertPt = 0; }
  for (i = 0; i < numberOfItems; i++) {
    if (source.options[i].selected === true) {
      msg = source.options[i].text;
      for (j = 0; j < target.options.length; j++) {
        if (msg === target.options[j].text) {
          j = -1;
          break;
        }
      }
      if (j > 0) {
        target.options[insertPt] = new Option(msg);
        insertPt = target.options.length;
      }
    }
  }
}
function takefromlist(targetID)
{
  target = document.getElementById(targetID);
  if (target.options.length < 0) { return; }
  for (var i = target.options.length - 1; i >= 0; i--) {
    if (target.options[i].selected) {
      target.options[i] = null;
      if (target.options.length === 0) { target.options[0] = new Option(""); }
    }
  }
}
</script>
</head>
<body>
<h1>Welcome to the web interface of the automatic Titan test builder...</h1>
<form method="POST" action="<?php echo $_SERVER['PHP_SELF']; ?>" onSubmit="return validate_selections()">
<input type="hidden" name="_submit_check" value="1" />
<?php
define("HISTORY_FILE", "./titan_builder_history.txt");

function get_platforms_selection($platforms)
{
  $result =
    "<table>\n" .
    "<tr>\n" .
    "<td>Select platform(s):</td>\n" .
    "</tr>\n" .
    "<tr>\n" .
    "<td><select name=\"platforms[]\" id= \"platforms[]\" multiple size=\"" . count($platforms) . "\">\n";
  for ($i = 0; $i < count($platforms); $i++) {
    $platform = split(" ", $platforms[$i]);
    $result .= "<option value=\"" . $platform[0] . "\">" . $platform[0] . "</option>\n";
  }
  $result .=
    "</select></td>\n" .
    "</tr>\n" .
    "</table>\n";
  return $result;
}

function get_recipients_selection($recipients)
{
  $result =
    "<table border=\"0\">\n" .
    "<tr>\n" .
    "<td align=\"left\">Select recipient(s):</td>\n" .
    "</tr>\n" .
    "<tr>\n" .
    "<th><select name=\"all_recipients[]\" id=\"all_recipients[]\" size=\"" . count($recipients) . "\" multiple=\"multiple\" style=\"width:320;height:180\"" .
    "ondblclick=\"addtolist('all_recipients[]', 'selected_recipients[]');\">\n";
  for ($i = 0; $i < count($recipients); $i++) {
    $result .= "<option>" . htmlspecialchars($recipients[$i]) . "</option>\n";
  }
  $result .=
    "</select>&nbsp;</th>\n" .
    "<th style=\"vertical-align:middle\">\n" .
    "<input type=\"button\" onclick=\"addtolist('all_recipients[]', 'selected_recipients[]');\" value=\"--&gt;\" /><br />\n" .
    "<input type=\"button\" style=\"button\" onclick=\"takefromlist('selected_recipients[]');\" value=\"&lt;--\" />\n" .
    "</th>\n" .
    "<th>&nbsp;\n" .
    "<select name=\"selected_recipients[]\" id=\"selected_recipients[]\" size=\"" . count($recipients) . "\" multiple=\"multiple\" style=\"width:320;height:180\"" .
    "ondblclick=\"takefromlist('selected_recipients[]');\">\n" .
    "<option></option>\n" .
    "</select>\n" .
    "</th>\n" .
    "</tr>\n" .
    "</table>\n";
  return $result;
}

function get_tests_selection()
{
  $result =
    "<table border=\"0\">\n" .
    "<tr>\n" .
    "<td align=\"left\">Select test(s) to run:</td>\n" .
    "</tr>\n" .
    "<tr>\n" .
    "<td><input type=\"checkbox\" disabled=\"disabled\" checked=\"checked\" /> Build</td>\n" .
    "</tr>\n" .
    "<tr>\n" .
    "<td><input type=\"checkbox\" name=\"regtests\" value=\"regtests\" /> Regression tests</td>\n" .
    "</tr>\n" .
    "<tr>\n" .
    "<td><input type=\"checkbox\" name=\"functests\" value=\"functests\" /> Function tests</td>\n" .
    "</tr>\n" .
    "<tr>\n" .
    "<td><input type=\"checkbox\" name=\"perftests\" value=\"perftests\" /> Performance tests</td>\n" .
    "</tr>\n" .
    "</table>\n" .
    "<p><input type=\"submit\" name=\"press_and_pray\" value=\"Press & Pray!\"></p>\n";
  return $result;
}

function get_history()
{
  if (!file_exists(HISTORY_FILE)) {
    return "";
  }
  $file = fopen(HISTORY_FILE, "r");
  $result = "<br />\n";
  while (!feof($file)) {
    $result .= (htmlspecialchars(fgets($file)) . "<br />\n");
  }
  fclose($file);
  return $result;
}

function get_scripts()
{
  $command = "ssh titanrt@esekits1064 \"bash -c 'cd /home/titanrt/titan_nightly_builds && rm -rf *.{py,pyc,sh} TTCNv3/etc/autotest ;";
  $command .= " . /home/titanrt/.titan_builder ; cvs co TTCNv3/etc/autotest ; ln -sf TTCNv3/etc/autotest/*.{py,sh} .'\"";
  pclose(popen($command, "r"));
  echo "<p>" . htmlspecialchars($command) . "</p>";
}

function start_the_build($platform_strings, $recipient_strings, $test_strings)
{
  $command = "ssh titanrt@esekits1064 \"bash -c 'cd /home/titanrt/titan_nightly_builds && . /home/titanrt/.titan_builder ;";
  $command .= " ./titan_builder.sh -c " . $platform_strings . " -A \"'\"'\"" . $recipient_strings . "\"'\"'\" " . (strlen($test_strings) > 0 ? "-t " . $test_strings : "") . "'\" &";
  pclose(popen($command, "r"));
  echo "<p>" . htmlspecialchars($command) . "</p>";
}

exec('/home/titanrt/titan_nightly_builds/titan_builder.py -d', $output_platforms);
exec('/home/titanrt/titan_nightly_builds/titan_builder.py -a', $output_recipients);
if (array_key_exists("_submit_check", $_POST)) {
  echo "Notification will be sent to the following recipients: "
    . htmlspecialchars(implode(", ", $_POST["selected_recipients"])) . "<br /><br />\n";
  echo "The build has started on the following platforms:<br /><br />\n";
  for ($i = 0; $i < count($_POST["platforms"]); $i++) {
    echo $i . ". " . $_POST["platforms"][$i] . "<br />\n";
  }
  $platform_strings = implode(",", $_POST["platforms"]);
  $recipient_strings = implode(",", $_POST["selected_recipients"]);
  $test_strings = ((isset($_POST["regtests"]) && $_POST["regtests"] == "regtests") ? "r" : "")
    . ((isset($_POST["functests"]) && $_POST["functests"] == "functests") ? "f" : "")
    . ((isset($_POST["perftests"]) && $_POST["perftests"] == "perftests") ? "p" : "");
  start_the_build($platform_strings, $recipient_strings, $test_strings);
  $contents = file_get_contents(HISTORY_FILE);
  file_put_contents(HISTORY_FILE, date("Y-m-d H:i:s") . " "
    . implode(", ", $_POST["selected_recipients"]) . " "
    . implode(", ", $_POST["platforms"]) . "\n" . $contents);
} else {
  get_scripts();
  $html_output =
    "<table>\n" .
    "<tr>\n<td>\n" .
    "Run multiple tests at the same time for your own risk.<br />\n" .
    "Number of available platforms: " . count($output_platforms) . ".<br />\n" .
    "The tests will be run by `titanrt' on the selected platform(s).\n" .
    "</td>\n</tr>\n" .
    "<tr>\n<td>\n" .
    get_platforms_selection($output_platforms) .
    "</td>\n</tr>\n" .
    "<tr>\n<td>\n" .
    get_recipients_selection($output_recipients) .
    "</td>\n</tr>\n" .
    "<tr>\n<td>\n" .
    get_tests_selection() .
    "</td>\n</tr>\n" .
    "<tr>\n<td class=\"history\">\n" .
    get_history() .
    "</td>\n</tr>\n" .
    "</table>\n";
  echo $html_output;
}
?>
</form>
</body>
</html>
