/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef FILE_UDM_CLIENT_SEEN
#define FILE_UDM_CLIENT_SEEN

#include "nf_service.hpp"
#include "udm_event.hpp"
#include "udm_profile.hpp"

namespace oai::udm::app {

class udm_client : public oai::sba::nf_service {
 public:
  explicit udm_client(udm_event& ev);
  udm_client(udm_client const&)     = delete;
  ~udm_client() override            = default;
  void operator=(udm_client const&) = delete;

  void generate_udm_profile();
  bool register_to_nrf();
  bool deregister_to_nrf();
  bool discover_nf(
      const std::string& target_nf_type, const std::string& service_name,
      std::string& endpoint);

 protected:
  bool nrf_registration_enabled() const override;
  uint64_t nrf_registration_retry_seconds() const override;
  void on_registration_outcome(
      bool success, const oai::sba::sbi_http_response& resp) override;

 private:
  udm_profile udm_nf_profile;
};

}  // namespace oai::udm::app

#endif /* FILE_UDM_CLIENT_SEEN */
