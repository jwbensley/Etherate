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
#include <endian.h>          // __BYTE_ORDER
#include <errno.h>           // errno
#include <ifaddrs.h>         // freeifaddrs(), getifaddrs()
#define __STDC_FORMAT_MACROS
#include <inttypes.h>        // intN_t, uintN_t, PRIuN, SCNuN
#include <linux/if_arp.h>    // IFNAMSIZ, struct ifreq, sockaddr_ll
#include <linux/if_ether.h>  // ETH_P_ALL (0x003)
                             // ETH_FRAME_LEN (default 1514)
                             // ETH_ALEN (default 6)
#include <linux/version.h>   // KERNEL_VERSION(), LINUX_VERSION_CODE
#include <linux/net.h>       // SOCK_RAW
#include <math.h>            // fabsl(), roundl()
#include <netinet/in.h>      // htonl(), htons(), ntohl(), ntohs()
#include <signal.h>          // signal()
#include <stdbool.h>         // false, true
#include <stdio.h>           // perror(), printf()
#include <stdlib.h>          // atoi(), calloc(), EXIT_SUCCESS, free(),
                             // fscanf(), malloc(), strtoul(), strtoull()
#include <string.h>          // memcpy(), strncmp(), strsep()
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

#include "etherate.h"
#include "functions.c"
#include "defaults.c"
#include "tests.c"



int main(int argc, char *argv[]) {

    struct app_params app_params;
    struct frame_headers frame_headers;
    struct mtu_test mtu_test;
    struct qm_test qm_test;
    struct speed_test speed_test;
    struct test_interface test_interface;
    struct test_params test_params;

    uint8_t testing = true;
    while(testing == true)
    {



        /*
         ******************************************************* DEFAULT VALUES
         */

        set_default_values(&app_params, &frame_headers, &mtu_test, &qm_test,
                           &speed_test, &test_interface, &test_params);



        /*
         ************************************************************* CLI ARGS
         */

        int16_t cli_ret = cli_args(argc, argv, &app_params, &frame_headers,
                                   &mtu_test, &qm_test, &speed_test,
                                   &test_interface, &test_params);

        if (cli_ret != EXIT_SUCCESS) {
            reset_app(&frame_headers, &qm_test, &speed_test, &test_interface);
        
            if (cli_ret == RET_EXIT_APP) {
                return EXIT_SUCCESS;
            } else {
                return cli_ret;
            }
        }

        // Check for root privs, low level socket access is required
        if (getuid() != 0) {
            printf("Must be root to use this program!\n");
            reset_app(&frame_headers, &qm_test, &speed_test, &test_interface);
            return EX_NOPERM;
        }



        /*
         ****************************************************** INTERFACE SETUP
         */

        if (setup_socket(&test_interface) != EXIT_SUCCESS) {
            int32_t errnum = errno;
            reset_app(&frame_headers, &qm_test, &speed_test, &test_interface);
            return errnum;
        }

        if (setup_socket_interface(&frame_headers, &test_interface, &test_params) != EXIT_SUCCESS)                                             
        {
            int32_t errnum = errno;
            reset_app(&frame_headers, &qm_test, &speed_test, &test_interface);
            return errnum;
        }

        update_frame_size(&frame_headers, &test_interface, &test_params);

        // Set the network interface to promiscuos mode
        if (set_interface_promisc(&test_interface) != EXIT_SUCCESS)
        {
            int32_t errnum = errno;
            reset_app(&frame_headers, &qm_test, &speed_test, &test_interface);
            return errnum;
        }

        // Declare sigint handler, TX will signal RX to reset when it quits.
        // This can not be declared until now because the interfaces must be
        // set up so that a dying gasp can be sent.
        signal (SIGINT, signal_handler);

        if (setup_frame(&app_params, &frame_headers, &test_interface, &test_params) != EXIT_SUCCESS)
        {
            int32_t errnum = errno;
            reset_app(&frame_headers, &qm_test, &speed_test, &test_interface);
            return errnum;
        }



        /*
         ******************************************************** SETTINGS SYNC
         */

        if (app_params.tx_sync == true)
            sync_settings(&app_params, &frame_headers, &mtu_test, &qm_test,
                          &speed_test, &test_interface, &test_params);

        // Pause to allow the RX host to process the sent settings
        sleep(2);

        // Rebuild the test frame headers in case any settings have been changed
        // by the TX host
        if (app_params.tx_mode != true) build_headers(&frame_headers);
        printf("Frame size is %d bytes\n", test_params.f_size_total);


        // Try to measure the TX to RX one way delay
        if (app_params.tx_delay == true)
            delay_test(&app_params, &frame_headers, &test_interface, 
                       &test_params, &qm_test);



        /*
         ****************************************************** MAIN TEST PHASE
         */

        app_params.ts_now =   time(0);
        app_params.tm_local = localtime(&app_params.ts_now);
        printf("Starting test on %s\n", asctime(app_params.tm_local));   

        if (mtu_test.enabled) {

            mtu_sweep_test(&app_params, &frame_headers, &test_interface,
                           &test_params, &mtu_test);

        } else if (qm_test.enabled) {

            latency_test(&app_params, &frame_headers, &test_interface,
                         &test_params, &qm_test);

        } else if (speed_test.f_payload_size > 0) {

            speed_test.enabled = true;
            send_custom_frame(&app_params, &frame_headers, &speed_test,
                              &test_interface, &test_params);

        } else {

            speed_test.enabled = true;
            speed_test_default(&app_params, &frame_headers, &speed_test,
                               &test_interface, &test_params);

        }
        
        app_params.ts_now = time(0);
        app_params.tm_local = localtime(&app_params.ts_now);
        printf("Ending test on %s\n\n", asctime(app_params.tm_local));

        if (remove_interface_promisc(&test_interface) != EXIT_SUCCESS)
        {
            printf("Error leaving promiscuous mode\n");
        }

        reset_app(&frame_headers, &qm_test, &speed_test, &test_interface);

        // End the testing loop if TX host
        if (app_params.tx_mode == true) testing = false;


    }


    return EXIT_SUCCESS;

}
