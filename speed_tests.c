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
 * File: Speed Test Functions
 *
 * File Contents:
 * void speed_test_prep()
 * void speed_test_bps()
 * void speed_test_fps()
 * void speed_test_full()
 * void speed_test_pacing()
 * void speed_test_rx()
 *
 */



#include "speed_tests.h"

void speed_test_prep(struct app_params *app_params,
                struct frame_headers *frame_headers,
                struct speed_test *speed_test,
                struct test_interface *test_interface,
                struct test_params *test_params)
{

    build_tlv(frame_headers, htons(TYPE_TESTFRAME), htonl(VALUE_TEST_SUB_TLV));

    // By default test until F_DURATION_DEF has passed
    speed_test->testMax = (uint64_t*)&test_params->f_duration;
    speed_test->testBase = (uint64_t*)&test_params->s_elapsed;

    if (test_params->f_bytes > 0) ///// Move all this recuring code to a parent function
    {    
        // Testing until N bytes sent
        speed_test->testMax  = &test_params->f_bytes;
        speed_test->testBase = &speed_test->b_tx;

    } else if (test_params->f_count > 0) {   
        // Testing until N frames sent
        speed_test->testMax  = &test_params->f_count;
        speed_test->testBase = &test_params->f_tx_count;

    } else if (test_params->f_duration > 0) {
        // Testing until N seconds have passed
        test_params->f_duration -= 1;
    }

    if (app_params->tx_mode == true)
    {

        printf("Seconds\t\tMbps Tx\t\tMBs Tx\t\tFrmTx/s\t\tFrames Tx\n");

        if (speed_test->b_tx_speed_max != B_TX_SPEED_MAX_DEF) {
            speed_test_bps(app_params, frame_headers, speed_test,
                           test_interface, test_params);
        } else if (test_params->f_tx_count_max != F_TX_COUNT_MAX_DEF) {
            speed_test_fps(app_params, frame_headers, speed_test,
                           test_interface, test_params);
        } else if (test_params->f_tx_dly != F_TX_DLY_DEF) {
            speed_test_pacing(app_params, frame_headers, speed_test,
                              test_interface, test_params);
        } else {
            speed_test_full(app_params, frame_headers, speed_test,
                            test_interface, test_params);
        }


    // Else, Rx mode
    } else {

        printf("Seconds\t\tMbps Rx\t\tMBs Rx\t\tFrmRx/s\t\tFrames Rx\n");

        speed_test_rx(app_params, frame_headers, speed_test, test_interface,
                      test_params);

    }

    if (test_params->s_elapsed > 0)
    {
        speed_test->b_speed_avg = (speed_test->b_speed_avg/test_params->s_elapsed);
        speed_test->f_speed_avg = (speed_test->f_speed_avg/test_params->s_elapsed);
    }

    printf("Speed test complete\n");
    speed_test_results(speed_test, test_params, app_params->tx_mode);

}



void speed_test_bps(struct app_params *app_params,
                    struct frame_headers *frame_headers,
                    struct speed_test *speed_test,
                    struct test_interface *test_interface,
                    struct test_params *test_params)
{

    int16_t tx_ret = 0;
    int16_t rx_len = 0;

