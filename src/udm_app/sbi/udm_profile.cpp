/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "udm_profile.hpp"

#include "logger.hpp"
#include "string.hpp"

using namespace oai::udm::app;

udm_profile::udm_profile() : oai::sba::nf_profile(), udm_info() {
  nf_type = "NF_TYPE_UNKNOWN";
}

udm_profile::udm_profile(const std::string& id)
    : oai::sba::nf_profile(id), udm_info() {
  nf_type = "NF_TYPE_UNKNOWN";
}

udm_profile::udm_profile(const udm_profile& other)
    : oai::sba::nf_profile(), udm_info() {
  *this = other;
}

udm_profile& udm_profile::operator=(const udm_profile& other) {
  if (this == &other) return *this;
  nf_instance_id   = other.nf_instance_id;
  nf_instance_name = other.nf_instance_name;
  nf_type          = other.nf_type;
  nf_status        = other.nf_status;
  heartBeat_timer  = other.heartBeat_timer;
  plmn_list        = other.plmn_list;
  snssais          = other.snssais;
  fqdn             = other.fqdn;
  ipv4_addresses   = other.ipv4_addresses;
  ipv6_addresses   = other.ipv6_addresses;
  priority         = other.priority;
  capacity         = other.capacity;
  json_data        = other.json_data;
  nf_services      = other.nf_services;
  custom_info      = other.custom_info;
  is_updated       = other.is_updated;
  udm_info         = other.udm_info;
  return *this;
}

std::string udm_profile::get_nf_fqdn() const {
  return get_fqdn();
}

void udm_profile::set_nf_fqdn(const std::string& value) {
  set_fqdn(value);
}

void udm_profile::set_udm_info(const oai::common::sbi::udm_info_t& info) {
  udm_info = info;
}

void udm_profile::get_udm_info(oai::common::sbi::udm_info_t& info) const {
  info = udm_info;
}

void udm_profile::display() {
  oai::sba::nf_profile::display();
  Logger::udm_app().debug("\tUDM Info");
  Logger::udm_app().debug("\t\tGroupId: %s", udm_info.groupid.c_str());
  for (const auto& supi : udm_info.supi_ranges) {
    Logger::udm_app().debug(
        "\t\tSupiRanges: Start - %s, End - %s, Pattern - %s",
        supi.supi_range.start.c_str(), supi.supi_range.end.c_str(),
        supi.supi_range.pattern.c_str());
  }
  for (const auto& indicator : udm_info.routing_indicator) {
    Logger::udm_app().debug("\t\tRouting Indicators: %s", indicator.c_str());
  }
}

void udm_profile::to_json(nlohmann::json& data) const {
  oai::sba::nf_profile::to_json(data);
  data.erase("json_data");
  if (snssais.empty()) data["sNssais"] = nlohmann::json::array();
  data["fqdn"] = fqdn;

  data["udmInfo"]["groupId"]                        = udm_info.groupid;
  data["udmInfo"]["supiRanges"]                     = nlohmann::json::array();
  data["udmInfo"]["gpsiRanges"]                     = nlohmann::json::array();
  data["udmInfo"]["externalGroupIdentifiersRanges"] = nlohmann::json::array();
  data["udmInfo"]["routingIndicators"]              = nlohmann::json::array();
  data["udmInfo"]["internalGroupIdentifiersRanges"] = nlohmann::json::array();
  for (const auto& item : udm_info.supi_ranges) {
    data["udmInfo"]["supiRanges"].push_back(
        {{"start", item.supi_range.start},
         {"end", item.supi_range.end},
         {"pattern", item.supi_range.pattern}});
  }
  for (const auto& item : udm_info.gpsi_ranges) {
    data["udmInfo"]["gpsiRanges"].push_back(
        {{"start", item.identity_range.start},
         {"end", item.identity_range.end},
         {"pattern", item.identity_range.pattern}});
  }
  for (const auto& item : udm_info.ext_grp_id_ranges) {
    data["udmInfo"]["externalGroupIdentifiersRanges"].push_back(
        {{"start", item.identity_range.start},
         {"end", item.identity_range.end},
         {"pattern", item.identity_range.pattern}});
  }
  for (const auto& item : udm_info.routing_indicator) {
    data["udmInfo"]["routingIndicators"].push_back(item);
  }
  for (const auto& item : udm_info.int_grp_id_ranges) {
    data["udmInfo"]["internalGroupIdentifiersRanges"].push_back(
        {{"start", item.int_grpid_range.start},
         {"end", item.int_grpid_range.end},
         {"pattern", item.int_grpid_range.pattern}});
  }
  Logger::udm_app().debug("UDM profile to JSON:\n %s", data.dump().c_str());
}

