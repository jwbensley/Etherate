/*
 * License:
 *
 * Copyright (c) 2012-2016 James Bensley.
 *
 * Permission is hereby granted, free of uint8_tge, to any person obtaining
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
 * File: Etherate Setup Function
 *
 * File Contents:
 * void set_default_values()
 * int16_t setup_frame()
 * int16_t setup_socket()
 * int16_t setup_socket_interface()
 *
 */



#include "funcs.cpp"

// Restore all test and interface values to default at the start of each RX loop
void set_default_values(struct APP_PARAMS *APP_PARAMS,
                        struct FRAME_HEADERS *FRAME_HEADERS,
                        struct MTU_TEST *MTU_TEST,
                        struct TEST_INTERFACE *TEST_INTERFACE,
                        struct TEST_PARAMS * TEST_PARAMS,
                        struct QM_TEST *QM_TEST);

// Set up Etherate test frame
int16_t setup_frame(struct FRAME_HEADERS *FRAME_HEADERS,
                struct TEST_INTERFACE *TEST_INTERFACE,
                struct TEST_PARAMS *TEST_PARAMS);

// Set up an OS socket
int16_t setup_socket(struct TEST_INTERFACE *TEST_INTERFACE);

// Set up the physical interface and bind to socket
int16_t setup_socket_interface(struct FRAME_HEADERS *FRAME_HEADERS,
                           struct TEST_INTERFACE *TEST_INTERFACE,
                           struct TEST_PARAMS *TEST_PARAMS);



void set_default_values(struct APP_PARAMS *APP_PARAMS,
                        struct FRAME_HEADERS *FRAME_HEADERS,
                        struct MTU_TEST *MTU_TEST,
                        struct TEST_INTERFACE *TEST_INTERFACE,
                        struct TEST_PARAMS * TEST_PARAMS,
                        struct QM_TEST *QM_TEST)
{

    pFRAME_HEADERS                   = FRAME_HEADERS;
    FRAME_HEADERS->SOURCE_MAC[0]     = 0x00;
    FRAME_HEADERS->SOURCE_MAC[1]     = 0x00;
    FRAME_HEADERS->SOURCE_MAC[2]     = 0x5E;
    FRAME_HEADERS->SOURCE_MAC[3]     = 0x00;
    FRAME_HEADERS->SOURCE_MAC[4]     = 0x00;
    FRAME_HEADERS->SOURCE_MAC[5]     = 0x01;
    FRAME_HEADERS->DEST_MAC[0]       = 0x00;
    FRAME_HEADERS->DEST_MAC[1]       = 0x00;
    FRAME_HEADERS->DEST_MAC[2]       = 0x5E;
    FRAME_HEADERS->DEST_MAC[3]       = 0x00;
    FRAME_HEADERS->DEST_MAC[4]       = 0x00;
    FRAME_HEADERS->DEST_MAC[5]       = 0x02;
    FRAME_HEADERS->LENGTH            = HEADERS_LEN_DEF;
    FRAME_HEADERS->ETHERTYPE         = ETHERTYPE_DEF;
    FRAME_HEADERS->PCP               = PCP_DEF;
    FRAME_HEADERS->VLAN_ID           = VLAN_ID_DEF;
    FRAME_HEADERS->VLAN_DEI          = 0;
    FRAME_HEADERS->QINQ_ID           = QINQ_ID_DEF;
    FRAME_HEADERS->QINQ_PCP          = QINQ_PCP_DEF;
    FRAME_HEADERS->QINQ_DEI          = 0;
    FRAME_HEADERS->LSP_SOURCE_MAC[0] = 0x00;
    FRAME_HEADERS->LSP_SOURCE_MAC[1] = 0x00;
    FRAME_HEADERS->LSP_SOURCE_MAC[2] = 0x00;
    FRAME_HEADERS->LSP_SOURCE_MAC[3] = 0x00;
    FRAME_HEADERS->LSP_SOURCE_MAC[4] = 0x00;
    FRAME_HEADERS->LSP_SOURCE_MAC[5] = 0x00;
    FRAME_HEADERS->LSP_DEST_MAC[0]   = 0x00;
    FRAME_HEADERS->LSP_DEST_MAC[1]   = 0x00;
    FRAME_HEADERS->LSP_DEST_MAC[2]   = 0x00;
    FRAME_HEADERS->LSP_DEST_MAC[3]   = 0x00;
    FRAME_HEADERS->LSP_DEST_MAC[4]   = 0x00;
    FRAME_HEADERS->LSP_DEST_MAC[5]   = 0x00;
    FRAME_HEADERS->MPLS_LABELS       = 0;
    for (uint16_t i = 0; i<MPLS_LABELS_MAX; i += 1) {
        FRAME_HEADERS->MPLS_LABEL[i] = 0;
        FRAME_HEADERS->MPLS_EXP[i]   = 0;
        FRAME_HEADERS->MPLS_TTL[i]   = 0;
    }
    FRAME_HEADERS->PWE_CONTROL_WORD  = 0;
    FRAME_HEADERS->TLV_SIZE          = sizeof(uint8_t) + sizeof(uint16_t) +
                                       sizeof(uint32_t);
    FRAME_HEADERS->SUB_TLV_SIZE      = FRAME_HEADERS->TLV_SIZE + 
                                       sizeof(uint8_t) + sizeof(uint16_t) + 
                                       sizeof(uint64_t);

