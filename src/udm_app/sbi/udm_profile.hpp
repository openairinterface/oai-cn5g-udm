/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef FILE_UDM_PROFILE_HPP_SEEN
#define FILE_UDM_PROFILE_HPP_SEEN

#include <nlohmann/json.hpp>

#include "nf_profile.hpp"
#include "udm.h"

namespace oai::udm::app {

class udm_profile : public oai::sba::nf_profile {
 public:
  udm_profile();
  explicit udm_profile(const std::string& id);
  udm_profile(const udm_profile& other);
  udm_profile& operator=(const udm_profile& other);
  ~udm_profile() override = default;

  // Compatibility aliases retained for existing UDM call sites.
  std::string get_nf_fqdn() const;
  void set_nf_fqdn(const std::string& fqdn);

  void set_udm_info(const oai::common::sbi::udm_info_t& info);
  void get_udm_info(oai::common::sbi::udm_info_t& info) const;

  void display() override;
  void to_json(nlohmann::json& data) const override;
  void from_json(const nlohmann::json& data);

  void handle_heartbeart_timeout(uint64_t ms);

 private:
  oai::common::sbi::udm_info_t udm_info;
};

}  // namespace oai::udm::app

#endif
