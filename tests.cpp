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

// Calculate the one-way delay from TX to RX
void delay_test(struct APP_PARAMS *APP_PARAMS,
                struct FRAME_HEADERS *FRAME_HEADERS,
                struct TEST_INTERFACE *TEST_INTERFACE,
                struct TEST_PARAMS * TEST_PARAMS,
                struct QM_TEST *QM_TEST);

// Run an MTU sweep test from TX to RX
void mtu_sweep_test(struct APP_PARAMS *APP_PARAMS,
                    struct FRAME_HEADERS *FRAME_HEADERS,
                    struct TEST_INTERFACE *TEST_INTERFACE,
                    struct TEST_PARAMS *TEST_PARAMS,
                    struct MTU_TEST *MTU_TEST);

// Run some quality measurements from TX to RX
void latency_test(struct APP_PARAMS *APP_PARAMS,
                  struct FRAME_HEADERS *FRAME_HEADERS,
                  struct TEST_INTERFACE *TEST_INTERFACE,
                  struct TEST_PARAMS *TEST_PARAMS,
                  struct QM_TEST *QM_TEST);

// Run a speedtest (which is the default test operation if none is specified)
void speed_test(struct APP_PARAMS *APP_PARAMS,
                struct FRAME_HEADERS *FRAME_HEADERS,
                struct TEST_INTERFACE *TEST_INTERFACE,
                struct TEST_PARAMS *TEST_PARAMS);

// Tx a custom frame loaded from file
void send_custom_frame(struct APP_PARAMS *APP_PARAMS,
                       struct FRAME_HEADERS *FRAME_HEADERS,
                       struct TEST_INTERFACE *TEST_INTERFACE,
                       struct TEST_PARAMS *TEST_PARAMS);



void delay_test(struct APP_PARAMS *APP_PARAMS,
                struct FRAME_HEADERS *FRAME_HEADERS,
                struct TEST_INTERFACE *TEST_INTERFACE,
                struct TEST_PARAMS * TEST_PARAMS,
                struct QM_TEST *QM_TEST)
{
// Calculate the delay between TX and RX hosts. The uptime is exchanged twice
// to estimate the delay between the hosts. Then the process is repeated so
// an average can be taken


    long double UPTIME;
    uint64_t    TX_UPTIME;
    int16_t     TX_RET_VAL;
    int16_t     RX_LEN;
    uint8_t     WAITING;


    build_tlv(FRAME_HEADERS, htons(TYPE_TESTFRAME), htonl(VALUE_TEST_SUB_TLV));

    printf("Calculating delay between hosts...\n");

    if(APP_PARAMS->TX_MODE == true)
    {

        for (uint32_t i=0; i < QM_TEST->DELAY_TEST_COUNT; i += 1)
        {

            // Get the current time and send it to RX
            clock_gettime(CLOCK_MONOTONIC_RAW, &QM_TEST->TS_RTT);

            // Convert it to double then long long (uint64t_) for transmission
            UPTIME = QM_TEST->TS_RTT.tv_sec + ((double)QM_TEST->TS_RTT.tv_nsec*1e-9);
            TX_UPTIME = roundl(UPTIME*1000000000.0);

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_DELAY1), htonll(TX_UPTIME));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD,
                                FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0,
                                (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            if (TX_RET_VAL <=0)
            {
                perror("Delay test TX error ");
                return;
            }

            // Repeat
            clock_gettime(CLOCK_MONOTONIC_RAW, &QM_TEST->TS_RTT);

            UPTIME = QM_TEST->TS_RTT.tv_sec + ((double)QM_TEST->TS_RTT.tv_nsec*1e-9);
            TX_UPTIME = roundl(UPTIME * 1000000000.0);

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_DELAY2), htonll(TX_UPTIME));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD,
                                FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0,
                                (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            if (TX_RET_VAL <=0)
            {
                perror("Delay test TX error ");
                return;
            }

            // Wait for RX to send back delay value

            WAITING = true;
            while (WAITING)
            {
                RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD,
                                  FRAME_HEADERS->RX_BUFFER,
                                  TEST_PARAMS->F_SIZE_TOTAL,
                                  0, NULL, NULL);

                if (RX_LEN >0)
                {

                    if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_DELAY)
                    {
                        QM_TEST->pDELAY_RESULTS[i] = (ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE) / 1000000000.0);
                        WAITING = false;
                    }

                } else if (RX_LEN <0) {

                    perror("Delay test TX error ");
                    return;

                }

            }

        } // End delay TX loop

        // Let the receiver know all delay values have been received
        build_tlv(FRAME_HEADERS, htons(TYPE_TESTFRAME), htonl(VALUE_DELAY_END));

        build_sub_tlv(FRAME_HEADERS, htons(TYPE_TESTFRAME),
                      htonll(VALUE_DELAY_END));

        TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD,
                            FRAME_HEADERS->TX_BUFFER,
                            FRAME_HEADERS->LENGTH+FRAME_HEADERS->TLV_SIZE,
                            0,
                            (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                            sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

        if (TX_RET_VAL <=0)
        {
            perror("Delay test TX error ");
            return;
        }


        double DELAY_AVERAGE = 0;

        for (uint32_t i=0; i < QM_TEST->DELAY_TEST_COUNT; i += 1)
        {
            DELAY_AVERAGE += QM_TEST->pDELAY_RESULTS[i];
        }

        DELAY_AVERAGE = (DELAY_AVERAGE/QM_TEST->DELAY_TEST_COUNT);

        printf("Tx to Rx delay calculated as %.9f seconds\n\n", DELAY_AVERAGE);


    // This is the RX host
    } else {


        // These values are used to calculate the delay between TX and RX hosts
        double *TIME_TX_1 = (double*)calloc(QM_TEST->DELAY_TEST_COUNT, sizeof(double));
        double *TIME_TX_2 = (double*)calloc(QM_TEST->DELAY_TEST_COUNT, sizeof(double));
        double *TIME_RX_1 = (double*)calloc(QM_TEST->DELAY_TEST_COUNT, sizeof(double));
        double *TIME_RX_2 = (double*)calloc(QM_TEST->DELAY_TEST_COUNT, sizeof(double));
        double *TIME_TX_DIFF = (double*)calloc(QM_TEST->DELAY_TEST_COUNT, sizeof(double));
        double *TIME_RX_DIFF = (double*)calloc(QM_TEST->DELAY_TEST_COUNT, sizeof(double));

        uint32_t DELAY_INDEX = 0;
        
        WAITING = true;

        while(WAITING)
        {

            RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD,
                              FRAME_HEADERS->RX_BUFFER,
                              TEST_PARAMS->F_SIZE_TOTAL,
                              0, NULL, NULL);

            if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_DELAY1)
            {

                // Get the time RX is receiving TX's sent time figure
                clock_gettime(CLOCK_MONOTONIC_RAW, &QM_TEST->TS_RTT);

                // Record the time RX received this TX value
                TIME_RX_1[DELAY_INDEX] = QM_TEST->TS_RTT.tv_sec + ((double)QM_TEST->TS_RTT.tv_nsec*1e-9);
                // Record the TX value received
                TIME_TX_1[DELAY_INDEX] = (ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE)/1000000000.0);


            } else if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_DELAY2) {


                clock_gettime(CLOCK_MONOTONIC_RAW, &QM_TEST->TS_RTT);
                TIME_RX_2[DELAY_INDEX] = QM_TEST->TS_RTT.tv_sec + ((double)QM_TEST->TS_RTT.tv_nsec*1e-9);
                TIME_TX_2[DELAY_INDEX] = (ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE)/1000000000.0);

                // Calculate the delay
                TIME_TX_DIFF[DELAY_INDEX] = TIME_TX_2[DELAY_INDEX]-TIME_TX_1[DELAY_INDEX];
                TIME_RX_DIFF[DELAY_INDEX] = TIME_RX_2[DELAY_INDEX]-TIME_RX_1[DELAY_INDEX];

                if (TIME_RX_DIFF[DELAY_INDEX]-TIME_TX_DIFF[DELAY_INDEX] > 0) {

                    QM_TEST->pDELAY_RESULTS[DELAY_INDEX] = TIME_RX_DIFF[DELAY_INDEX]-TIME_TX_DIFF[DELAY_INDEX];

                // This value returned is minus and thus invalid
                } else {

                    QM_TEST->pDELAY_RESULTS[DELAY_INDEX] = 0;
                }


                // Send the calculated delay back to the TX host
                build_sub_tlv(FRAME_HEADERS, htons(TYPE_DELAY),
                              htonll(roundl(QM_TEST->pDELAY_RESULTS[DELAY_INDEX]*1000000000.0)));

                TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                    FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                    0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                    sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

                if (TX_RET_VAL <=0)
                {
                    perror("Delay test TX error ");
                    return;
                }


                DELAY_INDEX += 1;


            } else if (ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_DELAY_END) {

                WAITING = false;

                double DELAY_AVERAGE = 0;

                for (uint32_t i=0; i < QM_TEST->DELAY_TEST_COUNT; i += 1)
                {
                    DELAY_AVERAGE += QM_TEST->pDELAY_RESULTS[i];
                }

                DELAY_AVERAGE = (DELAY_AVERAGE/QM_TEST->DELAY_TEST_COUNT);

                printf("Tx to Rx delay calculated as %.9f seconds\n\n", DELAY_AVERAGE);

            }


        } // End of WAITING

        free(TIME_TX_1);
        free(TIME_TX_2);
        free(TIME_RX_1);
        free(TIME_RX_2);
        free(TIME_TX_DIFF);
        free(TIME_RX_DIFF);

    } // End of RX mode

    return;
}



