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
 * File: Etherate Gerneral Functions
 *
 * File Contents:
 * int16_t broadcast_etherate()
 * void build_headers()
 * void build_tlv()
 * void build_sub_tlv()
 * int16_t cli_args()
 * uint8_t explode_char()
 * int32_t get_interface_mtu_by_name()
 * int16_t get_sock_interface()
 * void list_interfaces()
 * void print_usage()
 * int16_t remove_interface_promisc()
 * void reset_app()
 * int16_t set_int_promisc()
 * int16_t set_sock_interface_index()
 * int16_t set_sock_interface_name()
 * void signal_handler()
 * void sync_settings()
 * void update_frame_size()
 *
 */



#include "functions.h"

int16_t broadcast_etherate(struct etherate *eth)
{

    uint8_t tmp_mac[6] = {
    eth->frm.dst_mac[0],
    eth->frm.dst_mac[1],
    eth->frm.dst_mac[2],
    eth->frm.dst_mac[3],
    eth->frm.dst_mac[4],
    eth->frm.dst_mac[5],
    };

    eth->frm.dst_mac[0] = 0xFF;
    eth->frm.dst_mac[1] = 0xFF;
    eth->frm.dst_mac[2] = 0xFF;
    eth->frm.dst_mac[3] = 0xFF;
    eth->frm.dst_mac[4] = 0xFF;
    eth->frm.dst_mac[5] = 0xFF;

    // Rebuild frame headers with destination MAC
    build_headers(&eth->frm);

    build_tlv(&eth->frm, htons(TYPE_BROADCAST), htonl(VALUE_PRESENCE));

    // Build a dummy sub-TLV to align the buffers and pointers
    build_sub_tlv(&eth->frm, htons(TYPE_APPLICATION_SUB_TLV),
                  htonll(VALUE_DUMMY));

    int16_t tx_ret = 0;

    printf("Sending gratuitous broadcasts...\n");
    for (uint8_t i = 1; i <= 3; i += 1)
    {

        tx_ret = sendto(eth->intf.sock_fd,
                        eth->frm.tx_buffer,
                        eth->frm.length + eth->frm.tlv_size,
                        0, 
                        (struct sockaddr*)&eth->intf.sock_addr,
                        sizeof(eth->intf.sock_addr));

        if (tx_ret <= 0)
        {
            perror("Broadcast failed ");
            return tx_ret;
        }

        sleep(1);

    }

    printf("Done.\n");


    // Restore the original destination MAC
    eth->frm.dst_mac[0] = tmp_mac[0];
    eth->frm.dst_mac[1] = tmp_mac[1];
    eth->frm.dst_mac[2] = tmp_mac[2];
    eth->frm.dst_mac[3] = tmp_mac[3];
    eth->frm.dst_mac[4] = tmp_mac[4];
    eth->frm.dst_mac[5] = tmp_mac[5];

    return EXIT_SUCCESS;

}



void build_headers(struct frm *frm)
{

    uint16_t mpls_offset  = 0;
    uint16_t eth_offset   = 0;
    uint32_t mpls_hdr     = 0;
    uint16_t tpi          = 0;
    uint16_t tci          = 0;
    uint16_t *ptpi        = &tpi;
    uint16_t *ptci        = &tci;
    uint16_t vlan_id_tmp;


    // Check if transporting over MPLS
    if (frm->mpls_labels > 0) {

        // Copy the destination and source LSR MAC addresses
        memcpy((void*)frm->tx_buffer,
               (void*)frm->lsp_dst_mac, ETH_ALEN);

        mpls_offset+=ETH_ALEN;

        memcpy((void*)(frm->tx_buffer + mpls_offset),
               (void*)frm->lsp_src_mac, ETH_ALEN);

        mpls_offset+=ETH_ALEN;

        // Push on the ethertype for unicast MPLS
        tpi = htons(0x8847);
        memcpy((void*)(frm->tx_buffer + mpls_offset), ptpi, sizeof(tpi));
        mpls_offset+=sizeof(tpi);

        // For each MPLS label copy it onto the stack
        uint8_t i = 0;
        while (i < frm->mpls_labels) {

            mpls_hdr = (frm->mpls_label[i] & 0xFFFFF) << 12;
            mpls_hdr = mpls_hdr | (frm->mpls_exp[i] & 0x07) << 9;

            // MPLS BoS bit
            if (i == frm->mpls_labels - 1) {
                mpls_hdr = mpls_hdr | 1 << 8;
            } else {
                mpls_hdr = mpls_hdr | 0 << 8;
            }

            mpls_hdr = mpls_hdr | (frm->mpls_ttl[i] & 0xFF);

            mpls_hdr = htonl(mpls_hdr);

            memcpy((void*)(frm->tx_buffer + mpls_offset),
                   &mpls_hdr, sizeof(mpls_hdr));

            mpls_offset+=sizeof(mpls_hdr);

            mpls_hdr = 0;

            i += 1;

        }

        // Check if we need to push on a pseudowire control word
        if (frm->pwe_ctrl_word == 1){
            memset((void*)(frm->tx_buffer + mpls_offset), 0, 4);
            mpls_offset += 4;
        }

    }

    eth_offset += mpls_offset;

    // Copy the destination and source MAC addresses
    memcpy((void*)(frm->tx_buffer + eth_offset),
           (void*)frm->dst_mac, ETH_ALEN);

    eth_offset += ETH_ALEN;

    memcpy((void*)(frm->tx_buffer + eth_offset),
           (void*)frm->src_mac, ETH_ALEN);

    eth_offset += ETH_ALEN;

    // Check if QinQ VLAN ID has been supplied
    if (frm->qinq_pcp != QINQ_PCP_DEF ||
        frm->qinq_id != QINQ_ID_DEF)
    {

        // Add on the QinQ Tag Protocol Identifier
        // 0x88a8 == IEEE802.1ad, 0x9100 == older IEEE802.1QinQ
        tpi = htons(0x88a8);
        memcpy((void*)(frm->tx_buffer + eth_offset),
               ptpi, sizeof(tpi));

        eth_offset += sizeof(tpi);


        // Build the QinQ Tag Control Identifier...

        // pcp value
        vlan_id_tmp = frm->qinq_id;
        tci = (frm->qinq_pcp & 0x07) << 5;

        // DEI value
        if (frm->qinq_dei == 1)
        {
            tci = tci | (1 << 4);
        }

        // VLAN ID, first 4 bits
        frm->qinq_id = frm->qinq_id >> 8;
        tci = tci | (frm->qinq_id & 0x0f);

        // VLAN ID, last 8 bits
        frm->qinq_id = vlan_id_tmp;
        frm->qinq_id = frm->qinq_id << 8;
        tci = tci | (frm->qinq_id & 0xffff);
        frm->qinq_id = vlan_id_tmp;

        memcpy((void*)(frm->tx_buffer + eth_offset),
               ptci, sizeof(tci));

        eth_offset += sizeof(tci);

        // If an outer VLAN ID has been set, but not an inner one
        // (assume to be a mistake) set it to 1 so the frame is still valid
        if (frm->vlan_id == 0) frm->vlan_id = 1;

    }

    // Check to see if an inner VLAN pcp value or VLAN ID has been supplied
    if (frm->pcp != PCP_DEF || frm->vlan_id != VLAN_ID_DEF)
    {

        tpi = htons(0x8100);
        memcpy((void*)(frm->tx_buffer + eth_offset),
               ptpi, sizeof(tpi));

        eth_offset += sizeof(tpi);


        // Build the inner VLAN tci...

        // pcp value
        vlan_id_tmp = frm->vlan_id;
        tci = (frm->pcp & 0x07) << 5;

        // DEI value
        if (frm->vlan_dei==1)
        {
            tci = tci | (1 << 4);
        }

        // VLAN ID, first 4 bits
        frm->vlan_id = frm->vlan_id >> 8;
        tci = tci | (frm->vlan_id & 0x0f);

        // VLAN ID, last 8 bits
        frm->vlan_id = vlan_id_tmp;
        frm->vlan_id = frm->vlan_id << 8;
        tci = tci | (frm->vlan_id & 0xffff);
        frm->vlan_id = vlan_id_tmp;

        memcpy((void*)(frm->tx_buffer + eth_offset),
               ptci, sizeof(tci));

        eth_offset += sizeof(tci);

    }

    // Push on the ethertype for the Etherate payload
    tpi = htons(frm->etype);

    memcpy((void*)(frm->tx_buffer + eth_offset),
           ptpi, sizeof(tpi));

    eth_offset += sizeof(tpi);

    frm->length = eth_offset;


    // Pointers to the payload section of the frame
    frm->tx_data = frm->tx_buffer + frm->length;

    frm->rx_data = frm->rx_buffer + frm->length;

    /* When receiving a VLAN tagged frame (one or more VLAN tags) the Linux
     * Kernel is stripping off the outer most VLAN, so for the RX buffer the
     * data starts 4 bytes earlier, see this post:
     * http://stackoverflow.com/questions/24355597/linux-when-sending-ethernet-
     * frames-the-ethertype-is-being-re-written
     */
    if (frm->vlan_id != VLAN_ID_DEF ||
        frm->qinq_id != QINQ_ID_DEF) {
        frm->rx_data -= 4;
    }

}


