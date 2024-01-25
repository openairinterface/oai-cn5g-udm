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

#include "udm_config.hpp"

#include <iostream>
#include <libconfig.h++>

#include "api_helper.h"
#include "common_defs.h"
#include "fqdn.hpp"
#include "if.hpp"
#include "logger.hpp"
#include "string.hpp"
#include "udm.h"

#include <arpa/inet.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

namespace oai::udm::config {
using namespace oai::udm::api;

//------------------------------------------------------------------------------
udm_config::udm_config() : instance(0), pid_dir(), udm_name(), sbi() {
  udr_addr.ipv4_addr.s_addr = INADDR_ANY;
  udr_addr.port             = 8080;  // HTTP2 by default
  udr_addr.api_version      = "v1";
  nrf_addr.ipv4_addr.s_addr = INADDR_ANY;
  nrf_addr.port             = 8080;  // HTTP2 by default
  nrf_addr.api_version      = "v1";
  use_http2                 = false;
  register_nrf              = false;
  log_level                 = spdlog::level::debug;
  curl_timeout              = 3000;
}

//------------------------------------------------------------------------------
udm_config::~udm_config() {}

//------------------------------------------------------------------------------
std::string udm_config::get_udr_slice_selection_subscription_data_retrieval_uri(
    const std::string& supi, const oai::model::common::PlmnId& plmn_id) {
  return get_udr_uri_base() + "/subscription-data/" + supi + "/" +
         plmn_id.getMcc() + plmn_id.getMnc() + "/provisioned-data/am-data";
}

//------------------------------------------------------------------------------
std::string udm_config::get_udr_access_and_mobility_subscription_data_uri(
    const std::string& supi, const oai::model::common::PlmnId& plmn_id) {
  return get_udr_uri_base() + "/subscription-data/" + supi + "/" +
         plmn_id.getMcc() + plmn_id.getMnc() + "/provisioned-data/am-data";
}

//------------------------------------------------------------------------------
std::string udm_config::get_udr_session_management_subscription_data_uri(
    const std::string& supi, const oai::model::common::PlmnId& plmn_id) {
  return get_udr_uri_base() + "/subscription-data/" + supi + "/" +
         plmn_id.getMcc() + plmn_id.getMnc() + "/provisioned-data/sm-data";
}

//------------------------------------------------------------------------------
std::string udm_config::get_udr_smf_selection_subscription_data_uri(
    const std::string& supi, const oai::model::common::PlmnId& plmn_id) {
  return get_udr_uri_base() + "/subscription-data/" + supi + "/" +
         plmn_id.getMcc() + plmn_id.getMnc() +
         "/provisioned-data/smf-selection-subscription-data";
}

//------------------------------------------------------------------------------
std::string udm_config::get_udr_sdm_subscriptions_uri(const std::string& supi) {
  return get_udr_uri_base() + "/subscription-data/" + supi +
         "/context-data/sdm-subscriptions";
}

//------------------------------------------------------------------------------
std::string udm_config::get_udr_authentication_subscription_uri(
    const std::string& supi) {
  std::string udr_path_fmt = {};
  api_helper::get_fmt_format_form(
      api_helper::UdrDrPathSubscriptionDataAuthenticationSubscription,
      udr_path_fmt);
  return get_udr_uri_base() + fmt::format(udr_path_fmt, supi);
}

//------------------------------------------------------------------------------
std::string udm_config::get_udr_authentication_status_uri(
    const std::string& supi) {
  return get_udr_uri_base() + "/subscription-data/" + supi +
         "/authentication-data/authentication-status";
}

//------------------------------------------------------------------------------
std::string udm_config::get_udr_amf_3gpp_registration_uri(
    const std::string& supi) {
  return get_udr_uri_base() + "/subscription-data/" + supi +
         "/context-data/amf-3gpp-access";
}

//------------------------------------------------------------------------------
std::string udm_config::get_udm_ueau_base() {
  return udr_addr.uri_root +
         oai::udm::api::api_helper::UeAuthenticationServiceBase;
}
//------------------------------------------------------------------------------
std::string udm_config::get_udr_uri_base() {
  return udr_addr.uri_root + api_helper::UdrDataRepositoryBase +
         udr_addr.api_version;
}

}  // namespace oai::udm::config