    // Get clock times for the speed limit restriction and starting time
    clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->elapsed_time);

    // Tx test loop
    while (*speed_test->testBase <= *speed_test->testMax)
    {

        clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->current_time);

        // One second has passed
        if ((test_params->current_time.tv_sec - test_params->elapsed_time.tv_sec) >= 1)
        {
            test_params->s_elapsed += 1;
            speed_test->b_speed   = (((double)speed_test->b_tx-speed_test->b_tx_prev) * 8) / 1000 / 1000;
            speed_test->b_tx_prev = speed_test->b_tx;
            speed_test->f_speed   = (test_params->f_tx_count - test_params->f_tx_count_prev);
            test_params->f_tx_count_prev = test_params->f_tx_count;

            printf("%" PRIu64 "\t\t%.2f\t\t%" PRIu64 "\t\t%" PRIu64"\t\t%" PRIu64 "\n",
                   test_params->s_elapsed,
                   speed_test->b_speed,
                   (speed_test->b_tx / 1024) / 1024,
                   (speed_test->f_speed),
                   test_params->f_tx_count);

            if (speed_test->b_speed > speed_test->b_speed_max)
                speed_test->b_speed_max = speed_test->b_speed;

            if (speed_test->f_speed > speed_test->f_speed_max)
                speed_test->f_speed_max = speed_test->f_speed;

            speed_test->b_speed_avg += speed_test->b_speed;
            speed_test->f_speed_avg += speed_test->f_speed;
            test_params->elapsed_time.tv_sec  = test_params->current_time.tv_sec;
            test_params->elapsed_time.tv_nsec = test_params->current_time.tv_nsec;

            speed_test->b_tx_speed_prev = 0;

        } else {

            // Poll has been disabled in favour of a non-blocking recvfrom (for now)
            rx_len = recvfrom(test_interface->sock_fd, frame_headers->rx_buffer,
                              test_params->f_size_total, MSG_DONTWAIT, NULL, NULL);

            if (rx_len > 0) {

                // Running in ACK mode
                if (test_params->f_ack)
                {

                    if (ntohl(*frame_headers->rx_tlv_value) == VALUE_TEST_SUB_TLV &&
                        ntohs(*frame_headers->rx_sub_tlv_type) == TYPE_ACKINDEX)
                    {

                        test_params->f_rx_count    += 1;
                        test_params->f_waiting_ack = false;

                    if (ntohll(*frame_headers->rx_sub_tlv_value) == (speed_test->f_speed_max+1)) {
                        test_params->f_rx_ontime  += 1;
                        speed_test->f_speed_max += 1;
                    } else if (ntohll(*frame_headers->rx_sub_tlv_value) > (test_params->f_rx_count)) {
                        speed_test->f_speed_max = ntohll(*frame_headers->rx_sub_tlv_value);
                        test_params->f_rx_early += 1;
                    } else if (ntohll(*frame_headers->rx_sub_tlv_value) <= test_params->f_rx_count) {
                        test_params->f_rx_late += 1;
                    }

                    } else if (ntohs(*frame_headers->rx_tlv_type) == TYPE_APPLICATION &&
                               ntohl(*frame_headers->rx_tlv_value) == VALUE_DYINGGASP) {

                        printf("\nRx host has quit\n");
                        speed_test_results(speed_test, test_params, app_params->tx_mode);
                        return;

                    // Received a non-test frame
                    } else {
                        test_params->f_rx_other += 1;
                    }

                // Not running in ACK mode
                } else if (ntohs(*frame_headers->rx_tlv_type) == TYPE_APPLICATION &&
                           ntohll(*frame_headers->rx_tlv_value) == VALUE_DYINGGASP) {

                    printf("\nRx host has quit\n");
                    speed_test_results(speed_test, test_params, app_params->tx_mode);
                    return;

                // Received a non-test frame
                } else {
                    test_params->f_rx_other += 1;
                }

            } // rx_len > 0

            // If it hasn't been 1 second yet, send more test frames
            if (test_params->f_waiting_ack == false)
            {

                // Check if sending another frame exceeds the max speed configured
                if ((speed_test->b_tx_speed_prev + test_params->f_size_total) <= speed_test->b_tx_speed_max)
                {

                    build_sub_tlv(frame_headers, htons(TYPE_FRAMEINDEX), 
                                  htonll(test_params->f_tx_count+1));

                    tx_ret = sendto(test_interface->sock_fd,
                                    frame_headers->tx_buffer,
                                    test_params->f_size_total, MSG_DONTWAIT, 
                                    (struct sockaddr*)&test_interface->sock_addr,
                                    sizeof(test_interface->sock_addr));

                    if (tx_ret <= 0)
                    {
                        perror("Speed test Tx error ");
                        return;
                    }


                    test_params->f_tx_count += 1;
                    speed_test->b_tx += test_params->f_size_total;
                    speed_test->b_tx_speed_prev += test_params->f_size_total;
                    if (test_params->f_ack) test_params->f_waiting_ack = true;

                }

            }


        } // End of current_time.tv_sec - ts_elapsed_TIME.tv_sec

    } // *testBase<=*testMax

}



void speed_test_fps(struct app_params *app_params,
                    struct frame_headers *frame_headers,
                    struct speed_test *speed_test,
                    struct test_interface *test_interface,
                    struct test_params *test_params)
{

    int16_t tx_ret = 0;
    int16_t rx_len = 0;


