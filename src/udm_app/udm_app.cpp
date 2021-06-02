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

/*! \file udm_app.cpp
 \brief
 \author  Tien-Thinh NGUYEN
 \company Eurecom
 \date 2020
 \email: Tien-Thinh.Nguyen@eurecom.fr
 */

#include "udm_app.hpp"

#include <unistd.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>

#include "logger.hpp"
#include "udm_client.hpp"
#include "udm_config.hpp"
#include "curl.hpp"
#include "ProblemDetails.h"
#include "conversions.hpp"
#include "authentication_algorithms_with_5gaka.hpp"
#include "SequenceNumber.h"
#include "PatchItem.h"
#include "comUt.hpp"
#include "sha256.hpp"

using namespace oai::udm::app;
using namespace oai::udm::model;
using namespace std::chrono;
using namespace config;

extern udm_app* udm_app_inst;
extern udm_config udm_cfg;
udm_client* udm_client_inst = nullptr;

//------------------------------------------------------------------------------
udm_app::udm_app(const std::string& config_file) {
  // logger::udm_server().startup("Starting...");

  // logger::udm_server().startup("Started");
}

udm_app::~udm_app() {
  // logger::udm_server().debug("Delete UDM_APP instance...");
}

void udm_app::handle_generate_auth_data_request(
    const std::string& supiOrSuci,
    const oai::udm::model::AuthenticationInfoRequest& authenticationInfoRequest,
    nlohmann::json& auth_info_response, Pistache::Http::Code& code) {
  uint8_t rand[16] = {0};
  uint8_t opc[16]  = {0};
  uint8_t key[16]  = {0};
  uint8_t sqn[6]   = {0};
  uint8_t amf[2]   = {0};

  uint8_t* r_sqn = NULL;     // for resync
  std::string r_sqnms_s;     // for resync
  uint8_t r_rand[16] = {0};  // for resync
  uint8_t r_auts[14] = {0};  // for resync

  uint8_t mac_a[8]     = {0};
  uint8_t ck[16]       = {0};
  uint8_t ik[16]       = {0};
  uint8_t ak[6]        = {0};
  uint8_t xres[8]      = {0};
  uint8_t xresStar[16] = {0};
  uint8_t autn[16]     = {0};
  uint8_t kausf[32]    = {0};

  std::string rand_s;
  std::string autn_s;
  std::string xresStar_s;
  std::string kausf_s;
  std::string sqn_s;
  std::string amf_s;
  std::string key_s;
  std::string opc_s;

  std::string snn  = authenticationInfoRequest.getServingNetworkName();
  std::string supi = supiOrSuci;

  std::string udr_ip =
      std::string(inet_ntoa(*((struct in_addr*) &udm_cfg.udr_addr.ipv4_addr)));
  std::string udr_port = std::to_string(udm_cfg.udr_addr.port);
  std::string remoteUri;
  std::string Method;
  std::string msgBody;
  std::string Response;

  nlohmann::json j_ProblemDetails;
  ProblemDetails m_ProblemDetails;

  // UDR GET interface ----- get authentication related info--------------------
  remoteUri = udr_ip + ":" + udr_port + "/nudr-dr/v2/subscription-data/" +
              supi + "/authentication-data/authentication-subscription";
  Logger::udm_ueau().debug("GET Request:" + remoteUri);
  Method = "GET";

  Curl::curl_http_client(remoteUri, Method, "", Response);

  nlohmann::json response_data = {};
  try {
    response_data = nlohmann::json::parse(Response.c_str());
  } catch (nlohmann::json::exception& e) {  // error handling
    Logger::udm_ueau().info("Could not get Json content from UDR response");

    m_ProblemDetails.setCause("USER_NOT_FOUND");
    m_ProblemDetails.setStatus(404);
    m_ProblemDetails.setDetail("User " + supi + " not found in Database");
    to_json(j_ProblemDetails, m_ProblemDetails);

    Logger::udm_ueau().error("User " + supi + " not found in Database");
    Logger::udm_ueau().info("Send 404 Not_Found response to AUSF");
    // response.send(Pistache::Http::Code::Not_Found, j_ProblemDetails.dump());
    auth_info_response = j_ProblemDetails.dump();
    code               = Pistache::Http::Code::Not_Found;
    return;
  }

  std::string authMethod_s = response_data.at("authenticationMethod");
  if (!authMethod_s.compare("5G_AKA") ||
      !authMethod_s.compare("AuthenticationVector")) {
    try {
      key_s = response_data.at("encPermanentKey");
      conv::hex_str_to_uint8(key_s.c_str(), key);
      // comUt::print_buffer("udm_ueau", "Result For F1-Alg: key", key , 16);

      opc_s = response_data.at("encOpcKey");
      conv::hex_str_to_uint8(opc_s.c_str(), opc);
      // comUt::print_buffer("udm_ueau", "Result For F1-Alg: opc", opc , 16);

      amf_s = response_data.at("authenticationManagementField");
      conv::hex_str_to_uint8(amf_s.c_str(), amf);
      // comUt::print_buffer("udm_ueau", "Result For F1-Alg: amf", amf , 2);

      sqn_s = response_data["sequenceNumber"].at("sqn");
      conv::hex_str_to_uint8(sqn_s.c_str(), sqn);
      // comUt::print_buffer("udm_ueau", "Result For F1-Alg: sqn", sqn , 6);
    } catch (nlohmann::json::exception& e) {
      // error handling
      m_ProblemDetails.setCause("AUTHENTICATION_REJECTED");
      m_ProblemDetails.setStatus(403);
      m_ProblemDetails.setDetail(
          "Missing authentication parameter in UDR response");
      to_json(j_ProblemDetails, m_ProblemDetails);

      Logger::udm_ueau().error(
          "Missing authentication parameter in UDR response");
      Logger::udm_ueau().info("Send 403 Forbidden response to AUSF");
      // response.send(Pistache::Http::Code::Forbidden,
      // j_ProblemDetails.dump());
      auth_info_response = j_ProblemDetails.dump();
      code               = Pistache::Http::Code::Forbidden;
      return;
    }
  } else {
    // error handling
    m_ProblemDetails.setCause("UNSUPPORTED_PROTECTION_SCHEME");
    m_ProblemDetails.setStatus(501);
    m_ProblemDetails.setDetail(
        "Non 5G_AKA authenticationMethod configuration in database");
    to_json(j_ProblemDetails, m_ProblemDetails);

    Logger::udm_ueau().error(
        "Non 5G_AKA authenticationMethod configuration in "
        "database, method set = " +
        authMethod_s);
    Logger::udm_ueau().info("Send 501 Not_Implemented response to AUSF");
    // response.send(
    //    Pistache::Http::Code::Not_Implemented, j_ProblemDetails.dump());
    auth_info_response = j_ProblemDetails.dump();
    code               = Pistache::Http::Code::Not_Implemented;

    return;
  }

  if (authenticationInfoRequest.resynchronizationInfoIsSet()) {
    // resync procedure
    // ---------------------------------------------------------
    Logger::udm_ueau().info("Start resynchronization procedure");
    ResynchronizationInfo m_ResynchronizationInfo =
        authenticationInfoRequest.getResynchronizationInfo();
    std::string r_rand_s = m_ResynchronizationInfo.getRand();
    std::string r_auts_s = m_ResynchronizationInfo.getAuts();

    Logger::udm_ueau().debug("[resync] r_rand = " + r_rand_s);
    Logger::udm_ueau().debug("[resync] r_auts = " + r_auts_s);

    conv::hex_str_to_uint8(r_rand_s.c_str(), r_rand);
    conv::hex_str_to_uint8(r_auts_s.c_str(), r_auts);

    r_sqn = Authentication_5gaka::sqn_ms_derive(opc, key, r_auts, r_rand, amf);

    if (r_sqn) {  // Not NULL (validate auts)
      Logger::udm_ueau().debug("Valid AUTS, generate new AV with SQNms");

      // UDR PATCH interface ------- replace SQNhe with
      // SQNms------------------------------
      remoteUri = udr_ip + ":" + udr_port + "/nudr-dr/v2/subscription-data/" +
                  supi + "/authentication-data/authentication-subscription";
      Logger::udm_ueau().debug("PATCH Request:" + remoteUri);
      Method = "PATCH";

      nlohmann::json j_SequenceNumber;
      SequenceNumber m_SequenceNumber;
      m_SequenceNumber.setSqnScheme("NON_TIME_BASED");
      r_sqnms_s = conv::uint8_to_hex_string(r_sqn, 6);
      m_SequenceNumber.setSqn(r_sqnms_s);
      std::map<std::string, int32_t> index;
      index["ausf"] = 0;
      m_SequenceNumber.setLastIndexes(index);
      to_json(j_SequenceNumber, m_SequenceNumber);

      nlohmann::json j_PatchItem;
      PatchItem m_PatchItem;
      m_PatchItem.setValue(j_SequenceNumber.dump());
      m_PatchItem.setOp("replace");
      m_PatchItem.setFrom("");
      m_PatchItem.setPath("");
      to_json(j_PatchItem, m_PatchItem);

      msgBody = "[" + j_PatchItem.dump() + "]";
      Logger::udm_ueau().debug("PATCH Request body = " + msgBody);

      Curl::curl_http_client(remoteUri, Method, msgBody, Response);

      // replace SQNhe with SQNms
      int i = 0;
      for (i; i < 6; i++) sqn[i] = r_sqn[i];  // generate first, increase later
      sqn_s = conv::uint8_to_hex_string(sqn, 16);
      // Logger::udm_ueau().debug("sqn string = "+sqn_s);
      sqn_s[12] = '\0';

      comUt::print_buffer("udm_ueau", "SQNms", sqn, 6);

      if (r_sqn) {  // free
        free(r_sqn);
        r_sqn = NULL;
      }
    } else {
      Logger::udm_ueau().error(
          "Invalid AUTS, generate new AV with SQNhe = " + sqn_s);
    }
  }

  // 5GAKA functions---------------------------------------------------------
  Authentication_5gaka::generate_random(rand, 16);  // generate rand
  Authentication_5gaka::f1(
      opc, key, rand, sqn, amf,
      mac_a);  // to compute mac_a
  Authentication_5gaka::f2345(
      opc, key, rand, xres, ck, ik,
      ak);  // to compute XRES, CK, IK, AK
  Authentication_5gaka::generate_autn(
      sqn, ak, amf, mac_a,
      autn);  // generate AUTN
  Authentication_5gaka::annex_a_4_33501(
      ck, ik, xres, rand, snn,
      xresStar);  // generate xres*
  Authentication_5gaka::derive_kausf(
      ck, ik, snn, sqn, ak,
      kausf);  // derive Kausf

  // convert uint8_t to string
  rand_s     = conv::uint8_to_hex_string(rand, 16);
  autn_s     = conv::uint8_to_hex_string(autn, 16);
  xresStar_s = conv::uint8_to_hex_string(xresStar, 16);
  kausf_s    = conv::uint8_to_hex_string(kausf, 32);

  // convert to json
  nlohmann::json AuthInfoResult                      = {};
  AuthInfoResult["authType"]                         = "5G_AKA";
  AuthInfoResult["authenticationVector"]["avType"]   = "5G_HE_AKA";
  AuthInfoResult["authenticationVector"]["rand"]     = rand_s;
  AuthInfoResult["authenticationVector"]["autn"]     = autn_s;
  AuthInfoResult["authenticationVector"]["xresStar"] = xresStar_s;
  AuthInfoResult["authenticationVector"]["kausf"]    = kausf_s;

  // TODO: Separate into a new function
  // Do it after send ok to AUSF (to be verified)

  // calculate new sqn----------------------------------------------------------
  unsigned long long sqn_value;
  std::stringstream s1;
  s1 << std::hex << sqn_s;
  s1 >> sqn_value;  // hex string to decimal value
  sqn_value += 32;
  std::stringstream s2;
  s2 << std::hex << std::setw(12) << std::setfill('0')
     << sqn_value;  // decimal value to hex string
  std::string new_sqn(s2.str());

  Logger::udm_ueau().debug("new_sqn = " + new_sqn);

  // UDR PATCH interface ------- increase
  // sqn------------------------------------
  remoteUri = udr_ip + ":" + udr_port + "/nudr-dr/v2/subscription-data/" +
              supi + "/authentication-data/authentication-subscription";
  Logger::udm_ueau().debug("PATCH Request:" + remoteUri);
  Method = "PATCH";

  nlohmann::json j_SequenceNumber;
  SequenceNumber m_SequenceNumber;
  m_SequenceNumber.setSqnScheme("NON_TIME_BASED");
  m_SequenceNumber.setSqn(new_sqn);
  std::map<std::string, int32_t> index;
  index["ausf"] = 0;
  m_SequenceNumber.setLastIndexes(index);
  to_json(j_SequenceNumber, m_SequenceNumber);

  nlohmann::json j_PatchItem;
  PatchItem m_PatchItem;
  m_PatchItem.setValue(j_SequenceNumber.dump());
  m_PatchItem.setOp("replace");
  m_PatchItem.setFrom("");
  m_PatchItem.setPath("");
  to_json(j_PatchItem, m_PatchItem);

  msgBody = "[" + j_PatchItem.dump() + "]";
  // Logger::udm_ueau().debug("PATCH Request body = " + msgBody);

  Curl::curl_http_client(remoteUri, Method, msgBody, Response);

  Logger::udm_ueau().info("Send 200 Ok response to AUSF");
  // response.send(Pistache::Http::Code::Ok, AuthInfoResult.dump());
  auth_info_response = AuthInfoResult.dump();
  code               = Pistache::Http::Code::Ok;
  return;
}

