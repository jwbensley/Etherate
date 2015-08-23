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


void mtu_sweep_test(struct APP_PARAMS *APP_PARAMS, struct FRAME_HEADERS *FRAME_HEADERS,
                    struct TEST_INTERFACE *TEST_INTERFACE, struct TEST_PARAMS *TEST_PARAMS,
                    struct MTU_TEST *MTU_TEST)
{

    ///// REMOVE THIS FILTH
    std::string param;
    std::stringstream ss;
    ss.setf(ios::fixed);
    ss.setf(ios::showpoint);
    ss.precision(9);
    cout<<fixed<<setprecision(9);
    ///// AND THIS FILTH
    vector<string> exploded;
    string explodestring;


    // Display the current interface MTU
    int PHY_MTU = get_interface_mtu_by_name(TEST_INTERFACE);

    if (MTU_TEST->MTU_TX_MAX > PHY_MTU) {
        cout << "Starting MTU sweep from "<<MTU_TEST->MTU_TX_MIN<<" to "<<PHY_MTU
             << " (interface max)"<<endl;
        MTU_TEST->MTU_TX_MAX = PHY_MTU;
    } else {
        cout << "Starting MTU sweep from "<<MTU_TEST->MTU_TX_MIN<<" to "<<MTU_TEST->MTU_TX_MAX<<endl;
    }


    FD_ZERO(&TEST_INTERFACE->FD_READS);
    TEST_INTERFACE->SOCKET_FD_COUNT = TEST_INTERFACE->SOCKET_FD + 1;

    int TX_RET_VAL = 0;
    int RX_LEN = 0;

    if (APP_PARAMS->TX_MODE) {

        // Fill the test frame with some junk data
        int PAYLOAD_INDEX = 0;
        for (PAYLOAD_INDEX = 0; PAYLOAD_INDEX < MTU_TEST->MTU_TX_MAX; PAYLOAD_INDEX++)
        {
            FRAME_HEADERS->TX_DATA[PAYLOAD_INDEX] = (char)((255.0*rand()/(RAND_MAX+1.0)));
        }

        int MTU_TX_CURRENT = 0;
        int MTU_ACK_PREVIOUS = 0;
        int MTU_ACK_CURRENT = 0;
        int MTU_ACK_LARGEST = 0;

        for (MTU_TX_CURRENT = MTU_TEST->MTU_TX_MIN; MTU_TX_CURRENT <= MTU_TEST->MTU_TX_MAX; MTU_TX_CURRENT++)
        {

            // Send each MTU test frame 3 times
            for (int i = 1; i <= 3; i++)
            {

                ss.str("");
                ss.clear();
                ss<<"etheratemtu:"<<MTU_TX_CURRENT<<":";
                param = ss.str();

                strncpy(FRAME_HEADERS->TX_DATA,param.c_str(),param.length());

                TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                    FRAME_HEADERS->LENGTH+MTU_TX_CURRENT, 0,
                                    (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                    sizeof(TEST_INTERFACE->SOCKET_ADDRESS));


                // Check for largest ACK from RX host
                TEST_INTERFACE->TV_SELECT_DELAY.tv_sec = 0;
                TEST_INTERFACE->TV_SELECT_DELAY.tv_usec = 000000;
                FD_SET(TEST_INTERFACE->SOCKET_FD, &TEST_INTERFACE->FD_READS);

                TEST_INTERFACE->SELECT_RET_VAL = select(TEST_INTERFACE->SOCKET_FD_COUNT, &TEST_INTERFACE->FD_READS, NULL,
                                        NULL, &TEST_INTERFACE->TV_SELECT_DELAY);

                if (TEST_INTERFACE->SELECT_RET_VAL > 0) {
                    if (FD_ISSET(TEST_INTERFACE->SOCKET_FD, &TEST_INTERFACE->FD_READS)) {

                        RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->RX_BUFFER, MTU_TEST->MTU_TX_MAX,
                                          0, NULL, NULL);

                        param = "etheratemtuack";

                        if (strncmp(FRAME_HEADERS->RX_DATA,param.c_str(),param.length()) == 0)
                        {

                            // Get the MTU size RX is ACK'ing
                            exploded.clear();
                            explodestring = FRAME_HEADERS->RX_DATA;
                            string_explode(explodestring, ":", &exploded);
                            MTU_ACK_CURRENT = atoi(exploded[1].c_str());

                            if (MTU_ACK_CURRENT > MTU_ACK_PREVIOUS) 
                            {
                                MTU_ACK_LARGEST = MTU_ACK_CURRENT;
                            }

                            MTU_ACK_PREVIOUS = MTU_ACK_CURRENT;

                        }

                    }
                } // End of TEST_INTERFACE->SELECT_RET_VAL

            } // End of 3 frame retry
            
        } // End of TX transmit


