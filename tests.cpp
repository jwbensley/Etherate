/*
 * License:
 *
 * Copyright (c) 2012-2015 James Bensley.
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
 * void delay_test(struct APP_PARAMS *APP_PARAMS, struct FRAME_HEADERS *FRAME_HEADERS,
                   struct TEST_INTERFACE *TEST_INTERFACE, struct TEST_PARAMS * TEST_PARAMS,
                   struct QM_TEST *QM_TEST);
 * void mtu_sweep_test(struct APP_PARAMS *APP_PARAMS, struct FRAME_HEADERS *FRAME_HEADERS,
                       struct TEST_INTERFACE *TEST_INTERFACE, struct TEST_PARAMS *TEST_PARAMS,
                       struct MTU_TEST *MTU_TEST)
 * void latency_test(struct APP_PARAMS *APP_PARAMS, struct FRAME_HEADERS *FRAME_HEADERS,
                     struct TEST_INTERFACE *TEST_INTERFACE, struct TEST_PARAMS *TEST_PARAMS,
                     struct QM_TEST *QM_TEST)
 * void speed_test(struct APP_PARAMS *APP_PARAMS, struct FRAME_HEADERS *FRAME_HEADERS,
                   struct TEST_INTERFACE *TEST_INTERFACE, struct TEST_PARAMS *TEST_PARAMS)
 *
 */

void delay_test(struct APP_PARAMS *APP_PARAMS, struct FRAME_HEADERS *FRAME_HEADERS,
                struct TEST_INTERFACE *TEST_INTERFACE, struct TEST_PARAMS * TEST_PARAMS,
                struct QM_TEST *QM_TEST);

// Run an MTU sweep test from TX to RX
void mtu_sweep_test(struct APP_PARAMS *APP_PARAMS, struct FRAME_HEADERS *FRAME_HEADERS,
                    struct TEST_INTERFACE *TEST_INTERFACE, struct TEST_PARAMS *TEST_PARAMS,
                    struct MTU_TEST *MTU_TEST);

// Run some quality measurements from TX to RX
void latency_test(struct APP_PARAMS *APP_PARAMS, struct FRAME_HEADERS *FRAME_HEADERS,
                  struct TEST_INTERFACE *TEST_INTERFACE, struct TEST_PARAMS *TEST_PARAMS,
                  struct QM_TEST *QM_TEST);

// Run a speedtest (which is the default test operation if none other is specified)
void speed_test(struct APP_PARAMS *APP_PARAMS, struct FRAME_HEADERS *FRAME_HEADERS,
                struct TEST_INTERFACE *TEST_INTERFACE, struct TEST_PARAMS *TEST_PARAMS);




void delay_test(struct APP_PARAMS *APP_PARAMS, struct FRAME_HEADERS *FRAME_HEADERS,
                struct TEST_INTERFACE *TEST_INTERFACE, struct TEST_PARAMS * TEST_PARAMS,
                struct QM_TEST *QM_TEST)
{
// Calculate the delay between TX and RX hosts. The uptime us exchanged twice
// to estimate the delay between the hosts. Then the process is repeated so
// an average can be taken


    long double UPTIME;
    long long   TX_UPTIME;
    int         TX_RET_VAL;
    int         RX_LEN;
    char        WAITING;


    build_tlv(FRAME_HEADERS, htons(TYPE_TESTFRAME), htonl(TYPE_TESTFRAME));


    if(APP_PARAMS->TX_MODE)
    {

        printf("Calculating delay between hosts...\n");

        for (int i=0; i < QM_TEST->DELAY_TEST_COUNT; i++)
        {
          
            // Get the current time and send it to RX
            clock_gettime(CLOCK_MONOTONIC_RAW, &QM_TEST->TS_RTT);

            // Convert it to double then long long for transmission
            UPTIME = QM_TEST->TS_RTT.tv_sec + ((double)QM_TEST->TS_RTT.tv_nsec*1e-9);
            TX_UPTIME = roundl(UPTIME*1000000000.0);

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_DELAY1), htonll(TX_UPTIME));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

            // Repeat
            clock_gettime(CLOCK_MONOTONIC_RAW, &QM_TEST->TS_RTT);

            UPTIME = QM_TEST->TS_RTT.tv_sec + ((double)QM_TEST->TS_RTT.tv_nsec*1e-9);
            TX_UPTIME = roundl(UPTIME*1000000000.0);

