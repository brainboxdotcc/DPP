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
#pragma once
#include <dpp/export.h>
#include <dpp/sslclient.h>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace dpp {

/**
 * @brief HTTP connection status
 */
enum http_state : uint8_t
{
  /**
   * @brief Sending/receiving HTTP headers and request body
   */
  HTTPS_HEADERS,

  /**
   * @brief Receiving body content.
   */
  HTTPS_CONTENT,

  /**
   * @brief Completed connection, as it was closed or the body is >=
   * Content-Length
   *
   */
  HTTPS_DONE,

  HTTPS_CHUNK_LEN,

  HTTPS_CHUNK_TRAILER,

  HTTPS_CHUNK_LAST,

  HTTPS_CHUNK_CONTENT
};

/**
 * @brief Request headers
 */
using http_headers = std::multimap<std::string, std::string>;

/**
 * @brief Represents a multipart mime body and the correct top-level mime type
 * If a non-multipart request is passed in, this is represented as a plain body
 * and the application/json mime type.
 */
struct multipart_content {
  /**
   * @brief Multipart body
   */
  std::string body;
  /**
   * @brief MIME type
   */
  std::string mimetype;
};

/**
 * @brief Represents a HTTP scheme, hostname and port
 * split into parts for easy use in https_client.
 */
struct http_connect_info {
  /**
   * @brief True if the connection should be SSL
   */
  bool is_ssl;
  /**
   * @brief The request scheme, e.g. 'https' or 'http'
   */
  std::string scheme;
  /**
   * @brief The request hostname part, e.g. 'discord.com'
   */
  std::string hostname;
  /**
   * @brief The port number, either determined from the scheme,
   * or from the part of the hostname after a colon ":" character
   */
  uint16_t port;
};

/**
 * @brief Implements a HTTPS socket client based on the SSL client.
 * @note plaintext HTTP without SSL is also supported via a "downgrade" setting
 */
class DPP_EXPORT https_client : public ssl_client
{
  /**
   * @brief Current connection state
   */
  http_state state;

  /**
   * @brief The type of the request, e.g. GET, POST
   */
  std::string request_type;

  /**
   * @brief Path part of URL for HTTPS connection
   */
  std::string path;

  /**
   * @brief The request body, e.g. form data
   */
  std::string request_body;

  /**
   * @brief The response body, e.g. file content or JSON
   */
  std::string body;

  /**
   * @brief The reported length of the content. If this is
   * UULONG_MAX, then no length was reported by the server.
   */
  uint64_t content_length;

  /**
   * @brief Headers for the request, e.g. Authorization, etc.
   */
  http_headers request_headers;

  /**
   * @brief The status of the HTTP request from the server,
   * e.g. 200 for OK, 404 for not found. A value of 0 means
   * no request has been completed.
   */
  uint16_t status;

  /**
   * @brief Time at which the request should be abandoned
   */
  time_t timeout;

  bool chunked;
  bool waiting_end_marker;
  size_t chunk_size;
  size_t chunk_receive;

  /**
   * @brief Headers from the server's response, e.g. RateLimit
   * headers, cookies, etc.
   */
  std::map<std::string, std::string> response_headers;

  bool do_buffer(std::string &buffer);

protected:
  /**
   * @brief Start the connection
   */
  void connect() override;

  /**
   * @brief Get request state
   * @return request state
   */
  http_state get_state();

public:
  /**
   * @brief Connect to a specific HTTP(S) server and complete a request.
   *
   * The constructor will attempt the connection, and return the content.
   * By the time the constructor completes, the HTTP request will be stored
   * in the object.
   *
   * @note This is a blocking call. It starts a loop which runs non-blocking
   * functions within it, but does not return until the request completes.
   * See queues.cpp for how to make this asynchronous.
   *
   * @param hostname Hostname to connect to
   * @param port Port number to connect to, usually 443 for SSL and 80 for
   * plaintext
   * @param urlpath path part of URL, e.g. "/api"
   * @param verb Request verb, e.g. GET or POST
   * @param req_body Request body, use dpp::https_client::build_multipart() to
   * build a multipart MIME body (e.g. for multiple file upload)
   * @param extra_headers Additional request headers, e.g. user-agent,
   * authorization, etc
   * @param plaintext_connection Set to true to make the connection plaintext
   * (turns off SSL)
   * @param request_timeout How many seconds before the connection is considered
   * failed if not finished
   */
  https_client(const std::string &hostname, uint16_t port = 443,
               const std::string &urlpath = "/",
               const std::string &verb = "GET",
               const std::string &req_body = "",
               const http_headers &extra_headers = {},
               bool plaintext_connection = false, uint16_t request_timeout = 5);

  /**
   * @brief Destroy the https client object
   */
  ~https_client() override;

  /**
   * @brief Build a multipart content from a set of files and some json
   *
   * @param json The json content
   * @param filenames File names of files to send
   * @param contents Contents of each of the files to send
   * @return multipart mime content and headers
   */
  static multipart_content
  build_multipart(const std::string &json,
                  const std::vector<std::string> &filenames = {},
                  const std::vector<std::string> &contents = {});

  /**
   * @brief Processes incoming data from the SSL socket input buffer.
   *
   * @param buffer The buffer contents. Can modify this value removing the head
   * elements when processed.
   */
  bool handle_buffer(std::string &buffer) override;

  /**
   * @brief Close HTTPS socket
   */
  void close() override;

  /** Fires every second from the underlying socket I/O loop, used for sending
   * websocket pings */
  void one_second_timer() override;

  /**
   * @brief Get a HTTP response header
   *
   * @param header_name Header name to find, case insensitive
   * @return Header content or empty string if not found
   */
  [[nodiscard]] const std::string get_header(std::string header_name) const;

  /**
   * @brief Get all HTTP response headers
   *
   * @return headers as a map
   */
  [[nodiscard]] const std::map<std::string, std::string> get_headers() const;

  /**
   * @brief Get the response content
   *
   * @return response content
   */
  [[nodiscard]] const std::string get_content() const;

  /**
   * @brief Get the response HTTP status, e.g.
   * 200 for OK, 404 for not found, 429 for rate limited.
   * A value of 0 indicates the request was not completed.
   *
   * @return uint16_t HTTP status
   */
  [[nodiscard]] uint16_t get_status() const;

  /**
   * @brief Break down a scheme, hostname and port into
   * a http_connect_info.
   *
   * All but the hostname portion are optional. The path component
   * should not be passed to this function.
   *
   * @param url URL to break down
   * @return Split URL
   */
  static http_connect_info get_host_info(std::string url);
};

}; // namespace dpp