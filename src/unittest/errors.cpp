/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2021 Craig Edwards and D++ contributors
 * (https://github.com/brainboxdotcc/DPP/graphs/contributors)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ************************************************************************************/
#include "test.h"

/* Unit tests for human-readable error translation */
void errors_test() {
	set_test(ERRORS, false);

	/* Prepare a confirmation_callback_t in error state (400) */
	dpp::confirmation_callback_t error_test;
	bool error_message_success = false;
	error_test.http_info.status = 400;

	error_test.http_info.body = "{\
		\"message\": \"Invalid Form Body\",\
		\"code\": 50035,\
		\"errors\": {\
			\"options\": {\
				\"0\": {\
					\"name\": {\
						\"_errors\": [\
							{\
								\"code\": \"STRING_TYPE_REGEX\",\
								\"message\": \"String value did not match validation regex.\"\
							},\
							{\
								\"code\": \"APPLICATION_COMMAND_INVALID_NAME\",\
								\"message\": \"Command name is invalid\"\
							}\
						]\
					}\
				}\
			}\
		}\
	}";
	error_message_success = (error_test.get_error().human_readable == "50035: Invalid Form Body\n\t- options[0].name: String value did not match validation regex. (STRING_TYPE_REGEX)\n\t- options[0].name: Command name is invalid (APPLICATION_COMMAND_INVALID_NAME)");

	error_test.http_info.body = "{\
		\"message\": \"Invalid Form Body\",\
		\"code\": 50035,\
		\"errors\": {\
			\"type\": {\
				\"_errors\": [\
					{\
						\"code\": \"BASE_TYPE_CHOICES\",\
						\"message\": \"Value must be one of {4, 5, 9, 10, 11}.\"\
					}\
				]\
			}\
		}\
	}";
	error_message_success = (error_message_success && error_test.get_error().human_readable == "50035: Invalid Form Body - type: Value must be one of {4, 5, 9, 10, 11}. (BASE_TYPE_CHOICES)");

	error_test.http_info.body = "{\
		\"message\": \"Unknown Guild\",\
		\"code\": 10004\
	}";
	error_message_success = (error_message_success && error_test.get_error().human_readable == "10004: Unknown Guild");

	error_test.http_info.body = "{\
		\"message\": \"Invalid Form Body\",\
		\"code\": 50035,\
		\"errors\": {\
			\"allowed_mentions\": {\
				\"_errors\": [\
					{\
						\"code\": \"MESSAGE_ALLOWED_MENTIONS_PARSE_EXCLUSIVE\",\
						\"message\": \"parse:[\\\"users\\\"] and users: [ids...] are mutually exclusive.\"\
					}\
				]\
			}\
		}\
	}";
	error_message_success = (error_message_success && error_test.get_error().human_readable == "50035: Invalid Form Body - allowed_mentions: parse:[\"users\"] and users: [ids...] are mutually exclusive. (MESSAGE_ALLOWED_MENTIONS_PARSE_EXCLUSIVE)");

	error_test.http_info.body = "{\
		\"message\": \"Invalid Form Body\",\
		\"code\": 50035,\
		\"errors\": {\
			\"1\": {\
				\"options\": {\
					\"1\": {\
						\"description\": {\
							\"_errors\": [\
								{\
									\"code\": \"BASE_TYPE_BAD_LENGTH\",\
									\"message\": \"Must be between 1 and 100 in length.\"\
								}\
							]\
						}\
					}\
				}\
			}\
		}\
	}";
	error_message_success = (error_message_success && error_test.get_error().human_readable == "50035: Invalid Form Body - <array>[1].options[1].description: Must be between 1 and 100 in length. (BASE_TYPE_BAD_LENGTH)");

	set_test(ERRORS, error_message_success);
}
