#pragma once

/* This is the snowflake ID of the bot's developer.
 * The eval command will be restricted to this user.
 */
#define MY_DEVELOPER 189759562910400512ULL

/* Any functions you want to be usable from within an eval,
 * that are not part of D++ itself or the message event, you
 * can put here as forward declarations. The test_function()
 * serves as an example.
 */

int test_function();