            build_sub_tlv(FRAME_HEADERS, htons(TYPE_DELAY2), htonll(TX_UPTIME));

            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE,
                                0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));                                


            // Wait for RX to send back delay value
            WAITING = true;
            while (WAITING)
            {
                RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD,
                                  FRAME_HEADERS->RX_BUFFER,
                                  TEST_PARAMS->F_SIZE_TOTAL,
                                  0, NULL, NULL);

                if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_DELAY)
                {
                    QM_TEST->pDELAY_RESULTS[i] = (ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE)/1000000000.0);
                    WAITING = false;
                }

            }

        } // End delay TX loop

        // Let the receiver know all delay values have been received
        build_tlv(FRAME_HEADERS, htons(TYPE_TESTFRAME), htonl(VALUE_DELAY_END));

        TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                            FRAME_HEADERS->LENGTH+FRAME_HEADERS->TLV_SIZE,
                            0,(struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                            sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

        double DELAY_AVERAGE = 0;

        for (int i=0; i < QM_TEST->DELAY_TEST_COUNT; i++)
        {
            DELAY_AVERAGE += QM_TEST->pDELAY_RESULTS[i];
        }

        DELAY_AVERAGE = (DELAY_AVERAGE/QM_TEST->DELAY_TEST_COUNT);

        printf("Tx to Rx delay calculated as %.9f seconds\n\n", DELAY_AVERAGE);


    // This is the RX host
    } else {

        printf("Calculating delay between hosts...\n");

        // These values are used to calculate the delay between TX and RX hosts
        double TIME_TX_1[QM_TEST->DELAY_TEST_COUNT];
        double TIME_TX_2[QM_TEST->DELAY_TEST_COUNT];
        double TIME_RX_1[QM_TEST->DELAY_TEST_COUNT];
        double TIME_RX_2[QM_TEST->DELAY_TEST_COUNT];
        double TIME_TX_DIFF[QM_TEST->DELAY_TEST_COUNT];
        double TIME_RX_DIFF[QM_TEST->DELAY_TEST_COUNT];
        memset(TIME_TX_1,    0, sizeof(TIME_TX_1));
        memset(TIME_TX_2,    0, sizeof(TIME_TX_2));
        memset(TIME_RX_1,    0, sizeof(TIME_RX_1));
        memset(TIME_RX_2,    0, sizeof(TIME_RX_2));
        memset(TIME_TX_DIFF, 0, sizeof(TIME_TX_DIFF));
        memset(TIME_RX_DIFF, 0, sizeof(TIME_RX_DIFF));

        int DELAY_INDEX = 0;

        while(WAITING)
        {

            RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD,
                              FRAME_HEADERS->RX_BUFFER,
                              TEST_PARAMS->F_SIZE_TOTAL,
                              0, NULL, NULL);

            if (ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_DELAY1)
            {

                // Get the time we are receiving TX's sent time figure
                clock_gettime(CLOCK_MONOTONIC_RAW, &QM_TEST->TS_RTT);

                // Record the time we received this TX value
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

                DELAY_INDEX++;


            } // End if delay value

            if (ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_DELAY_END)
            {
                WAITING = false;

                double DELAY_AVERAGE = 0;

                for (int i=0; i < QM_TEST->DELAY_TEST_COUNT; i++)
                {
                    DELAY_AVERAGE += QM_TEST->pDELAY_RESULTS[i];
                }

                DELAY_AVERAGE = (DELAY_AVERAGE/QM_TEST->DELAY_TEST_COUNT);

                printf("Tx to Rx delay calculated as %.9f seconds\n\n", DELAY_AVERAGE);

            }

        } // End of WAITING

    }

    return;
}




void mtu_sweep_test(struct APP_PARAMS *APP_PARAMS, struct FRAME_HEADERS *FRAME_HEADERS,
                    struct TEST_INTERFACE *TEST_INTERFACE, struct TEST_PARAMS *TEST_PARAMS,
                    struct MTU_TEST *MTU_TEST)
{

    // Display the current interface MTU
    int PHY_MTU = get_interface_mtu_by_name(TEST_INTERFACE);
    if (PHY_MTU==-1) printf("Physical interface MTU unknown, "
                            "test might exceed physical MTU!\n");

    if (MTU_TEST->MTU_TX_MAX > PHY_MTU) {
        printf("Starting MTU sweep from %u to %u (interface max)\n",
               MTU_TEST->MTU_TX_MIN, PHY_MTU);
        MTU_TEST->MTU_TX_MAX = PHY_MTU;
    } else {
        printf("Starting MTU sweep from %u to %u\n",
               MTU_TEST->MTU_TX_MIN, MTU_TEST->MTU_TX_MAX);
    }


    FD_ZERO(&TEST_INTERFACE->FD_READS);
    TEST_INTERFACE->SOCKET_FD_COUNT = TEST_INTERFACE->SOCKET_FD + 1;

    int TX_RET_VAL = 0;
    int RX_LEN =     0;

    build_tlv(FRAME_HEADERS, htons(TYPE_TESTFRAME), htonl(VALUE_TEST_SUB_TLV));