void mtu_sweep_test(struct APP_PARAMS *APP_PARAMS,
                    struct FRAME_HEADERS *FRAME_HEADERS,
                    struct TEST_INTERFACE *TEST_INTERFACE,
                    struct TEST_PARAMS *TEST_PARAMS,
                    struct MTU_TEST *MTU_TEST)
{

    // Check the interface MTU
    uint16_t PHY_MTU = get_interface_mtu_by_name(TEST_INTERFACE);

    if (MTU_TEST->MTU_TX_MAX > PHY_MTU) {
        printf("Starting MTU sweep from %u to %u (interface max)\n",
               MTU_TEST->MTU_TX_MIN, PHY_MTU);
        MTU_TEST->MTU_TX_MAX = PHY_MTU;

    } else {

        printf("Starting MTU sweep from %u to %u\n",
               MTU_TEST->MTU_TX_MIN, MTU_TEST->MTU_TX_MAX);
    
    }


    int16_t TX_RET_VAL = 0;
    int16_t RX_LEN     = 0;

    build_tlv(FRAME_HEADERS, htons(TYPE_TESTFRAME), htonl(VALUE_TEST_SUB_TLV));

    if (APP_PARAMS->TX_MODE == true) {

        uint16_t MTU_TX_CURRENT   = 0;
        uint16_t MTU_ACK_PREVIOUS = 0;
        uint16_t MTU_ACK_CURRENT  = 0;
        uint16_t MTU_ACK_LARGEST  = 0;
        uint8_t  WAITING          = true;

        for (MTU_TX_CURRENT = MTU_TEST->MTU_TX_MIN; MTU_TX_CURRENT <= MTU_TEST->MTU_TX_MAX; MTU_TX_CURRENT += 1)
        {

            // Get the current time
            clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_ELAPSED_TIME);

            // Send each MTU test frame 3 times
            for (uint16_t i = 1; i <= 3; i += 1)
            {

                build_sub_tlv(FRAME_HEADERS, htons(TYPE_FRAMEINDEX), htonll(MTU_TX_CURRENT));

                TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD,
                                    FRAME_HEADERS->TX_BUFFER,
                                    FRAME_HEADERS->LENGTH+MTU_TX_CURRENT,
                                    0, 
                                    (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                    sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

                if (TX_RET_VAL <=0)
                {
                    perror("MTU test TX error ");
                    return;
                }


                TEST_PARAMS->F_TX_COUNT += 1;

                WAITING = true;

            }

            // Wait for the ACK from back from RX
            while(WAITING)
            {

                // Get the current time
                clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_CURRENT_TIME);

                // Poll has been disabled in favour of a non-blocking recvfrom (for now)
                RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD,
                                  FRAME_HEADERS->RX_BUFFER,
                                  MTU_TEST->MTU_TX_MAX,
                                  MSG_DONTWAIT, NULL, NULL);

                if (RX_LEN > 0) {


                    if (ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_TEST_SUB_TLV &&
                        ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_ACKINDEX)
                    {

                        TEST_PARAMS->F_RX_COUNT += 1;

                        // Get the MTU size RX is ACK'ing
                        MTU_ACK_CURRENT = (uint32_t)ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE);
                        if (MTU_ACK_CURRENT > MTU_ACK_LARGEST) 
                        {
                            MTU_ACK_LARGEST = MTU_ACK_CURRENT;
                        }


                        if (MTU_ACK_CURRENT < MTU_ACK_PREVIOUS)
                        {
                            // Frame received out of order, later than expected
                            TEST_PARAMS->F_RX_LATE += 1;
                        } else if (MTU_ACK_CURRENT > MTU_ACK_PREVIOUS + 1) {
                            // Frame received out of order, earlier than expected
                            TEST_PARAMS->F_RX_EARLY += 1;
                        } else if (MTU_ACK_CURRENT == MTU_ACK_PREVIOUS + 1) {
                            // Frame received in order
                            MTU_ACK_PREVIOUS = MTU_ACK_CURRENT;
                        }


                    // A non-test frame was received
                    } else {

                        TEST_PARAMS->F_RX_OTHER += 1;

                    }


                } // RX_LEN > 0

