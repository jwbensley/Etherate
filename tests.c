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
 * void speed_test()
 * void send_custom_frame()
 *
 */



#include "tests.h"

void delay_test(struct app_params *app_params,
                struct frame_headers *frame_headers,
                struct test_interface *test_interface,
                struct test_params * test_params,
                struct qm_test *qm_test)
{
// Calculate the delay between Tx and Rx hosts. The uptime is exchanged twice
// to estimate the delay between the hosts. Then the process is repeated so
// an average can be taken


    long double UPTIME;
    uint64_t    TX_UPTIME;
    int16_t     TX_RET_VAL;
    int16_t     RX_LEN;
    uint8_t     WAITING;


    build_tlv(frame_headers, htons(TYPE_TESTFRAME), htonl(VALUE_TEST_SUB_TLV));

    printf("Calculating delay between hosts...\n");

    if (app_params->TX_MODE == true)
    {

        for (uint32_t i=0; i < qm_test->DELAY_TEST_COUNT; i += 1)
        {

            // Get the current time and send it to Rx
            clock_gettime(CLOCK_MONOTONIC_RAW, &qm_test->TS_RTT);

            // Convert it to double then long long (uint64t_) for transmission
            UPTIME = qm_test->TS_RTT.tv_sec + ((double)qm_test->TS_RTT.tv_nsec*1e-9);
            TX_UPTIME = roundl(UPTIME*1000000000.0);

            build_sub_tlv(frame_headers, htons(TYPE_DELAY1), htonll(TX_UPTIME));

            TX_RET_VAL = sendto(test_interface->SOCKET_FD,
                                frame_headers->TX_BUFFER,
                                frame_headers->LENGTH+frame_headers->SUB_TLV_SIZE,
                                0,
                                (struct sockaddr*)&test_interface->SOCKET_ADDRESS,
                                sizeof(test_interface->SOCKET_ADDRESS));

            if (TX_RET_VAL <= 0)
            {
                perror("Delay test Tx error ");
                return;
            }

            // Repeat
            clock_gettime(CLOCK_MONOTONIC_RAW, &qm_test->TS_RTT);

            UPTIME = qm_test->TS_RTT.tv_sec + ((double)qm_test->TS_RTT.tv_nsec*1e-9);
            TX_UPTIME = roundl(UPTIME * 1000000000.0);

            build_sub_tlv(frame_headers, htons(TYPE_DELAY2), htonll(TX_UPTIME));

            TX_RET_VAL = sendto(test_interface->SOCKET_FD,
                                frame_headers->TX_BUFFER,
                                frame_headers->LENGTH+frame_headers->SUB_TLV_SIZE,
                                0,
                                (struct sockaddr*)&test_interface->SOCKET_ADDRESS,
                                sizeof(test_interface->SOCKET_ADDRESS));

            if (TX_RET_VAL <= 0)
            {
                perror("Delay test Tx error ");
                return;
            }


            // Wait for Rx to send back delay value
            WAITING = true;
            while (WAITING)
            {
                RX_LEN = recvfrom(test_interface->SOCKET_FD,
                                  frame_headers->RX_BUFFER,
                                  test_params->F_SIZE_TOTAL,
                                  0, NULL, NULL);

                if (RX_LEN > 0)
                {

                    if (ntohs(*frame_headers->RX_SUB_TLV_TYPE) == TYPE_DELAY)
                    {
                        qm_test->pDELAY_RESULTS[i] = (ntohll(*frame_headers->RX_SUB_TLV_VALUE) / 1000000000.0);
                        WAITING = false;
                    }

                } else if (RX_LEN < 0) {

                    perror("Delay test Tx error ");
                    return;

                }

            }

        } // End delay Tx loop


        // Let the receiver know all delay values have been received
        build_tlv(frame_headers, htons(TYPE_TESTFRAME), htonl(VALUE_DELAY_END));

        build_sub_tlv(frame_headers, htons(TYPE_TESTFRAME),
                      htonll(VALUE_DELAY_END));

        TX_RET_VAL = sendto(test_interface->SOCKET_FD,
                            frame_headers->TX_BUFFER,
                            frame_headers->LENGTH+frame_headers->SUB_TLV_SIZE,
                            0,
                            (struct sockaddr*)&test_interface->SOCKET_ADDRESS,
                            sizeof(test_interface->SOCKET_ADDRESS));

        if (TX_RET_VAL <= 0)
        {
            perror("Delay test Tx error ");
            return;
        }


        double DELAY_AVERAGE = 0;

        for (uint32_t i=0; i < qm_test->DELAY_TEST_COUNT; i += 1)
        {
            DELAY_AVERAGE += qm_test->pDELAY_RESULTS[i];
        }

        DELAY_AVERAGE = (DELAY_AVERAGE/qm_test->DELAY_TEST_COUNT);

        printf("Tx to Rx delay calculated as %.9f seconds\n\n", DELAY_AVERAGE);


    // This is the Rx host
    } else {


        // These values are used to calculate the delay between Tx and Rx hosts
        qm_test->TIME_TX_1 = (double*)calloc(qm_test->DELAY_TEST_COUNT, sizeof(double));
        qm_test->TIME_TX_2 = (double*)calloc(qm_test->DELAY_TEST_COUNT, sizeof(double));
        qm_test->TIME_RX_1 = (double*)calloc(qm_test->DELAY_TEST_COUNT, sizeof(double));
        qm_test->TIME_RX_2 = (double*)calloc(qm_test->DELAY_TEST_COUNT, sizeof(double));
        qm_test->TIME_TX_DIFF = (double*)calloc(qm_test->DELAY_TEST_COUNT, sizeof(double));
        qm_test->TIME_RX_DIFF = (double*)calloc(qm_test->DELAY_TEST_COUNT, sizeof(double));

        uint32_t DELAY_INDEX = 0;
        
        WAITING = true;

        while(WAITING)
        {

            RX_LEN = recvfrom(test_interface->SOCKET_FD,
                              frame_headers->RX_BUFFER,
                              test_params->F_SIZE_TOTAL,
                              0, NULL, NULL);

            if (ntohs(*frame_headers->RX_SUB_TLV_TYPE) == TYPE_DELAY1)
            {

                // Get the time Rx is receiving Tx's sent time figure
                clock_gettime(CLOCK_MONOTONIC_RAW, &qm_test->TS_RTT);

                // Record the time Rx received this Tx value
                qm_test->TIME_RX_1[DELAY_INDEX] = qm_test->TS_RTT.tv_sec + ((double)qm_test->TS_RTT.tv_nsec*1e-9);
                // Record the Tx value received
                qm_test->TIME_TX_1[DELAY_INDEX] = (ntohll(*frame_headers->RX_SUB_TLV_VALUE)/1000000000.0);


            } else if (ntohs(*frame_headers->RX_SUB_TLV_TYPE) == TYPE_DELAY2) {


                // Grab the Rx time and sent Tx value for the second time
                clock_gettime(CLOCK_MONOTONIC_RAW, &qm_test->TS_RTT);
                qm_test->TIME_RX_2[DELAY_INDEX] = qm_test->TS_RTT.tv_sec + ((double)qm_test->TS_RTT.tv_nsec*1e-9);
                qm_test->TIME_TX_2[DELAY_INDEX] = (ntohll(*frame_headers->RX_SUB_TLV_VALUE)/1000000000.0);

                // Calculate the delay
                qm_test->TIME_TX_DIFF[DELAY_INDEX] = qm_test->TIME_TX_2[DELAY_INDEX]-qm_test->TIME_TX_1[DELAY_INDEX];
                qm_test->TIME_RX_DIFF[DELAY_INDEX] = qm_test->TIME_RX_2[DELAY_INDEX]-qm_test->TIME_RX_1[DELAY_INDEX];

                // Rarely a negative value is calculated
                if (qm_test->TIME_RX_DIFF[DELAY_INDEX]-qm_test->TIME_TX_DIFF[DELAY_INDEX] > 0) {
                    qm_test->pDELAY_RESULTS[DELAY_INDEX] = qm_test->TIME_RX_DIFF[DELAY_INDEX]-qm_test->TIME_TX_DIFF[DELAY_INDEX];
                // This value returned is minus and thus invalid
                } else {

                    qm_test->pDELAY_RESULTS[DELAY_INDEX] = 0;
                }


                // Send the calculated delay back to the Tx host
                build_sub_tlv(frame_headers, htons(TYPE_DELAY),
                              htonll(roundl(qm_test->pDELAY_RESULTS[DELAY_INDEX]*1000000000.0)));

                TX_RET_VAL = sendto(test_interface->SOCKET_FD, frame_headers->TX_BUFFER,
                                    frame_headers->LENGTH+frame_headers->SUB_TLV_SIZE,
                                    0,(struct sockaddr*)&test_interface->SOCKET_ADDRESS,
                                    sizeof(test_interface->SOCKET_ADDRESS));

                if (TX_RET_VAL <= 0)
                {
                    perror("Delay test Rx error ");
                    return;
                }


                DELAY_INDEX += 1;


            } else if (ntohl(*frame_headers->RX_TLV_VALUE) == VALUE_DELAY_END) {

                WAITING = false;

                double DELAY_AVERAGE = 0;

                for (uint32_t i=0; i < qm_test->DELAY_TEST_COUNT; i += 1)
                {
                    DELAY_AVERAGE += qm_test->pDELAY_RESULTS[i];
                }

                DELAY_AVERAGE = (DELAY_AVERAGE/qm_test->DELAY_TEST_COUNT);

                printf("Tx to Rx delay calculated as %.9f seconds\n\n", DELAY_AVERAGE);

            }


        } // End of WAITING

        free(qm_test->TIME_TX_1);
        free(qm_test->TIME_TX_2);
        free(qm_test->TIME_RX_1);
        free(qm_test->TIME_RX_2);
        free(qm_test->TIME_TX_DIFF);
        free(qm_test->TIME_RX_DIFF);
        qm_test->TIME_TX_1    = NULL;
        qm_test->TIME_TX_2    = NULL;
        qm_test->TIME_RX_1    = NULL;
        qm_test->TIME_RX_2    = NULL;
        qm_test->TIME_TX_DIFF = NULL;
        qm_test->TIME_RX_DIFF = NULL;

    } // End of Rx mode

}



