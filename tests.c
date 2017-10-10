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
 * File: Etherate Test Functions
 *
 * File Contents:
 * void delay_test()
 * void mtu_sweep_test()
 * void latency_test()
 * void speed_test_default()
 * void send_custom_frame()
 *
 */



#include "tests.h"

void delay_test(struct app_params *app_params,
                struct frame_headers *frame_headers,
                struct test_interface *test_interface,
                struct test_params *test_params,
                struct qm_test *qm_test)
{
// Calculate the delay between Tx and Rx hosts. The uptime is exchanged twice
// to estimate the delay between the hosts. Then the process is repeated so
// an average can be taken


    long double UPTIME;
    uint64_t    tx_uptime;
    int16_t     tx_ret;
    int16_t     rx_len;
    uint8_t     WAITING;


    build_tlv(frame_headers, htons(TYPE_TESTFRAME), htonl(VALUE_TEST_SUB_TLV));

    printf("Calculating delay between hosts...\n");

    if (app_params->tx_mode == true)
    {

        for (uint32_t i=0; i < qm_test->delay_test_count; i += 1)
        {

            // Get the current time and send it to Rx
            clock_gettime(CLOCK_MONOTONIC_RAW, &qm_test->ts_rtt);

            // Convert it to double then long long (uint64t_) for transmission
            UPTIME = qm_test->ts_rtt.tv_sec + ((double)qm_test->ts_rtt.tv_nsec * 1e-9);
            tx_uptime = roundl(UPTIME * 1000000000.0);

            build_sub_tlv(frame_headers, htons(TYPE_DELAY1), htonll(tx_uptime));

            tx_ret = sendto(test_interface->sock_fd,
                            frame_headers->tx_buffer,
                            frame_headers->length+frame_headers->sub_tlv_size,
                            0,
                            (struct sockaddr*)&test_interface->sock_addr,
                            sizeof(test_interface->sock_addr));

            if (tx_ret <= 0)
            {
                perror("Delay test Tx error ");
                return;
            }

            // Repeat
            clock_gettime(CLOCK_MONOTONIC_RAW, &qm_test->ts_rtt);

            UPTIME = qm_test->ts_rtt.tv_sec + ((double)qm_test->ts_rtt.tv_nsec * 1e-9);
            tx_uptime = roundl(UPTIME * 1000000000.0);

            build_sub_tlv(frame_headers, htons(TYPE_DELAY2), htonll(tx_uptime));

            tx_ret = sendto(test_interface->sock_fd,
                            frame_headers->tx_buffer,
                            frame_headers->length+frame_headers->sub_tlv_size,
                            0,
                            (struct sockaddr*)&test_interface->sock_addr,
                            sizeof(test_interface->sock_addr));

            if (tx_ret <= 0)
            {
                perror("Delay test Tx error ");
                return;
            }


            // Wait for Rx to send back delay value
            WAITING = true;
            while (WAITING)
            {
                rx_len = recvfrom(test_interface->sock_fd,
                                  frame_headers->rx_buffer,
                                  test_params->f_size_total,
                                  0, NULL, NULL);

                if (rx_len > 0)
                {

                    if (ntohs(*frame_headers->rx_sub_tlv_type) == TYPE_DELAY)
                    {
                        qm_test->delay_results[i] = (ntohll(*frame_headers->rx_sub_tlv_value) / 1000000000.0);
                        WAITING = false;
                    }

                } else if (rx_len < 0) {

                    perror("Delay test Tx error ");
                    return;

                }

            }

        } // End delay Tx loop


        // Let the receiver know all delay values have been received
        build_tlv(frame_headers, htons(TYPE_TESTFRAME), htonl(VALUE_DELAY_END));

        build_sub_tlv(frame_headers, htons(TYPE_TESTFRAME),
                      htonll(VALUE_DELAY_END));

        tx_ret = sendto(test_interface->sock_fd,
                        frame_headers->tx_buffer,
                        frame_headers->length+frame_headers->sub_tlv_size,
                        0,
                        (struct sockaddr*)&test_interface->sock_addr,
                        sizeof(test_interface->sock_addr));

        if (tx_ret <= 0)
        {
            perror("Delay test Tx error ");
            return;
        }


        double delay_avg = 0;

        for (uint32_t i=0; i < qm_test->delay_test_count; i += 1)
        {
            delay_avg += qm_test->delay_results[i];
        }

        delay_avg = (delay_avg/qm_test->delay_test_count);

        printf("Tx to Rx delay calculated as %.9f seconds\n\n", delay_avg);


    // This is the Rx host
    } else {


        // These values are used to calculate the delay between Tx and Rx hosts
        qm_test->time_tx_1 = (double*)calloc(qm_test->delay_test_count, sizeof(double));
        qm_test->time_tx_2 = (double*)calloc(qm_test->delay_test_count, sizeof(double));
        qm_test->time_rx_1 = (double*)calloc(qm_test->delay_test_count, sizeof(double));
        qm_test->time_rx_2 = (double*)calloc(qm_test->delay_test_count, sizeof(double));
        qm_test->time_tx_diff = (double*)calloc(qm_test->delay_test_count, sizeof(double));
        qm_test->time_rx_diff = (double*)calloc(qm_test->delay_test_count, sizeof(double));

        uint32_t delay_index = 0;
        
        WAITING = true;

        while(WAITING)
        {

            rx_len = recvfrom(test_interface->sock_fd,
                              frame_headers->rx_buffer,
                              test_params->f_size_total,
                              0, NULL, NULL);

            if (ntohs(*frame_headers->rx_sub_tlv_type) == TYPE_DELAY1)
            {

                // Get the time Rx is receiving Tx's sent time figure
                clock_gettime(CLOCK_MONOTONIC_RAW, &qm_test->ts_rtt);

                // Record the time Rx received this Tx value
                qm_test->time_rx_1[delay_index] = qm_test->ts_rtt.tv_sec + ((double)qm_test->ts_rtt.tv_nsec * 1e-9);
                // Record the Tx value received
                qm_test->time_tx_1[delay_index] = (ntohll(*frame_headers->rx_sub_tlv_value) / 1000000000.0);


            } else if (ntohs(*frame_headers->rx_sub_tlv_type) == TYPE_DELAY2) {


                // Grab the Rx time and sent Tx value for the second time
                clock_gettime(CLOCK_MONOTONIC_RAW, &qm_test->ts_rtt);
                qm_test->time_rx_2[delay_index] = qm_test->ts_rtt.tv_sec + ((double)qm_test->ts_rtt.tv_nsec * 1e-9);
                qm_test->time_tx_2[delay_index] = (ntohll(*frame_headers->rx_sub_tlv_value) / 1000000000.0);

                // Calculate the delay
                qm_test->time_tx_diff[delay_index] = qm_test->time_tx_2[delay_index] - qm_test->time_tx_1[delay_index];
                qm_test->time_rx_diff[delay_index] = qm_test->time_rx_2[delay_index] - qm_test->time_rx_1[delay_index];

                // Rarely a negative value is calculated
                if (qm_test->time_rx_diff[delay_index] - qm_test->time_tx_diff[delay_index] > 0) {
                    qm_test->delay_results[delay_index] = qm_test->time_rx_diff[delay_index] - qm_test->time_tx_diff[delay_index];
                // This value returned is minus and thus invalid
                } else {

                    qm_test->delay_results[delay_index] = 0;
                }


                // Send the calculated delay back to the Tx host
                build_sub_tlv(frame_headers, htons(TYPE_DELAY),
                              htonll(roundl(qm_test->delay_results[delay_index]*1000000000.0)));

                tx_ret = sendto(test_interface->sock_fd, frame_headers->tx_buffer,
                                    frame_headers->length+frame_headers->sub_tlv_size,
                                    0,(struct sockaddr*)&test_interface->sock_addr,
                                    sizeof(test_interface->sock_addr));

                if (tx_ret <= 0)
                {
                    perror("Delay test Rx error ");
                    return;
                }


                delay_index += 1;


            } else if (ntohl(*frame_headers->rx_tlv_value) == VALUE_DELAY_END) {

                WAITING = false;

                double delay_avg = 0;

                for (uint32_t i=0; i < qm_test->delay_test_count; i += 1)
                {
                    delay_avg += qm_test->delay_results[i];
                }

                delay_avg = (delay_avg/qm_test->delay_test_count);

                printf("Tx to Rx delay calculated as %.9f seconds\n\n", delay_avg);

            }


        } // End of WAITING

        free(qm_test->time_tx_1);
        free(qm_test->time_tx_2);
        free(qm_test->time_rx_1);
        free(qm_test->time_rx_2);
        free(qm_test->time_tx_diff);
        free(qm_test->time_rx_diff);
        qm_test->time_tx_1    = NULL;
        qm_test->time_tx_2    = NULL;
        qm_test->time_rx_1    = NULL;
        qm_test->time_rx_2    = NULL;
        qm_test->time_tx_diff = NULL;
        qm_test->time_rx_diff = NULL;

    } // End of Rx mode

}



