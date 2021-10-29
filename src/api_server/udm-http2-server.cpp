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

/*! \file udm_http2-server.cpp
 \brief
 \author  Tien-Thinh NGUYEN
 \company Eurecom
 \date 2020
 \email: tien-thinh.nguyen@eurecom.fr
 */

#include "udm-http2-server.h"
#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <regex>
#include <nlohmann/json.hpp>
#include <string>
#include "string.hpp"

#include "logger.hpp"
#include "udm_config.hpp"
#include "3gpp_29.500.h"

using namespace nghttp2::asio_http2;
using namespace nghttp2::asio_http2::server;

using namespace oai::udm::config;
using namespace oai::udm::model;

extern udm_config udm_cfg;

//------------------------------------------------------------------------------
void udm_http2_server::start() {
  boost::system::error_code ec;

  Logger::udm_server().info("HTTP2 server started");

  server.handle(
      NUDM_SDM_BASE + udm_cfg.sbi.api_version,
      [&](const request& request, const response& response) {
        request.on_data([&](const uint8_t* data, std::size_t len) {
          std::string msg((char*) data, len);
          try {
            std::vector<std::string> split_result;
            boost::split(
                split_result, request.uri().path, boost::is_any_of("/"));
            if (request.method().compare("POST") == 0 && len > 0) {
            }
          } catch (std::exception& e) {
            Logger::udm_server().warn("Invalid request (error: %s)!", e.what());
            response.write_head(
                http_status_code_e::HTTP_STATUS_CODE_400_BAD_REQUEST);
            response.end();
            return;
          }
        });
      });

  if (server.listen_and_serve(ec, m_address, std::to_string(m_port))) {
    std::cerr << "HTTP Server error: " << ec.message() << std::endl;
  }
}

void udm_http2_server::generate_auth_data_request(
    const std::string& supiOrSuci,
    const oai::udm::model::AuthenticationInfoRequest& authenticationInfoRequest,
    const response& response) {}

void udm_http2_server::confirm_auth(
    const std::string& supi, const oai::udm::model::AuthEvent& authEvent,
    const response& response) {}

void udm_http2_server::delete_auth(
    const std::string& supi, const std::string& authEventId,
    const oai::udm::model::AuthEvent& authEvent, const response& response) {}

void udm_http2_server::access_mobility_subscription_data_retrieval_handler(
    const std::string& supi, const response& response,
    oai::udm::model::PlmnId PlmnId) {}

void udm_http2_server::amf_registration_for_3gpp_access_handler(
    const std::string& ue_id,
    const oai::udm::model::Amf3GppAccessRegistration&
        amf_3gpp_access_registration,
    const response& response) {}

void udm_http2_server::session_management_subscription_data_retrieval_handler(
    const std::string& supi, const response& response,
    oai::udm::model::Snssai snssai, std::string dnn,
    oai::udm::model::PlmnId plmn_id) {}

void udm_http2_server::slice_selection_subscription_data_retrieval_handler(
    const std::string& supi, const response& response,
    std::string supported_features, oai::udm::model::PlmnId plmn_id) {}

void udm_http2_server::smf_selection_subscription_data_retrieval_handler(
    const std::string& supi, const response& response,
    std::string supported_features, oai::udm::model::PlmnId plmn_id) {}

void udm_http2_server::subscription_creation_handler(
    const std::string& supi,
    const oai::udm::model::SdmSubscription& sdmSubscription,
    const response& response) {}
