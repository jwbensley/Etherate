/*
 * License: MIT
 *
 * Copyright (c) 2012-2018 James Bensley.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * File: Etherate Setup Functions
 *
 */



#include "defaults.h"

int16_t default_app(struct etherate *eth)
{

    eth->app.broadcast   = true;
    eth->app.tx_mode     = true;
    eth->app.tx_sync     = true;
    eth->app.tx_delay    = TX_DELAY_DEF;


    eth->frm.dst_mac[0]        = 0x00;
    eth->frm.dst_mac[1]        = 0x00;
    eth->frm.dst_mac[2]        = 0x5E;
    eth->frm.dst_mac[3]        = 0x00;
    eth->frm.dst_mac[4]        = 0x00;
    eth->frm.dst_mac[5]        = 0x02;
    eth->frm.etype             = ETYPE_DEF;
    eth->frm.length            = HEADERS_LEN_DEF;
    eth->frm.lsp_dst_mac[0]    = 0x00;
    eth->frm.lsp_dst_mac[1]    = 0x00;
    eth->frm.lsp_dst_mac[2]    = 0x00;
    eth->frm.lsp_dst_mac[3]    = 0x00;
    eth->frm.lsp_dst_mac[4]    = 0x00;
    eth->frm.lsp_dst_mac[5]    = 0x00;
    eth->frm.lsp_src_mac[0]    = 0x00;
    eth->frm.lsp_src_mac[1]    = 0x00;
    eth->frm.lsp_src_mac[2]    = 0x00;
    eth->frm.lsp_src_mac[3]    = 0x00;
    eth->frm.lsp_src_mac[4]    = 0x00;
    eth->frm.lsp_src_mac[5]    = 0x00;
    eth->frm.mpls_labels       = 0;
    for (uint16_t i = 0; i<MPLS_LABELS_MAX; i += 1) {
        eth->frm.mpls_label[i] = 0;
        eth->frm.mpls_exp[i]   = 0;
        eth->frm.mpls_ttl[i]   = 0;
    }
    eth->frm.pcp               = PCP_DEF;
    eth->frm.pwe_ctrl_word     = 0;
    eth->frm.qinq_dei          = 0;
    eth->frm.qinq_id           = QINQ_ID_DEF;
    eth->frm.qinq_pcp          = QINQ_PCP_DEF;
    eth->frm.rx_buffer         = (uint8_t*)calloc(1, F_SIZE_MAX);
    eth->frm.src_mac[0]        = 0x00;
    eth->frm.src_mac[1]        = 0x00;
    eth->frm.src_mac[2]        = 0x5E;
    eth->frm.src_mac[3]        = 0x00;
    eth->frm.src_mac[4]        = 0x00;
    eth->frm.src_mac[5]        = 0x01;
    eth->frm.tlv_size          = sizeof(uint8_t) + sizeof(uint16_t) +
                                 sizeof(uint32_t);
    eth->frm.sub_tlv_size      = eth->frm.tlv_size + sizeof(uint8_t) +
                                 sizeof(uint16_t) + sizeof(uint64_t);
    eth->frm.sub_tlv_val       = NULL; /////
    eth->frm.tx_buffer         = (uint8_t*)calloc(1, F_SIZE_MAX);
    eth->frm.vlan_dei          = 0;
    eth->frm.vlan_id           = VLAN_ID_DEF;

    if (eth->frm.rx_buffer == NULL ||
        eth->frm.tx_buffer == NULL ) {
        perror("Couldn't allocate Tx/Rx buffers ");
        return EXIT_FAILURE;
    }


    eth->mtu_test.enabled    = false;
    eth->mtu_test.largest    = 0;
    eth->mtu_test.mtu_tx_min = 1400;
    eth->mtu_test.mtu_tx_max = 1500;


    eth->qm_test.enabled          = false;
    eth->qm_test.delay_test_count = 10000;
    eth->qm_test.interval         = 1000;
    eth->qm_test.interval_sec     = 0;
    eth->qm_test.interval_nsec    = 0;
    eth->qm_test.interval_min     = 99999.99999;
    eth->qm_test.interval_max     = 0.0;
    eth->qm_test.jitter_min       = 999999.999999;
    eth->qm_test.jitter_max       = 0.0;
    eth->qm_test.rtt_min          = 999999.999999;
    eth->qm_test.rtt_max          = 0.0;
    eth->qm_test.test_count       = 0;
    eth->qm_test.time_tx_1        = NULL;
    eth->qm_test.time_tx_2        = NULL;
    eth->qm_test.time_rx_1        = NULL;
    eth->qm_test.time_rx_2        = NULL;
    eth->qm_test.time_tx_diff     = NULL;
    eth->qm_test.time_rx_diff     = NULL;
    eth->qm_test.timeout          = 1000;
    eth->qm_test.timeout_nsec     = 0;
    eth->qm_test.timeout_sec      = 0;
    eth->qm_test.timeout_count    = 0;
    eth->qm_test.delay_results    = (double*)calloc(eth->qm_test.delay_test_count,sizeof(double));

    if (eth->qm_test.delay_results == NULL) {
        perror("Couldn't allocate quality test results buffer ");
        return EXIT_FAILURE;
    }


    eth->speed_test.enabled         = false;
    eth->speed_test.b_rx            = 0;
    eth->speed_test.b_rx_prev       = 0;
    eth->speed_test.b_tx            = 0;
    eth->speed_test.b_tx_prev       = 0;
    eth->speed_test.b_tx_speed_max  = B_TX_SPEED_MAX_DEF;
    eth->speed_test.b_tx_speed_prev = 0;
    eth->speed_test.f_payload       = (uint8_t*)calloc(1, F_SIZE_MAX);
    eth->speed_test.f_payload_size  = 0;
    eth->speed_test.f_index_prev    = 0;
    eth->speed_test.f_speed         = 0;
    eth->speed_test.f_speed_avg     = 0;
    eth->speed_test.f_speed_max     = 0;
    eth->speed_test.b_speed         = 0;
    eth->speed_test.b_speed_max     = 0;
    eth->speed_test.b_speed_avg     = 0;

    if (eth->speed_test.f_payload == NULL) {
        perror("Couldn't allocate custom frame buffer ");
        return EXIT_FAILURE;
    }


    eth->intf.if_index = IF_INDEX_DEF;
    for (uint16_t i = 0; i<IFNAMSIZ; i += 1) {
        eth->intf.if_name[i] = 0;
    }
    eth->intf.sock_fd = SOCK_FD_DEF;


    eth->params.f_ack           = false;
    eth->params.f_ack_pending   = 0;
    eth->params.f_bytes         = F_BYTES_DEF;
    eth->params.f_count         = F_COUNT_DEF; 
    eth->params.f_duration      = F_DURATION_DEF;
    eth->params.f_rx_count      = 0;
    eth->params.f_rx_count_prev = 0;
    eth->params.f_rx_early      = 0;
    eth->params.f_rx_late       = 0;
    eth->params.f_rx_ontime     = 0;
    eth->params.f_rx_other      = 0;
    eth->params.f_tx_dly        = F_TX_DLY_DEF;
    eth->params.f_tx_count      = 0;
    eth->params.f_tx_count_max  = F_TX_COUNT_MAX_DEF;
    eth->params.f_tx_count_prev = 0;
    eth->params.f_size          = F_SIZE_DEF;
    eth->params.f_size_total    = F_SIZE_DEF + eth->frm.length;
    eth->params.f_waiting_ack   = false;
    eth->params.s_elapsed       = 0;

    return EXIT_SUCCESS;
    
}