void mtu_sweep_test(struct app_params *app_params,
                    struct frame_headers *frame_headers,
                    struct test_interface *test_interface,
                    struct test_params *test_params,
                    struct mtu_test *mtu_test)
{

    // Check the interface MTU
    int32_t PHY_MTU = get_interface_mtu_by_name(test_interface);

    if (mtu_test->MTU_TX_MAX > PHY_MTU) {
        printf("Starting MTU sweep from %u to %d (interface max)\n",
               mtu_test->MTU_TX_MIN, PHY_MTU);
        mtu_test->MTU_TX_MAX = PHY_MTU;

    } else {

        printf("Starting MTU sweep from %u to %u\n",
               mtu_test->MTU_TX_MIN, mtu_test->MTU_TX_MAX);
    
    }


    int16_t TX_RET_VAL = 0;
    int16_t RX_LEN     = 0;

    build_tlv(frame_headers, htons(TYPE_TESTFRAME), htonl(VALUE_TEST_SUB_TLV));

    if (app_params->TX_MODE == true) {

        uint16_t MTU_TX_CURRENT   = 0;
        uint16_t MTU_ACK_PREVIOUS = 0;
        uint16_t MTU_ACK_CURRENT  = 0;
        uint16_t MTU_ACK_LARGEST  = 0;
        uint8_t  WAITING          = true;

        for (MTU_TX_CURRENT = mtu_test->MTU_TX_MIN; MTU_TX_CURRENT <= mtu_test->MTU_TX_MAX; MTU_TX_CURRENT += 1)
        {

            // Get the current time
            clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->TS_ELAPSED_TIME);

            // Send each MTU test frame 3 times
            for (uint16_t i = 1; i <= 3; i += 1)
            {

                build_sub_tlv(frame_headers, htons(TYPE_FRAMEINDEX), htonll(MTU_TX_CURRENT));

                TX_RET_VAL = sendto(test_interface->SOCKET_FD,
                                    frame_headers->TX_BUFFER,
                                    frame_headers->LENGTH+MTU_TX_CURRENT,
                                    0, 
                                    (struct sockaddr*)&test_interface->SOCKET_ADDRESS,
                                    sizeof(test_interface->SOCKET_ADDRESS));

                if (TX_RET_VAL <=0)
                {
                    perror("MTU test Tx error ");
                    return;
                }


                test_params->F_TX_COUNT += 1;

                WAITING = true;

            }

            // Wait for the ACK from back from Rx
            while(WAITING)
            {

                // Get the current time
                clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->TS_CURRENT_TIME);

                // Poll has been disabled in favour of a non-blocking recvfrom (for now)
                RX_LEN = recvfrom(test_interface->SOCKET_FD,
                                  frame_headers->RX_BUFFER,
                                  mtu_test->MTU_TX_MAX,
                                  MSG_DONTWAIT, NULL, NULL);

                if (RX_LEN > 0) {


                    if (ntohl(*frame_headers->RX_TLV_VALUE) == VALUE_TEST_SUB_TLV &&
                        ntohs(*frame_headers->RX_SUB_TLV_TYPE) == TYPE_ACKINDEX)
                    {

                        test_params->F_RX_COUNT += 1;

                        // Get the MTU size Rx is ACK'ing
                        MTU_ACK_CURRENT = (uint32_t)ntohll(*frame_headers->RX_SUB_TLV_VALUE);
                        if (MTU_ACK_CURRENT > MTU_ACK_LARGEST) 
                        {
                            MTU_ACK_LARGEST = MTU_ACK_CURRENT;
                        }


                        if (MTU_ACK_CURRENT < MTU_ACK_PREVIOUS)
                        {
                            // Frame received out of order, later than expected
                            test_params->F_RX_LATE += 1;
                        } else if (MTU_ACK_CURRENT > (MTU_ACK_PREVIOUS + 2)) {
                            // Frame received out of order, earlier than expected
                            test_params->F_RX_EARLY += 1;
                            MTU_ACK_PREVIOUS = MTU_ACK_CURRENT;
                        } else if (MTU_ACK_CURRENT == (MTU_ACK_PREVIOUS + 1)) {
                            // Fame received in order
                            MTU_ACK_PREVIOUS = MTU_ACK_CURRENT;
                            test_params->F_RX_ONTIME += 1;
                        } else if (MTU_ACK_CURRENT == MTU_ACK_PREVIOUS) {
                            // Frame received in order
                            MTU_ACK_PREVIOUS = MTU_ACK_CURRENT;
                            test_params->F_RX_ONTIME += 1;
                        }


                    // A non-test frame was received
                    } else {

                        test_params->F_RX_OTHER += 1;

                    }


                } // Rx_LEN > 0

                // 1 seconds have passed Tx has probably missed/lost the ACK
                if ((test_params->TS_CURRENT_TIME.tv_sec - 
                     test_params->TS_ELAPSED_TIME.tv_sec) >= 1) WAITING = false;

            } // End of WAITING

            
        } // End of Tx transmit


        printf("Largest MTU ACK'ed from Rx is %u\n\n", MTU_ACK_LARGEST);

        printf("Test frames transmitted: %" PRIu64 "\n"
               "Test frames received: %" PRIu64 "\n"
               "Non test frames received: %" PRIu64 "\n"
               "In order ACK frames received: %" PRIu64 "\n"
               "Out of order ACK frames received early: %" PRIu64 "\n"
               "Out of order ACK frames received late: %" PRIu64 "\n",
               test_params->F_TX_COUNT,
               test_params->F_RX_COUNT,
               test_params->F_RX_OTHER,
               test_params->F_RX_ONTIME,
               test_params->F_RX_EARLY,
               test_params->F_RX_LATE);


    // Running in Rx mode
    } else {

        uint32_t MTU_RX_PREVIOUS = 0;
        uint32_t MTU_RX_CURRENT  = 0;
        uint32_t MTU_RX_LARGEST  = 0;
        uint8_t WAITING          = true;

        // If 5 seconds pass without receiving a frame, end the MTU sweep test
        // (assume MTU has been exceeded)
        uint8_t MTU_RX_ANY = false;
        
        clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->TS_ELAPSED_TIME);

        while (WAITING)
        {

            // Get the current time
            clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->TS_CURRENT_TIME);

            // Check for test frame from Tx
            RX_LEN = recvfrom(test_interface->SOCKET_FD,
                              frame_headers->RX_BUFFER,
                              mtu_test->MTU_TX_MAX,
                              MSG_DONTWAIT, NULL, NULL);

            if (RX_LEN > 0) {

                if (ntohl(*frame_headers->RX_TLV_VALUE) == VALUE_TEST_SUB_TLV &&
                    ntohs(*frame_headers->RX_SUB_TLV_TYPE) == TYPE_FRAMEINDEX)
                {

                    MTU_RX_ANY = true;

                    test_params->F_RX_COUNT += 1;

                    // Get the MTU size Tx is sending
                    MTU_RX_CURRENT = (uint32_t)ntohll(*frame_headers->RX_SUB_TLV_VALUE);

                    if (MTU_RX_CURRENT > MTU_RX_LARGEST)
                    {

                        MTU_RX_LARGEST = MTU_RX_CURRENT;

                        // ACK that back to Tx as new largest MTU received
                        // (send the ACK 3 times)
                        for(uint8_t i = 1; i <= 3; i += 1)
                        {

                        build_sub_tlv(frame_headers, htons(TYPE_ACKINDEX), htonll(MTU_RX_CURRENT));

                        TX_RET_VAL = sendto(test_interface->SOCKET_FD,
                                            frame_headers->TX_BUFFER,
                                            frame_headers->LENGTH+frame_headers->SUB_TLV_SIZE, 0, 
                                            (struct sockaddr*)&test_interface->SOCKET_ADDRESS,
                                            sizeof(test_interface->SOCKET_ADDRESS));

                        if (TX_RET_VAL <=0)
                        {
                            perror("MTU test Rx error ");
                            return;
                        }


                        test_params->F_TX_COUNT += 1;

                        }


                    } // MTU_RX_CURRENT > MTU_RX_LARGEST



                    if (MTU_RX_CURRENT < MTU_RX_PREVIOUS)
                    {
                        // Frame received out of order, later than expected
                        test_params->F_RX_LATE += 1;
                    } else if (MTU_RX_CURRENT > (MTU_RX_PREVIOUS + 2)) {
                        // Frame received out of order, earlier than expected
                        test_params->F_RX_EARLY += 1;
                        MTU_RX_PREVIOUS = MTU_RX_CURRENT;
                    } else if (MTU_RX_CURRENT == (MTU_RX_PREVIOUS + 1)) {
                        // Fame received in order
                        MTU_RX_PREVIOUS = MTU_RX_CURRENT;
                        test_params->F_RX_ONTIME += 1;
                    } else if (MTU_RX_CURRENT == MTU_RX_PREVIOUS) {
                        // Frame received in order
                        MTU_RX_PREVIOUS = MTU_RX_CURRENT;
                        test_params->F_RX_ONTIME += 1;
                    }


                // A non-test frame was recieved
                } else {

                    test_params->F_RX_OTHER += 1;

                } //End of TEST_FRAME


            } // RX_LEN > 0


            // If Rx has received the largest MTU Tx hoped to send,
            // the MTU sweep test is over
            if (MTU_RX_LARGEST == mtu_test->MTU_TX_MAX)
            {

                // Signal back to Tx the largest MTU Rx recieved at the end
                WAITING = false;

                printf("MTU sweep test complete\n"
                      "Largest MTU received was %u\n\n", MTU_RX_LARGEST);

                printf("Test frames transmitted: %" PRIu64 "\n"
                       "Test frames received: %" PRIu64 "\n"
                       "Non test frames received: %" PRIu64 "\n"
                       "In order test frames received: %" PRIu64 "\n"
                       "Out of order test frames received early: %" PRIu64 "\n"
                       "Out of order test frames received late: %" PRIu64 "\n",
                       test_params->F_TX_COUNT,
                       test_params->F_RX_COUNT,
                       test_params->F_RX_OTHER,
                       test_params->F_RX_ONTIME,
                       test_params->F_RX_EARLY,
                       test_params->F_RX_LATE);

                return;

            }

            // 5 seconds have passed without any MTU sweep frames being receeved
            // Assume the max MTU has been exceeded
            if ((test_params->TS_CURRENT_TIME.tv_sec-test_params->TS_ELAPSED_TIME.tv_sec) >= 5)
            {

                if (MTU_RX_ANY == false)
                {
                    printf("Timeout waiting for MTU sweep frames from Tx, "
                           "ending the test.\nLargest MTU received %u\n\n",
                           MTU_RX_LARGEST);

                    WAITING = false;

                } else {

                    clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->TS_ELAPSED_TIME);
                    MTU_RX_ANY = false;

                }

            }


        } // End of WAITING


    } // End of Tx/Rx mode


}



