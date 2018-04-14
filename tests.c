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
 * File: Etherate Test Functions
 *
 */



#include "tests.h"

void delay_test(struct etherate *eth)
{
// Calculate the delay between Tx and Rx hosts. The uptime is exchanged twice
// to estimate the delay between the hosts. Then the process is repeated so
// an average can be taken


    long double UPTIME;
    uint64_t    tx_uptime;
    int16_t     tx_ret;
    int16_t     rx_len;
    uint8_t     WAITING;


    build_tlv(&eth->frm, htons(TYPE_TESTFRAME), htonl(VALUE_TEST_SUB_TLV));

    printf("Calculating delay between hosts...\n");

    if (eth->app.tx_mode == true)
    {

        for (uint32_t i=0; i < eth->qm_test.delay_test_count; i += 1)
        {

            // Get the current time and send it to Rx
            clock_gettime(CLOCK_MONOTONIC_RAW, &eth->qm_test.ts_rtt);

            // Convert it to double then long long (uint64t_) for transmission
            UPTIME = eth->qm_test.ts_rtt.tv_sec + ((double)eth->qm_test.ts_rtt.tv_nsec * 1e-9);
            tx_uptime = roundl(UPTIME * 1000000000.0);

            build_sub_tlv(&eth->frm, htons(TYPE_DELAY1), htonll(tx_uptime));

            tx_ret = send(eth->intf.sock_fd,
                          eth->frm.tx_buffer,
                          eth->frm.length + eth->frm.sub_tlv_size,
                          0);


            if (tx_ret <= 0)
            {
                perror("Delay test Tx error ");
                return;
            }

            // Repeat
            clock_gettime(CLOCK_MONOTONIC_RAW, &eth->qm_test.ts_rtt);

            UPTIME = eth->qm_test.ts_rtt.tv_sec + ((double)eth->qm_test.ts_rtt.tv_nsec * 1e-9);
            tx_uptime = roundl(UPTIME * 1000000000.0);

            build_sub_tlv(&eth->frm, htons(TYPE_DELAY2), htonll(tx_uptime));

            tx_ret = send(eth->intf.sock_fd,
                          eth->frm.tx_buffer,
                          eth->frm.length + eth->frm.sub_tlv_size,
                          0);

            if (tx_ret <= 0)
            {
                perror("Delay test Tx error ");
                return;
            }


            // Wait for Rx to send back delay value
            WAITING = true;
            while (WAITING)
            {
                rx_len = read(eth->intf.sock_fd,
                              eth->frm.rx_buffer,
                              eth->params.f_size_total);

                if (rx_len > 0)
                {

                    if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_DELAY)
                    {
                        eth->qm_test.delay_results[i] = (ntohll(*eth->frm.rx_sub_tlv_value) / 1000000000.0);
                        WAITING = false;
                    }

                } else if (rx_len < 0) {

                    perror("Delay test Tx error ");
                    return;

                }

            }

        } // End delay Tx loop


        // Let the receiver know all delay values have been received
        build_tlv(&eth->frm, htons(TYPE_TESTFRAME), htonl(VALUE_DELAY_END));

        build_sub_tlv(&eth->frm, htons(TYPE_TESTFRAME),
                      htonll(VALUE_DELAY_END));

        tx_ret = send(eth->intf.sock_fd,
                      eth->frm.tx_buffer,
                      eth->frm.length + eth->frm.sub_tlv_size,
                      0);

        if (tx_ret <= 0)
        {
            perror("Delay test Tx error ");
            return;
        }


        double delay_avg = 0;

        for (uint32_t i=0; i < eth->qm_test.delay_test_count; i += 1)
        {
            delay_avg += eth->qm_test.delay_results[i];
        }

        delay_avg = (delay_avg/eth->qm_test.delay_test_count);

        printf("Tx to Rx delay calculated as %.9f seconds\n\n", delay_avg);


    // This is the Rx host
    } else {


        // These values are used to calculate the delay between Tx and Rx hosts
        eth->qm_test.time_tx_1 = (double*)calloc(eth->qm_test.delay_test_count, sizeof(double));
        eth->qm_test.time_tx_2 = (double*)calloc(eth->qm_test.delay_test_count, sizeof(double));
        eth->qm_test.time_rx_1 = (double*)calloc(eth->qm_test.delay_test_count, sizeof(double));
        eth->qm_test.time_rx_2 = (double*)calloc(eth->qm_test.delay_test_count, sizeof(double));
        eth->qm_test.time_tx_diff = (double*)calloc(eth->qm_test.delay_test_count, sizeof(double));
        eth->qm_test.time_rx_diff = (double*)calloc(eth->qm_test.delay_test_count, sizeof(double));

        if (eth->qm_test.time_tx_1 == NULL ||
            eth->qm_test.time_tx_2 == NULL ||
            eth->qm_test.time_rx_1 == NULL ||
            eth->qm_test.time_rx_2 == NULL ||
            eth->qm_test.time_tx_diff == NULL ||
            eth->qm_test.time_rx_diff == NULL)
        {
            perror("Couldn't allocate delay buffers ");
            return;
        }

        uint32_t delay_index = 0;
        
        WAITING = true;

        while(WAITING)
        {

            rx_len = read(eth->intf.sock_fd,
                          eth->frm.rx_buffer,
                          eth->params.f_size_total);

            if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_DELAY1)
            {

                // Get the time Rx is receiving Tx's sent time figure
                clock_gettime(CLOCK_MONOTONIC_RAW, &eth->qm_test.ts_rtt);

                // Record the time Rx received this Tx value
                eth->qm_test.time_rx_1[delay_index] = eth->qm_test.ts_rtt.tv_sec + ((double)eth->qm_test.ts_rtt.tv_nsec * 1e-9);
                // Record the Tx value received
                eth->qm_test.time_tx_1[delay_index] = (ntohll(*eth->frm.rx_sub_tlv_value) / 1000000000.0);


            } else if (ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_DELAY2) {


                // Grab the Rx time and sent Tx value for the second time
                clock_gettime(CLOCK_MONOTONIC_RAW, &eth->qm_test.ts_rtt);
                eth->qm_test.time_rx_2[delay_index] = eth->qm_test.ts_rtt.tv_sec + ((double)eth->qm_test.ts_rtt.tv_nsec * 1e-9);
                eth->qm_test.time_tx_2[delay_index] = (ntohll(*eth->frm.rx_sub_tlv_value) / 1000000000.0);

                // Calculate the delay
                eth->qm_test.time_tx_diff[delay_index] = eth->qm_test.time_tx_2[delay_index] - eth->qm_test.time_tx_1[delay_index];
                eth->qm_test.time_rx_diff[delay_index] = eth->qm_test.time_rx_2[delay_index] - eth->qm_test.time_rx_1[delay_index];

                // Rarely a negative value is calculated
                if (eth->qm_test.time_rx_diff[delay_index] - eth->qm_test.time_tx_diff[delay_index] > 0) {
                    eth->qm_test.delay_results[delay_index] = eth->qm_test.time_rx_diff[delay_index] - eth->qm_test.time_tx_diff[delay_index];
                // This value returned is minus and thus invalid
                } else {

                    eth->qm_test.delay_results[delay_index] = 0;
                }


                // Send the calculated delay back to the Tx host
                build_sub_tlv(&eth->frm, htons(TYPE_DELAY),
                              htonll(roundl(eth->qm_test.delay_results[delay_index]*1000000000.0)));

                tx_ret = send(eth->intf.sock_fd,
                              eth->frm.tx_buffer,
                              eth->frm.length + eth->frm.sub_tlv_size,
                              0);

                if (tx_ret <= 0)
                {
                    perror("Delay test Rx error ");
                    return;
                }


                delay_index += 1;


            } else if (ntohl(*eth->frm.rx_tlv_value) == VALUE_DELAY_END) {

                WAITING = false;

                double delay_avg = 0;

                for (uint32_t i=0; i < eth->qm_test.delay_test_count; i += 1)
                {
                    delay_avg += eth->qm_test.delay_results[i];
                }

                delay_avg = (delay_avg/eth->qm_test.delay_test_count);

                printf("Tx to Rx delay calculated as %.9f seconds\n\n", delay_avg);

            }


        } // End of WAITING

        free(eth->qm_test.time_tx_1);
        free(eth->qm_test.time_tx_2);
        free(eth->qm_test.time_rx_1);
        free(eth->qm_test.time_rx_2);
        free(eth->qm_test.time_tx_diff);
        free(eth->qm_test.time_rx_diff);
        eth->qm_test.time_tx_1    = NULL;
        eth->qm_test.time_tx_2    = NULL;
        eth->qm_test.time_rx_1    = NULL;
        eth->qm_test.time_rx_2    = NULL;
        eth->qm_test.time_tx_diff = NULL;
        eth->qm_test.time_rx_diff = NULL;

    } // End of Rx mode

}



