/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef FILE_UDM_APP_HPP_SEEN
#define FILE_UDM_APP_HPP_SEEN

#include <map>
#include <memory>
#include <shared_mutex>
#include <string>

#include "Amf3GppAccessRegistration.h"
#include "AuthEvent.h"
#include "AuthenticationInfoRequest.h"
#include "CreatedEeSubscription.h"
#include "DataRestorationNotification.h"
#include "EeMonitoringRevoked.h"
#include "EeSubscription.h"
#include "MonitoringReport.h"
#include "PatchItem.h"
#include "PlmnId.h"
#include "ProblemDetails.h"
#include "SdmSubscription.h"
#include "Snssai.h"
#include "udm.h"
#include "udm_event.hpp"
#include "uint_generator.hpp"

namespace oai::udm::app {

// Which NF is able to detect a given Nudm_EE event type: either the UDM
// itself, or a remote NF the subscription has to be relayed to.
enum class ee_event_detector_t {
  UDM_LOCAL,
  AMF_RELAY,
  SMF_RELAY,  // not relayed yet, the subscription is only accepted and stored
  SMS_GMSC,   // not supported, the subscription is only accepted and stored
  UNKNOWN
};

// Links a local Nudm_EE subscription to the subscription that the UDM created
// on a remote AMF for it, so that an unsubscribe, an update or a change of
// serving AMF can be applied to the remote subscription as well.
typedef struct relay_correlation_s {
  std::string remote_nf_type;           // "AMF"
  std::string remote_subscription_uri;  // Location returned by the remote NF
  std::string amf_instance_id;          // serving AMF at subscription time
} relay_correlation_t;

class udm_app {
 public:
  explicit udm_app(const std::string& config_file, udm_event& ev);
  udm_app(udm_app const&)        = delete;
  void operator=(udm_app const&) = delete;
  virtual ~udm_app();

  bool start();
  void stop();

  /*
   * Handle a request to generate the authentication data
   * @param [const std::string&] supiOrSuci: UE's SUPI/SUCI
   * @param [const oai::_3gpp::model::AuthenticationInfoRequest&]
   * authenticationInfoRequest: request's info
   * @param [nlohmann::json&] auth_info_response: Authentication response's info
   * @param [uint32_t&] code: response's code
   * @return void
   */
  void handle_generate_auth_data_request(
      const std::string& supiOrSuci,
      const oai::_3gpp::model::AuthenticationInfoRequest&
          authenticationInfoRequest,
      nlohmann::json& auth_info_response, uint32_t& code);

  /*
   * Handle a request to confirm the authentication data
   * @param [const std::string&] supi: UE's SUPI
   * @param [const oai::_3gpp::model::AuthEvent&] authEvent: Authentication
   * Event
   * @param [nlohmann::json&] confirm_response: Confirm response
   * @param [std::string&] location: location of the resource
   * @param [uint32_t&] code: response's code
   * @return void
   */
  void handle_confirm_auth(
      const std::string& supi, const oai::_3gpp::model::AuthEvent& authEvent,
      nlohmann::json& confirm_response, std::string& location, uint32_t& code);

  /*
   * Handle a request to delete an authentication data
   * @param [const std::string&] supi: UE's SUPI
   * @param [const std::string&] authEventId: Event ID
   * @param [const oai::_3gpp::model::AuthEvent&] authEvent: Authentication
   * Event
   * @param [nlohmann::json&] auth_response: Authentication response
   * @param [uint32_t&] code: response's code
   * @return void
   */
  void handle_delete_auth(
      const std::string& supi, const std::string& authEventId,
      const oai::_3gpp::model::AuthEvent& authEvent,
      nlohmann::json& auth_response, uint32_t& code);

  /*
   * Handle a request to get the Access and Mobility Subscription Data
   * @param [const std::string&] supi: UE's SUPI
   * @param [nlohmann::json&] response_data: response's info
   * @param [uint32_t&] code: response's code
   * @param [oai::_3gpp::model::PlmnId] PlmnId: PLMN ID
   * @return void
   */
  void handle_access_mobility_subscription_data_retrieval(
      const std::string& supi, nlohmann::json& response_data, uint32_t& code,
      oai::_3gpp::model::PlmnId PlmnId = {});

  /*
   * Handle a request to Create an AMF Registration for 3GPP Access info
   * @param [const std::string&] supi: UE's SUPI
   * @param [const oai::_3gpp::model::Amf3GppAccessRegistration&]
   * amf_3gpp_access_registration: Registration info
   * @param [nlohmann::json&] response_data: response's info
   * @param [uint32_t&] code: response's code
   * @param [oai::_3gpp::model::PlmnId] PlmnId: PLMN ID
   * @return void
   */
  void handle_amf_registration_for_3gpp_access(
      const std::string& ue_id,
      const oai::_3gpp::model::Amf3GppAccessRegistration&
          amf_3gpp_access_registration,
      nlohmann::json& response_data, uint32_t& code);