                // 1 seconds have passed TX has probably missed/lost the ACK
                if ((TEST_PARAMS->TS_CURRENT_TIME.tv_sec - 
                     TEST_PARAMS->TS_ELAPSED_TIME.tv_sec) >= 1) WAITING = false;

            } // End of WAITING

            
        } // End of TX transmit


        printf("Largest MTU ACK'ed from RX is %u\n\n", MTU_ACK_LARGEST);

        printf("Test frames transmitted: %" PRIu64 "\n"
               "Test frames received: %" PRIu64 "\n"
               "Non test frames received: %" PRIu64 "\n"
               "In order ACK frames received: %" PRIu64 "\n"
               "Out of order ACK frames received early: %" PRIu64 "\n"
               "Out of order ACK frames received late: %" PRIu64 "\n",
               TEST_PARAMS->F_TX_COUNT,
               TEST_PARAMS->F_RX_COUNT,
               TEST_PARAMS->F_RX_OTHER,
               TEST_PARAMS->F_RX_ONTIME,
               TEST_PARAMS->F_RX_EARLY,
               TEST_PARAMS->F_RX_LATE);

        return;


    // Running in RX mode
    } else {

        uint32_t MTU_RX_PREVIOUS = 0;
        uint32_t MTU_RX_CURRENT  = 0;
        uint32_t MTU_RX_LARGEST  = 0;
        uint8_t WAITING          = true;

        // If 5 seconds pass without receiving a frame, end the MTU sweep test
        // (assume MTU has been exceeded)
        uint8_t MTU_RX_ANY = false;
        
        clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_ELAPSED_TIME);

        while (WAITING)
        {

            // Get the current time
            clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_CURRENT_TIME);

            // Check for test frame from TX
            RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD,
                              FRAME_HEADERS->RX_BUFFER,
                              MTU_TEST->MTU_TX_MAX,
                              MSG_DONTWAIT, NULL, NULL);

            if (RX_LEN > 0) {

                if (ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_TEST_SUB_TLV &&
                    ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_FRAMEINDEX)
                {

                    MTU_RX_ANY = true;

                    TEST_PARAMS->F_RX_COUNT += 1;

                    // Get the MTU size TX is sending
                    MTU_RX_CURRENT = (uint32_t)ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE);

                    if (MTU_RX_CURRENT > MTU_RX_LARGEST)
                    {

                        MTU_RX_LARGEST = MTU_RX_CURRENT;

                        // ACK that back to TX as new largest MTU received
                        // (send the ACK 3 times)
                        for(uint8_t i = 1; i <= 3; i += 1)
                        {

                        build_sub_tlv(FRAME_HEADERS, htons(TYPE_ACKINDEX), htonll(MTU_RX_CURRENT));

                        TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD,
                                            FRAME_HEADERS->TX_BUFFER,
                                            FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE, 0, 
                                            (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                            sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

                        if (TX_RET_VAL <=0)
                        {
                            perror("MTU test TX error ");
                            return;
                        }


                        TEST_PARAMS->F_TX_COUNT += 1;

                        }


                    } // MTU_RX_CURRENT > MTU_RX_LARGEST


                    if (MTU_RX_CURRENT < MTU_RX_PREVIOUS)
                    {
                        // Frame received out of order, later than expected
                        TEST_PARAMS->F_RX_LATE += 1;
                    } else if (MTU_RX_CURRENT > MTU_RX_PREVIOUS+1) {
                        // Frame received out of order, earlier than expected
                        TEST_PARAMS->F_RX_EARLY += 1;
                    } else if (MTU_RX_CURRENT == MTU_RX_PREVIOUS+1) {
                        // Frame received in order
                        MTU_RX_PREVIOUS = MTU_RX_CURRENT;
                    }


                // A non-test frame was recieved
                } else {

                    TEST_PARAMS->F_RX_OTHER += 1;

                } //End of TEST_FRAME


            } // RX_LEN > 0


            // If RX has received the largest MTU TX hoped to send,
            // the MTU sweep test is over
            if (MTU_RX_LARGEST == MTU_TEST->MTU_TX_MAX)
            {

                // Signal back to TX the largest MTU RX recieved at the end
                WAITING = false;

                printf("MTU sweep test complete\n"
                      "Largest MTU received was %u\n\n", MTU_RX_LARGEST);

                printf("Test frames transmitted: %" PRIu64 "\n"
                       "Test frames received: %" PRIu64 "\n"
                       "Non test frames received: %" PRIu64 "\n"
                       "In order test frames received: %" PRIu64 "\n"
                       "Out of order test frames received early: %" PRIu64 "\n"
                       "Out of order test frames received late: %" PRIu64 "\n",
                       TEST_PARAMS->F_TX_COUNT,
                       TEST_PARAMS->F_RX_COUNT,
                       TEST_PARAMS->F_RX_OTHER,
                       TEST_PARAMS->F_RX_ONTIME,
                       TEST_PARAMS->F_RX_EARLY,
                       TEST_PARAMS->F_RX_LATE);

                return;

            }

            // 5 seconds have passed without any MTU sweep frames being receeved
            // Assume the max MTU has been exceeded
            if ((TEST_PARAMS->TS_CURRENT_TIME.tv_sec-TEST_PARAMS->TS_ELAPSED_TIME.tv_sec) >= 5)
            {

                if (MTU_RX_ANY == false)
                {
                    printf("Timeout waiting for MTU sweep frames from TX, "
                           "ending the test.\nLargest MTU received %u\n\n",
                           MTU_RX_LARGEST);

                    WAITING = false;

                } else {

                    clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_ELAPSED_TIME);
                    MTU_RX_ANY = false;

                }

            }


        } // End of WAITING


    } // End of TX/RX mode


    return;
}



void latency_test(struct APP_PARAMS *APP_PARAMS,
                  struct FRAME_HEADERS *FRAME_HEADERS,
                  struct TEST_INTERFACE *TEST_INTERFACE,
                  struct TEST_PARAMS *TEST_PARAMS,
                  struct QM_TEST *QM_TEST)
{

    build_tlv(FRAME_HEADERS, htons(TYPE_TESTFRAME), htonl(VALUE_TEST_SUB_TLV));

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