void latency_test(struct app_params *app_params,
                  struct frame_headers *frame_headers,
                  struct test_interface *test_interface,
                  struct test_params *test_params,
                  struct qm_test *qm_test)
{

    build_tlv(frame_headers, htons(TYPE_TESTFRAME), htonl(VALUE_TEST_SUB_TLV));

    int16_t     TX_RET_VAL    = 0;
    int16_t     RX_LEN        = 0;
    long double UPTIME_1     = 0.0;
    long double UPTIME_2     = 0.0;
    long double UPTIME_RX    = 0.0;
    long double RTT          = 0.0;
    long double RTT_PREV     = 0.0;
    long double JITTER       = 0.0;
    long double INTERVAL     = 0.0;
    uint64_t    TX_UPTIME    = 0;
    uint8_t     WAITING      = false;
    uint8_t     ECHO_WAITING = false;

    uint64_t *testBase, *testMax;


    if (app_params->TX_MODE == true)
    {

        if (test_params->F_COUNT > 0) {   
            // Testing until N RTT measurements
            testMax = &test_params->F_COUNT;
            testBase = &test_params->F_TX_COUNT;

        } else {
            // Testing until N seconds have passed
            if (test_params->F_DURATION > 0) test_params->F_DURATION -= 1;
            testMax = (uint64_t*)&test_params->F_DURATION;
            testBase = (uint64_t*)&test_params->S_ELAPSED;
        }


        clock_gettime(CLOCK_MONOTONIC_RAW, &qm_test->TS_START);

        printf("No.\tRTT\t\tJitter\n");

        while (*testBase<=*testMax)
        {
            
            clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->TS_CURRENT_TIME);

            UPTIME_1 = test_params->TS_CURRENT_TIME.tv_sec + ((double)test_params->TS_CURRENT_TIME.tv_nsec * 1e-9);
            TX_UPTIME = roundl(UPTIME_1 * 1000000000.0);

            build_sub_tlv(frame_headers, htons(TYPE_PING), htonll(TX_UPTIME));

            TX_RET_VAL = sendto(test_interface->SOCKET_FD,
                                frame_headers->TX_BUFFER,
                                frame_headers->LENGTH+frame_headers->SUB_TLV_SIZE,
                                0, 
                                (struct sockaddr*)&test_interface->SOCKET_ADDRESS,
                                sizeof(test_interface->SOCKET_ADDRESS));

            if (TX_RET_VAL <=0 )
            {
                perror("Latency test Tx error ");
                return;
            }


            test_params->F_TX_COUNT += 1;

            WAITING = true;
            ECHO_WAITING = true;

            while (WAITING)
            {

                // Get the current time
                clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->TS_ELAPSED_TIME);

                // Poll has been disabled in favour of a non-blocking recvfrom (for now)
                RX_LEN = recvfrom(test_interface->SOCKET_FD,
                                  frame_headers->RX_BUFFER,
                                  test_params->F_SIZE_TOTAL,
                                  MSG_DONTWAIT, NULL, NULL);

                if (RX_LEN > 0) {

                    // Received an echo reply/pong
                    if (ntohl(*frame_headers->RX_TLV_VALUE) == VALUE_TEST_SUB_TLV &&
                        ntohs(*frame_headers->RX_SUB_TLV_TYPE) == TYPE_PONG)
                    {

                        UPTIME_2 = test_params->TS_ELAPSED_TIME.tv_sec + 
                                   ((double)test_params->TS_ELAPSED_TIME.tv_nsec * 1e-9);

                        // Check it's the time value originally sent
                        UPTIME_RX = (double)ntohll(*frame_headers->RX_SUB_TLV_VALUE) / 1000000000.0;

                        if (UPTIME_RX == UPTIME_1)
                        {

                            RTT = UPTIME_2-UPTIME_1;

                            if (RTT < qm_test->RTT_MIN)
                            {
                                qm_test->RTT_MIN = RTT;
                            } else if (RTT > qm_test->RTT_MAX) {
                                qm_test->RTT_MAX = RTT;
                            }

                            JITTER = fabsl(RTT-RTT_PREV);

                            if (JITTER < qm_test->JITTER_MIN)
                            {
                                qm_test->JITTER_MIN = JITTER;
                            } else if (JITTER > qm_test->JITTER_MAX) {
                                qm_test->JITTER_MAX = JITTER;
                            }

                            qm_test->TEST_COUNT += 1;

                            printf ("%" PRIu32 ":\t%.9Lfs\t%.9Lfs\n", qm_test->TEST_COUNT, RTT, JITTER);

                            RTT_PREV = RTT;

                            ECHO_WAITING = false;

                            test_params->F_RX_COUNT += 1;

                        } else {
                            test_params->F_RX_OTHER += 1;
                        }


                    } else {

                        test_params->F_RX_OTHER += 1;

                    }


                    // Check if Rx host has quit/died;
                    if (ntohl(*frame_headers->RX_TLV_VALUE) == VALUE_DYINGGASP)
                    {

                        printf("Rx host has quit\n");
                        return;

                    }


                } // RX_LEN > 0


                // If Tx is waiting for echo reply, check if the echo reply has timed out
                if (ECHO_WAITING) {
                    if ( (test_params->TS_ELAPSED_TIME.tv_sec-
                          test_params->TS_CURRENT_TIME.tv_sec >= qm_test->TIMEOUT_SEC)
                         &&
                         (test_params->TS_ELAPSED_TIME.tv_nsec-
                          test_params->TS_CURRENT_TIME.tv_nsec >= qm_test->TIMEOUT_NSEC) )
                    {

                        ECHO_WAITING = false;
                        printf("*\n");
                        qm_test->TIMEOUT_COUNT += 1;
                        qm_test->TEST_COUNT += 1;

                    }
                }


                // Check if the echo interval has passed (time to send another ping)
                if ( (test_params->TS_ELAPSED_TIME.tv_sec-
                     test_params->TS_CURRENT_TIME.tv_sec >= qm_test->INTERVAL_SEC)
                    &&
                    (test_params->TS_ELAPSED_TIME.tv_nsec-
                     test_params->TS_CURRENT_TIME.tv_nsec >= qm_test->INTERVAL_NSEC) )
                {

                    test_params->TS_CURRENT_TIME.tv_sec = test_params->TS_ELAPSED_TIME.tv_sec;
                    test_params->TS_CURRENT_TIME.tv_nsec = test_params->TS_ELAPSED_TIME.tv_nsec;

                    ECHO_WAITING = false;
                    WAITING = false;

                }


                // Check if 1 second has passed to increment test duration
                if (test_params->TS_ELAPSED_TIME.tv_sec-
                    qm_test->TS_START.tv_sec >= 1) {

                    clock_gettime(CLOCK_MONOTONIC_RAW, &qm_test->TS_START);

                    test_params->S_ELAPSED += 1;

                }


            } // WAITING=1


        } // testBase<=testMax

        printf("Test frames transmitted: %" PRIu64 "\n"
               "Test frames received: %" PRIu64 "\n"
               "Non test or out-of-order frames received: %" PRIu64 "\n"
               "Number of timeouts: %" PRIu32 "\n"
               "Min/Max RTT during test: %.9Lfs/%.9Lfs\n"
               "Min/Max jitter during test: %.9Lfs/%.9Lfs\n",
               test_params->F_TX_COUNT,
               test_params->F_RX_COUNT,
               test_params->F_RX_OTHER,
               qm_test->TIMEOUT_COUNT += 1,
               qm_test->RTT_MIN,
               qm_test->RTT_MAX,
               qm_test->JITTER_MIN,
               qm_test->JITTER_MAX);


    // Else, Rx mode
    } else {


        if (test_params->F_COUNT > 0) {   
            // Testing until N RTT measurements
            testMax = &test_params->F_COUNT;
            testBase = &test_params->F_RX_COUNT;

        } else {
            // Testing until N seconds have passed
            if (test_params->F_DURATION > 0) test_params->F_DURATION -= 1;
            testMax = (uint64_t*)&test_params->F_DURATION;
            testBase = (uint64_t*)&test_params->S_ELAPSED;
        }

        // Wait for the first test frame to be received before starting the test loop
        uint8_t FIRST_FRAME = false;
        while (!FIRST_FRAME)
        {

            RX_LEN = recvfrom(test_interface->SOCKET_FD, frame_headers->RX_BUFFER,
                              test_params->F_SIZE_TOTAL, MSG_PEEK, NULL, NULL);

            // Check if this is an Etherate test frame
            if (ntohl(*frame_headers->RX_TLV_VALUE) == VALUE_TEST_SUB_TLV &&
                ntohs(*frame_headers->RX_SUB_TLV_TYPE) == TYPE_PING)
            {
                FIRST_FRAME = true;
            }

        }


        clock_gettime(CLOCK_MONOTONIC_RAW, &qm_test->TS_START);

        printf("No.\tEcho Interval\n");


        // Rx test loop
        while (*testBase<=*testMax)
        {


            clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->TS_CURRENT_TIME);

            WAITING      = true;
            ECHO_WAITING = true;

            while (WAITING)
            {

                // Get the current time
                clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->TS_ELAPSED_TIME);

                // Poll has been disabled in favour of a non-blocking recvfrom (for now)
                RX_LEN = recvfrom(test_interface->SOCKET_FD,
                                  frame_headers->RX_BUFFER,
                                  test_params->F_SIZE_TOTAL,
                                  MSG_DONTWAIT, NULL, NULL);

                if (RX_LEN > 0) {

                    if ( ntohl(*frame_headers->RX_TLV_VALUE) == VALUE_TEST_SUB_TLV &&
                         ntohs(*frame_headers->RX_SUB_TLV_TYPE) == TYPE_PING )
                    {

                        // Time Rx received this value
                        UPTIME_2 = test_params->TS_ELAPSED_TIME.tv_sec + 
                                   ((double)test_params->TS_ELAPSED_TIME.tv_nsec * 1e-9);

                        // Send the Tx uptime value back to the Tx host
                        build_sub_tlv(frame_headers, htons(TYPE_PONG),
                                      *frame_headers->RX_SUB_TLV_VALUE);

                        TX_RET_VAL = sendto(test_interface->SOCKET_FD,
                                            frame_headers->TX_BUFFER,
                                            frame_headers->LENGTH+frame_headers->SUB_TLV_SIZE,
                                            0, 
                                            (struct sockaddr*)&test_interface->SOCKET_ADDRESS,
                                            sizeof(test_interface->SOCKET_ADDRESS));

                        if (TX_RET_VAL <= 0)
                        {
                            perror("Latency test Rx error ");
                            return;
                        }


                        test_params->F_TX_COUNT += 1;

                        INTERVAL = fabsl(UPTIME_2-UPTIME_1);

                        qm_test->TEST_COUNT += 1;

                        // Interval between receiving this uptime value and the last
                        if (UPTIME_1 != 0.0)
                        {
                            printf("%" PRIu32 ":\t%.9Lfs\n", qm_test->TEST_COUNT, INTERVAL);
                        } else {
                            printf("%" PRIu32 ":\t0.0s\n", qm_test->TEST_COUNT);
                        }
                        UPTIME_1 = UPTIME_2;

                        if (INTERVAL < qm_test->INTERVAL_MIN)
                        {
                            qm_test->INTERVAL_MIN = INTERVAL;
                        } else if (INTERVAL > qm_test->INTERVAL_MAX) {
                            qm_test->INTERVAL_MAX = INTERVAL;
                        }

                        ECHO_WAITING = false;

                        test_params->F_RX_COUNT += 1;

                    } else {

                        test_params->F_RX_OTHER += 1;

                    }

                } // RX_LEN > 0


                // Check if the echo request has timed out
                if (ECHO_WAITING == true) {

                    if ( (test_params->TS_ELAPSED_TIME.tv_sec-
                          test_params->TS_CURRENT_TIME.tv_sec >= qm_test->INTERVAL_SEC)
                         &&
                         (test_params->TS_ELAPSED_TIME.tv_nsec-
                          test_params->TS_CURRENT_TIME.tv_nsec >= qm_test->INTERVAL_NSEC) )
                    {

                        printf("*\n");
                        qm_test->TIMEOUT_COUNT += 1;
                        qm_test->TEST_COUNT += 1;
                        ECHO_WAITING = false;

                    }
                    
                }


                // Check if the echo interval has passed (time to receive another ping)
                if ( (test_params->TS_ELAPSED_TIME.tv_sec-
                     test_params->TS_CURRENT_TIME.tv_sec >= qm_test->INTERVAL_SEC)
                    &&
                    (test_params->TS_ELAPSED_TIME.tv_nsec-
                     test_params->TS_CURRENT_TIME.tv_nsec >= qm_test->INTERVAL_NSEC) )
                {

                    test_params->TS_CURRENT_TIME.tv_sec = test_params->TS_ELAPSED_TIME.tv_sec;
                    test_params->TS_CURRENT_TIME.tv_nsec = test_params->TS_ELAPSED_TIME.tv_nsec;

                    ECHO_WAITING = false;
                    WAITING      = false;

                }


                // Check if 1 second has passed to increment test duration
                if (test_params->TS_ELAPSED_TIME.tv_sec-
                    qm_test->TS_START.tv_sec >= 1) {

                    clock_gettime(CLOCK_MONOTONIC_RAW, &qm_test->TS_START);

                    test_params->S_ELAPSED += 1;

                }


                // Check if Tx host has quit/died;
                if (ntohl(*frame_headers->RX_TLV_VALUE) == VALUE_DYINGGASP)
                {

                    printf("Tx host has quit\n");
                    return;

                }


            } // WAITING


        } // testBase<=testMax

        printf("Test frames transmitted: %" PRIu64 "\n"
               "Test frames received: %" PRIu64 "\n"
               "Non test frames received: %" PRIu64 "\n"
               "Number of timeouts: %u\n"
               "Min/Max interval during test %.9Lfs/%.9Lfs\n",
               test_params->F_TX_COUNT,
               test_params->F_RX_COUNT,
               test_params->F_RX_OTHER,
               qm_test->TIMEOUT_COUNT,
               qm_test->INTERVAL_MIN,
               qm_test->INTERVAL_MAX);


    } // End of Tx/Rx

 }