    // Send and receive buffers for incoming/outgoing ethernet frames
    FRAME_HEADERS->RX_BUFFER = (uint8_t*)calloc(1, F_SIZE_MAX);
    FRAME_HEADERS->TX_BUFFER = (uint8_t*)calloc(1, F_SIZE_MAX);

    pTEST_INTERFACE          = TEST_INTERFACE;
    TEST_INTERFACE->IF_INDEX = IF_INDEX_DEF;
    for (uint16_t i = 0; i<IFNAMSIZ; i += 1) {
        TEST_INTERFACE->IF_NAME[i] = 0;
    }

    pTEST_PARAMS                 = TEST_PARAMS;
    TEST_PARAMS->F_SIZE          = F_SIZE_DEF;
    TEST_PARAMS->F_SIZE_TOTAL    = F_SIZE_DEF + FRAME_HEADERS->LENGTH;
    TEST_PARAMS->F_DURATION      = F_DURATION_DEF;
    TEST_PARAMS->F_COUNT         = F_COUNT_DEF; 
    TEST_PARAMS->F_BYTES         = F_BYTES_DEF;
    TEST_PARAMS->F_PAYLOAD       = (uint8_t*)calloc(1, F_SIZE_MAX);
    TEST_PARAMS->F_PAYLOAD_SIZE  = 0;
    TEST_PARAMS->S_ELAPSED       = 0;
    TEST_PARAMS->B_TX_SPEED_MAX  = B_TX_SPEED_MAX_DEF;
    TEST_PARAMS->B_TX_SPEED_PREV = 0;
    TEST_PARAMS->F_TX_COUNT      = 0;
    TEST_PARAMS->F_TX_COUNT_PREV = 0;
    TEST_PARAMS->F_TX_SPEED_MAX  = F_TX_SPEED_MAX_DEF;
    TEST_PARAMS->F_SPEED         = 0;
    TEST_PARAMS->F_SPEED_AVG     = 0;
    TEST_PARAMS->F_SPEED_MAX     = 0;
    TEST_PARAMS->B_TX            = 0;
    TEST_PARAMS->B_TX_PREV       = 0;
    TEST_PARAMS->F_RX_COUNT      = 0;
    TEST_PARAMS->F_RX_COUNT_PREV = 0;
    TEST_PARAMS->B_RX            = 0;
    TEST_PARAMS->B_RX_PREV       = 0;
    TEST_PARAMS->F_INDEX_PREV    = 0;
    TEST_PARAMS->F_RX_ONTIME     = 0;
    TEST_PARAMS->F_RX_EARLY      = 0;
    TEST_PARAMS->F_RX_LATE       = 0;
    TEST_PARAMS->F_RX_OTHER      = 0;
    TEST_PARAMS->B_SPEED         = 0;
    TEST_PARAMS->B_SPEED_MAX     = 0;
    TEST_PARAMS->B_SPEED_AVG     = 0;
    TEST_PARAMS->F_ACK           = false;
    TEST_PARAMS->F_WAITING_ACK   = false;

