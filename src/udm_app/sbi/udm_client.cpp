/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this
 * file except in compliance with the License. You may obtain a copy of the
 * License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include "udm_client.hpp"

#include <curl/curl.h>
//#include <pistache/http.h>
#include <nlohmann/json.hpp>
//#include <stdexcept>

#include "logger.hpp"
#include "udm.h"
#include "udm_config.hpp"
#include "utils.hpp"

using namespace oai::udm::app;
using json = nlohmann::json;

using namespace oai::udm::config;
extern udm_config udm_cfg;

//------------------------------------------------------------------------------
// To read content of the response from NF
static std::size_t callback(
    const char* in, std::size_t size, std::size_t num, std::string* out) {
  const std::size_t totalBytes(size * num);
  out->append(in, totalBytes);
  return totalBytes;
}

//------------------------------------------------------------------------------
udm_client::udm_client() {}

//------------------------------------------------------------------------------
udm_client::~udm_client() {
  Logger::udm_server().debug("Delete UDM Client instance...");
}

//------------------------------------------------------------------------------
bool udm_client::send_request(
    const std::string& remote_uri, const http_method_e& method,
    const std::string& msg_body, std::string& response, long& response_code) {
  Logger::udm_app().info("Send HTTP message with body %s", msg_body.c_str());

  bool result = false;

  uint32_t str_len = msg_body.length();
  char* body_data  = (char*) malloc(str_len + 1);
  memset(body_data, 0, str_len + 1);
  memcpy((void*) body_data, (void*) msg_body.c_str(), str_len);

  curl_global_init(CURL_GLOBAL_ALL);
  CURL* curl = curl_easy_init();

  if (curl) {
    CURLcode res               = {};
    struct curl_slist* headers = nullptr;
    if ((method == http_method_e::POST) or (method == http_method_e::PUT) or
        (method == http_method_e::PATCH)) {
      std::string content_type = "Content-Type: application/json";
      headers = curl_slist_append(headers, content_type.c_str());
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    curl_easy_setopt(curl, CURLOPT_URL, remote_uri.c_str());
    if (method == http_method_e::POST)
      curl_easy_setopt(curl, CURLOPT_HTTPPOST, 1);
    else if (method == http_method_e::PUT)
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    else if (method == http_method_e::DELETE)
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    else if (method == http_method_e::PATCH)
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    else
      curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);

    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, udm_cfg.curl_timeout);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1);
    curl_easy_setopt(curl, CURLOPT_INTERFACE, udm_cfg.sbi.if_name.c_str());

    if (udm_cfg.use_http2) {
      if (Logger::should_log(spdlog::level::debug))
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      // we use a self-signed test server, skip verification during debugging
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      curl_easy_setopt(
          curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE);
    }

    // response information
    std::unique_ptr<std::string> http_header_data(new std::string());

    // Hook up data handling function
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, http_header_data.get());

    if ((method == http_method_e::POST) or (method == http_method_e::PUT) or
        (method == http_method_e::PATCH)) {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, msg_body.length());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_data);
    }

    int num_retries = 0;
    while (num_retries < kNumberOfCurlRetries) {
      num_retries++;
      res = curl_easy_perform(curl);
      if (res != CURLE_OK) {
        // Sleep between two consecutive retries
        usleep(kBaseTimeIntervalBetweenCurlRetries * pow(2, num_retries - 1));
        Logger::udm_app().debug("Curl retry %d ...", num_retries);
        continue;
      } else {
        break;
      }
    }

    if (res != CURLE_OK) {
      Logger::udm_app().debug(
          "Still could not reach the destination after %d retries",
          kNumberOfCurlRetries);
      // TODO: set Problem details
    } else {
      result = true;
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
      Logger::udm_app().debug(
          "Get response with HTTP code (%d)", response_code);
      // HttpStatusCode response_code_e =
      //     utils::from_int_to_enum_type(response_code);

      if (response_code == 0) {
        Logger::udm_app().info(
            "Cannot get response when calling %s", remote_uri.c_str());
        // TODO: set problem details
        // free curl before returning
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        utils::free_wrapper((void**) &body_data);
        return false;
      }

      // Process the response
      if (!response.empty())
        Logger::udm_app().info("Get response with data: %s", response.c_str());
      // TODO: set Problem details
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  } else {
    curl_easy_cleanup(curl);
  }

  curl_global_cleanup();
  utils::free_wrapper((void**) &body_data);

  return result;
}
