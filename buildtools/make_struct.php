<?php

chdir('buildtools');

require __DIR__ . '/vendor/autoload.php';

use Dpp\StructGeneratorInterface;

if (count($argv) < 2) {
    die("You must specify a generator type\n");
} else {
    $generatorName = $argv[1];
    $generator = new $generatorName();
}

chdir('..');

/* Get the content of all cluster source files into an array */
exec("cat src/dpp/cluster/*.cpp", $clustercpp);

/* These methods have signatures incompatible with this script */
$blacklist = [
    'channel_edit_permissions',
    'message_add_reaction',
    'message_delete_reaction',
    'message_delete_reaction_emoji',
    'message_delete_all_reactions',
    'message_delete_own_reaction',
    'message_get_reactions',
    'channel_typing',
];

/* The script cannot determine the correct return type of these methods,
 * so we specify it by hand here.
 */
$forcedReturn = [
    'direct_message_create' => 'message',
    'guild_get_members' => 'guild_member_map',
    'guild_search_members' => 'guild_member_map',
    'message_create' => 'message',
    'message_edit' => 'message',
];

/* Get the contents of cluster.h into an array */
$header = explode("\n", file_get_contents('include/dpp/cluster.h'));

/* Finite state machine state constants */
const STATE_SEARCH_FOR_FUNCTION = 0;
const STATE_IN_FUNCTION = 1;
const STATE_END_OF_FUNCTION = 2;

$state = STATE_SEARCH_FOR_FUNCTION;
$currentFunction = $parameters = $returnType = '';
$content = $generator->generateHeaderStart();
$cppcontent = $generator->generatecppStart();

if (!$generator->checkForChanges()) {
    exit(0);
}

/* Scan every line of the C++ source */
foreach ($clustercpp as $cpp) {
    /* Look for declaration of function body */
    if ($state == STATE_SEARCH_FOR_FUNCTION &&
        preg_match('/^\s*void\s+cluster::([^(]+)\s*\((.*)command_completion_event_t\s*callback\s*\)/', $cpp, $matches)) {
        $currentFunction = $matches[1];
        $parameters = preg_replace('/,\s*$/', '', $matches[2]);
        if (!in_array($currentFunction, $blacklist)) {
            $state = STATE_IN_FUNCTION;
        }
        /* Scan function body */
    } elseif ($state == STATE_IN_FUNCTION) {
        /* End of function */
        if (preg_match('/^\}\s*$/', $cpp)) {
            $state = STATE_END_OF_FUNCTION;
            /* look for the return type of the method */
        } elseif (preg_match('/rest_request<([^>]+)>/', $cpp, $matches)) {
            /* rest_request<T> */
            $returnType = $matches[1];
        } elseif (preg_match('/rest_request_list<([^>]+)>/', $cpp, $matches)) {
            /* rest_request_list<T> */
            $returnType = $matches[1] . '_map';
        } elseif (preg_match('/callback\(confirmation_callback_t\(\w+, ([^(]+)\(.*, \w+\)\)/', $cpp, $matches)) {
            /* confirmation_callback_t */
            $returnType = $matches[1];
        } elseif (!empty($forcedReturn[$currentFunction])) {
            /* Forced return type */
            $returnType = $forcedReturn[$currentFunction];
        }
    }
    /* Completed parsing of function body */
    if ($state == STATE_END_OF_FUNCTION && !empty($currentFunction) && !empty($returnType)) {
        if (!in_array($currentFunction, $blacklist)) {
            $parameterList = explode(',', $parameters);
            $parameterNames = [];
            foreach ($parameterList as $parameter) {
                $parts = explode(' ', trim($parameter));
                $parameterNames[] = trim(preg_replace('/[\s\*\&]+/', '', $parts[count($parts) - 1]));
            }
            $content .= getComments($generator, $currentFunction, $returnType, $parameterNames) . "\n";
            $fullParameters = getFullParameters($currentFunction, $parameterNames);
            $parameterNames = trim(join(', ', $parameterNames));
            if (!empty($parameterNames)) {
                $parameterNames = ', ' . $parameterNames;
            }
            $noDefaults = $parameters;
            $parameters = !empty($fullParameters) ? $fullParameters : $parameters;
            $content .= $generator->generateHeaderDef($returnType, $currentFunction, $parameters, $noDefaults, $parameterNames);
            $cppcontent .= $generator->generateCppDef($returnType, $currentFunction, $parameters, $noDefaults, $parameterNames);
        }
        $currentFunction = $parameters = $returnType = '';
        $state = STATE_SEARCH_FOR_FUNCTION;
    }
}
$content .= <<<EOT

/* End of auto-generated definitions */

EOT;
$cppcontent .= <<<EOT

};

/* End of auto-generated definitions */

EOT;

/**
 * @brief Get parameters of a function with defaults
 * @param string $currentFunction Current function name
 * @param array $parameters Parameter names
 * @return string Parameter list
 */
function getFullParameters(string $currentFunction, array $parameters): string
{
    global $header;
    foreach ($header as $line) {
        if (preg_match('/^\s*void\s+' . $currentFunction . '\s*\((.*' . join('.*', $parameters) . '.*)command_completion_event_t\s*callback\s*/', $line, $matches)) {
            return preg_replace('/,\s*$/', '', $matches[1]);
        }
    }
    return '';
}

/**
 * @brief Get the comment block of a function.
 * Adds see/return doxygen tags
 * @param string $currentFunction function name
 * @param string $returnType Return type of function
 * @param array $parameters Parameter names
 * @return string Comment block
 */
function getComments(StructGeneratorInterface $generator, string $currentFunction, string $returnType, array $parameters): string
{
    global $header;
    /* First find the function */
    foreach ($header as $i => $line) {
        if (preg_match('/^\s*void\s+' . $currentFunction . '\s*\(.*' . join('.*', $parameters) . '.*command_completion_event_t\s*callback\s*/', $line)) {
            /* Backpeddle */
            $lineIndex = 1;
            for ($n = $i; $n != 0; --$n, $lineIndex++) {
                $header[$n] = preg_replace('/^\t+/', '', $header[$n]);
                $header[$n] = preg_replace('/@see (.+?)$/', '@see dpp::cluster::' . $currentFunction . "\n * @see \\1", $header[$n]);
                $header[$n] = preg_replace('/@param callback .*$/', '@return ' . $returnType . ' returned object on completion', $header[$n]);
                if (preg_match('/\s*\* On success /i', $header[$n])) {
                    $header[$n] = "";
                }
                if (preg_match('/\s*\/\*\*\s*$/', $header[$n])) {
                    $part = array_slice($header, $n, $lineIndex - 1);
                    array_splice($part, count($part) - 1, 0, $generator->getCommentArray());
                    return str_replace("\n\n", "\n", join("\n", $part));
                }
            }
            return '';
        }
    }
    return '';
}

/* Finished parsing, output autogenerated files */
$generator->saveHeader($content);
$generator->savecpp($cppcontent);