        // Wait now for the final ACK from RX confirming the largest MTU received
        bool MTU_TX_WAITING = true;

        // Only wait 5 seconds for this
        clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_ELAPSED_TIME);

        while (MTU_TX_WAITING)
        {

            // Get the current time
            clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_CURRENT_TIME);

            // 5 seconds have passed we have probably missed/lost it
            if ((TEST_PARAMS->TS_CURRENT_TIME.tv_sec-TEST_PARAMS->TS_ELAPSED_TIME.tv_sec) >= 5)
            {
                cout << "Timeout waiting for final MTU ACK from RX"<<endl;
                MTU_TX_WAITING = false;
            }

            TEST_INTERFACE->TV_SELECT_DELAY.tv_sec = 0;
            TEST_INTERFACE->TV_SELECT_DELAY.tv_usec = 000000;
            FD_SET(TEST_INTERFACE->SOCKET_FD, &TEST_INTERFACE->FD_READS);

            TEST_INTERFACE->SELECT_RET_VAL = select(TEST_INTERFACE->SOCKET_FD_COUNT, &TEST_INTERFACE->FD_READS, NULL,
                                    NULL, &TEST_INTERFACE->TV_SELECT_DELAY);

            if (TEST_INTERFACE->SELECT_RET_VAL > 0) {
                if (FD_ISSET(TEST_INTERFACE->SOCKET_FD, &TEST_INTERFACE->FD_READS)) {

                    RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->RX_BUFFER, MTU_TEST->MTU_TX_MAX, 0,
                                      NULL, NULL);

                    param = "etheratemtuack";

                    if (strncmp(FRAME_HEADERS->RX_DATA,param.c_str(),param.length()) == 0)
                    {

                        // Get the MTU size RX is ACK'ing
                        exploded.clear();
                        explodestring = FRAME_HEADERS->RX_DATA;
                        string_explode(explodestring, ":", &exploded);
                        MTU_ACK_CURRENT = atoi(exploded[1].c_str());

                        if (MTU_ACK_CURRENT > MTU_ACK_LARGEST)
                            MTU_ACK_LARGEST = MTU_ACK_CURRENT;

                        if (MTU_ACK_LARGEST == MTU_TEST->MTU_TX_MAX)
                            MTU_TX_WAITING = false;

                    }

                }
            } // End of TEST_INTERFACE->SELECT_RET_VAL

        } // End of MTU TX WAITING

        cout << "Largest MTU ACK'ed from RX is "<<MTU_ACK_LARGEST<<endl<<endl;

        return;


    } else { // Running in RX mode

        int MTU_RX_PREVIOUS = 0;
        int MTU_RX_CURRENT = 0;
        int MTU_RX_LARGEST = 0;

        bool MTU_RX_WAITING = true;


        // Set up some counters, if 3 seconds pass without receiving
        // a frame, end the test (assume MTU has been exceeded)
        bool MTU_RX_ANY = false;
        clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_ELAPSED_TIME);
        while (MTU_RX_WAITING)
        {

            // Check for largest ACK from RX host
            TEST_INTERFACE->TV_SELECT_DELAY.tv_sec = 0;
            TEST_INTERFACE->TV_SELECT_DELAY.tv_usec = 000000;
            FD_SET(TEST_INTERFACE->SOCKET_FD, &TEST_INTERFACE->FD_READS);

            TEST_INTERFACE->SELECT_RET_VAL = select(TEST_INTERFACE->SOCKET_FD_COUNT, &TEST_INTERFACE->FD_READS, NULL, NULL,
                                    &TEST_INTERFACE->TV_SELECT_DELAY);

            if (TEST_INTERFACE->SELECT_RET_VAL > 0) {
                if (FD_ISSET(TEST_INTERFACE->SOCKET_FD, &TEST_INTERFACE->FD_READS)) {

                    RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->RX_BUFFER, MTU_TEST->MTU_TX_MAX, 0,
                                      NULL, NULL);

                    param = "etheratemtu";

                    if (strncmp(FRAME_HEADERS->RX_DATA,param.c_str(),param.length()) == 0)
                    {


                        MTU_RX_ANY = true;

                        // Get the MTU size TX is sending
                        exploded.clear();
                        explodestring = FRAME_HEADERS->RX_DATA;
                        string_explode(explodestring, ":", &exploded);
                        MTU_RX_CURRENT = atoi(exploded[1].c_str());

                        if (MTU_RX_CURRENT > MTU_RX_PREVIOUS)
                        {

                            MTU_RX_LARGEST = MTU_RX_CURRENT;

                            // ACK that back to TX as new largest MTU received
                            ss.str("");
                            ss.clear();
                            ss<<"etheratemtuack:"<<MTU_RX_CURRENT<<":";
                            param = ss.str();

                            strncpy(FRAME_HEADERS->TX_DATA,param.c_str(),param.length());

                            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                                FRAME_HEADERS->LENGTH+param.length(), 0,
                                                (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));


                        }

                        MTU_RX_PREVIOUS = MTU_RX_CURRENT;

                    }

                }
            } // End of TEST_INTERFACE->SELECT_RET_VAL

            // If we have received the largest MTU TX hoped to send,
            // the MTU sweep test is over
            if (MTU_RX_LARGEST == MTU_TEST->MTU_TX_MAX)
            {

                // Signal back to TX the largest MTU we recieved at the end
                ss.str("");
                ss.clear();
                ss<<"etheratemtuack:"<<MTU_RX_LARGEST<<":";
                param = ss.str();

                strncpy(FRAME_HEADERS->TX_DATA,param.c_str(),param.length());

                TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER,
                                    FRAME_HEADERS->LENGTH+param.length(), 0,
                                    (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                    sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

                MTU_RX_WAITING = false;

                cout << "MTU sweep test complete"<<endl
                     << "Largest MTU received was "<<MTU_RX_LARGEST<<endl
                     << endl;

            }

            // Get the current time
            clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_CURRENT_TIME);

            // 5 seconds have passed without any MTU sweep frames being receeved
            if ((TEST_PARAMS->TS_CURRENT_TIME.tv_sec-TEST_PARAMS->TS_ELAPSED_TIME.tv_sec) >= 5)
            {

                if (MTU_RX_ANY == false)
                {
                    cout << "Timeout waiting for MTU sweep frames from TX, "
                         << "ending the sweep"<<endl<<"Largest MTU received "
                         << MTU_RX_LARGEST<<endl<<endl;

                    MTU_RX_WAITING = false;
                } else {
                    clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_ELAPSED_TIME);
                    MTU_RX_ANY = false;
                }

            }


        } // End of RX MTU WAITING


    } // End of TX/RX mode


    return;
}


