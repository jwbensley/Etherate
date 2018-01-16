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
 * File Contents:
 * void set_default_values()
 * int16_t setup_frame()
 * int16_t setup_socket()
 * int16_t setup_socket_interface()
 *
 */



#include "defaults.h"

void set_default_values(struct app_params *app_params,
                        struct frame_headers *frame_headers,
                        struct mtu_test *mtu_test,
                        struct qm_test *qm_test,
                        struct speed_test *speed_test,
                        struct test_interface *test_interface,
                        struct test_params * test_params)
{

    papp_params             = app_params;
    app_params->broadcast   = true;
    app_params->tx_mode     = true;
    app_params->tx_sync     = true;
    app_params->tx_delay    = TX_DELAY_DEF;


    pframe_headers                   = frame_headers;
    frame_headers->dst_mac[0]        = 0x00;
    frame_headers->dst_mac[1]        = 0x00;
    frame_headers->dst_mac[2]        = 0x5E;
    frame_headers->dst_mac[3]        = 0x00;
    frame_headers->dst_mac[4]        = 0x00;
    frame_headers->dst_mac[5]        = 0x02;
    frame_headers->etype             = ETYPE_DEF;
    frame_headers->length            = HEADERS_LEN_DEF;
    frame_headers->lsp_dst_mac[0]    = 0x00;
    frame_headers->lsp_dst_mac[1]    = 0x00;
    frame_headers->lsp_dst_mac[2]    = 0x00;
    frame_headers->lsp_dst_mac[3]    = 0x00;
    frame_headers->lsp_dst_mac[4]    = 0x00;
    frame_headers->lsp_dst_mac[5]    = 0x00;
    frame_headers->lsp_src_mac[0]    = 0x00;
    frame_headers->lsp_src_mac[1]    = 0x00;
    frame_headers->lsp_src_mac[2]    = 0x00;
    frame_headers->lsp_src_mac[3]    = 0x00;
    frame_headers->lsp_src_mac[4]    = 0x00;
    frame_headers->lsp_src_mac[5]    = 0x00;
    frame_headers->mpls_labels       = 0;
    for (uint16_t i = 0; i<MPLS_LABELS_MAX; i += 1) {
        frame_headers->mpls_label[i] = 0;
        frame_headers->mpls_exp[i]   = 0;
        frame_headers->mpls_ttl[i]   = 0;
    }
    frame_headers->pcp               = PCP_DEF;
    frame_headers->pwe_ctrl_word     = 0;
    frame_headers->qinq_dei          = 0;
    frame_headers->qinq_id           = QINQ_ID_DEF;
    frame_headers->qinq_pcp          = QINQ_PCP_DEF;
    frame_headers->rx_buffer         = (uint8_t*)calloc(1, F_SIZE_MAX);
    frame_headers->src_mac[0]        = 0x00;
    frame_headers->src_mac[1]        = 0x00;
    frame_headers->src_mac[2]        = 0x5E;
    frame_headers->src_mac[3]        = 0x00;
    frame_headers->src_mac[4]        = 0x00;
    frame_headers->src_mac[5]        = 0x01;
    frame_headers->sub_tlv_size      = frame_headers->tlv_size + 
                                       sizeof(uint8_t) + sizeof(uint16_t) + 
                                       sizeof(uint64_t);
    frame_headers->tlv_size          = sizeof(uint8_t) + sizeof(uint16_t) +
                                       sizeof(uint32_t);
    frame_headers->tx_buffer         = (uint8_t*)calloc(1, F_SIZE_MAX);
    frame_headers->vlan_dei          = 0;
    frame_headers->vlan_id           = VLAN_ID_DEF;


    pmtu_test            = mtu_test;
    mtu_test->enabled    = false;
    mtu_test->largest    = 0;
    mtu_test->mtu_tx_min = 1400;
    mtu_test->mtu_tx_max = 1500;


