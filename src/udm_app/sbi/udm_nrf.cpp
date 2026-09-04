/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "udm_nrf.hpp"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>

#include "3gpp_29.500.h"
#include "http_client.hpp"
#include "logger.hpp"
#include "sbi_helper.hpp"
#include "udm.h"
#include "udm_app.hpp"
#include "udm_profile.hpp"

using namespace oai::udm::app;
using namespace oai::udm::config;
using namespace oai::_3gpp::model;
using namespace boost::placeholders;

extern udm_config udm_cfg;
extern std::shared_ptr<oai::http::http_client> http_client_inst;

//------------------------------------------------------------------------------
udm_nrf::udm_nrf(udm_event& ev) : m_event_sub(ev) {
  // generate UUID
  udm_instance_id = to_string(boost::uuids::random_generator()());
  // Generate NF Profile
  generate_udm_profile();
}

//------------------------------------------------------------------------------
udm_nrf::~udm_nrf() {
  if (task_connection.connected()) task_connection.disconnect();
  if (retry_nrf_registration_task_connection.connected())
    retry_nrf_registration_task_connection.disconnect();
}
//---------------------------------------------------------------------------------------------
void udm_nrf::generate_udm_profile() {
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

  // Advertise the Nudm_EE (Event Exposure) service so consumers can discover it
  // via NRF (the profile carried no nfServices before).
  nf_service_t ee_service        = {};
  ee_service.service_instance_id = "nudm-ee-1";
  ee_service.service_name        = "nudm-ee";
  ee_service.api_version_in_uri  = udm_cfg.sbi.api_version.value_or("v1");
  ee_service.api_full_version    = "1.2.3";  // TS 29.503 Rel-17 Nudm_EE
  ee_service.scheme              = "http";
  ee_service.nf_service_status   = "REGISTERED";
  ee_service.ipv4_address        = inet_ntoa(udm_cfg.sbi.addr4);
  ee_service.port                = udm_cfg.sbi.port;
  udm_nf_profile.add_nf_service(ee_service);

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
  // TODO: Disable UDM Info item temporarily, should get the values from the
  // configuration file udm_nf_profile.set_udm_info(udm_info_item);
  // ToDo:- Add remaining fields
  // identity_range_udm_info_item_t ext_grp_id_ranges;
  // internal_grpid_range_udm_info_item_t int_grp_id_ranges;
  // UDM info item end

  udm_nf_profile.display();
}
//---------------------------------------------------------------------------------------------
void udm_nrf::register_to_nrf() {
  nlohmann::json response_data = {};
  std::string nrf_uri          = {};

  nlohmann::json json_data = {};
  udm_nf_profile.to_json(json_data);

  sbi_helper::get_nrf_nf_instance_uri(
      udm_cfg.nrf_addr, udm_instance_id, nrf_uri);
  Logger::udm_nrf().info(
      "Sending NF registration request to NRF, NRF's URI: %s", nrf_uri);

  bool registration_success = false;

  oai::http::request http_request =
      http_client_inst->prepare_json_request(nrf_uri, json_data.dump());
  auto http_response = http_client_inst->send_http_request(
      oai::common::sbi::method_e::PUT, http_request);

  if ((http_response.status_code == oai::common::sbi::http_status_code::OK) or
      (http_response.status_code ==
       oai::common::sbi::http_status_code::CREATED)) {
    try {
      response_data = nlohmann::json::parse(http_response.body);
      // TODO: use Heart-beart timer interval returned from NRF
      if (response_data.find("nfStatus") != response_data.end()) {
        std::string status = response_data["nfStatus"].get<std::string>();
        if (status.compare("REGISTERED") == 0) {
          registration_success = true;
          start_event_nf_heartbeat(nrf_uri);
          stop_nrf_registration_retry();
        }
      }
    } catch (nlohmann::json::exception& e) {
      Logger::udm_nrf().info("NF Registration procedure failed, try again ...");
    }
  } else {
    Logger::udm_nrf().info("Could not get response from NRF, try again ...");
  }

  if (!registration_success) {
    start_nrf_registration_retry();
  }
}