void udm_profile::from_json(const nlohmann::json& data) {
  snssais.clear();
  ipv4_addresses.clear();
  udm_info = {};
  if (data.contains("nfInstanceId")) nf_instance_id = data["nfInstanceId"];
  if (data.contains("nfInstanceName"))
    nf_instance_name = data["nfInstanceName"];
  if (data.contains("nfType")) nf_type = data["nfType"];
  if (data.contains("nfStatus")) nf_status = data["nfStatus"];
  if (data.contains("heartBeatTimer")) heartBeat_timer = data["heartBeatTimer"];
  if (data.contains("fqdn")) fqdn = data["fqdn"];
  if (data.contains("sNssais")) {
    for (const auto& item : data["sNssais"])
      snssais.push_back({item["sst"], item["sd"]});
  }
  if (data.contains("ipv4Addresses")) {
    for (const auto& item : data["ipv4Addresses"]) {
      struct in_addr address = {};
      auto value             = item.get<std::string>();
      if (inet_pton(AF_INET, oai::utils::trim(value).c_str(), &address) == 1)
        ipv4_addresses.push_back(address);
      else
        Logger::udm_app().warn(
            "Address conversion: Bad value %s", value.c_str());
    }
  }
  if (data.contains("priority")) priority = data["priority"];
  if (data.contains("capacity")) capacity = data["capacity"];
  if (data.contains("udmInfo")) {
    const auto& info = data["udmInfo"];
    if (info.contains("groupId")) udm_info.groupid = info["groupId"];
    if (info.contains("supiRanges"))
      for (const auto& value : info["supiRanges"]) {
        oai::common::sbi::supi_range_info_item_t item = {};
        item.supi_range.start                         = value["start"];
        item.supi_range.end                           = value["end"];
        item.supi_range.pattern                       = value["pattern"];
        udm_info.supi_ranges.push_back(item);
      }
    if (info.contains("gpsiRanges"))
      for (const auto& value : info["gpsiRanges"]) {
        oai::common::sbi::identity_range_info_item_t item = {};
        item.identity_range.start                         = value["start"];
        item.identity_range.end                           = value["end"];
        item.identity_range.pattern                       = value["pattern"];
        udm_info.gpsi_ranges.push_back(item);
      }
    if (info.contains("externalGroupIdentifiersRanges"))
      for (const auto& value : info["externalGroupIdentifiersRanges"]) {
        oai::common::sbi::identity_range_info_item_t item = {};
        item.identity_range.start                         = value["start"];
        item.identity_range.end                           = value["end"];
        item.identity_range.pattern                       = value["pattern"];
        udm_info.ext_grp_id_ranges.push_back(item);
      }
    if (info.contains("routingIndicators"))
      for (const auto& value : info["routingIndicators"])
        udm_info.routing_indicator.push_back(value);
    if (info.contains("internalGroupIdentifiersRanges"))
      for (const auto& value : info["internalGroupIdentifiersRanges"]) {
        oai::common::sbi::internal_grpid_range_info_item_t item = {};
        item.int_grpid_range.start   = value["start"];
        item.int_grpid_range.end     = value["end"];
        item.int_grpid_range.pattern = value["pattern"];
        udm_info.int_grp_id_ranges.push_back(item);
      }
  }
  display();
}

void udm_profile::handle_heartbeart_timeout(uint64_t ms) {
  Logger::udm_app().info(
      "Handle heartbeart timeout profile %s, time %d", nf_instance_id.c_str(),
      ms);
  set_nf_status("SUSPENDED");
}
