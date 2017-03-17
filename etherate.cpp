/*
 * License: MIT
 *
 * Copyright (c) 2012-2017 James Bensley.
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
 * File: Etherate Main Loop
 *
 * File Contents:
 * DEFAULT VALUES
 * CLI ARGS
 * INTERFACE SETUP
 * SETTINGS SYNC
 * MAIN TEST PHASE
 *
 */



#include <byteswap.h>        // __bswap_64
#include <cstring>           // memcpy()
#include <ctime>             // timers
#include <endian.h>          // __BYTE_ORDER
#include <ifaddrs.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>        // intN_t, uintN_t, PRIuN, SCNuN
#include <linux/if_arp.h>    // sockaddr_ll
#include <linux/if_ether.h>  // ETH_P_ALL (0x003)
                             // ETH_FRAME_LEN (default 1514)
                             // ETH_ALEN (default 6)
#include <linux/net.h>       // SOCK_RAW
#include <math.h>            // fabsl(), roundl()
#include <netinet/in.h>      // htonl(), htons(), ntohl(), ntohs()
#include <signal.h>          // signal()
#include <stdio.h>           // perror(), printf()
#include <stdlib.h>          // atoi(), calloc(), EXIT_SUCCESS, free(),
                             // fscanf(), malloc(), strtoul(), strtoull()
#include <string.h>          // strncmp()
#define MAX_IFS 64
#include <sys/ioctl.h>
#include <sys/socket.h>      // AF_PACKET
#include "sysexits.h"        // EX_USAGE, EX_NOPERM, EX_PROTOCOL, EX_SOFTWARE
#include <time.h>            // clock_gettime(), struct timeval, time_t,
                             // struct tm
#include "unistd.h"          // getuid(), sleep()
#ifndef CLOCK_MONOTONIC_RAW
#define CLOCK_MONOTONIC_RAW 4
#endif

#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

#include "etherate.h"
#include "functions.cpp"
#include "defaults.cpp"
#include "tests.cpp"



