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
 * File: Speed Tests
 *
 */



#include "speed_tests.h"

void speed_test_prep(struct etherate *eth)
{

    build_tlv(&eth->frm, htons(TYPE_TESTFRAME), htonl(VALUE_TEST_SUB_TLV));

    // By default test until F_DURATION_DEF has passed
    eth->speed_test.testMax = (uint64_t*)&eth->params.f_duration;
    eth->speed_test.testBase = (uint64_t*)&eth->params.s_elapsed;

    if (eth->params.f_bytes > 0) ///// Move all this recuring code to a parent function
    {    
        // Testing until N bytes sent
        eth->speed_test.testMax  = &eth->params.f_bytes;
        eth->speed_test.testBase = &eth->speed_test.b_tx;

    } else if (eth->params.f_count > 0) {   
        // Testing until N frames sent
        eth->speed_test.testMax  = &eth->params.f_count;
        eth->speed_test.testBase = &eth->params.f_tx_count;

    } else if (eth->params.f_duration > 0) {
        // Testing until N seconds have passed
        eth->params.f_duration -= 1;
    }

    if (eth->app.tx_mode == true)
    {

        printf("Seconds\t\tMbps Tx\t\tMBs Tx\t\tFrmTx/s\t\tFrames Tx\n");

        if (eth->speed_test.b_tx_speed_max != B_TX_SPEED_MAX_DEF) {
            speed_test_bps(eth);
        } else if (eth->params.f_tx_count_max != F_TX_COUNT_MAX_DEF) {
            speed_test_fps(eth);
        } else if (eth->params.f_tx_dly != F_TX_DLY_DEF) {
            speed_test_pacing(eth);
        } else {
            speed_test_full(eth);
        }


    // Else, Rx mode
    } else {

        printf("Seconds\t\tMbps Rx\t\tMBs Rx\t\tFrmRx/s\t\tFrames Rx\n");

        speed_test_rx(eth);

    }

    if (eth->params.s_elapsed > 0)
    {
        eth->speed_test.b_speed_avg = (eth->speed_test.b_speed_avg / eth->params.s_elapsed);
        eth->speed_test.f_speed_avg = (eth->speed_test.f_speed_avg / eth->params.s_elapsed);
    }

    printf("Speed test complete\n");
    speed_test_results(eth);

}



void speed_test_bps(struct etherate *eth)
{

    int16_t tx_ret = 0;
    int16_t rx_len = 0;

    // Get clock times for the speed limit restriction and starting time
    clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.elapsed_time);

    // Tx test loop
    while (*eth->speed_test.testBase <= *eth->speed_test.testMax)
    {

        clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.current_time);

        // One second has passed
        if ((eth->params.current_time.tv_sec - eth->params.elapsed_time.tv_sec) >= 1)
        {
            eth->params.s_elapsed += 1;
            eth->speed_test.b_speed   = (((double)eth->speed_test.b_tx-eth->speed_test.b_tx_prev) * 8) / 1000 / 1000;
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

                // Running in ACK mode
                if (eth->params.f_ack)
                {

                    if (ntohl(*eth->frm.rx_tlv_value) == VALUE_TEST_SUB_TLV &&
                        ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_ACKINDEX)
                    {

                        eth->params.f_rx_count    += 1;
                        eth->params.f_waiting_ack = false;

                        // Record if the frame is in-order, early or late
                        if (likely(ntohll(*eth->frm.rx_sub_tlv_value) == eth->params.f_rx_count)) {
                            eth->params.f_rx_ontime  += 1;
                        } else if (ntohll(*eth->frm.rx_sub_tlv_value) > eth->params.f_rx_count) {
                            eth->params.f_rx_early   += 1;
                        } else if (ntohll(*eth->frm.rx_sub_tlv_value) < eth->params.f_rx_count) {
                            eth->params.f_rx_late    += 1;
                        }

                    } else if (ntohs(*eth->frm.rx_tlv_type) == TYPE_APPLICATION &&
                               ntohl(*eth->frm.rx_tlv_value) == VALUE_DYINGGASP) {

                        printf("\nRx host has quit\n");
                        speed_test_results(eth);
                        return;

                    // Received a non-test frame
                    } else {
                        eth->params.f_rx_other += 1;
                    }

                // Not running in ACK mode
                } else if (ntohs(*eth->frm.rx_tlv_type) == TYPE_APPLICATION &&
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

            // If it hasn't been 1 second yet, send more test frames
            if (eth->params.f_waiting_ack == false)
            {

                // Check if sending another frame exceeds the max speed configured
                if ((eth->speed_test.b_tx_speed_prev + eth->params.f_size_total) <= eth->speed_test.b_tx_speed_max)
                {

                    build_sub_tlv(&eth->frm, htons(TYPE_FRAMEINDEX), 
                                  htonll(eth->params.f_tx_count+1));

                    tx_ret = send(eth->intf.sock_fd,
                                  eth->frm.tx_buffer,
                                  eth->params.f_size_total,
                                  MSG_DONTWAIT);

                    if (tx_ret > 0)
                    {
                        eth->params.f_tx_count += 1;
                        eth->speed_test.b_tx += eth->params.f_size_total;
                        eth->speed_test.b_tx_speed_prev += eth->params.f_size_total;
                        if (eth->params.f_ack) eth->params.f_waiting_ack = true;

                    } else {

                        if (errno != EAGAIN || errno != EWOULDBLOCK)
                        {
                            perror("Speed test Tx error ");
                            return;
                        }
                        
                    }

                }

            }


        } // End of current_time.tv_sec - ts_elapsed_TIME.tv_sec

    } // *testBase<=*testMax

}



