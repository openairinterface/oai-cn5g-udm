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

/*! \file udm_nrf.cpp
 \brief
 \author  Tien-Thinh NGUYEN Rohan Kharade
 \company Eurecom
 \date 2020
 \email:
 */

#include "udm_nrf.hpp"
#include "udm_app.hpp"
#include "udm_profile.hpp"
#include "udm_client.hpp"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <curl/curl.h>
#include <pistache/http.h>
#include <pistache/mime.h>
#include <nlohmann/json.hpp>
#include <stdexcept>

#include "logger.hpp"
#include "udm.h"

using namespace oai::udm::app;
using namespace oai::udm::config;

// using json = nlohmann::json;

extern udm_config udm_cfg;
extern udm_nrf* udm_nrf_inst;
udm_client* udm_client_instance = nullptr;

//------------------------------------------------------------------------------
udm_nrf::udm_nrf() {}
//---------------------------------------------------------------------------------------------
void udm_nrf::get_udm_api_root(std::string& api_root) {
  api_root =
      std::string(inet_ntoa(*((struct in_addr*) &udm_cfg.nrf_addr.ipv4_addr))) +
      ":" + std::to_string(udm_cfg.nrf_addr.port) + NNRF_NFM_BASE +
      udm_cfg.nrf_addr.api_version;
}

//---------------------------------------------------------------------------------------------
void udm_nrf::generate_udm_profile(
    udm_profile& udm_nf_profile, std::string& udm_instance_id) {
  // TODO: remove hardcoded values
  udm_nf_profile.set_nf_instance_id(udm_instance_id);
  udm_nf_profile.set_nf_instance_name("OAI-UDM");
  udm_nf_profile.set_fqdn("oai-udm");
  udm_nf_profile.set_nf_type("UDM");
  udm_nf_profile.set_nf_status("REGISTERED");
  udm_nf_profile.set_nf_heartBeat_timer(50);
  udm_nf_profile.set_nf_priority(1);
  udm_nf_profile.set_nf_capacity(100);
  // udm_nf_profile.set_fqdn(udm_cfg.fqdn);
  udm_nf_profile.add_nf_ipv4_addresses(udm_cfg.sbi.addr4);  // N4's Addr

  // UDM info (Hardcoded for now)
  // ToDo: If none of these parameters are provided, the UDM can serve any
  // external group and any SUPI or GPSI managed by the PLMN of the UDM
  // instance. If "supiRanges", "gpsiRanges" and
  // "externalGroupIdentifiersRanges" attributes are absent, and "groupId" is
  // present, the SUPIs / GPSIs / ExternalGroups served by this UDM instance is
  // determined by the NRF (see 3GPP TS 23.501 [2], clause 6.2.6.2)
  udm_info_t udm_info_item;
  udm_info_item.groupid = "oai-udm-testgroupid";
  udm_info_item.routing_indicators.push_back("0210");
  udm_info_item.routing_indicators.push_back("9876");
  supi_range_udm_info_item_t supi_ranges;
  supi_ranges.supi_range.start   = "208950000000031";
  supi_ranges.supi_range.pattern = "^imsi-20895[31-131]{10}$";
  supi_ranges.supi_range.end     = "208950000000131";
  udm_info_item.supi_ranges.push_back(supi_ranges);
  identity_range_udm_info_item_t gpsi_ranges;
  gpsi_ranges.identity_range.start   = "752740000";
  gpsi_ranges.identity_range.pattern = "^gpsi-75274[0-9]{4}$";
  gpsi_ranges.identity_range.end     = "752749999";
  udm_info_item.gpsi_ranges.push_back(gpsi_ranges);
  udm_nf_profile.set_udm_info(udm_info_item);
  // ToDo:- Add remaining fields
  // identity_range_udm_info_item_t ext_grp_id_ranges;
  // internal_grpid_range_udm_info_item_t int_grp_id_ranges;
  // UDM info item end

  udm_nf_profile.display();
}
//---------------------------------------------------------------------------------------------
void udm_nrf::register_to_nrf() {
  // generate UUID
  std::string udm_instance_id;  // UDM instance id
  udm_instance_id              = to_string(boost::uuids::random_generator()());
  nlohmann::json response_data = {};

  // Generate NF Profile
  udm_profile udm_nf_profile;
  generate_udm_profile(udm_nf_profile, udm_instance_id);

  // Send NF registeration request
  std::string udm_api_root = {};
  std::string response     = {};
  std::string method       = {"PUT"};
  get_udm_api_root(udm_api_root);
  std::string remoteUri = udm_api_root + UDM_NF_REGISTER_URL + udm_instance_id;
  nlohmann::json json_data = {};
  udm_nf_profile.to_json(json_data);

  Logger::udm_nrf().info("Sending NF registeration request");
  udm_client_instance->curl_http_client(
      remoteUri, method, response, json_data.dump().c_str());

  try {
    response_data = nlohmann::json::parse(response);
    if (response_data["nfStatus"].dump().c_str() == "REGISTERED") {
      // ToDo Trigger NF heartbeats
    }
  } catch (nlohmann::json::exception& e) {
    Logger::udm_nrf().info("NF registeration procedure failed");
  }
}
//---------------------------------------------------------------------------------------------