void latency_test(struct APP_PARAMS *APP_PARAMS, struct FRAME_HEADERS *FRAME_HEADERS,
                  struct TEST_INTERFACE *TEST_INTERFACE, struct TEST_PARAMS *TEST_PARAMS,
                  struct QM_TEST *QM_TEST)
{
    /*
     ***************************************************** QUALITY MEASUREMENTS
     */

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

            bool QM_WAITING = false;

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

                if (QM_WAITING)
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


                            QM_WAITING = false;

                        }
                    }

                } // QM_WAITING


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


    ///// REMOVE THIS FILTH
    std::string param;
    std::stringstream ss;
    ss.setf(ios::fixed);
    ss.setf(ios::showpoint);
    ss.precision(9);
    cout<<fixed<<setprecision(9);
    ///// AND THIS FILTH
    vector<string> exploded;
    string explodestring;

    int TX_RET_VAL = 0;
    int RX_LEN = 0;

    APP_PARAMS->TS_NOW = time(0);
    APP_PARAMS->TM_LOCAL = localtime(&APP_PARAMS->TS_NOW);
    cout << endl<<"Starting test on "<<asctime(APP_PARAMS->TM_LOCAL)<<endl;
    ss.precision(2);
    cout << fixed << setprecision(2);

    FD_ZERO(&TEST_INTERFACE->FD_READS);
    TEST_INTERFACE->SOCKET_FD_COUNT = TEST_INTERFACE->SOCKET_FD + 1;

    if (APP_PARAMS->TX_MODE == true)
    {

        cout << "Seconds\t\tMbps TX\t\tMBs Tx\t\tFrmTX/s\t\tFrames TX"<<endl;

        long long *testBase, *testMax;
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
            testMax = &TEST_PARAMS->F_DURATION;
            testBase = &TEST_PARAMS->S_ELAPSED;
        }


        // Get clock time for the speed limit option,
        // get another to record the initial starting time
        clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_TX_LIMIT);
        clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_ELAPSED_TIME);

        // Main TX loop
        while (*testBase<=*testMax)
        {

            // Get the current time
            clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_CURRENT_TIME);

            // One second has passed
            if ((TEST_PARAMS->TS_CURRENT_TIME.tv_sec-TEST_PARAMS->TS_ELAPSED_TIME.tv_sec) >= 1)
            {
                TEST_PARAMS->S_ELAPSED++;
                TEST_PARAMS->B_SPEED = (((float)TEST_PARAMS->B_TX-(float)TEST_PARAMS->B_TX_PREV)*8)/1000/1000;
                TEST_PARAMS->B_TX_PREV = TEST_PARAMS->B_TX;

                cout << TEST_PARAMS->S_ELAPSED<<"\t\t"<<TEST_PARAMS->B_SPEED<<"\t\t"<<(TEST_PARAMS->B_TX/1000)/1000
                     << "\t\t"<<(TEST_PARAMS->F_TX_COUNT-TEST_PARAMS->F_TX_COUNT_PREV)<<"\t\t"<<TEST_PARAMS->F_TX_COUNT
                     << endl;

                TEST_PARAMS->F_TX_COUNT_PREV = TEST_PARAMS->F_TX_COUNT;
                TEST_PARAMS->TS_ELAPSED_TIME.tv_sec = TEST_PARAMS->TS_CURRENT_TIME.tv_sec;
                TEST_PARAMS->TS_ELAPSED_TIME.tv_nsec = TEST_PARAMS->TS_CURRENT_TIME.tv_nsec;
            }

            // Poll the socket file descriptor with select() for incoming frames
            TEST_INTERFACE->TV_SELECT_DELAY.tv_sec = 0;
            TEST_INTERFACE->TV_SELECT_DELAY.tv_usec = 000000;
            FD_SET(TEST_INTERFACE->SOCKET_FD, &TEST_INTERFACE->FD_READS);
            TEST_INTERFACE->SELECT_RET_VAL = select(TEST_INTERFACE->SOCKET_FD_COUNT, &TEST_INTERFACE->FD_READS, NULL, NULL, &TEST_INTERFACE->TV_SELECT_DELAY);

            if (TEST_INTERFACE->SELECT_RET_VAL > 0) {
                if (FD_ISSET(TEST_INTERFACE->SOCKET_FD, &TEST_INTERFACE->FD_READS)) {

                    RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->RX_BUFFER, TEST_PARAMS->F_SIZE_TOTAL, 0, NULL, NULL);

                    if (TEST_PARAMS->F_ACK)
                    {
                        param = "etherateack";

                        if (strncmp(FRAME_HEADERS->RX_DATA,param.c_str(),param.length()) == 0)
                        {
                            TEST_PARAMS->F_RX_COUNT++;
                            TEST_PARAMS->F_WAITING_ACK = false;

                        } else {

                            // Check if RX host has sent a dying gasp
                            param = "etheratedeath";

                            if (strncmp(FRAME_HEADERS->RX_DATA,param.c_str(),param.length()) == 0)
                            {
                                APP_PARAMS->TS_NOW = time(0);
                                APP_PARAMS->TM_LOCAL = localtime(&APP_PARAMS->TS_NOW);
                                cout << "RX host is going down."<<endl
                                     << "Ending test and resetting on "
                                     << asctime(APP_PARAMS->TM_LOCAL)<<endl;
                                return;

                            } else {
                                TEST_PARAMS->F_RX_OTHER++;
                            }

                        }

                    } else {
                        
                        // Check if RX host has sent a dying gasp
                        param = "etheratedeath";
                        if (strncmp(FRAME_HEADERS->RX_DATA,param.c_str(),param.length()) == 0)
                        {
                            APP_PARAMS->TS_NOW = time(0);
                            APP_PARAMS->TM_LOCAL = localtime(&APP_PARAMS->TS_NOW);
                            cout << "RX host is going down."<<endl
                                 << "Ending test and resetting on "
                                 << asctime(APP_PARAMS->TM_LOCAL)<<endl;
                            return;

                        } else {
                            TEST_PARAMS->F_RX_OTHER++;
                        }
                        
                    }
                    
                }
            }

            // If it hasn't been 1 second yet
            if (TEST_PARAMS->TS_CURRENT_TIME.tv_sec - TEST_PARAMS->TS_TX_LIMIT.tv_sec < 1)
            {

                if (!TEST_PARAMS->F_WAITING_ACK) {

                    // A max speed has been set
                    if (TEST_PARAMS->B_TX_SPEED_MAX != B_TX_SPEED_MAX_DEF)
                    {

                        // Check if sending another frame keeps us under the
                        // max speed limit
                        if ((TEST_PARAMS->B_TX_SPEED_PREV+TEST_PARAMS->F_SIZE) <= TEST_PARAMS->B_TX_SPEED_MAX)
                        {

                            ss.clear();
                            ss.str("");
                            ss << "etheratetest:"<<(TEST_PARAMS->F_TX_COUNT+1)<<":";
                            param = ss.str();
                            strncpy(FRAME_HEADERS->TX_DATA,param.c_str(), param.length());

                            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER, TEST_PARAMS->F_SIZE_TOTAL, 0, 
                                                (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                                sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

                            TEST_PARAMS->F_TX_COUNT++;
                            TEST_PARAMS->B_TX += TEST_PARAMS->F_SIZE;
                            TEST_PARAMS->B_TX_SPEED_PREV += TEST_PARAMS->F_SIZE;
                            if (TEST_PARAMS->F_ACK) TEST_PARAMS->F_WAITING_ACK = true;

                        }

                    } else {

                        ss.clear();
                        ss.str("");
                        ss << "etheratetest:" << (TEST_PARAMS->F_TX_COUNT+1) <<  ":";
                        param = ss.str();
                        strncpy(FRAME_HEADERS->TX_DATA,param.c_str(), param.length());

                        TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER, TEST_PARAMS->F_SIZE_TOTAL, 0, 
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

        cout << "Test frames transmitted: "<<TEST_PARAMS->F_TX_COUNT<<endl
             << "Test frames received: "<<TEST_PARAMS->F_RX_COUNT<<endl
             << "Non test frames received: "<<TEST_PARAMS->F_RX_OTHER<<endl;

        APP_PARAMS->TS_NOW = time(0);
        APP_PARAMS->TM_LOCAL = localtime(&APP_PARAMS->TS_NOW);
        cout << endl << "Ending test on " << asctime(APP_PARAMS->TM_LOCAL) << endl;


    // Else, we are in RX mode
    } else {

        cout << "Seconds\t\tMbps RX\t\tMBs Rx\t\tFrmRX/s\t\tFrames RX"<<endl;

        long long *testBase, *testMax;

        // Are we testing until X bytes received
        if (TEST_PARAMS->F_BYTES > 0)
        {
            testMax = &TEST_PARAMS->F_BYTES;
            testBase = &TEST_PARAMS->B_RX;

        // Or are we testing until X frames received
        } else if (TEST_PARAMS->F_COUNT > 0) {
            testMax = &TEST_PARAMS->F_COUNT;
            testBase = &TEST_PARAMS->F_RX_COUNT;

        // Or are we testing until X seconds has passed
        } else if (TEST_PARAMS->F_DURATION > 0) {
            TEST_PARAMS->F_DURATION--;
            testMax = &TEST_PARAMS->F_DURATION;
            testBase = &TEST_PARAMS->S_ELAPSED;
        }

        // Get the initial starting time
        clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_ELAPSED_TIME);

        // Main RX loop
        while (*testBase<=*testMax)
        {

            clock_gettime(CLOCK_MONOTONIC_RAW, &TEST_PARAMS->TS_CURRENT_TIME);
            // If one second has passed
            if ((TEST_PARAMS->TS_CURRENT_TIME.tv_sec-TEST_PARAMS->TS_ELAPSED_TIME.tv_sec) >= 1)
            {
                TEST_PARAMS->S_ELAPSED++;
                TEST_PARAMS->B_SPEED = float (((float)TEST_PARAMS->B_RX-(float)TEST_PARAMS->B_RX_PREV)*8)/1000/1000;
                TEST_PARAMS->B_RX_PREV = TEST_PARAMS->B_RX;

                cout << TEST_PARAMS->S_ELAPSED<<"\t\t"<<TEST_PARAMS->B_SPEED<<"\t\t"<<(TEST_PARAMS->B_RX/1000)/1000
                     << "\t\t"<<(TEST_PARAMS->F_RX_COUNT-TEST_PARAMS->F_RX_COUNT)<<"\t\t"<<TEST_PARAMS->F_RX_COUNT
                     << endl;

                TEST_PARAMS->F_RX_COUNT = TEST_PARAMS->F_RX_COUNT;
                TEST_PARAMS->TS_ELAPSED_TIME.tv_sec = TEST_PARAMS->TS_CURRENT_TIME.tv_sec;
                TEST_PARAMS->TS_ELAPSED_TIME.tv_nsec = TEST_PARAMS->TS_CURRENT_TIME.tv_nsec;
            }

            // Poll the socket file descriptor with select() to 
            // check for incoming frames
            TEST_INTERFACE->TV_SELECT_DELAY.tv_sec = 0;
            TEST_INTERFACE->TV_SELECT_DELAY.tv_usec = 000000;
            FD_SET(TEST_INTERFACE->SOCKET_FD, &TEST_INTERFACE->FD_READS);

            TEST_INTERFACE->SELECT_RET_VAL = select(TEST_INTERFACE->SOCKET_FD_COUNT, &TEST_INTERFACE->FD_READS, NULL, NULL,
                                    &TEST_INTERFACE->TV_SELECT_DELAY);

            if (TEST_INTERFACE->SELECT_RET_VAL > 0) {
                if (FD_ISSET(TEST_INTERFACE->SOCKET_FD, &TEST_INTERFACE->FD_READS))
                {

                    RX_LEN = recvfrom(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->RX_BUFFER, TEST_PARAMS->F_SIZE_TOTAL, 0,
                                      NULL, NULL);

                    // Check if this is an etherate test frame
                    param = "etheratetest";

                    if (strncmp(FRAME_HEADERS->RX_DATA,param.c_str(),param.length()) == 0)
                    {

                        // Update test stats
                        TEST_PARAMS->F_RX_COUNT++;
                        TEST_PARAMS->B_RX+=(RX_LEN-FRAME_HEADERS->LENGTH);

                        // Get the index of the received frame
                        exploded.clear();
                        explodestring = FRAME_HEADERS->RX_DATA;
                        string_explode(explodestring, ":", &exploded);
                        TEST_PARAMS->F_INDEX = atoi(exploded[1].c_str());


                        if (TEST_PARAMS->F_INDEX == (TEST_PARAMS->F_RX_COUNT) || TEST_PARAMS->F_INDEX == (TEST_PARAMS->F_INDEX+1))
                        {
                            TEST_PARAMS->F_RX_ONTIME++;
                            TEST_PARAMS->F_INDEX++;
                        } else if (TEST_PARAMS->F_INDEX > (TEST_PARAMS->F_RX_COUNT)) {
                            TEST_PARAMS->F_INDEX = TEST_PARAMS->F_INDEX;
                            TEST_PARAMS->F_RX_EARLY++;
                        } else if (TEST_PARAMS->F_INDEX < TEST_PARAMS->F_RX_COUNT) {
                            TEST_PARAMS->F_RX_LATE++;
                        }


                        // If running in ACK mode we need to ACK to TX host
                        if (TEST_PARAMS->F_ACK)
                        {

                            ss.clear();
                            ss.str("");
                            ss << "etherateack"<<TEST_PARAMS->F_RX_COUNT;
                            param = ss.str();
                            strncpy(FRAME_HEADERS->TX_DATA,param.c_str(), param.length());

                            TX_RET_VAL = sendto(TEST_INTERFACE->SOCKET_FD, FRAME_HEADERS->TX_BUFFER, TEST_PARAMS->F_SIZE_TOTAL, 0, 
                                         (struct sockaddr*)&TEST_INTERFACE->SOCKET_ADDRESS,
                                         sizeof(TEST_INTERFACE->SOCKET_ADDRESS));

                            TEST_PARAMS->F_TX_COUNT++;
                            
                        }

                    } else {

                        // We received a non-test frame
                        TEST_PARAMS->F_RX_OTHER++;

                    }

                    // Check if TX host has quit/died;
                    param = "etheratedeath";
                    if (strncmp(FRAME_HEADERS->RX_DATA,param.c_str(),param.length()) == 0)
                    {
                        APP_PARAMS->TS_NOW = time(0);
                        APP_PARAMS->TM_LOCAL = localtime(&APP_PARAMS->TS_NOW);

                        cout << "TX host is going down."<<endl
                             << "Ending test and resetting on "<<asctime(APP_PARAMS->TM_LOCAL)
                             << endl;

                        return;
                    }
                      
                }
            }

        }

        cout << "Test frames transmitted: "<<TEST_PARAMS->F_TX_COUNT<<endl
             << "Test frames received: "<<TEST_PARAMS->F_RX_COUNT<<endl
             << "Non test frames received: "<<TEST_PARAMS->F_RX_OTHER<<endl
             << "In order test frames received: "<< TEST_PARAMS->F_RX_ONTIME<<endl
             << "Out of order test frames received early: "<< TEST_PARAMS->F_RX_EARLY<<endl
             << "Out of order frames received late: "<< TEST_PARAMS->F_RX_LATE<<endl;

        APP_PARAMS->TS_NOW = time(0);
        APP_PARAMS->TM_LOCAL = localtime(&APP_PARAMS->TS_NOW);
        cout << endl<<"Ending test on "<<asctime(APP_PARAMS->TM_LOCAL)<<endl;
        close(TEST_INTERFACE->SOCKET_FD);

    }

    return;
 }