int16_t setup_frame(struct etherate *eth)
{

    printf("Source MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           eth->frm.src_mac[0], eth->frm.src_mac[1],
           eth->frm.src_mac[2], eth->frm.src_mac[3],
           eth->frm.src_mac[4], eth->frm.src_mac[5]);

    printf("Destination MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           eth->frm.dst_mac[0], eth->frm.dst_mac[1],
           eth->frm.dst_mac[2], eth->frm.dst_mac[3],
           eth->frm.dst_mac[4], eth->frm.dst_mac[5]);

    // Fill the test frame buffer with random data
    for (uint32_t i = 0; i < (uint16_t)(F_SIZE_MAX - eth->frm.length); i += 1)
    {
        eth->frm.tx_data[i] = (uint8_t)((255.0*rand()/(RAND_MAX+1.0)));
    }

    if (eth->app.broadcast == true) {

        // Broadcast to populate any switch/MAC tables
        int16_t broad_ret = broadcast_etherate(eth);
        if (broad_ret != 0) return broad_ret;

    }
    
    build_headers(&eth->frm);

    return EXIT_SUCCESS;

}



int16_t setup_sock(struct intf *intf)
{

    // Setup a new raw socket
    intf->sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if (intf->sock_fd < 0 )
    {
      perror("Error opening socket ");
      return EX_SOFTWARE;
    }


    // On newer Kernels skip the interface qdisc for a slight speed increase,
    // requires Kernel version 3.14.0 or newer
    #if defined(PACKET_QDISC_BYPASS)

        static const int32_t sock_qdisc_bypass = 1;
        int32_t sock_qdisc_ret = setsockopt(intf->sock_fd, SOL_PACKET, PACKET_QDISC_BYPASS, &sock_qdisc_bypass, sizeof(sock_qdisc_bypass));

        if (sock_qdisc_ret == -1) {
            perror("Error enabling QDISC bypass on socket ");
            return EXIT_FAILURE;
        }

    #endif


    return EXIT_SUCCESS;

}



