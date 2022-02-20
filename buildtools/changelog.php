<?php

// D++ changelog generator, saves 15 minutes for each release :-)

// Pattern list
$categories = [
	'break'			=>	'## 💣 Breaking Changes',
	'breaking'		=>	'## 💣 Breaking Changes',
	'feat'			=>	'## ✨ New Features',
	'feature'		=>	'## ✨ New Features',
	'add'			=>	'## ✨ New Features',
	'added'			=>	'## ✨ New Features',
	'fix'			=> 	'## 🐞 Bug Fixes',
	'bug'			=> 	'## 🐞 Bug Fixes',
	'bugfix'		=> 	'## 🐞 Bug Fixes',
	'fixed'			=> 	'## 🐞 Bug Fixes',
	'fixes'                 =>      '## 🐞 Bug Fixes',
	'perf'			=>	'## 🚀 Performance Improvements',
	'performance'	=>	'## 🚀 Performance Improvements',
	'impro'			=> 	'## ♻️ Refactoring',
	'improved'		=> 	'## ♻️ Refactoring',
	'improvement'	=> 	'## ♻️ Refactoring',
	'refactor'		=> 	'## ♻️ Refactoring',
	'refactored'	=> 	'## ♻️ Refactoring',
	'deprecated'    =>      '## ♻️ Refactoring',
	'deprecate'    =>      '## ♻️ Refactoring',
	'remove'		=> 	'## ♻️ Refactoring',
	'change'		=> 	'## ♻️ Refactoring',
	'changed'		=> 	'## ♻️ Refactoring',
	'test'			=>	'## 🚨 Testing',
	'testing'		=>	'## 🚨 Testing',
	'ci' 			=>	'## 👷 Build/CI',
	'build' 		=>	'## 👷 Build/CI',
	'docs'			=> 	'## 📚 Documentation',
	'documentation'	=> 	'## 📚 Documentation',
	'style'			=> 	'## 💎 Style Changes',
	'chore' 		=>	'## 🔧 Chore',
	'misc' 		=>	'## 📜 Miscellaneous Changes',
	'update'          =>      '## 📜 Miscellaneous Changes',
	'updated'          =>      '## 📜 Miscellaneous Changes',
];

$catgroup = [];
$changelog = [];

// Magic sauce
exec("git log --format=\"%s\" $(git log --no-walk --tags | head -n1 | cut -d ' ' -f 2)..HEAD | grep -v '^Merge '", $changelog);

// Leadin
echo "The changelog is listed below:\n\nRelease Changelog\n===========\n";

foreach ($changelog as $change) {
	
	// Wrap anything that looks like a symbol name in backticks
	$change = preg_replace('/(\S+\(\)|\w+::\w+|dpp::\w+|utility::\w+|[\w+_]+_t|on_[\w+_]+)/', '`$1`', $change);
	$change = preg_replace("/vs(\d+)/", "Visual Studio $1", $change);
	$change = preg_replace("/\bfaq\b/", "FAQ", $change);
	
	// Match keywords against categories
	$matched = false;
	foreach ($categories as $cat => $header) {
		// Purposefully ignored
		if (preg_match("/^Merge (branch|pull request|remote-tracking branch) /", $change) or preg_match("/version bump/i", $change)) {
			$matched = true;
			continue;
		}
		// Groupings
		if ((preg_match("/^" . $cat . ":/i", $change)) or (preg_match("/^\[" . $cat . "\//i", $change)) or (preg_match("/^\[" . $cat . "\]/i", $change))or (preg_match("/^\[" . $cat . ":/i", $change)) or (preg_match("/^" . $cat . "\//i", $change)) or (preg_match("/^" . $cat . ":/i", $change))) {
			if (!isset($catgroup[$header])) {
				$catgroup[$header] = [];
			}
			$matched = true;
			$catgroup[$header][] = preg_replace("/^\S+\s+/", "", $change);
			break;
		} else if (preg_match("/^" . $cat . " /i", $change)) {
			if (!isset($catgroup[$header])) {
				$catgroup[$header] = [];
			}
			$matched = true;
			$catgroup[$header][] = $change;
			break;
		}
	}
}

// Output tidy formatting
foreach ($catgroup as $cat => $list) {
	echo "\n" . $cat . "\n";
	foreach ($list as $item) {
		// Exclude bad commit messages like 'typo fix', 'test push' etc by pattern
		if (!preg_match("/^(typo|test|fix)\s\w+$/", $item)) {
			echo "- " . $item . "\n";
		}
	}
}

// Leadout
echo "\n\n**Thank you for using D++!**\n\n";