    if (APP_PARAMS->TX_MODE == true)
    {

        if (TEST_PARAMS->F_COUNT > 0) {   
            // Testing until N RTT measurements
            testMax = &TEST_PARAMS->F_COUNT;
            testBase = &TEST_PARAMS->F_TX_COUNT;

        } else {
            // Testing until N seconds have passed
            if (TEST_PARAMS->F_DURATION > 0) TEST_PARAMS->F_DURATION -= 1;
            testMax = (uint64_t*)&TEST_PARAMS->F_DURATION;
            testBase = (uint64_t*)&TEST_PARAMS->S_ELAPSED;
        }


        clock_gettime(CLOCK_MONOTONIC_RAW, &QM_TEST->TS_START);

        printf("No.\tRTT\t\tJitter\n");

        while (*testBase<=*testMax)
        {
            
            clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_CURRENT_TIME);

            UPTIME_1 = TEST_PARAMS->TS_CURRENT_TIME.tv_sec + ((double)TEST_PARAMS->TS_CURRENT_TIME.tv_nsec * 1e-9);
            TX_UPTIME = roundl(UPTIME_1 * 1000000000.0);

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_PING), htonll(TX_UPTIME));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD,
                                FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0, 
                                (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            if (TX_RET_VAL <=0)
            {
                perror("Latency test TX error ");
                return;
            }


            TEST_PARAMS->F_TX_COUNT += 1;

            WAITING = true;
            ECHO_WAITING = true;

            while (WAITING)
            {

                // Get the current time
                clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_ELAPSED_TIME);

                // Poll has been disabled in favour of a non-blocking recvfrom (for now)
                RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD,
                                  FRAME_HEADERS->RX_BUFFER,
                                  TEST_PARAMS->F_SIZE_TOTAL,
                                  MSG_DONTWAIT, NULL, NULL);

                if (RX_LEN > 0) {

                    // Received an echo reply/pong
                    if (ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_TEST_SUB_TLV &&
                        ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_PONG)
                    {

                        UPTIME_2 = TEST_PARAMS->TS_ELAPSED_TIME.tv_sec + 
                                   ((double)TEST_PARAMS->TS_ELAPSED_TIME.tv_nsec * 1e-9);

                        // Check it's the time value originally sent
                        UPTIME_RX = (double)ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE) / 1000000000.0;

                        if (UPTIME_RX == UPTIME_1)
                        {

                            RTT = UPTIME_2-UPTIME_1;

                            if (RTT < QM_TEST->RTT_MIN)
                            {
                                QM_TEST->RTT_MIN = RTT;
                            } else if (RTT > QM_TEST->RTT_MAX) {
                                QM_TEST->RTT_MAX = RTT;
                            }

                            JITTER = fabsl(RTT-RTT_PREV);

                            if (JITTER < QM_TEST->JITTER_MIN)
                            {
                                QM_TEST->JITTER_MIN = JITTER;
                            } else if (JITTER > QM_TEST->JITTER_MAX) {
                                QM_TEST->JITTER_MAX = JITTER;
                            }

                            QM_TEST->TEST_COUNT += 1;

                            printf ("%" PRIu32 ":\t%.9Lfs\t%.9Lfs\n", QM_TEST->TEST_COUNT, RTT, JITTER);

                            RTT_PREV = RTT;

                            ECHO_WAITING = false;

                            TEST_PARAMS->F_RX_COUNT += 1;

                        } else {
                            TEST_PARAMS->F_RX_OTHER += 1;
                        }


                    } else {

                        TEST_PARAMS->F_RX_OTHER += 1;

                    }


                    // Check if RX host has quit/died;
                    if (ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_DYINGGASP)
                    {

                        printf("RX host has quit\n");
                        return;

                    }


                } // RX_LEN > 0


                // If TX is waiting for echo reply, check if the echo reply has timed out
                if (ECHO_WAITING) {
                    if ( (TEST_PARAMS->TS_ELAPSED_TIME.tv_sec-
                          TEST_PARAMS->TS_CURRENT_TIME.tv_sec >= QM_TEST->TIMEOUT_SEC)
                         &&
                         (TEST_PARAMS->TS_ELAPSED_TIME.tv_nsec-
                          TEST_PARAMS->TS_CURRENT_TIME.tv_nsec >= QM_TEST->TIMEOUT_NSEC) )
                    {

                        ECHO_WAITING = false;
                        printf("*\n");
                        QM_TEST->TIMEOUT_COUNT += 1;
                        QM_TEST->TEST_COUNT += 1;

                    }
                }


                // Check if the echo interval has passed (time to send another ping)
                if ( (TEST_PARAMS->TS_ELAPSED_TIME.tv_sec-
                     TEST_PARAMS->TS_CURRENT_TIME.tv_sec >= QM_TEST->INTERVAL_SEC)
                    &&
                    (TEST_PARAMS->TS_ELAPSED_TIME.tv_nsec-
                     TEST_PARAMS->TS_CURRENT_TIME.tv_nsec >= QM_TEST->INTERVAL_NSEC) )
                {

                    TEST_PARAMS->TS_CURRENT_TIME.tv_sec = TEST_PARAMS->TS_ELAPSED_TIME.tv_sec;
                    TEST_PARAMS->TS_CURRENT_TIME.tv_nsec = TEST_PARAMS->TS_ELAPSED_TIME.tv_nsec;

                    ECHO_WAITING = false;
                    WAITING = false;

                }


                // Check if 1 second has passed to increment test duration
                if (TEST_PARAMS->TS_ELAPSED_TIME.tv_sec-
                    QM_TEST->TS_START.tv_sec >= 1) {

                    clock_gettime(CLOCK_MONOTONIC_RAW, &QM_TEST->TS_START);

                    TEST_PARAMS->S_ELAPSED += 1;

                }


            } // WAITING=1


        } // testBase<=testMax

        printf("Test frames transmitted: %" PRIu64 "\n"
               "Test frames received: %" PRIu64 "\n"
               "Non test or out-of-order frames received: %" PRIu64 "\n"
               "Number of timeouts: %" PRIu32 "\n"
               "Min/Max RTT during test: %.9Lfs/%.9Lfs\n"
               "Min/Max jitter during test: %.9Lfs/%.9Lfs\n",
               TEST_PARAMS->F_TX_COUNT,
               TEST_PARAMS->F_RX_COUNT,
               TEST_PARAMS->F_RX_OTHER,
               QM_TEST->TIMEOUT_COUNT += 1,
               QM_TEST->RTT_MIN,
               QM_TEST->RTT_MAX,
               QM_TEST->JITTER_MIN,
               QM_TEST->JITTER_MAX);


    // Else, RX mode
    } else {


        if (TEST_PARAMS->F_COUNT > 0) {   
            // Testing until N RTT measurements
            testMax = &TEST_PARAMS->F_COUNT;
            testBase = &TEST_PARAMS->F_RX_COUNT;

        } else {
            // Testing until N seconds have passed
            if (TEST_PARAMS->F_DURATION > 0) TEST_PARAMS->F_DURATION -= 1;
            testMax = (uint64_t*)&TEST_PARAMS->F_DURATION;
            testBase = (uint64_t*)&TEST_PARAMS->S_ELAPSED;
        }

        // Wait for the first test frame to be received before starting the test loop
        uint8_t FIRST_FRAME = false;
        while (!FIRST_FRAME)
        {

            RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->RX_BUFFER,
                              TEST_PARAMS->F_SIZE_TOTAL, MSG_PEEK, NULL, NULL);

            // Check if this is an Etherate test frame
            if (ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_TEST_SUB_TLV &&
                ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_PING)
            {
                FIRST_FRAME = true;
            }

        }


        clock_gettime(CLOCK_MONOTONIC_RAW, &QM_TEST->TS_START);

        printf("No.\tEcho Interval\n");


        // RX test loop
        while (*testBase<=*testMax)
        {


            clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_CURRENT_TIME);

            WAITING      = true;
            ECHO_WAITING = true;

            while (WAITING)
            {

                // Get the current time
                clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_ELAPSED_TIME);

                // Poll has been disabled in favour of a non-blocking recvfrom (for now)
                RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD,
                                  FRAME_HEADERS->RX_BUFFER,
                                  TEST_PARAMS->F_SIZE_TOTAL,
                                  MSG_DONTWAIT, NULL, NULL);

                if (RX_LEN > 0) {

                    if ( ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_TEST_SUB_TLV &&
                         ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_PING )
                    {

                        // Time RX received this value
                        UPTIME_2 = TEST_PARAMS->TS_ELAPSED_TIME.tv_sec + 
                                   ((double)TEST_PARAMS->TS_ELAPSED_TIME.tv_nsec * 1e-9);

                        // Send the TX uptime value back to the TX host
                        build_sub_tlv(FRAME_HEADERS, htons(TYPE_PONG),
                                      *FRAME_HEADERS->RX_SUB_TLV_VALUE);

                        TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD,
                                            FRAME_HEADERS->TX_BUFFER,
                                            FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                            0, 
                                            (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                            sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

                        if (TX_RET_VAL <=0)
                        {
                            perror("Latency test RX error ");
                            return;
                        }


                        TEST_PARAMS->F_TX_COUNT += 1;

                        INTERVAL = fabsl(UPTIME_2-UPTIME_1);

                        QM_TEST->TEST_COUNT += 1;

                        // Interval between receiving this uptime value and the last
                        if (UPTIME_1!=0.0)
                        {
                            printf("%" PRIu32 ":\t%.9Lfs\n", QM_TEST->TEST_COUNT, INTERVAL);
                        } else {
                            printf("%" PRIu32 ":\t0.0s\n", QM_TEST->TEST_COUNT);
                        }
                        UPTIME_1 = UPTIME_2;

                        if (INTERVAL < QM_TEST->INTERVAL_MIN)
                        {
                            QM_TEST->INTERVAL_MIN = INTERVAL;
                        } else if (INTERVAL > QM_TEST->INTERVAL_MAX) {
                            QM_TEST->INTERVAL_MAX = INTERVAL;
                        }

                        ECHO_WAITING = false;

                        TEST_PARAMS->F_RX_COUNT += 1;

                    } else {

                        TEST_PARAMS->F_RX_OTHER += 1;

                    }

                } // RX_LEN > 0


                // Check if the echo request has timed out
                if(ECHO_WAITING == true) {

                    if ( (TEST_PARAMS->TS_ELAPSED_TIME.tv_sec-
                          TEST_PARAMS->TS_CURRENT_TIME.tv_sec >= QM_TEST->INTERVAL_SEC)
                         &&
                         (TEST_PARAMS->TS_ELAPSED_TIME.tv_nsec-
                          TEST_PARAMS->TS_CURRENT_TIME.tv_nsec >= QM_TEST->INTERVAL_NSEC) )
                    {

                        printf("*\n");
                        QM_TEST->TIMEOUT_COUNT += 1;
                        QM_TEST->TEST_COUNT += 1;
                        ECHO_WAITING = false;

                    }
                    
                }


                // Check if the echo interval has passed (time to receive another ping)
                if ( (TEST_PARAMS->TS_ELAPSED_TIME.tv_sec-
                     TEST_PARAMS->TS_CURRENT_TIME.tv_sec >= QM_TEST->INTERVAL_SEC)
                    &&
                    (TEST_PARAMS->TS_ELAPSED_TIME.tv_nsec-
                     TEST_PARAMS->TS_CURRENT_TIME.tv_nsec >= QM_TEST->INTERVAL_NSEC) )
                {

                    TEST_PARAMS->TS_CURRENT_TIME.tv_sec = TEST_PARAMS->TS_ELAPSED_TIME.tv_sec;
                    TEST_PARAMS->TS_CURRENT_TIME.tv_nsec = TEST_PARAMS->TS_ELAPSED_TIME.tv_nsec;

                    ECHO_WAITING = false;
                    WAITING      = false;

                }


                // Check if 1 second has passed to increment test duration
                if (TEST_PARAMS->TS_ELAPSED_TIME.tv_sec-
                    QM_TEST->TS_START.tv_sec >= 1) {

                    clock_gettime(CLOCK_MONOTONIC_RAW, &QM_TEST->TS_START);

                    TEST_PARAMS->S_ELAPSED += 1;

                }


                // Check if TX host has quit/died;
                if (ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_DYINGGASP)
                {

                    printf("TX host has quit\n");
                    return;

                }


            } // WAITING


        } // testBase<=testMax

        printf("Test frames transmitted: %" PRIu64 "\n"
               "Test frames received: %" PRIu64 "\n"
               "Non test frames received: %" PRIu64 "\n"
               "Number of timeouts: %u\n"
               "Min/Max interval during test %.9Lfs/%.9Lfs\n",
               TEST_PARAMS->F_TX_COUNT,
               TEST_PARAMS->F_RX_COUNT,
               TEST_PARAMS->F_RX_OTHER,
               QM_TEST->TIMEOUT_COUNT,
               QM_TEST->INTERVAL_MIN,
               QM_TEST->INTERVAL_MAX);


    } // End of TX/RX


     return;

 }