    MTU_TEST->ENABLED    = false;
    MTU_TEST->MTU_TX_MIN = 1400;
    MTU_TEST->MTU_TX_MAX = 1500;

    pQM_TEST                  = QM_TEST;
    QM_TEST->ENABLED          = false;
    QM_TEST->INTERVAL         = 1000;
    QM_TEST->INTERVAL_SEC     = 0;
    QM_TEST->INTERVAL_NSEC    = 0;
    QM_TEST->INTERVAL_MIN     = 99999.99999;
    QM_TEST->INTERVAL_MAX     = 0.0;
    QM_TEST->TIMEOUT          = 1000;
    QM_TEST->TIMEOUT_NSEC     = 0;
    QM_TEST->TIMEOUT_SEC      = 0;
    QM_TEST->TIMEOUT_COUNT    = 0;
    QM_TEST->TEST_COUNT       = 0;
    QM_TEST->DELAY_TEST_COUNT = 10000;
    QM_TEST->RTT_MIN          = 999999.999999;
    QM_TEST->RTT_MAX          = 0.0;
    QM_TEST->JITTER_MIN       = 999999.999999;
    QM_TEST->JITTER_MAX       = 0.0;
    QM_TEST->pDELAY_RESULTS   = (double*)calloc(QM_TEST->DELAY_TEST_COUNT,
                                               sizeof(double));

    APP_PARAMS->BROADCAST   = true;
    APP_PARAMS->TX_MODE     = true;
    APP_PARAMS->TX_SYNC     = true;
    APP_PARAMS->TX_DELAY    = TX_DELAY_DEF;
        

    return;
}



int16_t setup_frame(struct APP_PARAMS *APP_PARAMS,
                    struct FRAME_HEADERS *FRAME_HEADERS,
                    struct TEST_INTERFACE *TEST_INTERFACE,
                    struct TEST_PARAMS *TEST_PARAMS)
{

    int16_t RET_VAL = EXIT_SUCCESS;

    printf("Source MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           FRAME_HEADERS->SOURCE_MAC[0],FRAME_HEADERS->SOURCE_MAC[1],
           FRAME_HEADERS->SOURCE_MAC[2],FRAME_HEADERS->SOURCE_MAC[3],
           FRAME_HEADERS->SOURCE_MAC[4],FRAME_HEADERS->SOURCE_MAC[5]);

    printf("Destination MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           FRAME_HEADERS->DEST_MAC[0],FRAME_HEADERS->DEST_MAC[1],
           FRAME_HEADERS->DEST_MAC[2],FRAME_HEADERS->DEST_MAC[3],
           FRAME_HEADERS->DEST_MAC[4],FRAME_HEADERS->DEST_MAC[5]);

    // Fill the test frame buffer with random data
    for (uint32_t i = 0; i < (uint16_t)(F_SIZE_MAX-FRAME_HEADERS->LENGTH); i += 1)
    {
        FRAME_HEADERS->TX_DATA[i] = (uint8_t)((255.0*rand()/(RAND_MAX+1.0)));
    }

    if (APP_PARAMS->BROADCAST == true) {

        // Broadcast to populate any switch/MAC tables
        int16_t BROAD_RET_VAL = broadcast_etherate(FRAME_HEADERS, TEST_INTERFACE);
        if (BROAD_RET_VAL != 0) return BROAD_RET_VAL;

    }
    
    build_headers(FRAME_HEADERS);

    return RET_VAL;

}



int16_t setup_socket(struct TEST_INTERFACE *TEST_INTERFACE)
{
    int16_t RET_VAL = EXIT_SUCCESS;

    TEST_INTERFACE->SOCKET_FD = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if (TEST_INTERFACE->SOCKET_FD < 0 )
    {
      printf("Error defining socket!\n");
      perror("socket() ");
      close(TEST_INTERFACE->SOCKET_FD);
      return EX_SOFTWARE;
    }

    return RET_VAL;
}



int16_t setup_socket_interface(struct FRAME_HEADERS *FRAME_HEADERS,
                               struct TEST_INTERFACE *TEST_INTERFACE,
                               struct TEST_PARAMS *TEST_PARAMS)
{

    int16_t RET_VAL = EXIT_SUCCESS;