void speed_test(struct app_params *app_params,
                struct frame_headers *frame_headers,
                struct test_interface *test_interface,
                struct test_params *test_params)
{

    int16_t TX_RET_VAL = 0;
    int16_t RX_LEN     = 0;

    build_tlv(frame_headers, htons(TYPE_TESTFRAME), htonl(VALUE_TEST_SUB_TLV));


    if (app_params->TX_MODE == true)
    {

        printf("Seconds\t\tMbps Tx\t\tMBs Tx\t\tFrmTx/s\t\tFrames Tx\n");

        // By default test until F_DURATION_DEF has passed
        uint64_t *testMax = (uint64_t*)&test_params->F_DURATION;
        uint64_t *testBase = (uint64_t*)&test_params->S_ELAPSED;

        if (test_params->F_BYTES > 0)
        {    
            // Testing until N bytes sent
            testMax = &test_params->F_BYTES;
            testBase = &test_params->B_TX;

        } else if (test_params->F_COUNT > 0) {   
            // Testing until N frames sent
            testMax = &test_params->F_COUNT;
            testBase = &test_params->F_TX_COUNT;

        } else if (test_params->F_DURATION > 0) {
            // Testing until N seconds have passed
            test_params->F_DURATION -= 1;
        }


        // Get clock times for the speed limit restriction and starting time
        clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->TS_ELAPSED_TIME);

