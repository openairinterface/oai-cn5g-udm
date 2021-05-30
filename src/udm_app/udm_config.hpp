/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this
 *file except in compliance with the License. You may obtain a copy of the
 *License at
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

/*! \file udm_config.hpp
 \brief
 \author  Hongxin WANG, BUPT
 \date 2021
 \email: contact@openairinterface.org
 */

#ifndef _UDM_CONFIG_H_
#define _UDM_CONFIG_H_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <libconfig.h++>
#include <mutex>
#include <string>
#include <vector>

#include "udm_config.hpp"
//#include "thread_sched.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

#define UDM_CONFIG_STRING_UDM_CONFIG "UDM"
#define UDM_CONFIG_STRING_PID_DIRECTORY "PID_DIRECTORY"
#define UDM_CONFIG_STRING_INSTANCE_ID "INSTANCE_ID"
#define UDM_CONFIG_STRING_UDM_NAME "UDM_NAME"

#define UDM_CONFIG_STRING_INTERFACES "INTERFACES"
#define UDM_CONFIG_STRING_INTERFACE_SBI_UDM "SBI_UDM"
#define UDM_CONFIG_STRING_INTERFACE_NUDR "NUDR"
#define UDM_CONFIG_STRING_INTERFACE_NAME "INTERFACE_NAME"
#define UDM_CONFIG_STRING_IPV4_ADDRESS "IPV4_ADDRESS"
#define UDM_CONFIG_STRING_PORT "PORT"
#define UDM_CONFIG_STRING_PPID "PPID"

// #define UDM_CONFIG_STRING_UDR_INSTANCES_POOL            "UDR_INSTANCES_POOL"
// #define UDM_CONFIG_STRING_UDR_INSTANCE_ID               "UDR_INSTANCE_ID"
// #define UDM_CONFIG_STRING_UDR_INSTANCE_PORT             "PORT"
// #define UDM_CONFIG_STRING_UDR_INSTANCE_VERSION          "VERSION"
// #define UDM_CONFIG_STRING_UDR_INSTANCE_SELECTED         "SELECTED"

// #define UDM_CONFIG_STRING_STATISTICS_TIMER_INTERVAL
// "STATISTICS_TIMER_INTERVAL"

// #define UDM_CONFIG_STRING_GUAMI                         "GUAMI"
// #define UDM_CONFIG_STRING_SERVED_GUAMI_LIST             "SERVED_GUAMI_LIST"
// #define UDM_CONFIG_STRING_RegionID                      "RegionID"
// #define UDM_CONFIG_STRING_AMFSetID                      "AMFSetID"
// #define UDM_CONFIG_STRING_AMFPointer                    "AMFPointer"
// #define UDM_CONFIG_STRING_RELATIVE_AMF_CAPACITY         "RELATIVE_CAPACITY"

// #define UDM_CONFIG_STRING_TAC                           "TAC"
// #define UDM_CONFIG_STRING_MCC                           "MCC"
// #define UDM_CONFIG_STRING_MNC                           "MNC"
// #define UDM_CONFIG_STRING_PLMN_SUPPORT_LIST             "PLMN_SUPPORT_LIST"

// #define UDM_CONFIG_STRING_SLICE_SUPPORT_LIST            "SLICE_SUPPORT_LIST"
// #define UDM_CONFIG_STRING_SST                           "SST"
// #define UDM_CONFIG_STRING_SD                            "SD"

// #define UDM_CONFIG_STRING_CORE_CONFIGURATION            "CORE_CONFIGURATION"
// #define UDM_CONFIG_STRING_EMERGENCY_SUPPORT             "EMERGENCY_SUPPORT"

// #define UDM_CONFIG_STRING_AUTHENTICATION                "AUTHENTICATION"
// #define UDM_CONFIG_STRING_AUTH_MYSQL_SERVER             "MYSQL_server"
// #define UDM_CONFIG_STRING_AUTH_MYSQL_USER               "MYSQL_user"
// #define UDM_CONFIG_STRING_AUTH_MYSQL_PASS               "MYSQL_pass"
// #define UDM_CONFIG_STRING_AUTH_MYSQL_DB                 "MYSQL_db"
// #define UDM_CONFIG_STRING_AUTH_OPERATOR_KEY             "OPERATOR_key"
// #define UDM_CONFIG_STRING_AUTH_RANDOM                   "RANDOM"

using namespace libconfig;

namespace config {

typedef struct interface_cfg_s {
  std::string if_name;
  struct in_addr addr4;
  struct in_addr network4;
  struct in6_addr addr6;
  unsigned int mtu;
  unsigned int port;
} interface_cfg_t;

// typedef struct slice_s {
//   std::string sST;
//   std::string sD;
// } slice_t;

// typedef struct plmn_support_item_s {
//   std::string mcc;
//   std::string mnc;
//   uint32_t tac;
//   std::vector<slice_t> slice_list;
// } plmn_item_t;

// typedef struct {
//   int id;
//   std::string ipv4;
//   std::string port;
//   std::string version;
//   bool selected;
// } udr_inst_t;

// typedef struct {
//   std::string mysql_server;
//   std::string mysql_user;
//   std::string mysql_pass;
//   std::string mysql_db;
//   std::string operator_key;
//   std::string random;
// } auth_conf;

class udm_config {
 public:
  udm_config();
  ~udm_config();
  int load(const std::string& config_file);
  int load_interface(const Setting& if_cfg, interface_cfg_t& cfg);
  void display();

  unsigned int instance;
  std::string pid_dir;
  std::string UDM_Name;

  interface_cfg_t sbi;
  interface_cfg_t nudr;

  // unsigned int statistics_interval;
  // std::vector<plmn_item_t> plmn_list;
  // std::string is_emergency_support;
  // auth_conf auth_para;
  // std::vector<udr_inst_t> udr_pool;
};

}  // namespace config

#endif