    // Get clock times for the speed limit restriction and starting time
    clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->elapsed_time);

    // Tx test loop
    while (*speed_test->testBase <= *speed_test->testMax)
    {

        clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->current_time);

        // One second has passed
        if ((test_params->current_time.tv_sec - test_params->elapsed_time.tv_sec) >= 1)
        {
            test_params->s_elapsed += 1;
            speed_test->b_speed   = (((double)speed_test->b_tx-speed_test->b_tx_prev) * 8) / 1000 / 1000;
            speed_test->b_tx_prev = speed_test->b_tx;
            speed_test->f_speed   = (test_params->f_tx_count - test_params->f_tx_count_prev);
            test_params->f_tx_count_prev = test_params->f_tx_count;

            printf("%" PRIu64 "\t\t%.2f\t\t%" PRIu64 "\t\t%" PRIu64"\t\t%" PRIu64 "\n",
                   test_params->s_elapsed,
                   speed_test->b_speed,
                   (speed_test->b_tx / 1024) / 1024,
                   (speed_test->f_speed),
                   test_params->f_tx_count);

            if (speed_test->b_speed > speed_test->b_speed_max)
                speed_test->b_speed_max = speed_test->b_speed;

            if (speed_test->f_speed > speed_test->f_speed_max)
                speed_test->f_speed_max = speed_test->f_speed;

            speed_test->b_speed_avg += speed_test->b_speed;
            speed_test->f_speed_avg += speed_test->f_speed;
            test_params->elapsed_time.tv_sec  = test_params->current_time.tv_sec;
            test_params->elapsed_time.tv_nsec = test_params->current_time.tv_nsec;

            speed_test->b_tx_speed_prev = 0;

        } else {

            // Poll has been disabled in favour of a non-blocking recvfrom (for now)
            rx_len = recvfrom(test_interface->sock_fd, frame_headers->rx_buffer,
                              test_params->f_size_total, MSG_DONTWAIT, NULL, NULL);

            if (rx_len > 0) {

                // Running in ACK mode
                if (test_params->f_ack)
                {

                    if (ntohl(*frame_headers->rx_tlv_value) == VALUE_TEST_SUB_TLV &&
                        ntohs(*frame_headers->rx_sub_tlv_type) == TYPE_ACKINDEX)
                    {

                        test_params->f_rx_count    += 1;
                        test_params->f_waiting_ack = false;

                    if (ntohll(*frame_headers->rx_sub_tlv_value) == (speed_test->f_speed_max+1)) {
                        test_params->f_rx_ontime  += 1;
                        speed_test->f_speed_max += 1;
                    } else if (ntohll(*frame_headers->rx_sub_tlv_value) > (test_params->f_rx_count)) {
                        speed_test->f_speed_max = ntohll(*frame_headers->rx_sub_tlv_value);
                        test_params->f_rx_early += 1;
                    } else if (ntohll(*frame_headers->rx_sub_tlv_value) <= test_params->f_rx_count) {
                        test_params->f_rx_late += 1;
                    }

                    } else if (ntohs(*frame_headers->rx_tlv_type) == TYPE_APPLICATION &&
                               ntohl(*frame_headers->rx_tlv_value) == VALUE_DYINGGASP) {

                        printf("\nRx host has quit\n");
                        speed_test_results(speed_test, test_params, app_params->tx_mode);
                        return;

                    // Received a non-test frame
                    } else {
                        test_params->f_rx_other += 1;
                    }

                // Not running in ACK mode
                } else if (ntohs(*frame_headers->rx_tlv_type) == TYPE_APPLICATION &&
                           ntohll(*frame_headers->rx_tlv_value) == VALUE_DYINGGASP) {

                    printf("\nRx host has quit\n");
                    speed_test_results(speed_test, test_params, app_params->tx_mode);
                    return;

                // Received a non-test frame
                } else {
                    test_params->f_rx_other += 1;
                }

            } // rx_len > 0

            // If it hasn't been 1 second yet, send more test frames
            if (test_params->f_waiting_ack == false)
            {

                // Check if sending another frame exceeds the max frame rate configured
                if (test_params->f_tx_count - test_params->f_tx_count_prev < test_params->f_tx_count_max)
                {

                    build_sub_tlv(frame_headers, htons(TYPE_FRAMEINDEX), 
                                  htonll(test_params->f_tx_count+1));

                    tx_ret = sendto(test_interface->sock_fd,
                                    frame_headers->tx_buffer,
                                    test_params->f_size_total, MSG_DONTWAIT,
                                    (struct sockaddr*)&test_interface->sock_addr,
                                    sizeof(test_interface->sock_addr));

                    if (tx_ret <= 0)
                    {
                        perror("Speed test Tx error ");
                        return;
                    }


                    test_params->f_tx_count += 1;
                    speed_test->b_tx += test_params->f_size_total;
                    speed_test->b_tx_speed_prev += test_params->f_size_total;
                    if (test_params->f_ack) test_params->f_waiting_ack = true;

                }

            }


        } // End of current_time.tv_sec - ts_elapsed_TIME.tv_sec

    } // *testBase<=*testMax

}



