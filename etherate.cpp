/*
 * License:
 *
 * Copyright (c) 2012-2016 James Bensley.
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
 * File: Etherate Main Body
 *
 * File Contents:
 * HEADERS
 * DEFAULT VALUES
 * CLI ARGS
 * INTERFACE SETUP
 * SETTINGS SYNC
 * CALCULATE DELAY
 * MAIN TEST PHASE
 *
 */



 /*
 ********************************************************************** HEADERS
 */

#include <byteswap.h>        // __bswap_64
#include <cstring>           // memcpy()
#include <ctime>             // timers
#include <endian.h>          // __BYTE_ORDER
#include <ifaddrs.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>  // ETH_P_ALL (0x003)
                             // ETH_FRAME_LEN (default 1514)
                             // ETH_ALEN (default 6)
#include <linux/net.h>       // SOCK_RAW
#include <math.h>            // fabsl(), roundl()
#include <netinet/in.h>      // htonl(), htons(), ntohl(), ntohs()
#include <signal.h>          // signal()
#include <stdio.h>           // perror(), printf()
#include <stdlib.h>          // atoi(), calloc(), EXIT_SUCCESS, free(),
                             // malloc(), strtoul(), strtoull()
#include <string.h>          // strncmp()
#define MAX_IFS 64
#include <sys/ioctl.h>
#include <sys/socket.h>      // AF_PACKET *Address Family*
#include "sysexits.h"        // EX_USAGE, EX_NOPERM, EX_PROTOCOL, EX_SOFTWARE
#include <time.h>            // clock_gettime(), struct timeval, time_t,
                             // struct tm
#include "unistd.h"          // getuid(), sleep()
#include <poll.h> /////
#ifndef CLOCK_MONOTONIC_RAW
#define CLOCK_MONOTONIC_RAW 4
#endif

#include "etherate.h"
#include "funcs.cpp"
#include "tests.cpp"