void speed_test_fps(struct etherate *eth)
{

    int16_t tx_ret = 0;
    int16_t rx_len = 0;


    // Get clock times for the speed limit restriction and starting time
    clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.elapsed_time);

    // Tx test loop
    while (*eth->speed_test.testBase <= *eth->speed_test.testMax)
    {

        clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.current_time);

        // One second has passed
        if ((eth->params.current_time.tv_sec - eth->params.elapsed_time.tv_sec) >= 1)
        {
            eth->params.s_elapsed += 1;
            eth->speed_test.b_speed   = (((double)eth->speed_test.b_tx-eth->speed_test.b_tx_prev) * 8) / 1000 / 1000;
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

                // Running in ACK mode
                if (eth->params.f_ack)
                {

                    if (ntohl(*eth->frm.rx_tlv_value) == VALUE_TEST_SUB_TLV &&
                        ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_ACKINDEX)
                    {

                        eth->params.f_rx_count    += 1;
                        eth->params.f_waiting_ack = false;

                        // Record if the frame is in-order, early or late
                        if (likely(ntohll(*eth->frm.rx_sub_tlv_value) == eth->params.f_rx_count)) {
                            eth->params.f_rx_ontime  += 1;
                        } else if (ntohll(*eth->frm.rx_sub_tlv_value) > eth->params.f_rx_count) {
                            eth->params.f_rx_early   += 1;
                        } else if (ntohll(*eth->frm.rx_sub_tlv_value) < eth->params.f_rx_count) {
                            eth->params.f_rx_late    += 1;
                        }

                    } else if (ntohs(*eth->frm.rx_tlv_type) == TYPE_APPLICATION &&
                               ntohl(*eth->frm.rx_tlv_value) == VALUE_DYINGGASP) {

                        printf("\nRx host has quit\n");
                        speed_test_results(eth);
                        return;

                    // Received a non-test frame
                    } else {
                        eth->params.f_rx_other += 1;
                    }

                // Not running in ACK mode
                } else if (ntohs(*eth->frm.rx_tlv_type) == TYPE_APPLICATION &&
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

            // If it hasn't been 1 second yet, send more test frames
            if (eth->params.f_waiting_ack == false)
            {

                // Check if sending another frame exceeds the max frame rate configured
                if (eth->params.f_tx_count - eth->params.f_tx_count_prev < eth->params.f_tx_count_max)
                {

                    build_sub_tlv(&eth->frm, htons(TYPE_FRAMEINDEX), 
                                  htonll(eth->params.f_tx_count+1));

                    tx_ret = send(eth->intf.sock_fd,
                                  eth->frm.tx_buffer,
                                  eth->params.f_size_total,
                                  MSG_DONTWAIT);

                    if (tx_ret > 0)
                    {
                        eth->params.f_tx_count += 1;
                        eth->speed_test.b_tx += eth->params.f_size_total;
                        eth->speed_test.b_tx_speed_prev += eth->params.f_size_total;
                        if (eth->params.f_ack) eth->params.f_waiting_ack = true;

                    } else {

                        if (errno != EAGAIN || errno != EWOULDBLOCK)
                        {
                            perror("Speed test Tx error ");
                            return;
                        }
                        
                    }

                }

            }


        } // End of current_time.tv_sec - ts_elapsed_TIME.tv_sec

    } // *testBase<=*testMax

}



