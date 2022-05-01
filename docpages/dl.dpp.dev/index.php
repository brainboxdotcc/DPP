<?php
// Force no caching of the download
header("Expires: Tue, 03 Jul 2001 06:00:00 GMT");
header("Last-Modified: " . gmdate("D, d M Y H:i:s") . " GMT");
header("Cache-Control: no-store, no-cache, must-revalidate, max-age=0");
header("Cache-Control: post-check=0, pre-check=0", false);
header("Pragma: no-cache");
header("Status: 200 OK");

// Split up url and set defaults
list($version, $arch, $type) = explode('/', preg_replace('/https:\/\/dl\.dpp\.dev\//', '', $_SERVER['REDIRECT_SCRIPT_URI']), 3);
$version = !empty($version) ? $version : 'latest';
$arch = !empty($arch) ? $arch : 'linux-x64';
$type = !empty($type) ? $type : 'deb';
$urls = [];

// All windows downloads are of type 'zip', if not specified, it is defaulted
if (preg_match('/^win/i', $arch) && $type === 'deb') {
	$type = 'zip';
}
// the short word 'win' is a shorthand for the vs2019 release build
if ($arch === 'win') {
	$arch = 'win32-release-vs2019';
}

// A crontab keeps this updated so we only perform one github api request per 5 minutes
$json = json_decode(file_get_contents('release.json'));

// If the user asked for 'latest', we find out what the latest release tag name is
if ($version === 'latest') {
	$version = $json[0]->tag_name;
}

// Build search filename
$searchName = 'libdpp-' . preg_replace('/^v/', '', $version) . '-' . $arch . '.' . $type;

// Iterate list of release artifacts across all releases
foreach ($json as $index => $release) {
	foreach ($release->assets as $index2 => $asset) {
		$url = $asset->browser_download_url;
		$name = $asset->name;
		$thisVersion = $release->tag_name;
		// We found a matching file, stream it to the user
		if (strtoupper($searchName) == strtoupper($name)) {
			header('Content-Type: application/octet-stream');
			header('Content-Disposition: attachment; filename="' . $name . '"');
			readfile($url);
			exit;
		}
		$urls[] = [
			'name' => $name,
			'url' => $url,
			'version' => $thisVersion,
		];
	}
}


if ($version === 'json') {
	header('Content-Type: application/json');
	echo json_encode($urls);
} else {
	// Nothing found, offer up some useful info
	foreach ($urls as $thisUrl) {
		printf("%s - <a href='%s' target='_blank'>%s</a><br />", $thisUrl['version'], $thisUrl['url'], $thisUrl['name']);
	}
}