void speed_test(struct APP_PARAMS *APP_PARAMS,
                struct FRAME_HEADERS *FRAME_HEADERS,
                struct TEST_INTERFACE *TEST_INTERFACE,
                struct TEST_PARAMS *TEST_PARAMS)
{

    int16_t TX_RET_VAL = 0;
    int16_t RX_LEN     = 0;

    build_tlv(FRAME_HEADERS, htons(TYPE_TESTFRAME), htonl(VALUE_TEST_SUB_TLV));


    if (APP_PARAMS->TX_MODE == true)
    {

        printf("Seconds\t\tMbps Tx\t\tMBs Tx\t\tFrmTX/s\t\tFrames Tx\n");

        // By default test until F_DURATION_DEF has passed
        uint64_t *testMax = (uint64_t*)&TEST_PARAMS->F_DURATION;
        uint64_t *testBase = (uint64_t*)&TEST_PARAMS->S_ELAPSED;

        if (TEST_PARAMS->F_BYTES > 0)
        {    
            // Testing until N bytes sent
            testMax = &TEST_PARAMS->F_BYTES;
            testBase = &TEST_PARAMS->B_TX;

        } else if (TEST_PARAMS->F_COUNT > 0) {   
            // Testing until N frames sent
            testMax = &TEST_PARAMS->F_COUNT;
            testBase = &TEST_PARAMS->F_TX_COUNT;

        } else if (TEST_PARAMS->F_DURATION > 0) {
            // Testing until N seconds have passed
            TEST_PARAMS->F_DURATION -= 1;
        }


        // Get clock times for the speed limit restriction and starting time
        clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_ELAPSED_TIME);

        // TX test loop
        while (*testBase<=*testMax)
        {

            clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_CURRENT_TIME);

            // One second has passed
            if ((TEST_PARAMS->TS_CURRENT_TIME.tv_sec - TEST_PARAMS->TS_ELAPSED_TIME.tv_sec) >= 1)
            {
                TEST_PARAMS->S_ELAPSED += 1;
                TEST_PARAMS->B_SPEED = (((double)TEST_PARAMS->B_TX-TEST_PARAMS->B_TX_PREV) * 8) / 1000 / 1000;
                TEST_PARAMS->B_TX_PREV = TEST_PARAMS->B_TX;
                TEST_PARAMS->F_SPEED = (TEST_PARAMS->F_TX_COUNT - TEST_PARAMS->F_TX_COUNT_PREV);
                TEST_PARAMS->F_TX_COUNT_PREV = TEST_PARAMS->F_TX_COUNT;

                printf("%" PRIu64 "\t\t%.2f\t\t%" PRIu64 "\t\t%" PRIu64"\t\t%" PRIu64 "\n",
                       TEST_PARAMS->S_ELAPSED,
                       TEST_PARAMS->B_SPEED,
                       (TEST_PARAMS->B_TX/1024)/1024,
                       (TEST_PARAMS->F_SPEED),
                       TEST_PARAMS->F_TX_COUNT);

                if (TEST_PARAMS->B_SPEED > TEST_PARAMS->B_SPEED_MAX)
                    TEST_PARAMS->B_SPEED_MAX = TEST_PARAMS->B_SPEED;

                if (TEST_PARAMS->F_SPEED > TEST_PARAMS->F_SPEED_MAX)
                    TEST_PARAMS->F_SPEED_MAX = TEST_PARAMS->F_SPEED;

                TEST_PARAMS->B_SPEED_AVG += TEST_PARAMS->B_SPEED;
                TEST_PARAMS->F_SPEED_AVG += TEST_PARAMS->F_SPEED;
                TEST_PARAMS->TS_ELAPSED_TIME.tv_sec = TEST_PARAMS->TS_CURRENT_TIME.tv_sec;
                TEST_PARAMS->TS_ELAPSED_TIME.tv_nsec = TEST_PARAMS->TS_CURRENT_TIME.tv_nsec;

                TEST_PARAMS->B_TX_SPEED_PREV = 0;

            } else {

                // Poll has been disabled in favour of a non-blocking recvfrom (for now)
                RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->RX_BUFFER,
                                  TEST_PARAMS->F_SIZE_TOTAL, MSG_DONTWAIT, NULL, NULL);

