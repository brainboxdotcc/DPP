<?php

$nodeploy = false;
if (count($argv) > 1 && $argv[1] == 'nodeploy') {
	$nodeploy = true;
}

/* Sanity checks */
system("sudo apt-get install graphviz screen >/dev/null");
system("echo \$GITHUB_TOKEN | gh auth login --with-token ");
system("gh auth status");
system("git clone https://braindigitalis:\$PERSONAL_ACCESS_TOKEN@github.com/brainboxdotcc/dpp-web.git /home/runner/dpp-web >/dev/null");

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
shell_exec("/usr/local/bin/doxygen >/dev/null");
chdir("docs");
system("rsync -rv --include='*' '.' '/home/runner/dpp-web' >/dev/null");
chdir("..");

if ($nodeploy) {
	exit(0);
}

/* Create old version docs */
chdir("/home/runner/work/DPP/DPP");
system("rm -rf " . sys_get_temp_dir() . "/dpp-old");
mkdir(sys_get_temp_dir() . "/dpp-old");

/* Fire up async tasks to run instances of doxygen for each past version */
$asyncRunners = [];
foreach ($tags as $tag) {
	$orig_tag = $tag;
	$tag = preg_replace("/^v/", "", $tag);
	if (!empty($tag)) {
		$asyncRunners[$tag] = true;
		$pid = pcntl_fork();
		if ($pid == 0) {
			posix_setsid();
			pcntl_exec(PHP_BINARY, ["docpages/makedocs-gh-single.php", $tag, $orig_tag], $_ENV);
			exit(0);
		}
	}
}

/* Wait for all async tasks to complete */
while (count($asyncRunners)) {
	foreach ($asyncRunners as $tag => $discarded) {
		if (file_exists("/tmp/completion_$tag") && file_get_contents("/tmp/completion_$tag") == $tag) {
			unset($asyncRunners[$tag]);
			echo "Runner for $tag is completed.\n";
		}
	}
	sleep(1);
}

/* Commit and push everything to the github pages repo */
echo "Commit and push\n";
chdir("/home/runner/dpp-web");
system("git add -A >/dev/null");
system("git commit -a -m \"automatic commit\" >/dev/null");
system("git push -f \"https://braindigitalis:\$PERSONAL_ACCESS_TOKEN@github.com/brainboxdotcc/dpp-web.git\" >/dev/null");
