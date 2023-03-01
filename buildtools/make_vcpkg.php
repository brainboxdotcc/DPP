<?php

function checkoutRepository(string $tag = "master") {
	echo "Check out repository: $tag\n";
	chdir(getenv('HOME'));
	system('rm -rf ./dpp');
	system('git config --global user.email "noreply@dpp.dev"');
	system('git config --global user.name "DPP VCPKG Bot"');
	system('git clone https://'.$_ENV['GITHUB_USER'].':' . $_ENV['GITHUB_TOKEN'] . '@github.com/brainboxdotcc/DPP ./dpp --depth=1');
	chdir(getenv("HOME") . '/dpp');
	system('git fetch -avt');
	system('git checkout ' . $tag);
}

function constructPortAndVersionFile(string $version, string $sha512 = "0"): string {
	echo "Construct portfile for $version, sha512: $sha512\n";
	chdir(getenv("HOME") . '/dpp');

	$portFileContent = 'vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO brainboxdotcc/DPP
    REF "v' . $version . '"
    SHA512 ' . $sha512 . '
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    DISABLE_PARALLEL_CONFIGURE
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(NO_PREFIX_CORRECTION)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share/dpp")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

if(VCPKG_LIBRARY_LINKAGE STREQUAL "static")
    file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/bin" "${CURRENT_PACKAGES_DIR}/debug/bin")
endif()

file(
    INSTALL "${SOURCE_PATH}/LICENSE"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
    RENAME copyright
)

file(COPY "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
';
	// ./vcpkg/ports/dpp/vcpkg.json
	$versionFileContent = '{
  "name": "dpp",
  "version": ' . json_encode($version) . ',
  "description": "D++ Extremely Lightweight C++ Discord Library.",
  "homepage": "https://dpp.dev/",
  "license": "Apache-2.0",
  "supports": "((windows & !static & !uwp) | linux | osx)",
  "dependencies": [
    "libsodium",
    "nlohmann-json",
    "openssl",
    "opus",
    "zlib",
    {
      "name": "vcpkg-cmake",
      "host": true
    },
    {
      "name": "vcpkg-cmake-config",
      "host": true
    }
  ]
}';
	echo "Writing portfile...\n";
	file_put_contents('./vcpkg/ports/dpp/vcpkg.json', $versionFileContent);
	return $portFileContent;
}

function firstBuild(string $portFileContent): string {
	echo "Starting first build\n";

	chdir(getenv("HOME") . '/dpp');
	echo "Create /usr/local/share/vcpkg/ports/dpp/\n";
	system('sudo mkdir -p /usr/local/share/vcpkg/ports/dpp/');
	echo "Copy vcpkg.json to /usr/local/share/vcpkg/ports/dpp/vcpkg.json\n";
	system('sudo cp -v -R ./vcpkg/ports/dpp/vcpkg.json /usr/local/share/vcpkg/ports/dpp/vcpkg.json');
	file_put_contents('/tmp/portfile', $portFileContent);
	system('sudo cp -v -R /tmp/portfile /usr/local/share/vcpkg/ports/dpp/portfile.cmake');
	unlink('/tmp/portfile');
	$buildResults = shell_exec('sudo /usr/local/share/vcpkg/vcpkg install dpp:x64-linux');
	$matches = [];
	if (preg_match('/Actual hash:\s+([0-9a-fA-F]+)/', $buildResults, $matches)) {
		echo "Obtained SHA512 for first build: " . $matches[1] . "\n";
		return $matches[1];
	}
	echo "No SHA512 found during first build :(\n";
	return '';
}

function build(string $portFileContent) {

	echo "Executing second build\n";

	echo "Copy local port files to /usr/local/share...\n";
	chdir(getenv("HOME") . '/dpp');
	file_put_contents('./vcpkg/ports/dpp/portfile.cmake', $portFileContent);
	system('sudo cp -v -R ./vcpkg/ports/dpp/vcpkg.json /usr/local/share/vcpkg/ports/dpp/vcpkg.json');
	system('sudo cp -v -R ./vcpkg/ports/dpp/portfile.cmake /usr/local/share/vcpkg/ports/dpp/portfile.cmake');
	system('sudo cp -v -R ./vcpkg/ports/* /usr/local/share/vcpkg/ports/');

	echo "vcpkg x-add-version...\n";
	chdir('/usr/local/share/vcpkg');
	system('sudo ./vcpkg format-manifest ./ports/dpp/vcpkg.json');
	system('sudo git add .');
	system('sudo git commit -m "[bot] VCPKG info update"');
	system('sudo /usr/local/share/vcpkg/vcpkg x-add-version dpp');

	echo "Copy back port files from /usr/local/share...\n";
	chdir(getenv('HOME') . '/dpp');
	system('cp -v -R /usr/local/share/vcpkg/ports/dpp/vcpkg.json ./vcpkg/ports/dpp/vcpkg.json');
	system('cp -v -R /usr/local/share/vcpkg/versions/baseline.json ./vcpkg/versions/baseline.json');
	system('cp -v -R /usr/local/share/vcpkg/versions/d-/dpp.json ./vcpkg/versions/d-/dpp.json');

	echo "Commit and push changes to master branch\n";
	system('git add .');
	system('git commit -m "[bot] VCPKG info update [skip ci]"');
	system('git config pull.rebase false');
	system('git pull');
	system('git push origin master');

	echo "vcpkg install...\n";
	system('sudo /usr/local/share/vcpkg/vcpkg install dpp:x64-linux');
}



echo "Starting vcpkg updater...\n";

$latestTag = preg_replace("/\n/", "", shell_exec("git describe --tags `git rev-list --tags --max-count=1`"));
$version = preg_replace('/^v/', '', $latestTag);

echo "Latest tag: " . $latestTag . " version: " . $version . "\n";

/* Check out source for latest tag */
checkoutRepository($latestTag);

/* First run with SHA512 of 0 to gather actual value */
$sha512 = firstBuild(constructPortAndVersionFile($version));

/* Now check out master */
checkoutRepository();

build(constructPortAndVersionFile($version, $sha512));