void mtu_sweep_test(struct app_params *app_params,
                    struct frame_headers *frame_headers,
                    struct test_interface *test_interface,
                    struct test_params *test_params,
                    struct mtu_test *mtu_test)
{

    // Check the interface MTU
    int32_t phy_mtu = get_interface_mtu_by_name(test_interface);

    if (mtu_test->mtu_tx_max > phy_mtu) {

        printf("Starting MTU sweep from %u to %d (interface max)\n",
               mtu_test->mtu_tx_min, phy_mtu);
        mtu_test->mtu_tx_max = phy_mtu;

    } else {

        printf("Starting MTU sweep from %u to %u\n",
               mtu_test->mtu_tx_min, mtu_test->mtu_tx_max);
    
    }


    int16_t tx_ret = 0;
    int16_t rx_len     = 0;

    build_tlv(frame_headers, htons(TYPE_TESTFRAME), htonl(VALUE_TEST_SUB_TLV));

    if (app_params->tx_mode == true) {

        uint16_t mtu_tx_current   = 0;
        uint16_t mtu_ack_previous = 0;
        uint16_t mtu_ack_current  = 0;
        uint8_t  WAITING          = true;

        for (mtu_tx_current = mtu_test->mtu_tx_min; mtu_tx_current <= mtu_test->mtu_tx_max; mtu_tx_current += 1)
        {

            // Get the current time
            clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->ts_elapsed_time);

            build_sub_tlv(frame_headers, htons(TYPE_FRAMEINDEX), htonll(mtu_tx_current));

            tx_ret = sendto(test_interface->sock_fd,
                            frame_headers->tx_buffer,
                            frame_headers->length+mtu_tx_current,
                            0, 
                            (struct sockaddr*)&test_interface->sock_addr,
                            sizeof(test_interface->sock_addr));

            if (tx_ret <=0)
            {
                perror("MTU test Tx error ");
                return;
            } else {
                test_params->f_tx_count += 1;
            }

            WAITING = true;

            // Wait for the ACK from back from Rx ///// Document the max wait time (3 seconds)
            while(WAITING)
            {

                // Get the current time
                clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->ts_current_time);

                // Poll has been disabled in favour of a non-blocking recvfrom (for now)
                rx_len = recvfrom(test_interface->sock_fd,
                                  frame_headers->rx_buffer,
                                  mtu_test->mtu_tx_max,
                                  MSG_DONTWAIT, NULL, NULL);

                if (rx_len > 0) {


                    if (ntohl(*frame_headers->rx_tlv_value) == VALUE_TEST_SUB_TLV &&
                        ntohs(*frame_headers->rx_sub_tlv_type) == TYPE_ACKINDEX)
                    {

                        test_params->f_rx_count += 1;

                        // Get the MTU size Rx is ACK'ing
                        mtu_ack_current = (uint32_t)ntohll(*frame_headers->rx_sub_tlv_value);
                        if (mtu_ack_current > mtu_test->largest) 
                        {
                            mtu_test->largest = mtu_ack_current;
                        }


                        if (mtu_ack_current < mtu_ack_previous)
                        {
                            // Frame received out of order, later than expected
                            test_params->f_rx_late += 1;
                        } else if (mtu_ack_previous == 0) {
                            // First frame
                            test_params->f_rx_ontime += 1;
                            mtu_ack_previous = mtu_ack_current;
                        } else if (mtu_ack_current > (mtu_ack_previous + 2)) {
                            // Frame received out of order, earlier than expected
                            test_params->f_rx_early += 1;
                            mtu_ack_previous = mtu_ack_current;
                        } else if (mtu_ack_current == (mtu_ack_previous + 1) && mtu_ack_current < mtu_test->mtu_tx_max) {
                            // Fame received in order
                            test_params->f_rx_ontime += 1;
                            mtu_ack_previous = mtu_ack_current;
                            WAITING = false;
                        } else if (mtu_ack_current == (mtu_ack_previous + 1)) {
                            // Frame received in order
                            test_params->f_rx_ontime += 1;
                        } else if (mtu_ack_current == mtu_ack_previous) {
                            // Frame received in order
                            test_params->f_rx_ontime += 1;
                        }


                    // A non-test frame was received
                    } else {

                        test_params->f_rx_other += 1;

                    }


                } // Rx_LEN > 0

                // 1 seconds have passed Tx has probably missed/lost the ACK
                if ((test_params->ts_current_time.tv_sec - 
                     test_params->ts_elapsed_time.tv_sec) >= 1) WAITING = false;

            } // End of WAITING

            
        } // End of Tx transmit

        printf("MTU sweep test complete\n");
        mtu_sweep_test_results(mtu_test->largest, test_params, app_params->tx_mode);


    // Running in Rx mode
    } else {

        uint32_t mtu_rx_previous = 0;
        uint32_t mtu_rx_current  = 0;
        uint8_t  WAITING         = true;


        while(true)
        {
        
            clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->ts_elapsed_time);

            WAITING    = true;

            while (WAITING)
            {

                // Get the current time
                clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->ts_current_time);

                // Check for test frame from Tx
                rx_len = recvfrom(test_interface->sock_fd,
                                  frame_headers->rx_buffer,
                                  mtu_test->mtu_tx_max,
                                  MSG_DONTWAIT, NULL, NULL);

                if (rx_len > 0) {

                    if (ntohl(*frame_headers->rx_tlv_value) == VALUE_TEST_SUB_TLV &&
                        ntohs(*frame_headers->rx_sub_tlv_type) == TYPE_FRAMEINDEX)
                    {

                        test_params->f_rx_count += 1;

                        // Get the MTU size Tx is sending
                        mtu_rx_current = (uint32_t)ntohll(*frame_headers->rx_sub_tlv_value);

                        if (mtu_rx_current > mtu_test->largest)
                        {

                            mtu_test->largest = mtu_rx_current;

                            // ACK that back to Tx as new largest MTU received
                            build_sub_tlv(frame_headers, htons(TYPE_ACKINDEX),
                                          htonll(mtu_rx_current));

                            tx_ret = sendto(test_interface->sock_fd,
                                            frame_headers->tx_buffer,
                                            frame_headers->length+frame_headers->sub_tlv_size,
                                            0, 
                                            (struct sockaddr*)&test_interface->sock_addr,
                                            sizeof(test_interface->sock_addr));

                            if (tx_ret <=0)
                            {
                                perror("MTU test Rx error ");
                                return;
                            } else {
                                test_params->f_tx_count += 1;
                            }


                        } // mtu_rx_current > mtu_test->largest


                        if (mtu_rx_current < mtu_rx_previous)
                        {
                            // Frame received out of order, later than expected
                            test_params->f_rx_late += 1;
                        } else if (mtu_rx_previous == 0) {
                            // First frame
                            test_params->f_rx_ontime += 1;
                            mtu_rx_previous = mtu_rx_current;
                        } else if (mtu_rx_current > (mtu_rx_previous + 2)) {
                            // Frame received out of order, earlier than expected
                            test_params->f_rx_early += 1;
                            mtu_rx_previous = mtu_rx_current;
                        } else if (mtu_rx_current == (mtu_rx_previous + 1) && mtu_rx_current < mtu_test->mtu_tx_max) {
                            // Frame received in order
                            test_params->f_rx_ontime += 1;
                            mtu_rx_previous = mtu_rx_current;
                            WAITING = false;
                        } else if (mtu_rx_current == (mtu_rx_previous + 1)) {
                            // Frame received in order
                            mtu_rx_previous = mtu_rx_current;
                            test_params->f_rx_ontime += 1;
                        } else if (mtu_rx_current == mtu_rx_previous) {
                            // Frame received in order
                            mtu_rx_previous = mtu_rx_current;
                            test_params->f_rx_ontime += 1;
                        }


                    // A non-test frame was recieved
                    } else {

                        test_params->f_rx_other += 1;

                    } //End of TEST_FRAME


                } // rx_len > 0


                // If Rx has received the largest MTU Tx hoped to send,
                // the MTU sweep test is over
                if (mtu_test->largest == mtu_test->mtu_tx_max)
                {

                    printf("MTU sweep test complete\n");
                    mtu_sweep_test_results(mtu_test->largest, test_params, app_params->tx_mode);

                    return;

                }

                // 5 seconds have passed without any MTU sweep frames being receeved,
                // assume the max MTU has been exceeded
                if ((test_params->ts_current_time.tv_sec-test_params->ts_elapsed_time.tv_sec) >= 5)  ///// Document this timeout
                {

                    printf("Timeout waiting for MTU sweep frames from Tx, "
                           "ending the test.\nEnding test.\n");

                    mtu_sweep_test_results(mtu_test->largest, test_params, app_params->tx_mode);
                    return;

                }


            } // End of WAITING

        } // while(true)


    } // End of Tx/Rx mode


}



