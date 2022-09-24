<?php
$tag = $argv[1];
$orig_tag = $argv[2];
print "ASYNC: Generate $orig_tag docs (https://dpp.dev/$tag/)\n";
mkdir(sys_get_temp_dir() . "/dpp-old/" . $tag);
chdir(sys_get_temp_dir() . "/dpp-old/" . $tag);
system("git clone --recursive https://github.com/brainboxdotcc/DPP.git >/dev/null");
chdir("DPP");
system("git fetch --tags -a >/dev/null");
system("git checkout tags/$orig_tag >/dev/null");
system("git checkout tags/v$orig_tag >/dev/null");
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
shell_exec("/usr/local/bin/doxygen >/dev/null");
if (!file_exists("/home/runner/dpp-web/$tag")) {
	mkdir("/home/runner/dpp-web/$tag");
}
chdir("docs");
/* Add giscus inside the actual content as postprocess */
system('perl -p -i -e \'s/(<.div><.-- contents -->)/$1<script src="https:\/\/giscus.app\/client.js" data-repo="brainboxdotcc\/dpp-comments" ' .
	'data-repo-id="R_kgDOHOY4xg" data-category="General" data-category-id="DIC_kwDOHOY4xs4CRYtj" data-mapping="pathname" data-strict="1" ' .
	'data-reactions-enabled="1" data-emit-metadata="1" data-input-position="top" data-theme="preferred_color_scheme" data-lang="en" ' .
	'data-loading="lazy" crossorigin="anonymous" async><\/script>/\' *.html');
system("rsync -r --include='*' '.' '/home/runner/dpp-web/".$tag."' >/dev/null");
chdir("/");
system("rm -rf " . sys_get_temp_dir() . "/dpp-old/$tag");
file_put_contents("/tmp/completion_$tag", $tag);
echo "ASYNC: Completed $tag\n";