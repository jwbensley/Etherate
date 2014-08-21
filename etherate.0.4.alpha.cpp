/*
 * Etherate Main Body
 *
 * License: First rule of license club...
 *
 * Updates: https://github.com/jwbensley/Etherate and http://null.53bits.co.uk
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
 * TEST PHASE
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
    bool txMode = true;

    // Interface index, default to -1 so we know later if the user changed it
    int ifIndex = ifIndexDefault;
 
    // Source MAC address - Default is in IANA unassigned range
    sourceMAC[0] = 0x00;
    sourceMAC[1] = 0x00;
    sourceMAC[2] = 0x5E;
    sourceMAC[3] = 0x00;
    sourceMAC[4] = 0x00;
    sourceMAC[5] = 0x01;

    // Destination MAC address - Default is in IANA unassigned range
    destMAC[0] = 0x00;
    destMAC[1] = 0x00;
    destMAC[2] = 0x5E;
    destMAC[3] = 0x00;
    destMAC[4] = 0x00;
    destMAC[5] = 0x02;

    // This is the length of a frame header by default
    headersLength = headersLengthDefault;

    // The ethertype for the Etherate payload
    ethertype = ethertypeDefault;

    // Default 802.1p PCP/CoS value = 0
    PCP = PCPDef;

    // Default 802.1q VLAN ID = 0
    vlanID = vlanIDDef;

    // Default 802.1ad VLAN ID of QinQ outer frame = 0
    qinqID = qinqIDDef;

    // Default 802.1p PCP/CoS value of outer frame = 0
    qinqPCP = qinqPCPDef;

    // Frame payload in bytes
    int fSize = fSizeDef;

    // Total frame size including headers
    int fSizeTotal = fSizeDef + headersLength;

    // Duration in seconds
    long long fDuration = fDurationDef;

    // Number of frames to send
    long long fCount = fCountDef; 

    // Amount of data to transmit in bytes
    long long fBytes = fBytesDef;

    // Speed to transmit at (Max bytes per second)
    long bTXSpeed = bTXSpeedDef;

    // How fast we were transmitting for the last second
    long bTXSpeedLast = 0;

    // Total number of frames transmitted
    long long fTX = 0;

    // Frames sent at last count for stats
    long long fTXlast = 0;

    // Total number of frames received
    long long fRX = 0;

    // Frames received at last count for stats
    long long fRXlast = 0;

    // Index of the current test frame sent/received;
    long long fIndex = 0;

    // Index of the last test frame sent/received;
    long long fIndexLast = 0;

    // Frames received out of order that are early
    long long fEarly = 0;

    // Frames received out of order that are late
    long long fLate = 0;

    // Number of non test frames received
    long fRXother = 0;

    // Total number of bytes transmitted
    long long bTX = 0;

    // Bytes sent at last count for calculating speed
    long long bTXlast = 0;

    // Total number of bytes received
    long long bRX = 0;

    // Bytes received at last count for calculating speed
    long long bRXlast = 0;

    // Max speed whilst running the test
    float bSpeed = 0;

    // Are we running in ACK mode during transmition
    bool fACK = false;

    // Are we waiting for an ACK frame
    bool fWaiting = false;

    // Timeout to wait for a frame ACK
    timespec tsACK;

    // These timespecs are used for calculating delay/RTT
    timespec tsRTT;

    // 5 doubles to calculate the delay 5 times, to get an average
    double delay[5];

    // Two timers for timing the test and calculating stats
    timespec tsCurrent, tsElapsed;

    // Seconds the test has been running
    long long sElapsed = 0;

    // Timer used for rate limiting the TX host
    timespec txLimit;

    // Time type for holding the current date and time
    time_t timeNow;

    // Time struct for breaking down the above time type
    tm* localtm;

    // Elapsed time struct for polling the socket file descriptor
    struct timeval tvSelectDelay;

    // A set of socket file descriptors for polling
    fd_set readfds;


    /* 
      These variables are declared here and used over and over throughout;

       The string explode function is used several times throughout,
       so rather than continuously defining variables, they are defined here 
    */
    vector<string> exploded;
    string explodestring;

    // Also, a generic loop counter
    int lCounter;


    /*
     ********************************************************* GLOBAL VARIABLES
     */




    /*
     ***************************************************************** CLI ARGS
     */


    if(argc>1) 
    {

        for (lCounter = 1; lCounter < argc; lCounter++) 
        {

            // Change to receive mode
            if(strncmp(argv[lCounter],"-r",2)==0) 
            {
                txMode = false;

                sourceMAC[0] = 0x00;
                sourceMAC[1] = 0x00;
                sourceMAC[2] = 0x5E;
                sourceMAC[3] = 0x00;
                sourceMAC[4] = 0x00;
                sourceMAC[5] = 0x02;

                destMAC[0] = 0x00;
                destMAC[1] = 0x00;
                destMAC[2] = 0x5E;
                destMAC[3] = 0x00;
                destMAC[4] = 0x00;
                destMAC[5] = 0x01;


            // Specifying a custom destination MAC address
            } else if(strncmp(argv[lCounter],"-d",2)==0) {
                explodestring = argv[lCounter+1];
                exploded.clear();
                string_explode(explodestring, ":", &exploded);

                if((int) exploded.size() != 6) 
                {
                    cout << "Error: Invalid destination MAC address!" << endl
                         << "Usage info: " << argv[0] << " -h" << endl;
                    return EX_USAGE;
                }
                cout << "Destination MAC ";

                // Here strtoul() is used to convert the interger value of c_str()
                // to a hex number by giving it the number base 16 (hexadecimal)
                // otherwise it would be atoi()
                for(int i = 0; (i < 6) && (i < exploded.size()); ++i)
                {
                    destMAC[i] = (unsigned char)strtoul(exploded[i].c_str(), NULL, 16);
                    cout << setw(2) << setfill('0') << hex << int(destMAC[i]) << ":";
                }
                cout << dec << endl;
                lCounter++;


            // Specifying a custom source MAC address
            } else if(strncmp(argv[lCounter],"-s",2)==0) {
                explodestring = argv[lCounter+1];
                exploded.clear();
                string_explode(explodestring, ":", &exploded);

                if((int) exploded.size() != 6) 
                {
                    cout << "Error: Invalid source MAC address!" << endl
                         << "Usage info: " << argv[0] << " -h" << endl;
                    return EX_USAGE;
                }
                cout << "Custom source MAC ";

                for(int i = 0; (i < 6) && (i < exploded.size()); ++i)
                {
                    sourceMAC[i] = (unsigned char)strtoul(exploded[i].c_str(), NULL, 16);
                    cout << setw(2) << setfill('0') << hex << int(sourceMAC[i]) << ":";
                }
                cout << dec << endl;
                lCounter++;

            // Specifying a custom interface name
            } else if(strncmp(argv[lCounter],"-i",2)==0) {
                if (argc>(lCounter+1))
                {
                    strncpy(ifName,argv[lCounter+1],sizeof(argv[lCounter+1]));
                    lCounter++;
                } else {
                    cout << "Oops! Missing interface name" << endl
                         << "Usage info: " << argv[0] << " -h" << endl;
                    return EX_USAGE;
                }


            // Specifying a custom interface index
            } else if(strncmp(argv[lCounter],"-I",2)==0) {
                if (argc>(lCounter+1))
                {
                    ifIndex = atoi(argv[lCounter+1]);
                    lCounter++;
                } else {
                    cout << "Oops! Missing interface index" << endl
                         << "Usage info: " << argv[0] << " -h" << endl;
                    return EX_USAGE;
                }


            // Requesting to list interfaces
            } else if(strncmp(argv[lCounter],"-l",2)==0) {
                list_interfaces();
                return EXIT_SUCCESS;


            // Specifying a custom frame payload size in bytes
            } else if(strncmp(argv[lCounter],"-f",2)==0) {
                if (argc>(lCounter+1))
                {
                    fSize = atoi(argv[lCounter+1]);
                    if(fSize > 1500)
                    {
                        cout << "WARNING: Make sure your device supports baby giant"
                             << "or jumbo frames as required" << endl;
                    }
                    lCounter++;

                } else {
                    cout << "Oops! Missing frame size" << endl
                         << "Usage info: " << argv[0] << " -h" << endl;
                    return EX_USAGE;
                }


            // Specifying a custom transmission duration in seconds
            } else if(strncmp(argv[lCounter],"-t",2)==0) {
                if (argc>(lCounter+1))
                {
                    fDuration = atoll(argv[lCounter+1]);
                    lCounter++;
                } else {
                    cout << "Oops! Missing transmission duration" << endl
                         << "Usage info: " << argv[0] << " -h" << endl;
                    return EX_USAGE;
                }


            // Specifying the total number of frames to send instead of duration
            } else if(strncmp(argv[lCounter],"-c",2)==0) {
                if (argc>(lCounter+1))
                {
                    fCount = atoll(argv[lCounter+1]);
                    lCounter++;
                } else {
                    cout << "Oops! Missing max frame count" << endl
                         << "Usage info: " << argv[0] << " -h" << endl;
                    return EX_USAGE;
                }


            // Specifying the total number of bytes to send instead of duration
            } else if(strncmp(argv[lCounter],"-b",2)==0) {
                if (argc>(lCounter+1))
                {
                    fBytes = atoll(argv[lCounter+1]);
                    lCounter++;
                } else {
                    cout << "Oops! Missing max byte transfer limit" << endl
                         << "Usage info: " << argv[0] << " -h" << endl;
                    return EX_USAGE;
                }


            // Enable ACK mode testing
            } else if(strncmp(argv[lCounter],"-a",2)==0) {
                fACK = true;


            // Limit TX rate to max mbps
            } else if(strncmp(argv[lCounter],"-m",2)==0) {
                if (argc>(lCounter+1))
                {
                    bTXSpeed = atol(argv[lCounter+1]);
                    lCounter++;
                } else {
                    cout << "Oops! Missing max TX rate" << endl
                         << "Usage info: " << argv[0] << " -h" << endl;
                    return EX_USAGE;
                }


            // Set 802.1p PCP value
            } else if(strncmp(argv[lCounter],"-p",2)==0) {
                if (argc>(lCounter+1))
                {
                    PCP = atoi(argv[lCounter+1]);
                    lCounter++;
                } else {
                    cout << "Oops! Missing 802.1p PCP value" << endl
                         << "Usage info: " << argv[0] << " -h" << endl;
                    return EX_USAGE;
                }


            // Set 802.1q VLAN ID
            } else if(strncmp(argv[lCounter],"-v",2)==0) {
                if (argc>(lCounter+1))
                {
                    vlanID = atoi(argv[lCounter+1]);
                    lCounter++;
                } else {
                    cout << "Oops! Missing 802.1p VLAN ID" << endl
                         << "Usage info: " << argv[0] << " -h" << endl;
                    return EX_USAGE;
                }


            // Set 802.1ad QinQ outer VLAN ID
            } else if(strncmp(argv[lCounter],"-q",2)==0) {
                if (argc>(lCounter+1))
                {
                    qinqID = atoi(argv[lCounter+1]);
                    lCounter++;
                } else {
                    cout << "Oops! Missing 802.1ad QinQ outer VLAN ID" << endl
                         << "Usage info: " << argv[0] << " -h" << endl;
                    return EX_USAGE;
                }


            // Set 802.1ad QinQ outer PCP value
            } else if(strncmp(argv[lCounter],"-o",2)==0){
                if (argc>(lCounter+1))
                {
                    qinqPCP = atoi(argv[lCounter+1]);
                    lCounter++;
                } else {
                    cout << "Oops! Missing 802.1ad QinQ outer PCP value" << endl
                         << "Usage info: " << argv[0] << " -h" << endl;
                    return EX_USAGE;
                }


            // Set a custom ethertype
            } else if(strncmp(argv[lCounter],"-e",2)==0){
                if (argc>(lCounter+1))
                {
                    ethertype = (int)strtoul(argv[lCounter+1], NULL, 16);
                    lCounter++;
                } else {
                    cout << "Oops! Missing ethertype value" << endl
                         << "Usage info: " << argv[0] << " -h" << endl;
                    return EX_USAGE;
                }


            // Display version
            } else if(strncmp(argv[lCounter],"-V",2)==0 ||
                      strncmp(argv[lCounter],"--version",9)==0) {
                cout << "Etherate version " << version << endl;
                return EXIT_SUCCESS;


            // Display usage instructions
            } else if(strncmp(argv[lCounter],"-h",2)==0 ||
                      strncmp(argv[lCounter],"--help",6)==0) {
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
        cout << "Must be root to use this program!" << endl;
        return EX_NOPERM;
    }


    sockFD = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    /* Here we opened a socket with three arguments;
     * Communication Domain = AF_PACKET
     * Type/Communication Semantics = SOCK_RAW
     * Protocol = ETH_P_ALL == 0x0003, which is everything, from if_ether.h)
     */


    if (sockFD<0 )
    {
      cout << "Error defining socket." << endl;
      perror("socket() ");
      close(sockFD);
      return EX_SOFTWARE;
    }


    // If the user has supplied an interface index try to use that
    if (ifIndex>0) {

        ifIndex = set_sock_interface_index(sockFD, ifIndex);
        if (ifIndex==0)
        {
            cout << "Error: Couldn't set interface with index, returned index was 0." << endl;
            return EX_SOFTWARE;
        }

    // Or if the user has supplied an interface name try to use that        
    } else if(strcmp(ifName,"")!=0) {

        ifIndex = set_sock_interface_name(sockFD, *ifName);
        if (ifIndex==0)
        {
            cout << "Error: Couldn't set interface index from name, returned index was 0." << endl;
            return EX_SOFTWARE;
        }

    // Try and guess the best guess an interface
    } else if (ifIndex==ifIndexDefault) {

        ifIndex = get_sock_interface(sockFD);
        if (ifIndex==0)
        {
            cout << "Error: Couldn't find appropriate interface ID, returned ID was 0."
                 << "This is typically the loopback interface ID."
                 << "Try supplying a source MAC address with the -s option." << endl;
            return EX_SOFTWARE;
        }

    }


    // Link layer socket struct (socket_address)
    // RAW packet communication = PF_PACKET
    socket_address.sll_family   = PF_PACKET;	
    // We don't use a protocol above ethernet layer so use anything here
    socket_address.sll_protocol = htons(ETH_P_IP);
    // Index of the network device
    socket_address.sll_ifindex  = ifIndex;
    // sock_address.sll_ifindex = if_nametoindex(your_interface_name);
    // ARP hardware identifier is ethernet
    socket_address.sll_hatype   = ARPHRD_ETHER;
    // Target is another host
    socket_address.sll_pkttype  = PACKET_OTHERHOST;
    // Layer 2 address length
    socket_address.sll_halen    = ETH_ALEN;		
    // Destination MAC Address
    socket_address.sll_addr[0]  = destMAC[0];		
    socket_address.sll_addr[1]  = destMAC[1];
    socket_address.sll_addr[2]  = destMAC[2];
    socket_address.sll_addr[3]  = destMAC[3];
    socket_address.sll_addr[4]  = destMAC[4];
    socket_address.sll_addr[5]  = destMAC[5];
     // Last 2 bytes not used in Ethernet
    socket_address.sll_addr[6]  = 0x00;
    socket_address.sll_addr[7]  = 0x00;

    //  RX buffer for incoming ethernet frames
    char* rxBuffer = (char*)operator new(fSizeMax);

    //  TX buffer for outgoing ethernet frames
    txBuffer = (char*)operator new(fSizeMax);

    txEtherhead = (unsigned char*)txBuffer;

    build_headers(txBuffer, destMAC, sourceMAC, ethertype, PCP, vlanID, qinqID, qinqPCP, headersLength);

    // Userdata pointers in ethernet frames
    char* rxData = rxBuffer + headersLength;
    txData = txBuffer + headersLength;

    // 0 is success exit code for sending frames
    sendResult = 0;
    // Length of the received frame
    int rxLength = 0;


    // http://www.linuxjournal.com/article/4659?page=0,1
    cout << "Entering promiscuous mode" << endl;

    // Get the interface name
    strncpy(ethreq.ifr_name,ifName,IFNAMSIZ);

    // Set the network card in promiscuos mode

    if (ioctl(sockFD,SIOCGIFFLAGS,&ethreq)==-1) 
    {
        cout << "Error getting socket flags, entering promiscuous mode failed." << endl;
        perror("ioctl() ");
        close(sockFD);
        return EX_SOFTWARE;
    }

    ethreq.ifr_flags|=IFF_PROMISC;

    if (ioctl(sockFD,SIOCSIFFLAGS,&ethreq)==-1)
    {
        cout << "Error setting socket flags, entering promiscuous mode failed." << endl;
        perror("ioctl() ");
        close(sockFD);
        return EX_SOFTWARE;
    }

    std::string param;
    std::stringstream ss;
    ss.setf(ios::fixed);
    ss.setf(ios::showpoint);
    ss.precision(9);
    cout<<fixed<<setprecision(9);

    // At this point, declare our sigint handler, from now on when the two hosts
    // start communicating it will be of use, the TX will signal RX to reset when it dies
    signal (SIGINT,signal_handler);

    printf("Source MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           sourceMAC[0],sourceMAC[1],sourceMAC[2],sourceMAC[3],sourceMAC[4],sourceMAC[5]);
    printf("Destination MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
           destMAC[0],destMAC[1],destMAC[2],destMAC[3],destMAC[4],destMAC[5]);

    /* 
     * Before any communication happens between the local host and remote host, we must 
     * broadcast our whereabouts to ensure there is no initial loss of frames
     *
     * Set the dest MAC to broadcast (FF...FF) and build the frame like this,
     * then transmit three frames with a short brake between each.
     * Rebuild the frame headers with the actual dest MAC.
     */

    unsigned char broadMAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    build_headers(txBuffer, broadMAC, sourceMAC, ethertype, PCP, vlanID, qinqID, qinqPCP, headersLength);
    rxData = rxBuffer + headersLength;
    txData = txBuffer + headersLength;

    cout << "Sending gratuitous broadcasts..." << endl;
    for(lCounter=1; lCounter<=3; lCounter++)
    {
        param = "etheratepresence";
        strncpy(txData,param.c_str(),param.length());
        sendResult = sendto(sockFD, txBuffer, param.length()+headersLength, 0, 
                     (struct sockaddr*)&socket_address, sizeof(socket_address));
        sleep(1);
    }

    build_headers(txBuffer, destMAC, sourceMAC, ethertype, PCP, vlanID, qinqID, qinqPCP, headersLength);

    if(headersLength==18)
    {
        rxData = rxBuffer + (headersLength-4);
        txData = txBuffer + headersLength;
    } else if (headersLength==22) {
        rxData = rxBuffer + headersLength-4;
        txData = txBuffer + headersLength;
    } else {
        rxData = rxBuffer + headersLength;
        txData = txBuffer + headersLength;
    }
    fSizeTotal = fSize + headersLength;


    /*
     ************************************************************** TX/RX SETUP
     */



    /*
     ************************************************************ SETTINGS SYNC
     */


    // Set up the test by communicating settings with the RX host receiver
    if(txMode==true)
    { // If we are the TX host...

        cout << "Running in TX mode, synchronising settings" << endl;

        // Testing with a custom frame size
        if(ethertype!=ethertypeDefault)
        {
            ss << "etherateethertype" << ethertype;
            param = ss.str();
            strncpy(txData,param.c_str(),param.length());
            sendResult = sendto(sockFD, txBuffer, param.length()+headersLength, 0, 
                   (struct sockaddr*)&socket_address, sizeof(socket_address));
            printf("Ethertype set to 0x%X\n", ethertype);
        }

        // Testing with a custom frame size
        if(fSize!=fSizeDef)
        {
            ss << "etheratesize" << fSize;
            param = ss.str();
            strncpy(txData,param.c_str(),param.length());
            sendResult = sendto(sockFD, txBuffer, param.length()+headersLength, 0, 
                   (struct sockaddr*)&socket_address, sizeof(socket_address));
            cout << "Frame size set to " << fSize << endl;
        }


        // Testing with a custom duration
        if(fDuration!=fDurationDef)
        {
            ss << "etherateduration" << fDuration;
            param = ss.str();
            strncpy(txData,param.c_str(),param.length());
            sendResult = sendto(sockFD, txBuffer, param.length()+headersLength, 0, 
                   (struct sockaddr*)&socket_address, sizeof(socket_address));
            cout << "Test duration set to " << fDuration << endl;
        }


        // Testing with a custom frame count
        if(fCount!=fCountDef) {
            ss << "etheratecount" << fCount;
            param = ss.str();
            strncpy(txData,param.c_str(),param.length());
            sendResult = sendto(sockFD, txBuffer, param.length()+headersLength, 0, 
                   (struct sockaddr*)&socket_address, sizeof(socket_address));
            cout << "Frame count set to " << fCount << endl;
        }


        // Testing with a custom byte limit
        if(fBytes!=fBytesDef) {
            ss << "etheratebytes" << fBytes;
            param = ss.str();
            strncpy(txData,param.c_str(),param.length());
            sendResult = sendto(sockFD, txBuffer, param.length()+headersLength, 0, 
                   (struct sockaddr*)&socket_address, sizeof(socket_address));
            cout << "Byte limit set to " << fBytes << endl;
        }


        // Testing with a custom max speed limit
        if(bTXSpeed!=bTXSpeedDef)
        {
            ss << "etheratespeed" << bTXSpeed;
            param = ss.str();
            strncpy(txData,param.c_str(),param.length());
            sendResult = sendto(sockFD, txBuffer, param.length()+headersLength, 0, 
                   (struct sockaddr*)&socket_address, sizeof(socket_address));
            cout << "Max TX speed set to " << ((bTXSpeed*8)/1000/1000) << "Mbps" << endl;
        }

        // Testing with a custom inner VLAN PCP value
        if(PCP!=PCPDef)
        {
            ss << "etheratepcp" << PCP;
            param = ss.str();
            strncpy(txData,param.c_str(),param.length());
            sendResult = sendto(sockFD, txBuffer, param.length()+headersLength, 0, 
                   (struct sockaddr*)&socket_address, sizeof(socket_address));
            cout << "Inner VLAN PCP value set to " << PCP << endl;
        }

        // Tesing with a custom QinQ PCP value
        if(PCP!=PCPDef)
        {
            ss << "etherateqinqpcp" << qinqPCP;
            param = ss.str();
            strncpy(txData,param.c_str(),param.length());
            sendResult = sendto(sockFD, txBuffer, param.length()+headersLength, 0, 
                   (struct sockaddr*)&socket_address, sizeof(socket_address));
            cout << "QinQ VLAN PCP value set to " << qinqPCP << endl;
        }

        // Tell the receiver to run in ACK mode
        if(fACK==true)
        {
            param = "etherateack";
            strncpy(txData,param.c_str(),param.length());
            sendResult = sendto(sockFD, txBuffer, param.length()+headersLength, 0, 
                   (struct sockaddr*)&socket_address, sizeof(socket_address));
            cout << "ACK mode enabled" << endl;
        }


        // Send over the time for delay calculation between hosts,
        // We send it twice each time repeating this process multiple times to get an average;
        cout << "Calculating delay between hosts..." << endl;
        for(lCounter=0; lCounter<=4; lCounter++)
        {
            // The monotonic value is used here in case we are unlucky enough to
            // run this during and NTP update, so we stay consistent
            clock_gettime(CLOCK_MONOTONIC_RAW, &tsRTT);
            ss.str("");
            ss.clear();
            ss<<"etheratetime"<<lCounter<<"1:"<<tsRTT.tv_sec<<":"<<tsRTT.tv_nsec;
            param = ss.str();
            strncpy(txData,param.c_str(),param.length());
            sendResult = sendto(sockFD, txBuffer, param.length()+headersLength, 0, 
                         (struct sockaddr*)&socket_address, sizeof(socket_address));

            clock_gettime(CLOCK_MONOTONIC_RAW, &tsRTT);
            ss.str("");
            ss.clear();
            ss<<"etheratetime"<<lCounter<<"2:"<<tsRTT.tv_sec<<":"<<tsRTT.tv_nsec;
            param = ss.str();
            strncpy(txData,param.c_str(),param.length());
            sendResult = sendto(sockFD, txBuffer, param.length()+headersLength, 0, 
                         (struct sockaddr*)&socket_address, sizeof(socket_address));

            // After sending the 2nd time value we wait for the returned delay;
            bool waiting = true;
            ss.str("");
            ss.clear();
            ss<<"etheratedelay.";
            param = ss.str();

            while (waiting)
            {
                rxLength = recvfrom(sockFD, rxBuffer, fSizeTotal, 0, NULL, NULL);

                if(param.compare(0,param.size(),rxData,0,14)==0)
                {
                    exploded.clear();
                    explodestring = rxData;
                    string_explode(explodestring, ".", &exploded);
                    ss.str("");
                    ss.clear();
                    ss << exploded[1].c_str()<<"."<<exploded[2].c_str();
                    param = ss.str();
                    delay[lCounter] = strtod(param.c_str(), NULL);
                    waiting = false;
                }

            }

        }

        cout << "Tx to Rx delay calculated as " <<
             ((delay[0]+delay[1]+delay[2]+delay[3]+delay[4])/5)*1000 << "ms" << endl;
        // Let the receiver know all settings have been sent
        param = "etherateallset";
        strncpy(txData,param.c_str(),param.length());
        sendResult = sendto(sockFD, txBuffer, param.length()+headersLength, 0, 
               (struct sockaddr*)&socket_address, sizeof(socket_address));
        cout << "Settings have been synchronised." << endl;


    } else {

        cout << "Running in RX mode, awaiting TX host" << endl;
        // In listening mode we start by waiting for each parameter to come through
        // So we start a loop until they have all been received
        bool waiting = true;
        // Used for grabing sent values after the starting string in the received buffer
        int diff;
        // These values are used to calculate the delay between TX and RX hosts
        double timeTX1,timeRX1,timeTX2,timeRX2,timeTXdiff,timeRXdiff,timeTXdelay;

        // In this loop we are grabbing each incoming frame and looking at the 
        // string that prefixes it, to state which variable the following value
        // will be for
        while(waiting)
        {

            rxLength = recvfrom(sockFD, rxBuffer, fSizeTotal, 0, NULL, NULL);

            // TX has sent a non-default ethertype
            if(strncmp(rxData,"etherateethertype",17)==0)
            {
                diff = (rxLength-17);
                ss << rxData;
                param = ss.str().substr(17,diff);
                ethertype = strtol(param.c_str(),NULL,10);
                printf("Ethertype set to 0x%X\n", ethertype);

            }


            // TX has sent a non-default frame payload size
            if(strncmp(rxData,"etheratesize",12)==0)
            {
                diff = (rxLength-12);
                ss << rxData;
                param = ss.str().substr(12,diff);
                fSize = strtol(param.c_str(),NULL,10);
                cout << "Frame size set to " << fSize << endl;
            }


            // TX has sent a non-default transmition duration
            if(strncmp(rxData,"etherateduration",16)==0)
            {
                diff = (rxLength-16);
                ss << rxData;
                param = ss.str().substr(16,diff);
                fDuration = strtoull(param.c_str(),0,10);
                cout << "Test duration set to " << fDuration << endl;
            }


            // TX has sent a frame count to use instead of duration
            if(strncmp(rxData,"etheratecount",13)==0)
            {
                diff = (rxLength-13);
                ss << rxData;
                param = ss.str().substr(13,diff);
                fCount = strtoull(param.c_str(),0,10);
                cout << "Frame count set to " << fCount << endl;
            }


            // TX has sent a total bytes value to use instead of frame count
            if(strncmp(rxData,"etheratebytes",13)==0)
            {
                diff = (rxLength-13);
                ss << rxData;
                param = ss.str().substr(13,diff);
                fBytes = strtoull(param.c_str(),0,10);
                cout << "Byte limit set to " << fBytes << endl;
            }

            // TX has set a custom PCP value
            if(strncmp(rxData,"etheratepcp",11)==0)
            {
                diff = (rxLength-11);
                ss << rxData;
                param = ss.str().substr(11,diff);
                PCP = strtoull(param.c_str(),0,10);
                cout << "PCP value set to " << PCP << endl;
            }

            // TX has set a custom PCP value
            if(strncmp(rxData,"etherateqinqpcp",15)==0)
            {
                diff = (rxLength-15);
                ss << rxData;
                param = ss.str().substr(15,diff);
                qinqPCP = strtoull(param.c_str(),0,10);
                cout << "QinQ PCP value set to " << qinqPCP << endl;
            }

            // TX has requested we run in ACK mode
            if(strncmp(rxData,"etherateack",11)==0)
            {
                fACK = true;
                cout << "ACK mode enabled" << endl;
            }

            // Now begin the part to calculate the delay between TX and RX hosts.
            // Several time values will be exchanged to estimate the delay between
            // the two. Then the process is repeated two more times so and average
            // can be taken, which is shared back with TX;
            if( strncmp(rxData,"etheratetime01:",15)==0 ||
                strncmp(rxData,"etheratetime11:",15)==0 ||
                strncmp(rxData,"etheratetime21:",15)==0 ||
                strncmp(rxData,"etheratetime31:",15)==0 ||
                strncmp(rxData,"etheratetime41:",15)==0  )
            {

                // Get the time we are receiving TX's sent time figure
                clock_gettime(CLOCK_MONOTONIC_RAW, &tsRTT);
                ss.str("");
                ss.clear();
                ss << tsRTT.tv_sec << "." << tsRTT.tv_nsec;
                ss >> timeRX1;

                // Extract the sent time
                exploded.clear();
                explodestring = rxData;
                string_explode(explodestring, ":", &exploded);

                ss.str("");
                ss.clear();
                ss << atol(exploded[1].c_str()) << "." << atol(exploded[2].c_str());
                ss >> timeTX1;

            }


            if( strncmp(rxData,"etheratetime02:",15)==0 ||
                strncmp(rxData,"etheratetime12:",15)==0 ||
                strncmp(rxData,"etheratetime22:",15)==0 ||
                strncmp(rxData,"etheratetime32:",15)==0 ||
                strncmp(rxData,"etheratetime42:",15)==0    )
            {

                // Get the time we are receiving TXs 2nd sent time figure
                clock_gettime(CLOCK_MONOTONIC_RAW, &tsRTT);
                ss.str("");
                ss.clear();
                ss << tsRTT.tv_sec << "." << tsRTT.tv_nsec;
                ss >> timeRX2;

                // Extract the sent time
                exploded.clear();
                explodestring = rxData;
                string_explode(explodestring, ":", &exploded);

                ss.clear();
                ss.str("");
                ss << atol(exploded[1].c_str()) << "." << atol(exploded[2].c_str());
                ss >> timeTX2;

                // Calculate the delay
                timeTXdiff = timeTX2-timeTX1;
                timeTXdelay = (timeRX2-timeTXdiff)-timeRX1;
                //cout << timeRX1 << " : " << timeTX1 << endl;
                //cout << timeRX2 << " : " << timeTX2 << endl;
                //cout << "timeTXdiff = " << timeTXdiff << endl;
                //cout << "timeTXdelay = " << timeTXdelay << endl;

                if(strncmp(rxData,"etheratetime02:",15)==0) delay[0] = timeTXdelay;
                if(strncmp(rxData,"etheratetime12:",15)==0) delay[1] = timeTXdelay;
                if(strncmp(rxData,"etheratetime22:",15)==0) delay[2] = timeTXdelay;
                if(strncmp(rxData,"etheratetime32:",15)==0) delay[3] = timeTXdelay;
                if(strncmp(rxData,"etheratetime42:",15)==0) 
                { 
                    delay[4] = timeTXdelay;
                    cout << "Tx to Rx delay calculated as " <<
                         ((delay[0]+delay[1]+delay[2]+delay[3]+delay[4])/5)*1000
                         << "ms" << endl;
                }

                // Send it back to the TX host
                ss.clear();
                ss.str("");
                ss << "etheratedelay." << timeTXdelay;
                param = ss.str();
                strncpy(txData,param.c_str(), param.length());
                sendResult = sendto(sockFD, txBuffer, param.length()+headersLength, 0, 
                                    (struct sockaddr*)&socket_address,
                                    sizeof(socket_address));
            }

            // All settings have been received
            if(strncmp(rxData,"etherateallset",14)==0)
            {
                waiting = false;
                cout<<"Settings have been synchronised"<<endl;
            }

        } // Waiting bool

        // Rebuild the test frame headers in case any settings have been changed
        // by the TX host
        build_headers(txBuffer, destMAC, sourceMAC, ethertype, PCP, vlanID,
                      qinqID, qinqPCP, headersLength);
      
    } // TX or RX mode


    /*
     ************************************************************ SETTINGS SYNC
     */



    /*
     *************************************************************** TEST PHASE
     */


    // Fill the test frame with some junk data
    int junk = 0;
    for (junk = 0; junk < fSize; junk++)
    {
        txData[junk] = (char)((int) 65); // ASCII 65 = A;
        //(255.0*rand()/(RAND_MAX+1.0)));
    }


    timeNow = time(0);
    localtm = localtime(&timeNow);
    cout << endl << "Starting test on " << asctime(localtm) << endl;
    ss.precision(2);
    cout << fixed << setprecision(2);

    FD_ZERO(&readfds);
    int sockFDCount = sockFD + 1;
    int selectRetVal;

    if (txMode==true)
    {

        cout << "Seconds\t\tMbps TX\t\tMBs Tx\t\tFrmTX/s\t\tFrames TX" << endl;

        long long *testBase, *testMax;
        if (fBytes>0)
        {
            // We are testing until X bytes received
            testMax = &fBytes;
            testBase = &bTX;

        } else if (fCount>0) {
            // We are testing until X frames received
            testMax = &fCount;
            testBase = &fTX;

        } else if (fDuration>0) {
            // We are testing until X seconds has passed
            testMax = &fDuration;
            testBase = &sElapsed;
        }


        // Get clock time for the speed limit option,
        // get another to record the initial starting time
        clock_gettime(CLOCK_MONOTONIC_RAW, &txLimit);
        clock_gettime(CLOCK_MONOTONIC_RAW, &tsElapsed);

        // Main TX loop
        while (*testBase<=*testMax)
        {

            // Get the current time
            clock_gettime(CLOCK_MONOTONIC_RAW, &tsCurrent);

            // One second has passed
            if((tsCurrent.tv_sec-tsElapsed.tv_sec)>=1)
            {
                sElapsed+=1;
                bSpeed = (((float)bTX-(float)bTXlast)*8)/1000/1000;
                bTXlast = bTX;
                cout << sElapsed << "\t\t" << bSpeed << "\t\t" << (bTX/1000)/1000 << "\t\t"
                     << (fTX-fTXlast) << "\t\t" << fTX << endl;
                fTXlast = fTX;
                tsElapsed.tv_sec = tsCurrent.tv_sec;
                tsElapsed.tv_nsec = tsCurrent.tv_nsec;
            }

            // Poll the socket file descriptor with select() for incoming frames
            tvSelectDelay.tv_sec = 0;
            tvSelectDelay.tv_usec = 000000;
            FD_SET(sockFD, &readfds);
            selectRetVal = select(sockFDCount, &readfds, NULL, NULL, &tvSelectDelay);
            if (selectRetVal > 0) {
                if (FD_ISSET(sockFD, &readfds)) {

                    rxLength = recvfrom(sockFD, rxBuffer, fSizeTotal, 0, NULL, NULL);
                    if(fACK)
                    {
                        param = "etherateack";
                        if(strncmp(rxData,param.c_str(),param.length())==0)
                        {
                            fRX++;
                            fWaiting = false;
                        } else {
                            // Check if RX host has sent a dying gasp
                            param = "etheratedeath";
                            if(strncmp(rxData,param.c_str(),param.length())==0)
                            {
                                timeNow = time(0);
                                localtm = localtime(&timeNow);
                                cout << "RX host is going down." << endl
                                     << "Ending test and resetting on "
                                     << asctime(localtm) << endl;
                                goto finish;
                            } else {
                                fRXother++;
                            }
                        }

                    } else {
                        
                        // Check if RX host has sent a dying gasp
                        param = "etheratedeath";
                        if(strncmp(rxData,param.c_str(),param.length())==0)
                        {
                            timeNow = time(0);
                            localtm = localtime(&timeNow);
                            cout << "RX host is going down." << endl
                                 << "Ending test and resetting on "
                                 << asctime(localtm) << endl;
                            goto finish;
                        }
                        
                    }
                    
                }
            }

            // If it hasn't been 1 second yet
            if (tsCurrent.tv_sec-txLimit.tv_sec<1)
            {

                if(!fWaiting) {

                // A max speed has been set
                if(bTXSpeed!=bTXSpeedDef)
                {

                    // Check if sending another frame keeps us under the max speed limit
                    if((bTXSpeedLast+fSize)<=bTXSpeed)
                    {

                        ss.clear();
                        ss.str("");
                        ss << "etheratetest" << (fTX+1) <<  ":";
                        param = ss.str();
                        strncpy(txData,param.c_str(), param.length());
                        sendResult = sendto(sockFD, txBuffer, fSizeTotal, 0, 
                   	                        (struct sockaddr*)&socket_address,
                                            sizeof(socket_address));
                        fTX+=1;
                        bTX+=fSize;
                        bTXSpeedLast+=fSize;
                        if (fACK) fWaiting = true;

                    }

                } else {

                    ss.clear();
                    ss.str("");
                    ss << "etheratetest" << (fTX+1) <<  ":";
                    param = ss.str();
                    strncpy(txData,param.c_str(), param.length());
                    sendResult = sendto(sockFD, txBuffer, fSizeTotal, 0, 
                                        (struct sockaddr*)&socket_address,
                                        sizeof(socket_address));
                    fTX+=1;
                    bTX+=fSize;
                    bTXSpeedLast+=fSize;
                    if (fACK) fWaiting = true;
                }

                }

            } else { // 1 second has passed

              bTXSpeedLast = 0;
              clock_gettime(CLOCK_MONOTONIC_RAW, &txLimit);

            } // End of txLimit.tv_sec<1

        }

        cout << sElapsed << "\t\t" << bSpeed << "\t\t" << (bTX/1000)/1000 << "\t\t" << (fTX-fTXlast)
             << "\t\t" << fTX << endl << endl
             << "Non test frames received: " << fRXother << endl;

        timeNow = time(0);
        localtm = localtime(&timeNow);
        cout << endl << "Ending test on " << asctime(localtm) << endl;


    // Else, we are in RX mode
    } else {

        cout << "Seconds\t\tMbps RX\t\tMBs Rx\t\tFrmRX/s\t\tFrames RX" << endl;

        long long *testBase, *testMax;

        // Are we testing until X bytes received
        if (fBytes>0)
        {
            testMax = &fBytes;
            testBase = &bRX;

        // Or are we testing until X frames received
        } else if (fCount>0) {
            testMax = &fCount;
            testBase = &fRX;

        // Or are we testing until X seconds has passed
        } else if (fDuration>0) {
            testMax = &fDuration;
            testBase = &sElapsed;
        }

        // Get the initial starting time
        clock_gettime(CLOCK_MONOTONIC_RAW, &tsElapsed);

        // Main RX loop
        while (*testBase<=*testMax)
        {

            clock_gettime(CLOCK_MONOTONIC_RAW, &tsCurrent);
            // If one second has passed
            if((tsCurrent.tv_sec-tsElapsed.tv_sec)>=1)
            {
                sElapsed+=1;
                bSpeed = float (((float)bRX-(float)bRXlast)*8)/1000/1000;
                bRXlast = bRX;
                cout << sElapsed << "\t\t" << bSpeed << "\t\t" << (bRX/1000)/1000 << "\t\t"
                     << (fRX-fRXlast) << "\t\t" << fRX << endl;
                fRXlast = fRX;
                tsElapsed.tv_sec = tsCurrent.tv_sec;
                tsElapsed.tv_nsec = tsCurrent.tv_nsec;
            }

            // Poll the socket file descriptor with select() to 
            // check for incoming frames
            tvSelectDelay.tv_sec = 0;
            tvSelectDelay.tv_usec = 000000;
            FD_SET(sockFD, &readfds);
            selectRetVal = select(sockFDCount, &readfds, NULL, NULL, &tvSelectDelay);
            if (selectRetVal > 0) {
                if (FD_ISSET(sockFD, &readfds))
                {

                    rxLength = recvfrom(sockFD, rxBuffer, fSizeTotal, 0, NULL, NULL);

                    // Check if this is an etherate test frame
                    param = "etheratetest";
                    if(strncmp(rxData,param.c_str(),param.length())==0)
                    {

                        // Update test stats
                        fRX+=1;
                        bRX+=(rxLength-headersLength);

                        // If running in ACK mode we need to ACK to TX host
                        if(fACK)
                        {

                            // Get the index of the received frame
                            exploded.clear();
                            explodestring = rxData;
                            string_explode(explodestring, ".", &exploded);
                            fIndex = atoi(exploded[1].c_str());

                            if(fIndex==(fIndexLast+1))
                            {

                            } else if (fIndex<fIndexLast) {
                                fEarly++;

                            } else if (fIndex>(fIndexLast+1)) {
                                fLate++;
                            }

                            ss.clear();
                            ss.str("");
                            ss << "etherateack" << fRX;
                            param = ss.str();
                            strncpy(txData,param.c_str(), param.length());
                            sendResult = sendto(sockFD, txBuffer, fSizeTotal, 0, 
                                         (struct sockaddr*)&socket_address,
                                         sizeof(socket_address));
                            
                        }

                    } else {

                        // We received a non-test frame
                        fRXother++;

                    }

                    // Check if TX host has quit/died;
                    param = "etheratedeath";
                    if(strncmp(rxData,param.c_str(),param.length())==0)
                    {
                        timeNow = time(0);
                        localtm = localtime(&timeNow);
                        cout << "TX host is going down." << endl <<
                                "Ending test and resetting on " << asctime(localtm)
                                << endl;
                        goto restart;
                    }
                      
                }
            }

        }

        cout << sElapsed << "\t\t" << bSpeed << "\t\t" << (bRX/1000)/1000 << "\t\t"
             << (fRX-fRXlast) << "\t\t" << fRX << endl << endl
             << "Non test frames received: " << fRXother << endl;

        timeNow = time(0);
        localtm = localtime(&timeNow);
        cout << endl << "Ending test on " << asctime(localtm) << endl;
        close(sockFD);
        goto restart;

    }


    finish:

    cout << "Leaving promiscuous mode" << endl;

    strncpy(ethreq.ifr_name,ifName,IFNAMSIZ);

    if (ioctl(sockFD,SIOCGIFFLAGS,&ethreq)==-1)
    {
        cout << "Error getting socket flags, entering promiscuous mode failed." << endl;
        perror("ioctl() ");
        close(sockFD);
        return EX_SOFTWARE;
    }

    ethreq.ifr_flags &= ~IFF_PROMISC;

    if (ioctl(sockFD,SIOCSIFFLAGS,&ethreq)==-1)
    {
        cout << "Error setting socket flags, promiscuous mode failed." << endl;
        perror("ioctl() ");
        close(sockFD);
        return EX_SOFTWARE;
    }

    close(sockFD);


    /*
     *************************************************************** TEST PHASE
     */


    return EXIT_SUCCESS;

} // End of main()


