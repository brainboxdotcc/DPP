<?php

/* Sanity checks */
if (!file_exists(getenv("HOME") . "/dpp-web")) {
	echo "\n*** YOU PROBABLY SHOULDN'T BE RUNNING THIS!!! ***\n\n";
	echo "\nClone https://github.com/brainboxdotcc/dpp-web.git to " . getenv("HOME") . "/dpp-web before running this script.\n";
	echo "\nIMPORTANT: This script is for documentation maintainers to auto update the website. You do NOT need to run this script\nto generate offline documentation. Instead, just run `doxygen` from the repository root.\n\n";
	exit(0);
}

/* Make drop down list of versions from the tags */
echo "Make version drop down select\n";
$opts = shell_exec("git tag | perl -p -e s/^v//g | sort -n -r | (echo \"<option value='/'>master</option>\" && sed \"s/.*/<option value='\/&\/'>&<\/option>/\")");
$opts = preg_replace("/\r|\n|/", "", $opts);

$template = file_get_contents("header.template.html");
$header = str_replace("##VERSION_OPTIONS##", $opts, $template);

file_put_contents("header.html", $header);

echo "Generate `master` docs\n";

chdir("..");
shell_exec("/usr/local/bin/doxygen");
system("cp -r docs/* " . getenv("HOME") . "/dpp-web/");

/* Create old version docs */
chdir(getenv("HOME") . "/D++");
$tags = explode("\n", shell_exec("git tag"));
system("rm -rf " . sys_get_temp_dir() . "/dpp-old");
mkdir(sys_get_temp_dir() . "/dpp-old");
chdir(sys_get_temp_dir() . "/dpp-old");
foreach ($tags as $tag) {
	$orig_tag = $tag;
	$tag = preg_replace("/^v/", "", $tag);
	if (!empty($tag)) {
		print "Generate $orig_tag docs (https://dpp.dev/$tag/)\n";
		system("git clone https://github.com/brainboxdotcc/DPP.git");
		chdir("DPP");
		system("git checkout tags/$orig_tag");
		system("cp -rv " . getenv("HOME") . "/D++/docpages/* docpages/");
		system("cp -rv " . getenv("HOME") . "/D++/Doxyfile Doxyfile");
		shell_exec("/usr/local/bin/doxygen");
		system("cp -r docs/* " . getenv("HOME") . "/dpp-web/$tag");
		chdir("..");
		system("rm -rf " . sys_get_temp_dir() . "/dpp-old/DPP");
	}
}

/* Commit and push everything to the github pages repo */
echo "Commit and push\n";
chdir(getenv("HOME") . "/dpp-web");
system("git add -A");
system("git commit -a -m \"automatic commit\"");
system("git push");