void speed_test_full(struct etherate *eth)
{

    int16_t tx_ret = 0;
    int16_t rx_len = 0;

    // Call this once to set up the subTLV
    build_sub_tlv(&eth->frm, htons(TYPE_FRAMEINDEX), 0);

    // Get clock times for the speed limit restriction and starting time
    clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.elapsed_time);


    // Tx test loop
    while (*eth->speed_test.testBase <= *eth->speed_test.testMax)
    {

        clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.current_time);

        // One second has passed
        if ((eth->params.current_time.tv_sec - eth->params.elapsed_time.tv_sec) >= 1)
        {
            eth->params.s_elapsed += 1;
            eth->speed_test.b_speed   = (((double)eth->speed_test.b_tx-eth->speed_test.b_tx_prev) * 8) / 1000 / 1000;
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

                // Running in ACK mode
                if (eth->params.f_ack)
                {

                    if (ntohl(*eth->frm.rx_tlv_value) == VALUE_TEST_SUB_TLV &&
                        ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_ACKINDEX)
                    {

                        eth->params.f_rx_count    += 1;
                        eth->params.f_waiting_ack = false;

                        // Record if the frame is in-order, early or late
                        if (likely(ntohll(*eth->frm.rx_sub_tlv_value) == eth->params.f_rx_count)) {
                            eth->params.f_rx_ontime  += 1;
                        } else if (ntohll(*eth->frm.rx_sub_tlv_value) > eth->params.f_rx_count) {
                            eth->params.f_rx_early   += 1;
                        } else if (ntohll(*eth->frm.rx_sub_tlv_value) < eth->params.f_rx_count) {
                            eth->params.f_rx_late    += 1;
                        }

                    } else if (ntohs(*eth->frm.rx_tlv_type) == TYPE_APPLICATION &&
                               ntohl(*eth->frm.rx_tlv_value) == VALUE_DYINGGASP) {

                        printf("\nRx host has quit\n");
                        speed_test_results(eth);
                        return;

                    // Received a non-test frame
                    } else {
                        eth->params.f_rx_other += 1;
                    }

                // Not running in ACK mode
                } else if (ntohs(*eth->frm.rx_tlv_type) == TYPE_APPLICATION &&
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

            // If it hasn't been 1 second yet, send more test frames
            if (eth->params.f_waiting_ack == false)
            {

                ////// Speed up this packet update?
                build_sub_tlv(&eth->frm, htons(TYPE_FRAMEINDEX),
                              htonll(eth->params.f_tx_count+1));

                tx_ret = send(eth->intf.sock_fd,
                              eth->frm.tx_buffer,
                              eth->params.f_size_total,
                              MSG_DONTWAIT);

                if (tx_ret > 0)
                {
                    eth->params.f_tx_count += 1;
                    eth->speed_test.b_tx += eth->params.f_size_total;
                    eth->speed_test.b_tx_speed_prev += eth->params.f_size_total;
                    if (eth->params.f_ack) eth->params.f_waiting_ack = true;

                } else {

                    if (errno != EAGAIN || errno != EWOULDBLOCK)
                    {
                        perror("Speed test Tx error ");
                        return;
                    }

                }

            }

        } // End of current_time.tv_sec - ts_elapsed_TIME.tv_sec

    } // *testBase<=*testMax

}