        // Tx test loop
        while (*testBase<=*testMax)
        {

            clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->TS_CURRENT_TIME);

            // One second has passed
            if ((test_params->TS_CURRENT_TIME.tv_sec - test_params->TS_ELAPSED_TIME.tv_sec) >= 1)
            {
                test_params->S_ELAPSED += 1;
                test_params->B_SPEED = (((double)test_params->B_TX-test_params->B_TX_PREV) * 8) / 1000 / 1000;
                test_params->B_TX_PREV = test_params->B_TX;
                test_params->F_SPEED = (test_params->F_TX_COUNT - test_params->F_TX_COUNT_PREV);
                test_params->F_TX_COUNT_PREV = test_params->F_TX_COUNT;

                printf("%" PRIu64 "\t\t%.2f\t\t%" PRIu64 "\t\t%" PRIu64"\t\t%" PRIu64 "\n",
                       test_params->S_ELAPSED,
                       test_params->B_SPEED,
                       (test_params->B_TX/1024)/1024,
                       (test_params->F_SPEED),
                       test_params->F_TX_COUNT);

                if (test_params->B_SPEED > test_params->B_SPEED_MAX)
                    test_params->B_SPEED_MAX = test_params->B_SPEED;

                if (test_params->F_SPEED > test_params->F_SPEED_MAX)
                    test_params->F_SPEED_MAX = test_params->F_SPEED;

                test_params->B_SPEED_AVG += test_params->B_SPEED;
                test_params->F_SPEED_AVG += test_params->F_SPEED;
                test_params->TS_ELAPSED_TIME.tv_sec = test_params->TS_CURRENT_TIME.tv_sec;
                test_params->TS_ELAPSED_TIME.tv_nsec = test_params->TS_CURRENT_TIME.tv_nsec;

                test_params->B_TX_SPEED_PREV = 0;

            } else {

                // Poll has been disabled in favour of a non-blocking recvfrom (for now)
                RX_LEN = recvfrom(test_interface->SOCKET_FD, frame_headers->RX_BUFFER,
                                  test_params->F_SIZE_TOTAL, MSG_DONTWAIT, NULL, NULL);

                if (RX_LEN > 0) {

                    // Running in ACK mode
                    if (test_params->F_ACK)
                    {

                        if (ntohl(*frame_headers->RX_TLV_VALUE) == VALUE_TEST_SUB_TLV &&
                            ntohs(*frame_headers->RX_SUB_TLV_TYPE) == TYPE_ACKINDEX)
                        {

                            test_params->F_RX_COUNT += 1;
                            test_params->F_WAITING_ACK = false;

                        if (ntohll(*frame_headers->RX_SUB_TLV_VALUE) == (test_params->F_INDEX_PREV+1)) {
                            test_params->F_RX_ONTIME += 1;
                            test_params->F_INDEX_PREV += 1;
                        } else if (ntohll(*frame_headers->RX_SUB_TLV_VALUE) > (test_params->F_RX_COUNT)) {
                            test_params->F_INDEX_PREV = ntohll(*frame_headers->RX_SUB_TLV_VALUE);
                            test_params->F_RX_EARLY += 1;
                        } else if (ntohll(*frame_headers->RX_SUB_TLV_VALUE) <= test_params->F_RX_COUNT) {
                            test_params->F_RX_LATE += 1;
                        }

                        } else if (ntohs(*frame_headers->RX_TLV_TYPE) == TYPE_APPLICATION &&
                                   ntohl(*frame_headers->RX_TLV_VALUE) == VALUE_DYINGGASP) {

                            printf("Rx host has quit\n");

                            return;

                        // Received a non-test frame
                        } else {
                            test_params->F_RX_OTHER += 1;
                        }

                    // Not running in ACK mode
                    } else if (ntohs(*frame_headers->RX_TLV_TYPE) == TYPE_APPLICATION &&
                               ntohll(*frame_headers->RX_TLV_VALUE) == VALUE_DYINGGASP) {

                        printf("Rx host has quit\n");

                        return;

                    // Received a non-test frame
                    } else {
                        test_params->F_RX_OTHER += 1;
                    }

                } // RX_LEN > 0

                // If it hasn't been 1 second yet, send more test frames
                if (test_params->F_WAITING_ACK == false)
                {

                    // Check if a max speed has been configured
                    if (test_params->B_TX_SPEED_MAX != B_TX_SPEED_MAX_DEF)
                    {

                        // Check if sending another frame exceeds the max speed configured
                        if ((test_params->B_TX_SPEED_PREV + test_params->F_SIZE) <= test_params->B_TX_SPEED_MAX)
                        {


                            build_sub_tlv(frame_headers, htons(TYPE_FRAMEINDEX), 
                                          htonll(test_params->F_TX_COUNT+1));

                            TX_RET_VAL = sendto(test_interface->SOCKET_FD,
                                                frame_headers->TX_BUFFER,
                                                test_params->F_SIZE_TOTAL, 0, 
                                                (struct sockaddr*)&test_interface->SOCKET_ADDRESS,
                                                sizeof(test_interface->SOCKET_ADDRESS));

                            if (TX_RET_VAL <= 0)
                            {
                                perror("Speed test Tx error ");
                                return;
                            }


                            test_params->F_TX_COUNT += 1;
                            test_params->B_TX += test_params->F_SIZE;
                            test_params->B_TX_SPEED_PREV += test_params->F_SIZE;
                            if (test_params->F_ACK) test_params->F_WAITING_ACK = true;

                        }

                    // Check if a max frames per second limit is configured
                    } else if (test_params->F_TX_SPEED_MAX != F_TX_SPEED_MAX_DEF) {

                        // Check if sending another frame exceeds the max frame rate configured
                        if (test_params->F_TX_COUNT - test_params->F_TX_COUNT_PREV < test_params->F_TX_SPEED_MAX)
                        {

                            build_sub_tlv(frame_headers, htons(TYPE_FRAMEINDEX), 
                                          htonll(test_params->F_TX_COUNT+1));

                            TX_RET_VAL = sendto(test_interface->SOCKET_FD,
                                                frame_headers->TX_BUFFER,
                                                test_params->F_SIZE_TOTAL, 0, 
                                                (struct sockaddr*)&test_interface->SOCKET_ADDRESS,
                                                sizeof(test_interface->SOCKET_ADDRESS));

                            if (TX_RET_VAL <= 0)
                            {
                                perror("Speed test Tx error ");
                                return;
                            }


                            test_params->F_TX_COUNT += 1;
                            test_params->B_TX += test_params->F_SIZE;
                            test_params->B_TX_SPEED_PREV += test_params->F_SIZE;
                            if (test_params->F_ACK) test_params->F_WAITING_ACK = true;

                        }

                    // Else there are no speed restrictions
                    } else {

                        build_sub_tlv(frame_headers, htons(TYPE_FRAMEINDEX),
                                      htonll(test_params->F_TX_COUNT+1));

                        TX_RET_VAL = sendto(test_interface->SOCKET_FD,
                                            frame_headers->TX_BUFFER,
                                            test_params->F_SIZE_TOTAL, 0, 
                                            (struct sockaddr*)&test_interface->SOCKET_ADDRESS,
                                            sizeof(test_interface->SOCKET_ADDRESS));

                        if (TX_RET_VAL <= 0)
                        {
                            perror("Speed test Tx error ");
                            return;
                        }


                        test_params->F_TX_COUNT += 1;
                        test_params->B_TX += test_params->F_SIZE;
                        test_params->B_TX_SPEED_PREV += test_params->F_SIZE;
                        if (test_params->F_ACK) test_params->F_WAITING_ACK = true;

                    }
                }


            } // End of TS_CURRENT_TIME.tv_sec - S_ELAPSED_TIME.tv_sec

        }


        if (test_params->S_ELAPSED > 0)
        {
            test_params->B_SPEED_AVG = (test_params->B_SPEED_AVG/test_params->S_ELAPSED);
            test_params->F_SPEED_AVG = (test_params->F_SPEED_AVG/test_params->S_ELAPSED);
        }

        printf("Test frames transmitted: %" PRIu64 "\n"
               "Test frames received: %" PRIu64 "\n"
               "Non test frames received: %" PRIu64 "\n"
               "In order ACK frames received: %" PRIu64 "\n"
               "Out of order ACK frames received early: %" PRIu64 "\n"
               "Out of order ACK frames received late: %" PRIu64 "\n"
               "Maximum speed during test: %.2fMbps, %" PRIu64 "Fps\n"
               "Average speed during test: %.2LfMbps, %" PRIu64 "Fps\n"
               "Data transmitted during test: %" PRIu64 "MBs\n",
               test_params->F_TX_COUNT,
               test_params->F_RX_COUNT,
               test_params->F_RX_OTHER,
               test_params->F_RX_ONTIME,
               test_params->F_RX_EARLY,
               test_params->F_RX_LATE,
               test_params->B_SPEED_MAX,
               test_params->F_SPEED_MAX,
               test_params->B_SPEED_AVG,
               test_params->F_SPEED_AVG,
               (test_params->B_TX/1024)/1024);


    // Else, Rx mode
    } else {

        printf("Seconds\t\tMbps Rx\t\tMBs Rx\t\tFrmRx/s\t\tFrames Rx\n");

        // By default test until F_DURATION_DEF has passed
        uint64_t *testMax = (uint64_t*)&test_params->F_DURATION;
        uint64_t *testBase = (uint64_t*)&test_params->S_ELAPSED;

        // Testing until N bytes received
        if (test_params->F_BYTES > 0)
        {
            testMax = &test_params->F_BYTES;
            testBase = &test_params->B_RX;

        // Testing until N frames received
        } else if (test_params->F_COUNT > 0) {
            testMax = &test_params->F_COUNT;
            testBase = &test_params->F_RX_COUNT;

        // Testing until N seconds have passed
        } else if (test_params->F_DURATION > 0) {
            test_params->F_DURATION -= 1;
        }


        // Wait for the first test frame to be received before starting the test loop
        uint8_t FIRST_FRAME = false;
        while (!FIRST_FRAME)
        {

            RX_LEN = recvfrom(test_interface->SOCKET_FD, frame_headers->RX_BUFFER,
                              test_params->F_SIZE_TOTAL, MSG_PEEK, NULL, NULL);

            // Check if this is an Etherate test frame
            if (ntohl(*frame_headers->RX_TLV_VALUE) == VALUE_TEST_SUB_TLV &&
                ntohs(*frame_headers->RX_SUB_TLV_TYPE) == TYPE_FRAMEINDEX)
            {
                FIRST_FRAME = true;
            }

        }

        clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->TS_ELAPSED_TIME);

        // Rx test loop
        while (*testBase<=*testMax)
        {

            clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->TS_CURRENT_TIME);

            // If one second has passed
            if ((test_params->TS_CURRENT_TIME.tv_sec-test_params->TS_ELAPSED_TIME.tv_sec) >= 1)
            {
                test_params->S_ELAPSED += 1;
                test_params->B_SPEED = (double)(test_params->B_RX-test_params->B_RX_PREV) * 8 / 1000000;
                test_params->B_RX_PREV = test_params->B_RX;
                test_params->F_SPEED = (test_params->F_RX_COUNT - test_params->F_RX_COUNT_PREV);
                test_params->F_RX_COUNT_PREV = test_params->F_RX_COUNT;

                printf("%" PRIu64 "\t\t%.2f\t\t%" PRIu64 "\t\t%" PRIu64 "\t\t%" PRIu64 "\n",
                       test_params->S_ELAPSED,
                       test_params->B_SPEED,
                       (test_params->B_RX/1024)/1024,
                       (test_params->F_SPEED),
                       test_params->F_RX_COUNT);

                if (test_params->B_SPEED > test_params->B_SPEED_MAX)
                    test_params->B_SPEED_MAX = test_params->B_SPEED;

                if (test_params->F_SPEED > test_params->F_SPEED_MAX)
                    test_params->F_SPEED_MAX = test_params->F_SPEED;

                test_params->B_SPEED_AVG += test_params->B_SPEED;
                test_params->F_SPEED_AVG += test_params->F_SPEED;
                test_params->TS_ELAPSED_TIME.tv_sec = test_params->TS_CURRENT_TIME.tv_sec;
                test_params->TS_ELAPSED_TIME.tv_nsec = test_params->TS_CURRENT_TIME.tv_nsec;

            }

            // Poll has been disabled in favour of a non-blocking recvfrom (for now)
            RX_LEN = recvfrom(test_interface->SOCKET_FD,
                              frame_headers->RX_BUFFER,
                              test_params->F_SIZE_TOTAL,
                              MSG_DONTWAIT, NULL, NULL);

            if (RX_LEN > 0)
            {

                // Check if this is an Etherate test frame
                if (likely(ntohl(*frame_headers->RX_TLV_VALUE) == VALUE_TEST_SUB_TLV &&
                    ntohs(*frame_headers->RX_SUB_TLV_TYPE) == TYPE_FRAMEINDEX))
                {

                    // Update test stats
                    test_params->F_RX_COUNT += 1;
                    test_params->B_RX+=(RX_LEN-frame_headers->LENGTH);

                    // Record if the frame is in-order, early or late
                    if (likely(ntohll(*frame_headers->RX_SUB_TLV_VALUE) == (test_params->F_INDEX_PREV+1))) {
                        test_params->F_RX_ONTIME += 1;
                        test_params->F_INDEX_PREV += 1;
                    } else if (ntohll(*frame_headers->RX_SUB_TLV_VALUE) > (test_params->F_RX_COUNT)) {
                        test_params->F_INDEX_PREV = ntohll(*frame_headers->RX_SUB_TLV_VALUE);
                        test_params->F_RX_EARLY += 1;
                    } else if (ntohll(*frame_headers->RX_SUB_TLV_VALUE) <= test_params->F_RX_COUNT) {
                        test_params->F_RX_LATE += 1;
                    }

                    // If running in ACK mode Rx needs to ACK to Tx host
                    if (test_params->F_ACK)
                    {

                        build_sub_tlv(frame_headers, htons(TYPE_ACKINDEX),
                                      *frame_headers->RX_SUB_TLV_VALUE);

                        TX_RET_VAL = sendto(test_interface->SOCKET_FD,
                                            frame_headers->TX_BUFFER,
                                            frame_headers->LENGTH+frame_headers->SUB_TLV_SIZE, 0, 
                                            (struct sockaddr*)&test_interface->SOCKET_ADDRESS,
                                            sizeof(test_interface->SOCKET_ADDRESS));

                        if (TX_RET_VAL <= 0)
                        {
                            perror("Speed test Tx error ");
                            return;
                        }


                        test_params->F_TX_COUNT += 1;
                        
                    }

                } else {
                    // Received a non-test frame
                    test_params->F_RX_OTHER += 1;
                }

                // Check if Tx host has quit/died;
                if (unlikely(ntohl(*frame_headers->RX_TLV_VALUE) == VALUE_DYINGGASP))
                {

                    printf("Tx host has quit\n");
                    return;

                }
                      

            } // RX_LEN > 0


        } // *testBase<=*testMax


        if (test_params->S_ELAPSED > 0)
        {
            test_params->B_SPEED_AVG = (test_params->B_SPEED_AVG/test_params->S_ELAPSED);
            test_params->F_SPEED_AVG = (test_params->F_SPEED_AVG/test_params->S_ELAPSED);
        }

        printf("Test frames transmitted: %" PRIu64 "\n"
               "Test frames received: %" PRIu64 "\n"
               "Non test frames received: %" PRIu64 "\n"
               "In order test frames received: %" PRIu64 "\n"
               "Out of order test frames received early: %" PRIu64 "\n"
               "Out of order test frames received late: %" PRIu64 "\n"
               "Maximum speed during test: %.2fMbps, %" PRIu64 "Fps\n"
               "Average speed during test: %.2LfMbps, %" PRIu64 "Fps\n"
               "Data received during test: %" PRIu64 "MBs\n",
               test_params->F_TX_COUNT,
               test_params->F_RX_COUNT,
               test_params->F_RX_OTHER,
               test_params->F_RX_ONTIME,
               test_params->F_RX_EARLY,
               test_params->F_RX_LATE,
               test_params->B_SPEED_MAX,
               test_params->F_SPEED_MAX,
               test_params->B_SPEED_AVG,
               test_params->F_SPEED_AVG,
               (test_params->B_RX/1024)/1024);

    }

 }