    pqm_test                  = qm_test;
    qm_test->enabled          = false;
    qm_test->delay_test_count = 10000;
    qm_test->interval         = 1000;
    qm_test->interval_sec     = 0;
    qm_test->interval_nsec    = 0;
    qm_test->interval_min     = 99999.99999;
    qm_test->interval_max     = 0.0;
    qm_test->jitter_min       = 999999.999999;
    qm_test->jitter_max       = 0.0;
    qm_test->rtt_min          = 999999.999999;
    qm_test->rtt_max          = 0.0;
    qm_test->test_count       = 0;
    qm_test->time_tx_1        = NULL;
    qm_test->time_tx_2        = NULL;
    qm_test->time_rx_1        = NULL;
    qm_test->time_rx_2        = NULL;
    qm_test->time_tx_diff     = NULL;
    qm_test->time_rx_diff     = NULL;
    qm_test->timeout          = 1000;
    qm_test->timeout_nsec     = 0;
    qm_test->timeout_sec      = 0;
    qm_test->timeout_count    = 0;
    qm_test->delay_results    = (double*)calloc(qm_test->delay_test_count,
                                                sizeof(double));


    pspeed_test                 = speed_test;
    speed_test->enabled         = false;
    speed_test->b_rx            = 0;
    speed_test->b_rx_prev       = 0;
    speed_test->b_tx            = 0;
    speed_test->b_tx_prev       = 0;
    speed_test->b_tx_speed_max  = B_TX_SPEED_MAX_DEF;
    speed_test->b_tx_speed_prev = 0;
    speed_test->f_payload       = (uint8_t*)calloc(1, F_SIZE_MAX);
    speed_test->f_payload_size  = 0;
    speed_test->f_index_prev    = 0;
    speed_test->f_speed         = 0;
    speed_test->f_speed_avg     = 0;
    speed_test->f_speed_max     = 0;
    speed_test->b_speed         = 0;
    speed_test->b_speed_max     = 0;
    speed_test->b_speed_avg     = 0;


    ptest_interface          = test_interface;
    test_interface->if_index = IF_INDEX_DEF;
    for (uint16_t i = 0; i<IFNAMSIZ; i += 1) {
        test_interface->if_name[i] = 0;
    }
    test_interface->sock_fd = SOCK_FD_DEF;


    ptest_params                 = test_params;
    test_params->f_ack           = false;
    test_params->f_ack_pending   = 0;
    test_params->f_bytes         = F_BYTES_DEF;
    test_params->f_count         = F_COUNT_DEF; 
    test_params->f_duration      = F_DURATION_DEF;
    test_params->f_rx_count      = 0;
    test_params->f_rx_count_prev = 0;
    test_params->f_rx_early      = 0;
    test_params->f_rx_late       = 0;
    test_params->f_rx_ontime     = 0;
    test_params->f_rx_other      = 0;
    test_params->f_tx_dly        = F_TX_DLY_DEF;
    test_params->f_tx_count      = 0;
    test_params->f_tx_count_max  = F_TX_COUNT_MAX_DEF;
    test_params->f_tx_count_prev = 0;
    test_params->f_size          = F_SIZE_DEF;
    test_params->f_size_total    = F_SIZE_DEF + frame_headers->length;
    test_params->f_waiting_ack   = false;
    test_params->s_elapsed       = 0;
    
}



int16_t setup_frame(struct app_params *app_params,
                    struct frame_headers *frame_headers,
                    struct test_interface *test_interface,
                    struct test_params *test_params)
{

    printf("Source MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           frame_headers->src_mac[0],frame_headers->src_mac[1],
           frame_headers->src_mac[2],frame_headers->src_mac[3],
           frame_headers->src_mac[4],frame_headers->src_mac[5]);

    printf("Destination MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           frame_headers->dst_mac[0],frame_headers->dst_mac[1],
           frame_headers->dst_mac[2],frame_headers->dst_mac[3],
           frame_headers->dst_mac[4],frame_headers->dst_mac[5]);

    // Fill the test frame buffer with random data
    for (uint32_t i = 0; i < (uint16_t)(F_SIZE_MAX-frame_headers->length); i += 1)
    {
        frame_headers->tx_data[i] = (uint8_t)((255.0*rand()/(RAND_MAX+1.0)));
    }

