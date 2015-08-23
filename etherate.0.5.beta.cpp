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
 * File: Etherate Main Body
 *
 * Updates: https://github.com/jwbensley/Etherate
 * http://null.53bits.co.uk/index.php?page=etherate
 * Please send corrections, ideas and help to: jwbensley@gmail.com 
 * (I'm a beginner if that isn't obvious!)
 *
 * Compile: g++ -o etherate etherate.cpp -lrt
 *
 * File Contents:
 *
 * HEADERS AND DEFS
 * GLOBAL VARIABLES
 * CLI ARGS
 * TX/RX SETUP
 * SETTINGS SYNC
 * MTU SWEEP
 * LINK QUALITY MEASUREMENT
 * MAIN TEST PHASE
 *
 *
 ************************************************************* HEADERS AND DEFS
 */

#include <cmath> // std::abs() from cmath for floats and doubles
#include <cstring> // Needed for memcpy
#include <ctime> // Required for timer types
#include <fcntl.h> // fcntl()
#include <iomanip> //  setprecision() and precision()
#include <iostream> // cout
using namespace std;
#include <sys/socket.h> // Provides AF_PACKET *Address Family*
#include <sys/select.h>
#include <ifaddrs.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h> // Defines ETH_P_ALL [Ether type 0x003 (ALL PACKETS!)]
// Also defines ETH_FRAME_LEN with default 1514
// Also defines ETH_ALEN which is Ethernet Address Lenght in Octets (defined as 6)
#include <linux/if_packet.h>
#include <linux/net.h> // Provides SOCK_RAW *Socket Type*
#include <netinet/in.h> // Needed for htons() and htonl()
#include <signal.h> // For use of signal() to catch SIGINT
#include <sstream> // Stringstream objects and functions
#include <stdio.h> //perror()
#include <stdlib.h> //malloc() itoa()
#include <stdbool.h> // bool/_Bool
#include <string.h> //Required for strncmp() function
#define MAX_IFS 64
#include "sysexits.h"
/*
 * EX_USAGE 64 command line usage error
 * EX_NOPERM 77 permission denied
 * EX_SOFTWARE 70 internal software error
 * EX_PROTOCOL 76 remote error in protocol
 */
#include <sys/ioctl.h>
#include <time.h> // Required for clock_gettime(), struct timeval
#include "unistd.h" // sleep(), getuid()
#include <vector> // Required for string explode function

// CLOCK_MONOTONIC_RAW is in from linux-2.26.28 onwards,
// It's needed here for accurate delay calculation
// There is a good S-O post about this here;
// http://stackoverflow.com/questions/3657289/linux-clock-gettimeclock-
// monotonic-strange-non-monotonic-behavior
#ifndef CLOCK_MONOTONIC_RAW
#define CLOCK_MONOTONIC_RAW 4
#endif

#include "etherate.0.5.beta.h"
#include "etherate.0.5.beta.funcs.cpp"
#include "etherate.0.5.beta.tests.cpp"


/*
 ************************************************************* HEADERS AND DEFS
 */