                if (RX_LEN > 0) {

                    // Running in ACK mode
                    if (TEST_PARAMS->F_ACK)
                    {

                        if (ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_TEST_SUB_TLV &&
                            ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_ACKINDEX)
                        {

                            TEST_PARAMS->F_RX_COUNT += 1;
                            TEST_PARAMS->F_WAITING_ACK = false;

                        if (ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE) == (TEST_PARAMS->F_INDEX_PREV+1)) {
                            TEST_PARAMS->F_RX_ONTIME += 1;
                            TEST_PARAMS->F_INDEX_PREV += 1;
                        } else if (ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE) > (TEST_PARAMS->F_RX_COUNT)) {
                            TEST_PARAMS->F_INDEX_PREV = ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE);
                            TEST_PARAMS->F_RX_EARLY += 1;
                        } else if (ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE) <= TEST_PARAMS->F_RX_COUNT) {
                            TEST_PARAMS->F_RX_LATE += 1;
                        }

                        } else if (ntohs(*FRAME_HEADERS->RX_TLV_TYPE) == TYPE_APPLICATION &&
                                   ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_DYINGGASP) {

                            printf("RX host has quit\n");

                            return;

                        // Received a non-test frame
                        } else {
                            TEST_PARAMS->F_RX_OTHER += 1;
                        }

                    // Not running in ACK mode
                    } else if (ntohs(*FRAME_HEADERS->RX_TLV_TYPE) == TYPE_APPLICATION &&
                               ntohll(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_DYINGGASP) {

                        printf("RX host has quit\n");

                        return;

                    // Received a non-test frame
                    } else {
                        TEST_PARAMS->F_RX_OTHER += 1;
                    }

                } // RX_LEN > 0

                // If it hasn't been 1 second yet, send more test frames
                if (TEST_PARAMS->F_WAITING_ACK == false)
                {

                    // Check if a max speed has been configured
                    if (TEST_PARAMS->B_TX_SPEED_MAX != B_TX_SPEED_MAX_DEF)
                    {

                        // Check if sending another frame exceeds the max speed configured
                        if ((TEST_PARAMS->B_TX_SPEED_PREV + TEST_PARAMS->F_SIZE) <= TEST_PARAMS->B_TX_SPEED_MAX)
                        {


                            build_sub_tlv(FRAME_HEADERS, htons(TYPE_FRAMEINDEX), 
                                          htonll(TEST_PARAMS->F_TX_COUNT+1));

                            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD,
                                                FRAME_HEADERS->TX_BUFFER,
                                                TEST_PARAMS->F_SIZE_TOTAL, 0, 
                                                (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

                            if (TX_RET_VAL <=0)
                            {
                                perror("Speed test TX error ");
                                return;
                            }


                            TEST_PARAMS->F_TX_COUNT += 1;
                            TEST_PARAMS->B_TX += TEST_PARAMS->F_SIZE;
                            TEST_PARAMS->B_TX_SPEED_PREV += TEST_PARAMS->F_SIZE;
                            if (TEST_PARAMS->F_ACK) TEST_PARAMS->F_WAITING_ACK = true;

                        }

                    // Check if a max frames per second limit is configured
                    } else if(TEST_PARAMS->F_TX_SPEED_MAX != F_TX_SPEED_MAX_DEF) {

                        // Check if sending another frame exceeds the max frame rate configured
                        if (TEST_PARAMS->F_TX_COUNT - TEST_PARAMS->F_TX_COUNT_PREV < TEST_PARAMS->F_TX_SPEED_MAX)
                        {

                            build_sub_tlv(FRAME_HEADERS, htons(TYPE_FRAMEINDEX), 
                                          htonll(TEST_PARAMS->F_TX_COUNT+1));

                            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD,
                                                FRAME_HEADERS->TX_BUFFER,
                                                TEST_PARAMS->F_SIZE_TOTAL, 0, 
                                                (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

                            if (TX_RET_VAL <=0)
                            {
                                perror("Speed test TX error ");
                                return;
                            }


                            TEST_PARAMS->F_TX_COUNT += 1;
                            TEST_PARAMS->B_TX += TEST_PARAMS->F_SIZE;
                            TEST_PARAMS->B_TX_SPEED_PREV += TEST_PARAMS->F_SIZE;
                            if (TEST_PARAMS->F_ACK) TEST_PARAMS->F_WAITING_ACK = true;

                        }

                    // Else there are no speed restrictions
                    } else {

                        build_sub_tlv(FRAME_HEADERS, htons(TYPE_FRAMEINDEX),
                                      htonll(TEST_PARAMS->F_TX_COUNT+1));

                        TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD,
                                            FRAME_HEADERS->TX_BUFFER,
                                            TEST_PARAMS->F_SIZE_TOTAL, 0, 
                                            (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                            sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

                        if (TX_RET_VAL <=0)
                        {
                            perror("Speed test TX error ");
                            return;
                        }


                        TEST_PARAMS->F_TX_COUNT += 1;
                        TEST_PARAMS->B_TX += TEST_PARAMS->F_SIZE;
                        TEST_PARAMS->B_TX_SPEED_PREV += TEST_PARAMS->F_SIZE;
                        if (TEST_PARAMS->F_ACK) TEST_PARAMS->F_WAITING_ACK = true;

                    }
                }


            } // End of TS_CURRENT_TIME.tv_sec - S_ELAPSED_TIME.tv_sec

        }


        if (TEST_PARAMS->S_ELAPSED > 0)
        {
            TEST_PARAMS->B_SPEED_AVG = (TEST_PARAMS->B_SPEED_AVG/TEST_PARAMS->S_ELAPSED);
            TEST_PARAMS->F_SPEED_AVG = (TEST_PARAMS->F_SPEED_AVG/TEST_PARAMS->S_ELAPSED);
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
               TEST_PARAMS->F_TX_COUNT,
               TEST_PARAMS->F_RX_COUNT,
               TEST_PARAMS->F_RX_OTHER,
               TEST_PARAMS->F_RX_ONTIME,
               TEST_PARAMS->F_RX_EARLY,
               TEST_PARAMS->F_RX_LATE,
               TEST_PARAMS->B_SPEED_MAX,
               TEST_PARAMS->F_SPEED_MAX,
               TEST_PARAMS->B_SPEED_AVG,
               TEST_PARAMS->F_SPEED_AVG,
               (TEST_PARAMS->B_TX/1024)/1024);


    // Else, RX mode
    } else {

        printf("Seconds\t\tMbps Rx\t\tMBs Rx\t\tFrmRX/s\t\tFrames Rx\n");

        // By default test until F_DURATION_DEF has passed
        uint64_t *testMax = (uint64_t*)&TEST_PARAMS->F_DURATION;
        uint64_t *testBase = (uint64_t*)&TEST_PARAMS->S_ELAPSED;

        // Testing until N bytes received
        if (TEST_PARAMS->F_BYTES > 0)
        {
            testMax = &TEST_PARAMS->F_BYTES;
            testBase = &TEST_PARAMS->B_RX;

        // Testing until N frames received
        } else if (TEST_PARAMS->F_COUNT > 0) {
            testMax = &TEST_PARAMS->F_COUNT;
            testBase = &TEST_PARAMS->F_RX_COUNT;

        // Testing until N seconds have passed
        } else if (TEST_PARAMS->F_DURATION > 0) {
            TEST_PARAMS->F_DURATION -= 1;
        }


        // Wait for the first test frame to be received before starting the test loop
        uint8_t FIRST_FRAME = false;
        while (!FIRST_FRAME)
        {

            RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->RX_BUFFER,
                              TEST_PARAMS->F_SIZE_TOTAL, MSG_PEEK, NULL, NULL);

            // Check if this is an Etherate test frame
            if (ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_TEST_SUB_TLV &&
                ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_FRAMEINDEX)
            {
                FIRST_FRAME = true;
            }

        }

        clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_ELAPSED_TIME);

        // RX test loop
        while (*testBase<=*testMax)
        {

            clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_CURRENT_TIME);

            // If one second has passed
            if ((TEST_PARAMS->TS_CURRENT_TIME.tv_sec-TEST_PARAMS->TS_ELAPSED_TIME.tv_sec) >= 1)
            {
                TEST_PARAMS->S_ELAPSED += 1;
                TEST_PARAMS->B_SPEED = (double)(TEST_PARAMS->B_RX-TEST_PARAMS->B_RX_PREV) * 8 / 1000000;
                TEST_PARAMS->B_RX_PREV = TEST_PARAMS->B_RX;
                TEST_PARAMS->F_SPEED = (TEST_PARAMS->F_RX_COUNT - TEST_PARAMS->F_RX_COUNT_PREV);
                TEST_PARAMS->F_RX_COUNT_PREV = TEST_PARAMS->F_RX_COUNT;

                printf("%" PRIu64 "\t\t%.2f\t\t%" PRIu64 "\t\t%" PRIu64 "\t\t%" PRIu64 "\n",
                       TEST_PARAMS->S_ELAPSED,
                       TEST_PARAMS->B_SPEED,
                       (TEST_PARAMS->B_RX/1024)/1024,
                       (TEST_PARAMS->F_SPEED),
                       TEST_PARAMS->F_RX_COUNT);

                if (TEST_PARAMS->B_SPEED > TEST_PARAMS->B_SPEED_MAX)
                    TEST_PARAMS->B_SPEED_MAX = TEST_PARAMS->B_SPEED;

                if (TEST_PARAMS->F_SPEED > TEST_PARAMS->F_SPEED_MAX)
                    TEST_PARAMS->F_SPEED_MAX = TEST_PARAMS->F_SPEED;

                TEST_PARAMS->B_SPEED_AVG += TEST_PARAMS->B_SPEED;
                TEST_PARAMS->F_SPEED_AVG += TEST_PARAMS->F_SPEED;
                TEST_PARAMS->TS_ELAPSED_TIME.tv_sec = TEST_PARAMS->TS_CURRENT_TIME.tv_sec;
                TEST_PARAMS->TS_ELAPSED_TIME.tv_nsec = TEST_PARAMS->TS_CURRENT_TIME.tv_nsec;

            }

            // Poll has been disabled in favour of a non-blocking recvfrom (for now)
            RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD,
                              FRAME_HEADERS->RX_BUFFER,
                              TEST_PARAMS->F_SIZE_TOTAL,
                              MSG_DONTWAIT, NULL, NULL);

            if (RX_LEN > 0)
            {

                // Check if this is an Etherate test frame
                if (likely(ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_TEST_SUB_TLV &&
                    ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_FRAMEINDEX))
                {

                    // Update test stats
                    TEST_PARAMS->F_RX_COUNT += 1;
                    TEST_PARAMS->B_RX+=(RX_LEN-FRAME_HEADERS->LENGTH);

                    // Record if the frame is in-order, early or late
                    if (likely(ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE) == (TEST_PARAMS->F_INDEX_PREV+1))) {
                        TEST_PARAMS->F_RX_ONTIME += 1;
                        TEST_PARAMS->F_INDEX_PREV += 1;
                    } else if (ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE) > (TEST_PARAMS->F_RX_COUNT)) {
                        TEST_PARAMS->F_INDEX_PREV = ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE);
                        TEST_PARAMS->F_RX_EARLY += 1;
                    } else if (ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE) <= TEST_PARAMS->F_RX_COUNT) {
                        TEST_PARAMS->F_RX_LATE += 1;
                    }

                    // If running in ACK mode RX needs to ACK to TX host
                    if (TEST_PARAMS->F_ACK)
                    {

                        build_sub_tlv(FRAME_HEADERS, htons(TYPE_ACKINDEX),
                                      *FRAME_HEADERS->RX_SUB_TLV_VALUE);

                        TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD,
                                            FRAME_HEADERS->TX_BUFFER,
                                            FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE, 0, 
                                            (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                            sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

                        if (TX_RET_VAL <=0)
                        {
                            perror("Speed test TX error ");
                            return;
                        }


                        TEST_PARAMS->F_TX_COUNT += 1;
                        
                    }

                } else {
                    // Received a non-test frame
                    TEST_PARAMS->F_RX_OTHER += 1;
                }

                // Check if TX host has quit/died;
                if (unlikely(ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_DYINGGASP))
                {

                    printf("TX host has quit\n");
                    return;

                }
                      

            } // RX_LEN > 0


        } // *testBase<=*testMax


        if (TEST_PARAMS->S_ELAPSED > 0)
        {
            TEST_PARAMS->B_SPEED_AVG = (TEST_PARAMS->B_SPEED_AVG/TEST_PARAMS->S_ELAPSED);
            TEST_PARAMS->F_SPEED_AVG = (TEST_PARAMS->F_SPEED_AVG/TEST_PARAMS->S_ELAPSED);
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
               TEST_PARAMS->F_TX_COUNT,
               TEST_PARAMS->F_RX_COUNT,
               TEST_PARAMS->F_RX_OTHER,
               TEST_PARAMS->F_RX_ONTIME,
               TEST_PARAMS->F_RX_EARLY,
               TEST_PARAMS->F_RX_LATE,
               TEST_PARAMS->B_SPEED_MAX,
               TEST_PARAMS->F_SPEED_MAX,
               TEST_PARAMS->B_SPEED_AVG,
               TEST_PARAMS->F_SPEED_AVG,
               (TEST_PARAMS->B_RX/1024)/1024);


    }

    return;
 }



