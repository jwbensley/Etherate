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
#include <endian.h>          // __BYTE_ORDER
#include <errno.h>           // errno
#include <ifaddrs.h>         // freeifaddrs(), getifaddrs()
#define __STDC_FORMAT_MACROS
#include <inttypes.h>        // intN_t, uintN_t, PRIuN, SCNuN
#include <linux/if_arp.h>    // IFNAMSIZ, struct ifreq, sockaddr_ll
#include <linux/if_ether.h>  // ETH_P_ALL (0x003)
                             // ETH_FRAME_LEN (default 1514)
                             // ETH_ALEN (default 6)
#include <linux/net.h>       // SOCK_RAW
#include <math.h>            // fabsl(), floorl(), roundl()
#include <netinet/in.h>      // htonl(), htons(), ntohl(), ntohs()
#include <signal.h>          // signal()
#include <stdbool.h>         // false, true
#include <stdio.h>           // perror(), printf()
#include <stdlib.h>          // atoi(), calloc(), EXIT_SUCCESS, free(),
                             // fscanf(), malloc(), strtoul(), strtoull()
#include <string.h>          // memcpy(), strncmp(), strsep(), strtok_r()
#define MAX_IFS 64           // Max struct ifreq
#include <sys/ioctl.h>       // ioctl()
#include <sys/socket.h>      // AF_PACKET
#include "sysexits.h"        // EX_USAGE, EX_NOPERM, EX_PROTOCOL, EX_SOFTWARE
#include <time.h>            // clock_gettime(), struct timeval, time_t,
                             // struct tm
#include "unistd.h"          // getuid(), sleep()

#ifndef CLOCK_MONOTONIC_RAW
#define CLOCK_MONOTONIC_RAW 4
#endif

#include "etherate.h"
#include "test_functions.c"
#include "functions.c"
#include "defaults.c"
#include "speed_tests.c"
#include "tests.c"



int main(int argc, char *argv[]) {

    struct etherate eth;

    // Global pointer used by signal handler function
    eth_p = &eth;

    uint8_t testing = true;
    while(testing == true)
    {

        // Setup default application settings
        int16_t def_ret = default_app(&eth);
        if (def_ret != EXIT_SUCCESS)
            return def_ret;


        // Check for root privs, low level socket access is required
        if (getuid() != 0) {
            printf("Must be root to use this program!\n");
            reset_app(&eth);
            return EX_NOPERM;
        }


        // Parse CLI arguments
        int16_t cli_ret = cli_args(argc, argv, &eth);

        if (cli_ret != EXIT_SUCCESS) {
            reset_app(&eth);
        
            if (cli_ret == RET_EXIT_APP) {
                return EXIT_SUCCESS;
            } else {
                return cli_ret;
            }
        }


        // Set up a new socket bind to an interface
        if (setup_sock(&eth.intf) != EXIT_SUCCESS) {
            int32_t errnum = errno;
            reset_app(&eth);
            return errnum;
        }

        if (set_sock_int(&eth) != EXIT_SUCCESS)
        {
            int32_t errnum = errno;
            reset_app(&eth);
            return errnum;
        }

        update_frame_size(&eth);


        // Set the network interface to promiscuos mode
        if (set_int_promisc(&eth.intf) != EXIT_SUCCESS)
        {
            int32_t errnum = errno;
            reset_app(&eth);
            return errnum;
        }


        // Declare sigint handler, TX will signal RX to reset when it quits.
        // This can not be declared until now because the interfaces must be
        // set up so that a dying gasp can be sent.
        signal (SIGINT, signal_handler);


        // Set up the frame data and send broadcasts
        if (setup_frame(&eth) != EXIT_SUCCESS)
        {
            int32_t errnum = errno;
            reset_app(&eth);
            return errnum;
        }


        if (eth.app.tx_sync == true)
            sync_settings(&eth);

        // Pause to allow the RX host to process the sent settings
        sleep(2);


        // If this is the RX host, rebuild the test frame headers incase any
        // settings have been changed by the TX host
        if (eth.app.tx_mode != true) build_headers(&eth.frm);


        // If using frame pacing, calculate the inter-frame delay now that the
        // frame size is no longer subject to change
        if (eth.params.f_tx_dly != F_TX_DLY_DEF) {
            long double f_tx_max  = floor((eth.params.f_tx_dly / (eth.params.f_size_total * 8)));
            
            eth.params.f_tx_dly = (1000000000 / f_tx_max) * 1e-9;

            printf("Pacing to a maximum rate of %.0Lffps (%.9Lfs spacing)\n",
                   f_tx_max, eth.params.f_tx_dly);
        }


        // Try to measure the TX to RX one way delay
        if (eth.app.tx_delay == true)
            delay_test(&eth);


        // Start the chosen test
        eth.app.ts_now =   time(0);
        eth.app.tm_local = localtime(&eth.app.ts_now);
        printf("Starting test on %s\n", asctime(eth.app.tm_local));   

        if (eth.mtu_test.enabled) {
            mtu_sweep_test(&eth);

        } else if (eth.qm_test.enabled) {
            latency_test(&eth);

        } else if (eth.speed_test.f_payload_size > 0) {
            eth.speed_test.enabled = true;
            printf("Frame size is %" PRId16 " bytes\n", eth.params.f_size_total);
            send_custom_frame(&eth);

        } else {
            eth.speed_test.enabled = true;
            printf("Frame size is %" PRId16 " bytes\n", eth.params.f_size_total);
            speed_test_prep(&eth);

        }

        eth.app.ts_now = time(0);
        eth.app.tm_local = localtime(&eth.app.ts_now);
        printf("Ending test on %s\n\n", asctime(eth.app.tm_local));


        if (remove_interface_promisc(&eth.intf) != EXIT_SUCCESS)
            printf("Error leaving promiscuous mode\n");

        reset_app(&eth);

        // End the testing loop if TX host
        if (eth.app.tx_mode == true) testing = false;


    }


    return EXIT_SUCCESS;

}