int main(int argc, char *argv[]) {


    restart:

    // Set default values

    struct FRAME_HEADERS FRAME_HEADERS;
    pFRAME_HEADERS = &FRAME_HEADERS;
    FRAME_HEADERS.SOURCE_MAC[0] = 0x00;
    FRAME_HEADERS.SOURCE_MAC[1] = 0x00;
    FRAME_HEADERS.SOURCE_MAC[2] = 0x5E;
    FRAME_HEADERS.SOURCE_MAC[3] = 0x00;
    FRAME_HEADERS.SOURCE_MAC[4] = 0x00;
    FRAME_HEADERS.SOURCE_MAC[5] = 0x01;
    FRAME_HEADERS.DESTINATION_MAC[0] = 0x00;
    FRAME_HEADERS.DESTINATION_MAC[1] = 0x00;
    FRAME_HEADERS.DESTINATION_MAC[2] = 0x5E;
    FRAME_HEADERS.DESTINATION_MAC[3] = 0x00;
    FRAME_HEADERS.DESTINATION_MAC[4] = 0x00;
    FRAME_HEADERS.DESTINATION_MAC[5] = 0x02;
    FRAME_HEADERS.LENGTH = HEADERS_LEN_DEF;
    FRAME_HEADERS.ETHERTYPE = ETHERTYPE_DEF;
    FRAME_HEADERS.PCP = PCP_DEF;
    FRAME_HEADERS.VLAN_ID = VLAN_ID_DEF;
    FRAME_HEADERS.QINQ_ID = QINQ_ID_DEF;
    FRAME_HEADERS.QINQ_PCP = QINQ_PCP_DEF; 


    struct TEST_INTERFACE TEST_INTERFACE;
    pTEST_INTERFACE = &TEST_INTERFACE;
    TEST_INTERFACE.IF_INDEX = IF_INDEX_DEF;


    struct TEST_PARAMS TEST_PARAMS;
    TEST_PARAMS.F_SIZE = F_SIZE_DEF;
    TEST_PARAMS.F_SIZE_TOTAL = F_SIZE_DEF + FRAME_HEADERS.LENGTH;
    TEST_PARAMS.F_DURATION = F_DURATION_DEF;
    TEST_PARAMS.F_COUNT = F_COUNT_DEF; 
    TEST_PARAMS.F_BYTES = F_BYTES_DEF;
    TEST_PARAMS.S_ELAPSED = 0;
    TEST_PARAMS.B_TX_SPEED_MAX = B_TX_SPEED_MAX_DEF;
    TEST_PARAMS.B_TX_SPEED_PREV = 0;
    TEST_PARAMS.F_TX_COUNT = 0;
    TEST_PARAMS.F_TX_COUNT_PREV = 0;
    TEST_PARAMS.B_TX = 0;
    TEST_PARAMS.B_TX_PREV = 0;
    TEST_PARAMS.F_RX_COUNT = 0;
    TEST_PARAMS.F_RX_COUNT_PREV = 0;
    TEST_PARAMS.B_RX = 0;
    TEST_PARAMS.B_RX_PREV = 0;
    TEST_PARAMS.F_INDEX = 0;
    TEST_PARAMS.F_INDEX_PREV = 0;
    TEST_PARAMS.F_RX_ONTIME = 0;
    TEST_PARAMS.F_RX_EARLY = 0;
    TEST_PARAMS.F_RX_LATE = 0;
    TEST_PARAMS.F_RX_OTHER = 0;
    TEST_PARAMS.B_SPEED = 0;
    TEST_PARAMS.F_ACK = false;
    TEST_PARAMS.F_WAITING_ACK = false;

    struct MTU_TEST MTU_TEST;
    MTU_TEST.ENABLED = false;
    MTU_TEST.MTU_TX_MIN = 1400;
    MTU_TEST.MTU_TX_MAX = 1500;

    struct QM_TEST QM_TEST;
    QM_TEST.ENABLED = false;
    QM_TEST.INTERVAL = 1000;
    QM_TEST.INTERVAL_SEC = 0;
    QM_TEST.INTERVAL_NSEC = 0;
    QM_TEST.TIMEOUT = 1000;
    QM_TEST.TIMEOUT_NSEC = 0;
    QM_TEST.TIMEOUT_SEC = 0;
    QM_TEST.DELAY_TEST_COUNT = 10000;
    QM_TEST.pDELAY_RESULTS = (double*)malloc(sizeof(double) * QM_TEST.DELAY_TEST_COUNT);


    struct APP_PARAMS APP_PARAMS;

    APP_PARAMS.TX_MODE = true;
    APP_PARAMS.TX_SYNC = true;



    /*
     ***************************************************************** CLI ARGS
     */

    if (cli_args(argc, argv, &APP_PARAMS, &FRAME_HEADERS, &TEST_INTERFACE,
                 &TEST_PARAMS, &MTU_TEST, &QM_TEST) == -1) return EX_USAGE;



    /*
     ************************************************************** TX/RX SETUP
     */


    // Check for root access, low level socket access is needed;
    if (getuid() != 0) {
        cout << "Must be root to use this program!"<<endl;
        return EX_NOPERM;
    }


    TEST_INTERFACE.SOCKET_FD = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    /* A socket must be opened with three arguments;
     * Communication Domain = AF_PACKET
     * Type/Communication Semantics = SOCK_RAW
     * Protocol = ETH_P_ALL == 0x0003, which is everything, from if_ether.h)
     */


    if (TEST_INTERFACE.SOCKET_FD < 0 )
    {
      cout << "Error defining socket."<<endl;
      perror("socket() ");
      close(TEST_INTERFACE.SOCKET_FD);
      return EX_SOFTWARE;
    }


    // If the user has supplied an interface index try to use that
    if (TEST_INTERFACE.IF_INDEX > 0) {

        TEST_INTERFACE.IF_INDEX = set_sock_interface_index(&TEST_INTERFACE);
        if (TEST_INTERFACE.IF_INDEX == 0)
        {
            cout << "Error: Couldn't set interface with index, "
                 << "returned index was 0."<<endl;
            return EX_SOFTWARE;
        }

    // Or if the user has supplied an interface name try to use that        
    } else if (strcmp(TEST_INTERFACE.IF_NAME,"") != 0) {

        TEST_INTERFACE.IF_INDEX = set_sock_interface_name(&TEST_INTERFACE);
        if (TEST_INTERFACE.IF_INDEX == 0)
        {
            cout << "Error: Couldn't set interface index from name, "
                 << "returned index was 0."<<endl;
            return EX_SOFTWARE;
        }

    // Otherwise, try and best guess an interface
    } else if (TEST_INTERFACE.IF_INDEX == IF_INDEX_DEF) {

        TEST_INTERFACE.IF_INDEX = get_sock_interface(&TEST_INTERFACE);
        if (TEST_INTERFACE.IF_INDEX == 0)
        {
            cout << "Error: Couldn't find appropriate interface ID, returned ID "
                 << "was 0. This is typically the loopback interface ID."
                 << "Try supplying a source MAC address with the -s option."<<endl;
            return EX_SOFTWARE;
        }

    }


    // Link layer socket struct
    TEST_INTERFACE.SOCKET_ADDRESS.sll_family   = PF_PACKET;	
    TEST_INTERFACE.SOCKET_ADDRESS.sll_protocol = htons(ETH_P_IP);
    TEST_INTERFACE.SOCKET_ADDRESS.sll_ifindex  = TEST_INTERFACE.IF_INDEX;    
    TEST_INTERFACE.SOCKET_ADDRESS.sll_hatype   = ARPHRD_ETHER;
    TEST_INTERFACE.SOCKET_ADDRESS.sll_pkttype  = PACKET_OTHERHOST;
    TEST_INTERFACE.SOCKET_ADDRESS.sll_halen    = ETH_ALEN;		
    TEST_INTERFACE.SOCKET_ADDRESS.sll_addr[0]  = FRAME_HEADERS.DESTINATION_MAC[0];		
    TEST_INTERFACE.SOCKET_ADDRESS.sll_addr[1]  = FRAME_HEADERS.DESTINATION_MAC[1];
    TEST_INTERFACE.SOCKET_ADDRESS.sll_addr[2]  = FRAME_HEADERS.DESTINATION_MAC[2];
    TEST_INTERFACE.SOCKET_ADDRESS.sll_addr[3]  = FRAME_HEADERS.DESTINATION_MAC[3];
    TEST_INTERFACE.SOCKET_ADDRESS.sll_addr[4]  = FRAME_HEADERS.DESTINATION_MAC[4];
    TEST_INTERFACE.SOCKET_ADDRESS.sll_addr[5]  = FRAME_HEADERS.DESTINATION_MAC[5];
    TEST_INTERFACE.SOCKET_ADDRESS.sll_addr[6]  = 0x00;
    TEST_INTERFACE.SOCKET_ADDRESS.sll_addr[7]  = 0x00;

    // Send and receive buffers for incoming/outgoing ethernet frames
    FRAME_HEADERS.RX_BUFFER = (char*)malloc(F_SIZE_MAX);
    FRAME_HEADERS.TX_BUFFER = (char*)malloc(F_SIZE_MAX);
    
    build_headers(&FRAME_HEADERS);

    // Pointers to the payload section of the frame
    FRAME_HEADERS.RX_DATA = FRAME_HEADERS.RX_BUFFER + FRAME_HEADERS.LENGTH;
    FRAME_HEADERS.TX_DATA = FRAME_HEADERS.TX_BUFFER + FRAME_HEADERS.LENGTH;

    // 0 is success exit code for sending frames
    int TX_RET_VAL = 0;

    // Length of the received frame
    int RX_LEN = 0;


    // http://www.linuxjournal.com/article/4659?page=0,1
    cout << "Entering promiscuous mode"<<endl;
    strncpy(ethreq.ifr_name,TEST_INTERFACE.IF_NAME,IFNAMSIZ);

    // Set the network card in promiscuos mode
    if (ioctl(TEST_INTERFACE.SOCKET_FD,SIOCGIFFLAGS,&ethreq) == -1) 
    {
        cout << "Error getting socket flags, entering promiscuous mode failed."
             << endl;

        perror("ioctl() ");
        close(TEST_INTERFACE.SOCKET_FD);
        return EX_SOFTWARE;
    }

    ethreq.ifr_flags|=IFF_PROMISC;

    if (ioctl(TEST_INTERFACE.SOCKET_FD,SIOCSIFFLAGS,&ethreq) == -1)
    {
        cout << "Error setting socket flags, entering promiscuous mode failed."
             << endl;

        perror("ioctl() ");
        close(TEST_INTERFACE.SOCKET_FD);
        return EX_SOFTWARE;
    }


    // At this point, declare our sigint handler, from now on when the two hosts
    // start communicating it will be of use, the TX will signal RX to reset
    // when it dies
    signal (SIGINT,signal_handler);

    printf("Source MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           FRAME_HEADERS.SOURCE_MAC[0],FRAME_HEADERS.SOURCE_MAC[1],FRAME_HEADERS.SOURCE_MAC[2],
           FRAME_HEADERS.SOURCE_MAC[3],FRAME_HEADERS.SOURCE_MAC[4],FRAME_HEADERS.SOURCE_MAC[5]);

    printf("Destination MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           FRAME_HEADERS.DESTINATION_MAC[0],FRAME_HEADERS.DESTINATION_MAC[1],
           FRAME_HEADERS.DESTINATION_MAC[2],FRAME_HEADERS.DESTINATION_MAC[3],
           FRAME_HEADERS.DESTINATION_MAC[4],FRAME_HEADERS.DESTINATION_MAC[5]);

    /* 
     * Before any communication happens between the local host and remote host,
     * we must broadcast our whereabouts to ensure there is no initial loss of frames
     *
     * Set the dest MAC to broadcast (FF...FF) and build the frame like this,
     * then transmit three frames with a short brake between each.
     * Rebuild the frame headers with the actual dest MAC.
     */

    unsigned char TEMP_MAC[6] = {
        FRAME_HEADERS.DESTINATION_MAC[0],
        FRAME_HEADERS.DESTINATION_MAC[1],
        FRAME_HEADERS.DESTINATION_MAC[2],
        FRAME_HEADERS.DESTINATION_MAC[3],
        FRAME_HEADERS.DESTINATION_MAC[4],
        FRAME_HEADERS.DESTINATION_MAC[5],
    };

    FRAME_HEADERS.DESTINATION_MAC[0] = 0xFF;
    FRAME_HEADERS.DESTINATION_MAC[1] = 0xFF;
    FRAME_HEADERS.DESTINATION_MAC[2] = 0xFF;
    FRAME_HEADERS.DESTINATION_MAC[3] = 0xFF;
    FRAME_HEADERS.DESTINATION_MAC[4] = 0xFF;
    FRAME_HEADERS.DESTINATION_MAC[5] = 0xFF;

    build_headers(&FRAME_HEADERS);

    FRAME_HEADERS.RX_DATA = FRAME_HEADERS.RX_BUFFER + FRAME_HEADERS.LENGTH;
    FRAME_HEADERS.TX_DATA = FRAME_HEADERS.TX_BUFFER + FRAME_HEADERS.LENGTH;

    ///// REMOVE THIS FILTH
    std::string param;

    cout << "Sending gratuitous broadcasts..."<<endl<<endl;
    for (int i=1; i<=3; i++)
    {
        param = "etheratepresence";
        strncpy(FRAME_HEADERS.TX_DATA,param.c_str(),param.length());

        TX_RET_VAL = sendto(TEST_INTERFACE.SOCKET_FD, FRAME_HEADERS.TX_BUFFER,
                            param.length()+FRAME_HEADERS.LENGTH, 0, 
                            (struct sockaddr*)&TEST_INTERFACE.SOCKET_ADDRESS,
                            sizeof(TEST_INTERFACE.SOCKET_ADDRESS));

        sleep(1);
    }

    FRAME_HEADERS.DESTINATION_MAC[0] = TEMP_MAC[0];
    FRAME_HEADERS.DESTINATION_MAC[1] = TEMP_MAC[1];
    FRAME_HEADERS.DESTINATION_MAC[2] = TEMP_MAC[2];
    FRAME_HEADERS.DESTINATION_MAC[3] = TEMP_MAC[3];
    FRAME_HEADERS.DESTINATION_MAC[4] = TEMP_MAC[4];
    FRAME_HEADERS.DESTINATION_MAC[5] = TEMP_MAC[5];

    build_headers(&FRAME_HEADERS);


    // Adjust for single dot1q tags
    if (FRAME_HEADERS.LENGTH == 18)
    {
        FRAME_HEADERS.RX_DATA = FRAME_HEADERS.RX_BUFFER + (FRAME_HEADERS.LENGTH-4);
        FRAME_HEADERS.TX_DATA = FRAME_HEADERS.TX_BUFFER + FRAME_HEADERS.LENGTH;
    } else if (FRAME_HEADERS.LENGTH == 22) {
        // Linux is swallowing the outter tag of QinQ, see:
        // http://stackoverflow.com/questions/24355597/
        // linux-when-sending-ethernet-frames-the-ethertype-is-being-re-written
        FRAME_HEADERS.RX_DATA = FRAME_HEADERS.RX_BUFFER + (FRAME_HEADERS.LENGTH-4);
        FRAME_HEADERS.TX_DATA = FRAME_HEADERS.TX_BUFFER + FRAME_HEADERS.LENGTH;
    } else {
        FRAME_HEADERS.RX_DATA = FRAME_HEADERS.RX_BUFFER + FRAME_HEADERS.LENGTH;
        FRAME_HEADERS.TX_DATA = FRAME_HEADERS.TX_BUFFER + FRAME_HEADERS.LENGTH;
    }

    TEST_PARAMS.F_SIZE_TOTAL = TEST_PARAMS.F_SIZE + FRAME_HEADERS.LENGTH;



    /*
     ************************************************************ SETTINGS SYNC
     */

    sync_settings(&APP_PARAMS, &FRAME_HEADERS, &TEST_INTERFACE, &TEST_PARAMS,
                  &MTU_TEST, &QM_TEST);

    // Rebuild the test frame headers in case any settings have been changed
    // by the TX host
    if (!APP_PARAMS.TX_MODE) build_headers(&FRAME_HEADERS);



    /*
     **************************************************************** MTU SWEEP
     */

    // Run a max MTU sweep test from TX to RX
    if (MTU_TEST.ENABLED) {

        mtu_sweep_test(&APP_PARAMS, &FRAME_HEADERS, &TEST_INTERFACE, &TEST_PARAMS, &MTU_TEST);
        goto restart; ///// REMOVE THIS FILTH

    }



    /*
     ********************************************************** SPEED TEST PHASE
     */    

    // Fill the test frame with some random data
    int PAYLOAD_INDEX = 0;
    for (PAYLOAD_INDEX = 0; PAYLOAD_INDEX < MTU_TEST.MTU_TX_MAX; PAYLOAD_INDEX++)
    {
        FRAME_HEADERS.TX_DATA[PAYLOAD_INDEX] = (char)((255.0*rand()/(RAND_MAX+1.0)));
    }

    
    speed_test(&APP_PARAMS, &FRAME_HEADERS, &TEST_INTERFACE, &TEST_PARAMS);
    if (!APP_PARAMS.TX_MODE) goto restart;


    finish:

    cout << "Leaving promiscuous mode"<<endl;

    strncpy(ethreq.ifr_name,TEST_INTERFACE.IF_NAME,IFNAMSIZ);

    if (ioctl(TEST_INTERFACE.SOCKET_FD,SIOCGIFFLAGS,&ethreq) == -1)
    {
        cout << "Error getting socket flags, entering promiscuous mode failed."<<endl;
        perror("ioctl() ");
        close(TEST_INTERFACE.SOCKET_FD);
        return EX_SOFTWARE;
    }

    ethreq.ifr_flags &= ~IFF_PROMISC;

    if (ioctl(TEST_INTERFACE.SOCKET_FD,SIOCSIFFLAGS,&ethreq) == -1)
    {
        cout << "Error setting socket flags, promiscuous mode failed."<<endl;
        perror("ioctl() ");
        close(TEST_INTERFACE.SOCKET_FD);
        return EX_SOFTWARE;
    }

    close(TEST_INTERFACE.SOCKET_FD);

    return EXIT_SUCCESS;

} // End of main()
