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
#include "sbi_helper.hpp"

using namespace oai::udm::app;
using namespace oai::udm::config;
using namespace oai::model::common;
using namespace boost::placeholders;

extern udm_config udm_cfg;
extern udm_nrf* udm_nrf_inst;
udm_client* udm_client_instance = nullptr;

//------------------------------------------------------------------------------
udm_nrf::udm_nrf(udm_event& ev) : m_event_sub(ev) {}

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
  udm_info_item.routing_indicator.push_back("0210");
  udm_info_item.routing_indicator.push_back("9876");
  supi_range_info_item_t supi_ranges;
  supi_ranges.supi_range.start   = "208950000000031";
  supi_ranges.supi_range.pattern = "^imsi-20895[31-131]{10}$";
  supi_ranges.supi_range.end     = "208950000000131";
  udm_info_item.supi_ranges.push_back(supi_ranges);
  identity_range_info_item_t gpsi_ranges;
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
  udm_instance_id              = to_string(boost::uuids::random_generator()());
  nlohmann::json response_data = {};

  // Generate NF Profile
  udm_profile udm_nf_profile;
  generate_udm_profile(udm_nf_profile, udm_instance_id);

  // Send NF registration request
  std::string nrf_api_root = {};
  std::string response_str = {};
  long response_code       = 0;
  std::string remote_uri   = {};
  sbi_helper::get_nrf_nf_instance_uri(
      udm_cfg.nrf_addr, udm_instance_id, remote_uri);
  nlohmann::json json_data = {};
  udm_nf_profile.to_json(json_data);

  Logger::udm_nrf().info(
      "Sending NF registration request to NRF, NRF's URI: %s", remote_uri);

  bool registration_success = false;
  bool registration_result  = false;
  int num_retries           = 0;

  while (num_retries < kNumberOfNfRegisterRetries) {
    num_retries++;
    if (!udm_client::get_instance().send_request(
            remote_uri, http_method_e::PUT, json_data.dump().c_str(),
            response_str, response_code)) {
      sleep(kTimeIntervalBetweenNfRegisterRetries * pow(2, num_retries - 1));
      Logger::udm_nrf().debug("NF Register Retry %d ...", num_retries);
      continue;
    } else {
      registration_result = true;
      break;
    }
  }

  // Process the result if available
  if (registration_result) {
    try {
      response_data = nlohmann::json::parse(response_str);
      // TODO: use Heart-beart timer interval returned from NRF
      if (response_data.find("nfStatus") != response_data.end()) {
        std::string status = response_data["nfStatus"].get<std::string>();
        if (status.compare("REGISTERED") == 0) {
          registration_success = true;
          start_event_nf_heartbeat(remote_uri);
          stop_nrf_registration_retry();
        }
      }
    } catch (nlohmann::json::exception& e) {
      Logger::udm_nrf().info("NF Registration procedure failed, try again ...");
    }
  } else {
    Logger::udm_nrf().info(
        "NF Registration procedure failed after %d retries, try again ...",
        num_retries);
    // TODO:
  }
  if (!registration_success) {
    start_nrf_registration_retry();
  }
}

//---------------------------------------------------------------------------------------------
void udm_nrf::deregister_to_nrf() {
  nlohmann::json response_data = {};

  // Send NF Registration request
  std::string response_str = {};
  long response_code       = {0};

  std::string nrf_uri = {};
  sbi_helper::get_nrf_nf_instance_uri(
      udm_cfg.nrf_addr, udm_instance_id, nrf_uri);

  Logger::udm_nrf().info("Sending NF Deregistration request");

  bool registration_result = false;
  int num_retries          = 0;
  while (num_retries < kNumberOfNfDeregisterRetries) {
    num_retries++;
    if (!udm_client::get_instance().send_request(
            nrf_uri, http_method_e::DELETE, "", response_str, response_code)) {
      sleep(kTimeIntervalBetweenNfDeregisterRetries * pow(2, num_retries - 1));
      Logger::udm_app().debug("NF Deregister Retry %d ...", num_retries);
      continue;
    } else {
      registration_result = true;
      break;
    }
  }

  if (registration_result) {
    // TODO:
  } else {
    Logger::udm_nrf().info(
        "NF Deregistration procedure failed after %d retries", num_retries);
    // TODO:
  }
}

//---------------------------------------------------------------------------------------------
void udm_nrf::start_event_nf_heartbeat(std::string& remoteURI) {
  // get current time
  uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
  struct itimerspec its;
  its.it_value.tv_sec  = HEART_BEAT_TIMER;  // seconds
  its.it_value.tv_nsec = 0;                 // 100 * 1000 * 1000; //100ms
  const uint64_t interval =
      its.it_value.tv_sec * 1000 +
      its.it_value.tv_nsec / 1000000;  // convert sec, nsec to msec

  task_connection = m_event_sub.subscribe_task_nf_heartbeat(
      boost::bind(&udm_nrf::trigger_nf_heartbeat_procedure, this, _1), interval,
      ms + interval);
}

//---------------------------------------------------------------------------------------------
void udm_nrf::trigger_nf_heartbeat_procedure(uint64_t ms) {
  _unused(ms);
  PatchItem patch_item = {};
  std::vector<PatchItem> patch_items;
  //{"op":"replace","path":"/nfStatus", "value": "REGISTERED"}
  PatchOperation op;
  op.setEnumValue(PatchOperation_anyOf::ePatchOperation_anyOf::REPLACE);
  patch_item.setOp(op);
  patch_item.setPath("/nfStatus");
  patch_item.setValue("REGISTERED");
  patch_items.push_back(patch_item);
  Logger::udm_app().info("Sending NF heartbeat request");

  std::string response = {};
  long response_code   = 0;

  nlohmann::json json_data = nlohmann::json::array();
  for (auto i : patch_items) {
    nlohmann::json item = {};
    to_json(item, i);
    json_data.push_back(item);
  }

  std::string nrf_uri = {};
  sbi_helper::get_nrf_nf_instance_uri(
      udm_cfg.nrf_addr, udm_instance_id, nrf_uri);

  udm_client::get_instance().send_request(
      nrf_uri, http_method_e::PATCH, json_data.dump().c_str(), response,
      response_code);
  if (!response.empty()) task_connection.disconnect();
}

//---------------------------------------------------------------------------------------------
void udm_nrf::start_nrf_registration_retry() {
  if (!retry_nrf_registration_task_connection.connected()) {
    // get current time
    uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
    const uint64_t interval =
        NRF_REGISTRATION_RETRY_TIMER * 1000;  // convert sec to msec

    Logger::udm_nrf().debug("Start NRF registration retry task");
    retry_nrf_registration_task_connection =
        m_event_sub.subscribe_task_nf_heartbeat(
            boost::bind(
                &udm_nrf::trigger_nrf_registration_retry_procedure, this, _1),
            interval, ms + interval);
  }
}

//---------------------------------------------------------------------------------------------
void udm_nrf::trigger_nrf_registration_retry_procedure(uint64_t ms) {
  _unused(ms);
  register_to_nrf();
}

//---------------------------------------------------------------------------------------------
void udm_nrf::stop_nrf_registration_retry() {
  // get current time
  uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();
  if (retry_nrf_registration_task_connection.connected()) {
    Logger::udm_nrf().debug("Stop NRF registration retry task");
    retry_nrf_registration_task_connection.disconnect();
  }
}
