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

#include "output_wrapper.hpp"

#include "iostream"

//------------------------------------------------------------------------------
void output_wrapper::print_buffer(
    const std::string app, const std::string sink, const uint8_t* buf,
    int len) {
  if (Logger::should_log(spdlog::level::debug)) {
    if (!app.compare("config")) Logger::config().info(sink.c_str());
    if (!app.compare("system")) Logger::system().info(sink.c_str());
    if (!app.compare("udm_ueau")) Logger::udm_ueau().info(sink.c_str());
    if (!app.compare("udm_uecm")) Logger::udm_uecm().info(sink.c_str());
    if (!app.compare("udm_ee")) Logger::udm_ee().info(sink.c_str());
    if (!app.compare("udm_sdm")) Logger::udm_sdm().info(sink.c_str());
    if (!app.compare("udm_nrf")) Logger::udm_nrf().info(sink.c_str());
    if (!app.compare("udm_app")) Logger::udm_app().info(sink.c_str());
    if (!app.compare("ausf_server")) Logger::udm_server().info(sink.c_str());
    for (int i = 0; i < len; i++) printf("%x ", buf[i]);
    printf("\n");
  }
}

//------------------------------------------------------------------------------
void output_wrapper::hex_str_2_byte(
    const char* src, unsigned char* dest, int len) {
  short i;
  unsigned char hBy, lBy;
  for (i = 0; i < len; i += 2) {
    hBy = toupper(src[i]);
    lBy = toupper(src[i + 1]);
    if (hBy > 0x39)
      hBy -= 0x37;
    else
      hBy -= 0x30;
    if (lBy > 0x39)
      lBy -= 0x37;
    else
      lBy -= 0x30;
    dest[i / 2] = (hBy << 4) | lBy;
  }
}
