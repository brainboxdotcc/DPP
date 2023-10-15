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

/* Unit tests for HTTPS client */
void http_client_tests(const std::string& token) {
	dpp::http_connect_info hci;
	set_test(HOSTINFO, false);

	hci = dpp::https_client::get_host_info("https://test.com:444");
	bool hci_test = (hci.scheme == "https" && hci.hostname == "test.com" && hci.port == 444 && hci.is_ssl == true);

	hci = dpp::https_client::get_host_info("https://test.com");
	hci_test = hci_test && (hci.scheme == "https" && hci.hostname == "test.com" && hci.port == 443 && hci.is_ssl == true);

	hci = dpp::https_client::get_host_info("http://test.com");
	hci_test = hci_test && (hci.scheme == "http" && hci.hostname == "test.com" && hci.port == 80 && hci.is_ssl == false);

	hci = dpp::https_client::get_host_info("http://test.com:90");
	hci_test = hci_test && (hci.scheme == "http" && hci.hostname == "test.com" && hci.port == 90 && hci.is_ssl == false);

	hci = dpp::https_client::get_host_info("test.com:97");
	hci_test = hci_test && (hci.scheme == "http" && hci.hostname == "test.com" && hci.port == 97 && hci.is_ssl == false);

	hci = dpp::https_client::get_host_info("test.com");
	hci_test = hci_test && (hci.scheme == "http" && hci.hostname == "test.com" && hci.port == 80 && hci.is_ssl == false);

	set_test(HOSTINFO, hci_test);

	set_test(HTTPS, false);
	if (!offline) {
		dpp::multipart_content multipart = dpp::https_client::build_multipart(
			"{\"content\":\"test\"}", {"test.txt", "blob.blob"}, {"ABCDEFGHI", "BLOB!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"}, {"text/plain", "application/octet-stream"}
		);
		try {
			dpp::https_client c("discord.com", 443, "/api/channels/" + std::to_string(TEST_TEXT_CHANNEL_ID) + "/messages", "POST", multipart.body,
					    {
						    {"Content-Type", multipart.mimetype},
						    {"Authorization", "Bot " + token}
					    }
			);
			std::string hdr1 = c.get_header("server");
			std::string content1 = c.get_content();
			set_test(HTTPS, hdr1 == "cloudflare" && c.get_status() == 200);
		}
		catch (const dpp::exception& e) {
			std::cout << e.what() << "\n";
			set_test(HTTPS, false);
		}
	}

	set_test(HTTP, false);
	try {
		dpp::https_client c2("github.com", 80, "/", "GET", "", {}, true);
		std::string hdr2 = c2.get_header("location");
		std::string content2 = c2.get_content();
		set_test(HTTP, hdr2 == "https://github.com/" && c2.get_status() == 301);
	}
	catch (const dpp::exception& e) {
		std::cout << e.what() << "\n";
		set_test(HTTP, false);
	}

	set_test(MULTIHEADER, false);
	try {
		dpp::https_client c2("www.google.com", 80, "/", "GET", "", {}, true);
		size_t count = c2.get_header_count("set-cookie");
		size_t count_list = c2.get_header_list("set-cookie").size();
		// Google sets a bunch of cookies when we start accessing it.
		set_test(MULTIHEADER, c2.get_status() == 200 && count > 1 && count == count_list);
	}
	catch (const dpp::exception& e) {
		std::cout << e.what() << "\n";
		set_test(MULTIHEADER, false);
	}
}