    // If the user has supplied an interface index try to use that
    if (TEST_INTERFACE->IF_INDEX != IF_INDEX_DEF) {

        TEST_INTERFACE->IF_INDEX = set_sock_interface_index(TEST_INTERFACE);
        if (TEST_INTERFACE->IF_INDEX == -1)
        {
            printf("Error: Couldn't set interface with index, "
                   "returned index was 0!\n");
            return EX_SOFTWARE;
        }

    // Or if the user has supplied an interface name try to use that        
    } else if (strcmp((char*)TEST_INTERFACE->IF_NAME, "") != 0) {

        TEST_INTERFACE->IF_INDEX = set_sock_interface_name(TEST_INTERFACE);
        if (TEST_INTERFACE->IF_INDEX == -1)
        {
            printf("Error: Couldn't set interface index from name, "
                   "returned index was 0!\n");
            return EX_SOFTWARE;
        }

    // Otherwise, try and best guess an interface
    } else if (TEST_INTERFACE->IF_INDEX == IF_INDEX_DEF) {

        TEST_INTERFACE->IF_INDEX = get_sock_interface(TEST_INTERFACE);
        if (TEST_INTERFACE->IF_INDEX == -1)
        {
            printf("Error: Couldn't find appropriate interface ID, "
                  "returned ID was 0!\n Try supplying a source MAC address "
                  "with the -s option.\n");
            return EX_SOFTWARE;
        }

    }


    // Link layer socket setup
    TEST_INTERFACE->SOCKET_ADDRESS.sll_family   = PF_PACKET;    
    TEST_INTERFACE->SOCKET_ADDRESS.sll_protocol = htons(ETH_P_IP);
    TEST_INTERFACE->SOCKET_ADDRESS.sll_ifindex  = TEST_INTERFACE->IF_INDEX;    
    TEST_INTERFACE->SOCKET_ADDRESS.sll_hatype   = ARPHRD_ETHER;
    TEST_INTERFACE->SOCKET_ADDRESS.sll_pkttype  = PACKET_OTHERHOST;
    TEST_INTERFACE->SOCKET_ADDRESS.sll_halen    = ETH_ALEN;        
    TEST_INTERFACE->SOCKET_ADDRESS.sll_addr[0]  = FRAME_HEADERS->DEST_MAC[0];        
    TEST_INTERFACE->SOCKET_ADDRESS.sll_addr[1]  = FRAME_HEADERS->DEST_MAC[1];
    TEST_INTERFACE->SOCKET_ADDRESS.sll_addr[2]  = FRAME_HEADERS->DEST_MAC[2];
    TEST_INTERFACE->SOCKET_ADDRESS.sll_addr[3]  = FRAME_HEADERS->DEST_MAC[3];
    TEST_INTERFACE->SOCKET_ADDRESS.sll_addr[4]  = FRAME_HEADERS->DEST_MAC[4];
    TEST_INTERFACE->SOCKET_ADDRESS.sll_addr[5]  = FRAME_HEADERS->DEST_MAC[5];
    TEST_INTERFACE->SOCKET_ADDRESS.sll_addr[6]  = 0x00;
    TEST_INTERFACE->SOCKET_ADDRESS.sll_addr[7]  = 0x00;

    build_headers(FRAME_HEADERS);

    // Total size of the frame data (paylod size+headers), this excludes the
    // preamble & start frame delimiter, FCS and inter frame gap
    TEST_PARAMS->F_SIZE_TOTAL = TEST_PARAMS->F_SIZE + FRAME_HEADERS->LENGTH;


    uint16_t PHY_MTU = get_interface_mtu_by_name(TEST_INTERFACE);
    
    if (PHY_MTU == -1 || PHY_MTU == 0) {

        printf("\nPhysical interface MTU unknown, "
               "test might exceed physical MTU!\n\n");

    } else if (TEST_PARAMS->F_SIZE_TOTAL > PHY_MTU + 14) {
        
        printf("\nPhysical interface MTU (%u with headers) is less than\n"
               "the test frame size (%u with headers). Test frames shall\n"
               "be limited to the interface MTU size\n\n",
               PHY_MTU+14, TEST_PARAMS->F_SIZE_TOTAL);
        
        TEST_PARAMS->F_SIZE_TOTAL = PHY_MTU + 14;

    }

    return RET_VAL;

}