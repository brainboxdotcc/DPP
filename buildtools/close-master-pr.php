<?php

// Comment on and close all PRs targeted at master branch

// Magic sauce
exec("gh pr list --base master | sed 's/\|/ /' |awk '{print $1}'", $master_prs);

foreach ($master_prs as $pr) {
    $pr = (int)$pr;
    if ($pr > 0) {
        system("gh pr comment $pr -b \"You have opened a PR against the master branch. PRs must target the \`dev\` branch, as such this PR has been automatically closed. Please re-target your PR against the dev branch if you reopen it. Thank you for your contribution.\"");
        system("gh pr close $pr");
    }
}

// Tidy up the workflow run list so it isn't littered with these
exec("gh run list -w \"Close master-targeted PRs\"", $runs);
$runindex = 0;
foreach ($runs as $run) {
    $run = preg_replace('/  /', ' ', $run);
    $data = preg_split('/\s+/', $run);
    $id = $data[sizeof($data) - 3];
    $id = (int)$id;
    if ($id > 0 && $runindex > 0) {
        // Delete all but the first completed workflow run and this one
        // (the first is the currently executing one!)
        exec("gh api repos/brainboxdotcc/DPP/actions/runs/$id -X DELETE");
        sleep(1);
    }
    $runindex++;
}