int16_t set_sock_int(struct etherate *eth)
{


    // If the user has supplied an interface index try to use that
    if (eth->intf.if_index != IF_INDEX_DEF) {

        eth->intf.if_index = set_sock_interface_index(&eth->intf);
        if (eth->intf.if_index <= 0)
        {
            printf("Error: Couldn't set interface with index, "
                   "returned index was %" PRId32 "!\n",
                   eth->intf.if_index);

            return EX_SOFTWARE;
        }

    // Or if the user has supplied an interface name try to use that        
    } else if (strcmp((char*)eth->intf.if_name, "") != 0) {

        eth->intf.if_index = set_sock_interface_name(&eth->intf);
        if (eth->intf.if_index <= 0)
        {
            printf("Error: Couldn't set interface index from name, "
                   "returned index was %" PRId32 "!\n",
                   eth->intf.if_index);

            return EX_SOFTWARE;
        }

    // Otherwise, try and best guess an interface
    } else if (eth->intf.if_index == IF_INDEX_DEF) {

        eth->intf.if_index = get_sock_interface(&eth->intf);
        if (eth->intf.if_index <= 0)
        {
            printf("Error: Couldn't find appropriate interface ID, "
                  "returned ID was %" PRId32 "!\n",
                  eth->intf.if_index);

            return EX_SOFTWARE;
        }

    }


    // Link layer socket setup
    eth->intf.sock_addr.sll_family   = AF_PACKET;    
    eth->intf.sock_addr.sll_protocol = htons(ETH_P_ALL);
    eth->intf.sock_addr.sll_ifindex  = eth->intf.if_index;    
    eth->intf.sock_addr.sll_hatype   = ARPHRD_ETHER;
    eth->intf.sock_addr.sll_pkttype  = PACKET_OTHERHOST;
    eth->intf.sock_addr.sll_halen    = ETH_ALEN;        
    eth->intf.sock_addr.sll_addr[0]  = eth->frm.dst_mac[0];        
    eth->intf.sock_addr.sll_addr[1]  = eth->frm.dst_mac[1];
    eth->intf.sock_addr.sll_addr[2]  = eth->frm.dst_mac[2];
    eth->intf.sock_addr.sll_addr[3]  = eth->frm.dst_mac[3];
    eth->intf.sock_addr.sll_addr[4]  = eth->frm.dst_mac[4];
    eth->intf.sock_addr.sll_addr[5]  = eth->frm.dst_mac[5];
    eth->intf.sock_addr.sll_addr[6]  = 0x00;
    eth->intf.sock_addr.sll_addr[7]  = 0x00;

    // Bind the socket to the physical interface to remove duplicate frames
    // when VLAN tagged sub-interfaces are present
    int32_t sock_bind = bind(eth->intf.sock_fd,
                             (struct sockaddr *)&eth->intf.sock_addr,
                             sizeof(eth->intf.sock_addr));

    if (sock_bind == -1) {
        perror("Can't bind socket to interface ");
        return EXIT_FAILURE;
    }

 
    build_headers(&eth->frm);

    return EXIT_SUCCESS;

}