void speed_test_full(struct app_params *app_params,
                struct frame_headers *frame_headers,
                struct speed_test *speed_test,
                struct test_interface *test_interface,
                struct test_params *test_params)
{

    int16_t tx_ret = 0;
    int16_t rx_len = 0;

    // Get clock times for the speed limit restriction and starting time
    clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->elapsed_time);


    // Tx test loop
    while (*speed_test->testBase <= *speed_test->testMax)
    {

        clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->current_time);

        // One second has passed
        if ((test_params->current_time.tv_sec - test_params->elapsed_time.tv_sec) >= 1)
        {
            test_params->s_elapsed += 1;
            speed_test->b_speed   = (((double)speed_test->b_tx-speed_test->b_tx_prev) * 8) / 1000 / 1000;
            speed_test->b_tx_prev = speed_test->b_tx;
            speed_test->f_speed   = (test_params->f_tx_count - test_params->f_tx_count_prev);
            test_params->f_tx_count_prev = test_params->f_tx_count;

            printf("%" PRIu64 "\t\t%.2f\t\t%" PRIu64 "\t\t%" PRIu64"\t\t%" PRIu64 "\n",
                   test_params->s_elapsed,
                   speed_test->b_speed,
                   (speed_test->b_tx / 1024) / 1024,
                   (speed_test->f_speed),
                   test_params->f_tx_count);

            if (speed_test->b_speed > speed_test->b_speed_max)
                speed_test->b_speed_max = speed_test->b_speed;

            if (speed_test->f_speed > speed_test->f_speed_max)
                speed_test->f_speed_max = speed_test->f_speed;

            speed_test->b_speed_avg += speed_test->b_speed;
            speed_test->f_speed_avg += speed_test->f_speed;
            test_params->elapsed_time.tv_sec  = test_params->current_time.tv_sec;
            test_params->elapsed_time.tv_nsec = test_params->current_time.tv_nsec;

            speed_test->b_tx_speed_prev = 0;

        } else {

            // Poll has been disabled in favour of a non-blocking recvfrom (for now)
            rx_len = recvfrom(test_interface->sock_fd, frame_headers->rx_buffer,
                              test_params->f_size_total, MSG_DONTWAIT, NULL, NULL);

            if (rx_len > 0) {

                // Running in ACK mode
                if (test_params->f_ack)
                {

                    if (ntohl(*frame_headers->rx_tlv_value) == VALUE_TEST_SUB_TLV &&
                        ntohs(*frame_headers->rx_sub_tlv_type) == TYPE_ACKINDEX)
                    {

                        test_params->f_rx_count    += 1;
                        test_params->f_waiting_ack = false;

                    if (ntohll(*frame_headers->rx_sub_tlv_value) == (speed_test->f_speed_max+1)) {
                        test_params->f_rx_ontime  += 1;
                        speed_test->f_speed_max += 1;
                    } else if (ntohll(*frame_headers->rx_sub_tlv_value) > (test_params->f_rx_count)) {
                        speed_test->f_speed_max = ntohll(*frame_headers->rx_sub_tlv_value);
                        test_params->f_rx_early += 1;
                    } else if (ntohll(*frame_headers->rx_sub_tlv_value) <= test_params->f_rx_count) {
                        test_params->f_rx_late += 1;
                    }

                    } else if (ntohs(*frame_headers->rx_tlv_type) == TYPE_APPLICATION &&
                               ntohl(*frame_headers->rx_tlv_value) == VALUE_DYINGGASP) {

                        printf("\nRx host has quit\n");
                        speed_test_results(speed_test, test_params, app_params->tx_mode);
                        return;

                    // Received a non-test frame
                    } else {
                        test_params->f_rx_other += 1;
                    }

                // Not running in ACK mode
                } else if (ntohs(*frame_headers->rx_tlv_type) == TYPE_APPLICATION &&
                           ntohll(*frame_headers->rx_tlv_value) == VALUE_DYINGGASP) {

                    printf("\nRx host has quit\n");
                    speed_test_results(speed_test, test_params, app_params->tx_mode);
                    return;

                // Received a non-test frame
                } else {
                    test_params->f_rx_other += 1;
                }

            } // rx_len > 0

            // If it hasn't been 1 second yet, send more test frames
            if (test_params->f_waiting_ack == false)
            {

                ////// Speed up this packet update?
                build_sub_tlv(frame_headers, htons(TYPE_FRAMEINDEX),
                              htonll(test_params->f_tx_count+1));

                tx_ret = sendto(test_interface->sock_fd,
                                frame_headers->tx_buffer,
                                test_params->f_size_total, MSG_DONTWAIT, 
                                (struct sockaddr*)&test_interface->sock_addr,
                                sizeof(test_interface->sock_addr));

                if (tx_ret <= 0)
                {
                    perror("Speed test Tx error ");
                    return;
                }


                test_params->f_tx_count += 1;
                speed_test->b_tx += test_params->f_size_total;
                speed_test->b_tx_speed_prev += test_params->f_size_total;
                if (test_params->f_ack) test_params->f_waiting_ack = true;

            }

        } // End of current_time.tv_sec - ts_elapsed_TIME.tv_sec

    } // *testBase<=*testMax

}