void send_custom_frame(struct APP_PARAMS *APP_PARAMS,
                       struct FRAME_HEADERS *FRAME_HEADERS,
                       struct TEST_INTERFACE *TEST_INTERFACE,
                       struct TEST_PARAMS *TEST_PARAMS)
{

    int16_t TX_RET_VAL = 0;

    printf("Seconds\t\tMbps Tx\t\tMBs Tx\t\tFrmTX/s\t\tFrames Tx\n");

    // By default test until F_DURATION_DEF has passed
    uint64_t *testMax = (uint64_t*)&TEST_PARAMS->F_DURATION;
    uint64_t *testBase = (uint64_t*)&TEST_PARAMS->S_ELAPSED;

    if (TEST_PARAMS->F_BYTES > 0)
    {    
        // Testing until N bytes sent
        testMax = &TEST_PARAMS->F_BYTES;
        testBase = &TEST_PARAMS->B_TX;

    } else if (TEST_PARAMS->F_COUNT > 0) {   
        // Testing until N frames sent
        testMax = &TEST_PARAMS->F_COUNT;
        testBase = &TEST_PARAMS->F_TX_COUNT;

    } else if (TEST_PARAMS->F_DURATION > 0) {
        // Testing until N seconds have passed
        TEST_PARAMS->F_DURATION -= 1;
    }


