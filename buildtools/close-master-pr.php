<?php

// Comment on and close all PRs targetted at master branch

// Magic sauce
exec("gh pr list --base master | sed 's/\|/ /' |awk '{print $1}'", $master_prs);

foreach ($master_prs as $pr) {

	$pr = (int)$pr;
	if ($pr > 0) {
		system("gh pr comment $pr -b \"You have opened a PR against the master branch. PRs must target the \`dev\` branch, as such this PR has been automatically closed. Please re-target your PR against the dev branch if you reopen it. Thank you for your contribution.\"");
		system("gh pr close $pr");
	}
}