int main(int argc, char *argv[]) {


    struct FRAME_HEADERS FRAME_HEADERS;
    struct TEST_INTERFACE TEST_INTERFACE;
    struct TEST_PARAMS TEST_PARAMS;
    struct MTU_TEST MTU_TEST;
    struct QM_TEST QM_TEST;
    struct APP_PARAMS APP_PARAMS;

    char TESTING = true;

    while(TESTING)
    {

        /*
         ******************************************************* DEFAULT VALUES
         */

        pFRAME_HEADERS                  = &FRAME_HEADERS;
        FRAME_HEADERS.SOURCE_MAC[0]     = 0x00;
        FRAME_HEADERS.SOURCE_MAC[1]     = 0x00;
        FRAME_HEADERS.SOURCE_MAC[2]     = 0x5E;
        FRAME_HEADERS.SOURCE_MAC[3]     = 0x00;
        FRAME_HEADERS.SOURCE_MAC[4]     = 0x00;
        FRAME_HEADERS.SOURCE_MAC[5]     = 0x01;
        FRAME_HEADERS.DEST_MAC[0]       = 0x00;
        FRAME_HEADERS.DEST_MAC[1]       = 0x00;
        FRAME_HEADERS.DEST_MAC[2]       = 0x5E;
        FRAME_HEADERS.DEST_MAC[3]       = 0x00;
        FRAME_HEADERS.DEST_MAC[4]       = 0x00;
        FRAME_HEADERS.DEST_MAC[5]       = 0x02;
        FRAME_HEADERS.LENGTH            = HEADERS_LEN_DEF;
        FRAME_HEADERS.ETHERTYPE         = ETHERTYPE_DEF;
        FRAME_HEADERS.PCP               = PCP_DEF;
        FRAME_HEADERS.VLAN_ID           = VLAN_ID_DEF;
        FRAME_HEADERS.QINQ_ID           = QINQ_ID_DEF;
        FRAME_HEADERS.QINQ_PCP          = QINQ_PCP_DEF;
        FRAME_HEADERS.LSP_SOURCE_MAC[0] = 0x00;
        FRAME_HEADERS.LSP_SOURCE_MAC[1] = 0x00;
        FRAME_HEADERS.LSP_SOURCE_MAC[2] = 0x00;
        FRAME_HEADERS.LSP_SOURCE_MAC[3] = 0x00;
        FRAME_HEADERS.LSP_SOURCE_MAC[4] = 0x00;
        FRAME_HEADERS.LSP_SOURCE_MAC[5] = 0x00;
        FRAME_HEADERS.LSP_SOURCE_MAC[6] = 0x00;
        FRAME_HEADERS.LSP_DEST_MAC[0]   = 0x00;
        FRAME_HEADERS.LSP_DEST_MAC[1]   = 0x00;
        FRAME_HEADERS.LSP_DEST_MAC[2]   = 0x00;
        FRAME_HEADERS.LSP_DEST_MAC[3]   = 0x00;
        FRAME_HEADERS.LSP_DEST_MAC[4]   = 0x00;
        FRAME_HEADERS.LSP_DEST_MAC[5]   = 0x00;
        FRAME_HEADERS.LSP_DEST_MAC[6]   = 0x00;
        FRAME_HEADERS.MPLS_LABELS       = 0;
        for (int i = 0; i<MPLS_LABELS_MAX; i++) {
            FRAME_HEADERS.MPLS_LABEL[i] = 0;
            FRAME_HEADERS.MPLS_EXP[i]   = 0;
            FRAME_HEADERS.MPLS_TTL[i]   = 0;
        }
        FRAME_HEADERS.PWE_CONTROL_WORD  = 0;
        FRAME_HEADERS.MPLS_IGNORE       = 0;
        FRAME_HEADERS.TLV_SIZE          = sizeof(char) + sizeof(short) +
                                          sizeof(unsigned long);
        FRAME_HEADERS.SUB_TLV_SIZE      = FRAME_HEADERS.TLV_SIZE + 
                                          sizeof(char) + sizeof(short) + 
                                          sizeof(unsigned long long);

        pTEST_INTERFACE         = &TEST_INTERFACE;
        TEST_INTERFACE.IF_INDEX = IF_INDEX_DEF;
        for (int i = 0; i<IFNAMSIZ; i++) {
            TEST_INTERFACE.IF_NAME[i] = 0;
        }

        TEST_PARAMS.F_SIZE          = F_SIZE_DEF;
        TEST_PARAMS.F_SIZE_TOTAL    = F_SIZE_DEF + FRAME_HEADERS.LENGTH;
        TEST_PARAMS.F_DURATION      = F_DURATION_DEF;
        TEST_PARAMS.F_COUNT         = F_COUNT_DEF; 
        TEST_PARAMS.F_BYTES         = F_BYTES_DEF;
        TEST_PARAMS.S_ELAPSED       = 0;
        TEST_PARAMS.B_TX_SPEED_MAX  = B_TX_SPEED_MAX_DEF;
        TEST_PARAMS.B_TX_SPEED_PREV = 0;
        TEST_PARAMS.F_TX_COUNT      = 0;
        TEST_PARAMS.F_TX_COUNT_PREV = 0;
        TEST_PARAMS.F_TX_SPEED_MAX  = F_TX_SPEED_MAX_DEF;
        TEST_PARAMS.B_TX            = 0;
        TEST_PARAMS.B_TX_PREV       = 0;
        TEST_PARAMS.F_RX_COUNT      = 0;
        TEST_PARAMS.F_RX_COUNT_PREV = 0;
        TEST_PARAMS.B_RX            = 0;
        TEST_PARAMS.B_RX_PREV       = 0;
        TEST_PARAMS.F_INDEX_PREV    = 0;
        TEST_PARAMS.F_RX_ONTIME     = 0;
        TEST_PARAMS.F_RX_EARLY      = 0;
        TEST_PARAMS.F_RX_LATE       = 0;
        TEST_PARAMS.F_RX_OTHER      = 0;
        TEST_PARAMS.B_SPEED         = 0;
        TEST_PARAMS.B_SPEED_MAX     = 0;
        TEST_PARAMS.B_SPEED_AVG     = 0;
        TEST_PARAMS.F_ACK           = false;
        TEST_PARAMS.F_WAITING_ACK   = false;

        MTU_TEST.ENABLED    = false;
        MTU_TEST.MTU_TX_MIN = 1400;
        MTU_TEST.MTU_TX_MAX = 1500;

        pQM_TEST                 = &QM_TEST;
        QM_TEST.ENABLED          = false;
        QM_TEST.INTERVAL         = 1000;
        QM_TEST.INTERVAL_SEC     = 0;
        QM_TEST.INTERVAL_NSEC    = 0;
        QM_TEST.INTERVAL_MIN     = 99999.99999;
        QM_TEST.INTERVAL_MAX     = 0.0;
        QM_TEST.TIMEOUT          = 1000;
        QM_TEST.TIMEOUT_NSEC     = 0;
        QM_TEST.TIMEOUT_SEC      = 0;
        QM_TEST.TIMEOUT_COUNT    = 0;
        QM_TEST.DELAY_TEST_COUNT = 10000;
        QM_TEST.RTT_MIN          = 999999.999999;
        QM_TEST.RTT_MAX          = 0.0;
        QM_TEST.JITTER_MIN       = 999999.999999;
        QM_TEST.JITTER_MAX       = 0.0;
        QM_TEST.pDELAY_RESULTS   = (double*)calloc(QM_TEST.DELAY_TEST_COUNT,
                                                   sizeof(double));

        APP_PARAMS.TX_MODE     = true;
        APP_PARAMS.TX_SYNC     = true;
        APP_PARAMS.TX_DELAY    = TX_DELAY_DEF;
        


        /*
         ************************************************************* CLI ARGS
         */

        int CLI_RET_VAL = cli_args(argc, argv, &APP_PARAMS, &FRAME_HEADERS,
                                   &TEST_INTERFACE, &TEST_PARAMS, &MTU_TEST,
                                   &QM_TEST);

        if (CLI_RET_VAL == -2) {
            return EXIT_SUCCESS;
        } else if (CLI_RET_VAL == -1) {
            return EX_USAGE;
        }


        // Check for root privs because low level socket access is required
        if (getuid() != 0) {
            printf("Must be root to use this program!\n");
            return EX_NOPERM;
        }


        if (APP_PARAMS.TX_MODE) printf("Running in TX mode\n");
        else printf("Running in RX mode\n");


        /*
         ****************************************************** INTERFACE SETUP
         */

        TEST_INTERFACE.SOCKET_FD = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

///// Monitor sock1 for input
TEST_INTERFACE.fds[0].fd = TEST_INTERFACE.SOCKET_FD;
TEST_INTERFACE.fds[0].events = POLLIN;
TEST_INTERFACE.fds[0].revents = 0;

        if (TEST_INTERFACE.SOCKET_FD < 0 )
        {
          printf("Error defining socket!\n");
          perror("socket() ");
          close(TEST_INTERFACE.SOCKET_FD);
          return EX_SOFTWARE;
        }


        // If the user has supplied an interface index try to use that
        if (TEST_INTERFACE.IF_INDEX != IF_INDEX_DEF) {

            TEST_INTERFACE.IF_INDEX = set_sock_interface_index(&TEST_INTERFACE);
            if (TEST_INTERFACE.IF_INDEX == -1)
            {
                printf("Error: Couldn't set interface with index, "
                       "returned index was 0!\n");
                return EX_SOFTWARE;
            }

        // Or if the user has supplied an interface name try to use that        
        } else if (strcmp(TEST_INTERFACE.IF_NAME, "") != 0) {

            TEST_INTERFACE.IF_INDEX = set_sock_interface_name(&TEST_INTERFACE);
            if (TEST_INTERFACE.IF_INDEX == -1)
            {
                printf("Error: Couldn't set interface index from name, "
                       "returned index was 0!\n");
                return EX_SOFTWARE;
            }

        // Otherwise, try and best guess an interface
        } else if (TEST_INTERFACE.IF_INDEX == IF_INDEX_DEF) {

            TEST_INTERFACE.IF_INDEX = get_sock_interface(&TEST_INTERFACE);
            if (TEST_INTERFACE.IF_INDEX == -1)
            {
                printf("Error: Couldn't find appropriate interface ID, "
                      "returned ID was 0!\n Try supplying a source MAC address "
                      "with the -s option.\n");
                return EX_SOFTWARE;
            }

        }


        // Link layer socket setup
        TEST_INTERFACE.SOCKET_ADDRESS.sll_family   = PF_PACKET;	
        TEST_INTERFACE.SOCKET_ADDRESS.sll_protocol = htons(ETH_P_IP);
        TEST_INTERFACE.SOCKET_ADDRESS.sll_ifindex  = TEST_INTERFACE.IF_INDEX;    
        TEST_INTERFACE.SOCKET_ADDRESS.sll_hatype   = ARPHRD_ETHER;
        TEST_INTERFACE.SOCKET_ADDRESS.sll_pkttype  = PACKET_OTHERHOST;
        TEST_INTERFACE.SOCKET_ADDRESS.sll_halen    = ETH_ALEN;		
        TEST_INTERFACE.SOCKET_ADDRESS.sll_addr[0]  = FRAME_HEADERS.DEST_MAC[0];		
        TEST_INTERFACE.SOCKET_ADDRESS.sll_addr[1]  = FRAME_HEADERS.DEST_MAC[1];
        TEST_INTERFACE.SOCKET_ADDRESS.sll_addr[2]  = FRAME_HEADERS.DEST_MAC[2];
        TEST_INTERFACE.SOCKET_ADDRESS.sll_addr[3]  = FRAME_HEADERS.DEST_MAC[3];
        TEST_INTERFACE.SOCKET_ADDRESS.sll_addr[4]  = FRAME_HEADERS.DEST_MAC[4];
        TEST_INTERFACE.SOCKET_ADDRESS.sll_addr[5]  = FRAME_HEADERS.DEST_MAC[5];
        TEST_INTERFACE.SOCKET_ADDRESS.sll_addr[6]  = 0x00;
        TEST_INTERFACE.SOCKET_ADDRESS.sll_addr[7]  = 0x00;

        // Send and receive buffers for incoming/outgoing ethernet frames
        FRAME_HEADERS.RX_BUFFER = (char*)calloc(1, F_SIZE_MAX);
        FRAME_HEADERS.TX_BUFFER = (char*)calloc(1, F_SIZE_MAX);

        build_headers(&FRAME_HEADERS);

        // Total size of the frame data (paylod size+headers), this excludes the
        // preamble & start frame delimiter, FCS and inter frame gap
        TEST_PARAMS.F_SIZE_TOTAL = TEST_PARAMS.F_SIZE + FRAME_HEADERS.LENGTH;


        int PHY_MTU = get_interface_mtu_by_name(&TEST_INTERFACE);
        
        if (PHY_MTU==-1) {

            printf("\nPhysical interface MTU unknown, "
                   "test might exceed physical MTU!\n\n");

        } else if (TEST_PARAMS.F_SIZE_TOTAL > PHY_MTU + 14) {
            
            printf("\nPhysical interface MTU (%u with headers) is less than\n"
                   "the test frame size (%u with headers). Test frames shall\n"
                   "bo limited to the interface MTU size\n\n",
                   PHY_MTU+14, TEST_PARAMS.F_SIZE_TOTAL);
            
            TEST_PARAMS.F_SIZE_TOTAL = PHY_MTU + 14;

        }

        // Fill the test frame with some random data
        for (int i = 0; i < (F_SIZE_MAX-FRAME_HEADERS.LENGTH); i++)
        {
            FRAME_HEADERS.TX_DATA[i] = (char)((255.0*rand()/(RAND_MAX+1.0)));
        }

        
        // Send and receive ret vals
        int TX_RET_VAL = 0;
        int RX_LEN     = 0;


        // Set the network interface to promiscuos mode
        printf("Entering promiscuous mode\n");
        strncpy(ethreq.ifr_name,TEST_INTERFACE.IF_NAME,IFNAMSIZ);

        if (ioctl(TEST_INTERFACE.SOCKET_FD,SIOCGIFFLAGS,&ethreq) == -1) 
        {

            printf("Error getting socket flags, entering promiscuous mode failed!\n");
            perror("ioctl() ");
            close(TEST_INTERFACE.SOCKET_FD);
            return EX_SOFTWARE;

        }

        ethreq.ifr_flags|=IFF_PROMISC;

        if (ioctl(TEST_INTERFACE.SOCKET_FD,SIOCSIFFLAGS,&ethreq) == -1)
        {

            printf("Error setting socket flags, entering promiscuous mode failed!\n");
            perror("ioctl() ");
            close(TEST_INTERFACE.SOCKET_FD);
            return EX_SOFTWARE;

        }


        // Declare sigint handler, TX will signal RX to reset when it quits
        signal (SIGINT,signal_handler);


        printf("Source MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
               FRAME_HEADERS.SOURCE_MAC[0],FRAME_HEADERS.SOURCE_MAC[1],
               FRAME_HEADERS.SOURCE_MAC[2],FRAME_HEADERS.SOURCE_MAC[3],
               FRAME_HEADERS.SOURCE_MAC[4],FRAME_HEADERS.SOURCE_MAC[5]);

        printf("Destination MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
               FRAME_HEADERS.DEST_MAC[0],FRAME_HEADERS.DEST_MAC[1],
               FRAME_HEADERS.DEST_MAC[2],FRAME_HEADERS.DEST_MAC[3],
               FRAME_HEADERS.DEST_MAC[4],FRAME_HEADERS.DEST_MAC[5]);


        // Broadcacst to populate any TCAM or MAC tables
        unsigned char TEMP_MAC[6] = {
            FRAME_HEADERS.DEST_MAC[0],
            FRAME_HEADERS.DEST_MAC[1],
            FRAME_HEADERS.DEST_MAC[2],
            FRAME_HEADERS.DEST_MAC[3],
            FRAME_HEADERS.DEST_MAC[4],
            FRAME_HEADERS.DEST_MAC[5],
        };

        FRAME_HEADERS.DEST_MAC[0] = 0xFF;
        FRAME_HEADERS.DEST_MAC[1] = 0xFF;
        FRAME_HEADERS.DEST_MAC[2] = 0xFF;
        FRAME_HEADERS.DEST_MAC[3] = 0xFF;
        FRAME_HEADERS.DEST_MAC[4] = 0xFF;
        FRAME_HEADERS.DEST_MAC[5] = 0xFF;

        // Rebuild frame headers with destination MAC
        build_headers(&FRAME_HEADERS);

        build_tlv(&FRAME_HEADERS, htons(TYPE_BROADCAST), htonl(VALUE_PRESENCE));
        // Build a dummy sub-TLV to align the buffers and pointers
        build_sub_tlv(&FRAME_HEADERS, htons(TYPE_APPLICATION_SUB_TLV),
                      htonll(VALUE_DUMMY));

        printf("Sending gratuitous broadcasts...\n");
        for (int i=1; i<=3; i++)
        {

            TX_RET_VAL = sendto(TEST_INTERFACE.SOCKET_FD, FRAME_HEADERS.TX_BUFFER,
                                FRAME_HEADERS.LENGTH+FRAME_HEADERS.TLV_SIZE, 0, 
                                (struct sockaddr*)&TEST_INTERFACE.SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE.SOCKET_ADDRESS));

            sleep(1);

        }


        FRAME_HEADERS.DEST_MAC[0] = TEMP_MAC[0];
        FRAME_HEADERS.DEST_MAC[1] = TEMP_MAC[1];
        FRAME_HEADERS.DEST_MAC[2] = TEMP_MAC[2];
        FRAME_HEADERS.DEST_MAC[3] = TEMP_MAC[3];
        FRAME_HEADERS.DEST_MAC[4] = TEMP_MAC[4];
        FRAME_HEADERS.DEST_MAC[5] = TEMP_MAC[5];

        build_headers(&FRAME_HEADERS);
        /////build_tlv(&FRAME_HEADERS, htons(TYPE_BROADCAST), htonl(VALUE_PRESENCE));
        /////build_sub_tlv(&FRAME_HEADERS, htons(TYPE_APPLICATION_SUB_TLV),
        /////htonll(VALUE_DUMMY));

        // Total size of the frame data (paylod size+headers), this excludes the
        // preamble & start frame delimiter, FCS and inter frame gap
/////        TEST_PARAMS.F_SIZE_TOTAL = TEST_PARAMS.F_SIZE + FRAME_HEADERS.LENGTH;



        /*
         ******************************************************** SETTINGS SYNC
         */

        sync_settings(&APP_PARAMS, &FRAME_HEADERS, &TEST_INTERFACE, &TEST_PARAMS,
                      &MTU_TEST, &QM_TEST);

        // Pause to allow the RX host to process the sent settings
        sleep(1);

        // Rebuild the test frame headers in case any settings have been changed
        // by the TX host
        if (!APP_PARAMS.TX_MODE) build_headers(&FRAME_HEADERS);


        // Try to measure the TX to RX one way delay
        if (APP_PARAMS.TX_DELAY)
            delay_test(&APP_PARAMS, &FRAME_HEADERS, &TEST_INTERFACE, &TEST_PARAMS,
                       &QM_TEST);


        /*
         ****************************************************** MAIN TEST PHASE
         */    

        if (MTU_TEST.ENABLED) {

            mtu_sweep_test(&APP_PARAMS, &FRAME_HEADERS, &TEST_INTERFACE,
                           &TEST_PARAMS, &MTU_TEST);

        } else if (QM_TEST.ENABLED) {

            latency_test(&APP_PARAMS, &FRAME_HEADERS, &TEST_INTERFACE,
                         &TEST_PARAMS, &QM_TEST);

        } else {

            speed_test(&APP_PARAMS, &FRAME_HEADERS, &TEST_INTERFACE,
                       &TEST_PARAMS);

        }
        

        // End the testing loop if TX host
        if (APP_PARAMS.TX_MODE) TESTING=false;

    }


    printf("Leaving promiscuous mode\n");

    strncpy(ethreq.ifr_name,TEST_INTERFACE.IF_NAME,IFNAMSIZ);

    if (ioctl(TEST_INTERFACE.SOCKET_FD,SIOCGIFFLAGS,&ethreq) == -1)
    {
        printf("Error getting socket flags, leaving promiscuous mode failed!\n");
        perror("ioctl() ");
        close(TEST_INTERFACE.SOCKET_FD);
        return EX_SOFTWARE;
    }

    ethreq.ifr_flags &= ~IFF_PROMISC;

    if (ioctl(TEST_INTERFACE.SOCKET_FD,SIOCSIFFLAGS,&ethreq) == -1)
    {
        printf("Error setting socket flags, leaving promiscuous mode failed\n");
        perror("ioctl() ");
        close(TEST_INTERFACE.SOCKET_FD);
        return EX_SOFTWARE;
    }

    close(TEST_INTERFACE.SOCKET_FD);

    free (QM_TEST.pDELAY_RESULTS);
    free (FRAME_HEADERS.RX_BUFFER);
    free (FRAME_HEADERS.TX_BUFFER);

    return EXIT_SUCCESS;

}