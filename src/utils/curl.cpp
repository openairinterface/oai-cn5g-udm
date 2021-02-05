/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this
 *file except in compliance with the License. You may obtain a copy of the
 *License at
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

/*! \file curl.cpp
 \brief
 \author  Hongxin WANG, BUPT
 \date 2021
 \email: contact@openairinterface.org
 */

#include "curl.hpp"

using namespace config;
extern udm_config udm_cfg;

std::size_t callback(const char *in, std::size_t size, std::size_t num,
                     std::string *out) {
  const std::size_t totalBytes(size * num);
  out->append(in, totalBytes);
  return totalBytes;
}

long Curl::curl_http_client(std::string remoteUri, std::string Method,
                            std::string msgBody, std::string &Response) {

  Logger::udm_ueau().info("Send HTTP message with body %s", msgBody.c_str());

  uint32_t str_len = msgBody.length();
  char *body_data = (char *)malloc(str_len + 1);
  memset(body_data, 0, str_len + 1);
  memcpy((void *)body_data, (void *)msgBody.c_str(), str_len);

  curl_global_init(CURL_GLOBAL_ALL);
  CURL *curl = curl_easy_init();
  long httpCode = {0};

  if (curl) {
    CURLcode res = {};
    struct curl_slist *headers = nullptr;
    if (!Method.compare("POST") || !Method.compare("PUT") ||
        !Method.compare("PATCH")) {
      std::string content_type = "Content-Type: application/json";
      headers = curl_slist_append(headers, content_type.c_str());
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    curl_easy_setopt(curl, CURLOPT_URL, remoteUri.c_str());
    if (!Method.compare("POST"))
      curl_easy_setopt(curl, CURLOPT_HTTPPOST, 1);
    else if (!Method.compare("PUT"))
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    else if (!Method.compare("DELETE"))
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    else if (!Method.compare("PATCH"))
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    else
      curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, CURL_TIMEOUT_MS);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1);
    curl_easy_setopt(curl, CURLOPT_INTERFACE, udm_cfg.nudr.if_name.c_str());
    Logger::udm_ueau().info("[CURL] request sent by interface " +
                            udm_cfg.nudr.if_name);

    // Response information.
    std::unique_ptr<std::string> httpData(new std::string());
    std::unique_ptr<std::string> httpHeaderData(new std::string());

    // Hook up data handling function.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, httpData.get());
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, httpHeaderData.get());
    if (!Method.compare("POST") || !Method.compare("PUT") ||
        !Method.compare("PATCH")) {
      curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, msgBody.length());
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_data);
    }
    res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    // get the response
    std::string response = *httpData.get();
    std::string json_data_response = "";
    std::string resMsg = "";
    bool is_response_ok = true;
    Logger::udm_ueau().info("Get response with httpcode (%d)", httpCode);

    if (httpCode == 0) {
      Logger::udm_ueau().info("Cannot get response when calling %s",
                              remoteUri.c_str());
      // free curl before returning
      curl_slist_free_all(headers);
      curl_easy_cleanup(curl);
      return httpCode;
    }

    nlohmann::json response_data = {};

    if (httpCode != 200 && httpCode != 201 && httpCode != 204) {
      is_response_ok = false;
      if (response.size() < 1) {
        Logger::udm_ueau().info("There's no content in the response");
        // TODO: send context response error
        return httpCode;
      }
      Logger::udm_ueau().info("Wrong response code");

      return httpCode;
    }

    else { // httpCode = 200 || httpCode = 201 || httpCode = 204
      /*
      //store location of the created context
      std::string header_response = *httpHeaderData.get();
      std::string CRLF = "\r\n";
      std::size_t location_pos = header_response.find("Location");

      if (location_pos != std::string::npos)
      {
        std::size_t crlf_pos = header_response.find(CRLF, location_pos);
        if (crlf_pos != std::string::npos)
        {
          std::string location = header_response.substr(location_pos + 10,
      crlf_pos - (location_pos + 10)); printf("Location of the created SMF
      context: %s", location.c_str());

        }
      }

      try
      {
        response_data = nlohmann::json::parse(response);
      }
      catch (nlohmann::json::exception &e)
      {
        printf("Could not get Json content from the response");
        //Set the default Cause
        response_data["error"]["cause"] = "504 Gateway Timeout";
      }*/

      Response = *httpData.get();
    }

    if (!is_response_ok) {
      try {
        response_data = nlohmann::json::parse(json_data_response);
      } catch (nlohmann::json::exception &e) {
        Logger::udm_ueau().info("Could not get Json content from the response");
        // Set the default Cause
        response_data["error"]["cause"] = "504 Gateway Timeout";
      }

      Logger::udm_ueau().info("Get response with jsonData: %s",
                              json_data_response.c_str());

      std::string cause = response_data["error"]["cause"];
      Logger::udm_ueau().info("Call Network Function services failure");
      Logger::udm_ueau().info("Cause value: %s", cause.c_str());
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
  }

  curl_global_cleanup();

  if (body_data) {
    free(body_data);
    body_data = NULL;
  }
  fflush(stdout);

  return httpCode;
}