    // Get clock times for the speed limit restriction and starting time
    clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_ELAPSED_TIME);

    // TX test loop
    while (*testBase<=*testMax)
    {

        clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_CURRENT_TIME);

        // One second has passed
        if ((TEST_PARAMS->TS_CURRENT_TIME.tv_sec - TEST_PARAMS->TS_ELAPSED_TIME.tv_sec) >= 1)
        {
            TEST_PARAMS->S_ELAPSED += 1;
            TEST_PARAMS->B_SPEED = ((double)(TEST_PARAMS->B_TX - TEST_PARAMS->B_TX_PREV) * 8) / 1000 / 1000;
            TEST_PARAMS->B_TX_PREV = TEST_PARAMS->B_TX;
            TEST_PARAMS->F_SPEED = (TEST_PARAMS->F_TX_COUNT - TEST_PARAMS->F_TX_COUNT_PREV);
            TEST_PARAMS->F_TX_COUNT_PREV = TEST_PARAMS->F_TX_COUNT;

            printf("%" PRIu64 "\t\t%.2f\t\t%" PRIu64 "\t\t%" PRIu64"\t\t%" PRIu64 "\n",
                   TEST_PARAMS->S_ELAPSED,
                   TEST_PARAMS->B_SPEED,
                   (TEST_PARAMS->B_TX/1024)/1024,
                   (TEST_PARAMS->F_SPEED),
                   TEST_PARAMS->F_TX_COUNT);

            if (TEST_PARAMS->B_SPEED > TEST_PARAMS->B_SPEED_MAX)
                TEST_PARAMS->B_SPEED_MAX = TEST_PARAMS->B_SPEED;

            if (TEST_PARAMS->F_SPEED > TEST_PARAMS->F_SPEED_MAX)
                TEST_PARAMS->F_SPEED_MAX = TEST_PARAMS->F_SPEED;

            TEST_PARAMS->B_SPEED_AVG += TEST_PARAMS->B_SPEED;
            TEST_PARAMS->F_SPEED_AVG += TEST_PARAMS->F_SPEED;
            TEST_PARAMS->TS_ELAPSED_TIME.tv_sec = TEST_PARAMS->TS_CURRENT_TIME.tv_sec;
            TEST_PARAMS->TS_ELAPSED_TIME.tv_nsec = TEST_PARAMS->TS_CURRENT_TIME.tv_nsec;

            TEST_PARAMS->B_TX_SPEED_PREV = 0;

        } else {


            // Check if a max speed has been configured
            if (TEST_PARAMS->B_TX_SPEED_MAX != B_TX_SPEED_MAX_DEF)
            {

                // Check if sending another frame exceeds the max speed configured
                if ((TEST_PARAMS->B_TX_SPEED_PREV + TEST_PARAMS->F_PAYLOAD_SIZE) <= TEST_PARAMS->B_TX_SPEED_MAX)
                {


                    TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD,
                                        TEST_PARAMS->F_PAYLOAD,
                                        TEST_PARAMS->F_PAYLOAD_SIZE, 0, 
                                        (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                        sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

                    if (TX_RET_VAL <=0)
                    {
                        perror("Frame sending TX error ");
                        return;
                    }


                    TEST_PARAMS->F_TX_COUNT += 1;
                    TEST_PARAMS->B_TX += TEST_PARAMS->F_PAYLOAD_SIZE;
                    TEST_PARAMS->B_TX_SPEED_PREV += TEST_PARAMS->F_PAYLOAD_SIZE;

                }

            // Check if a max frames per second limit is configured
            } else if(TEST_PARAMS->F_TX_SPEED_MAX != F_TX_SPEED_MAX_DEF) {

                // Check if sending another frame exceeds the max frame rate configured
                if (TEST_PARAMS->F_TX_COUNT - TEST_PARAMS->F_TX_COUNT_PREV < TEST_PARAMS->F_TX_SPEED_MAX)
                {

                    TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD,
                                        TEST_PARAMS->F_PAYLOAD,
                                        TEST_PARAMS->F_PAYLOAD_SIZE, 0, 
                                        (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                        sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

                    if (TX_RET_VAL <=0)
                    {
                        perror("Frame sending TX error ");
                        return;
                    }


                    TEST_PARAMS->F_TX_COUNT += 1;
                    TEST_PARAMS->B_TX += TEST_PARAMS->F_PAYLOAD_SIZE;
                    TEST_PARAMS->B_TX_SPEED_PREV += TEST_PARAMS->F_PAYLOAD_SIZE;

                }

            // Else there are no speed restrictions
            } else {

                TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD,
                                    TEST_PARAMS->F_PAYLOAD,
                                    TEST_PARAMS->F_PAYLOAD_SIZE, 0, 
                                    (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                    sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

                if (TX_RET_VAL <=0)
                {
                    perror("Frame sending TX error ");
                    return;
                }


                TEST_PARAMS->F_TX_COUNT += 1;
                TEST_PARAMS->B_TX += TEST_PARAMS->F_PAYLOAD_SIZE;
                TEST_PARAMS->B_TX_SPEED_PREV += TEST_PARAMS->F_PAYLOAD_SIZE;
            }

        } // End of TS_CURRENT_TIME.tv_sec - TS_ELAPSED_TIME.tv_sec

    }


    if (TEST_PARAMS->S_ELAPSED > 0)
    {
        TEST_PARAMS->B_SPEED_AVG = (TEST_PARAMS->B_SPEED_AVG/TEST_PARAMS->S_ELAPSED);
        TEST_PARAMS->F_SPEED_AVG = (TEST_PARAMS->F_SPEED_AVG/TEST_PARAMS->S_ELAPSED);
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
           TEST_PARAMS->F_TX_COUNT,
           TEST_PARAMS->F_RX_COUNT,
           TEST_PARAMS->F_RX_OTHER,
           TEST_PARAMS->F_RX_ONTIME,
           TEST_PARAMS->F_RX_EARLY,
           TEST_PARAMS->F_RX_LATE,
           TEST_PARAMS->B_SPEED_MAX,
           TEST_PARAMS->F_SPEED_MAX,
           TEST_PARAMS->B_SPEED_AVG,
           TEST_PARAMS->F_SPEED_AVG,
           (TEST_PARAMS->B_TX/1024)/1024);


    return;
 }