void speed_test_pacing(struct app_params *app_params,
                       struct frame_headers *frame_headers,
                       struct speed_test *speed_test,
                       struct test_interface *test_interface,
                       struct test_params *test_params)
{

    int16_t tx_ret = 0;
    int16_t rx_len = 0;
    long double current_time = 0.0;
    long double last_tx      = 0.0;

    // Get clock times for the speed limit restriction and starting time
    clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->elapsed_time);

    // Initialise the "last tx interval" for the frame pacing feature before
    // current_time to ensure the first check of "time since last tx" passes
    clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->last_tx);

    // Tx test loop
    while (*speed_test->testBase <= *speed_test->testMax)
    {

        clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->current_time);

        // One second has passed
        if ((test_params->current_time.tv_sec - test_params->elapsed_time.tv_sec) >= 1)
        {
            test_params->s_elapsed += 1;
            speed_test->b_speed   = (((double)speed_test->b_tx-speed_test->b_tx_prev) * 8) / 1000 / 1000;
            speed_test->b_tx_prev = speed_test->b_tx;
            speed_test->f_speed   = (test_params->f_tx_count - test_params->f_tx_count_prev);
            test_params->f_tx_count_prev = test_params->f_tx_count;

            printf("%" PRIu64 "\t\t%.2f\t\t%" PRIu64 "\t\t%" PRIu64"\t\t%" PRIu64 "\n",
                   test_params->s_elapsed,
                   speed_test->b_speed,
                   (speed_test->b_tx / 1024) / 1024,
                   (speed_test->f_speed),
                   test_params->f_tx_count);

            if (speed_test->b_speed > speed_test->b_speed_max)
                speed_test->b_speed_max = speed_test->b_speed;

            if (speed_test->f_speed > speed_test->f_speed_max)
                speed_test->f_speed_max = speed_test->f_speed;

            speed_test->b_speed_avg += speed_test->b_speed;
            speed_test->f_speed_avg += speed_test->f_speed;
            test_params->elapsed_time.tv_sec  = test_params->current_time.tv_sec;
            test_params->elapsed_time.tv_nsec = test_params->current_time.tv_nsec;

            speed_test->b_tx_speed_prev = 0;

        } else {

            // Poll has been disabled in favour of a non-blocking recvfrom (for now)
            rx_len = recvfrom(test_interface->sock_fd, frame_headers->rx_buffer,
                              test_params->f_size_total, MSG_DONTWAIT, NULL, NULL);

            if (rx_len > 0) {

                // Running in ACK mode
                if (test_params->f_ack)
                {

                    if (ntohl(*frame_headers->rx_tlv_value) == VALUE_TEST_SUB_TLV &&
                        ntohs(*frame_headers->rx_sub_tlv_type) == TYPE_ACKINDEX)
                    {

                        test_params->f_rx_count    += 1;
                        test_params->f_waiting_ack = false;

                    if (ntohll(*frame_headers->rx_sub_tlv_value) == (speed_test->f_speed_max+1)) {
                        test_params->f_rx_ontime  += 1;
                        speed_test->f_speed_max += 1;
                    } else if (ntohll(*frame_headers->rx_sub_tlv_value) > (test_params->f_rx_count)) {
                        speed_test->f_speed_max = ntohll(*frame_headers->rx_sub_tlv_value);
                        test_params->f_rx_early += 1;
                    } else if (ntohll(*frame_headers->rx_sub_tlv_value) <= test_params->f_rx_count) {
                        test_params->f_rx_late += 1;
                    }

                    } else if (ntohs(*frame_headers->rx_tlv_type) == TYPE_APPLICATION &&
                               ntohl(*frame_headers->rx_tlv_value) == VALUE_DYINGGASP) {

                        printf("\nRx host has quit\n");
                        speed_test_results(speed_test, test_params, app_params->tx_mode);
                        return;

                    // Received a non-test frame
                    } else {
                        test_params->f_rx_other += 1;
                    }

                // Not running in ACK mode
                } else if (ntohs(*frame_headers->rx_tlv_type) == TYPE_APPLICATION &&
                           ntohll(*frame_headers->rx_tlv_value) == VALUE_DYINGGASP) {

                    printf("\nRx host has quit\n");
                    speed_test_results(speed_test, test_params, app_params->tx_mode);
                    return;

                // Received a non-test frame
                } else {
                    test_params->f_rx_other += 1;
                }

            } // rx_len > 0

            // If it hasn't been 1 second yet, send more test frames
            if (test_params->f_waiting_ack == false)
            {

                // Convert the timespec values into a double. The tv_nsec
                // value loops around, it is not linearly incrementing indefinitely
                current_time = test_params->current_time.tv_sec + ((double)test_params->current_time.tv_nsec * 1e-9);
                last_tx = test_params->last_tx.tv_sec + ((double)test_params->last_tx.tv_nsec * 1e-9);

                if (current_time - last_tx > test_params->f_tx_dly) {
                //if (pace_diff >= test_params->f_tx_dly) {

                    // Set the time of the last frame Tx first before
                    // sending the frame, this was the frame Tx time is
                    // included in the inter-frame delay without having
                    // to know the link speed
                    clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->last_tx);

                    build_sub_tlv(frame_headers, htons(TYPE_FRAMEINDEX), 
                                  htonll(test_params->f_tx_count+1));

                    tx_ret = sendto(test_interface->sock_fd,
                                    frame_headers->tx_buffer,
                                    test_params->f_size_total, MSG_DONTWAIT, 
                                    (struct sockaddr*)&test_interface->sock_addr,
                                    sizeof(test_interface->sock_addr));

                    if (tx_ret <= 0)
                    {
                        perror("Speed test Tx error ");
                        return;
                    }

                    test_params->f_tx_count += 1;
                    speed_test->b_tx += test_params->f_size_total;
                    speed_test->b_tx_speed_prev += test_params->f_size_total;
                    if (test_params->f_ack) test_params->f_waiting_ack = true;

                }

            }


        } // End of current_time.tv_sec - ts_elapsed_TIME.tv_sec

    } // *testBase<=*testMax

}