  /*
   * Handle a request to get the Session Management Subscription Data
   * @param [const std::string&] supi: UE's SUPI
   * @param [nlohmann::json&] response_data: response's info
   * @param [uint32_t&] code: response's code
   * @param [oai::_3gpp::model::Snssai] snssai: SNSSAI
   * @param [oai::_3gpp::model::PlmnId] PlmnId: PLMN ID
   * @param [std::string&] dnn: DNN
   * @return void
   */
  void handle_session_management_subscription_data_retrieval(
      const std::string& supi, nlohmann::json& response_data, uint32_t& code,
      const std::optional<oai::_3gpp::model::Snssai>& snssai,
      const std::optional<std::string>& dnn,
      const std::optional<oai::_3gpp::model::PlmnId>& plmn_id);

  /*
   * Handle a request to get the Slice Selection Subscription Data
   * @param [const std::string&] supi: UE's SUPI
   * @param [nlohmann::json&] response_data: response's info
   * @param [uint32_t&] code: response's code
   * @param [oai::_3gpp::model::PlmnId] PlmnId: PLMN ID
   * @return void
   */
  void handle_slice_selection_subscription_data_retrieval(
      const std::string& supi, nlohmann::json& response_data, uint32_t& code,
      std::string supported_features    = {},
      oai::_3gpp::model::PlmnId plmn_id = {});

  /*
   * Handle a request to get the SMF Selection Subscription Data
   * @param [const std::string&] supi: UE's SUPI
   * @param [nlohmann::json&] response_data: response's info
   * @param [uint32_t&] code: response's code
   * @param [std::string] supported_features: supported features
   * @param [oai::_3gpp::model::PlmnId] PlmnId: PLMN ID
   * @return void
   */
  void handle_smf_selection_subscription_data_retrieval(
      const std::string& supi, nlohmann::json& response_data, uint32_t& code,
      std::string supported_features    = {},
      oai::_3gpp::model::PlmnId plmn_id = {});

  /*
   * Handle a request to create a subscription
   * @param [const std::string&] supi: UE's SUPI
   * @param [const oai::_3gpp::model::SdmSubscription&] sdmSubscription:
   * Suscription info
   * @param [nlohmann::json&] response_data: response's info
   * @param [uint32_t&] code: response's code
   * @return void
   */
  void handle_subscription_creation(
      const std::string& supi,
      const oai::_3gpp::model::SdmSubscription& sdmSubscription,
      nlohmann::json& response_data, uint32_t& code);

  /*
   * Handle a request to create an event subscription
   * @param [const std::string&] supi: UE's SUPI
   * @param [const oai::_3gpp::model::EeSubscription&] eeSubscription:
   * suscription info
   * @param [const oai::_3gpp::model::CreatedEeSubscription&] createdSub:
   * created suscription info
   * @param [uint32_t&] code: response's code
   * @return subscription Id
   */
  evsub_id_t handle_create_ee_subscription(
      const std::string& ueIdentity,
      const oai::_3gpp::model::EeSubscription& eeSubscription,
      oai::_3gpp::model::CreatedEeSubscription& createdSub,
      oai::_3gpp::model::ProblemDetails& problemDetails, uint32_t& code);

  /*
   * Handle a request to delete an event subscription
   * @param [const std::string&] ueIdentity: UE's identity
   * @param [const std::string&] subscriptionId: subscription's Id
   * @param [oai::_3gpp::model::ProblemDetails&] problemDetails: problem
   * happened (if exist) when deleting the even
   * @param [uint32_t&] code: response's code
   * @return void
   */
  void handle_delete_ee_subscription(
      const std::string& ueIdentity, const std::string& subscriptionId,
      oai::_3gpp::model::ProblemDetails& problemDetails, uint32_t& code);

  /*
   * Handle a request to update an event subscription
   * @param [const std::string&] ueIdentity: UE's identity
   * @param [const std::string&] subscriptionId: subscription's Id
   * @param [const std::vector<oai::_3gpp::model::PatchItem>&] patchItem: list
   * of actions
   * @param [oai::_3gpp::model::ProblemDetails&] problemDetails: problem
   * happened (if exist) when executing the requests
   * @param [uint32_t&] code: response's code
   * @return void
   */
  void handle_update_ee_subscription(
      const std::string& ueIdentity, const std::string& subscriptionId,
      const std::vector<oai::_3gpp::model::PatchItem>& patchItem,
      oai::_3gpp::model::ProblemDetails& problemDetails, uint32_t& code);