void speed_test_pacing(struct etherate *eth)
{

    int16_t tx_ret = 0;
    int16_t rx_len = 0;
    long double current_time = 0.0;
    long double last_tx      = 0.0;

    // Get clock times for the speed limit restriction and starting time
    clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.elapsed_time);

    // Initialise the "last tx interval" for the frame pacing feature before
    // current_time to ensure the first check of "time since last tx" passes
    clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.last_tx);

    // Tx test loop
    while (*eth->speed_test.testBase <= *eth->speed_test.testMax)
    {

        clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.current_time);

        // One second has passed
        if ((eth->params.current_time.tv_sec - eth->params.elapsed_time.tv_sec) >= 1)
        {
            eth->params.s_elapsed += 1;
            eth->speed_test.b_speed   = (((double)eth->speed_test.b_tx-eth->speed_test.b_tx_prev) * 8) / 1000 / 1000;
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

                // Running in ACK mode
                if (eth->params.f_ack)
                {

                    if (ntohl(*eth->frm.rx_tlv_value) == VALUE_TEST_SUB_TLV &&
                        ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_ACKINDEX)
                    {

                        eth->params.f_rx_count    += 1;
                        eth->params.f_waiting_ack = false;

                        // Record if the frame is in-order, early or late
                        if (likely(ntohll(*eth->frm.rx_sub_tlv_value) == eth->params.f_rx_count)) {
                            eth->params.f_rx_ontime  += 1;
                        } else if (ntohll(*eth->frm.rx_sub_tlv_value) > eth->params.f_rx_count) {
                            eth->params.f_rx_early   += 1;
                        } else if (ntohll(*eth->frm.rx_sub_tlv_value) < eth->params.f_rx_count) {
                            eth->params.f_rx_late    += 1;
                        }

                    } else if (ntohs(*eth->frm.rx_tlv_type) == TYPE_APPLICATION &&
                               ntohl(*eth->frm.rx_tlv_value) == VALUE_DYINGGASP) {

                        printf("\nRx host has quit\n");
                        speed_test_results(eth);
                        return;

                    // Received a non-test frame
                    } else {
                        eth->params.f_rx_other += 1;
                    }

                // Not running in ACK mode
                } else if (ntohs(*eth->frm.rx_tlv_type) == TYPE_APPLICATION &&
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

            // If it hasn't been 1 second yet, send more test frames
            if (eth->params.f_waiting_ack == false)
            {

                // Convert the timespec values into a double. The tv_nsec
                // value loops around, it is not linearly incrementing indefinitely
                current_time = eth->params.current_time.tv_sec + ((double)eth->params.current_time.tv_nsec * 1e-9);
                last_tx = eth->params.last_tx.tv_sec + ((double)eth->params.last_tx.tv_nsec * 1e-9);

                if (current_time - last_tx > eth->params.f_tx_dly) {
                //if (pace_diff >= eth->params.f_tx_dly) {

                    // Set the time of the last frame Tx first before
                    // sending the frame, this was the frame Tx time is
                    // included in the inter-frame delay without having
                    // to know the link speed
                    clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.last_tx);

                    build_sub_tlv(&eth->frm, htons(TYPE_FRAMEINDEX), 
                                  htonll(eth->params.f_tx_count+1));
                    
                    tx_ret = send(eth->intf.sock_fd,
                                  eth->frm.tx_buffer,
                                  eth->params.f_size_total,
                                  MSG_DONTWAIT);

                    if (tx_ret > 0)
                    {
                        eth->params.f_tx_count += 1;
                        eth->speed_test.b_tx += eth->params.f_size_total;
                        eth->speed_test.b_tx_speed_prev += eth->params.f_size_total;
                        if (eth->params.f_ack) eth->params.f_waiting_ack = true;

                    } else {

                        if (errno != EAGAIN || errno != EWOULDBLOCK)
                        {
                            perror("Speed test Tx error ");
                            return;
                        }
                        
                    }

                }

            }


        } // End of current_time.tv_sec - ts_elapsed_TIME.tv_sec

    } // *testBase<=*testMax

}