void speed_test_rx(struct app_params *app_params,
                   struct frame_headers *frame_headers,
                   struct speed_test *speed_test,
                   struct test_interface *test_interface,
                   struct test_params *test_params)
{

    int16_t tx_ret = 0;
    int16_t rx_len = 0;

    // Wait for the first test frame to be received before starting the test loop
    uint8_t first_frame = false;
    while (!first_frame)
    {

        rx_len = recvfrom(test_interface->sock_fd, frame_headers->rx_buffer,
                          test_params->f_size_total, MSG_PEEK, NULL, NULL);

        // Check if this is an Etherate test frame
        if (ntohl(*frame_headers->rx_tlv_value) == VALUE_TEST_SUB_TLV &&
            ntohs(*frame_headers->rx_sub_tlv_type) == TYPE_FRAMEINDEX)
        {

            first_frame = true;

        }  else {

           // If the frame is not an Etherate frame it needs to be
           // "consumed" otherwise the next MSG_PEEK will show the
           // same frame:
           rx_len = recvfrom(test_interface->sock_fd, frame_headers->rx_buffer,
                             test_params->f_size_total, MSG_DONTWAIT, NULL,
                             NULL);

        }

    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->elapsed_time);

    // Rx test loop
    while (speed_test->testBase <= speed_test->testMax)
    {

        clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->current_time);

