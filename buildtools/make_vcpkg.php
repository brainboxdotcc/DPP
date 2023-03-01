<?php

function checkoutRepository(string $tag = "master") {
	chdir(getenv('HOME'));
	system('rm -rf ./dpp');
	system('git config --global user.email "noreply@dpp.dev"');
	system('git config --global user.name "DPP VCPKG Bot"');
	system('git clone https://braindigitalis:${{ secrets.PERSONAL_ACCESS_TOKEN }}@github.com/brainboxdotcc/dpp ./dpp --depth=1');
	system('git checkout ' . $tag);
}

function constructPortAndVersionFile(string $version, string $sha512 = "0"): string {
	chdir('./dpp');

	$portFileContent = 'vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO brainboxdotcc/DPP
    REF "' . $latestTag . '"
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
	file_put_contents('./vcpkg/ports/dpp/vcpkg.json', $versionFileContent);
	return $portFileContent;
}

function firstBuild(string $portFileContent): string {
	chdir('./dpp');
	system('sudo mkdir /usr/local/share/vcpkg/ports/dpp/');
	system('sudo cp -v -R ./vcpkg/ports/dpp/vcpkg.json /usr/local/share/vcpkg/ports/dpp/vcpkg.json');
	file_put_contents('/tmp/portfile', $portFileContent);
	system('sudo cp -v -R /tmp/portfile /usr/local/share/vcpkg/ports/dpp/portfile.cmake');
	unlink('/tmp/portfile');
	$buildResults = shell_exec('sudo /usr/local/share/vcpkg/vcpkg install dpp:x64-linux');
	if (preg_match('/Actual hash:\s+([0-9a-fA-F]+)/', $matches)) {
		return $matches[1];
	}
	return '';
}

function build(string $portFileContent) {
	/*
                sudo git add . &&
                sudo git commit -m "[bot] VCPKG info update [skip ci]" &&
                sudo git config pull.rebase false &&
                sudo git pull &&
                sudo git push origin master &&
                sudo /usr/local/share/vcpkg/vcpkg install dpp:x64-linux || true &&
                sudo cat /usr/local/share/vcpkg/buildtrees/dpp/*.log || true
*/
	file_put_contents('./vcpkg/ports/dpp/portfile.cmake', $portFileContent);
	system('sudo cp -v -R ./vcpkg/ports/dpp/vcpkg.json /usr/local/share/vcpkg/ports/dpp/vcpkg.json');
	system('sudo cp -v -R ./vcpkg/ports/dpp/portfile.cmake /usr/local/share/vcpkg/ports/dpp/portfile.cmake');
	system('sudo cp -v -R ./vcpkg/ports/* /usr/local/share/vcpkg/ports/');

	chdir('/usr/local/share/vcpkg');
	system('sudo ./vcpkg format-manifest ./ports/dpp/vcpkg.json');
	system('sudo git add .');
	system('sudo git commit -m "[bot] VCPKG info update"');
	system('sudo /usr/local/share/vcpkg/vcpkg x-add-version dpp');

	chdir(getenv('HOME') . '/dpp');
	system('cp -v -R /usr/local/share/vcpkg/ports/dpp/vcpkg.json ./vcpkg/ports/dpp/vcpkg.json');
	system('cp -v -R /usr/local/share/vcpkg/versions/baseline.json ./vcpkg/versions/baseline.json');
	system('cp -v -R /usr/local/share/vcpkg/versions/d-/dpp.json ./vcpkg/versions/d-/dpp.json');

	system('git add .');
	system('git commit -m "[bot] VCPKG info update [skip ci]"');
	system('git config pull.rebase false');
	system('git pull');
	system('git push origin master');
	system('sudo /usr/local/share/vcpkg/vcpkg install dpp:x64-linux');
}


$latestTag = shell_exec("git describe --tags `git rev-list --tags --max-count=1`");
$version = preg_replace('/^v/', '', $latestTag);

/* Check out source for latest tag */
checkoutRepository($latestTag);

/* First run with SHA512 of 0 to gather actual value */
$sha512 = firstBuild(constructPortAndVersionFile($version));

/* Now check out master */
checkoutRepository("master");

build(constructPortAndVersionFile($sha512));