void latency_test(struct app_params *app_params,
                  struct frame_headers *frame_headers,
                  struct test_interface *test_interface,
                  struct test_params *test_params,
                  struct qm_test *qm_test)
{

    build_tlv(frame_headers, htons(TYPE_TESTFRAME), htonl(VALUE_TEST_SUB_TLV));

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


    if (app_params->tx_mode == true)
    {

        if (test_params->f_count > 0) {   
            // Testing until N rtt measurements
            testMax  = &test_params->f_count;
            testBase = &test_params->f_tx_count;

        } else {
            // Testing until N seconds have passed
            if (test_params->f_duration > 0) test_params->f_duration -= 1;
            testMax  = (uint64_t*)&test_params->f_duration;
            testBase = (uint64_t*)&test_params->s_elapsed;
        }


        clock_gettime(CLOCK_MONOTONIC_RAW, &qm_test->ts_start);

        printf("No.\trtt\t\tJitter\n");

        while (*testBase<=*testMax)
        {
            
            clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->ts_current_time);

            uptime_1 = test_params->ts_current_time.tv_sec + ((double)test_params->ts_current_time.tv_nsec * 1e-9);
            tx_uptime = roundl(uptime_1 * 1000000000.0);

            build_sub_tlv(frame_headers, htons(TYPE_PING), htonll(tx_uptime));

            tx_ret = sendto(test_interface->sock_fd,
                            frame_headers->tx_buffer,
                            frame_headers->length+frame_headers->sub_tlv_size,
                            0, 
                            (struct sockaddr*)&test_interface->sock_addr,
                            sizeof(test_interface->sock_addr));

            if (tx_ret <=0 )
            {
                perror("Latency test Tx error ");
                return;
            }


            test_params->f_tx_count += 1;
            qm_test->test_count     += 1;

            printf("%" PRIu32 ":", qm_test->test_count);

            WAITING      = true;
            echo_waiting = true;


            while (WAITING)
            {

                // Get the current time
                clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->ts_elapsed_time);


                // Check if 1 second has passed to increment test duration
                if (test_params->ts_elapsed_time.tv_sec-
                    qm_test->ts_start.tv_sec >= 1) {

                    clock_gettime(CLOCK_MONOTONIC_RAW, &qm_test->ts_start);

                    test_params->s_elapsed += 1;

                }


                // Poll has been disabled in favour of a non-blocking recvfrom (for now)
                rx_len = recvfrom(test_interface->sock_fd,
                                  frame_headers->rx_buffer,
                                  test_params->f_size_total,
                                  MSG_DONTWAIT, NULL, NULL);

                if (rx_len > 0) {

                    // Received an echo reply/pong
                    if (ntohl(*frame_headers->rx_tlv_value) == VALUE_TEST_SUB_TLV &&
                        ntohs(*frame_headers->rx_sub_tlv_type) == TYPE_PONG)
                    {

                        uptime_2 = test_params->ts_elapsed_time.tv_sec + 
                                   ((double)test_params->ts_elapsed_time.tv_nsec * 1e-9);

                        // Check it's the time value originally sent
                        uptime_rx = (double)ntohll(*frame_headers->rx_sub_tlv_value) / 1000000000.0;

                        if (uptime_rx == uptime_1)
                        {

                            rtt = uptime_2-uptime_1;

                            if (rtt < qm_test->rtt_min)
                            {
                                qm_test->rtt_min = rtt;
                            } else if (rtt > qm_test->rtt_max) {
                                qm_test->rtt_max = rtt;
                            }

                            jitter = fabsl(rtt-rtt_prev);

                            if (jitter < qm_test->jitter_min)
                            {
                                qm_test->jitter_min = jitter;
                            } else if (jitter > qm_test->jitter_max) {
                                qm_test->jitter_max = jitter;
                            }

                            printf ("\t%.9Lfs\t%.9Lfs\n", rtt, jitter);

                            rtt_prev = rtt;

                            echo_waiting = false;

                            test_params->f_rx_count += 1;

                        // We may have received a frame with "the right bits in the right place"
                        } else {
                            test_params->f_rx_other += 1;
                        }


                    // Check if Rx host has quit/died
                    } else if (ntohl(*frame_headers->rx_tlv_value) == VALUE_DYINGGASP) {

                        printf("\nRx host has quit\n");
                        latency_test_results(qm_test, test_params, app_params->tx_mode);
                        return;

                    } else {

                        test_params->f_rx_other += 1;

                    }

                } // rx_len > 0


                // Convert the timespec values into a double. The  tv_nsec
                // value loops around, it is not linearly incrementing indefinitely
                elapsed_time = test_params->ts_elapsed_time.tv_sec + ((double)test_params->ts_elapsed_time.tv_nsec * 1e-9);
                current_time = test_params->ts_current_time.tv_sec + ((double)test_params->ts_current_time.tv_nsec * 1e-9);
                max_time = qm_test->timeout_sec + ((double)qm_test->timeout_nsec * 1e-9);

                // If Tx is waiting for echo reply, check if the echo reply has timed out
                if (echo_waiting) {
                    if ((elapsed_time - current_time) > max_time) {

                        printf("\t*\n");
                        echo_waiting = false;
                        qm_test->timeout_count += 1;

                    }
                }


                // Check if the echo interval has passed (time to send another ping)
                max_time = qm_test->interval_sec + ((double)qm_test->interval_nsec * 1e-9);

                if ((elapsed_time - current_time) > max_time) {
                    WAITING  = false;
                }


            } // WAITING=true


        } // testBase<=testMax

        printf("Link quality test complete\n");
        latency_test_results(qm_test, test_params, app_params->tx_mode);


    // Else, Rx mode
    } else {


        if (test_params->f_count > 0) {   
            // Testing until N rtt measurements
            testMax  = &test_params->f_count;
            testBase = &test_params->f_rx_count;

        } else {
            // Testing until N seconds have passed
            if (test_params->f_duration > 0) test_params->f_duration -= 1;
            testMax  = (uint64_t*)&test_params->f_duration;
            testBase = (uint64_t*)&test_params->s_elapsed;
        }


        clock_gettime(CLOCK_MONOTONIC_RAW, &qm_test->ts_start);

        printf("No.\tEcho Interval\n");


        // Wait for the first test frame to be received before starting the test loop
        uint8_t first_frame = false;
        while (!first_frame)
        {

            rx_len = recvfrom(test_interface->sock_fd, frame_headers->rx_buffer,
                              test_params->f_size_total, MSG_PEEK, NULL, NULL);

            // Check if this is an Etherate test frame
            if (ntohl(*frame_headers->rx_tlv_value) == VALUE_TEST_SUB_TLV &&
                ntohs(*frame_headers->rx_sub_tlv_type) == TYPE_PING)
            {

                first_frame = true;
                qm_test->test_count += 1;
                printf("%" PRIu32 ":\t", qm_test->test_count);

            } else {

               // If the frame is not an Etherate frame it needs to be
               // "consumed" otherwise the next MSG_PEEK will show the
               // same frame:
               rx_len = recvfrom(test_interface->sock_fd, frame_headers->rx_buffer,
                                 test_params->f_size_total, MSG_DONTWAIT, NULL,
                                 NULL);

            }

        }


        // Rx test loop
        while (*testBase<=*testMax)
        {

            clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->ts_current_time);

            WAITING      = true;
            echo_waiting = true;


            while (WAITING)
            {

                // Get the current time
                clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->ts_elapsed_time);

                // Check if 1 second has passed to increment test duration
                if (test_params->ts_elapsed_time.tv_sec-
                    qm_test->ts_start.tv_sec >= 1) {

                    clock_gettime(CLOCK_MONOTONIC_RAW, &qm_test->ts_start);

                    test_params->s_elapsed += 1;

                }


                // Poll has been disabled in favour of a non-blocking
                // recvfrom (for now)
                rx_len = recvfrom(test_interface->sock_fd,
                                  frame_headers->rx_buffer,
                                  test_params->f_size_total,
                                  MSG_DONTWAIT, NULL, NULL);


                if (rx_len > 0) {

                    if ( ntohl(*frame_headers->rx_tlv_value) == VALUE_TEST_SUB_TLV &&
                         ntohs(*frame_headers->rx_sub_tlv_type) == TYPE_PING )
                    {

                        // Time Rx received this value
                        uptime_2 = test_params->ts_elapsed_time.tv_sec + 
                                   ((double)test_params->ts_elapsed_time.tv_nsec * 1e-9);

                        // Send the Tx uptime value back to the Tx host
                        build_sub_tlv(frame_headers, htons(TYPE_PONG),
                                      *frame_headers->rx_sub_tlv_value);

                        tx_ret = sendto(test_interface->sock_fd,
                                        frame_headers->tx_buffer,
                                        frame_headers->length+frame_headers->sub_tlv_size,
                                        0, 
                                        (struct sockaddr*)&test_interface->sock_addr,
                                        sizeof(test_interface->sock_addr));

                        if (tx_ret <= 0)
                        {
                            perror("Latency test Rx error ");
                            return;
                        }


                        test_params->f_rx_count += 1;
                        test_params->f_tx_count += 1;

                        interval = fabsl(uptime_2-uptime_1);


                        // Interval between receiving this uptime value and the last
                        if (uptime_1 != 0.0)
                        {
                            printf("%.9Lfs\n", interval);
                        } else {
                            printf("0.0\n");
                        }
                        uptime_1 = uptime_2;

                        if (interval < qm_test->interval_min)
                        {
                            qm_test->interval_min = interval;
                        } else if (interval > qm_test->interval_max) {
                            qm_test->interval_max = interval;
                        }

                        echo_waiting = false;
                        qm_test->test_count += 1;
                        printf("%" PRIu32 ":\t", qm_test->test_count);


                    } else {

                        test_params->f_rx_other += 1;

                    }

                } // rx_len > 0


                // Check if the echo request has timed out
                elapsed_time = test_params->ts_elapsed_time.tv_sec + ((double)test_params->ts_elapsed_time.tv_nsec * 1e-9);
                current_time = test_params->ts_current_time.tv_sec + ((double)test_params->ts_current_time.tv_nsec * 1e-9);
                max_time = qm_test->timeout_sec + ((double)qm_test->timeout_nsec * 1e-9);
                
                if (echo_waiting == true) {
                    if ((elapsed_time - current_time) > max_time) {

                        printf("*\n");
                        qm_test->timeout_count += 1;
                        echo_waiting = false;

                    }
                    
                }


                // Check if the echo interval has passed (time to receive another ping)
                elapsed_time = test_params->ts_elapsed_time.tv_sec + ((double)test_params->ts_elapsed_time.tv_nsec * 1e-9);
                current_time = test_params->ts_current_time.tv_sec + ((double)test_params->ts_current_time.tv_nsec * 1e-9);
                max_time = qm_test->interval_sec + ((double)qm_test->interval_nsec * 1e-9);

                if ((elapsed_time - current_time) > max_time) {
                    WAITING = false;
                }


                // Check if Tx host has quit/died;
                if (ntohl(*frame_headers->rx_tlv_value) == VALUE_DYINGGASP)
                {

                    printf("\nTx host has quit\n");
                    latency_test_results(qm_test, test_params, app_params->tx_mode);
                    return;

                }


            } // WAITING=true


        } // testBase<=testMax

        printf("Link quality test complete\n");
        latency_test_results(qm_test, test_params, app_params->tx_mode);

    } // End of Tx/Rx

 }