  /*
   * Generate an unique ID for the new subscription
   * @return the generated ID
   */
  evsub_id_t generate_ev_subscription_id();

  /*
   * Add an Event Subscription to the list
   * @param [const evsub_id_t&] sub_id: Subscription ID
   * @param [std::string] ue_id: UE's identity
   * @param [std::shared_ptr<oai::_3gpp::model::CreatedEeSubscription>] ces: a
   * shared pointer stored information of the created subscription
   * @return void
   */
  void add_event_subscription(
      const evsub_id_t& sub_id, const std::string& ue_id,
      std::shared_ptr<oai::_3gpp::model::CreatedEeSubscription>& ces);

  /*
   * Delete an Event Subscription
   * @param [const std::string&] sub_id: Subscription ID
   * @param [std::string] ue_id: UE's identity
   * @return true if success, otherwise false
   */
  bool delete_event_subscription(
      const std::string& sub_id, const std::string& ue_id);

  /*
   * Update a new item for a subscription
   * @param [const std::string &] path: item name
   * @param [const std::string &] value: new value
   * @return true if success, otherwise false
   */
  bool replace_ee_subscription_item(
      const std::string& subscriptionId, const std::string& path,
      const std::string& value);

  /*
   * Add a new item for a subscription
   * @param [const std::string &] subscriptionId: subscription's Id
   * @param [const std::string &] path: item name
   * @param [const std::string &] value: new value
   * @return true if success, otherwise false
   */
  bool add_ee_subscription_item(
      const std::string& subscriptionId, const std::string& path,
      const std::string& value);

  /*
   * Remove an item for a subscription
   * @param [const std::string &] subscriptionId: subscription's Id
   * @param [const std::string &] path: item name
   * @return true if success, otherwise false
   */
  bool remove_ee_subscription_item(
      const std::string& subscriptionId, const std::string& path);

  /*
   * Handle Loss of Connectivity Event
   * @param [const std::string&] ue_id: UE's identity (e.g., SUPI)
   * @param [uint8_t] status: Connectivity status
   * @param [uint8_t] http_version: HTTP version
   * @return void
   */
  /*
   * Notify a consumer that an event occurred, by posting the corresponding
   * Monitoring Reports to its callback URI
   * @param [const std::string&] callback_uri: consumer's callbackReference
   * @param [const std::vector<oai::_3gpp::model::MonitoringReport>&] reports:
   * reports to be sent
   * @return void
   */
  void notify_event_occurrence(
      const std::string& callback_uri,
      const std::vector<oai::_3gpp::model::MonitoringReport>& reports);

  /*
   * Send a Monitoring Revocation (EeMonitoringRevoked) to tell the subscriber
   * that the monitoring is revoked, e.g. because the AF is no longer authorized
   * or the UE left the group. The notification is sent to the secondary
   * callback of the subscription. Note that the events that should trigger a
   * revocation are not detected yet, this is only the sender.
   * @param [const evsub_id_t&] sub_id: subscription whose secondCallbackRef
   * should be notified
   * @param [const oai::_3gpp::model::EeMonitoringRevoked&] revoked: revocation
   * info to be sent
   * @return void
   */
  void send_revocation(
      const evsub_id_t& sub_id,
      const oai::_3gpp::model::EeMonitoringRevoked& revoked);

  /*
   * Send a Data Restoration notification to tell the subscribers that
   * subscription data has been restored, e.g. after a data loss in the UDR. The
   * notification is sent to every EE subscription that provided a
   * dataRestorationCallbackUri. Note that the UDR data loss itself is not
   * detected yet, this is only the sender.
   * @param [const oai::_3gpp::model::DataRestorationNotification&]
   * notification: restoration info to be sent
   * @return void
   */
  void send_data_restoration(
      const oai::_3gpp::model::DataRestorationNotification& notification);

  void handle_ee_loss_of_connectivity(
      const std::string& ue_id, uint8_t status, uint8_t http_version);

  /*
   * Handle UE Reachability For Data Event
   * @param [const std::string&] ue_id: UE's identity (e.g., SUPI)
   * @param [uint8_t] status: UE Reachability For Data status
   * @param [uint8_t] http_version: HTTP version
   * @return void
   */
  void handle_ee_ue_reachability_for_data(
      const std::string& ue_id, uint8_t status, uint8_t http_version);

  /*
   * Increase the value of SQN with a value of 32
   * @param [const std::string&] c_sqn: Current value in form of string
   * @param [std::string&] n_sqn: New value in form of string
   * @return void
   */
  void increment_sqn(const std::string& c_sqn, std::string& n_sqn);