void speed_test_rx(struct etherate *eth)
{

    int16_t tx_ret = 0;
    int16_t rx_len = 0;

    // Wait for the first test frame to be received before starting the test loop
    uint8_t first_frame = false;
    while (!first_frame)
    {

        rx_len = recv(eth->intf.sock_fd, eth->frm.rx_buffer,
                      eth->params.f_size_total, MSG_PEEK);

        // Check if this is an Etherate test frame
        if (ntohl(*eth->frm.rx_tlv_value) == VALUE_TEST_SUB_TLV &&
            ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_FRAMEINDEX)
        {

            first_frame = true;

        }  else {

           // If the frame is not an Etherate frame it needs to be
           // "consumed" otherwise the next MSG_PEEK will show the
           // same frame:
           rx_len = recv(eth->intf.sock_fd, eth->frm.rx_buffer,
                         eth->params.f_size_total, MSG_DONTWAIT);

        }

    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.elapsed_time);

    // Rx test loop
    while (*eth->speed_test.testBase <= *eth->speed_test.testMax)
    {

        clock_gettime(CLOCK_MONOTONIC_RAW, &eth->params.current_time);

        // If one second has passed
        if ((eth->params.current_time.tv_sec - eth->params.elapsed_time.tv_sec) >= 1)
        {
            eth->params.s_elapsed += 1;
            eth->speed_test.b_speed   = (double)(eth->speed_test.b_rx - eth->speed_test.b_rx_prev) * 8 / 1000000;
            eth->speed_test.b_rx_prev = eth->speed_test.b_rx;
            eth->speed_test.f_speed   = (eth->params.f_rx_count - eth->params.f_rx_count_prev);
            eth->params.f_rx_count_prev = eth->params.f_rx_count;

            printf("%" PRIu64 "\t\t%.2f\t\t%" PRIu64 "\t\t%" PRIu64 "\t\t%" PRIu64 "\n",
                   eth->params.s_elapsed,
                   eth->speed_test.b_speed,
                   (eth->speed_test.b_rx / 1024) / 1024,
                   (eth->speed_test.f_speed),
                   eth->params.f_rx_count);

            if (eth->speed_test.b_speed > eth->speed_test.b_speed_max)
                eth->speed_test.b_speed_max = eth->speed_test.b_speed;

            if (eth->speed_test.f_speed > eth->speed_test.f_speed_max)
                eth->speed_test.f_speed_max = eth->speed_test.f_speed;

            eth->speed_test.b_speed_avg += eth->speed_test.b_speed;
            eth->speed_test.f_speed_avg += eth->speed_test.f_speed;
            eth->params.elapsed_time.tv_sec = eth->params.current_time.tv_sec;
            eth->params.elapsed_time.tv_nsec = eth->params.current_time.tv_nsec;

        }

        // Poll has been disabled in favour of a non-blocking recv (for now)
        rx_len = recv(eth->intf.sock_fd,
                      eth->frm.rx_buffer,
                      eth->params.f_size_total,
                      MSG_DONTWAIT);

        if (rx_len > 0)
        {

            // Check if this is an Etherate test frame
            if (likely(ntohl(*eth->frm.rx_tlv_value) == VALUE_TEST_SUB_TLV &&
                ntohs(*eth->frm.rx_sub_tlv_type) == TYPE_FRAMEINDEX))
            {

                // If a VLAN tag is used Linux helpfully strips it off
                if (eth->frm.vlan_id != VLAN_ID_DEF) rx_len += 4;

                // Update test stats
                eth->params.f_rx_count += 1;
                eth->speed_test.b_rx += rx_len;

                // Record if the frame is in-order, early or late
                if (likely(ntohll(*eth->frm.rx_sub_tlv_value) == eth->params.f_rx_count)) {
                    eth->params.f_rx_ontime  += 1;
                } else if (ntohll(*eth->frm.rx_sub_tlv_value) > eth->params.f_rx_count) {
                    eth->params.f_rx_early   += 1;
                } else if (ntohll(*eth->frm.rx_sub_tlv_value) < eth->params.f_rx_count) {
                    eth->params.f_rx_late    += 1;
                }

                // If running in ACK mode Rx needs to ACK to Tx host
                if (eth->params.f_ack)
                {

                    build_sub_tlv(&eth->frm, htons(TYPE_ACKINDEX),
                                  *eth->frm.rx_sub_tlv_value);

                    tx_ret = send(eth->intf.sock_fd,
                                  eth->frm.tx_buffer,
                                  eth->params.f_size_total,
                                  MSG_DONTWAIT);

                    if (tx_ret > 0)
                    {
                        eth->params.f_tx_count += 1;
                    } else {

                        if (errno != EAGAIN || errno != EWOULDBLOCK)
                        {
                            perror("Speed test Tx error ");
                            return;
                        }
                        
                    }

                    
                }

            } else {
                // Received a non-test frame
                eth->params.f_rx_other += 1;
            }

            // Check if Tx host has quit/died;
            if (unlikely(ntohl(*eth->frm.rx_tlv_value) == VALUE_DYINGGASP))
            {

                printf("\nTx host has quit\n");
                speed_test_results(eth);
                return;

            }
                  

        } else { // rx_len <= 0
            if (errno != EAGAIN || errno != EWOULDBLOCK)
                perror("Speed test Rx error ");
        }


    } // *testBase<=*testMax

}