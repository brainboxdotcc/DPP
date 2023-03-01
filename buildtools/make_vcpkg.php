<?php

require __DIR__ . '/vendor/autoload.php';
use Dpp\Packager\Vcpkg;

$vcpkg = new Vcpkg();
/* Check out source for latest tag */
$vcpkg->checkoutRepository($vcpkg->getTag());
/* First run with SHA512 of 0 to gather actual value and save it */
$sha512 = $vcpkg->firstBuild($vcpkg->constructPortAndVersionFile());
if (!empty($sha512)) {
    /* Now check out master */
    $vcpkg->checkoutRepository();
    /* Attempt second build with the valid SHA512 sum */
    $vcpkg->secondBuild($vcpkg->constructPortAndVersionFile($sha512));
}