  /*
   * Set problem details to be returned to the request client
   * @param [uint16_t ] status: Status code
   * @param [uint16_t] cause: cause of the problem
   * @param [const std::detail&] detail: Description of the problem
   * @param [nlohmann::json& ] problem_details: problem details in json format
   * @return void
   */
  void set_problem_details(
      uint16_t status, uint16_t cause, const std::string& detail,
      nlohmann::json& problem_details);

  /*
   * Validate the format of SNN and get the corresponding PLMN ID if valid
   * @param [const std::string&] snn: Serving Network Name
   * @param [oai::_3gpp::model::PlmnId&] plmn_id: PLMN ID
   * @return true if SNN follows the regex specification otherwise return false
   */
  bool validate_snn(const std::string& snn, oai::_3gpp::model::PlmnId& plmn_id);

  /*
   * Get the UE's Home PLMN
   * @param [const std::string& ] supi: UE's SUPI
   * @param [std::optional<oai::_3gpp::model::PlmnId>&] plmn_id: PLMN Id
   * @return void
   */
  void get_hplmn_id(
      const std::string& supi,
      std::optional<oai::_3gpp::model::PlmnId>& plmn_id);

  /*
   * Get the PLMN ID from SNN and store in the DB
   * @param [const std::string& ] supi: UE's SUPI
   * @param [const oai::_3gpp::model::PlmnId&] plmn_id: PLMN ID
   * @return void
   */
  void store_plmn_id(
      const std::string& supi, const oai::_3gpp::model::PlmnId& plmn_id);

  /*
   * Find out which NF is able to detect a given Nudm_EE event type
   * @param [EventType_anyOf::eEventType_anyOf] ev: Nudm_EE event type
   * @return the NF in charge of the detection
   */
  ee_event_detector_t classify_event_detector(
      oai::_3gpp::model::EventType_anyOf::eEventType_anyOf ev) const;

  /*
   * Get the Namf_EventExposure event type matching a Nudm_EE event type
   * @param [EventType_anyOf::eEventType_anyOf] ev: Nudm_EE event type
   * @return the Namf_EventExposure event type, empty if the event is not
   * detected by the AMF
   */
  std::string namf_event_type_for(
      oai::_3gpp::model::EventType_anyOf::eEventType_anyOf ev) const;

  /*
   * Get the instance ID of the AMF currently serving a UE, by reading the AMF
   * registration back from the UDR
   * @param [const std::string&] ue_id: UE's identity (e.g., SUPI)
   * @return the AMF instance ID, empty if it could not be retrieved
   */
  std::string get_serving_amf_instance_id(const std::string& ue_id);

  /*
   * Relay an EE subscription to the serving AMF (Namf_EventExposure). The
   * consumer's callback and correlation ID are forwarded to the AMF, so that
   * the AMF notifies the consumer directly. On success, the created remote
   * subscription is stored to be able to update or delete it later on.
   * @param [const evsub_id_t&] sub_id: local subscription's ID
   * @param [const std::string&] ue_id: UE's identity (e.g., SUPI)
   * @return void
   */
  void relay_subscribe_to_amf(
      const evsub_id_t& sub_id, const std::string& ue_id);

  /*
   * Delete the subscription created on the remote AMF, if there is one
   * @param [const evsub_id_t&] sub_id: local subscription's ID
   * @return void
   */
  void relay_unsubscribe(const evsub_id_t& sub_id);

 private:
  oai::utils::uint_generator<uint32_t> evsub_id_generator;
  std::map<
      evsub_id_t, std::shared_ptr<oai::_3gpp::model::CreatedEeSubscription>>
      udm_event_subscriptions;
  std::map<std::string, std::vector<evsub_id_t>> udm_event_subscriptions_per_ue;
  // Subscription ID -> subscription created on the remote AMF for it (guarded
  // by the mutex above)
  std::map<evsub_id_t, relay_correlation_t> udm_event_relay_correlation;
  mutable std::shared_mutex m_mutex_udm_event_subscriptions;
  std::map<std::string, oai::_3gpp::model::PlmnId> hplmn;
  mutable std::shared_mutex m_mutex_hplmn;

  // for Event Handling
  udm_event& event_sub;
  bs2::connection loss_of_connectivity_connection;
  bs2::connection ue_reachability_for_data_connection;

  // UDM instance ID, used as nfId when relaying a subscription to another NF.
  // TODO: reuse the UUID registered to the NRF instead of generating a new one
  std::string m_udm_instance_id;
};
}  // namespace oai::udm::app
#include "udm_config.hpp"

#endif /* FILE_UDM_APP_HPP_SEEN */