#if defined(NOINLINE)
void build_tlv(struct frm *frm, uint16_t TLV_TYPE,
                      uint32_t TLV_VALUE)
{

    uint8_t *buffer_offset = frm->tx_data;

    (void) memcpy(buffer_offset, &TLV_TYPE, sizeof(TLV_TYPE));
    buffer_offset += sizeof(TLV_TYPE);

    *buffer_offset++ = sizeof(TLV_VALUE);

    (void) memcpy(buffer_offset, &TLV_VALUE, sizeof(TLV_VALUE));

    frm->rx_tlv_type  = (uint16_t*) frm->rx_data;

    frm->rx_tlv_value = (uint32_t*) (frm->rx_data + sizeof(TLV_TYPE) +
                                     sizeof(uint8_t));

}



void build_sub_tlv(struct frm *frm, uint16_t SUB_TLV_TYPE,
                   uint64_t SUB_TLV_VALUE)
{

    uint8_t *buffer_offset = frm->tx_data + frm->tlv_size;

    (void) memcpy(buffer_offset, &SUB_TLV_TYPE, sizeof(SUB_TLV_TYPE));
    buffer_offset += sizeof(SUB_TLV_TYPE);

    *buffer_offset++ = sizeof(SUB_TLV_VALUE);

    (void) memcpy(buffer_offset, &SUB_TLV_VALUE, sizeof(SUB_TLV_VALUE));

    frm->sub_tlv_val = (uint64_t*)buffer_offset;

    frm->rx_sub_tlv_type  = (uint16_t*) (frm->rx_data + frm->tlv_size);

    frm->rx_sub_tlv_value = (uint64_t*) (frm->rx_data + frm->tlv_size +
                                         sizeof(SUB_TLV_TYPE) + sizeof(uint8_t));
}
#endif