    if (APP_PARAMS->TX_MODE) {

        int MTU_TX_CURRENT =   0;
        int MTU_ACK_PREVIOUS = 0;
        int MTU_ACK_CURRENT =  0;
        int MTU_ACK_LARGEST =  0;
        char WAITING =         true;

        for (MTU_TX_CURRENT = MTU_TEST->MTU_TX_MIN; MTU_TX_CURRENT <= MTU_TEST->MTU_TX_MAX; MTU_TX_CURRENT++)
        {

            // Get the current time
            clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_ELAPSED_TIME);

            // Send each MTU test frame 3 times
            for (int i = 1; i <= 3; i++)
            {

                build_sub_tlv(FRAME_HEADERS, htons(TYPE_FRAMEINDEX), htonll(MTU_TX_CURRENT));

                TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD,
                                    FRAME_HEADERS->TX_BUFFER,
                                    FRAME_HEADERS->LENGTH+MTU_TX_CURRENT, 0, 
                                    (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                    sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

                TEST_PARAMS->F_TX_COUNT++;

                WAITING = true;

            }

            // Wait for the ACK from back from RX
            while(WAITING)
            {

                // Get the current time
                clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_CURRENT_TIME);

                TEST_INTERFACE->TV_SELECT_DELAY.tv_sec = 0;
                TEST_INTERFACE->TV_SELECT_DELAY.tv_usec = 000000;
                FD_SET(TEST_INTERFACE->SOCKET_FD, &TEST_INTERFACE->FD_READS);

                TEST_INTERFACE->SELECT_RET_VAL = select(TEST_INTERFACE->SOCKET_FD_COUNT,
                                                        &TEST_INTERFACE->FD_READS,
                                                        NULL, NULL,
                                                        &TEST_INTERFACE->TV_SELECT_DELAY);

                if (TEST_INTERFACE->SELECT_RET_VAL > 0 &&
                    FD_ISSET(TEST_INTERFACE->SOCKET_FD, &TEST_INTERFACE->FD_READS))
                {

                    RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD,
                                      FRAME_HEADERS->RX_BUFFER,
                                      MTU_TEST->MTU_TX_MAX,
                                      0, NULL, NULL);

                    if (ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_TEST_SUB_TLV &&
                        ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_ACKINDEX)
                    {

                        TEST_PARAMS->F_RX_COUNT++;

                        // Get the MTU size RX is ACK'ing
                        MTU_ACK_CURRENT = (unsigned int)ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE);
                        if (MTU_ACK_CURRENT > MTU_ACK_LARGEST) 
                        {
                            MTU_ACK_LARGEST = MTU_ACK_CURRENT;
                        }


                        if (MTU_ACK_CURRENT < MTU_ACK_PREVIOUS)
                        {
                            // Frame received out of order, later than expected
                            TEST_PARAMS->F_RX_LATE+1;
                        } else if (MTU_ACK_CURRENT > MTU_ACK_PREVIOUS+1) {
                            // Frame received out of order, earlier than expected
                            TEST_PARAMS->F_RX_EARLY+1;
                        } else if (MTU_ACK_CURRENT == MTU_ACK_PREVIOUS+1) {
                            // Frame received in order
                            MTU_ACK_PREVIOUS = MTU_ACK_CURRENT;
                        }


                    // A non-test frame was received
                    } else {

                        TEST_PARAMS->F_RX_OTHER++;

                    }


                } // End of TEST_INTERFACE->SELECT_RET_VAL

