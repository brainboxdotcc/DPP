/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
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
#include <dpp/cluster.h>
#include <dpp/json.h>

namespace dpp {


confirmation_callback_t::confirmation_callback_t(cluster* creator, const confirmable_t& _value, const http_request_completion_t& _http)
	: http_info(_http), value(_value), bot(creator)
{
	if (std::holds_alternative<confirmation>(_value)) {
		confirmation newvalue = std::get<confirmation>(_value);
		newvalue.success = (http_info.status < 400);
		value = newvalue;
	}
}

confirmation_callback_t::confirmation_callback_t(const http_request_completion_t& _http)
	: http_info(_http),  value(), bot(nullptr)
{
}

confirmation_callback_t::confirmation_callback_t(cluster* creator) : bot(creator) {
	http_info = {};
	value = {};
}

bool confirmation_callback_t::is_error() const {
	if (http_info.status >= 400) {
		/* Invalid JSON or 4xx/5xx response */
		return true;
	}
	if (http_info.status == 204) {
		/* Body is empty so we can't parse it but interaction is not an error*/
		return false;
	}
	try {
		json j = json::parse(this->http_info.body);
		if (j.find("code") != j.end() && j.find("errors") != j.end() && j.find("message") != j.end()) {
			if (j["code"].is_number_unsigned() && j["errors"].is_object() && j["message"].is_string()) {
				return true;
			} else {
				return false;
			}
		}
		return false;
	}
	catch (const std::exception &) {
		/* JSON parse error indicates the content is not JSON.
		 * This means that its an empty body e.g. 204 response, and not an actual error.
		 */
		return false;
	}
}

error_info confirmation_callback_t::get_error() const {
	if (is_error()) {
		json j = json::parse(this->http_info.body);
		error_info e;

		set_int32_not_null(&j, "code", e.code);
		set_string_not_null(&j, "message", e.message);
		json& errors = j["errors"];
		for (auto obj = errors.begin(); obj != errors.end(); ++obj) {

			/* Arrays in the error report are numerically indexed with a number in a string. Ugh. */
			if (isdigit(*(obj.key().c_str()))) {
				/* An array of error messages */
				int array_index = std::atoll(obj.key().c_str());
				for (auto index = obj->begin(); index != obj->end(); ++index) {
					if (index->find("_errors") != index->end()) {
						/* A single object where one or more fields generated an error */
						for (auto errordetails = (*index)["_errors"].begin(); errordetails != (*index)["_errors"].end(); ++errordetails) {
							error_detail detail;
							detail.code = (*errordetails)["code"].get<std::string>();
							detail.reason = (*errordetails)["message"].get<std::string>();
							detail.object.clear();
							detail.field = obj.key();
							detail.index = array_index;
							e.errors.emplace_back(detail);
						}
					} else {
						/* An object where one or more fields within it generated an error, e.g. slash command */
						for (auto fields = index->begin(); fields != index->end(); ++fields) {
							if (fields->find("_errors") != fields->end()) {
								for (auto errordetails = (*fields)["_errors"].begin(); errordetails != (*fields)["_errors"].end(); ++errordetails) {
									error_detail detail;
									detail.code = (*errordetails)["code"].get<std::string>();
									detail.reason = (*errordetails)["message"].get<std::string>();
									detail.field = fields.key();
									detail.object = obj.key();
									detail.index = array_index;
									e.errors.emplace_back(detail);
								}
							} else {
								/* An array of objects where one or more generated an error, e.g. slash command bulk registration */
								for (auto fields2 = fields->begin(); fields2 != fields->end(); ++fields2) {
									for (auto errordetails = (*fields2)["_errors"].begin(); errordetails != (*fields2)["_errors"].end(); ++errordetails) {
										error_detail detail;
										detail.code = (*errordetails)["code"].get<std::string>();
										detail.reason = (*errordetails)["message"].get<std::string>();
										detail.field = index.key() + "[" + fields.key() +  "]." + fields2.key();
										detail.object = obj.key();
										detail.index = array_index;
										e.errors.emplace_back(detail);
									}
								}
							}
						}
					}
				}

			} else if (obj->find("_errors") != obj->end()) {
				/* An object of error messages (rare) */
				e.errors.reserve((*obj)["_errors"].size());
				for (auto errordetails = (*obj)["_errors"].begin(); errordetails != (*obj)["_errors"].end(); ++errordetails) {
					error_detail detail;
					detail.code = (*errordetails)["code"].get<std::string>();
					detail.reason = (*errordetails)["message"].get<std::string>();
					detail.object.clear();
					detail.field = obj.key();
					detail.index = 0;
					e.errors.emplace_back(detail);
				}
			} else {
				/* An object that has a subobject with errors */
				for (auto index = obj->begin(); index != obj->end(); ++index) {
					int array_index = std::atoll(index.key().c_str());
					for (auto index2 = index->begin(); index2 != index->end(); ++index2) {
						if (index2->find("_errors") != index2->end()) {
							/* A single object where one or more fields generated an error */
							for (auto errordetails = (*index2)["_errors"].begin(); errordetails != (*index2)["_errors"].end(); ++errordetails) {
								error_detail detail;
								detail.code = (*errordetails)["code"].get<std::string>();
								detail.reason = (*errordetails)["message"].get<std::string>();
								detail.object = obj.key();
								detail.field = index2.key();
								detail.index = array_index;
								e.errors.emplace_back(detail);
							}
						}
					}
				}
			}
		}

		e.human_readable = std::to_string(e.code) + ": " + e.message;
		std::string prefix = e.errors.size() == 1 ? " " : "\n\t";
		for (const auto& error : e.errors) {
			if (error.object.empty()) {
				/* A singular field with an error in an unnamed object */
				e.human_readable += prefix + "- " + error.field + ": " + error.reason + " (" + error.code + ")";
			} else if (isdigit(*(error.object.c_str()))) {
				/* An unnamed array of objects where one or more generated an error, e.g. slash command bulk registration */
				e.human_readable += prefix + "- <array>[" + error.object + "]." + error.field + ": " + error.reason + " (" + error.code + ")";
			} else {
				/* A named array of objects whre a field in the object has an error */
				e.human_readable += prefix + "- " + error.object + "[" + std::to_string(error.index) + "]." + error.field + ": " + error.reason + " (" + error.code + ")";
			}
		}

		return e;
	}
	return error_info();
}

} // namespace dpp
