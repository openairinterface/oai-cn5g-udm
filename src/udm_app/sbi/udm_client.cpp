/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "udm_client.hpp"

#include "logger.hpp"
#include "udm.h"
#include "udm_config.hpp"

using namespace oai::udm::app;
using namespace oai::udm::config;

extern udm_config udm_cfg;
extern std::shared_ptr<oai::sba::sbi_http_client> http_client_inst;

namespace {
std::shared_ptr<oai::sba::nf_event> borrowed_event(udm_event& event) {
  return std::shared_ptr<oai::sba::nf_event>(
      &event, [](oai::sba::nf_event*) {});
}
}  // namespace

udm_client::udm_client(udm_event& event)
    : oai::sba::nf_service(borrowed_event(event), http_client_inst) {
  generate_udm_profile();
}

void udm_client::generate_udm_profile() {
  udm_nf_profile.set_nf_instance_id(nf_instance_id);
  udm_nf_profile.set_nf_instance_name("OAI-UDM");
  udm_nf_profile.set_nf_fqdn(udm_cfg.udm_name);
  udm_nf_profile.set_nf_type("UDM");
  udm_nf_profile.set_nf_status("REGISTERED");
  udm_nf_profile.set_nf_heartBeat_timer(HEART_BEAT_TIMER);
  udm_nf_profile.set_nf_priority(1);
  udm_nf_profile.set_nf_capacity(100);
  udm_nf_profile.add_nf_ipv4_addresses(udm_cfg.sbi.addr4);

  oai::common::sbi::nf_service_t ee_service         = {};
  ee_service.service_instance_id                    = "nudm-ee-1";
  ee_service.service_name                           = "nudm-ee";
  oai::common::sbi::nf_service_version_t ee_version = {};
  ee_version.api_version_in_uri = udm_cfg.sbi.api_version.value_or("v1");
  ee_version.api_full_version   = "1.2.3";
  ee_service.versions.push_back(ee_version);
  ee_service.scheme                           = "http";
  ee_service.nf_service_status                = "REGISTERED";
  oai::common::sbi::ip_endpoint_t ee_endpoint = {};
  ee_endpoint.ipv4_address                    = udm_cfg.sbi.addr4;
  ee_endpoint.transport                       = "TCP";
  ee_endpoint.port                            = udm_cfg.sbi.port;
  ee_service.ip_endpoints.push_back(ee_endpoint);
  udm_nf_profile.add_nf_service(ee_service);

  udm_nf_profile.display();
}

bool udm_client::register_to_nrf() {
  nlohmann::json profile = {};
  udm_nf_profile.to_json(profile);
  return oai::sba::nf_service::register_to_nrf(udm_cfg.nrf_addr, profile);
}

bool udm_client::deregister_to_nrf() {
  return oai::sba::nf_service::deregister_to_nrf();
}

bool udm_client::discover_nf(
    const std::string& target_nf_type, const std::string& service_name,
    std::string& endpoint) {
  return oai::sba::nf_service::discover_nf(
      udm_cfg.nrf_addr, "UDM", target_nf_type, service_name, endpoint);
}

bool udm_client::nrf_registration_enabled() const {
  return udm_cfg.register_nrf;
}

uint64_t udm_client::nrf_registration_retry_seconds() const {
  return NRF_REGISTRATION_RETRY_TIMER;
}

void udm_client::on_registration_outcome(
    bool success, const oai::sba::sbi_http_response& resp) {
  if (success) {
    Logger::udm_nrf().info(
        "NF registration procedure successful (status %d)", resp.status_code);
    start_event_nf_heartbeat(HEART_BEAT_TIMER);
    stop_nrf_registration_retry();
  } else {
    Logger::udm_nrf().info("NF registration procedure failed, try again");
    start_nrf_registration_retry();
  }
}
