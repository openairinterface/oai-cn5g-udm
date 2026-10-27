/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef FILE_UDM_EVENT_HPP_SEEN
#define FILE_UDM_EVENT_HPP_SEEN

#include <boost/signals2.hpp>
namespace bs2 = boost::signals2;

#include "nf_event.hpp"
#include "udm.h"

namespace oai::udm::app {

typedef bs2::signal_type<
    void(uint64_t), bs2::keywords::mutex_type<bs2::dummy_mutex>>::type
    task_sig_t;

// Signal for Loss of Connectivity
// SUPI, Connectivity status, HTTP version
typedef bs2::signal_type<
    void(std::string, uint8_t, uint8_t),
    bs2::keywords::mutex_type<bs2::dummy_mutex>>::type
    loss_of_connectivity_sig_t;

// Signal for UE Reachability for Data
// SUPI, Reachability status, HTTP version
typedef bs2::signal_type<
    void(std::string, uint8_t, uint8_t),
    bs2::keywords::mutex_type<bs2::dummy_mutex>>::type
    ue_reachability_for_data_sig_t;

// UE_REACHABILITY_FOR_SMS
// LOCATION_REPORTING
// CHANGE_OF_SUPI_PEI_ASSOCIATION
// ROAMING_STATUS
// COMMUNICATION_FAILURE
// AVAILABILITY_AFTER_DNN_FAILURE
// CN_TYPE_CHANGE

}  // namespace oai::udm::app

namespace oai::udm::app {
class udm_event : public oai::sba::nf_event {
 public:
  udm_event() {};
  udm_event(udm_event const&)      = delete;
  void operator=(udm_event const&) = delete;

  static udm_event& get_instance() {
    static udm_event instance;
    return instance;
  }

  // class register/handle event
  friend class udm_app;
  friend class udm_client;

  //------------------------------------------------------------------------------
  /*
   * Subscribe to the task tick event
   * @param [const task_sig_t::slot_type &] sig
   * @param [uint64_t] period: interval between two events
   * @param [uint64_t] start:
   * @return void
   */
  bs2::connection subscribe_task_nf_heartbeat(
      const oai::sba::task_sig_t::slot_type& sig, uint64_t period,
      uint64_t start = 0) override;

  void notify_task_tick(uint64_t tick) override;
  //------------------------------------------------------------------------------
  /*
   * Subscribe to UE Loss of Connectivity Status signal
   * @param [const loss_of_connectivity_sig_t::slot_type&] sig: slot_type
   * parameter
   * @return boost::signals2::connection: the connection between the signal and
   * the slot
   */
  bs2::connection subscribe_loss_of_connectivity(
      const loss_of_connectivity_sig_t::slot_type& sig);

  /*
   * Subscribe to UE Reachability for Data signal
   * @param [const ue_reachability_for_data_sig_t::slot_type&] sig: slot_type
   * parameter
   * @return boost::signals2::connection: the connection between the signal and
   * the slot
   */
  bs2::connection subscribe_ue_reachability_for_data(
      const ue_reachability_for_data_sig_t::slot_type& sig);

 private:
  task_sig_t task_tick;

  loss_of_connectivity_sig_t
      loss_of_connectivity;  // Signal for Loss of Connectivity Report
  ue_reachability_for_data_sig_t
      ue_reachability_for_data;  // Signal for UE Reachability for Data Report
};
}  // namespace oai::udm::app
#endif
