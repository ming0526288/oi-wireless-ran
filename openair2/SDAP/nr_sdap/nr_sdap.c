/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
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

#include "nr_sdap.h"
//MODIF 1
#if LATSEQ
  #include "common/utils/LATSEQ/latseq.h"
  #include "executables/nr-uesoftmodem.h"
#endif

uint8_t nas_qfi;
uint8_t nas_pduid;

bool sdap_data_req(protocol_ctxt_t *ctxt_p,
                   const ue_id_t ue_id,
                   const srb_flag_t srb_flag,
                   const rb_id_t rb_id,
                   const mui_t mui,
                   const confirm_t confirm,
                   const sdu_size_t sdu_buffer_size,
                   unsigned char *const sdu_buffer,
                   const pdcp_transmission_mode_t pt_mode,
                   const uint32_t *sourceL2Id,
                   const uint32_t *destinationL2Id,
                   const uint8_t qfi,
                   const bool rqi,
                   const int pdusession_id) {
  nr_sdap_entity_t *sdap_entity;
  sdap_entity = nr_sdap_get_entity(ue_id, pdusession_id);
   // MODIF 2
  #if LATSEQ
  if (latseq_ul)
  {
    if (sdap_entity != NULL)
    {
    //check IP version

      //IP info
      uint8_t ip_type = sdu_buffer[0] >> 4; // >> ((sdu_buffer_size * 8) - 4)
      if (ip_type == 4)
      {
        uint16_t ip_id = sdu_buffer[4] << 8 | sdu_buffer[5];
        uint8_t ip_hdr_size = (sdu_buffer[0] & 0x0f) * 4; // size is number of line with 4 bytes
        uint8_t protocol_id = sdu_buffer[9];

        //UDP info
        if (protocol_id == 17)
        {
          uint8_t udp_hdr_size = 8;
          int full_hdr_size = ip_hdr_size + udp_hdr_size;
          int app_count_idx = full_hdr_size+8;
          uint8_t app_count_size_bytes = 8;
          //uint8_t decal = 7;
          uint64_t app_count = 0;//sdu_buffer[app_count_idx]; // step 1  << (full_hdr_size * 8)

          uint8_t ind = ip_hdr_size + 0;
          uint8_t ind2 = ip_hdr_size + 1;
          uint16_t sp = sdu_buffer[ind] << 8 | sdu_buffer[ind2];

          ind = ip_hdr_size + 2;
          ind2 = ip_hdr_size + 3;
          uint16_t dp = sdu_buffer[ind] << 8 | sdu_buffer[ind2];

          for (int i = app_count_idx; i < app_count_idx + app_count_size_bytes; i++)
          {
            app_count = app_count << 8| sdu_buffer[i];
          }

          LATSEQ_P("U sdap.ts","uid=%d,rbid=%d,slen=%d,ipt=%d,iphl=%d,ptid=%d,appc=%ld,ipid=%d,sp=%d,dp=%d",
                                      ue_id, rb_id, sdu_buffer_size,ip_type,
                                      ip_hdr_size, protocol_id, app_count, ip_id,
                                      sp, dp);
        }
        else
        {
          LATSEQ_P("U sdap.ts","uid=%d,rbid=%d,slen=%d,ipt=%d,iphl=%d,ptid=%d,ipid=%d",
                                      ue_id, rb_id, sdu_buffer_size, ip_type,
                                      ip_hdr_size, protocol_id, ip_id);
        }
      }
      else
      {
        LATSEQ_P("U sdap.ts.ipv6","uid=%d,rbid=%d,slen=%d,ipt=%d",
                                      ue_id, rb_id, sdu_buffer_size, ip_type);
      }
    }
    else
    {
      LATSEQ_P("U sdap.ts","uid=%d,rbid=%d,slen=%d", ue_id, rb_id, sdu_buffer_size);
    }
  }
  #endif
  if(sdap_entity == NULL) {
    LOG_E(SDAP, "%s:%d:%s: Entity not found with ue: 0x%"PRIx64" and pdusession id: %d\n", __FILE__, __LINE__, __FUNCTION__, ue_id, pdusession_id);
    return 0;
  }

  bool ret = sdap_entity->tx_entity(sdap_entity,
                                    ctxt_p,
                                    srb_flag,
                                    rb_id,
                                    mui,
                                    confirm,
                                    sdu_buffer_size,
                                    sdu_buffer,
                                    pt_mode,
                                    sourceL2Id,
                                    destinationL2Id,
                                    qfi,
                                    rqi);
  return ret;
}

void sdap_data_ind(rb_id_t pdcp_entity,
                   int is_gnb,
                   bool has_sdap_rx,
                   int pdusession_id,
                   ue_id_t ue_id,
                   char *buf,
                   int size) {
  nr_sdap_entity_t *sdap_entity;
  sdap_entity = nr_sdap_get_entity(ue_id, pdusession_id);

  if (sdap_entity == NULL) {
    LOG_E(SDAP, "%s:%d:%s: Entity not found for ue rnti/ue_id: %lx and pdusession id: %d\n", __FILE__, __LINE__, __FUNCTION__, ue_id, pdusession_id);
    return;
  }

  sdap_entity->rx_entity(sdap_entity,
                         pdcp_entity,
                         is_gnb,
                         has_sdap_rx,
                         pdusession_id,
                         ue_id,
                         buf,
                         size);
}

void set_qfi_pduid(uint8_t qfi, uint8_t pduid){
  nas_qfi = qfi;
  nas_pduid = pduid;
  return;
}