void send_custom_frame(struct app_params *app_params,
                       struct frame_headers *frame_headers,
                       struct test_interface *test_interface,
                       struct test_params *test_params)
{

    int16_t TX_RET_VAL = 0;

    printf("Seconds\t\tMbps Tx\t\tMBs Tx\t\tFrmTx/s\t\tFrames Tx\n");

    // By default test until F_DURATION_DEF has passed
    uint64_t *testMax = (uint64_t*)&test_params->F_DURATION;
    uint64_t *testBase = (uint64_t*)&test_params->S_ELAPSED;

    if (test_params->F_BYTES > 0)
    {    
        // Testing until N bytes sent
        testMax = &test_params->F_BYTES;
        testBase = &test_params->B_TX;

    } else if (test_params->F_COUNT > 0) {   
        // Testing until N frames sent
        testMax = &test_params->F_COUNT;
        testBase = &test_params->F_TX_COUNT;

    } else if (test_params->F_DURATION > 0) {
        // Testing until N seconds have passed
        test_params->F_DURATION -= 1;
    }


    // Get clock times for the speed limit restriction and starting time
    clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->TS_ELAPSED_TIME);

    // Tx test loop
    while (*testBase<=*testMax)
    {

        clock_gettime(CLOCK_MONOTONIC_RAW, &test_params->TS_CURRENT_TIME);

        // One second has passed
        if ((test_params->TS_CURRENT_TIME.tv_sec - test_params->TS_ELAPSED_TIME.tv_sec) >= 1)
        {
            test_params->S_ELAPSED += 1;
            test_params->B_SPEED = ((double)(test_params->B_TX - test_params->B_TX_PREV) * 8) / 1000 / 1000;
            test_params->B_TX_PREV = test_params->B_TX;
            test_params->F_SPEED = (test_params->F_TX_COUNT - test_params->F_TX_COUNT_PREV);
            test_params->F_TX_COUNT_PREV = test_params->F_TX_COUNT;

            printf("%" PRIu64 "\t\t%.2f\t\t%" PRIu64 "\t\t%" PRIu64"\t\t%" PRIu64 "\n",
                   test_params->S_ELAPSED,
                   test_params->B_SPEED,
                   (test_params->B_TX/1024)/1024,
                   (test_params->F_SPEED),
                   test_params->F_TX_COUNT);

            if (test_params->B_SPEED > test_params->B_SPEED_MAX)
                test_params->B_SPEED_MAX = test_params->B_SPEED;

            if (test_params->F_SPEED > test_params->F_SPEED_MAX)
                test_params->F_SPEED_MAX = test_params->F_SPEED;

            test_params->B_SPEED_AVG += test_params->B_SPEED;
            test_params->F_SPEED_AVG += test_params->F_SPEED;
            test_params->TS_ELAPSED_TIME.tv_sec = test_params->TS_CURRENT_TIME.tv_sec;
            test_params->TS_ELAPSED_TIME.tv_nsec = test_params->TS_CURRENT_TIME.tv_nsec;

            test_params->B_TX_SPEED_PREV = 0;

        } else {


            // Check if a max speed has been configured
            if (test_params->B_TX_SPEED_MAX != B_TX_SPEED_MAX_DEF)
            {

                // Check if sending another frame exceeds the max speed configured
                if ((test_params->B_TX_SPEED_PREV + test_params->F_PAYLOAD_SIZE) <= test_params->B_TX_SPEED_MAX)
                {


                    TX_RET_VAL = sendto(test_interface->SOCKET_FD,
                                        test_params->F_PAYLOAD,
                                        test_params->F_PAYLOAD_SIZE, 0, 
                                        (struct sockaddr*)&test_interface->SOCKET_ADDRESS,
                                        sizeof(test_interface->SOCKET_ADDRESS));

                    if (TX_RET_VAL <= 0)
                    {
                        perror("Frame sending Tx error ");
                        return;
                    }


                    test_params->F_TX_COUNT += 1;
                    test_params->B_TX += test_params->F_PAYLOAD_SIZE;
                    test_params->B_TX_SPEED_PREV += test_params->F_PAYLOAD_SIZE;

                }

            // Check if a max frames per second limit is configured
            } else if (test_params->F_TX_SPEED_MAX != F_TX_SPEED_MAX_DEF) {

                // Check if sending another frame exceeds the max frame rate configured
                if (test_params->F_TX_COUNT - test_params->F_TX_COUNT_PREV < test_params->F_TX_SPEED_MAX)
                {

                    TX_RET_VAL = sendto(test_interface->SOCKET_FD,
                                        test_params->F_PAYLOAD,
                                        test_params->F_PAYLOAD_SIZE, 0, 
                                        (struct sockaddr*)&test_interface->SOCKET_ADDRESS,
                                        sizeof(test_interface->SOCKET_ADDRESS));

                    if (TX_RET_VAL <= 0)
                    {
                        perror("Frame sending Tx error ");
                        return;
                    }


                    test_params->F_TX_COUNT += 1;
                    test_params->B_TX += test_params->F_PAYLOAD_SIZE;
                    test_params->B_TX_SPEED_PREV += test_params->F_PAYLOAD_SIZE;

                }

            // Else there are no speed restrictions
            } else {

                TX_RET_VAL = sendto(test_interface->SOCKET_FD,
                                    test_params->F_PAYLOAD,
                                    test_params->F_PAYLOAD_SIZE, 0, 
                                    (struct sockaddr*)&test_interface->SOCKET_ADDRESS,
                                    sizeof(test_interface->SOCKET_ADDRESS));

                if (TX_RET_VAL <= 0)
                {
                    perror("Frame sending Tx error ");
                    return;
                }


                test_params->F_TX_COUNT += 1;
                test_params->B_TX += test_params->F_PAYLOAD_SIZE;
                test_params->B_TX_SPEED_PREV += test_params->F_PAYLOAD_SIZE;
            }

        } // End of TS_CURRENT_TIME.tv_sec - TS_ELAPSED_TIME.tv_sec

    }


    if (test_params->S_ELAPSED > 0)
    {
        test_params->B_SPEED_AVG = (test_params->B_SPEED_AVG/test_params->S_ELAPSED);
        test_params->F_SPEED_AVG = (test_params->F_SPEED_AVG/test_params->S_ELAPSED);
    }

    printf("Test frames transmitted: %" PRIu64 "\n"
           "Test frames received: %" PRIu64 "\n"
           "Non test frames received: %" PRIu64 "\n"
           "In order ACK frames received: %" PRIu64 "\n"
           "Out of order ACK frames received early: %" PRIu64 "\n"
           "Out of order ACK frames received late: %" PRIu64 "\n"
           "Maximum speed during test: %.2fMbps, %" PRIu64 "Fps\n"
           "Average speed during test: %.2LfMbps, %" PRIu64 "Fps\n"
           "Data transmitted during test: %" PRIu64 "MBs\n",
           test_params->F_TX_COUNT,
           test_params->F_RX_COUNT,
           test_params->F_RX_OTHER,
           test_params->F_RX_ONTIME,
           test_params->F_RX_EARLY,
           test_params->F_RX_LATE,
           test_params->B_SPEED_MAX,
           test_params->F_SPEED_MAX,
           test_params->B_SPEED_AVG,
           test_params->F_SPEED_AVG,
           (test_params->B_TX/1024)/1024);


 }