int16_t cli_args(int argc, char *argv[], struct etherate *eth)
{

    if (argc > 1) 
    {

        for (uint16_t i = 1; i < argc; i += 1) 
        {

            // Change to receive mode
            if (strncmp(argv[i], "-r" ,2)==0) 
            {
                eth->app.tx_mode = false;

                eth->frm.src_mac[0] = 0x00;
                eth->frm.src_mac[1] = 0x00;
                eth->frm.src_mac[2] = 0x5E;
                eth->frm.src_mac[3] = 0x00;
                eth->frm.src_mac[4] = 0x00;
                eth->frm.src_mac[5] = 0x02;

                eth->frm.dst_mac[0]   = 0x00;
                eth->frm.dst_mac[1]   = 0x00;
                eth->frm.dst_mac[2]   = 0x5E;
                eth->frm.dst_mac[3]   = 0x00;
                eth->frm.dst_mac[4]   = 0x00;
                eth->frm.dst_mac[5]   = 0x01;


            // Specifying a custom destination MAC address
            } else if (strncmp(argv[i], "-d", 2)==0) {
                uint8_t count;
                char    *tokens[6];
                char    delim[] = ":";

                count = explode_char(argv[i+1], delim, tokens);
                if (count == 6) {
                    for (uint8_t j = 0; j < 6; j += 1)
                    {
                        eth->frm.dst_mac[j] = (uint8_t)strtoul(tokens[j], NULL, 16);
                    }

                } else {
                    printf("Error: Invalid destination MAC address!\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }
                i += 1;


            // Disable settings sync between TX and RX
            } else if (strncmp(argv[i], "-g", 2)==0) {
                eth->app.tx_sync = false;


            // Disable the TX to RX delay check
            } else if (strncmp(argv[i], "-G", 2)==0) {
                eth->app.tx_delay = 0;


            // Specifying a custom source MAC address
            } else if (strncmp(argv[i], "-s" ,2)==0) {
                uint8_t count;
                char    *tokens[6];
                char    delim[] = ":";

                count = explode_char(argv[i+1], delim, tokens);
                if (count == 6) {
                    for (uint8_t j = 0; j < 6; j += 1)
                    {
                        eth->frm.src_mac[j] = (uint8_t)strtoul(tokens[j], NULL, 16);
                    }

                } else {
                    printf("Error: Invalid source MAC address!\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }
                i += 1;


            // Specifying a custom interface name
            } else if (strncmp(argv[i], "-i", 2)==0) {
                if (argc > (i+1))
                {
                    strncpy((char*)eth->intf.if_name, argv[i+1],
                            sizeof(eth->intf.if_name));
                    i += 1;
                } else {
                    printf("Oops! Missing interface name\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }


            // Specifying a custom interface index
            } else if (strncmp(argv[i], "-I", 2)==0) {
                if (argc > (i+1))
                {
                    eth->intf.if_index = (int)strtoul(argv[i+1], NULL, 0);
                    i += 1;
                } else {
                    printf("Oops! Missing interface index\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }


            // Requesting to list interfaces
            } else if (strncmp(argv[i], "-l", 2)==0) {
                list_interfaces();
                return RET_EXIT_APP;


            // Specifying a custom frame payload size in bytes
            } else if (strncmp(argv[i], "-f", 2)==0) {
                if (argc > (i+1))
                {
                    eth->params.f_size = (uint32_t)strtoul(argv[i+1], NULL, 0);
                    if (eth->params.f_size > 1500)
                    {
                        printf("WARNING: Make sure your device supports baby "
                               "giants or jumbo frames as required\n");
                    }
                    if (eth->params.f_size < 46) {
                        printf("WARNING: Minimum ethernet payload is 46 bytes, "
                               "Linux may pad the frame out to 46 bytes\n");
                        if (eth->params.f_size < (eth->frm.sub_tlv_size))
                            eth->params.f_size = eth->frm.sub_tlv_size;
                    }
                    
                    i += 1;

                } else {
                    printf("Oops! Missing frame size\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }


            // Limit TX rate using a frame pacing interval
            } else if (strncmp(argv[i], "-T", 2)==0) {
                if (argc > (i+1))
                {
                    eth->params.f_tx_dly = strtoul(argv[i+1], NULL, 0);
                    i += 1;
                } else {
                    printf("Oops! Missing max TX rate\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }


            // Specifying a custom transmission duration in seconds
            } else if (strncmp(argv[i], "-t", 2)==0) {
                if (argc > (i+1))
                {
                    eth->params.f_duration = strtoull(argv[i+1], NULL, 0);
                    i += 1;
                } else {
                    printf("Oops! Missing transmission duration\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }


            // Specifying the total number of frames to send instead of duration
            } else if (strncmp(argv[i], "-c", 2)==0) {
                if (argc > (i+1))
                {
                    eth->params.f_count = strtoull(argv[i+1], NULL, 0);
                    i += 1;
                } else {
                    printf("Oops! Missing max frame count\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }


            // Load a custom frame from file for TX
            } else if (strncmp(argv[i], "-C", 2)==0) {
                if (argc > (i+1))
                {
                    FILE* f_payload = fopen(argv[i+1], "r");
                    if (f_payload==NULL){
                        printf("Opps! File loading error!\n");
                        return EX_SOFTWARE;
                    }

                    int file_ret = 0;
                    eth->speed_test.f_payload_size = 0;
                    while (file_ret != EOF &&
                          (eth->speed_test.f_payload_size < F_SIZE_MAX)) {

                        file_ret = fscanf(f_payload, "%hhx",
                                          eth->speed_test.f_payload + eth->speed_test.f_payload_size);

                        if (file_ret == EOF)
                            break;

                        eth->speed_test.f_payload_size += 1;
                    }
                    if (fclose(f_payload) != 0)
                    {
                        perror("Error closing file ");
                    }

                    printf("Using custom frame (%" PRIu16 " octets loaded)\n",
                            eth->speed_test.f_payload_size);

                    // Disable initial broadcast
                    eth->app.broadcast = false;
                    // Disable settings sync
                    eth->app.tx_sync   = false;
                    // Disable delay calculation
                    eth->app.tx_delay  = false;

                    i += 1;
                } else {
                    printf("Oops! Missing filename\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }


            // Specifying the total number of bytes to send instead of duration
            } else if (strncmp(argv[i], "-b", 2)==0) {
                if (argc > (i+1))
                {
                    eth->params.f_bytes = strtoull(argv[i+1], NULL, 0);
                    i += 1;
                } else {
                    printf("Oops! Missing max byte transfer limit\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }


            // Enable ACK mode when testing
            } else if (strncmp(argv[i], "-a", 2)==0) {
                if (argc > (i+2))
                {
                    eth->params.f_ack = true;
                    eth->params.f_ack_timeout = strtoul(argv[i+1], NULL, 0);
                    eth->params.f_ack_count = strtoul(argv[i+2], NULL, 0);
                    i += 2;
                } else {
                    printf("Oops! Missing timeout and ACK frame count\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }
                


            // Specify the max frames per second rate
            } else if (strncmp(argv[i], "-F", 2)==0) {
                if (argc > (i+1))
                {
                    eth->params.f_tx_count_max = strtoul(argv[i+1], NULL, 0);
                    i += 1;
                } else {
                    printf("Oops! Missing max frame rate\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }


            // Limit TX rate to max bytes per second
            } else if (strncmp(argv[i], "-m", 2)==0) {
                if (argc > (i+1))
                {
                    eth->speed_test.b_tx_speed_max = strtoull(argv[i+1], NULL, 0);
                    i += 1;
                } else {
                    printf("Oops! Missing max TX rate\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }


            // Limit TX rate to max bits per second
            } else if (strncmp(argv[i], "-M", 2)==0) {
                if (argc > (i+1))
                {
                    eth->speed_test.b_tx_speed_max = (uint64_t)floor(strtoull(argv[i+1], NULL, 0) / 8);
                    i += 1;
                } else {
                    printf("Oops! Missing max TX rate\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }


            // Set 802.1q VLAN ID
            } else if (strncmp(argv[i], "-v", 2)==0) {
                if (argc > (i+1))
                {
                    eth->frm.vlan_id = (uint16_t)atoi(argv[i+1]);
                    i += 1;
                } else {
                    printf("Oops! Missing 802.1p VLAN ID\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }


            // Set 802.1p pcp value
            } else if (strncmp(argv[i], "-p", 2)==0) {
                if (argc > (i+1))
                {
                    eth->frm.pcp = (uint16_t)atoi(argv[i+1]);
                    i += 1;
                } else {
                    printf("Oops! Missing 802.1p pcp value\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }


            // Set the DEI bit on the inner VLAN
            } else if (strncmp(argv[i], "-x", 2)==0) {
                eth->frm.vlan_dei = 1;


            // Set 802.1ad QinQ outer VLAN ID
            } else if (strncmp(argv[i], "-q", 2)==0) {
                if (argc > (i+1))
                {
                    eth->frm.qinq_id = (uint16_t)atoi(argv[i+1]);
                    i += 1;
                } else {
                    printf("Oops! Missing 802.1ad QinQ outer VLAN ID\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }


            // Set 802.1ad QinQ outer pcp value
            } else if (strncmp(argv[i], "-o", 2)==0) {
                if (argc > (i+1))
                {
                    eth->frm.qinq_pcp = (uint16_t)atoi(argv[i+1]);
                    i += 1;
                } else {
                    printf("Oops! Missing 802.1ad QinQ outer pcp value\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }


            // Set the DEI bit on the outer VLAN
            } else if (strncmp(argv[i], "-X", 2)==0) {
                eth->frm.qinq_dei = 1;


            // Set a custom ethertype
            } else if (strncmp(argv[i], "-e", 2)==0) {
                if (argc > (i+1))
                {
                    eth->frm.etype = (uint16_t)strtol(argv[i+1], NULL, 16);
                    i += 1;
                } else {
                    printf("Oops! Missing ethertype value\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }


            // Destination and source MAC for next-hop MPLS LSR
            } else if (strncmp(argv[i], "-D", 2)==0) {
                uint8_t count;
                char    *tokens[6];
                char    delim[] = ":";

                count = explode_char(argv[i+1], delim, tokens);
                if (count == 6) {
                    for (uint8_t j = 0; j < 6; j += 1)
                    {
                        eth->frm.lsp_dst_mac[j] = (uint8_t)strtoul(tokens[j], NULL, 16);
                    }

                } else {
                    printf("Error: Invalid destination LSR MAC address!\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }
                i += 1;

                count = explode_char(argv[i+1], delim, tokens);
                if (count == 6) {
                    for (uint8_t j = 0; j < 6; j += 1)
                    {
                        eth->frm.lsp_src_mac[j] = (uint8_t)strtoul(tokens[j], NULL, 16);
                    }

                } else {
                    printf("Error: Invalid source LSR MAC address!\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }
                i += 1;


            // Push an MPLS label onto the stack
            } else if (strncmp(argv[i], "-L", 2)==0) {
                if (argc > (i+3))
                {
                    if (eth->frm.mpls_labels < MPLS_LABELS_MAX) {

                        if ((uint32_t)atoi(argv[i+1]) > 1048575) {
                            printf("Oops! MPLS label higher than 1,048,575\n");
                            return RET_EXIT_FAILURE;
                        }

                        if ((uint16_t)atoi(argv[i+2]) > 7) {
                            printf("Oops! MPLS EXP higher than 7\n");
                            return RET_EXIT_FAILURE;
                        }

                        if ((uint16_t)atoi(argv[i+3]) > 255) {
                            printf("Oops! MPLS TTL higher than 255\n");
                            return RET_EXIT_FAILURE;
                        }

                        eth->frm.mpls_label[eth->frm.mpls_labels] =
                            (uint32_t)atoi(argv[i+1]);
                        
                        eth->frm.mpls_exp[eth->frm.mpls_labels]   =
                            (uint16_t)atoi(argv[i+2]);
                        
                        eth->frm.mpls_ttl[eth->frm.mpls_labels]   =
                            (uint16_t)atoi(argv[i+3]);
                        
                        eth->frm.mpls_labels += 1;

                        i+=3;

                    } else {
                        printf("Oops! You have exceeded the maximum number of "
                               "MPLS labels (%" PRIu32 ")\n", MPLS_LABELS_MAX);
                        return RET_EXIT_FAILURE;
                    }
                } else {
                    printf("Oops! Missing MPLS label values\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }


            // Push pseudowire control word atop the label stack
            } else if (strncmp(argv[i], "-P", 2)==0) {
                eth->frm.pwe_ctrl_word = 1;


            // Enable the MTU sweep test
            } else if (strncmp(argv[i], "-U", 2)==0) {
                if (argc > (i+2))
                {
                    eth->mtu_test.mtu_tx_min = (uint16_t)atoi(argv[i+1]);
                    eth->mtu_test.mtu_tx_max = (uint16_t)atoi(argv[i+2]);

                    if (eth->mtu_test.mtu_tx_max > F_SIZE_MAX) { 
                        printf("MTU size can not exceed the maximum hard coded"
                               " size: %" PRIu32 "\n", F_SIZE_MAX);
                             return RET_EXIT_FAILURE;
                    }

                    eth->mtu_test.enabled = true;
                    i+=2;

                } else {
                    printf("Oops! Missing min/max MTU sizes\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }


            // Enable the link quality measurement tests
            } else if (strncmp(argv[i], "-Q", 2)==0) {
                if (argc > (i+2))
                {
                    eth->qm_test.interval = (uint32_t)atoi(argv[i+1]);
                    eth->qm_test.timeout =  (uint32_t)atoi(argv[i+2]);

                    if (eth->qm_test.timeout > eth->qm_test.interval) { 
                        printf("Oops! Echo timeout exceeded the interval\n");
                        return RET_EXIT_FAILURE;
                    }

                    // Convert to ns for use with timespec
                    eth->qm_test.interval_nsec = (eth->qm_test.interval * 1000000) % 1000000000;
                    eth->qm_test.interval_sec  = ((eth->qm_test.interval * 1000000) - eth->qm_test.interval_nsec) / 1000000000;
                    eth->qm_test.timeout_nsec  = (eth->qm_test.timeout * 1000000) % 1000000000;
                    eth->qm_test.timeout_sec   = ((eth->qm_test.timeout * 1000000) - eth->qm_test.timeout_nsec) / 1000000000;
                    eth->qm_test.enabled       = true;
                    i += 2;

                } else {
                    printf("Oops! Missing interval and timeout values\n"
                           "Usage info: %s -h|--h\n", argv[0]);
                    return RET_EXIT_FAILURE;
                }


            // Display version
            } else if (strncmp(argv[i], "-V", 2)==0 ||
                       strncmp(argv[i], "--version", 9)==0) {

                printf("Etherate version %s\n", APP_VERSION);
                return RET_EXIT_APP;


            // Display usage instructions
            } else if (strncmp(argv[i], "-h", 2)==0 ||
                       strncmp(argv[i], "--help", 6)==0) {

                print_usage(eth);
                return RET_EXIT_APP;


            // Else the user entered an invalid argument
            } else {
                    printf("Oops! Invalid argument %s\n"
                           "Usage info: %s -h|--h\n", argv[i], argv[0]);
                    return RET_EXIT_FAILURE;
            }


        }


    } 

    if (eth->app.tx_mode == true)
    {
        printf("Running in TX mode\n");
    } else {
        printf("Running in RX mode\n");
    }

    return EXIT_SUCCESS;

}



uint8_t explode_char(char *string, char *delim, char *tokens[])
{

    // Stole this function from somewhere, can't remember where,
    // either way, props to someone else ¯\_(ツ)_/¯
    uint8_t count = 0;
    char   *token;
    char *stringp = string;
    
    while ((token = strtok_r(stringp, delim, &stringp)))
    {
        tokens[count] = token;
        count += 1;
    }
    
    return count;

}



int32_t get_interface_mtu_by_name(struct intf *intf)
{

    struct ifreq ifr;
    strncpy(ifr.ifr_name, (char *)intf->if_name, sizeof(ifr.ifr_name));

    if (ioctl(intf->sock_fd, SIOCGIFMTU, &ifr)==0)
        return ifr.ifr_mtu;

    return RET_EXIT_FAILURE;

}



int16_t get_sock_interface(struct intf *intf)
{

    struct ifreq *ifr, *ifend;
    struct ifreq ifreq;
    struct ifconf ifc;
    struct ifreq ifs[MAX_IFS];

    ifc.ifc_len = sizeof(ifs);
    ifc.ifc_req = ifs;

    if (ioctl(intf->sock_fd, SIOCGIFCONF, &ifc) == -1)
    {
        perror("No compatible interfaces found ");
        return RET_EXIT_FAILURE;
    }

    // Loop over all interfaces
    ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));
    for (ifr = ifc.ifc_req; ifr < ifend; ifr += 1)
    {

        // Is this a packet capable device? (Doesn't work with AF_PACKET?)
        if (ifr->ifr_addr.sa_family == AF_INET)
        {

            // Try to get a typical ethernet adapter by name, not a bridge or virt interface
            if (strncmp(ifr->ifr_name, "eth",  3)==0 ||
                strncmp(ifr->ifr_name, "en",   2)==0 ||
                strncmp(ifr->ifr_name, "em",   3)==0 ||
                strncmp(ifr->ifr_name, "wlan", 4)==0 ||
                strncmp(ifr->ifr_name, "wlp",  3)==0 )
            {

                strncpy(ifreq.ifr_name, ifr->ifr_name, sizeof(ifreq.ifr_name));

                // Does this device even have hardware address?
                if (ioctl (intf->sock_fd, SIOCGIFHWADDR, &ifreq) == -1) break;

                // Copy MAC address before SIOCGIFINDEX
                char mac[6];
                memcpy(mac, ifreq.ifr_addr.sa_data, 6);

                if (ioctl(intf->sock_fd, SIOCGIFINDEX, &ifreq) == -1) break;

                int32_t ifindex = ifreq.ifr_ifindex;
                strncpy((char*)intf->if_name, ifreq.ifr_name, IFNAMSIZ);


                // Is this interface even up?
                if (ioctl(intf->sock_fd, SIOCGIFFLAGS, &ifreq) == -1) break;
                if (!(ifreq.ifr_flags & IFF_UP && ifreq.ifr_flags & IFF_RUNNING)) break;


                printf("Using device %s with address "
                       "%02x:%02x:%02x:%02x:%02x:%02x, interface index %" PRId32 "\n",
                       ifreq.ifr_name,
                       (uint8_t)mac[0],
                       (uint8_t)mac[1],
                       (uint8_t)mac[2],
                       (uint8_t)mac[3],
                       (uint8_t)mac[4],
                       (uint8_t)mac[5],
                       ifindex);

                return ifindex;

            } // If eth* || en* || em* || wlan*

        } // If AF_INET

    } // Looping through interface count

    return RET_EXIT_FAILURE;

}



void list_interfaces()
{

    struct ifreq ifreq;
    struct ifaddrs *ifaddr, *ifa;

    int32_t sock_fd;
    sock_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("Couldn't get interface list ");
        return;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {

        if (ifa->ifa_addr == NULL) {
            continue;
        }

        // Does this interface sub address family support AF_PACKET?
        if (ifa->ifa_addr->sa_family == AF_PACKET)
        {

            // Set the ifreq by interface name
            strncpy(ifreq.ifr_name,ifa->ifa_name,sizeof(ifreq.ifr_name));

            // Does this device have a hardware address?
            if (ioctl (sock_fd, SIOCGIFHWADDR, &ifreq) == 0)
            {

                // Copy the MAC before running SIOCGIFINDEX
                char mac[6];
                memcpy(mac, ifreq.ifr_addr.sa_data, 6);


                // Get the interface txqueuelen
                if (ioctl(sock_fd, SIOCGIFTXQLEN, &ifreq) == -1) {

                    perror("Couldn't get the interface txqueuelen ");
                    if (close(sock_fd) == -1) {
                        perror("Couldn't close socket ");
                    }
                    freeifaddrs(ifaddr);
                    return;

                }

                uint32_t txqueuelen = ifreq.ifr_qlen;


                // Get the interface index
                if (ioctl(sock_fd, SIOCGIFINDEX, &ifreq) == -1)
                {
                    perror("Couldn't get the interface index ");
                    if (close(sock_fd) == -1) {
                        perror("Couldn't close socket ");
                    }
                    freeifaddrs(ifaddr);
                    return;
                }

                printf("Device %s, "
                       "address %02" PRIx8 ":%02" PRIx8 ":%02" PRIx8 ":%02" PRIx8 ":%02" PRIx8 ":%02" PRIx8 ", "
                       "txqueuelen %" PRIu32 ", "
                       "has interface index %" PRId32 "\n",
                       ifreq.ifr_name,
                       (uint8_t)mac[0],
                       (uint8_t)mac[1],
                       (uint8_t)mac[2],
                       (uint8_t)mac[3],
                       (uint8_t)mac[4],
                       (uint8_t)mac[5],
                       txqueuelen,
                       ifreq.ifr_ifindex);

            }

        }

    }

    if (sock_fd > 0){
        if (close(sock_fd) == -1) {
            perror("Couldn't close socket ");
        }
    }
    freeifaddrs(ifaddr);

}



void print_usage (struct etherate *eth)
{

    printf ("Usage info; [Mode] [Destination] [Source] [Transport] [Additonal Tests] [Other]\n"
            "[Mode]\n"
            "\t-r\tChange to receive (listening) mode.\n"
            "\t\tDefault is transmit mode.\n"
            "[Destination]\n"
            "\t-d\tSpecify a custom desctination MAC address, \n"
            "\t\t-d 11:22:33:44:55:66. Without this the TX host\n"
            "\t\tdefaults to 00:00:5E:00:00:02 and the RX host\n"
            "\t\tdefaults to :01.\n"
            "\t-g\tTX host skips settings synchronisation with RX host\n"
            "\t\tand begins transmitting regardless of RX state.\n"
            "\t-G\tTX host skips delay calcuation towards RX host\n"
            "\t\tand begins transmitting regardless of RX state.\n"
            "[Source]\n"
            "\t-s\tSpecify a custom source MAC address,\n"
            "\t\t-s 11:22:33:44:55:66. Without this the TX host\n"
            "\t\tdefaults to 00:00:5E:00:00:01 and the RX host\n"
            "\t\tdefaults to 02.\n"
            "\t-i\tSet interface by name. Without this option we guess which\n"
            "\t\tinterface to use.\n"
            "\t-I\tSet interface by index. Without this option we guess which\n"
            "\t\tinterface to use.\n"
            "\t-l\tList interface indexes (then quit) for use with -I option.\n"
            "[Test Options]\n"
            "\t-a\tAck mode, acknowledge frames received back to the Tx host,\n"
            "\t\tspecify timeout in ms then number of frames to ack, -a 50 10\n"
            "\t-b\tLimit number of bytes to send, default is %" PRIu32 ", default behaviour\n"
            "\t\tis to wait for duration. Only one of -t, -c or -b can be used,\n"
            "\t\tboth override -t, -b overrides -c.\n"
            "\t-c\tLimit number of frames to send, default is %" PRIu32 ", default behaviour\n"
            "\t\tis to wait for duration.\n"
            "\t-C\tLoad a custom frame as hex from file\n"
            "\t\t-C ./tcp-syn.txt\n"
            "\t-e\tSet a custom ethertype, the default is 0x%04x.\n"
            "\t-f\tFrame payload size in bytes, default is %" PRIu32 "\n"
            "\t\t(%" PRIu32 " including SA+DA+Etype, excludes FCS).\n"
            "\t-F\tMax frames per/second to send, -F 83 (1Mbps).\n"
            "\t-m\tMax bytes per/second to send, -m 125000 (1Mps).\n"
            "\t-M\tMax bits per/second to send, -M 1000000 (1Mbps).\n"
            "\t-T\tMax bits per/second to send (frame pacing), -T 1000000 (1Mbps).\n"
            "\t-t\tTransmition duration, seconds as integer, default is %" PRIu32 ".\n"
            "[Transport]\n"
            "\t-v\tAdd an 802.1q VLAN tag. Not in the header by default.\n"
            "\t\tIf using a pcp value with -p a default VLAN of 0 is added.\n"
            "\t-p\tAdd an 802.1p pcp value from 1 to 7 using options -p 1 to\n"
            "\t\t-p 7. If more than one value is given, the highest is used.\n"
            "\t\tDefault is 0 if none specified.\n"
            "\t\t(If no 802.1q tag is set the VLAN 0 will be used).\n"
            "\t-x\tSet the DEI bit in the VLAN tag.\n"
            "\t-q\tAdd an outer Q-in-Q tag. If used without -v, 1 is used\n"
            "\t\tfor the inner VLAN ID.\n"
            "\t-o\tAdd an 802.1p pcp value to the outer Q-in-Q VLAN tag.\n"
            "\t\tIf no pcp value is specified and a Q-in-Q VLAN ID is,\n"
            "\t\t0 will be used. If no outer Q-in-Q VLAN ID is supplied this\n"
            "\t\toption is ignored. -o 1 to -o 7 like the -p option above.\n"
            "\t-X\tSet the DEI bit on the outer VLAN tag.\n"
            "\t-D\tSpecify a destination then source MAC for sending MPLS\n"
            "\t\ttagged frames to the next-hop LSR:\n"
            "\t\t-D 00:11:22:33:44:55 66:77:88:99:AA:BB\n"
            "\t-L\tPush an MPLS label onto the frame. The label, EXP value\n"
            "\t\tand TTL must be specified: -L 1122 0 255. Repeat the \n"
            "\t\toption to create a label stack from top to bottom. The last\n"
            "\t\tlabel will automatically have the bottom of stack bit set.\n"
            "\t-P\tPush a pseudowire control world on top of the MPLS\n"
            "\t\tlabel stack to emulate an Ethernet pseudowire.\n"
            "[Additonal Tests]\n"
            "\t-U\tSpecify a minimum and maximum MTU size in bytes then\n"
            "\t\tperform an MTU sweep on the link towards the RX host to\n"
            "\t\tfind the maximum size supported, -U 1400 1600\n"
            "\t-Q\tSpecify an echo interval and timeout value in millis\n"
            "\t\tthen measure link quality (RTT and jitter), using\n"
            "\t\t-Q 1000 1000 (default duration is %" PRIu32 ", see -t).\n"
            "[Other]\n"
            "\t-V|--version Display version\n"
            "\t-h|--help Display this help text\n",
            F_BYTES_DEF,
            F_COUNT_DEF,
            ETYPE_DEF,
            F_SIZE_DEF,
            (F_SIZE_DEF + eth->frm.length),
            F_DURATION_DEF,
            F_DURATION_DEF);

}



int16_t remove_interface_promisc(struct intf *intf)
{

    printf("Leaving promiscuous mode\n");

    struct ifreq ifreq;

    strncpy(ifreq.ifr_name, (char*)intf->if_name, IFNAMSIZ);

    if (ioctl(intf->sock_fd,SIOCGIFFLAGS, &ifreq) == -1)
    {
        perror("Error getting socket flags, leaving promiscuous mode failed ");
        return EX_SOFTWARE;
    }

    ifreq.ifr_flags &= ~IFF_PROMISC;

    if (ioctl(intf->sock_fd,SIOCSIFFLAGS, &ifreq) == -1)
    {
        perror("Error setting socket flags, leaving promiscuous mode failed ");
        return EX_SOFTWARE;
    }


    return EXIT_SUCCESS;

}



void reset_app(struct etherate *eth)
{

    if (eth->frm.rx_buffer != NULL)
        free(eth->frm.rx_buffer);

    if (eth->frm.tx_buffer != NULL)
        free(eth->frm.tx_buffer);

    if (eth->speed_test.f_payload != NULL)
        free(eth->speed_test.f_payload);

    if (eth->qm_test.delay_results)
        free(eth->qm_test.delay_results);
    
    if (eth->intf.sock_fd > 0) {
        if (close(eth->intf.sock_fd) == -1) {
            perror("Couldn't close socket ");
        }
    }

}



int16_t set_int_promisc(struct intf *intf)
{

    printf("Entering promiscuous mode\n");

    struct ifreq ifreq;

    strncpy(ifreq.ifr_name, (char*)intf->if_name, IFNAMSIZ);

    if (ioctl(intf->sock_fd, SIOCGIFFLAGS, &ifreq) == -1) 
    {
        perror("Getting socket flags failed when entering promiscuous mode ");
        return EX_SOFTWARE;
    }

    ifreq.ifr_flags |= IFF_PROMISC;

    if (ioctl(intf->sock_fd, SIOCSIFFLAGS, &ifreq) == -1)
    {
        perror("Setting socket flags failed when entering promiscuous mode ");
        return EX_SOFTWARE;

    }

    return EXIT_SUCCESS;

}



int16_t set_sock_interface_index(struct intf *intf)
{

    struct ifreq ifreq;
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("Couldn't get interface list ");
        return(RET_EXIT_FAILURE);
    }

    
    // Loop over all interfaces
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {

        if (ifa->ifa_addr == NULL) {
            continue;
        }

        // Does this interface sub address family support AF_PACKET
        if (ifa->ifa_addr->sa_family == AF_PACKET)
        {

            // Set the ifreq by interface name
            strncpy(ifreq.ifr_name, ifa->ifa_name, sizeof(ifreq.ifr_name));

            // Does this device have a hardware address?
            if (ioctl (intf->sock_fd, SIOCGIFHWADDR, &ifreq) == 0 )
            {

                // Copy the MAC before running SIOCGIFINDEX
                char mac[6];
                memcpy(mac, ifreq.ifr_addr.sa_data, 6);

                // Get the interface index
                if (ioctl(intf->sock_fd, SIOCGIFINDEX, &ifreq) == -1) break;

                int32_t ifindex = ifreq.ifr_ifindex;


                // Check if this is the interface index the user specified
                if (ifindex == intf->if_index)
                {

                    // Get the interface name
                    strncpy((char*)intf->if_name,
                            ifreq.ifr_name, IFNAMSIZ);


                    // Is this interface even up?
                    if (ioctl(intf->sock_fd, SIOCGIFFLAGS, &ifreq) == -1) break;
                    if (!(ifreq.ifr_flags & IFF_UP && ifreq.ifr_flags & IFF_RUNNING)) break;


                    printf("Using device %s with address "
                           "%02x:%02x:%02x:%02x:%02x:%02x, interface index %" PRId32 "\n",
                           ifreq.ifr_name,
                           (uint8_t)mac[0],
                           (uint8_t)mac[1],
                           (uint8_t)mac[2],
                           (uint8_t)mac[3],
                           (uint8_t)mac[4],
                           (uint8_t)mac[5],
                           ifindex);

                    freeifaddrs(ifaddr);

                    return ifindex;

                }

            }

        }

    }

    freeifaddrs(ifaddr);

    return RET_EXIT_FAILURE;

}



int16_t set_sock_interface_name(struct intf *intf)
{

    struct ifreq ifreq;
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("Couldn't get interface list ");
        return RET_EXIT_FAILURE;
    }

    // Loop over all interfaces
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {

        if (ifa->ifa_addr == NULL) {
            continue;
        }

        // Does this interface sub address family support AF_PACKET
        if (ifa->ifa_addr->sa_family == AF_PACKET)
        {

            // Set the ifreq by interface name
            strncpy(ifreq.ifr_name, ifa->ifa_name, sizeof(ifreq.ifr_name));

            // Does this device have a hardware address?
            if (ioctl (intf->sock_fd, SIOCGIFHWADDR, &ifreq) == 0)
            {

                // Copy the MAC before running SIOCGIFINDEX
                char mac[6];
                memcpy(mac, ifreq.ifr_addr.sa_data, 6);

                // Check if this is the interface name the user specified
                if (strcmp(ifreq.ifr_name, (char*)intf->if_name) == 0)
                {

                    // Get the interface index
                    if (ioctl(intf->sock_fd, SIOCGIFINDEX, &ifreq) == -1) break;

                    int32_t ifindex = ifreq.ifr_ifindex;


                    // Is this interface even up?
                    if (ioctl(intf->sock_fd, SIOCGIFFLAGS, &ifreq) == -1) break;
                    if (!(ifreq.ifr_flags & IFF_UP && ifreq.ifr_flags & IFF_RUNNING)) break;


                    printf("Using device %s with address "
                           "%02x:%02x:%02x:%02x:%02x:%02x, interface index %" PRId32 "\n",
                           ifreq.ifr_name,
                           (uint8_t)mac[0],
                           (uint8_t)mac[1],
                           (uint8_t)mac[2],
                           (uint8_t)mac[3],
                           (uint8_t)mac[4],
                           (uint8_t)mac[5],
                           ifindex);

                    freeifaddrs(ifaddr);

                    return ifindex;

                }

            }

        }

    }

    freeifaddrs(ifaddr);

    return RET_EXIT_FAILURE;

}



void signal_handler(int signal)
{

    printf("Quitting...\n");

    struct etherate *eth = eth_p;
    struct ifreq ifreq;

    // If a test was running, print the results so far
    if (eth->mtu_test.enabled) {

        mtu_sweep_test_results(eth);

    } else if (eth->qm_test.enabled) {

        latency_test_results(eth);

    } else if (eth->speed_test.enabled) {

        speed_test_results(eth);
    }

    // Send a dying gasp to the other host in case the application is ending
    // before the running test has finished

    build_tlv(&eth->frm, htons(TYPE_APPLICATION), htonl(VALUE_DYINGGASP));

    int16_t tx_ret = sendto(eth->intf.sock_fd,
                            eth->frm.tx_buffer,
                            eth->frm.length + eth->frm.tlv_size,
                            0,
                            (struct sockaddr*)&eth->intf.sock_addr,
                            sizeof(eth->intf.sock_addr));

    if (tx_ret <= 0)
    {
        perror("Dying gasp error ");
    }

    printf("Leaving promiscuous mode\n");

    strncpy(ifreq.ifr_name, (char*)eth->intf.if_name, IFNAMSIZ);

    if (ioctl(eth->intf.sock_fd, SIOCGIFFLAGS, &ifreq) == -1) {
        perror("Failed to get socket flags when leaving promiscuous mode ");
    }

    ifreq.ifr_flags &= ~IFF_PROMISC;

    if (ioctl(eth->intf.sock_fd, SIOCSIFFLAGS, &ifreq) == -1) {
        perror("Setting socket flags when leaving promiscuous mode ");
    }

    if (eth->intf.sock_fd > 0) { 
        if (close(eth->intf.sock_fd) != 0) {
            perror("Couldn't close file descriptor ");
        }
    }

    if (eth->frm.rx_buffer         != NULL) free (eth->frm.rx_buffer);
    if (eth->frm.tx_buffer         != NULL) free (eth->frm.tx_buffer);
    if (eth->qm_test.time_rx_1     != NULL) free(eth->qm_test.time_rx_1);
    if (eth->qm_test.time_rx_2     != NULL) free(eth->qm_test.time_rx_2);
    if (eth->qm_test.time_rx_diff  != NULL) free(eth->qm_test.time_rx_diff);
    if (eth->qm_test.time_tx_1     != NULL) free(eth->qm_test.time_tx_1);
    if (eth->qm_test.time_tx_2     != NULL) free(eth->qm_test.time_tx_2);
    if (eth->qm_test.time_tx_diff  != NULL) free(eth->qm_test.time_tx_diff);
    if (eth->qm_test.delay_results != NULL) free (eth->qm_test.delay_results);
    if (eth->speed_test.f_payload  != NULL) free (eth->speed_test.f_payload);

    exit(signal);

}



void sync_settings(struct etherate *eth)
{

    int16_t tx_ret;
    int16_t rx_len;

    build_tlv(&eth->frm, htons(TYPE_SETTING), htonl(VALUE_SETTING_SUB_TLV));

    // Send test settings to the RX host
    if (eth->app.tx_mode == true)
    {

        printf("\nSynchronising settings with RX host\n");

        // Disable the delay calculation
        if (eth->app.tx_delay != TX_DELAY_DEF)
        {

            build_sub_tlv(&eth->frm, htons(TYPE_TXDELAY),
                          htonll(eth->app.tx_delay));

            tx_ret = sendto(eth->intf.sock_fd,
                            eth->frm.tx_buffer,
                            eth->frm.length + eth->frm.sub_tlv_size,
                            0,
                            (struct sockaddr*)&eth->intf.sock_addr,
                            sizeof(eth->intf.sock_addr));

            if (tx_ret <= 0)
            {
                perror("Sync settings transmit error ");
                return;
            }


            printf("TX to RX delay calculation disabled\n");

        }

        // Testing with a custom ethertype
        if (eth->frm.etype != ETYPE_DEF)
        {

            build_sub_tlv(&eth->frm, htons(TYPE_ETYPE),
                          htonll(eth->frm.etype));

            tx_ret = sendto(eth->intf.sock_fd,
                            eth->frm.tx_buffer,
                            eth->frm.length + eth->frm.sub_tlv_size,
                            0,
                            (struct sockaddr*)&eth->intf.sock_addr,
                            sizeof(eth->intf.sock_addr));

            if (tx_ret <= 0)
            {
                perror("Sync settings transmit error ");
                return;
            }


            printf("Ethertype set to 0x%04hx\n", eth->frm.etype);

        }

        // Testing with a custom frame size
        if (eth->params.f_size != F_SIZE_DEF)
        {

            build_sub_tlv(&eth->frm, htons(TYPE_FRAMESIZE),
                          htonll(eth->params.f_size));

            tx_ret = sendto(eth->intf.sock_fd,
                            eth->frm.tx_buffer,
                            eth->frm.length + eth->frm.sub_tlv_size,
                            0,
                            (struct sockaddr*)&eth->intf.sock_addr,
                            sizeof(eth->intf.sock_addr));

            if (tx_ret <= 0)
            {
                perror("Sync settings transmit error ");
                return;
            }


            printf("Frame payload size set to %" PRIu16 "\n", eth->params.f_size);

        }

        // Testing with a custom duration
        if (eth->params.f_duration != F_DURATION_DEF)
        {

            build_sub_tlv(&eth->frm, htons(TYPE_DURATION),
                          htonll(eth->params.f_duration));

            tx_ret = sendto(eth->intf.sock_fd,
                            eth->frm.tx_buffer,
                            eth->frm.length + eth->frm.sub_tlv_size,
                            0,
                            (struct sockaddr*)&eth->intf.sock_addr,
                            sizeof(eth->intf.sock_addr));

            if (tx_ret <= 0)
            {
                perror("Sync settings transmit error ");
                return;
            }


            printf("Test duration set to %" PRIu64 "\n", eth->params.f_duration);

        }

        // Testing with a custom frame count
        if (eth->params.f_count != F_COUNT_DEF)
        {

            build_sub_tlv(&eth->frm, htons(TYPE_FRAMECOUNT),
                          htonll(eth->params.f_count));

            tx_ret = sendto(eth->intf.sock_fd,
                            eth->frm.tx_buffer,
                            eth->frm.length + eth->frm.sub_tlv_size,
                            0,
                            (struct sockaddr*)&eth->intf.sock_addr,
                            sizeof(eth->intf.sock_addr));

            if (tx_ret <= 0)
            {
                perror("Sync settings transmit error ");
                return;
            }


            printf("Frame count set to %" PRIu64 "\n", eth->params.f_count);

        }

        // Testing with a custom byte limit
        if (eth->params.f_bytes != F_BYTES_DEF)
        {

            build_sub_tlv(&eth->frm, htons(TYPE_BYTECOUNT),
                          htonll(eth->params.f_bytes));

            tx_ret = sendto(eth->intf.sock_fd,
                            eth->frm.tx_buffer,
                            eth->frm.length + eth->frm.sub_tlv_size,
                            0,
                            (struct sockaddr*)&eth->intf.sock_addr,
                            sizeof(eth->intf.sock_addr));

            if (tx_ret <= 0)
            {
                perror("Sync settings transmit error ");
                return;
            }


            printf("Byte limit set to %" PRIu64 "\n", eth->params.f_bytes);

        }

        // Testing with a custom max speed limit
        if (eth->speed_test.b_tx_speed_max != B_TX_SPEED_MAX_DEF)
        {

            build_sub_tlv(&eth->frm, htons(TYPE_MAXSPEED),
                          htonll(eth->speed_test.b_tx_speed_max));

            tx_ret = sendto(eth->intf.sock_fd,
                            eth->frm.tx_buffer,
                            eth->frm.length + eth->frm.sub_tlv_size,
                            0,
                            (struct sockaddr*)&eth->intf.sock_addr,
                            sizeof(eth->intf.sock_addr));

            if (tx_ret <= 0)
            {
                perror("Sync settings transmit error ");
                return;
            }


            printf("Max TX speed set to %.2fMbps\n",
                   (((double)eth->speed_test.b_tx_speed_max * 8) / 1000 / 1000));

        }

        // Testing with a custom inner VLAN pcp value
        if (eth->frm.pcp != PCP_DEF)
        {

            build_sub_tlv(&eth->frm, htons(TYPE_VLANPCP),
                          htonll(eth->frm.pcp));

            tx_ret = sendto(eth->intf.sock_fd,
                            eth->frm.tx_buffer,
                            eth->frm.length + eth->frm.sub_tlv_size,
                            0,
                            (struct sockaddr*)&eth->intf.sock_addr,
                            sizeof(eth->intf.sock_addr));

            if (tx_ret <= 0)
            {
                perror("Sync settings transmit error ");
                return;
            }


            printf("Inner VLAN pcp value set to %hu\n", eth->frm.pcp);

        }

        // Tesing with a custom QinQ pcp value
        if (eth->frm.qinq_pcp != QINQ_PCP_DEF)
        {

            build_sub_tlv(&eth->frm, htons(TYPE_QINQPCP),
                          htonll(eth->frm.qinq_pcp));

            tx_ret = sendto(eth->intf.sock_fd,
                            eth->frm.tx_buffer,
                            eth->frm.length + eth->frm.sub_tlv_size,
                            0,
                            (struct sockaddr*)&eth->intf.sock_addr,
                            sizeof(eth->intf.sock_addr));

            if (tx_ret <= 0)
            {
                perror("Sync settings transmit error ");
                return;
            }


            printf("QinQ VLAN pcp value set to %hu\n", eth->frm.qinq_pcp);

        }

        // Tell RX to run in ACK mode
        if (eth->params.f_ack == true)
        {

            build_sub_tlv(&eth->frm, htons(TYPE_ACKMODE),
                          htonll(eth->params.f_ack));

            tx_ret = sendto(eth->intf.sock_fd,
                            eth->frm.tx_buffer,
                            eth->frm.length + eth->frm.sub_tlv_size,
                            0,
                            (struct sockaddr*)&eth->intf.sock_addr,
                            sizeof(eth->intf.sock_addr));

            if (tx_ret <= 0)
            {
                perror("Sync settings transmit error ");
                return;
            }

            build_sub_tlv(&eth->frm, htons(TYPE_ACKTIMEOUT),
                          htonll(eth->params.f_ack_timeout));

            tx_ret = sendto(eth->intf.sock_fd,
                            eth->frm.tx_buffer,
                            eth->frm.length + eth->frm.sub_tlv_size,
                            0,
                            (struct sockaddr*)&eth->intf.sock_addr,
                            sizeof(eth->intf.sock_addr));

            if (tx_ret <= 0)
            {
                perror("Sync settings transmit error ");
                return;
            }


            build_sub_tlv(&eth->frm, htons(TYPE_ACKCOUNT),
                          htonll(eth->params.f_ack_count));

            tx_ret = sendto(eth->intf.sock_fd,
                            eth->frm.tx_buffer,
                            eth->frm.length + eth->frm.sub_tlv_size,
                            0,
                            (struct sockaddr*)&eth->intf.sock_addr,
                            sizeof(eth->intf.sock_addr));

            if (tx_ret <= 0)
            {
                perror("Sync settings transmit error ");
                return;
            }


            printf("ACK mode enabled\n");

        }

        // Tell RX an MTU sweep test will be performed
        if (eth->mtu_test.enabled)
        {

            build_sub_tlv(&eth->frm, htons(TYPE_MTUTEST),
                          htonll(eth->mtu_test.enabled));

            tx_ret = sendto(eth->intf.sock_fd,
                            eth->frm.tx_buffer,
                            eth->frm.length + eth->frm.sub_tlv_size,
                            0,
                            (struct sockaddr*)&eth->intf.sock_addr,
                            sizeof(eth->intf.sock_addr));

            if (tx_ret <= 0)
            {
                perror("Sync settings transmit error ");
                return;
            }


            build_sub_tlv(&eth->frm, htons(TYPE_MTUMIN),
                          htonll(eth->mtu_test.mtu_tx_min));

            tx_ret = sendto(eth->intf.sock_fd,
                            eth->frm.tx_buffer,
                            eth->frm.length + eth->frm.sub_tlv_size,
                            0,
                            (struct sockaddr*)&eth->intf.sock_addr,
                            sizeof(eth->intf.sock_addr));

            if (tx_ret <= 0)
            {
                perror("Sync settings transmit error ");
                return;
            }


            build_sub_tlv(&eth->frm, htons(TYPE_MTUMAX),
                          htonll(eth->mtu_test.mtu_tx_max));

            tx_ret = sendto(eth->intf.sock_fd,
                            eth->frm.tx_buffer,
                            eth->frm.length + eth->frm.sub_tlv_size,
                            0,
                            (struct sockaddr*)&eth->intf.sock_addr,
                            sizeof(eth->intf.sock_addr));

            if (tx_ret <= 0)
            {
                perror("Sync settings transmit error ");
                return;
            }


            printf("MTU sweep test enabled from %" PRIu16 " to %" PRIu16 "\n",
                   eth->mtu_test.mtu_tx_min, eth->mtu_test.mtu_tx_max);
        
        }

        // Tell Rx the link quality tests will be performed
        if (eth->qm_test.enabled)
        {

            build_sub_tlv(&eth->frm, htons(TYPE_QMTEST),
                          htonll(eth->qm_test.enabled));

            tx_ret = sendto(eth->intf.sock_fd,
                            eth->frm.tx_buffer,
                            eth->frm.length + eth->frm.sub_tlv_size,
                            0,
                            (struct sockaddr*)&eth->intf.sock_addr,
                            sizeof(eth->intf.sock_addr));

            if (tx_ret <= 0)
            {
                perror("Sync settings transmit error ");
                return;
            }


            build_sub_tlv(&eth->frm, htons(TYPE_QMINTERVAL),
                          htonll(eth->qm_test.interval));

            tx_ret = sendto(eth->intf.sock_fd,
                            eth->frm.tx_buffer,
                            eth->frm.length + eth->frm.sub_tlv_size,
                            0,
                            (struct sockaddr*)&eth->intf.sock_addr,
                            sizeof(eth->intf.sock_addr));

            if (tx_ret <= 0)
            {
                perror("Sync settings transmit error ");
                return;
            }


            build_sub_tlv(&eth->frm, htons(TYPE_QMTIMEOUT),
                          htonll(eth->qm_test.timeout));

            tx_ret = sendto(eth->intf.sock_fd,
                            eth->frm.tx_buffer,
                            eth->frm.length + eth->frm.sub_tlv_size,
                            0,
                            (struct sockaddr*)&eth->intf.sock_addr,
                            sizeof(eth->intf.sock_addr));

            if (tx_ret <= 0)
            {
                perror("Sync settings transmit error ");
                return;
            }


            printf("Link quality tests enabled\n");

        }

        // Let the receiver know all settings have been sent
        build_tlv(&eth->frm, htons(TYPE_SETTING),
                  htonl(VALUE_SETTING_END));

        tx_ret = sendto(eth->intf.sock_fd,
                        eth->frm.tx_buffer,
                        eth->frm.length + eth->frm.tlv_size,
                        0,
                        (struct sockaddr*)&eth->intf.sock_addr,
                        sizeof(eth->intf.sock_addr));

        if (tx_ret <= 0)
        {
            perror("Sync settings transmit error ");
            return;
        }


        printf("Settings have been synchronised\n\n");


    } else if (eth->app.tx_mode == false) {


        printf("\nWaiting for settings from TX host\n");

        // In listening mode RX waits for each parameter to come through,
        // Until TX signals all settings have been sent
        uint8_t WAITING = true;

        while (WAITING)
        {

            rx_len = recv(eth->intf.sock_fd,
                          eth->frm.rx_buffer,
                          eth->params.f_size_total,
                          0);

            if (rx_len >0)
            {


                // All settings have been received
                if (ntohs(*eth->frm.rx_tlv_type) == TYPE_SETTING &&
                    ntohl(*eth->frm.rx_tlv_value) == VALUE_SETTING_END) {

                    WAITING = false;
                    printf("Settings have been synchronised\n\n");

                // TX has disabled the TX to RX one way delay calculation
                } else if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_TXDELAY) {

                    eth->app.tx_delay = (uint8_t)ntohll(*eth->frm.rx_sub_tlv_value);
                    printf("TX to RX delay calculation disabled\n");

                // TX has sent a non-default ethertype
                } else if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_ETYPE) {

                    eth->frm.etype = (uint16_t)ntohll(*eth->frm.rx_sub_tlv_value);
                    printf("Ethertype set to 0x%04hx\n", eth->frm.etype);

                // TX has sent a non-default frame payload size
                } else if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_FRAMESIZE) {

                    eth->params.f_size = (uint16_t)ntohll(*eth->frm.rx_sub_tlv_value);
                    printf("Frame payload size set to %hu\n", eth->params.f_size);
                    update_frame_size(eth);

                // TX has sent a non-default transmition duration
                } else if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_DURATION) {

                    eth->params.f_duration = (uint64_t)ntohll(*eth->frm.rx_sub_tlv_value);
                    printf("Test duration set to %" PRIu64 "\n", eth->params.f_duration);

                // TX has sent a frame count to use instead of duration
                } else if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_FRAMECOUNT) {

                    eth->params.f_count = (uint64_t)ntohll(*eth->frm.rx_sub_tlv_value);
                    printf("Frame count set to %" PRIu64 "\n", eth->params.f_count);

                // TX has sent a total bytes value to use instead of frame count
                } else if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_BYTECOUNT) {

                    eth->params.f_bytes = (uint64_t)ntohll(*eth->frm.rx_sub_tlv_value);
                    printf("Byte limit set to %" PRIu64 "\n", eth->params.f_bytes);

                // TX speed is limited
                } else if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_MAXSPEED) {

                    eth->speed_test.b_tx_speed_max = (uint64_t)ntohll(*eth->frm.rx_sub_tlv_value);
                    printf("Max TX speed set to %.2fMbps\n", ((double)(eth->speed_test.b_tx_speed_max * 8) / 1000 / 1000));

                // TX has set a custom pcp value
                } else if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_VLANPCP) {

                    eth->frm.pcp = (uint16_t)ntohll(*eth->frm.rx_sub_tlv_value);
                    printf("pcp value set to %hu\n", eth->frm.pcp);

                // TX has set a custom QinQ pcp value
                } else if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_QINQPCP) {

                    eth->frm.qinq_pcp = (uint16_t)ntohll(*eth->frm.rx_sub_tlv_value);
                    printf("QinQ pcp value set to %hu\n", eth->frm.qinq_pcp);

                // TX has requested RX run in ACK mode
                } else if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_ACKMODE) {

                    eth->params.f_ack = true;
                    printf("ACK mode enabled\n");

                // TX has set ACK mode timeout
                } else if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_ACKTIMEOUT) {

                    eth->params.f_ack_timeout = (uint16_t)ntohll(*eth->frm.rx_sub_tlv_value);
                    printf("ACK mode timeout set to %" PRIu16"ms\n", eth->params.f_ack_timeout);

                // TX has set ACK mode frame count
                } else if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_ACKCOUNT) {

                    eth->params.f_ack_timeout = (uint32_t)ntohll(*eth->frm.rx_sub_tlv_value);
                    printf("ACK mode set to ACK every %" PRIu32 " frames\n", eth->params.f_ack_count);

                // TX has requested MTU sweep test
                } else if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_MTUTEST) {

                    eth->mtu_test.enabled = true;
                    printf("MTU sweep test enabled\n");

                // TX has set MTU sweep test minimum MTU size
                } else if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_MTUMIN) {

                    eth->mtu_test.mtu_tx_min = (uint16_t)ntohll(*eth->frm.rx_sub_tlv_value);
                    printf("Minimum MTU set to %" PRIu16 "\n", eth->mtu_test.mtu_tx_min);

                // TX has set MTU sweep test maximum MTU size
                } else if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_MTUMAX) {

                    eth->mtu_test.mtu_tx_max = (uint16_t)ntohll(*eth->frm.rx_sub_tlv_value);
                    printf("Maximum MTU set to %" PRIu16 "\n", eth->mtu_test.mtu_tx_max);

                // TX has enabled link quality tests
                } else if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_QMTEST) {

                    eth->qm_test.enabled = true;
                    printf("Link quality tests enabled\n");

                // TX has set echo interval
                } else if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_QMINTERVAL) {

                    // Convert to ns for use with timespec
                    eth->qm_test.interval_nsec =
                      (ntohll(*eth->frm.rx_sub_tlv_value)*1000000)%1000000000;

                    eth->qm_test.interval_sec = 
                      ((ntohll(*eth->frm.rx_sub_tlv_value)*1000000)-
                       eth->qm_test.interval_nsec)/1000000000;
                    
                // TX has set echo timeout
                } else if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_QMTIMEOUT) {

                    eth->qm_test.timeout_nsec =  (ntohll(*eth->frm.rx_sub_tlv_value) * 1000000) % 1000000000;
                    eth->qm_test.timeout_sec =   ((ntohll(*eth->frm.rx_sub_tlv_value) * 1000000) - eth->qm_test.timeout_nsec) / 1000000000;
                    
                }


            } else if (rx_len <0) {

                perror("Setting receive failed ");
                {
                    return;
                }

            }

        } // WAITING

      
    } // TX or RX mode

}



void update_frame_size(struct etherate *eth)
{

    // Total size of the frame data (paylod size+headers), this excludes the
    // preamble & start frame delimiter, FCS and inter frame gap
    eth->params.f_size_total = eth->params.f_size + eth->frm.length;

    int16_t PHY_MTU = get_interface_mtu_by_name(&eth->intf);
    
    if (PHY_MTU <= 0) {

        printf("\nPhysical interface MTU unknown, "
               "tests might exceed physical MTU!\n\n");

    } else if (eth->params.f_size_total > PHY_MTU + 14) {
        
        printf("\nPhysical interface MTU (%" PRId16 " with headers) is less than\n"
               "the test frame size (%" PRIu16 " with headers). Test frames shall\n"
               "be limited to the interface MTU size\n\n",
               PHY_MTU+14, eth->params.f_size_total);
        
        eth->params.f_size_total = PHY_MTU + 14;

    }

}