void mtu_sweep_test(struct etherate *eth)
{

    // Check the interface MTU
    int32_t phy_mtu = get_interface_mtu_by_name(&eth->intf);

    if (eth->mtu_test.mtu_tx_max > phy_mtu) {

        printf("Running MTU sweep from %" PRIu16 " to %" PRIi32 " (interface max)\n",
               eth->mtu_test.mtu_tx_min, phy_mtu);
        eth->mtu_test.mtu_tx_max = phy_mtu;

    } else {

        printf("Running MTU sweep from %" PRIu16 " to %" PRIu16 "\n",
               eth->mtu_test.mtu_tx_min, eth->mtu_test.mtu_tx_max);
    
    }


    int16_t tx_ret = 0;
    int16_t rx_len = 0;

    build_tlv(&eth->frm, htons(TYPE_TESTFRAME), htonl(VALUE_TEST_SUB_TLV));

    if (eth->app.tx_mode == true) {

        uint16_t mtu_tx_current   = 0;
        uint16_t mtu_ack_previous = 0;
        uint16_t mtu_ack_current  = 0;
        uint8_t  WAITING          = true;

        for (mtu_tx_current = eth->mtu_test.mtu_tx_min; mtu_tx_current <= eth->mtu_test.mtu_tx_max; mtu_tx_current += 1)
        {

            // Get the current time
            clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.elapsed_time);

            build_sub_tlv(&eth->frm, htons(TYPE_FRAMEINDEX), htonll(mtu_tx_current));

            tx_ret = send(eth->intf.sock_fd,
                          eth->frm.tx_buffer,
                          eth->frm.length + mtu_tx_current,
                          0);

            if (tx_ret <=0)
            {
                perror("MTU test Tx error ");
                return;

            } else {
                eth->params.f_tx_count += 1;
            }


            WAITING = true;

            // Wait for the ACK from back from Rx ///// Document the max wait time (3 seconds)
            while(WAITING)
            {

                // Get the current time
                clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.current_time);

                // Poll has been disabled in favour of a non-blocking recv (for now)
                rx_len = recv(eth->intf.sock_fd,
                              eth->frm.rx_buffer,
                              eth->mtu_test.mtu_tx_max,
                              MSG_DONTWAIT);

                if (rx_len > 0) {


                    if (ntohl(*eth->frm.rx_tlv_value) == VALUE_TEST_SUB_TLV &&
                        ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_ACKINDEX)
                    {

                        eth->params.f_rx_count += 1;

                        // Get the MTU size Rx is ACK'ing
                        mtu_ack_current = (uint32_t)ntohll(*eth->frm.rx_sub_tlv_value);
                        if (mtu_ack_current > eth->mtu_test.largest) 
                        {
                            eth->mtu_test.largest = mtu_ack_current;
                        }


                        if (mtu_ack_current < mtu_ack_previous)
                        {
                            // Frame received out of order, later than expected
                            eth->params.f_rx_late += 1;
                        } else if (mtu_ack_previous == 0) {
                            // First frame
                            eth->params.f_rx_ontime += 1;
                            mtu_ack_previous = mtu_ack_current;
                        } else if (mtu_ack_current > (mtu_ack_previous + 2)) {
                            // Frame received out of order, earlier than expected
                            eth->params.f_rx_early += 1;
                            mtu_ack_previous = mtu_ack_current;
                        } else if (mtu_ack_current == (mtu_ack_previous + 1) &&
                                   mtu_ack_current < eth->mtu_test.mtu_tx_max) {
                            // Fame received in order
                            eth->params.f_rx_ontime += 1;
                            mtu_ack_previous = mtu_ack_current;
                            WAITING = false;
                        } else if (mtu_ack_current == (mtu_ack_previous + 1)) {
                            // Frame received in order
                            eth->params.f_rx_ontime += 1;
                        } else if (mtu_ack_current == mtu_ack_previous) {
                            // Frame received in order
                            eth->params.f_rx_ontime += 1;
                        }


                    // A non-test frame was received
                    } else {

                        eth->params.f_rx_other += 1;

                    }


                } else { // rx_len > 0
                    if (errno != EAGAIN || errno != EWOULDBLOCK)
                    {
                        perror("Speed test Tx error ");
                        return;
                    }
                }

                // If 1 second has passed inside this loop,
                // Assume the ACK is lost/dropped
                if ((eth->params.current_time.tv_sec - 
                     eth->params.elapsed_time.tv_sec) >= 1) WAITING = false;

            } // End of WAITING

            
        } // End of Tx transmit

        printf("MTU sweep test complete\n");
        mtu_sweep_test_results(eth);


    // Running in Rx mode
    } else {

        uint32_t mtu_rx_previous = 0;
        uint32_t mtu_rx_current  = 0;
        uint8_t  WAITING         = true;


        while(true)
        {
        
            clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.elapsed_time);

            WAITING = true;

            while (WAITING)
            {

                // Get the current time
                clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.current_time);

                // Check for test frame from Tx
                rx_len = recv(eth->intf.sock_fd,
                              eth->frm.rx_buffer,
                              eth->mtu_test.mtu_tx_max,
                              MSG_DONTWAIT);

                if (rx_len > 0) {

                    if (ntohl(*eth->frm.rx_tlv_value) == VALUE_TEST_SUB_TLV &&
                        ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_FRAMEINDEX)
                    {

                        eth->params.f_rx_count += 1;

                        // Get the MTU size Tx is sending
                        mtu_rx_current = (uint32_t)ntohll(*eth->frm.rx_sub_tlv_value);

                        if (mtu_rx_current > eth->mtu_test.largest)
                        {

                            eth->mtu_test.largest = mtu_rx_current;

                            // ACK that back to Tx as new largest MTU received
                            build_sub_tlv(&eth->frm, htons(TYPE_ACKINDEX),
                                          htonll(mtu_rx_current));

                            tx_ret = send(eth->intf.sock_fd,
                                          eth->frm.tx_buffer,
                                          eth->frm.length + eth->frm.sub_tlv_size,
                                          0);

                            if (tx_ret <=0)
                            {
                                perror("MTU test Tx error ");
                                return;
                            } else {
                                eth->params.f_tx_count += 1;
                            }


                        } // mtu_rx_current > eth->mtu_test.largest


                        if (mtu_rx_current < mtu_rx_previous)
                        {
                            // Frame received out of order, later than expected
                            eth->params.f_rx_late += 1;
                        } else if (mtu_rx_previous == 0) {
                            // First frame
                            eth->params.f_rx_ontime += 1;
                            mtu_rx_previous = mtu_rx_current;
                        } else if (mtu_rx_current > (mtu_rx_previous + 2)) {
                            // Frame received out of order, earlier than expected
                            eth->params.f_rx_early += 1;
                            mtu_rx_previous = mtu_rx_current;
                        } else if (mtu_rx_current == (mtu_rx_previous + 1) &&
                                   mtu_rx_current < eth->mtu_test.mtu_tx_max) {
                            // Frame received in order
                            eth->params.f_rx_ontime += 1;
                            mtu_rx_previous = mtu_rx_current;
                            WAITING = false;
                        } else if (mtu_rx_current == (mtu_rx_previous + 1)) {
                            // Frame received in order
                            mtu_rx_previous = mtu_rx_current;
                            eth->params.f_rx_ontime += 1;
                        } else if (mtu_rx_current == mtu_rx_previous) {
                            // Frame received in order
                            mtu_rx_previous = mtu_rx_current;
                            eth->params.f_rx_ontime += 1;
                        }


                    // A non-test frame was recieved
                    } else {

                        eth->params.f_rx_other += 1;

                    } //End of TEST_FRAME


                } else { // rx_len > 0
                    if (errno != EAGAIN || errno != EWOULDBLOCK)
                    {
                        perror("Speed test Tx error ");
                        return;
                    }
                }


                // If Rx has received the largest MTU Tx hoped to send,
                // the MTU sweep test is over
                if (eth->mtu_test.largest == eth->mtu_test.mtu_tx_max)
                {

                    printf("MTU sweep test complete\n");
                    mtu_sweep_test_results(eth);

                    return;

                }

                // 5 seconds have passed without any MTU sweep frames being receeved,
                // assume the max MTU has been exceeded
                if ((eth->params.current_time.tv_sec-eth->params.elapsed_time.tv_sec) >= 5)  ///// Document this timeout
                {

                    printf("Timeout waiting for MTU sweep frames from Tx, "
                           "ending the test.\nEnding test.\n");

                    mtu_sweep_test_results(eth);
                    return;

                }


            } // End of WAITING

        } // while(true)


    } // End of Tx/Rx mode


}