int main(int argc, char *argv[]) {

    struct FRAME_HEADERS FRAME_HEADERS;
    struct TEST_INTERFACE TEST_INTERFACE;
    struct TEST_PARAMS TEST_PARAMS;
    struct MTU_TEST MTU_TEST;
    struct QM_TEST QM_TEST;
    struct APP_PARAMS APP_PARAMS;

    uint8_t TESTING = true;
    while(TESTING == true)
    {



        /*
         ******************************************************* DEFAULT VALUES
         */

        set_default_values(&APP_PARAMS, &FRAME_HEADERS, &MTU_TEST,
                           &TEST_INTERFACE, &TEST_PARAMS, &QM_TEST);



        /*
         ************************************************************* CLI ARGS
         */

        int16_t CLI_RET_VAL = cli_args(argc, argv, &APP_PARAMS, &FRAME_HEADERS,
                                       &TEST_INTERFACE, &TEST_PARAMS, &MTU_TEST,
                                       &QM_TEST);

        if (CLI_RET_VAL != EXIT_SUCCESS) {
            reset_app(&FRAME_HEADERS, &TEST_INTERFACE, &TEST_PARAMS, &QM_TEST);
        
            if(CLI_RET_VAL == RET_EXIT_APP) {
                return EXIT_SUCCESS;
            } else {
                return CLI_RET_VAL;
            }
        }

        // Check for root privs, low level socket access is required
        if (getuid() != 0) {
            printf("Must be root to use this program!\n");
            reset_app(&FRAME_HEADERS, &TEST_INTERFACE, &TEST_PARAMS, &QM_TEST);
            return EX_NOPERM;
        }



        /*
         ****************************************************** INTERFACE SETUP
         */

        int16_t SOCK_RET_VAL = setup_socket(&TEST_INTERFACE);
        if (SOCK_RET_VAL != EXIT_SUCCESS) {
            reset_app(&FRAME_HEADERS, &TEST_INTERFACE, &TEST_PARAMS, &QM_TEST);
            return SOCK_RET_VAL;
        }

        int16_t INT_RET_VAL = setup_socket_interface(&FRAME_HEADERS,
                                                     &TEST_INTERFACE,
                                                     &TEST_PARAMS);
        if (INT_RET_VAL != EXIT_SUCCESS) {
            reset_app(&FRAME_HEADERS, &TEST_INTERFACE, &TEST_PARAMS, &QM_TEST);
            return INT_RET_VAL;
        }
        
        // Set the network interface to promiscuos mode
        int16_t PROM_RET_VAL = set_interface_promisc(&TEST_INTERFACE);
        if (PROM_RET_VAL != EXIT_SUCCESS) {
            reset_app(&FRAME_HEADERS, &TEST_INTERFACE, &TEST_PARAMS, &QM_TEST);
            return PROM_RET_VAL;
        }

        // Declare sigint handler, TX will signal RX to reset when it quits.
        // This can not be declared until now because the interfaces must be
        // set up so that a dying gasp can be sent.
        signal (SIGINT,signal_handler);

        int16_t FRAME_RET_VAL = setup_frame(&APP_PARAMS, &FRAME_HEADERS,
                                            &TEST_INTERFACE, &TEST_PARAMS);
        if (FRAME_RET_VAL != EXIT_SUCCESS) {
            reset_app(&FRAME_HEADERS, &TEST_INTERFACE, &TEST_PARAMS, &QM_TEST);
            return FRAME_RET_VAL;
        }



        /*
         ******************************************************** SETTINGS SYNC
         */

        if (APP_PARAMS.TX_SYNC == true)
            sync_settings(&APP_PARAMS, &FRAME_HEADERS, &TEST_INTERFACE,
                          &TEST_PARAMS, &MTU_TEST, &QM_TEST);

        // Pause to allow the RX host to process the sent settings
        sleep(2);

        // Rebuild the test frame headers in case any settings have been changed
        // by the TX host
        if (APP_PARAMS.TX_MODE != true) build_headers(&FRAME_HEADERS);


        // Try to measure the TX to RX one way delay
        if (APP_PARAMS.TX_DELAY == true)
            delay_test(&APP_PARAMS, &FRAME_HEADERS, &TEST_INTERFACE, 
                       &TEST_PARAMS, &QM_TEST);



        /*
         ****************************************************** MAIN TEST PHASE
         */

        APP_PARAMS.TS_NOW =   time(0);
        APP_PARAMS.TM_LOCAL = localtime(&APP_PARAMS.TS_NOW);
        printf("Starting test on %s\n", asctime(APP_PARAMS.TM_LOCAL));   

        if (MTU_TEST.ENABLED) {

            mtu_sweep_test(&APP_PARAMS, &FRAME_HEADERS, &TEST_INTERFACE,
                           &TEST_PARAMS, &MTU_TEST);

        } else if (QM_TEST.ENABLED) {

            latency_test(&APP_PARAMS, &FRAME_HEADERS, &TEST_INTERFACE,
                         &TEST_PARAMS, &QM_TEST);

        } else if (TEST_PARAMS.F_PAYLOAD_SIZE > 0) {

            send_custom_frame(&APP_PARAMS, &FRAME_HEADERS, &TEST_INTERFACE,
                              &TEST_PARAMS);

        }else {

            speed_test(&APP_PARAMS, &FRAME_HEADERS, &TEST_INTERFACE,
                       &TEST_PARAMS);

        }
        
        APP_PARAMS.TS_NOW = time(0);
        APP_PARAMS.TM_LOCAL = localtime(&APP_PARAMS.TS_NOW);
        printf("Ending test on %s\n\n", asctime(APP_PARAMS.TM_LOCAL));

        int16_t RM_PROM_RET_VAL = remove_interface_promisc(&TEST_INTERFACE);
        if (RM_PROM_RET_VAL != EXIT_SUCCESS) {
            printf("Error: Leaving promiscuous mode\n");
        }

        int16_t CLOSE_RET_VAL = close(TEST_INTERFACE.SOCKET_FD);
        if (CLOSE_RET_VAL != EXIT_SUCCESS) {
            perror("Error: Closing socket failed ");
        }

        reset_app(&FRAME_HEADERS, &TEST_INTERFACE, &TEST_PARAMS, &QM_TEST);

        // End the testing loop if TX host
        if (APP_PARAMS.TX_MODE == true) TESTING = false;


    }


    return EXIT_SUCCESS;

}