void udm_app::handle_confirm_auth(
    const std::string& supi, const oai::udm::model::AuthEvent& authEvent,
    nlohmann::json& confirm_response, std::string& location,
    Pistache::Http::Code& code) {
  std::string udr_ip =
      std::string(inet_ntoa(*((struct in_addr*) &udm_cfg.udr_addr.ipv4_addr)));
  std::string udr_port = std::to_string(udm_cfg.udr_addr.port);
  std::string remoteUri;
  std::string Method;
  std::string msgBody;
  std::string Response;
  std::string Location;
  std::string authEventId;

  nlohmann::json j_ProblemDetails;
  ProblemDetails m_ProblemDetails;

  // UDR GET interface ----- get user info--------------------
  remoteUri = udr_ip + ":" + udr_port + "/nudr-dr/v2/subscription-data/" +
              supi + "/authentication-data/authentication-subscription";
  Logger::udm_ueau().debug("GET Request:" + remoteUri);
  Method = "GET";

  Curl::curl_http_client(remoteUri, Method, "", Response);

  nlohmann::json response_data = {};
  try {
    response_data = nlohmann::json::parse(Response.c_str());
  } catch (nlohmann::json::exception& e) {  // error handling
    Logger::udm_ueau().info("Could not get Json content from UDR response");

    m_ProblemDetails.setCause("USER_NOT_FOUND");
    m_ProblemDetails.setStatus(404);
    m_ProblemDetails.setDetail("User " + supi + " not found in Database");
    to_json(j_ProblemDetails, m_ProblemDetails);

    Logger::udm_ueau().error("User " + supi + " not found in Database");
    Logger::udm_ueau().info("Send 404 Not_Found response to AUSF");
    // response.send(Pistache::Http::Code::Not_Found, j_ProblemDetails.dump());
    confirm_response = j_ProblemDetails.dump();
    code             = Pistache::Http::Code::Not_Found;
    return;
  }

  if (authEvent.isAuthRemovalInd()) {
    // error handling
    m_ProblemDetails.setStatus(400);
    m_ProblemDetails.setDetail("authRemovalInd should be false");
    to_json(j_ProblemDetails, m_ProblemDetails);

    Logger::udm_ueau().error("authRemovalInd should be false");
    Logger::udm_ueau().info("Send 400 Bad_Request response to AUSF");
    // response.send(Pistache::Http::Code::Bad_Request,
    // j_ProblemDetails.dump());
    confirm_response = j_ProblemDetails.dump();
    code             = Pistache::Http::Code::Bad_Request;
    return;
  }

  // UDR PUT interface ------- put authentication
  // status------------------------------
  remoteUri = udr_ip + ":" + udr_port + "/nudr-dr/v2/subscription-data/" +
              supi + "/authentication-data/authentication-status";

  Logger::udm_ueau().debug("PUT Request:" + remoteUri);
  Method = "PUT";

  nlohmann::json j_authEvent;
  to_json(j_authEvent, authEvent);

  msgBody = j_authEvent.dump();
  Logger::udm_ueau().debug("PATCH Request body = " + msgBody);

  Curl::curl_http_client(remoteUri, Method, msgBody, Response);

  std::string hash_value = sha256(supi + authEvent.getServingNetworkName());
  // Logger::udm_ueau().debug("\n\nauthEventId=" +
  // hash_value.substr(0,hash_value.length()/2));
  Logger::udm_ueau().debug("authEventId=" + hash_value);

  authEventId = hash_value;  // Represents the authEvent Id per UE per serving
                             // network assigned by the UDM during
                             // ResultConfirmation service operation.
  location = std::string(inet_ntoa(*((struct in_addr*) &udm_cfg.sbi.addr4))) +
             ":" + std::to_string(udm_cfg.sbi.port) + "/nudm-ueau/v1/" + supi +
             "/auth-events/" + authEventId;

  Logger::udm_ueau().info("Send 201 Created response to AUSF");
  // response.headers().add<Pistache::Http::Header::Location>(Location);
  // response.send(Pistache::Http::Code::Created, j_authEvent.dump());
  confirm_response = j_authEvent.dump();
  code             = Pistache::Http::Code::Created;
  return;
}

