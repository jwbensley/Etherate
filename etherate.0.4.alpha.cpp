/*
 * Etherate Main Body
 *
 * License: First rule of license club...
 *
 * Updates: https://github.com/jwbensley/Etherate and http://null.53bits.co.uk/index.php?page=etherate
 * Please send corrections, ideas and help to: jwbensley@gmail.com 
 * (I'm a beginner if that isn't obvious!)
 *
 * compile with: g++ -o etherate etherate.cpp -lrt
 *
 * File Contents:
 *
 * HEADERS AND DEFS
 * GLOBAL VARIABLES
 * CLI ARGS
 * TX/RX SETUP
 * SETTINGS SYNC
 * SINGLE TESTS
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
//#include <linux/if.h>
#include <linux/net.h> // Provides SOCK_RAW *Socket Type*
//#include <net/if.h> // Provides if_indextoname/if_nametoindex
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

#include "etherate.0.4.alpha.h"
#include "funcs.0.4.alpha.cpp"


/*
 ************************************************************* HEADERS AND DEFS
 */


int main(int argc, char *argv[]) {


    /*
     ********************************************************* GLOBAL VARIABLES
     */


    restart:

    // By default we are transmitting data
    bool TX_MODE = true;

    // Interface index, default to -1 so we know later if the user changed it
    int IF_INDEX = IF_INDEX_DEF;
 
    // Source MAC address - Default is in IANA unassigned range
    SOURCE_MAC[0] = 0x00;
    SOURCE_MAC[1] = 0x00;
    SOURCE_MAC[2] = 0x5E;
    SOURCE_MAC[3] = 0x00;
    SOURCE_MAC[4] = 0x00;
    SOURCE_MAC[5] = 0x01;

    // Destination MAC address - Default is in IANA unassigned range
    DESTINATION_MAC[0] = 0x00;
    DESTINATION_MAC[1] = 0x00;
    DESTINATION_MAC[2] = 0x5E;
    DESTINATION_MAC[3] = 0x00;
    DESTINATION_MAC[4] = 0x00;
    DESTINATION_MAC[5] = 0x02;

    // This is the length of a frame header by default
    ETH_HEADERS_LEN = HEADERS_LEN_DEF;

    // The ETHERTYPE for the Etherate payload
    ETHERTYPE = ETHERTYPE_DEF;

    // Default 802.1p PCP/CoS value = 0
    PCP = PCP_DEF;

    // Default 802.1q VLAN ID = 0
    VLAN_ID = VLAN_ID_DEF;

    // Default 802.1ad VLAN ID of QinQ outer frame = 0
    QINQ_ID = QINQ_ID_DEF;

    // Default 802.1p PCP/CoS value of outer frame = 0
    QINQ_PCP = QINQ_PCP_DEF;

    // Frame payload in bytes
    int F_SIZE = F_SIZE_DEF;

    // Total frame size including headers
    int F_SIZE_TOTAL = F_SIZE_DEF + ETH_HEADERS_LEN;

    // Duration in seconds
    long long F_DURATION = F_DURATION_DEF;

    // Number of frames to send
    long long F_COUNT = F_COUNT_DEF; 

    // Amount of data to transmit in bytes
    long long F_BYTES = F_BYTES_DEF;

    // Speed to transmit at (Max bytes per second)
    long B_TX_SPEED_MAX = B_TX_SPEED_MAX_DEF;

    // How fast we were transmitting for the last second
    long B_TX_SPEED_PREV = 0;

    // Total number of frames transmitted
    long long F_TX_COUNT = 0;

    // Frames sent at last count for stats
    long long F_TX_COUNT_PREV = 0;

    // Total number of frames received
    long long F_RX_COUNT = 0;

    // Frames received at last count for stats
    long long F_RX_COUNT_PREV = 0;

    // Index of the current test frame sent/received;
    long long F_INDEX = 0;

    // Index of the last test frame sent/received;
    long long F_INDEX_PREV = 0;

    // Frames received on time
    long long F_RX_ONTIME = 0;

    // Frames received out of order that are early
    long long F_RX_EARLY = 0;

    // Frames received out of order that are late
    long long F_RX_LATE = 0;

    // Number of non test frames received
    long F_RX_OTHER = 0;

    // Total number of bytes transmitted
    long long B_TX = 0;

    // Bytes sent at last count for calculating speed
    long long B_TX_PREV = 0;

    // Total number of bytes received
    long long B_RX = 0;

    // Bytes received at last count for calculating speed
    long long B_RX_PREV = 0;

    // Current speed whilst running the test
    float B_SPEED = 0;

    // Are we running in ACK mode during transmition
    bool F_ACK = false;

    // Are we waiting for an ACK frame
    bool F_WAITING_ACK = false;

    // Timeout to wait for a frame ACK
    timespec TS_ACK_TIMEOUT;

    // TX host waits to sync settings with RX or skip to transmit
    bool TX_SYCN = true;

    // These timespecs are used for calculating delay/RTT
    timespec TS_RTT;

    // 5 doubles to calculate the delay between TX & RX 5 times, to get an average
    double DELAY_RESULTS[5];

    // Two timers for timing the test and calculating stats
    timespec TS_CURRENT_TIME, TS_ELAPSED_TIME;

    // Seconds the test has been running
    long long S_ELAPSED = 0;

    // Timer used for rate limiting the TX host
    timespec TS_TX_LIMIT;

    // Time type for holding the current date and time
    time_t TS_NOW;

    // Time struct for breaking down the above time type
    tm* TM_LOCAL;

    // Elapsed time struct for polling the socket file descriptor
    struct timeval TV_SELECT_DELAY;

    // A set of socket file descriptors for polling with select()
    fd_set FD_READS;

    // FD count and ret val for polling with select()
    int SOCKET_FD_COUNT;
    int SELECT_RET_VAL;

    // Indicator for MTU sweep mode
    bool MTU_SWEEP_TEST = false;

    // Minmum and maximum MTU sizes for MTU sweep
    int MTU_TX_MIN = 1400;
    int MTU_TX_MAX = 1600;


    /* 
      These variables are declared here and used over and over throughout;

       The string explode function is used several times throughout,
       so rather than continuously defining variables, they are defined here 
    */
    vector<string> exploded;
    string explodestring;

    // Also, a generic loop counter
    int LOOP_COUNTER;


    /*
     ********************************************************* GLOBAL VARIABLES
     */




    /*
     ***************************************************************** CLI ARGS
     */


    if(argc>1) 
    {

        for (LOOP_COUNTER=1;LOOP_COUNTER<argc;LOOP_COUNTER++) 
        {

            // Change to receive mode
            if(strncmp(argv[LOOP_COUNTER],"-r",2)==0) 
            {
                TX_MODE = false;

                SOURCE_MAC[0] = 0x00;
                SOURCE_MAC[1] = 0x00;
                SOURCE_MAC[2] = 0x5E;
                SOURCE_MAC[3] = 0x00;
                SOURCE_MAC[4] = 0x00;
                SOURCE_MAC[5] = 0x02;

                DESTINATION_MAC[0] = 0x00;
                DESTINATION_MAC[1] = 0x00;
                DESTINATION_MAC[2] = 0x5E;
                DESTINATION_MAC[3] = 0x00;
                DESTINATION_MAC[4] = 0x00;
                DESTINATION_MAC[5] = 0x01;


            // Specifying a custom destination MAC address
            } else if(strncmp(argv[LOOP_COUNTER],"-d",2)==0) {
                explodestring = argv[LOOP_COUNTER+1];
                exploded.clear();
                string_explode(explodestring, ":", &exploded);

                if((int) exploded.size() != 6) 
                {
                    cout << "Error: Invalid destination MAC address!"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return EX_USAGE;
                }

                cout << "Destination MAC ";

                // Here strtoul() is used to convert the interger value of c_str()
                // to a hex number by giving it the number base 16 (hexadecimal)
                // otherwise it would be atoi()
                for(int i = 0; (i < 6) && (i < exploded.size()); ++i)
                {
                    DESTINATION_MAC[i] = (unsigned char)strtoul(exploded[i].c_str(),
                                          NULL, 16);

                    cout << setw(2)<<setfill('0')<<hex<<int(DESTINATION_MAC[i])<<":";
                }

                cout << dec << endl;
                LOOP_COUNTER++;


            // Disable settings sync between TX and RX
            } else if(strncmp(argv[LOOP_COUNTER],"-g",2)==0) {
                TX_SYCN = false;


            // Specifying a custom source MAC address
            } else if(strncmp(argv[LOOP_COUNTER],"-s",2)==0) {
                explodestring = argv[LOOP_COUNTER+1];
                exploded.clear();
                string_explode(explodestring, ":", &exploded);

                if((int) exploded.size() != 6) 
                {
                    cout << "Error: Invalid source MAC address!"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return EX_USAGE;
                }
                cout << "Custom source MAC ";

                for(int i = 0; (i < 6) && (i < exploded.size()); ++i)
                {
                    SOURCE_MAC[i] = (unsigned char)strtoul(exploded[i].c_str(),
                                     NULL, 16);

                    cout << setw(2)<<setfill('0')<<hex<<int(SOURCE_MAC[i])<<":";
                }
                cout << dec << endl;
                LOOP_COUNTER++;


            // Specifying a custom interface name
            } else if(strncmp(argv[LOOP_COUNTER],"-i",2)==0) {
                if (argc>(LOOP_COUNTER+1))
                {
                    strncpy(IF_NAME,argv[LOOP_COUNTER+1],sizeof(argv[LOOP_COUNTER+1]));
                    LOOP_COUNTER++;
                } else {
                    cout << "Oops! Missing interface name"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return EX_USAGE;
                }


            // Specifying a custom interface index
            } else if(strncmp(argv[LOOP_COUNTER],"-I",2)==0) {
                if (argc>(LOOP_COUNTER+1))
                {
                    IF_INDEX = atoi(argv[LOOP_COUNTER+1]);
                    LOOP_COUNTER++;
                } else {
                    cout << "Oops! Missing interface index"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return EX_USAGE;
                }


            // Requesting to list interfaces
            } else if(strncmp(argv[LOOP_COUNTER],"-l",2)==0) {
                list_interfaces();
                return EXIT_SUCCESS;


            // Specifying a custom frame payload size in bytes
            } else if(strncmp(argv[LOOP_COUNTER],"-f",2)==0) {
                if (argc>(LOOP_COUNTER+1))
                {
                    F_SIZE = atoi(argv[LOOP_COUNTER+1]);
                    if(F_SIZE > 1500)
                    {
                        cout << "WARNING: Make sure your device supports baby"
                             << " giants or jumbo frames as required"<<endl;
                    }
                    LOOP_COUNTER++;

                } else {
                    cout << "Oops! Missing frame size"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return EX_USAGE;
                }


            // Specifying a custom transmission duration in seconds
            } else if(strncmp(argv[LOOP_COUNTER],"-t",2)==0) {
                if (argc>(LOOP_COUNTER+1))
                {
                    F_DURATION = atoll(argv[LOOP_COUNTER+1]);
                    LOOP_COUNTER++;
                } else {
                    cout << "Oops! Missing transmission duration"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return EX_USAGE;
                }


            // Specifying the total number of frames to send instead of duration
            } else if(strncmp(argv[LOOP_COUNTER],"-c",2)==0) {
                if (argc>(LOOP_COUNTER+1))
                {
                    F_COUNT = atoll(argv[LOOP_COUNTER+1]);
                    LOOP_COUNTER++;
                } else {
                    cout << "Oops! Missing max frame count"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return EX_USAGE;
                }


            // Specifying the total number of bytes to send instead of duration
            } else if(strncmp(argv[LOOP_COUNTER],"-b",2)==0) {
                if (argc>(LOOP_COUNTER+1))
                {
                    F_BYTES = atoll(argv[LOOP_COUNTER+1]);
                    LOOP_COUNTER++;
                } else {
                    cout << "Oops! Missing max byte transfer limit"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return EX_USAGE;
                }


            // Enable ACK mode testing
            } else if(strncmp(argv[LOOP_COUNTER],"-a",2)==0) {
                F_ACK = true;


            // Limit TX rate to max bytes per second
            } else if(strncmp(argv[LOOP_COUNTER],"-m",2)==0) {
                if (argc>(LOOP_COUNTER+1))
                {
                    B_TX_SPEED_MAX = atol(argv[LOOP_COUNTER+1]);
                    LOOP_COUNTER++;
                } else {
                    cout << "Oops! Missing max TX rate"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return EX_USAGE;
                }


            // Limit TX rate to max bits per second
            } else if(strncmp(argv[LOOP_COUNTER],"-M",2)==0) {
                if (argc>(LOOP_COUNTER+1))
                {
                    B_TX_SPEED_MAX = atol(argv[LOOP_COUNTER+1]) / 8;
                    LOOP_COUNTER++;
                } else {
                    cout << "Oops! Missing max TX rate"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return EX_USAGE;
                }


            // Set 802.1p PCP value
            } else if(strncmp(argv[LOOP_COUNTER],"-p",2)==0) {
                if (argc>(LOOP_COUNTER+1))
                {
                    PCP = atoi(argv[LOOP_COUNTER+1]);
                    LOOP_COUNTER++;
                } else {
                    cout << "Oops! Missing 802.1p PCP value"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return EX_USAGE;
                }


            // Set 802.1q VLAN ID
            } else if(strncmp(argv[LOOP_COUNTER],"-v",2)==0) {
                if (argc>(LOOP_COUNTER+1))
                {
                    VLAN_ID = atoi(argv[LOOP_COUNTER+1]);
                    LOOP_COUNTER++;
                } else {
                    cout << "Oops! Missing 802.1p VLAN ID"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return EX_USAGE;
                }


            // Set 802.1ad QinQ outer VLAN ID
            } else if(strncmp(argv[LOOP_COUNTER],"-q",2)==0) {
                if (argc>(LOOP_COUNTER+1))
                {
                    QINQ_ID = atoi(argv[LOOP_COUNTER+1]);
                    LOOP_COUNTER++;
                } else {
                    cout << "Oops! Missing 802.1ad QinQ outer VLAN ID"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return EX_USAGE;
                }


            // Set 802.1ad QinQ outer PCP value
            } else if(strncmp(argv[LOOP_COUNTER],"-o",2)==0){
                if (argc>(LOOP_COUNTER+1))
                {
                    QINQ_PCP = atoi(argv[LOOP_COUNTER+1]);
                    LOOP_COUNTER++;
                } else {
                    cout << "Oops! Missing 802.1ad QinQ outer PCP value"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return EX_USAGE;
                }


            // Set a custom ETHERTYPE
            } else if(strncmp(argv[LOOP_COUNTER],"-e",2)==0){
                if (argc>(LOOP_COUNTER+1))
                {
                    ETHERTYPE = (int)strtoul(argv[LOOP_COUNTER+1], NULL, 16);
                    LOOP_COUNTER++;
                } else {
                    cout << "Oops! Missing ETHERTYPE value"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return EX_USAGE;
                }


            // Run an MTU sweep test
            } else if(strncmp(argv[LOOP_COUNTER],"-U",2)==0){
                if (argc>(LOOP_COUNTER+1))
                {
                    MTU_TX_MIN = atoi(argv[LOOP_COUNTER+1]);
                    MTU_TX_MAX = atoi(argv[LOOP_COUNTER+2]);
                    if(MTU_TX_MAX > F_SIZE_MAX) { 
                    	cout << "MTU size can not exceed the maximum hard"
                    	     << " coded frame size: "<<F_SIZE_MAX<<endl;
                    	     return EX_USAGE;
                    }
                    MTU_SWEEP_TEST = true;
                    LOOP_COUNTER+=2;
                } else {
                    cout << "Oops! Missing min/max MTU sizes"<<endl
                         << "Usage info: "<<argv[0]<<" -h"<<endl;
                    return EX_USAGE;
                }


            // Display version
            } else if(strncmp(argv[LOOP_COUNTER],"-V",2)==0 ||
                      strncmp(argv[LOOP_COUNTER],"--version",9)==0) {
                cout << "Etherate version "<<version<<endl;
                return EXIT_SUCCESS;


            // Display usage instructions
            } else if(strncmp(argv[LOOP_COUNTER],"-h",2)==0 ||
                      strncmp(argv[LOOP_COUNTER],"--help",6)==0) {
                print_usage();
                return EXIT_SUCCESS;
            }


        }

    }


    /*
     ***************************************************************** CLI ARGS
     */




    /*
     ************************************************************** TX/RX SETUP
     */


    // Check for root access, needed low level socket access we desire;
    if(getuid()!=0) {
        cout << "Must be root to use this program!"<<endl;
        return EX_NOPERM;
    }


    SOCKET_FD = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    /* Here we opened a socket with three arguments;
     * Communication Domain = AF_PACKET
     * Type/Communication Semantics = SOCK_RAW
     * Protocol = ETH_P_ALL == 0x0003, which is everything, from if_ether.h)
     */


    if (SOCKET_FD<0 )
    {
      cout << "Error defining socket."<<endl;
      perror("socket() ");
      close(SOCKET_FD);
      return EX_SOFTWARE;
    }


    // If the user has supplied an interface index try to use that
    if (IF_INDEX>0) {

        IF_INDEX = set_sock_interface_index(SOCKET_FD, IF_INDEX);
        if (IF_INDEX==0)
        {
            cout << "Error: Couldn't set interface with index, "
                 << "returned index was 0."<<endl;
            return EX_SOFTWARE;
        }

    // Or if the user has supplied an interface name try to use that        
    } else if(strcmp(IF_NAME,"")!=0) {

        IF_INDEX = set_sock_interface_name(SOCKET_FD, *IF_NAME);
        if (IF_INDEX==0)
        {
            cout << "Error: Couldn't set interface index from name, "
                 << "returned index was 0."<<endl;
            return EX_SOFTWARE;
        }

    // Try and guess the best guess an interface
    } else if (IF_INDEX==IF_INDEX_DEF) {

        IF_INDEX = get_sock_interface(SOCKET_FD);
        if (IF_INDEX==0)
        {
            cout << "Error: Couldn't find appropriate interface ID, returned ID "
                 << "was 0. This is typically the loopback interface ID."
                 << "Try supplying a source MAC address with the -s option."<<endl;
            return EX_SOFTWARE;
        }

    }


    // Link layer socket struct (socket_address)
    // RAW packet communication = PF_PACKET
    socket_address.sll_family   = PF_PACKET;	
    // We don't use a protocol above ethernet layer so use anything here
    socket_address.sll_protocol = htons(ETH_P_IP);
    // Index of the network device
    socket_address.sll_ifindex  = IF_INDEX;
    // sock_address.sll_ifindex = if_nametoindex(your_interface_name);
    // ARP hardware identifier is ethernet
    socket_address.sll_hatype   = ARPHRD_ETHER;
    // Target is another host
    socket_address.sll_pkttype  = PACKET_OTHERHOST;
    // Layer 2 address length
    socket_address.sll_halen    = ETH_ALEN;		
    // Destination MAC Address
    socket_address.sll_addr[0]  = DESTINATION_MAC[0];		
    socket_address.sll_addr[1]  = DESTINATION_MAC[1];
    socket_address.sll_addr[2]  = DESTINATION_MAC[2];
    socket_address.sll_addr[3]  = DESTINATION_MAC[3];
    socket_address.sll_addr[4]  = DESTINATION_MAC[4];
    socket_address.sll_addr[5]  = DESTINATION_MAC[5];
     // Last 2 bytes not used in Ethernet
    socket_address.sll_addr[6]  = 0x00;
    socket_address.sll_addr[7]  = 0x00;

    //  RX buffer for incoming ethernet frames
    char* RX_BUFFER = (char*)operator new(F_SIZE_MAX);

    //  TX buffer for outgoing ethernet frames
    TX_BUFFER = (char*)operator new(F_SIZE_MAX);

    TX_ETHERNET_HEADER = (unsigned char*)TX_BUFFER;

    build_headers(TX_BUFFER, DESTINATION_MAC, SOURCE_MAC, ETHERTYPE, PCP,
                   VLAN_ID, QINQ_ID, QINQ_PCP, ETH_HEADERS_LEN);

    // Userdata pointers in ethernet frames
    char* RX_DATA = RX_BUFFER + ETH_HEADERS_LEN;
    TX_DATA = TX_BUFFER + ETH_HEADERS_LEN;

    // 0 is success exit code for sending frames
    TX_REV_VAL = 0;

    // Length of the received frame
    int RX_LEN = 0;


    // http://www.linuxjournal.com/article/4659?page=0,1
    cout << "Entering promiscuous mode"<<endl;

    // Get the interface name
    strncpy(ethreq.ifr_name,IF_NAME,IFNAMSIZ);

    // Set the network card in promiscuos mode

    if (ioctl(SOCKET_FD,SIOCGIFFLAGS,&ethreq)==-1) 
    {
        cout << "Error getting socket flags, entering promiscuous mode failed."
             << endl;

        perror("ioctl() ");
        close(SOCKET_FD);
        return EX_SOFTWARE;
    }

    ethreq.ifr_flags|=IFF_PROMISC;

    if (ioctl(SOCKET_FD,SIOCSIFFLAGS,&ethreq)==-1)
    {
        cout << "Error setting socket flags, entering promiscuous mode failed."
             << endl;

        perror("ioctl() ");
        close(SOCKET_FD);
        return EX_SOFTWARE;
    }

    std::string param;
    std::stringstream ss;
    ss.setf(ios::fixed);
    ss.setf(ios::showpoint);
    ss.precision(9);
    cout<<fixed<<setprecision(9);

    // At this point, declare our sigint handler, from now on when the two hosts
    // start communicating it will be of use, the TX will signal RX to reset
    // when it dies
    signal (SIGINT,signal_handler);

    printf("Source MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           SOURCE_MAC[0],SOURCE_MAC[1],SOURCE_MAC[2],SOURCE_MAC[3],SOURCE_MAC[4],
           SOURCE_MAC[5]);

    printf("Destination MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           DESTINATION_MAC[0],DESTINATION_MAC[1],DESTINATION_MAC[2],
           DESTINATION_MAC[3],DESTINATION_MAC[4],DESTINATION_MAC[5]);

    /* 
     * Before any communication happens between the local host and remote host,
     * we must broadcast our whereabouts to ensure there is no initial loss of frames
     *
     * Set the dest MAC to broadcast (FF...FF) and build the frame like this,
     * then transmit three frames with a short brake between each.
     * Rebuild the frame headers with the actual dest MAC.
     */

    unsigned char BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    build_headers(TX_BUFFER, BROADCAST_MAC, SOURCE_MAC, ETHERTYPE, PCP, VLAN_ID,
                  QINQ_ID, QINQ_PCP, ETH_HEADERS_LEN);

    RX_DATA = RX_BUFFER + ETH_HEADERS_LEN;
    TX_DATA = TX_BUFFER + ETH_HEADERS_LEN;

    cout << "Sending gratuitous broadcasts..."<<endl<<endl;
    for(LOOP_COUNTER=1; LOOP_COUNTER<=3; LOOP_COUNTER++)
    {
        param = "etheratepresence";
        strncpy(TX_DATA,param.c_str(),param.length());

        TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER, param.length()+ETH_HEADERS_LEN, 0, 
                     (struct sockaddr*)&socket_address, sizeof(socket_address));

        sleep(1);
    }

    build_headers(TX_BUFFER, DESTINATION_MAC, SOURCE_MAC, ETHERTYPE, PCP, VLAN_ID,
                  QINQ_ID, QINQ_PCP, ETH_HEADERS_LEN);


    // Adjust for single or double dot1q tags
    if(ETH_HEADERS_LEN==18)
    {
        RX_DATA = RX_BUFFER + (ETH_HEADERS_LEN-4);
        TX_DATA = TX_BUFFER + ETH_HEADERS_LEN;
    // Linux is swallowing the outter tag of QinQ, see:
    // http://stackoverflow.com/questions/24355597/
    // linux-when-sending-ethernet-frames-the-ethertype-is-being-re-written
    } else if (ETH_HEADERS_LEN==22) {
        RX_DATA = RX_BUFFER + (ETH_HEADERS_LEN-4);
        TX_DATA = TX_BUFFER + ETH_HEADERS_LEN;
    } else {
        RX_DATA = RX_BUFFER + ETH_HEADERS_LEN;
        TX_DATA = TX_BUFFER + ETH_HEADERS_LEN;
    }

    F_SIZE_TOTAL = F_SIZE + ETH_HEADERS_LEN;


    /*
     ************************************************************** TX/RX SETUP
     */



    /*
     ************************************************************ SETTINGS SYNC
     */


    // Set up the test by communicating settings with the RX host receiver
    if(TX_MODE==true && TX_SYCN==true)
    { // If we are the TX host...

        cout << "Running in TX mode, synchronising settings"<<endl;

        // Testing with a custom frame size
        if(ETHERTYPE!=ETHERTYPE_DEF)
        {
            ss << "etherateethertype"<<ETHERTYPE;
            param = ss.str();
            strncpy(TX_DATA,param.c_str(),param.length());

            TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER, param.length()+ETH_HEADERS_LEN,
                                0,(struct sockaddr*)&socket_address,
                                sizeof(socket_address));

            cout << "ETHERTYPE set to "<<ETHERTYPE<<endl;
        }

        // Testing with a custom frame size
        if(F_SIZE!=F_SIZE_DEF)
        {
            ss << "etheratesize"<<F_SIZE;
            param = ss.str();
            strncpy(TX_DATA,param.c_str(),param.length());

            TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER, param.length()+ETH_HEADERS_LEN,
                                0, (struct sockaddr*)&socket_address,
                                sizeof(socket_address));

            cout << "Frame size set to "<<F_SIZE<<endl;
        }


        // Testing with a custom duration
        if(F_DURATION!=F_DURATION_DEF)
        {
            ss << "etherateduration"<<F_DURATION;
            param = ss.str();
            strncpy(TX_DATA,param.c_str(),param.length());

            TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER, param.length()+ETH_HEADERS_LEN,
                                0, (struct sockaddr*)&socket_address,
                                sizeof(socket_address));

            cout <<"Test duration set to "<<F_DURATION<<endl;
        }


        // Testing with a custom frame count
        if(F_COUNT!=F_COUNT_DEF)
        {
            ss << "etheratecount"<<F_COUNT;
            param = ss.str();
            strncpy(TX_DATA,param.c_str(),param.length());

            TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER, param.length()+ETH_HEADERS_LEN,
                                0, (struct sockaddr*)&socket_address,
                                sizeof(socket_address));

            cout <<"Frame count set to "<<F_COUNT<<endl;
        }


        // Testing with a custom byte limit
        if(F_BYTES!=F_BYTES_DEF)
        {
            ss << "etheratebytes"<<F_BYTES;
            param = ss.str();
            strncpy(TX_DATA,param.c_str(),param.length());

            TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER, param.length()+ETH_HEADERS_LEN,
                                0, (struct sockaddr*)&socket_address,
                                sizeof(socket_address));

            cout << "Byte limit set to "<<F_BYTES<<endl;
        }


        // Testing with a custom max speed limit
        if(B_TX_SPEED_MAX!=B_TX_SPEED_MAX_DEF)
        {
            ss << "etheratespeed"<<B_TX_SPEED_MAX;
            param = ss.str();
            strncpy(TX_DATA,param.c_str(),param.length());

            TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER, param.length()+ETH_HEADERS_LEN,
                                0, (struct sockaddr*)&socket_address,
                                sizeof(socket_address));

            cout << "Max TX speed set to "<<((B_TX_SPEED_MAX*8)/1000/1000)<<"Mbps"<<endl;
        }

        // Testing with a custom inner VLAN PCP value
        if(PCP!=PCP_DEF)
        {
            ss << "etheratepcp"<<PCP;
            param = ss.str();
            strncpy(TX_DATA,param.c_str(),param.length());

            TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER, param.length()+ETH_HEADERS_LEN, 
                                0, (struct sockaddr*)&socket_address,
                                sizeof(socket_address));

            cout << "Inner VLAN PCP value set to "<<PCP<<endl;
        }

        // Tesing with a custom QinQ PCP value
        if(PCP!=PCP_DEF)
        {
            ss << "etherateQINQ_PCP"<<QINQ_PCP;
            param = ss.str();
            strncpy(TX_DATA,param.c_str(),param.length());

            TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER, param.length()+ETH_HEADERS_LEN,
                                0, (struct sockaddr*)&socket_address,
                                sizeof(socket_address));

            cout << "QinQ VLAN PCP value set to "<<QINQ_PCP<<endl;
        }

        // Tell the receiver to run in ACK mode
        if(F_ACK==true)
        {
            param = "etherateack";
            strncpy(TX_DATA,param.c_str(),param.length());

            TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER, param.length()+ETH_HEADERS_LEN,
                                0, (struct sockaddr*)&socket_address,
                                sizeof(socket_address));

            cout << "ACK mode enabled"<<endl;
        }

        // Tell RX we will run an MTU sweep test
        if(MTU_SWEEP_TEST==true)
        {
            ss.str("");
            ss.clear();
            ss<<"etheratemtusweep:"<<MTU_TX_MIN<<":"<<MTU_TX_MAX<<":";
            param = ss.str();
            strncpy(TX_DATA,param.c_str(),param.length());

            TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER, param.length()+ETH_HEADERS_LEN,
                                0, (struct sockaddr*)&socket_address,
                                sizeof(socket_address));

            cout << "MTU sweep test enabled from "<<MTU_TX_MIN<<" to "<<MTU_TX_MAX<<endl;
        }


        // Send over the time for delay calculation between hosts,
        // We send it twice each time repeating this process multiple times to
        // get an average;
        cout << "Calculating delay between hosts..." << endl;
        for(LOOP_COUNTER=0; LOOP_COUNTER<=4; LOOP_COUNTER++)
        {
            // The monotonic value is used here in case we are unlucky enough to
            // run this during and NTP update, so we stay consistent
            clock_gettime(CLOCK_MONOTONIC_RAW, &TS_RTT);
            ss.str("");
            ss.clear();
            ss<<"etheratetime"<<LOOP_COUNTER<<"1:"<<TS_RTT.tv_sec<<":"<<TS_RTT.tv_nsec;
            param = ss.str();
            strncpy(TX_DATA,param.c_str(),param.length());

            TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER, param.length()+ETH_HEADERS_LEN,
                                0, (struct sockaddr*)&socket_address,
                                sizeof(socket_address));

            clock_gettime(CLOCK_MONOTONIC_RAW, &TS_RTT);
            ss.str("");
            ss.clear();
            ss<<"etheratetime"<<LOOP_COUNTER<<"2:"<<TS_RTT.tv_sec<<":"<<TS_RTT.tv_nsec;
            param = ss.str();
            strncpy(TX_DATA,param.c_str(),param.length());

            TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER, param.length()+ETH_HEADERS_LEN,
                                0, (struct sockaddr*)&socket_address,
                                sizeof(socket_address));

            // After sending the 2nd time value we wait for RX to return the
            // calculated delay value;
            bool TX_DELAY_WAITING = true;
            ss.str("");
            ss.clear();
            ss<<"etheratedelay.";
            param = ss.str();

            while (TX_DELAY_WAITING)
            {
                RX_LEN = recvfrom(SOCKET_FD, RX_BUFFER, F_SIZE_TOTAL, 0, NULL, NULL);

                if(param.compare(0,param.size(),RX_DATA,0,14)==0)
                {
                    exploded.clear();
                    explodestring = RX_DATA;
                    string_explode(explodestring, ".", &exploded);
                    ss.str("");
                    ss.clear();
                    ss << exploded[1].c_str()<<"."<<exploded[2].c_str();
                    param = ss.str();
                    DELAY_RESULTS[LOOP_COUNTER] = strtod(param.c_str(), NULL);
                    TX_DELAY_WAITING = false;
                }

            }

        }

        cout << "Tx to Rx delay calculated as "<<((DELAY_RESULTS[0]+DELAY_RESULTS[1]+
                 DELAY_RESULTS[2]+DELAY_RESULTS[3]+DELAY_RESULTS[4])/5)*1000
                 <<"ms"<<endl;

        // Let the receiver know all settings have been sent
        param = "etherateallset";
        strncpy(TX_DATA,param.c_str(),param.length());

        TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER, param.length()+ETH_HEADERS_LEN,
                            0, (struct sockaddr*)&socket_address,
                            sizeof(socket_address));

        cout << "Settings have been synchronised."<<endl<<endl;


    } else if (TX_MODE==false && TX_SYCN==true) {

        cout << "Running in RX mode, awaiting TX host"<<endl;
        // In listening mode we start by waiting for each parameter to come through
        // So we start a loop until they have all been received
        bool SYNC_WAITING = true;

        // Used for grabing sent values after the starting string in the received buffer
        int diff;

        // These values are used to calculate the delay between TX and RX hosts
        double timeTX1,timeRX1,timeTX2,timeRX2,timeTXdiff,timeRXdiff,timeTXdelay;

        // In this loop we are grabbing each incoming frame and looking at the 
        // string that prefixes it, to state which variable the following value
        // will be for
        while(SYNC_WAITING)
        {

            RX_LEN = recvfrom(SOCKET_FD, RX_BUFFER, F_SIZE_TOTAL, 0, NULL, NULL);

            // TX has sent a non-default ethertype
            if(strncmp(RX_DATA,"etherateethertype",17)==0)
            {
                diff = (RX_LEN-17);
                ss << RX_DATA;
                param = ss.str().substr(17,diff);
                ETHERTYPE = strtol(param.c_str(),NULL,10);
                printf("ETHERTYPE set to 0x%X\n", ETHERTYPE);

            }


            // TX has sent a non-default frame payload size
            if(strncmp(RX_DATA,"etheratesize",12)==0)
            {
                diff = (RX_LEN-12);
                ss << RX_DATA;
                param = ss.str().substr(12,diff);
                F_SIZE = strtol(param.c_str(),NULL,10);
                cout << "Frame size set to " << F_SIZE << endl;
            }


            // TX has sent a non-default transmition duration
            if(strncmp(RX_DATA,"etherateduration",16)==0)
            {
                diff = (RX_LEN-16);
                ss << RX_DATA;
                param = ss.str().substr(16,diff);
                F_DURATION = strtoull(param.c_str(),0,10);
                cout << "Test duration set to " << F_DURATION << endl;
            }


            // TX has sent a frame count to use instead of duration
            if(strncmp(RX_DATA,"etheratecount",13)==0)
            {
                diff = (RX_LEN-13);
                ss << RX_DATA;
                param = ss.str().substr(13,diff);
                F_COUNT = strtoull(param.c_str(),0,10);
                cout << "Frame count set to " << F_COUNT << endl;
            }


            // TX has sent a total bytes value to use instead of frame count
            if(strncmp(RX_DATA,"etheratebytes",13)==0)
            {
                diff = (RX_LEN-13);
                ss << RX_DATA;
                param = ss.str().substr(13,diff);
                F_BYTES = strtoull(param.c_str(),0,10);
                cout << "Byte limit set to " << F_BYTES << endl;
            }

            // TX has set a custom PCP value
            if(strncmp(RX_DATA,"etheratepcp",11)==0)
            {
                diff = (RX_LEN-11);
                ss << RX_DATA;
                param = ss.str().substr(11,diff);
                PCP = strtoull(param.c_str(),0,10);
                cout << "PCP value set to " << PCP << endl;
            }

            // TX has set a custom PCP value
            if(strncmp(RX_DATA,"etherateqinqpcp",15)==0)
            {
                diff = (RX_LEN-15);
                ss << RX_DATA;
                param = ss.str().substr(15,diff);
                QINQ_PCP = strtoull(param.c_str(),0,10);
                cout << "QinQ PCP value set to "<<QINQ_PCP<<endl;
            }

            // TX has requested RX run in ACK mode
            if(strncmp(RX_DATA,"etherateack",11)==0)
            {
                F_ACK = true;
                cout << "ACK mode enabled" << endl;
            }

            // Tx has requested MTU sweep test
            if(strncmp(RX_DATA,"etheratemtusweep",16)==0)
            {
                exploded.clear();
                explodestring = RX_DATA;
                string_explode(explodestring, ":", &exploded);
                MTU_TX_MIN = atoi(exploded[1].c_str());
                MTU_TX_MAX = atoi(exploded[2].c_str());
                MTU_SWEEP_TEST = true;
                cout << "MTU sweep test enabled from "<<MTU_TX_MIN<<" to "
                     <<  MTU_TX_MAX<<endl;
            }

            // Now begin the part to calculate the delay between TX and RX hosts.
            // Several time values will be exchanged to estimate the delay between
            // the two. Then the process is repeated two more times so and average
            // can be taken, which is shared back with TX;
            if( strncmp(RX_DATA,"etheratetime01:",15)==0 ||
                strncmp(RX_DATA,"etheratetime11:",15)==0 ||
                strncmp(RX_DATA,"etheratetime21:",15)==0 ||
                strncmp(RX_DATA,"etheratetime31:",15)==0 ||
                strncmp(RX_DATA,"etheratetime41:",15)==0  )
            {

                // Get the time we are receiving TX's sent time figure
                clock_gettime(CLOCK_MONOTONIC_RAW, &TS_RTT);
                ss.str("");
                ss.clear();
                ss << TS_RTT.tv_sec<<"."<<TS_RTT.tv_nsec;
                ss >> timeRX1;

                // Extract the sent time
                exploded.clear();
                explodestring = RX_DATA;
                string_explode(explodestring, ":", &exploded);

                ss.str("");
                ss.clear();
                ss << atol(exploded[1].c_str())<<"."<<atol(exploded[2].c_str());
                ss >> timeTX1;

            }


            if( strncmp(RX_DATA,"etheratetime02:",15)==0 ||
                strncmp(RX_DATA,"etheratetime12:",15)==0 ||
                strncmp(RX_DATA,"etheratetime22:",15)==0 ||
                strncmp(RX_DATA,"etheratetime32:",15)==0 ||
                strncmp(RX_DATA,"etheratetime42:",15)==0    )
            {

                // Get the time we are receiving TXs 2nd sent time figure
                clock_gettime(CLOCK_MONOTONIC_RAW, &TS_RTT);
                ss.str("");
                ss.clear();
                ss << TS_RTT.tv_sec<<"."<<TS_RTT.tv_nsec;
                ss >> timeRX2;

                // Extract the sent time
                exploded.clear();
                explodestring = RX_DATA;
                string_explode(explodestring, ":", &exploded);

                ss.clear();
                ss.str("");
                ss << atol(exploded[1].c_str())<<"."<<atol(exploded[2].c_str());
                ss >> timeTX2;

                // Calculate the delay
                timeTXdiff = timeTX2-timeTX1;
                timeTXdelay = (timeRX2-timeTXdiff)-timeRX1;

                if(strncmp(RX_DATA,"etheratetime02:",15)==0) DELAY_RESULTS[0] = timeTXdelay;
                if(strncmp(RX_DATA,"etheratetime12:",15)==0) DELAY_RESULTS[1] = timeTXdelay;
                if(strncmp(RX_DATA,"etheratetime22:",15)==0) DELAY_RESULTS[2] = timeTXdelay;
                if(strncmp(RX_DATA,"etheratetime32:",15)==0) DELAY_RESULTS[3] = timeTXdelay;
                if(strncmp(RX_DATA,"etheratetime42:",15)==0) 
                { 
                    DELAY_RESULTS[4] = timeTXdelay;
                    cout << "Tx to Rx delay calculated as "<<((DELAY_RESULTS[0]+
                         DELAY_RESULTS[1]+DELAY_RESULTS[2]+DELAY_RESULTS[3]+
                         DELAY_RESULTS[4])/5)*1000<<"ms"<<endl;
                }

                // Send it back to the TX host
                ss.clear();
                ss.str("");
                ss << "etheratedelay."<<timeTXdelay;
                param = ss.str();
                strncpy(TX_DATA,param.c_str(), param.length());
                TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER,
                                    param.length()+ETH_HEADERS_LEN, 0, 
                                    (struct sockaddr*)&socket_address,
                                    sizeof(socket_address));
            }

            // All settings have been received
            if(strncmp(RX_DATA,"etherateallset",14)==0)
            {
                SYNC_WAITING = false;
                cout<<"Settings have been synchronised"<<endl<<endl;
            }

        } // SYNC_WAITING bool

        // Rebuild the test frame headers in case any settings have been changed
        // by the TX host
        build_headers(TX_BUFFER, DESTINATION_MAC, SOURCE_MAC, ETHERTYPE, PCP, VLAN_ID,
                      QINQ_ID, QINQ_PCP, ETH_HEADERS_LEN);
      
    } // TX or RX mode


    /*
     ************************************************************ SETTINGS SYNC
     */




    /*
     ************************************************************* SINGLE TESTS
     */

    // Run an max MTU sweep test from TX to RX
    if (MTU_SWEEP_TEST) {

        cout << "Starting MTU sweep from "<<MTU_TX_MIN<<" to "<<MTU_TX_MAX<<endl;

        FD_ZERO(&FD_READS);
        SOCKET_FD_COUNT = SOCKET_FD + 1;

        if(TX_MODE) {

            // Fill the test frame with some junk data
            int junk = 0;
            for (junk = 0; junk < MTU_TX_MAX; junk++)
            {
                TX_DATA[junk] = (char)((int) 65); // ASCII 65 = A;
            }

            int MTU_TX_CURRENT=0;
            int MTU_ACK_PREVIOUS=0;
            int MTU_ACK_CURRENT=0;
            int MTU_ACK_LARGEST=0;

            for(MTU_TX_CURRENT=MTU_TX_MIN;MTU_TX_CURRENT<=MTU_TX_MAX;MTU_TX_CURRENT++)
            {

                // Send each MTU test frame 3 times
                for(LOOP_COUNTER=1; LOOP_COUNTER<=3; LOOP_COUNTER++)
                {

                    ss.str("");
                    ss.clear();
                    ss<<"etheratemtu:"<<MTU_TX_CURRENT<<":";
                    param = ss.str();

                    strncpy(TX_DATA,param.c_str(),param.length());

                    TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER,
                                        ETH_HEADERS_LEN+MTU_TX_CURRENT, 0,
                                        (struct sockaddr*)&socket_address,
                                        sizeof(socket_address));


                    // Check for largest ACK from RX host
                    TV_SELECT_DELAY.tv_sec = 0;
                    TV_SELECT_DELAY.tv_usec = 000000;
                    FD_SET(SOCKET_FD, &FD_READS);

                    SELECT_RET_VAL = select(SOCKET_FD_COUNT, &FD_READS, NULL,
                                            NULL, &TV_SELECT_DELAY);

                    if (SELECT_RET_VAL > 0) {
                        if (FD_ISSET(SOCKET_FD, &FD_READS)) {

                            RX_LEN = recvfrom(SOCKET_FD, RX_BUFFER, MTU_TX_MAX,
                                              0, NULL, NULL);

                            param = "etheratemtuack";

                            if(strncmp(RX_DATA,param.c_str(),param.length())==0)
                            {

                                // Get the MTU size RX is ACK'ing
                                exploded.clear();
                                explodestring = RX_DATA;
                                string_explode(explodestring, ":", &exploded);
                                MTU_ACK_CURRENT = atoi(exploded[1].c_str());

                                if (MTU_ACK_CURRENT>MTU_ACK_PREVIOUS) 
                                {
                                    MTU_ACK_LARGEST = MTU_ACK_CURRENT;
                                }

                                MTU_ACK_PREVIOUS = MTU_ACK_CURRENT;

                                cout << MTU_ACK_CURRENT << " " << MTU_ACK_PREVIOUS << " " << MTU_ACK_LARGEST << endl;

                            }

                        }
                    } // End of SELECT_RET_VAL

                } // End of 3 frame retry
                
            } // End of TX transmit


            // Wait now for the final ACK from RX confirming the largest MTU received
            bool MTU_TX_WAITING = true;

            // Only wait 5 seconds for this
            clock_gettime(CLOCK_MONOTONIC_RAW, &TS_ELAPSED_TIME);

            while(MTU_TX_WAITING)
            {

                // Get the current time
                clock_gettime(CLOCK_MONOTONIC_RAW, &TS_CURRENT_TIME);

                // 5 seconds have passed so we have missed/lost it
                if((TS_CURRENT_TIME.tv_sec-TS_ELAPSED_TIME.tv_sec)>=5)
                {
                    cout << "No final MTU ACK from RX received"<<endl;
                    MTU_TX_WAITING = false;
                }

                TV_SELECT_DELAY.tv_sec = 0;
                TV_SELECT_DELAY.tv_usec = 000000;
                FD_SET(SOCKET_FD, &FD_READS);

                SELECT_RET_VAL = select(SOCKET_FD_COUNT, &FD_READS, NULL,
                                        NULL, &TV_SELECT_DELAY);

                if (SELECT_RET_VAL > 0) {
                    if (FD_ISSET(SOCKET_FD, &FD_READS)) {

                        RX_LEN = recvfrom(SOCKET_FD, RX_BUFFER, MTU_TX_MAX, 0,
                                          NULL, NULL);

                        param = "etheratemtuack";

                        if(strncmp(RX_DATA,param.c_str(),param.length())==0)
                        {

                            // Get the MTU size RX is ACK'ing
                            exploded.clear();
                            explodestring = RX_DATA;
                            string_explode(explodestring, ":", &exploded);
                            MTU_ACK_CURRENT = atoi(exploded[1].c_str());

                            if (MTU_ACK_CURRENT>MTU_ACK_LARGEST)
                                MTU_ACK_LARGEST = MTU_ACK_CURRENT;

                            if (MTU_ACK_LARGEST==MTU_TX_MAX)
                                MTU_TX_WAITING = false;

                        }

                    }
                } // End of SELECT_RET_VAL

            } // End of MTU TX WAITING

            cout << "Largest MTU ACK'ed from RX is "<<MTU_ACK_LARGEST<<endl<<endl;


        } else { // Running in RX mode

            int MTU_RX_PREVIOUS=0;
            int MTU_RX_CURRENT=0;
            int MTU_RX_LARGEST=0;

            bool MTU_RX_WAITING = true;


            // Set up some counters such that if we go 3 seconds without receiving
            // a frame, end the test (assume MTU has been exceeded)
            bool MTU_RX_ANY=false;
            clock_gettime(CLOCK_MONOTONIC_RAW, &TS_ELAPSED_TIME);
            while (MTU_RX_WAITING)
            {
                // Check for largest ACK from RX host
                TV_SELECT_DELAY.tv_sec = 0;
                TV_SELECT_DELAY.tv_usec = 000000;
                FD_SET(SOCKET_FD, &FD_READS);

                SELECT_RET_VAL = select(SOCKET_FD_COUNT, &FD_READS, NULL, NULL,
                                        &TV_SELECT_DELAY);

                if (SELECT_RET_VAL > 0) {
                    if (FD_ISSET(SOCKET_FD, &FD_READS)) {

                        RX_LEN = recvfrom(SOCKET_FD, RX_BUFFER, MTU_TX_MAX, 0,
                                          NULL, NULL);

                        param = "etheratemtu";

                        if(strncmp(RX_DATA,param.c_str(),param.length())==0)
                        {


                            MTU_RX_ANY = true;

                            // Get the MTU size TX is sending
                            exploded.clear();
                            explodestring = RX_DATA;
                            string_explode(explodestring, ":", &exploded);
                            MTU_RX_CURRENT = atoi(exploded[1].c_str());

                            if (MTU_RX_CURRENT>MTU_RX_PREVIOUS)
                            {

                                MTU_RX_LARGEST = MTU_RX_CURRENT;

                                // ACK that back to TX as new largest MTU received
                                ss.str("");
                                ss.clear();
                                ss<<"etheratemtuack:"<<MTU_RX_LARGEST<<":";
                                param = ss.str();

                                strncpy(TX_DATA,param.c_str(),param.length());

                                TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER,
                                                    ETH_HEADERS_LEN+param.length(), 0,
                                                    (struct sockaddr*)&socket_address,
                                                    sizeof(socket_address));


                            }

                            MTU_RX_PREVIOUS = MTU_RX_CURRENT;

                        }

                    }
                } // End of SELECT_RET_VAL

                // If we have received the largest MTU TX was hoping to send,
                // the MTU sweep test is over
                if(MTU_RX_LARGEST==MTU_TX_MAX)
                {

                    // Signal back to TX the largest MTU we recieved at the end
                    ss.str("");
                    ss.clear();
                    ss<<"etheratemtuackfinal:"<<MTU_RX_LARGEST<<":";
                    param = ss.str();

                    strncpy(TX_DATA,param.c_str(),param.length());

                    TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER,
                                        ETH_HEADERS_LEN+param.length(), 0,
                                        (struct sockaddr*)&socket_address,
                                        sizeof(socket_address));

                    MTU_RX_WAITING = false;

                    cout << "MTU sweep test complete"<<endl
                         << "Largest MTU received was "<<MTU_RX_LARGEST<<endl
                         << endl;

                }

                // Get the current time
                clock_gettime(CLOCK_MONOTONIC_RAW, &TS_CURRENT_TIME);

                // 3 seconds have passed without any sweep frames being receeved
                if((TS_CURRENT_TIME.tv_sec-TS_ELAPSED_TIME.tv_sec)>=3)
                {

                    if(MTU_RX_ANY==false)
                    {
                        cout << "Largest MTU received is less than sweep max, "
                             << "ending the sweep"<<endl<<"Largest MTU received "
                             << MTU_RX_LARGEST<<endl<<endl;

                        MTU_RX_WAITING = false;
                    } else {
                        clock_gettime(CLOCK_MONOTONIC_RAW, &TS_ELAPSED_TIME);
                        MTU_RX_ANY = false;
                    }

                }


            } // End of RX MTU WAITING


        } // End of TX/RX mode


    } // End of MTU sweep test


    // Ethernet ping goes here

    /*
     ************************************************************* SINGLE TESTS
     */



    /*
     ********************************************************** MAIN TEST PHASE
     */

    

    // Fill the test frame with some junk data
    int junk = 0;
    for (junk = 0; junk < F_SIZE; junk++)
    {
        TX_DATA[junk] = (char)((int) 65); // ASCII 65 = A;
        //(255.0*rand()/(RAND_MAX+1.0)));
    }


    TS_NOW = time(0);
    TM_LOCAL = localtime(&TS_NOW);
    cout << endl<<"Starting test on "<<asctime(TM_LOCAL)<<endl;
    ss.precision(2);
    cout << fixed << setprecision(2);

    FD_ZERO(&FD_READS);
    SOCKET_FD_COUNT = SOCKET_FD + 1;

    if (TX_MODE==true)
    {

        cout << "Seconds\t\tMbps TX\t\tMBs Tx\t\tFrmTX/s\t\tFrames TX"<<endl;

        long long *testBase, *testMax;
        if (F_BYTES>0)
        {
            // We are testing until X bytes received
            testMax = &F_BYTES;
            testBase = &B_TX;

        } else if (F_COUNT>0) {
            // We are testing until X frames received
            testMax = &F_COUNT;
            testBase = &F_TX_COUNT;

        } else if (F_DURATION>0) {
            // We are testing until X seconds has passed
            F_DURATION--;
            testMax = &F_DURATION;
            testBase = &S_ELAPSED;
        }


        // Get clock time for the speed limit option,
        // get another to record the initial starting time
        clock_gettime(CLOCK_MONOTONIC_RAW, &TS_TX_LIMIT);
        clock_gettime(CLOCK_MONOTONIC_RAW, &TS_ELAPSED_TIME);

        // Main TX loop
        while (*testBase<=*testMax)
        {

            // Get the current time
            clock_gettime(CLOCK_MONOTONIC_RAW, &TS_CURRENT_TIME);

            // One second has passed
            if((TS_CURRENT_TIME.tv_sec-TS_ELAPSED_TIME.tv_sec)>=1)
            {
                S_ELAPSED++;
                B_SPEED = (((float)B_TX-(float)B_TX_PREV)*8)/1000/1000;
                B_TX_PREV = B_TX;

                cout << S_ELAPSED<<"\t\t"<<B_SPEED<<"\t\t"<<(B_TX/1000)/1000
                     << "\t\t"<<(F_TX_COUNT-F_TX_COUNT_PREV)<<"\t\t"<<F_TX_COUNT
                     << endl;

                F_TX_COUNT_PREV = F_TX_COUNT;
                TS_ELAPSED_TIME.tv_sec = TS_CURRENT_TIME.tv_sec;
                TS_ELAPSED_TIME.tv_nsec = TS_CURRENT_TIME.tv_nsec;
            }

            // Poll the socket file descriptor with select() for incoming frames
            TV_SELECT_DELAY.tv_sec = 0;
            TV_SELECT_DELAY.tv_usec = 000000;
            FD_SET(SOCKET_FD, &FD_READS);
            SELECT_RET_VAL = select(SOCKET_FD_COUNT, &FD_READS, NULL, NULL, &TV_SELECT_DELAY);

            if (SELECT_RET_VAL > 0) {
                if (FD_ISSET(SOCKET_FD, &FD_READS)) {

                    RX_LEN = recvfrom(SOCKET_FD, RX_BUFFER, F_SIZE_TOTAL, 0, NULL, NULL);

                    if(F_ACK)
                    {
                        param = "etherateack";

                        if(strncmp(RX_DATA,param.c_str(),param.length())==0)
                        {
                            F_RX_COUNT++;
                            F_WAITING_ACK = false;

                        } else {

                            // Check if RX host has sent a dying gasp
                            param = "etheratedeath";

                            if(strncmp(RX_DATA,param.c_str(),param.length())==0)
                            {
                                TS_NOW = time(0);
                                TM_LOCAL = localtime(&TS_NOW);
                                cout << "RX host is going down."<<endl
                                     << "Ending test and resetting on "
                                     << asctime(TM_LOCAL)<<endl;
                                goto finish;

                            } else {
                                F_RX_OTHER++;
                            }

                        }

                    } else {
                        
                        // Check if RX host has sent a dying gasp
                        param = "etheratedeath";
                        if(strncmp(RX_DATA,param.c_str(),param.length())==0)
                        {
                            TS_NOW = time(0);
                            TM_LOCAL = localtime(&TS_NOW);
                            cout << "RX host is going down."<<endl
                                 << "Ending test and resetting on "
                                 << asctime(TM_LOCAL)<<endl;
                            goto finish;

                        } else {
                            F_RX_OTHER++;
                        }
                        
                    }
                    
                }
            }

            // If it hasn't been 1 second yet
            if (TS_CURRENT_TIME.tv_sec-TS_TX_LIMIT.tv_sec<1)
            {

                if(!F_WAITING_ACK) {

                    // A max speed has been set
                    if(B_TX_SPEED_MAX!=B_TX_SPEED_MAX_DEF)
                    {

                        // Check if sending another frame keeps us under the
                        // max speed limit
                        if((B_TX_SPEED_PREV+F_SIZE)<=B_TX_SPEED_MAX)
                        {

                            ss.clear();
                            ss.str("");
                            ss << "etheratetest:"<<(F_TX_COUNT+1)<<":";
                            param = ss.str();
                            strncpy(TX_DATA,param.c_str(), param.length());

                            TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER, F_SIZE_TOTAL, 0, 
                       	                        (struct sockaddr*)&socket_address,
                                                sizeof(socket_address));

                            F_TX_COUNT++;
                            B_TX+=F_SIZE;
                            B_TX_SPEED_PREV+=F_SIZE;
                            if (F_ACK) F_WAITING_ACK = true;

                        }

                    } else {

                        ss.clear();
                        ss.str("");
                        ss << "etheratetest:" << (F_TX_COUNT+1) <<  ":";
                        param = ss.str();
                        strncpy(TX_DATA,param.c_str(), param.length());

                        TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER, F_SIZE_TOTAL, 0, 
                                            (struct sockaddr*)&socket_address,
                                            sizeof(socket_address));

                        F_TX_COUNT+=1;
                        B_TX+=F_SIZE;
                        B_TX_SPEED_PREV+=F_SIZE;
                        if (F_ACK) F_WAITING_ACK = true;
                    }

                }

            } else { // 1 second has passed

              B_TX_SPEED_PREV = 0;
              clock_gettime(CLOCK_MONOTONIC_RAW, &TS_TX_LIMIT);

            } // End of TS_TX_LIMIT.tv_sec<1

        }

        cout << "Test frames transmitted: "<<F_TX_COUNT<<endl
             << "Test frames received: "<<F_RX_COUNT<<endl
             << "Non test frames received: "<<F_RX_OTHER<<endl;

        TS_NOW = time(0);
        TM_LOCAL = localtime(&TS_NOW);
        cout << endl << "Ending test on " << asctime(TM_LOCAL) << endl;


    // Else, we are in RX mode
    } else {

        cout << "Seconds\t\tMbps RX\t\tMBs Rx\t\tFrmRX/s\t\tFrames RX"<<endl;

        long long *testBase, *testMax;

        // Are we testing until X bytes received
        if (F_BYTES>0)
        {
            testMax = &F_BYTES;
            testBase = &B_RX;

        // Or are we testing until X frames received
        } else if (F_COUNT>0) {
            testMax = &F_COUNT;
            testBase = &F_RX_COUNT;

        // Or are we testing until X seconds has passed
        } else if (F_DURATION>0) {
            F_DURATION--;
            testMax = &F_DURATION;
            testBase = &S_ELAPSED;
        }

        // Get the initial starting time
        clock_gettime(CLOCK_MONOTONIC_RAW, &TS_ELAPSED_TIME);

        // Main RX loop
        while (*testBase<=*testMax)
        {

            clock_gettime(CLOCK_MONOTONIC_RAW, &TS_CURRENT_TIME);
            // If one second has passed
            if((TS_CURRENT_TIME.tv_sec-TS_ELAPSED_TIME.tv_sec)>=1)
            {
                S_ELAPSED++;
                B_SPEED = float (((float)B_RX-(float)B_RX_PREV)*8)/1000/1000;
                B_RX_PREV = B_RX;

                cout << S_ELAPSED<<"\t\t"<<B_SPEED<<"\t\t"<<(B_RX/1000)/1000
                     << "\t\t"<<(F_RX_COUNT-F_RX_COUNT_PREV)<<"\t\t"<<F_RX_COUNT
                     << endl;

                F_RX_COUNT_PREV = F_RX_COUNT;
                TS_ELAPSED_TIME.tv_sec = TS_CURRENT_TIME.tv_sec;
                TS_ELAPSED_TIME.tv_nsec = TS_CURRENT_TIME.tv_nsec;
            }

            // Poll the socket file descriptor with select() to 
            // check for incoming frames
            TV_SELECT_DELAY.tv_sec = 0;
            TV_SELECT_DELAY.tv_usec = 000000;
            FD_SET(SOCKET_FD, &FD_READS);

            SELECT_RET_VAL = select(SOCKET_FD_COUNT, &FD_READS, NULL, NULL,
                                    &TV_SELECT_DELAY);

            if (SELECT_RET_VAL > 0) {
                if (FD_ISSET(SOCKET_FD, &FD_READS))
                {

                    RX_LEN = recvfrom(SOCKET_FD, RX_BUFFER, F_SIZE_TOTAL, 0,
                                      NULL, NULL);

                    // Check if this is an etherate test frame
                    param = "etheratetest";

                    if(strncmp(RX_DATA,param.c_str(),param.length())==0)
                    {

                        // Update test stats
                        F_RX_COUNT++;
                        B_RX+=(RX_LEN-ETH_HEADERS_LEN);

                        // Get the index of the received frame
                        exploded.clear();
                        explodestring = RX_DATA;
                        string_explode(explodestring, ":", &exploded);
                        F_INDEX = atoi(exploded[1].c_str());


                        if(F_INDEX==(F_RX_COUNT) || F_INDEX==(F_INDEX_PREV+1))
                        {
                            F_RX_ONTIME++;
                            F_INDEX_PREV++;
                        } else if (F_INDEX>(F_RX_COUNT)) {
                            F_INDEX_PREV = F_INDEX;
                            F_RX_EARLY++;
                        } else if (F_INDEX<F_RX_COUNT) {
                            F_RX_LATE++;
                        }


                        // If running in ACK mode we need to ACK to TX host
                        if(F_ACK)
                        {

                            ss.clear();
                            ss.str("");
                            ss << "etherateack"<<F_RX_COUNT;
                            param = ss.str();
                            strncpy(TX_DATA,param.c_str(), param.length());

                            TX_REV_VAL = sendto(SOCKET_FD, TX_BUFFER, F_SIZE_TOTAL, 0, 
                                         (struct sockaddr*)&socket_address,
                                         sizeof(socket_address));

                            F_TX_COUNT++;
                            
                        }

                    } else {

                        // We received a non-test frame
                        F_RX_OTHER++;

                    }

                    // Check if TX host has quit/died;
                    param = "etheratedeath";
                    if(strncmp(RX_DATA,param.c_str(),param.length())==0)
                    {
                        TS_NOW = time(0);
                        TM_LOCAL = localtime(&TS_NOW);

                        cout << "TX host is going down."<<endl
                             << "Ending test and resetting on "<<asctime(TM_LOCAL)
                             << endl;

                        goto restart;
                    }
                      
                }
            }

        }

        cout << "Test frames transmitted: "<<F_TX_COUNT<<endl
             << "Test frames received: "<<F_RX_COUNT<<endl
             << "Non test frames received: "<<F_RX_OTHER<<endl
             << "In order test frames received: "<< F_RX_ONTIME<<endl
             << "Out of order test frames received early: "<< F_RX_EARLY<<endl
             << "Out of order frames received late: "<< F_RX_LATE<<endl;

        TS_NOW = time(0);
        TM_LOCAL = localtime(&TS_NOW);
        cout << endl<<"Ending test on "<<asctime(TM_LOCAL)<<endl;
        close(SOCKET_FD);
        goto restart;

    }


    finish:

    cout << "Leaving promiscuous mode"<<endl;

    strncpy(ethreq.ifr_name,IF_NAME,IFNAMSIZ);

    if (ioctl(SOCKET_FD,SIOCGIFFLAGS,&ethreq)==-1)
    {
        cout << "Error getting socket flags, entering promiscuous mode failed."<<endl;
        perror("ioctl() ");
        close(SOCKET_FD);
        return EX_SOFTWARE;
    }

    ethreq.ifr_flags &= ~IFF_PROMISC;

    if (ioctl(SOCKET_FD,SIOCSIFFLAGS,&ethreq)==-1)
    {
        cout << "Error setting socket flags, promiscuous mode failed."<<endl;
        perror("ioctl() ");
        close(SOCKET_FD);
        return EX_SOFTWARE;
    }

    close(SOCKET_FD);


    /*
     ********************************************************** MAIN TEST PHASE
     */


    return EXIT_SUCCESS;

} // End of main()


