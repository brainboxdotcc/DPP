<?php

$opts = shell_exec("git tag | perl -p -e s/^v//g | sort -n -r | (echo \"<option value='/'>master</option>\" && sed \"s/.*/<option value='\/&\/'>&<\/option>/\")");
$opts = preg_replace("/\r|\n|/", "", $opts);

$template = file_get_contents("header.template.html");
$header = str_replace("##VERSION_OPTIONS##", $opts, $template);

file_put_contents("header.html", $header);

chdir("..");
system("/usr/local/bin/doxygen");
system("cp -r docs/* /home/brain/dpp-web/");
chdir("/home/brain/dpp-web");
system("git add -A");
system("git commit -a -m \"automatic commit\"");
system("git push");