    if (app_params->broadcast == true) {

        // Broadcast to populate any switch/MAC tables
        int16_t BROAD_RET_VAL = broadcast_etherate(frame_headers, test_interface);
        if (BROAD_RET_VAL != 0) return BROAD_RET_VAL;

    }
    
    build_headers(frame_headers);

    return EXIT_SUCCESS;

}



int16_t setup_socket(struct test_interface *test_interface)
{

    test_interface->sock_fd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_ALL)); ///// Was SOCK_RAW

    if (test_interface->sock_fd < 0 )
    {
      perror("Error opening socket ");
      return EX_SOFTWARE;
    }

    #if LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)
            
        static const int32_t sock_qdisc_bypass = 1;
        int32_t sock_qdisc_ret = setsockopt(test_interface->sock_fd, SOL_PACKET, PACKET_QDISC_BYPASS, &sock_qdisc_bypass, sizeof(sock_qdisc_bypass));

        if (sock_qdisc_ret == -1) {
            perror("Error enabling QDISC bypass on socket ");
            return EXIT_FAILURE;
        }

    #endif

    return EXIT_SUCCESS;
}



int16_t setup_socket_interface(struct frame_headers *frame_headers,
                               struct test_interface *test_interface,
                               struct test_params *test_params)
{


    // If the user has supplied an interface index try to use that
    if (test_interface->if_index != IF_INDEX_DEF) {

        test_interface->if_index = set_sock_interface_index(test_interface);
        if (test_interface->if_index <= 0)
        {
            printf("Error: Couldn't set interface with index, "
                   "returned index was %" PRId32 "!\n", test_interface->if_index);
            return EX_SOFTWARE;
        }

    // Or if the user has supplied an interface name try to use that        
    } else if (strcmp((char*)test_interface->if_name, "") != 0) {

        test_interface->if_index = set_sock_interface_name(test_interface);
        if (test_interface->if_index <= 0)
        {
            printf("Error: Couldn't set interface index from name, "
                   "returned index was %" PRId32 "!\n", test_interface->if_index);
            return EX_SOFTWARE;
        }

    // Otherwise, try and best guess an interface
    } else if (test_interface->if_index == IF_INDEX_DEF) {

        test_interface->if_index = get_sock_interface(test_interface);
        if (test_interface->if_index <= 0)
        {
            printf("Error: Couldn't find appropriate interface ID, "
                  "returned ID was %" PRId32 "!\n", test_interface->if_index);
            return EX_SOFTWARE;
        }

    }


    // Link layer socket setup
    test_interface->sock_addr.sll_family   = AF_PACKET;    
    test_interface->sock_addr.sll_protocol = htons(ETH_P_ALL);
    test_interface->sock_addr.sll_ifindex  = test_interface->if_index;    
    test_interface->sock_addr.sll_hatype   = ARPHRD_ETHER;
    test_interface->sock_addr.sll_pkttype  = PACKET_OTHERHOST;
    test_interface->sock_addr.sll_halen    = ETH_ALEN;        
    test_interface->sock_addr.sll_addr[0]  = frame_headers->dst_mac[0];        
    test_interface->sock_addr.sll_addr[1]  = frame_headers->dst_mac[1];
    test_interface->sock_addr.sll_addr[2]  = frame_headers->dst_mac[2];
    test_interface->sock_addr.sll_addr[3]  = frame_headers->dst_mac[3];
    test_interface->sock_addr.sll_addr[4]  = frame_headers->dst_mac[4];
    test_interface->sock_addr.sll_addr[5]  = frame_headers->dst_mac[5];
    test_interface->sock_addr.sll_addr[6]  = 0x00;
    test_interface->sock_addr.sll_addr[7]  = 0x00;

    // Bind the socket to the physical interface to remove duplicate frames
    // when VLAN tagged sub-interfaces are present
    int32_t sock_bind = bind(test_interface->sock_fd,
                             (struct sockaddr *)&test_interface->sock_addr,
                             sizeof(test_interface->sock_addr));

    if (sock_bind == -1) {
        perror("Can't bind socket to interface ");
        return EXIT_FAILURE;
    }

 
    build_headers(frame_headers);

    return EXIT_SUCCESS;

}