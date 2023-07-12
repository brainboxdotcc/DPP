<?php

$header = "#pragma once\n\nnamespace dpp { namespace unicode_emoji {\n";

$emojis = json_decode(file_get_contents("https://raw.githubusercontent.com/ArkinSolomon/discord-emoji-converter/master/emojis.json"));
if ($emojis) {
    foreach ($emojis as $name=>$code) {
        if (preg_match("/^\d+/", $name)) {
            $name = "_" . $name;
        }
        $header .= "	constexpr std::string " .$name . "(\"$code\");\n";
    }
    $header .= "}\n};\n";
    file_put_contents("include/dpp/unicode_emoji.h", $header);
}