        // If one second has passed
        if ((test_params->current_time.tv_sec - test_params->elapsed_time.tv_sec) >= 1)
        {
            test_params->s_elapsed += 1;
            speed_test->b_speed   = (double)(speed_test->b_rx - speed_test->b_rx_prev) * 8 / 1000000;
            speed_test->b_rx_prev = speed_test->b_rx;
            speed_test->f_speed   = (test_params->f_rx_count - test_params->f_rx_count_prev);
            test_params->f_rx_count_prev = test_params->f_rx_count;

            printf("%" PRIu64 "\t\t%.2f\t\t%" PRIu64 "\t\t%" PRIu64 "\t\t%" PRIu64 "\n",
                   test_params->s_elapsed,
                   speed_test->b_speed,
                   (speed_test->b_rx / 1024) / 1024,
                   (speed_test->f_speed),
                   test_params->f_rx_count);

            if (speed_test->b_speed > speed_test->b_speed_max)
                speed_test->b_speed_max = speed_test->b_speed;

            if (speed_test->f_speed > speed_test->f_speed_max)
                speed_test->f_speed_max = speed_test->f_speed;

            speed_test->b_speed_avg += speed_test->b_speed;
            speed_test->f_speed_avg += speed_test->f_speed;
            test_params->elapsed_time.tv_sec = test_params->current_time.tv_sec;
            test_params->elapsed_time.tv_nsec = test_params->current_time.tv_nsec;

        }

        // Poll has been disabled in favour of a non-blocking recvfrom (for now)
        rx_len = recvfrom(test_interface->sock_fd,
                          frame_headers->rx_buffer,
                          test_params->f_size_total,
                          MSG_DONTWAIT, NULL, NULL); ///// Change to recv as NULL, NULL aren't used

        if (rx_len > 0)
        {

            // Check if this is an Etherate test frame
            if (likely(ntohl(*frame_headers->rx_tlv_value) == VALUE_TEST_SUB_TLV &&
                ntohs(*frame_headers->rx_sub_tlv_type) == TYPE_FRAMEINDEX))
            {

                // If a VLAN tag is used Linux helpfully strips it off
                if (frame_headers->vlan_id != VLAN_ID_DEF) rx_len += 4;

                // Update test stats
                test_params->f_rx_count += 1;
                speed_test->b_rx += rx_len;

                // Record if the frame is in-order, early or late
                if (likely(ntohll(*frame_headers->rx_sub_tlv_value) == (speed_test->f_speed_max + 1))) {
                    test_params->f_rx_ontime  += 1;
                    speed_test->f_speed_max += 1;
                } else if (ntohll(*frame_headers->rx_sub_tlv_value) > (test_params->f_rx_count)) {
                    speed_test->f_speed_max = ntohll(*frame_headers->rx_sub_tlv_value);
                    test_params->f_rx_early   += 1;
                } else if (ntohll(*frame_headers->rx_sub_tlv_value) <= test_params->f_rx_count) {
                    test_params->f_rx_late    += 1;
                }

                // If running in ACK mode Rx needs to ACK to Tx host
                if (test_params->f_ack)
                {

                    build_sub_tlv(frame_headers, htons(TYPE_ACKINDEX),
                                  *frame_headers->rx_sub_tlv_value);

                    tx_ret = sendto(test_interface->sock_fd,
                                    frame_headers->tx_buffer,
                                    frame_headers->length+frame_headers->sub_tlv_size, 0, 
                                    (struct sockaddr*)&test_interface->sock_addr,
                                    sizeof(test_interface->sock_addr));

                    if (tx_ret <= 0)
                    {
                        perror("Speed test Tx error ");
                        return;
                    }


                    test_params->f_tx_count += 1;
                    
                }

            } else {
                // Received a non-test frame
                test_params->f_rx_other += 1;
            }

            // Check if Tx host has quit/died;
            if (unlikely(ntohl(*frame_headers->rx_tlv_value) == VALUE_DYINGGASP))
            {

                printf("\nTx host has quit\n");
                speed_test_results(speed_test, test_params, app_params->tx_mode);
                return;

            }
                  

        } // rx_len > 0


    } // *testBase<=*testMax

}