void speed_test_default(struct app_params *app_params,
                        struct frame_headers *frame_headers,
                        struct speed_test *speed_test,
                        struct test_interface *test_interface,
                        struct test_params *test_params)
{

    int16_t tx_ret = 0;
    int16_t rx_len = 0;

    build_tlv(frame_headers, htons(TYPE_TESTFRAME), htonl(VALUE_TEST_SUB_TLV));


    if (app_params->tx_mode == true)
    {

        printf("Seconds\t\tMbps Tx\t\tMBs Tx\t\tFrmTx/s\t\tFrames Tx\n");

        // By default test until f_duration_DEF has passed
        uint64_t *testMax = (uint64_t*)&test_params->f_duration;
        uint64_t *testBase = (uint64_t*)&test_params->s_elapsed;

        if (test_params->f_bytes > 0)
        {    
            // Testing until N bytes sent
            testMax  = &test_params->f_bytes;
            testBase = &speed_test->b_tx;

        } else if (test_params->f_count > 0) {   
            // Testing until N frames sent
            testMax  = &test_params->f_count;
            testBase = &test_params->f_tx_count;

        } else if (test_params->f_duration > 0) {
            // Testing until N seconds have passed
            test_params->f_duration -= 1;
        }


        // Get clock times for the speed limit restriction and starting time
        clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->ts_elapsed_time);