//---------------------------------------------------------------------------------------------
void udm_nrf::deregister_to_nrf() {
  nlohmann::json response_data = {};
  std::string nrf_uri          = {};

  sbi_helper::get_nrf_nf_instance_uri(
      udm_cfg.nrf_addr, udm_instance_id, nrf_uri);
  Logger::udm_nrf().info("Sending NF Deregistration request");

  oai::http::request http_request =
      http_client_inst->prepare_json_request(nrf_uri);
  auto http_response = http_client_inst->send_http_request(
      oai::common::sbi::method_e::DELETE, http_request);

  if (http_response.status_code ==
      oai::common::sbi::http_status_code::NO_CONTENT) {
    Logger::udm_nrf().info("NF Deregistration procedure successful");
    // TODO:
  } else {
    Logger::udm_nrf().info("NF Deregistration procedure failed");
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
  Logger::udm_nrf().info("Sending NF heartbeat request");

  nlohmann::json json_data = nlohmann::json::array();
  for (auto i : patch_items) {
    nlohmann::json item = {};
    to_json(item, i);
    json_data.push_back(item);
  }

  std::string nrf_uri = {};
  sbi_helper::get_nrf_nf_instance_uri(
      udm_cfg.nrf_addr, udm_instance_id, nrf_uri);

  oai::http::request http_request =
      http_client_inst->prepare_json_request(nrf_uri, json_data.dump());
  auto http_response = http_client_inst->send_http_request(
      oai::common::sbi::method_e::PATCH, http_request);

  if ((http_response.status_code == oai::common::sbi::http_status_code::OK) or
      (http_response.status_code ==
       oai::common::sbi::http_status_code::NO_CONTENT)) {
    // TODO: process the response
  } else {
    Logger::udm_nrf().info(
        "NF Heartbeat procedure failed, try to register again");
    if (task_connection.connected()) task_connection.disconnect();
    register_to_nrf();
  }
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

//------------------------------------------------------------------------------
bool udm_nrf::discover_nf(
    const std::string& target_nf_type, const std::string& service_name,
    std::string& endpoint) {
  const std::string cache_key = target_nf_type + ":" + service_name;
  {
    std::unique_lock<std::mutex> lock(m_discovery_mutex);
    auto it = m_discovery_cache.find(cache_key);
    if (it != m_discovery_cache.end()) {
      endpoint = it->second;
      return true;
    }
  }

  // Build the NRF SearchNFInstances URI and add discovery query parameters.
  std::string uri = {};
  oai::common::sbi::sbi_helper::get_nrf_disc_search_nf_instances_uri(
      udm_cfg.nrf_addr, uri);
  uri += "?target-nf-type=" + target_nf_type + "&requester-nf-type=UDM";

  request req   = http_client::prepare_json_request(uri);
  response resp = http_client_inst->send_http_request(method_e::GET, req);
  if (resp.status_code != oai::common::sbi::http_status_code::OK) {
    Logger::udm_nrf().warn(
        "NRF discovery for %s failed (HTTP %d)", target_nf_type.c_str(),
        resp.status_code);
    return false;
  }

  nlohmann::json search_result = resp.get_json();
  if (!search_result.contains("nfInstances") ||
      !search_result["nfInstances"].is_array()) {
    Logger::udm_nrf().warn("NRF discovery: no nfInstances in SearchResult");
    return false;
  }

  for (const auto& nf_instance : search_result["nfInstances"]) {
    // Prefer the matching service's ipEndPoints; fall back to instance-level
    // ipv4Addresses if the desired service is not present.
    if (nf_instance.contains("nfServices") &&
        nf_instance["nfServices"].is_array()) {
      for (const auto& svc : nf_instance["nfServices"]) {
        if (svc.value("serviceName", std::string{}) != service_name) continue;
        std::string svc_scheme = svc.value("scheme", std::string{"http"});
        if (svc.contains("ipEndPoints") && svc["ipEndPoints"].is_array() &&
            !svc["ipEndPoints"].empty()) {
          const auto& ep = svc["ipEndPoints"][0];
          std::string ip = ep.value("ipv4Address", std::string{});
          int port       = ep.value("port", 80);
          if (!ip.empty()) {
            endpoint = svc_scheme + "://" + ip + ":" + std::to_string(port);
            std::unique_lock<std::mutex> lock(m_discovery_mutex);
            m_discovery_cache[cache_key] = endpoint;
            return true;
          }
        }
      }
    }
    // Fallback: instance-level ipv4Addresses
    if (nf_instance.contains("ipv4Addresses") &&
        nf_instance["ipv4Addresses"].is_array() &&
        !nf_instance["ipv4Addresses"].empty()) {
      std::string ip = nf_instance["ipv4Addresses"][0].get<std::string>();
      if (!ip.empty()) {
        endpoint = "http://" + ip;
        Logger::udm_nrf().warn(
            "NRF discovery: service %s not found for %s, using instance "
            "address %s",
            service_name.c_str(), target_nf_type.c_str(), endpoint.c_str());
        std::unique_lock<std::mutex> lock(m_discovery_mutex);
        m_discovery_cache[cache_key] = endpoint;
        return true;
      }
    }
  }

  Logger::udm_nrf().warn(
      "NRF discovery: no usable endpoint for %s/%s", target_nf_type.c_str(),
      service_name.c_str());
  return false;
}
