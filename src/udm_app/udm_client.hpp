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

#ifndef FILE_UDM_CLIENT_HPP_SEEN
#define FILE_UDM_CLIENT_HPP_SEEN

#include <string>
#include <curl/curl.h>

namespace oai::udm::app {

enum class http_method_e { GET, POST, PUT, PATCH, DELETE };

class udm_client {
 private:
 public:
  udm_client();
  virtual ~udm_client();

  udm_client(udm_client const&) = delete;

  static udm_client& get_instance() {
    static udm_client instance;
    return instance;
  }

  static bool send_request(
      const std::string& remote_uri, const http_method_e& method,
      const std::string& msg_body, std::string& response, long& response_code);
};
}  // namespace oai::udm::app
#endif /* FILE_UDM_CLIENT_HPP_SEEN */