        // Tx test loop
        while (*testBase<=*testMax)
        {

            clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->ts_current_time);

            // One second has passed
            if ((test_params->ts_current_time.tv_sec - test_params->ts_elapsed_time.tv_sec) >= 1)
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
                test_params->ts_elapsed_time.tv_sec  = test_params->ts_current_time.tv_sec;
                test_params->ts_elapsed_time.tv_nsec = test_params->ts_current_time.tv_nsec;

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

                    // Check if a max speed has been configured
                    if (speed_test->b_tx_speed_max != B_TX_SPEED_MAX_DEF)
                    {

                        // Check if sending another frame exceeds the max speed configured
                        if ((speed_test->b_tx_speed_prev + test_params->f_size_total) <= speed_test->b_tx_speed_max)
                        {


                            build_sub_tlv(frame_headers, htons(TYPE_FRAMEINDEX), 
                                          htonll(test_params->f_tx_count+1));

                            tx_ret = sendto(test_interface->sock_fd,
                                            frame_headers->tx_buffer,
                                            test_params->f_size_total, 0, 
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

                    // Check if a max frames per second limit is configured
                    } else if (test_params->f_tx_count_max != F_TX_COUNT_MAX_DEF) {

                        // Check if sending another frame exceeds the max frame rate configured
                        if (test_params->f_tx_count - test_params->f_tx_count_prev < test_params->f_tx_count_max)
                        {

                            build_sub_tlv(frame_headers, htons(TYPE_FRAMEINDEX), 
                                          htonll(test_params->f_tx_count+1));

                            tx_ret = sendto(test_interface->sock_fd,
                                            frame_headers->tx_buffer,
                                            test_params->f_size_total, 0, 
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

                    // Else there are no speed restrictions
                    } else {

                        build_sub_tlv(frame_headers, htons(TYPE_FRAMEINDEX),
                                      htonll(test_params->f_tx_count+1));

                        tx_ret = sendto(test_interface->sock_fd,
                                        frame_headers->tx_buffer,
                                        test_params->f_size_total, 0, 
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


            } // End of ts_current_time.tv_sec - s_elapsed_TIME.tv_sec

        }


        if (test_params->s_elapsed > 0)
        {
            speed_test->b_speed_avg = (speed_test->b_speed_avg/test_params->s_elapsed);
            speed_test->f_speed_avg = (speed_test->f_speed_avg/test_params->s_elapsed);
        }

        printf("Speed test complete\n");
        speed_test_results(speed_test, test_params, app_params->tx_mode);


    // Else, Rx mode
    } else {

        printf("Seconds\t\tMbps Rx\t\tMBs Rx\t\tFrmRx/s\t\tFrames Rx\n");

        // By default test until f_duration_DEF has passed
        uint64_t *testMax = (uint64_t*)&test_params->f_duration;
        uint64_t *testBase = (uint64_t*)&test_params->s_elapsed;

        // Testing until N bytes received
        if (test_params->f_bytes > 0)
        {
            testMax  = &test_params->f_bytes;
            testBase = &speed_test->b_rx;

        // Testing until N frames received
        } else if (test_params->f_count > 0) {
            testMax  = &test_params->f_count;
            testBase = &test_params->f_rx_count;

        // Testing until N seconds have passed
        } else if (test_params->f_duration > 0) {
            test_params->f_duration -= 1;
        }


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

        clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->ts_elapsed_time);

        // Rx test loop
        while (*testBase<=*testMax)
        {

            clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->ts_current_time);

            // If one second has passed
            if ((test_params->ts_current_time.tv_sec-test_params->ts_elapsed_time.tv_sec) >= 1)
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
                test_params->ts_elapsed_time.tv_sec = test_params->ts_current_time.tv_sec;
                test_params->ts_elapsed_time.tv_nsec = test_params->ts_current_time.tv_nsec;

            }

            // Poll has been disabled in favour of a non-blocking recvfrom (for now)
            rx_len = recvfrom(test_interface->sock_fd,
                              frame_headers->rx_buffer,
                              test_params->f_size_total,
                              MSG_DONTWAIT, NULL, NULL);

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
                    speed_test->b_rx += (rx_len - frame_headers->length);

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


        if (test_params->s_elapsed > 0)
        {
            speed_test->b_speed_avg = (speed_test->b_speed_avg/test_params->s_elapsed);
            speed_test->f_speed_avg = (speed_test->f_speed_avg/test_params->s_elapsed);
        }

        printf("Speed test complete\n");
        speed_test_results(speed_test, test_params, app_params->tx_mode);

    }

}



void send_custom_frame(struct app_params *app_params,
                       struct frame_headers *frame_headers,
                       struct speed_test *speed_test,
                       struct test_interface *test_interface,
                       struct test_params *test_params)
{