                // 1 seconds have passed TX has probably missed/lost the ACK
                if ((TEST_PARAMS->TS_CURRENT_TIME.tv_sec - 
                     TEST_PARAMS->TS_ELAPSED_TIME.tv_sec) >= 1) WAITING = false;

            } // End of WAITING

            
        } // End of TX transmit


        printf("Largest MTU ACK'ed from RX is %u\n\n", MTU_ACK_LARGEST);

        printf("Test frames transmitted: %llu\n"
               "Test frames received: %llu\n"
               "Non test frames received: %llu\n"
               "In order ACK frames received: %llu\n"
               "Out of order ACK frames received early: %llu\n"
               "Out of order ACK frames received late: %llu\n",
               TEST_PARAMS->F_TX_COUNT,
               TEST_PARAMS->F_RX_COUNT,
               TEST_PARAMS->F_RX_OTHER,
               TEST_PARAMS->F_RX_ONTIME,
               TEST_PARAMS->F_RX_EARLY,
               TEST_PARAMS->F_RX_LATE);

        return;


    // Running in RX mode
    } else {

        unsigned int MTU_RX_PREVIOUS = 0;
        unsigned int MTU_RX_CURRENT =  0;
        unsigned int MTU_RX_LARGEST =  0;
        char WAITING =                 true;

        // If 5 seconds pass without receiving a frame, end the MTU sweep test
        // (assume MTU has been exceeded)
        char MTU_RX_ANY = false;
        
        clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_ELAPSED_TIME);

        while (WAITING)
        {

            // Get the current time
            clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_CURRENT_TIME);

            // Check for test frame from TX
            TEST_INTERFACE->TV_SELECT_DELAY.tv_sec = 0;
            TEST_INTERFACE->TV_SELECT_DELAY.tv_usec = 000000;
            FD_SET(TEST_INTERFACE->SOCKET_FD, &TEST_INTERFACE->FD_READS);

            TEST_INTERFACE->SELECT_RET_VAL = select(TEST_INTERFACE->SOCKET_FD_COUNT,
                                                    &TEST_INTERFACE->FD_READS,
                                                    NULL, NULL,
                                                    &TEST_INTERFACE->TV_SELECT_DELAY);

            if (TEST_INTERFACE->SELECT_RET_VAL > 0 &&
                FD_ISSET(TEST_INTERFACE->SOCKET_FD, &TEST_INTERFACE->FD_READS))
            {

                RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD,
                                  FRAME_HEADERS->RX_BUFFER,
                                  MTU_TEST->MTU_TX_MAX,
                                  0, NULL, NULL);

                if (ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_TEST_SUB_TLV &&
                    ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_FRAMEINDEX)
                {

                    MTU_RX_ANY = true;

                    TEST_PARAMS->F_RX_COUNT++;

                    // Get the MTU size TX is sending
                    MTU_RX_CURRENT = (unsigned int)ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE);

                    if (MTU_RX_CURRENT > MTU_RX_LARGEST)
                    {

                        MTU_RX_LARGEST = MTU_RX_CURRENT;

                        // ACK that back to TX as new largest MTU received
                        // (send the ACK 3 times)
                        for(int i = 1; i <= 3; i++)
                        {

                        build_sub_tlv(FRAME_HEADERS, ntohs(TYPE_ACKINDEX), htonll(MTU_RX_CURRENT));

                        TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD,
                                            FRAME_HEADERS->TX_BUFFER,
                                            FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE, 0, 
                                            (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                            sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

                        TEST_PARAMS->F_TX_COUNT++;

                        }


                    } // MTU_RX_CURRENT > MTU_RX_LARGEST


                    if (MTU_RX_CURRENT < MTU_RX_PREVIOUS)
                    {
                        // Frame received out of order, later than expected
                        TEST_PARAMS->F_RX_LATE+1;
                    } else if (MTU_RX_CURRENT > MTU_RX_PREVIOUS+1) {
                        // Frame received out of order, earlier than expected
                        TEST_PARAMS->F_RX_EARLY+1;
                    } else if (MTU_RX_CURRENT == MTU_RX_PREVIOUS+1) {
                        // Frame received in order
                        MTU_RX_PREVIOUS = MTU_RX_CURRENT;
                    }


                // A non-test frame was recieved
                } else {

                    TEST_PARAMS->F_RX_OTHER++;

                } //End of TEST_FRAME


            } // End of TEST_INTERFACE->SELECT_RET_VAL


            // If we have received the largest MTU TX hoped to send,
            // the MTU sweep test is over
            if (MTU_RX_LARGEST == MTU_TEST->MTU_TX_MAX)
            {

                // Signal back to TX the largest MTU we recieved at the end
                WAITING = false;

                printf("MTU sweep test complete\n"
                      "Largest MTU received was %u\n\n", MTU_RX_LARGEST);

                printf("Test frames transmitted: %llu\n"
                       "Test frames received: %llu\n"
                       "Non test frames received: %llu\n"
                       "In order test frames received: %llu\n"
                       "Out of order test frames received early: %llu\n"
                       "Out of order test frames received late: %llu\n",
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


void latency_test(struct APP_PARAMS *APP_PARAMS, struct FRAME_HEADERS *FRAME_HEADERS,
                  struct TEST_INTERFACE *TEST_INTERFACE, struct TEST_PARAMS *TEST_PARAMS,
                  struct QM_TEST *QM_TEST)
{

 /*   if (QM_TEST.ENABLED)
    {


        FD_ZERO(&TEST_INTERFACE->FD_READS);
        TEST_INTERFACE->SOCKET_FD_COUNT = TEST_INTERFACE->SOCKET_FD + 1;


        if (TX_MODE)
        {

            // Fill the test frame with some junk data
            int PAYLOAD_INDEX = 0;
            for (PAYLOAD_INDEX = 0; PAYLOAD_INDEX < F_SIZE; PAYLOAD_INDEX++)
            {
                FRAME_HEADERS->TX_DATA[PAYLOAD_INDEX] = (char)((255.0*rand()/(RAND_MAX+1.0)));
            }

            long QM_TX_INDEX = 0;

            long QM_RX_INDEX;

            char WAITING = false;

            clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_ELAPSED_TIME);

            while (S_ELAPSED <= F_DURATION)
            {

                // Increment the echo frame sequence number
                QM_TX_INDEX++;

                // Send the current clock time to the RX host
                clock_gettime(CLOCK_MONOTONIC_RAW, &QM_TEST->TR_RTT);

                ss.str("");
                ss.clear();
                ss<<"etherateecho:"<<QM_TX_INDEX<<":"<<QM_TEST->TR_RTT.tv_sec<<":"<<QM_TEST->TR_RTT.tv_nsec;
                param = ss.str();
                strncpy(FRAME_HEADERS->TX_DATA,param.c_str(),param.length());

                TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER, param.length()+FRAME_HEADERS->LENGTH,
                                    0, (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                    sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

                QM_WAITING = true;

                if (WAITING)
                {

                    // Get the current time
                    clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_CURRENT_TIME);


                    // Poll the socket file descriptor with select() for incoming frames
                    TEST_INTERFACE->TV_SELECT_DELAY.tv_sec = 0;
                    TEST_INTERFACE->TV_SELECT_DELAY.tv_usec = 000000;
                    FD_SET(TEST_INTERFACE->SOCKET_FD, &TEST_INTERFACE->FD_READS);
                    TEST_INTERFACE->SELECT_RET_VAL = select(TEST_INTERFACE->SOCKET_FD_COUNT, &TEST_INTERFACE->FD_READS, NULL, NULL, &TEST_INTERFACE->TV_SELECT_DELAY);

                    if (TEST_INTERFACE->SELECT_RET_VAL > 0) {
                        if (FD_ISSET(TEST_INTERFACE->SOCKET_FD, &TEST_INTERFACE->FD_READS)) {

                            RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->RX_BUFFER, F_SIZE_TOTAL, 0, NULL, NULL);

                            param = "etherateecho";

                            if (strncmp(FRAME_HEADERS->RX_DATA,param.c_str(),param.length()) == 0)
                            {

                                // Get echo frame sequence number
                                exploded.clear();
                                explodestring = FRAME_HEADERS->RX_DATA;
                                string_explode(explodestring, ":", &exploded);
                                QM_RX_INDEX = atoi(exploded[1].c_str());

                                // This is the frame TX is waiting for,
                                // and not an early/late one
                                if (QM_RX_INDEX == QM_INDEX)
                                {

                                }

                            }

                        }
                    }


                    // Check if 1 second has passed
                    if ((TEST_PARAMS->TS_CURRENT_TIME.tv_sec-TEST_PARAMS->TS_ELAPSED_TIME.tv_sec) >= 1) {

                            S_ELAPSED++;

                            TEST_PARAMS->TS_ELAPSED_TIME.tv_sec = TEST_PARAMS->TS_CURRENT_TIME.tv_sec;
                            TEST_PARAMS->TS_ELAPSED_TIME.tv_nsec = TEST_PARAMS->TS_CURRENT_TIME.tv_nsec;
                    }

                    // Check if the echo reply has timed out
                    if ((TEST_PARAMS->TS_CURRENT_TIME.tv_sec-TEST_PARAMS->TS_ELAPSED_TIME.tv_sec) >= QM_TIMEOUT_SEC) {
                        if (TEST_PARAMS->TS_CURRENT_TIME.tv_nsec-TEST_PARAMS->TS_ELAPSED_TIME.tv_nsec) >= QM_TIMEOUT_NSEC) {


                            WAITING = false;

                        }
                    }

                } // End of WAITING


            } // F_DURATION



        // Else this is the RX host
        } else {



        } // End of TX/RX

    } */

    /*
     ***************************************************** QUALITY MEASUREMENTS
     */

     return;
 }



void speed_test(struct APP_PARAMS *APP_PARAMS, struct FRAME_HEADERS *FRAME_HEADERS,
                struct TEST_INTERFACE *TEST_INTERFACE, struct TEST_PARAMS *TEST_PARAMS)
 {

    APP_PARAMS->TS_NOW =   time(0);
    APP_PARAMS->TM_LOCAL = localtime(&APP_PARAMS->TS_NOW);
    printf("Starting speed test on %s\n", asctime(APP_PARAMS->TM_LOCAL));

    int TX_RET_VAL = 0;
    int RX_LEN =     0;

    FD_ZERO(&TEST_INTERFACE->FD_READS);
    TEST_INTERFACE->SOCKET_FD_COUNT = TEST_INTERFACE->SOCKET_FD + 1;

    build_tlv(FRAME_HEADERS, htons(TYPE_TESTFRAME), htonl(VALUE_TEST_SUB_TLV));

    if (APP_PARAMS->TX_MODE == true)
    {

        printf("Seconds\t\tMbps TX\t\tMBs Tx\t\tFrmTX/s\t\tFrames TX\n");

        unsigned long long *testBase, *testMax;

        if (TEST_PARAMS->F_BYTES > 0)
        {    
            // We are testing until X bytes received
            testMax = &TEST_PARAMS->F_BYTES;
            testBase = &TEST_PARAMS->B_TX;

        } else if (TEST_PARAMS->F_COUNT > 0) {   
            // We are testing until X frames received
            testMax = &TEST_PARAMS->F_COUNT;
            testBase = &TEST_PARAMS->F_TX_COUNT;

        } else if (TEST_PARAMS->F_DURATION > 0) {
            // We are testing until X seconds has passed
            TEST_PARAMS->F_DURATION--;
            testMax = (unsigned long long*)&TEST_PARAMS->F_DURATION;
            testBase = (unsigned long long*)&TEST_PARAMS->S_ELAPSED;
        }


        // Get clock times for the speed limit restriction and starting time
        clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_TX_LIMIT);
        clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_ELAPSED_TIME);

        // Main TX test loop
        while (*testBase<=*testMax)
        {

            clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_CURRENT_TIME);

            // One second has passed
            if ((TEST_PARAMS->TS_CURRENT_TIME.tv_sec - TEST_PARAMS->TS_ELAPSED_TIME.tv_sec) >= 1)
            {
                TEST_PARAMS->S_ELAPSED++;
                TEST_PARAMS->B_SPEED = (((float)TEST_PARAMS->B_TX-(float)TEST_PARAMS->B_TX_PREV)*8)/1000/1000;
                TEST_PARAMS->B_TX_PREV = TEST_PARAMS->B_TX;

                printf("%llu\t\t%.2f\t\t%llu\t\t%llu\t\t%llu\n",
                       TEST_PARAMS->S_ELAPSED,
                       TEST_PARAMS->B_SPEED,
                       (TEST_PARAMS->B_TX/1000)/1000,
                       (TEST_PARAMS->F_TX_COUNT-TEST_PARAMS->F_TX_COUNT_PREV),
                       TEST_PARAMS->F_TX_COUNT);

                if (TEST_PARAMS->B_SPEED > TEST_PARAMS->B_SPEED_MAX)
                    TEST_PARAMS->B_SPEED_MAX = TEST_PARAMS->B_SPEED;
                
                TEST_PARAMS->B_SPEED_AVG += TEST_PARAMS->B_SPEED;
                TEST_PARAMS->F_TX_COUNT_PREV = TEST_PARAMS->F_TX_COUNT;
                TEST_PARAMS->TS_ELAPSED_TIME.tv_sec = TEST_PARAMS->TS_CURRENT_TIME.tv_sec;
                TEST_PARAMS->TS_ELAPSED_TIME.tv_nsec = TEST_PARAMS->TS_CURRENT_TIME.tv_nsec;
            }

            // Poll incoming frames
            TEST_INTERFACE->TV_SELECT_DELAY.tv_sec = 0;
            TEST_INTERFACE->TV_SELECT_DELAY.tv_usec = 000000;
            FD_SET(TEST_INTERFACE->SOCKET_FD, &TEST_INTERFACE->FD_READS);
            TEST_INTERFACE->SELECT_RET_VAL = select(TEST_INTERFACE->SOCKET_FD_COUNT,
                                                    &TEST_INTERFACE->FD_READS,
                                                    NULL, NULL,
                                                    &TEST_INTERFACE->TV_SELECT_DELAY);

            if (TEST_INTERFACE->SELECT_RET_VAL > 0 && 
                FD_ISSET(TEST_INTERFACE->SOCKET_FD, &TEST_INTERFACE->FD_READS))
            {

                RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->RX_BUFFER,
                                  TEST_PARAMS->F_SIZE_TOTAL, 0, NULL, NULL);

                // Running in ACK mode
                if (TEST_PARAMS->F_ACK)
                {


                    if (ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_TEST_SUB_TLV &&
                        ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_ACKINDEX)
                    {

                        TEST_PARAMS->F_RX_COUNT++;
                        TEST_PARAMS->F_WAITING_ACK = false;

                    if (ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE) == (TEST_PARAMS->F_INDEX_PREV+1)) {
                        TEST_PARAMS->F_RX_ONTIME++;
                        TEST_PARAMS->F_INDEX_PREV++;
                    } else if (ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE) > (TEST_PARAMS->F_RX_COUNT)) {
                        TEST_PARAMS->F_INDEX_PREV = ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE);
                        TEST_PARAMS->F_RX_EARLY++;
                    } else if (ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE) <= TEST_PARAMS->F_RX_COUNT) {
                        TEST_PARAMS->F_RX_LATE++;
                    }

                    } else if (ntohs(*FRAME_HEADERS->RX_TLV_TYPE) == TYPE_APPLICATION &&
                               ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_DYINGGASP) {

                        APP_PARAMS->TS_NOW = time(0);
                        APP_PARAMS->TM_LOCAL = localtime(&APP_PARAMS->TS_NOW);

                        printf("RX host has quit\n"
                               "Ending test on %s\n",
                               asctime(APP_PARAMS->TM_LOCAL));

                        return;

                    // Received a non-test frame
                    } else {
                        TEST_PARAMS->F_RX_OTHER++;
                    }

                // Not running in ACK mode
                } else if (ntohs(*FRAME_HEADERS->RX_TLV_TYPE) == TYPE_APPLICATION &&
                           ntohll(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_DYINGGASP) {
                    
                    APP_PARAMS->TS_NOW = time(0);
                    APP_PARAMS->TM_LOCAL = localtime(&APP_PARAMS->TS_NOW);

                    printf("RX host has quit\n"
                           "Ending test on %s\n",
                           asctime(APP_PARAMS->TM_LOCAL));

                    return;

                // Received a non-test frame
                } else {
                    TEST_PARAMS->F_RX_OTHER++;
                }

            } // End of TEST_INTERFACE->SELECT_RET_VAL

            // If it hasn't been 1 second yet, send more test frames
            if (TEST_PARAMS->TS_CURRENT_TIME.tv_sec - TEST_PARAMS->TS_TX_LIMIT.tv_sec < 1)
            {

                if (TEST_PARAMS->F_WAITING_ACK == false)
                {

                    // Check if a max speed has been set and sending another frame
                    // would exceed the maximum speed
                    if (TEST_PARAMS->B_TX_SPEED_MAX != B_TX_SPEED_MAX_DEF)
                    {

                        // Check if sending another frame keeps us under the
                        // max speed limit
                        if ((TEST_PARAMS->B_TX_SPEED_PREV + TEST_PARAMS->F_SIZE) <= TEST_PARAMS->B_TX_SPEED_MAX)
                        {


                            build_sub_tlv(FRAME_HEADERS, htons(TYPE_FRAMEINDEX), 
                                          htonll(TEST_PARAMS->F_TX_COUNT+1));

                            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD,
                                                FRAME_HEADERS->TX_BUFFER,
                                                TEST_PARAMS->F_SIZE_TOTAL, 0, 
                                                (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

                            TEST_PARAMS->F_TX_COUNT++;
                            TEST_PARAMS->B_TX += TEST_PARAMS->F_SIZE;
                            TEST_PARAMS->B_TX_SPEED_PREV += TEST_PARAMS->F_SIZE;
                            if (TEST_PARAMS->F_ACK) TEST_PARAMS->F_WAITING_ACK = true;

                        }

                    // Else there is no speed restriction
                    } else {

                        build_sub_tlv(FRAME_HEADERS, htons(TYPE_FRAMEINDEX),
                                      htonll(TEST_PARAMS->F_TX_COUNT+1));

                        TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD,
                                            FRAME_HEADERS->TX_BUFFER,
                                            TEST_PARAMS->F_SIZE_TOTAL, 0, 
                                            (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                            sizeof(TEST_INTERFACE->SOCKET_ADDRESS));


                        TEST_PARAMS->F_TX_COUNT += 1;
                        TEST_PARAMS->B_TX += TEST_PARAMS->F_SIZE;
                        TEST_PARAMS->B_TX_SPEED_PREV += TEST_PARAMS->F_SIZE;
                        if (TEST_PARAMS->F_ACK) TEST_PARAMS->F_WAITING_ACK = true;

                    }
                }

            } else { // 1 second has passed

              TEST_PARAMS->B_TX_SPEED_PREV = 0;
              clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_TX_LIMIT);

            } // End of TEST_PARAMS->TS_TX_LIMIT.tv_sec<1

        }


        TEST_PARAMS->B_SPEED_AVG = (TEST_PARAMS->B_SPEED_AVG/TEST_PARAMS->S_ELAPSED);

        printf("Test frames transmitted: %llu\n"
               "Test frames received: %llu\n"
               "Non test frames received: %llu\n"
               "In order ACK frames received: %llu\n"
               "Out of order ACK frames received early: %llu\n"
               "Out of order ACK frames received late: %llu\n"
               "Maximum speed achieved during test: %.2f\n"
               "Average speed achieved during test: %.2Lf\n",
               TEST_PARAMS->F_TX_COUNT,
               TEST_PARAMS->F_RX_COUNT,
               TEST_PARAMS->F_RX_OTHER,
               TEST_PARAMS->F_RX_ONTIME,
               TEST_PARAMS->F_RX_EARLY,
               TEST_PARAMS->F_RX_LATE,
               TEST_PARAMS->B_SPEED_MAX,
               TEST_PARAMS->B_SPEED_AVG);

        APP_PARAMS->TS_NOW = time(0);
        APP_PARAMS->TM_LOCAL = localtime(&APP_PARAMS->TS_NOW);
        printf ("Ending speed test on %s\n", asctime(APP_PARAMS->TM_LOCAL));


    // Else, we are in RX mode
    } else {

        printf("Seconds\t\tMbps RX\t\tMBs Rx\t\tFrmRX/s\t\tFrames RX\n");

        unsigned long long *testBase, *testMax;

        // Are we testing until X bytes transmitted
        if (TEST_PARAMS->F_BYTES > 0)
        {
            testMax = &TEST_PARAMS->F_BYTES;
            testBase = &TEST_PARAMS->B_RX;

        // Or are we testing until X frames transmitted
        } else if (TEST_PARAMS->F_COUNT > 0) {
            testMax = &TEST_PARAMS->F_COUNT;
            testBase = &TEST_PARAMS->F_RX_COUNT;

        // Or are we testing until X seconds have passed
        } else if (TEST_PARAMS->F_DURATION > 0) {
            TEST_PARAMS->F_DURATION--;
            testMax = (unsigned long long*)&TEST_PARAMS->F_DURATION;
            testBase = (unsigned long long*)&TEST_PARAMS->S_ELAPSED;
        }


        clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_ELAPSED_TIME);

        // Main RX test loop
        while (*testBase<=*testMax)
        {

            clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_CURRENT_TIME);

            // If one second has passed
            if ((TEST_PARAMS->TS_CURRENT_TIME.tv_sec-TEST_PARAMS->TS_ELAPSED_TIME.tv_sec) >= 1)
            {
                TEST_PARAMS->S_ELAPSED++;
                TEST_PARAMS->B_SPEED = float (((float)TEST_PARAMS->B_RX-(float)TEST_PARAMS->B_RX_PREV)*8)/1000/1000;
                TEST_PARAMS->B_RX_PREV = TEST_PARAMS->B_RX;

                printf("%llu\t\t%.2f\t\t%llu\t\t%llu\t\t%llu\n",
                       TEST_PARAMS->S_ELAPSED,
                       TEST_PARAMS->B_SPEED,
                       (TEST_PARAMS->B_RX/1000)/1000,
                       (TEST_PARAMS->F_RX_COUNT-TEST_PARAMS->F_RX_COUNT_PREV),
                       TEST_PARAMS->F_RX_COUNT);

                if (TEST_PARAMS->B_SPEED > TEST_PARAMS->B_SPEED_MAX)
                    TEST_PARAMS->B_SPEED_MAX = TEST_PARAMS->B_SPEED;

                TEST_PARAMS->B_SPEED_AVG += TEST_PARAMS->B_SPEED;
                TEST_PARAMS->F_RX_COUNT_PREV = TEST_PARAMS->F_RX_COUNT;
                TEST_PARAMS->TS_ELAPSED_TIME.tv_sec = TEST_PARAMS->TS_CURRENT_TIME.tv_sec;
                TEST_PARAMS->TS_ELAPSED_TIME.tv_nsec = TEST_PARAMS->TS_CURRENT_TIME.tv_nsec;

            }

            // Poll for incoming frames
            TEST_INTERFACE->TV_SELECT_DELAY.tv_sec = 0;
            TEST_INTERFACE->TV_SELECT_DELAY.tv_usec = 000000;
            FD_SET(TEST_INTERFACE->SOCKET_FD, &TEST_INTERFACE->FD_READS);

            TEST_INTERFACE->SELECT_RET_VAL = select(TEST_INTERFACE->SOCKET_FD_COUNT,
                                                    &TEST_INTERFACE->FD_READS,
                                                    NULL, NULL,
                                                    &TEST_INTERFACE->TV_SELECT_DELAY);


            if (TEST_INTERFACE->SELECT_RET_VAL > 0 &&
                FD_ISSET(TEST_INTERFACE->SOCKET_FD, &TEST_INTERFACE->FD_READS))
            {

                RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD,
                                  FRAME_HEADERS->RX_BUFFER,
                                  TEST_PARAMS->F_SIZE_TOTAL,
                                  0, NULL, NULL);

                // Check if this is an etherate test frame
                if (ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_TEST_SUB_TLV &&
                    ntohs(*FRAME_HEADERS->RX_SUB_TLV_TYPE) == TYPE_FRAMEINDEX)
                {

                    // Update test stats
                    TEST_PARAMS->F_RX_COUNT++;
                    TEST_PARAMS->B_RX+=(RX_LEN-FRAME_HEADERS->LENGTH);

                    if (ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE) == (TEST_PARAMS->F_INDEX_PREV+1)) {
                        TEST_PARAMS->F_RX_ONTIME++;
                        TEST_PARAMS->F_INDEX_PREV++;
                    } else if (ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE) > (TEST_PARAMS->F_RX_COUNT)) {
                        TEST_PARAMS->F_INDEX_PREV = ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE);
                        TEST_PARAMS->F_RX_EARLY++;
                    } else if (ntohll(*FRAME_HEADERS->RX_SUB_TLV_VALUE) <= TEST_PARAMS->F_RX_COUNT) {
                        TEST_PARAMS->F_RX_LATE++;
                    }

                    // If running in ACK mode we need to ACK to TX host
                    if (TEST_PARAMS->F_ACK)
                    {

                        build_sub_tlv(FRAME_HEADERS, htons(TYPE_ACKINDEX),
                                      *FRAME_HEADERS->RX_SUB_TLV_VALUE);

                        TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD,
                                            FRAME_HEADERS->TX_BUFFER,
                                            FRAME_HEADERS->LENGTH+FRAME_HEADERS->SUB_TLV_SIZE, 0, 
                                            (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                            sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

                        TEST_PARAMS->F_TX_COUNT++;
                        
                    }

                } else {
                    // Received a non-test frame
                    TEST_PARAMS->F_RX_OTHER++;
                }

                // Check if TX host has quit/died;
                if (ntohl(*FRAME_HEADERS->RX_TLV_VALUE) == VALUE_DYINGGASP)
                {
                    APP_PARAMS->TS_NOW = time(0);
                    APP_PARAMS->TM_LOCAL = localtime(&APP_PARAMS->TS_NOW);

                    printf("TX host has quit\n"
                           "Ending test and resetting on %s\n",
                           asctime(APP_PARAMS->TM_LOCAL));

                    return;
                }
                      

            }

        }


        TEST_PARAMS->B_SPEED_AVG = (TEST_PARAMS->B_SPEED_AVG/TEST_PARAMS->S_ELAPSED);

        printf("Test frames transmitted: %llu\n"
               "Test frames received: %llu\n"
               "Non test frames received: %llu\n"
               "In order test frames received: %llu\n"
               "Out of order test frames received early: %llu\n"
               "Out of order test frames received late: %llu\n"
               "Maximum speed achieved during test: %.2f\n"
               "Average speed achieved during test: %.2Lf\n",
               TEST_PARAMS->F_TX_COUNT,
               TEST_PARAMS->F_RX_COUNT,
               TEST_PARAMS->F_RX_OTHER,
               TEST_PARAMS->F_RX_ONTIME,
               TEST_PARAMS->F_RX_EARLY,
               TEST_PARAMS->F_RX_LATE,
               TEST_PARAMS->B_SPEED_MAX,
               TEST_PARAMS->B_SPEED_AVG);

        APP_PARAMS->TS_NOW = time(0);
        APP_PARAMS->TM_LOCAL = localtime(&APP_PARAMS->TS_NOW);
        printf ("Ending speed test on %s", asctime(APP_PARAMS->TM_LOCAL));

    }

    return;
 }