void latency_test(struct etherate *eth)
{

    build_tlv(&eth->frm, htons(TYPE_TESTFRAME), htonl(VALUE_TEST_SUB_TLV));

    int16_t     tx_ret       = 0;
    int16_t     rx_len       = 0;
    long double uptime_1     = 0.0;
    long double uptime_2     = 0.0;
    long double uptime_rx    = 0.0;
    long double rtt          = 0.0;
    long double rtt_prev     = 0.0;
    long double jitter       = 0.0;
    long double interval     = 0.0;
    uint64_t    tx_uptime    = 0;
    uint8_t     WAITING      = false;
    uint8_t     echo_waiting = false;    
    // These are used to check for the ping/pong timeout and interval,
    // converting the tv_sec and tv_nsec values into a "single" number
    long double current_time = 0.0;
    long double elapsed_time = 0.0;
    long double max_time     = 0.0;


    uint64_t *testBase, *testMax;


    if (eth->app.tx_mode == true)
    {

        if (eth->params.f_count > 0) {   
            // Testing until N rtt measurements
            testMax  = &eth->params.f_count;
            testBase = &eth->params.f_tx_count;

        } else {
            // Testing until N seconds have passed
            if (eth->params.f_duration > 0) eth->params.f_duration -= 1;
            testMax  = (uint64_t*)&eth->params.f_duration;
            testBase = (uint64_t*)&eth->params.s_elapsed;
        }


        clock_gettime(CLOCK_MONOTONIC_RAW, &eth->qm_test.ts_start);

        printf("No.\trtt\t\tJitter\n");

        while (*testBase<=*testMax)
        {
            
            clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.current_time);

            uptime_1 = eth->params.current_time.tv_sec + ((double)eth->params.current_time.tv_nsec * 1e-9);
            tx_uptime = roundl(uptime_1 * 1000000000.0);

            build_sub_tlv(&eth->frm, htons(TYPE_PING), htonll(tx_uptime));

            tx_ret = send(eth->intf.sock_fd,
                          eth->frm.tx_buffer,
                          eth->frm.length + eth->frm.sub_tlv_size, 0);

            if (tx_ret <=0 )
            {
                perror("Latency test Tx error ");
                return;
            }


            eth->params.f_tx_count += 1;
            eth->qm_test.test_count     += 1;

            printf("%" PRIu32 ":", eth->qm_test.test_count);

            WAITING      = true;
            echo_waiting = true;


            while (WAITING)
            {

                // Get the current time
                clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.elapsed_time);


                // Check if 1 second has passed to increment test duration
                if (eth->params.elapsed_time.tv_sec-
                    eth->qm_test.ts_start.tv_sec >= 1) {

                    clock_gettime(CLOCK_MONOTONIC_RAW, &eth->qm_test.ts_start);

                    eth->params.s_elapsed += 1;

                }


                // Poll has been disabled in favour of a non-blocking recv (for now)
                rx_len = recv(eth->intf.sock_fd,
                              eth->frm.rx_buffer,
                              eth->params.f_size_total,
                              MSG_DONTWAIT);

                if (rx_len > 0) {

                    // Received an echo reply/pong
                    if (ntohl(*eth->frm.rx_tlv_value) == VALUE_TEST_SUB_TLV &&
                        ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_PONG)
                    {

                        uptime_2 = eth->params.elapsed_time.tv_sec + 
                                   ((double)eth->params.elapsed_time.tv_nsec * 1e-9);

                        // Check it's the time value originally sent
                        uptime_rx = (double)ntohll(*eth->frm.rx_sub_tlv_value) / 1000000000.0;

                        if (uptime_rx == uptime_1)
                        {

                            rtt = uptime_2-uptime_1;

                            if (rtt < eth->qm_test.rtt_min)
                            {
                                eth->qm_test.rtt_min = rtt;
                            } else if (rtt > eth->qm_test.rtt_max) {
                                eth->qm_test.rtt_max = rtt;
                            }

                            jitter = fabsl(rtt-rtt_prev);

                            if (jitter < eth->qm_test.jitter_min)
                            {
                                eth->qm_test.jitter_min = jitter;
                            } else if (jitter > eth->qm_test.jitter_max) {
                                eth->qm_test.jitter_max = jitter;
                            }

                            printf ("\t%.9Lfs\t%.9Lfs\n", rtt, jitter);

                            rtt_prev = rtt;

                            echo_waiting = false;

                            eth->params.f_rx_count += 1;

                        // We may have received a frame with "the right bits in the right place"
                        } else {
                            eth->params.f_rx_other += 1;
                        }


                    // Check if Rx host has quit/died
                    } else if (ntohl(*eth->frm.rx_tlv_value) == VALUE_DYINGGASP) {

                        printf("\nRx host has quit\n");
                        latency_test_results(eth);
                        return;

                    } else {

                        eth->params.f_rx_other += 1;

                    }

                } // rx_len > 0


                // Convert the timespec values into a double. The tv_nsec
                // value loops around, it is not linearly incrementing indefinitely
                elapsed_time = eth->params.elapsed_time.tv_sec + ((double)eth->params.elapsed_time.tv_nsec * 1e-9);
                current_time = eth->params.current_time.tv_sec + ((double)eth->params.current_time.tv_nsec * 1e-9);
                max_time = eth->qm_test.timeout_sec + ((double)eth->qm_test.timeout_nsec * 1e-9);

                // If Tx is waiting for echo reply, check if the echo reply has timed out
                if (echo_waiting) {
                    if ((elapsed_time - current_time) > max_time) {

                        printf("\t*\n");
                        echo_waiting = false;
                        eth->qm_test.timeout_count += 1;

                    }
                }


                // Check if the echo interval has passed (time to send another ping)
                max_time = eth->qm_test.interval_sec + ((double)eth->qm_test.interval_nsec * 1e-9);

                if ((elapsed_time - current_time) > max_time) {
                    WAITING  = false;
                }


            } // WAITING=true


        } // testBase<=testMax

        printf("Link quality test complete\n");
        latency_test_results(eth);


    // Else, Rx mode
    } else {


        if (eth->params.f_count > 0) {   
            // Testing until N rtt measurements
            testMax  = &eth->params.f_count;
            testBase = &eth->params.f_rx_count;

        } else {
            // Testing until N seconds have passed
            if (eth->params.f_duration > 0) eth->params.f_duration -= 1;
            testMax  = (uint64_t*)&eth->params.f_duration;
            testBase = (uint64_t*)&eth->params.s_elapsed;
        }


        clock_gettime(CLOCK_MONOTONIC_RAW, &eth->qm_test.ts_start);

        printf("No.\tEcho Interval\n");


        // Wait for the first test frame to be received before starting the test loop
        uint8_t first_frame = false;
        while (!first_frame)
        {

            rx_len = recv(eth->intf.sock_fd, eth->frm.rx_buffer,
                          eth->params.f_size_total, MSG_PEEK);

            // Check if this is an Etherate test frame
            if (ntohl(*eth->frm.rx_tlv_value) == VALUE_TEST_SUB_TLV &&
                ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_PING)
            {

                first_frame = true;
                eth->qm_test.test_count += 1;
                printf("%" PRIu32 ":\t", eth->qm_test.test_count);

            } else {

               // If the frame is not an Etherate frame it needs to be
               // "consumed" otherwise the next MSG_PEEK will show the
               // same frame:
               rx_len = read(eth->intf.sock_fd, eth->frm.rx_buffer,
                             eth->params.f_size_total);

               if (rx_len < 0)
                   perror("Latency test Rx error ");

            }

        }


        // Rx test loop
        while (*testBase<=*testMax)
        {

            clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.current_time);

            WAITING      = true;
            echo_waiting = true;


            while (WAITING)
            {

                // Get the current time
                clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.elapsed_time);

                // Check if 1 second has passed to increment test duration
                if (eth->params.elapsed_time.tv_sec-
                    eth->qm_test.ts_start.tv_sec >= 1) {

                    clock_gettime(CLOCK_MONOTONIC_RAW, &eth->qm_test.ts_start);

                    eth->params.s_elapsed += 1;

                }


                rx_len = recv(eth->intf.sock_fd,
                              eth->frm.rx_buffer,
                              eth->params.f_size_total,
                              MSG_DONTWAIT);


                if (rx_len > 0) {

                    if ( ntohl(*eth->frm.rx_tlv_value) == VALUE_TEST_SUB_TLV &&
                         ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_PING )
                    {

                        // Time Rx received this value
                        uptime_2 = eth->params.elapsed_time.tv_sec + 
                                   ((double)eth->params.elapsed_time.tv_nsec * 1e-9);

                        // Send the Tx uptime value back to the Tx host
                        build_sub_tlv(&eth->frm, htons(TYPE_PONG),
                                      *eth->frm.rx_sub_tlv_value);

                        tx_ret = send(eth->intf.sock_fd,
                                      eth->frm.tx_buffer,
                                      eth->frm.length + eth->frm.sub_tlv_size,
                                      0);

                        if (tx_ret <= 0)
                        {
                            perror("Latency test Rx error ");
                            return;
                        }


                        eth->params.f_rx_count += 1;
                        eth->params.f_tx_count += 1;

                        interval = fabsl(uptime_2-uptime_1);


                        // Interval between receiving this uptime value and the last
                        if (uptime_1 != 0.0)
                        {
                            printf("%.9Lfs\n", interval);
                        } else {
                            printf("0.0\n");
                        }
                        uptime_1 = uptime_2;

                        if (interval < eth->qm_test.interval_min)
                        {
                            eth->qm_test.interval_min = interval;
                        } else if (interval > eth->qm_test.interval_max) {
                            eth->qm_test.interval_max = interval;
                        }

                        echo_waiting = false;
                        eth->qm_test.test_count += 1;
                        printf("%" PRIu32 ":\t", eth->qm_test.test_count);


                    } else {

                        eth->params.f_rx_other += 1;

                    }

                } // rx_len > 0


                // Check if the echo request has timed out
                elapsed_time = eth->params.elapsed_time.tv_sec + ((double)eth->params.elapsed_time.tv_nsec * 1e-9);
                current_time = eth->params.current_time.tv_sec + ((double)eth->params.current_time.tv_nsec * 1e-9);
                max_time = eth->qm_test.timeout_sec + ((double)eth->qm_test.timeout_nsec * 1e-9);
                
                if (echo_waiting == true) {
                    if ((elapsed_time - current_time) > max_time) {

                        printf("*\n");
                        eth->qm_test.timeout_count += 1;
                        echo_waiting = false;

                    }
                    
                }


                // Check if the echo interval has passed (time to receive another ping)
                elapsed_time = eth->params.elapsed_time.tv_sec + ((double)eth->params.elapsed_time.tv_nsec * 1e-9);
                current_time = eth->params.current_time.tv_sec + ((double)eth->params.current_time.tv_nsec * 1e-9);
                max_time = eth->qm_test.interval_sec + ((double)eth->qm_test.interval_nsec * 1e-9);

                if ((elapsed_time - current_time) > max_time) {
                    WAITING = false;
                }


                // Check if Tx host has quit/died;
                if (ntohl(*eth->frm.rx_tlv_value) == VALUE_DYINGGASP)
                {

                    printf("\nTx host has quit\n");
                    latency_test_results(eth);
                    return;

                }


            } // WAITING=true


        } // testBase<=testMax

        printf("Link quality test complete\n");
        latency_test_results(eth);

    } // End of Tx/Rx

 }