    int16_t tx_ret = 0;
    int16_t rx_len = 0;

    printf("Seconds\t\tMbps Tx\t\tMBs Tx\t\tFrmTx/s\t\tFrames Tx\n");

    // By default test until f_duration_DEF has passed
    uint64_t *testMax = (uint64_t*)&test_params->f_duration;
    uint64_t *testBase = (uint64_t*)&test_params->s_elapsed;

    if (test_params->f_bytes > 0)
    {    
        // Testing until N bytes sent
        testMax  = &test_params->f_bytes;
        testBase = &speed_test->b_tx;

    } else if (test_params->f_count > 0) {   
        // Testing until N frames sent
        testMax  = &test_params->f_count;
        testBase = &test_params->f_tx_count;

    } else if (test_params->f_duration > 0) {
        // Testing until N seconds have passed
        test_params->f_duration -= 1;
    }


    // Get clock times for the speed limit restriction and starting time
    clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->ts_elapsed_time);

    // Tx test loop
    while (*testBase<=*testMax)
    {

        clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->ts_current_time);

        // One second has passed
        if ((test_params->ts_current_time.tv_sec - test_params->ts_elapsed_time.tv_sec) >= 1)
        {
            test_params->s_elapsed += 1;
            speed_test->b_speed   = ((double)(speed_test->b_tx - speed_test->b_tx_prev) * 8) / 1000 / 1000;
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
            test_params->ts_elapsed_time.tv_sec  = test_params->ts_current_time.tv_sec;
            test_params->ts_elapsed_time.tv_nsec = test_params->ts_current_time.tv_nsec;

            speed_test->b_tx_speed_prev = 0;

        } else {

            // Poll has been disabled in favour of a non-blocking recvfrom (for now)
            rx_len = recvfrom(test_interface->sock_fd, frame_headers->rx_buffer,
                              test_params->f_size_total, MSG_DONTWAIT, NULL, NULL);

            if (rx_len > 0) {

                // Check for dying gasp from Rx host
                if (ntohs(*frame_headers->rx_tlv_type) == TYPE_APPLICATION &&
                    ntohll(*frame_headers->rx_tlv_value) == VALUE_DYINGGASP) {

                    printf("\nRx host has quit\n");
                    speed_test_results(speed_test, test_params, app_params->tx_mode);
                    return;

                // Received a non-test frame
                } else {
                    test_params->f_rx_other += 1;
                }

            }


            // Check if a max speed has been configured
            if (speed_test->b_tx_speed_max != B_TX_SPEED_MAX_DEF)
            {

                // Check if sending another frame exceeds the max speed configured
                if ((speed_test->b_tx_speed_prev + speed_test->f_payload_size) <= speed_test->b_tx_speed_max)
                {


                    tx_ret = sendto(test_interface->sock_fd,
                                    speed_test->f_payload,
                                    speed_test->f_payload_size, 0, 
                                    (struct sockaddr*)&test_interface->sock_addr,
                                    sizeof(test_interface->sock_addr));

                    if (tx_ret <= 0)
                    {
                        perror("Frame sending Tx error ");
                        return;
                    }


                    test_params->f_tx_count += 1;
                    speed_test->b_tx += speed_test->f_payload_size;
                    speed_test->b_tx_speed_prev += speed_test->f_payload_size;

                }

            // Check if a max frames per second limit is configured
            } else if (test_params->f_tx_count_max != F_TX_COUNT_MAX_DEF) {

                // Check if sending another frame exceeds the max frame rate configured
                if (test_params->f_tx_count - test_params->f_tx_count_prev < test_params->f_tx_count_max)
                {

                    tx_ret = sendto(test_interface->sock_fd,
                                    speed_test->f_payload,
                                    speed_test->f_payload_size, 0, 
                                    (struct sockaddr*)&test_interface->sock_addr,
                                    sizeof(test_interface->sock_addr));

                    if (tx_ret <= 0)
                    {
                        perror("Frame sending Tx error ");
                        return;
                    }


                    test_params->f_tx_count += 1;
                    speed_test->b_tx += speed_test->f_payload_size;
                    speed_test->b_tx_speed_prev += speed_test->f_payload_size;

                }

            // Else there are no speed restrictions
            } else {

                tx_ret = sendto(test_interface->sock_fd,
                                speed_test->f_payload,
                                speed_test->f_payload_size, 0, 
                                (struct sockaddr*)&test_interface->sock_addr,
                                sizeof(test_interface->sock_addr));

                if (tx_ret <= 0)
                {
                    perror("Frame sending Tx error ");
                    return;
                }


                test_params->f_tx_count += 1;
                speed_test->b_tx += speed_test->f_payload_size;
                speed_test->b_tx_speed_prev += speed_test->f_payload_size;
            }

        } // End of ts_current_time.tv_sec - ts_elapsed_time.tv_sec

    }


    if (test_params->s_elapsed > 0)
    {
        speed_test->b_speed_avg = (speed_test->b_speed_avg/test_params->s_elapsed);
        speed_test->f_speed_avg = (speed_test->f_speed_avg/test_params->s_elapsed);
    }

    printf("Speed test complete\n");
    speed_test_results(speed_test, test_params, app_params->tx_mode);

 }
