/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef FILE_UDM_NRF_SEEN
#define FILE_UDM_NRF_SEEN

#include <map>
#include <thread>

#include "logger.hpp"
#include "udm_config.hpp"
#include "udm_event.hpp"
#include "udm_profile.hpp"

namespace oai::udm::app {

class udm_nrf {
 public:
  // timer_id_t timer_udm_heartbeat;

  udm_nrf(udm_event& ev);
  udm_nrf(udm_nrf const&) = delete;
  virtual ~udm_nrf();
  void operator=(udm_nrf const&) = delete;

  void generate_uuid();

  /*
   * Start event nf heartbeat procedure
   * @param [void]
   * @return void
   */
  void start_event_nf_heartbeat(std::string& remoteURI);

  /*
   * Trigger NF heartbeat procedure
   * @param [void]
   * @return void
   */
  void trigger_nf_heartbeat_procedure(uint64_t ms);

  /*
   * Start event nrf registration retry
   * @param [void]
   * @return void
   */
  void start_nrf_registration_retry();

  /*
   * Trigger NF registration procedure
   * @param [void]
   * @return void
   */
  void trigger_nrf_registration_retry_procedure(uint64_t ms);

  /*
   * Stop event nrf registration retry
   * @param [void]
   * @return void
   */
  void stop_nrf_registration_retry();

  /*
   * Generate a UDM profile for this instance
   * @param [void]
   * @return void
   */
  void generate_udm_profile();

  /*
   * Trigger NF instance registration to NRF
   * @param [void]
   * @return void
   */
  void register_to_nrf();

  /*
   * Trigger NF instance deregistration to NRF
   * @param [void]
   * @return void
   */
  void deregister_to_nrf();

  /*
   * Discover an NF endpoint via NRF, selecting the NFService whose serviceName
   * matches @service_name (e.g. "namf-evts", "nsmf-event-exposure").
   * @param [const std::string&] target_nf_type: e.g. "AMF", "SMF"
   * @param [const std::string&] service_name: target service name
   * @param [std::string&] endpoint: resolved "scheme://ipv4:port" on success
   * @return true on success, false otherwise
   */
  bool discover_nf(
      const std::string& target_nf_type, const std::string& service_name,
      std::string& endpoint);

 private:
  udm_event& m_event_sub;
  bs2::connection task_connection;
  bs2::connection retry_nrf_registration_task_connection;
  udm_profile udm_nf_profile;   // UDM profile
  std::string udm_instance_id;  // UDM instance id

  // Discovery cache keyed by "<nf_type>:<service_name>"
  std::map<std::string, std::string> m_discovery_cache;
  std::mutex m_discovery_mutex;
};
}  // namespace oai::udm::app
#endif /* FILE_UDM_NRF_SEEN */