///// Merge into other speed test functionm
void send_custom_frame(struct etherate *eth)
{

    int16_t tx_ret = 0;
    int16_t rx_len = 0;

    printf("Seconds\t\tMbps Tx\t\tMBs Tx\t\tFrmTx/s\t\tFrames Tx\n");

    // By default test until f_duration_DEF has passed
    uint64_t *testMax = (uint64_t*)&eth->params.f_duration;
    uint64_t *testBase = (uint64_t*)&eth->params.s_elapsed;

    if (eth->params.f_bytes > 0)
    {    
        // Testing until N bytes sent
        testMax  = &eth->params.f_bytes;
        testBase = &eth->speed_test.b_tx;

    } else if (eth->params.f_count > 0) {   
        // Testing until N frames sent
        testMax  = &eth->params.f_count;
        testBase = &eth->params.f_tx_count;

    } else if (eth->params.f_duration > 0) {
        // Testing until N seconds have passed
        eth->params.f_duration -= 1;
    }


    // Get clock times for the speed limit restriction and starting time
    clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.elapsed_time);

    // Tx test loop
    while (*testBase<=*testMax)
    {

        clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.current_time);

        // One second has passed
        if ((eth->params.current_time.tv_sec - eth->params.elapsed_time.tv_sec) >= 1)
        {
            eth->params.s_elapsed += 1;
            eth->speed_test.b_speed   = ((double)(eth->speed_test.b_tx - eth->speed_test.b_tx_prev) * 8) / 1000 / 1000;
            eth->speed_test.b_tx_prev = eth->speed_test.b_tx;
            eth->speed_test.f_speed   = (eth->params.f_tx_count - eth->params.f_tx_count_prev);
            eth->params.f_tx_count_prev = eth->params.f_tx_count;

            printf("%" PRIu64 "\t\t%.2f\t\t%" PRIu64 "\t\t%" PRIu64"\t\t%" PRIu64 "\n",
                   eth->params.s_elapsed,
                   eth->speed_test.b_speed,
                   (eth->speed_test.b_tx / 1024) / 1024,
                   (eth->speed_test.f_speed),
                   eth->params.f_tx_count);

            if (eth->speed_test.b_speed > eth->speed_test.b_speed_max)
                eth->speed_test.b_speed_max = eth->speed_test.b_speed;

            if (eth->speed_test.f_speed > eth->speed_test.f_speed_max)
                eth->speed_test.f_speed_max = eth->speed_test.f_speed;

            eth->speed_test.b_speed_avg += eth->speed_test.b_speed;
            eth->speed_test.f_speed_avg += eth->speed_test.f_speed;
            eth->params.elapsed_time.tv_sec  = eth->params.current_time.tv_sec;
            eth->params.elapsed_time.tv_nsec = eth->params.current_time.tv_nsec;

            eth->speed_test.b_tx_speed_prev = 0;

        } else {

            // Poll has been disabled in favour of a non-blocking recv (for now)
            rx_len = recv(eth->intf.sock_fd, eth->frm.rx_buffer,
                          eth->params.f_size_total, MSG_DONTWAIT);

            if (rx_len > 0) {

                // Check for dying gasp from Rx host
                if (ntohs(*eth->frm.rx_tlv_type) == TYPE_APPLICATION &&
                    ntohll(*eth->frm.rx_tlv_value) == VALUE_DYINGGASP) {

                    printf("\nRx host has quit\n");
                    speed_test_results(eth);
                    return;

                // Received a non-test frame
                } else {
                    eth->params.f_rx_other += 1;
                }

            } else { // rx_len <= 0
                if (errno != EAGAIN || errno != EWOULDBLOCK)
                    perror("Speed test Rx error ");
            }


            // Check if a max speed has been configured
            if (eth->speed_test.b_tx_speed_max != B_TX_SPEED_MAX_DEF)
            {

                // Check if sending another frame exceeds the max speed configured
                if ((eth->speed_test.b_tx_speed_prev + eth->speed_test.f_payload_size)
                    <= eth->speed_test.b_tx_speed_max)
                {


                    tx_ret = send(eth->intf.sock_fd,
                                  eth->speed_test.f_payload,
                                  eth->speed_test.f_payload_size,
                                  MSG_DONTWAIT);


                    if (tx_ret > 0)
                    {
                        eth->params.f_tx_count += 1;
                        eth->speed_test.b_tx += eth->speed_test.f_payload_size;
                        eth->speed_test.b_tx_speed_prev += eth->speed_test.f_payload_size;

                    } else {

                        if (errno != EAGAIN || errno != EWOULDBLOCK)
                        {
                            perror("Speed test Tx error ");
                            return;
                        }
                        
                    }

                }

            // Check if a max frames per second limit is configured
            } else if (eth->params.f_tx_count_max != F_TX_COUNT_MAX_DEF) {

                // Check if sending another frame exceeds the max frame rate configured
                if ((eth->params.f_tx_count - eth->params.f_tx_count_prev) <
                    eth->params.f_tx_count_max)
                {

                    tx_ret = send(eth->intf.sock_fd,
                                  eth->speed_test.f_payload,
                                  eth->speed_test.f_payload_size,
                                  MSG_DONTWAIT);

                    if (tx_ret > 0)
                    {
                        eth->params.f_tx_count += 1;
                        eth->speed_test.b_tx += eth->speed_test.f_payload_size;
                        eth->speed_test.b_tx_speed_prev += eth->speed_test.f_payload_size;

                    } else {

                        if (errno != EAGAIN || errno != EWOULDBLOCK)
                        {
                            perror("Speed test Tx error ");
                            return;
                        }
                        
                    }


                }

            // Else there are no speed restrictions
            } else {

                tx_ret = send(eth->intf.sock_fd,
                              eth->speed_test.f_payload,
                              eth->speed_test.f_payload_size,
                              MSG_DONTWAIT);

                if (tx_ret > 0)
                {
                    eth->params.f_tx_count += 1;
                    eth->speed_test.b_tx += eth->speed_test.f_payload_size;
                    eth->speed_test.b_tx_speed_prev += eth->speed_test.f_payload_size;

                } else {

                    if (errno != EAGAIN || errno != EWOULDBLOCK)
                    {
                        perror("Speed test Tx error ");
                        return;
                    }
                    
                }

            }

        } // End of current_time.tv_sec - elapsed_time.tv_sec

    }


    if (eth->params.s_elapsed > 0)
    {
        eth->speed_test.b_speed_avg = (eth->speed_test.b_speed_avg/eth->params.s_elapsed);
        eth->speed_test.f_speed_avg = (eth->speed_test.f_speed_avg/eth->params.s_elapsed);
    }

    printf("Speed test complete\n");
    speed_test_results(eth);

 }
