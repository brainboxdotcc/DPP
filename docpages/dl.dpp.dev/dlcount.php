<?php
/*****************************************************************************
  Counts release downloads on GitHub
  Copyright (C) 2018 Sylvain Hallé
 
  Usage: php dlcount.php [options] user1/repo1 [user2/repo2 ...]
  
  Options:
  --total    Only show total for all releases
  --image    Output an SVG of total download count to stdout
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

/* A whitelist of repositories for which the script accepts queries.
   Used to avoid anybody querying *your* script for *their* repos. */
$REPO_WHITELIST = array(
	"brainboxdotcc/dpp",
	"brainboxdotcc/windows-bot-template",
);

/* The SVG image to output */
$SVG_IMG = <<<EOD
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="106" height="20" role="img" aria-label="downloads: XXXX"><title>downloads: XXXX</title><linearGradient id="s" x2="0" y2="100%"><stop offset="0" stop-color="#bbb" stop-opacity=".1"/><stop offset="1" stop-opacity=".1"/></linearGradient><clipPath id="r"><rect width="106" height="20" rx="3" fill="#fff"/></clipPath><g clip-path="url(#r)"><rect width="69" height="20" fill="#555"/><rect x="69" width="37" height="20" fill="#97ca00"/><rect width="106" height="20" fill="url(#s)"/></g><g fill="#fff" text-anchor="middle" font-family="Verdana,Geneva,DejaVu Sans,sans-serif" text-rendering="geometricPrecision" font-size="110"><text aria-hidden="true" x="355" y="150" fill="#010101" fill-opacity=".3" transform="scale(.1)" textLength="590">downloads</text><text x="355" y="140" transform="scale(.1)" fill="#fff" textLength="590">downloads</text><text aria-hidden="true" x="865" y="150" fill="#010101" fill-opacity=".3" transform="scale(.1)" textLength="270">XXXX</text><text x="865" y="140" transform="scale(.1)" fill="#fff" textLength="270">XXXX</text></g></svg>
EOD;

$config = array(
	"only-total" => false,
	"generate-image" => false
);
$to_set = "";
$repos = array();
if (isset($argv))
{
	for ($i = 1; $i < count($argv); $i++)
	{
		$arg = $argv[$i];
		if ($arg === "--total")
		{
			$config["only-total"] = true;
		}
		else if ($arg === "--image")
		{
			$config["generate-image"] = true;
		}
		else
		{
			$repos[] = $arg;
		}
	}
}

if (isset($_REQUEST["repo"]))
{
	$config["generate-image"] = true;
	$config["only-total"] = true;
	if (in_array($_REQUEST["repo"], $REPO_WHITELIST))
	{
		$repos[] = $_REQUEST["repo"];
	}
}
if (empty($repos) && $config["generate-image"])
{
	// No repo or repo not in whitelist: Bad request
	http_response_code(404);
	//echo "This repository is not whitelisted.";
	exit(0);
}

$dl_cnt = 0;
foreach ($repos as $repo)
{
	list($user, $repo_name) = explode("/", $repo);
	printout("Stats for ".$user."/".$repo_name."\n\n");
	$gh_url = "https://api.github.com/repos/".$user."/".$repo_name."/releases";
	$contents = get_ssl_page($gh_url);
	$json = json_decode($contents);
	$dl_cnt = 0;
	foreach ($json as $release_nb => $release)
	{
		$r_dl = 0;
		if (!$config["only-total"])
			printout($release->name.":\n");
		foreach ($release->assets as $asset)
		{
			$r_dl += $asset->download_count;
			if (!$config["only-total"])
				printout("  ".$asset->name.": ".$asset->download_count."\n");
		}
		if (!$config["only-total"])
			printout("  Total: ".$r_dl."\n");
		$dl_cnt += $r_dl;
	}
	printout("TOTAL: $dl_cnt\n");
}

if ($config["generate-image"])
{
	if (isset($_REQUEST))
	{
		header("Content-Type: image/svg+xml");
	}
	$out_img = str_replace("XXXX", sprintf("%4d", $dl_cnt), $SVG_IMG);
	echo $out_img;
}

/**
 * Prints something to the standard output, only if the program is not
 * started from an HTTP request
 */
function printout($string)
{
	global $config;
	if ($config["generate-image"])
		return;
	echo $string;
}

/**
 * Crude way of getting an https:// URL
 */
function get_ssl_page($url) 
{
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, FALSE);
    curl_setopt($ch, CURLOPT_HEADER, false);
    curl_setopt($ch, CURLOPT_FOLLOWLOCATION, true);
    curl_setopt($ch, CURLOPT_URL, $url);
    curl_setopt($ch, CURLOPT_REFERER, $url);
    curl_setopt($ch, CURLOPT_USERAGENT, "PHP 7");
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);
    $result = curl_exec($ch);
    curl_close($ch);
    return $result;
}
// :tabWidth=4:folding=explicit:wrap=none:
?>
