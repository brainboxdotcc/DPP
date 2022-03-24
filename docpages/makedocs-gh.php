<?php

$nodeploy = false;
if (count($argv) > 1 && $argv[1] == 'nodeploy') {
	$nodeploy = true;
}

/* Sanity checks */
system("sudo apt-get install graphviz");
#system("sudo git clone \"https://".getenv("GITHUB_TOKEN")."@github.com/brainboxdotcc/dpp-web.git/\" /dpp-web");
system("echo \$GITHUB_TOKEN | gh auth login --with-token ");
system("gh auth status");
system("git clone https://braindigitalis:\$PERSONAL_ACCESS_TOKEN@github.com/brainboxdotcc/dpp-web.git /home/runner/dpp-web");

chdir("/home/runner/work/DPP/DPP");
system("sudo cp /home/runner/dpp-web/doxygen /usr/local/bin/doxygen && sudo chmod ugo+x /usr/local/bin/doxygen");
chdir("docpages");


system("git config --global user.email \"robot@dpp.dev\"");
system("git config --global user.name \"Docs Deployment Bot\"");
system("git config --global advice.detachedHead false");

/* Make drop down list of versions from the tags */
echo "Make version drop down select\n";
system("git fetch -av --tags");
$tags = explode("\n", shell_exec("git tag"));
for ($n = 0; $n < count($tags); ++$n) {
	$tags[$n] = preg_replace('/^v/', '', $tags[$n]);
}
natsort($tags);
$tags = array_reverse($tags);

$opts = "<option value='/'>master</option>";
foreach ($tags as $tag) {
	if ($tag != '') {
		$opts .= "<option value='/$tag/'>$tag</option>";
	}
}

$taglist = '';
foreach ($tags as $tag) {
	if ($tag != '') {
		$tag2 = str_replace("v", "", $tag);
		$taglist .= "<a href='/".$tag2."/'>D++ Library version $tag</a>";
	}
}

$template = file_get_contents("/home/runner/work/DPP/DPP/docpages/header.template.html");
$header = str_replace("##VERSION_OPTIONS##", $opts, $template);

$footer = file_get_contents("/home/runner/work/DPP/DPP/docpages/footer.template.html");
$footer = str_replace("###PREV###", $taglist, $footer);

file_put_contents("/home/runner/work/DPP/DPP/docpages/header.html", $header);
file_put_contents("/home/runner/work/DPP/DPP/docpages/footer.html", $footer);

echo "Generate `master` docs\n";

chdir("..");
shell_exec("/usr/local/bin/doxygen");
chdir("docs");
system("rsync -rv --include='*' '.' '/home/runner/dpp-web'");
chdir("..");

if ($nodeploy) {
	exit(0);
}

/* Create old version docs */
chdir("/home/runner/work/DPP/DPP");
system("rm -rf " . sys_get_temp_dir() . "/dpp-old");
mkdir(sys_get_temp_dir() . "/dpp-old");
chdir(sys_get_temp_dir() . "/dpp-old");
foreach ($tags as $tag) {
	$orig_tag = $tag;
	$tag = preg_replace("/^v/", "", $tag);
	if (!empty($tag)) {
		print "Generate $orig_tag docs (https://dpp.dev/$tag/)\n";

		system("git clone --recursive https://github.com/brainboxdotcc/DPP.git");
		chdir("DPP");
		system("git fetch --tags -a");
		system("git checkout tags/$orig_tag");
		system("git checkout tags/v$orig_tag");
		/* Older versions of the docs before 9.0.7 don't have these. Force them into the tree so old versions get current styling */
		system("cp -r /home/runner/work/DPP/DPP/docpages/images docpages");
		system("cp -r /home/runner/work/DPP/DPP/docpages/style.css docpages/style.css");
		system("cp -r /home/runner/work/DPP/DPP/docpages/*.html docpages/");
		system("cp -r /home/runner/work/DPP/DPP/doxygen-awesome-css doxygen-awesome-css");
		/* Always make sure that the version is using the latest doxygen,
		 * but rewrite version number (project number)
		 */
		$doxy = file_get_contents("/home/runner/work/DPP/DPP/Doxyfile");
		$doxy = str_replace("PROJECT_NUMBER         =", "PROJECT_NUMBER         = $tag", $doxy);
		file_put_contents("Doxyfile", $doxy);
		/* Rewrite selected version number so that each page has a new default selected in the drop down */
		$hdr = file_get_contents("/home/runner/work/DPP/DPP/docpages/header.html");
		$hdr = str_replace("option value='/$tag/'", "option selected value='/$tag/'", $hdr);
		/* Rewrite version info in header */
		file_put_contents("docpages/header.html", $hdr);		
		shell_exec("/usr/local/bin/doxygen");
		system("mkdir /home/runner/dpp-web/$tag 2>/dev/null");
		chdir("docs");
		system("rsync -r --include='*' '.' '/home/runner/dpp-web/".$tag."'");
		chdir("..");
		chdir("..");
		system("rm -rf " . sys_get_temp_dir() . "/dpp-old/DPP");
	}
}

/* Commit and push everything to the github pages repo */
echo "Commit and push\n";
chdir("/home/runner/dpp-web");
system("git add -A >/dev/null");
system("git commit -a -m \"automatic commit\" >/dev/null");
system("git push -f \"https://braindigitalis:\$PERSONAL_ACCESS_TOKEN@github.com/brainboxdotcc/dpp-web.git\"");