void udm_app::handle_delete_auth(
    const std::string& supi, const std::string& authEventId,
    const oai::udm::model::AuthEvent& authEvent, nlohmann::json& auth_response,
    Pistache::Http::Code& code) {
  std::string udr_ip =
      std::string(inet_ntoa(*((struct in_addr*) &udm_cfg.udr_addr.ipv4_addr)));
  std::string udr_port = std::to_string(udm_cfg.udr_addr.port);
  std::string remoteUri;
  std::string Method;
  std::string msgBody;
  std::string Response;
  std::string Location;

  nlohmann::json j_ProblemDetails;
  ProblemDetails m_ProblemDetails;

  // UDR GET interface ----- get user info--------------------
  remoteUri = udr_ip + ":" + udr_port + "/nudr-dr/v2/subscription-data/" +
              supi + "/authentication-data/authentication-subscription";
  Logger::udm_ueau().debug("GET Request:" + remoteUri);
  Method = "GET";

  Curl::curl_http_client(remoteUri, Method, "", Response);

  nlohmann::json response_data = {};
  try {
    response_data = nlohmann::json::parse(Response.c_str());
  } catch (nlohmann::json::exception& e) {  // error handling
    Logger::udm_ueau().info("Could not get Json content from UDR response");

    m_ProblemDetails.setCause("USER_NOT_FOUND");
    m_ProblemDetails.setStatus(404);
    m_ProblemDetails.setDetail("User " + supi + " not found in Database");
    to_json(j_ProblemDetails, m_ProblemDetails);

    Logger::udm_ueau().error("User " + supi + " not found in Database");
    Logger::udm_ueau().info("Send 404 Not_Found response to AUSF");
    // response.send(Pistache::Http::Code::Not_Found, j_ProblemDetails.dump());
    auth_response = j_ProblemDetails.dump();
    code          = Pistache::Http::Code::Not_Found;

    return;
  }

  if (!authEvent.isAuthRemovalInd()) {
    // error handling
    m_ProblemDetails.setStatus(400);
    m_ProblemDetails.setDetail("authRemovalInd should be true");
    to_json(j_ProblemDetails, m_ProblemDetails);

    Logger::udm_ueau().error("authRemovalInd should be true");
    Logger::udm_ueau().info("Send 400 Bad_Request response to AUSF");
    // response.send(Pistache::Http::Code::Bad_Request,
    // j_ProblemDetails.dump());
    auth_response = j_ProblemDetails.dump();
    code          = Pistache::Http::Code::Bad_Request;
    return;
  }

  std::string hash_value = sha256(supi + authEvent.getServingNetworkName());
  // Logger::udm_ueau().debug("\n\nauthEventId=" +
  // hash_value.substr(0,hash_value.length()/2));
  Logger::udm_ueau().debug("authEventId=" + hash_value);

  if (!hash_value.compare(authEventId)) {
    // UDR DELETE interface ------- delete authentication
    // status------------------------------
    remoteUri = udr_ip + ":" + udr_port + "/nudr-dr/v2/subscription-data/" +
                supi + "/authentication-data/authentication-status";

    Logger::udm_ueau().debug("DELETE Request:" + remoteUri);
    Method = "DELETE";

    nlohmann::json j_authEvent;
    to_json(j_authEvent, authEvent);

    Curl::curl_http_client(remoteUri, Method, "", Response);

    Logger::udm_ueau().info("Send 204 No_Content response to AUSF");
    // response.send(Pistache::Http::Code::No_Content, "");
    auth_response = {};
    code          = Pistache::Http::Code::No_Content;
    return;
  } else {
    // error handling
    // wrong autheventid
    m_ProblemDetails.setCause("DATA_NOT_FOUND");
    m_ProblemDetails.setStatus(404);
    m_ProblemDetails.setDetail("Wrong authEventId");
    to_json(j_ProblemDetails, m_ProblemDetails);

    Logger::udm_ueau().error("Wrong authEventId, should be = " + hash_value);
    Logger::udm_ueau().info("Send 404 Not_Found response to AUSF");
    // response.send(Pistache::Http::Code::Not_Found, j_ProblemDetails.dump());
    auth_response = j_ProblemDetails.dump();
    code          = Pistache::Http::Code::Not_Found;
  }
}
