/*
 * Etherate ~ 0.4.alpha
 * 2014-02-10
 *
 * Updates: https://github.com/jwbensley/Etherate and http://null.53bits.co.uk
 * Please send corrections, ideas and help to: jwbensley@gmail.com (I'm a beginner if that isn't obvious!)
 * compile with: g++ -o etherate etherate.cpp -lrt
 *
 */


/*
 * Exit codes are from: /usr/include/sysexits.h
 * EX_USAGE 64 command line usage error
 * EX_NOPERM 77 permission denied
 * EX_SOFTWARE 70 internal software error
 * EX_PROTOCOL 76 remote error in protocol
 */


#include <cmath> // std::abs() from cmath for floats and doubles
#include <cstring> // Needed for memcpy
#include <ctime> // Required for timer types
#include <iomanip> //  setprecision() and precision()
#include <iostream> // cout
using namespace std;
#include <sys/socket.h> // Provides AF_PACKET *Address Family*
#include <linux/if_arp.h>
#include <linux/if_ether.h> // Defines ETH_P_ALL [Ether type 0x003 (ALL PACKETS!)]
// Also defines ETH_FRAME_LEN with default 1514
// Also defines ETH_ALEN which is Ethernet Address Lenght in Octets (defined as 6)
#include <linux/if_packet.h>
//#include <linux/if.h>
#include <linux/net.h> // Provides SOCK_RAW *Socket Type*
//#include <net/if.h> // Provides if_indextoname/if_nametoindex
#include <netinet/in.h> // Needed for htons() and htonl()
// Host-to-Network-short htons()
// Host-to-Network-long htonl()
// Network-to-Host-short ntohs()
// Network-to-Host-long ntohl()
#include <signal.h> // For use of signal() to catch SIGINT
#include <sstream> // Stringstream objects and functions
#include <stdio.h> //perror()
#include <stdlib.h> //malloc() itoa()
#include <stdbool.h> // bool/_Bool
#include <string.h> //Required for strncmp() function
#define MAX_IFS 64
#include <sys/ioctl.h>
#include <time.h> // Required for clock_gettime()
#include "unistd.h" // sleep(), getuid()
#include <vector> // Required for string explode function

// CLOCK_MONOTONIC_RAW is in from linux-2.26.28 onwards,
// It's needed here for accurate delay calculation
// There is a good S-O post about this here;
// http://stackoverflow.com/questions/3657289/linux-clock-gettimeclock-monotonic-strange-non-monotonic-behavior
#ifndef CLOCK_MONOTONIC_RAW
#define CLOCK_MONOTONIC_RAW 4
#endif

#include "etherate.0.3.alpha.h"
#include "funcs.0.3.alpha.cpp"



int main(int argc, char *argv[]) {

/*
 ********************************************************************************************** Set initial variable values
 */

start:

// By default we are transmitting data
bool txMode = true;

// Interface index, default this to -1 so we know later if the user changed it
int ifIndex = -1;

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

// This is the length of a frame header by default, without dot1Q tags
headersLength = 14;

// Frame size in bytes (just the payload)
int fSize = fSizeDef;

// Total frame size including headers
int fSizeTotal = fSize + headersLength;

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

// Total number of bytes transmitted
long long bTX = 0;

// Bytes sent at last count for calculating speed
long long bTXlast = 0;

// Total number of bytes received
long long bRX = 0;

// Bytes received at last count for calculating speed
long long bRXlast = 0;

// Average speed whilst running the test
float bSpeed = 0;

// Are we running in ACK mode during transmition
bool ACKMode = false;

// These timespecs are used for calculating delay/RTT
timespec tspecTX, tspecRX;

// Three doubles to calculate the delay three times, to divide for an average
double delay[5];

// Two timers for timing the test and calculating stats
timespec durTimer1, durTimer2;

// Seconds the test has been running
long long sElapsed = 0;

// Timer used for rate limiting the TX host
timespec txLimit;

// Time type for holding the current date and time
time_t timeNow;

// Time struct for breaking down the above time type
tm* localtm;


/* 
  These variables are declared here and used over and over throughout;

   The string explode function is used several times throughout,
   so rather than continuously defining variables they are defined here 
*/
vector<string> exploded;
string explodestring;
// Also, a generic loop counter
int lCounter;




/*
 ********************************************************************************************** Process CLI arguments
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
            StringExplode(explodestring, ":", &exploded);

            if((int) exploded.size() != 6) 
            {
                cout << "Error: Invalid destination MAC address!" << endl
                     << "Usage info: " << argv[0] << " -h" << endl;
                return 64;
            }
            cout << "Destination MAC ";

            // Here strtoul() is used to convert the interger value of c_str() to a hex
            // number by giving it the number base 16 (hexadecimal) otherwise it would be atoi()
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
            StringExplode(explodestring, ":", &exploded);

            if((int) exploded.size() != 6) 
            {
                cout << "Error: Invalid source MAC address!" << endl
                     << "Usage info: " << argv[0] << " -h" << endl;
                return 64;
            }
            cout << "Custom source MAC ";

            for(int i = 0; (i < 6) && (i < exploded.size()); ++i)
            {
                sourceMAC[i] = (unsigned char)strtoul(exploded[i].c_str(), NULL, 16);
                cout << setw(2) << setfill('0') << hex << int(sourceMAC[i]) << ":";
            }
            cout << dec << endl;
            lCounter++;


        // Specifying a custom interface index
        } else if(strncmp(argv[lCounter],"-i",2)==0) {
            if (argc>(lCounter+1))
            {
                ifIndex = atoi(argv[lCounter+1]);
                lCounter++;
            } else {
                cout << "Oops! Missing interface index" << endl
                     << "Usage info: " << argv[0] << " -h" << endl;
                return 64;
            }


        // Requesting to list interfaces
        } else if(strncmp(argv[lCounter],"-l",2)==0) {
            ListInterfaces();
            return EXIT_SUCCESS;


        // Specifying a custom frame payload size in bytes
        } else if(strncmp(argv[lCounter],"-f",2)==0) {
            if (argc>(lCounter+1))
            {
                fSize = atoi(argv[lCounter+1]);
                if(fSize > 1500)
                {
                    cout << "WARNING: Make sure your device supports baby giant or jumbo"
                         << "frames as required" << endl;
                }
                lCounter++;

            } else {
                cout << "Oops! Missing frame size" << endl
                     << "Usage info: " << argv[0] << " -h" << endl;
                return 64;
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
                return 64;
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
                return 64;
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
                return 64;
            }


        // Enable ACK mode testing
        } else if(strncmp(argv[lCounter],"-a",2)==0) {
            ACKMode = true;


        // Limit TX rate to max mbps
        } else if(strncmp(argv[lCounter],"-m",2)==0) {
            if (argc>(lCounter+1))
            {
                bTXSpeed = atol(argv[lCounter+1]);
                lCounter++;
            } else {
                cout << "Oops! Missing max TX rate" << endl
                     << "Usage info: " << argv[0] << " -h" << endl;
                return 64;
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
                return 64;
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
                return 64;
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
                return 64;
            }


        // Set 802.1ad QinQ outer PCP value
        } else if(strncmp(argv[lCounter],"-o",2)==0) {
            if (argc>(lCounter+1))
            {
                qinqPCP = atoi(argv[lCounter+1]);
                lCounter++;
            } else {
                cout << "Oops! Missing 802.1ad QinQ outer PCP value" << endl
                     << "Usage info: " << argv[0] << " -h" << endl;
                return 64;
            }


        // Display version
        } else if(strncmp(argv[lCounter],"-V",2)==0 || strncmp(argv[lCounter],"--version",9)==0) {
            cout << "Etherate version " << version << endl;
            return 0;


        // Display usage instructions
        } else if(strncmp(argv[lCounter],"-h",2)==0 || strncmp(argv[lCounter],"--help",6)==0) {
            PrintUsage();
            return 0;
        }


    }

} // Argc > 1

/*
 ********************************************************************************************** End CLI arguments
 */




/*
 ********************************************************************************************** Socket and tx/rx definitions
 */

// Check for root access, otherwise we can't gain the low level socket access we desire;
if(getuid()!=0) {
    cout << "Must be root to use this program!" << endl;
    return 77;
}


sockFD = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
/* Here we opened a socket with three arguments;
 * Communication Domain = AF_PACKET (Low level packet interface address family
 *  [lower than IP] ), AF_PACKET is in Linux 2.2 and onwards, it's the same as PF_PACKET
 * Type/Communication Semantics = SOCK_RAW (Provides raw network protocol access,
 *  so no reliable stream or datagram communication, SOCK_DGRAM means the Kernal controls
 *  the headers, SOCK_RAW lets us write them)
 * Protocol = Host to Network byte ordering(0x0003, which is everything, from if_ether.h)
 */


if (sockFD < 0 )
{
  cout << "Error defining socket." << endl;
  perror("socket() ");
  close(sockFD);
  return 70;
}


// If the user hasn't set an interface index try and grab one
if(ifIndex<0)
{
    ifIndex = GetSockInterface(sockFD);
    if (ifIndex==0)
    {
        cout << "Error: Couldn't find appropriate interface ID, returned ID was 0." << endl
             << "This is typically the loopback interface ID. Supply a source MAC address "
             << "with the -s option to try and manually fudge it." << endl;
        return 70;
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
// Changed from C ctyle: (void*)malloc(ETH_FRAME_LEN);
char* rxBuffer = (char*)operator new(fSizeMax);

//  TX buffer for outgoing ethernet frames
txBuffer = (char*)operator new(fSizeMax);

txEtherhead = (unsigned char*)txBuffer;

BuildHeaders(txBuffer, destMAC, sourceMAC, PCP, vlanID, qinqID, qinqPCP, headersLength);

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
    return 70;
}

ethreq.ifr_flags|=IFF_PROMISC;

if (ioctl(sockFD,SIOCSIFFLAGS,&ethreq)==-1)
{
    cout << "Error setting socket flags, entering promiscuous mode failed." << endl;
    perror("ioctl() ");
    close(sockFD);
    return 70;
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

printf("Source MAC %02x:%02x:%02x:%02x:%02x:%02x\n",sourceMAC[0],sourceMAC[1],sourceMAC[2],sourceMAC[3],sourceMAC[4],sourceMAC[5]);
printf("Destination MAC %02x:%02x:%02x:%02x:%02x:%02x\n",destMAC[0],destMAC[1],destMAC[2],destMAC[3],destMAC[4],destMAC[5]);

/* 
 * Before any communication happens between the local host and remote host, we must 
 * broadcast our whereabouts to ensure there is no initial loss of frames
 *
 * Set the dest MAC to broadcast (FF...FF) and build the frame like this,
 * then transmit three frames with a short brake between each.
 * Rebuild the frame headers with the actual dest MAC.
 */

unsigned char broadMAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
BuildHeaders(txBuffer, broadMAC, sourceMAC, PCP, vlanID, qinqID, qinqPCP, headersLength);
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

BuildHeaders(txBuffer, destMAC, sourceMAC, PCP, vlanID, qinqID, qinqPCP, headersLength);

if(headersLength==18)
{
    rxData = rxBuffer + (headersLength-4);
    txData = txBuffer + headersLength;
} else if (headersLength==22) {
    rxData = rxBuffer + headersLength;
    txData = txBuffer + headersLength;
} else {
    rxData = rxBuffer + headersLength;
    txData = txBuffer + headersLength;
}
fSizeTotal = fSize + headersLength;


/*
 ********************************************************************************************** End socket definitions
 */



/*
 ********************************************************************************************** Sync settings between hosts
 */

// Set up the test by communicating settings with the RX host receiver
if(txMode==true)
{ // If we are the TX host...

    cout << "Running in TX mode, synchronising settings" << endl;

    // Testing with a custom frame size
    if(fSize!=fSizeDef) {
        ss << "etheratesize" << fSize;
        param = ss.str();
        strncpy(txData,param.c_str(),param.length());
        sendResult = sendto(sockFD, txBuffer, param.length()+headersLength, 0, 
               (struct sockaddr*)&socket_address, sizeof(socket_address));
        cout << "Frame size set to " << fSize << endl;
    }


    // Testing with a custom duration
    if(fDuration!=fDurationDef) {
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
    if(bTXSpeed!=bTXSpeedDef) {
        ss << "etheratespeed" << bTXSpeed;
        param = ss.str();
        strncpy(txData,param.c_str(),param.length());
        sendResult = sendto(sockFD, txBuffer, param.length()+headersLength, 0, 
               (struct sockaddr*)&socket_address, sizeof(socket_address));
        cout << "Max TX speed set to " << ((bTXSpeed*8)/1000/1000) << "Mbps" << endl;
    }


    // Tell the receiver to run in ACK mode
    if(ACKMode==true) {
        param = "etherateack";
        strncpy(txData,param.c_str(),param.length());
        sendResult = sendto(sockFD, txBuffer, param.length()+headersLength, 0, 
               (struct sockaddr*)&socket_address, sizeof(socket_address));
        cout << "ACK mode enabled" << endl;
    }


    // Send over the time for delay calculation between hosts,
    // We send it twice each time repeate this process multiple times to get an average;
    cout << "Calculating delay between hosts" << endl;
    for(lCounter=0; lCounter<=4; lCounter++)
    {
        // The monotonic value is used here in case we are unlucky enough to
        // run this during and NTP update, so we stay consistent
        clock_gettime(CLOCK_MONOTONIC_RAW, &tspecTX);
        ss.str("");
        ss.clear();
        ss<<"etheratetime"<<lCounter<<"1:"<<tspecTX.tv_sec<<":"<<tspecTX.tv_nsec;
        param = ss.str();
        strncpy(txData,param.c_str(),param.length());
        sendResult = sendto(sockFD, txBuffer, param.length()+headersLength, 0, 
                     (struct sockaddr*)&socket_address, sizeof(socket_address));

        clock_gettime(CLOCK_MONOTONIC_RAW, &tspecTX);
        ss.str("");
        ss.clear();
        ss<<"etheratetime"<<lCounter<<"2:"<<tspecTX.tv_sec<<":"<<tspecTX.tv_nsec;
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
            cout << "Length " << rxLength << " rxData: " << rxData << endl;

            if(param.compare(0,param.size(),rxData,0,14)==0)
            {
                exploded.clear();
                explodestring = rxData;
                StringExplode(explodestring, ".", &exploded);
                ss.str("");
                ss.clear();
                ss << exploded[1].c_str()<<"."<<exploded[2].c_str();
                param = ss.str();
                delay[lCounter] = strtod(param.c_str(), NULL);
                waiting = false;
            }

        }


    }


    cout << "Delay calculated as " << ((delay[0]+delay[1]+delay[2])/3) << endl;
    // Let the receiver know all settings have been sent
    param = "etherateallset";
    strncpy(txData,param.c_str(),param.length());
    sendResult = sendto(sockFD, txBuffer, param.length()+headersLength, 0, 
           (struct sockaddr*)&socket_address, sizeof(socket_address));
    cout<<"Settings have been synchronised."<<endl;


} else {

    cout << "Running in RX mode, awaiting settings" << endl;
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
    while(waiting) {

        rxLength = recvfrom(sockFD, rxBuffer, fSizeTotal, 0, NULL, NULL);
        cout << "Length " << rxLength << " rxData: " << rxData << endl;


        // TX has sent a non-default frame payload size
        if(strncmp(rxData,"etheratesize",12)==0) {
            diff = (rxLength-12);
            ss << rxData;
            param = ss.str().substr(12,diff);
            fSize = strtol(param.c_str(),0,10);
            cout << "Frame size set to " << fSize << endl;
        }


        // TX has sent a non-default transmition duration
        if(strncmp(rxData,"etherateduration",16)==0) {
            diff = (rxLength-16);
            ss << rxData;
            param = ss.str().substr(16,diff);
            fDuration = strtoull(param.c_str(),0,10);
            cout << "Test duration set to " << fDuration << endl;
        }


        // TX has sent a frame count to use instead of duration
        if(strncmp(rxData,"etheratecount",13)==0) {
            diff = (rxLength-13);
            ss << rxData;
            param = ss.str().substr(13,diff);
            fCount = strtoull(param.c_str(),0,10);
            cout << "Frame count set to " << fCount << endl;
        }


        // TX has sent a total bytes value to use instead of frame count
        if(strncmp(rxData,"etheratebytes",13)==0) {
            diff = (rxLength-13);
            ss << rxData;
            param = ss.str().substr(13,diff);
            fBytes = strtoull(param.c_str(),0,10);
            cout << "Byte limit set to " << fBytes << endl;
        }


        // TX has requested we run in ACK mode
        if(strncmp(rxData,"etherateack",11)==0) {
            ACKMode = true;
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

        cout << rxLength << endl;
            // Get the time we are receiving TX's sent time figure
            clock_gettime(CLOCK_MONOTONIC_RAW, &tspecRX);
            ss.str("");
            ss.clear();
            ss << tspecRX.tv_sec << "." << tspecRX.tv_nsec;
            ss >> timeRX1;

            // Extract the sent time
            exploded.clear();
            explodestring = rxData;
        cout << rxData << endl;
            StringExplode(explodestring, ":", &exploded);
            if((int) exploded.size() != 3)
            {
                int lala = (int) exploded.size();
                cout << lala << " ---- " << rxData << endl;
                cout << "Error: Invalid 1st time stamp received! " << endl;
                return 76;                          ///////////// DO WE WANT TO ALLOW CARRY ON WITHOUT DELAY OR INVALID DELAY?
            }
            ss.str("");
            ss.clear();
            ss << atol(exploded[1].c_str()) << "." << atol(exploded[2].c_str());
            ss >> timeTX1;

            // Get the difference in TX and RX clocks
            if(timeTX1<timeRX1)
            {
                timeRXdiff = std::abs(timeRX1-timeTX1);
            }

        }


        if( strncmp(rxData,"etheratetime02:",15)==0 ||
            strncmp(rxData,"etheratetime12:",15)==0 ||
            strncmp(rxData,"etheratetime22:",15)==0 ||
            strncmp(rxData,"etheratetime32:",15)==0 ||
            strncmp(rxData,"etheratetime42:",15)==0  )
        {

      cout << rxLength << endl;
            // Get the moment we are receiving TXs sent time figure
            clock_gettime(CLOCK_MONOTONIC_RAW, &tspecRX);
            ss.str("");
            ss.clear();
            ss << tspecRX.tv_sec << "." << tspecRX.tv_nsec;
            ss >> timeRX2;

            // Extract the sent time
            exploded.clear();
            explodestring = rxData;
      cout << rxData << endl;
            StringExplode(explodestring, ":", &exploded);
            if((int) exploded.size() != 3)
            {
                int lala = (int) exploded.size();
                cout << lala << " ---- " << rxData << endl;
                cout << "Error: Invalid 2nd time stamp received!" << endl;
                return 76;                                    ///////////// DO WE WANT TO ALLOW CARRY ON WITHOUT DELAY OR INVALID DELAY?
            }
            ss.clear();
            ss.str("");
            ss << atol(exploded[1].c_str()) << "." << atol(exploded[2].c_str());
            ss >> timeTX2;

            // Calculate the delay
            timeTXdiff = timeTX2-timeTX1;
            timeTXdelay = (timeRX2-timeTXdiff)-timeRX1;
            cout << timeRX1 << " : " << timeTX1 << endl;
            cout << timeRX2 << " : " << timeTX2 << endl;
            cout << "timeTXdiff = " << timeTXdiff << endl;
            cout << "timeTXdelay = " << timeTXdelay << endl;

            if(strncmp(rxData,"etheratetime02:",15)==0) delay[0] = timeTXdelay;
            if(strncmp(rxData,"etheratetime12:",15)==0) delay[1] = timeTXdelay;
            if(strncmp(rxData,"etheratetime22:",15)==0) delay[2] = timeTXdelay;
            if(strncmp(rxData,"etheratetime32:",15)==0) delay[3] = timeTXdelay;
            if(strncmp(rxData,"etheratetime42:",15)==0)
            {

                delay[4] = timeTXdelay;
                cout << "Delay calculated as " << ((delay[0]+delay[1]+delay[2]+delay[3]+delay[4])/5) << endl;
                double temp;
                int i,j;
                for(i=0;i<5;i++)
                {
                    for(j=i+1;j<5;j++)
                    {
                        if(delay[i]>delay[j])
                        {
                              temp=delay[j];
                              delay[j]=delay[i];
                              delay[i]=temp;
                        }
                    }
                }
                for (int n=0; n<5; n++)
                {
                    cout << delay[n] << endl;
                }
                  cout << "Median: " << delay[2] << endl;
            }

            // Send it back to the TX host
            ss.clear();
            ss.str("");
            ss << "etheratedelay." << timeTXdelay;
            param = ss.str();
            strncpy(txData,param.c_str(), param.length());
            sendResult = sendto(sockFD, txBuffer, param.length()+headersLength, 0, 
                   (struct sockaddr*)&socket_address, sizeof(socket_address));
        }

        // All settings have been received
        if(strncmp(rxData,"etherateallset",14)==0)
        {
            waiting = false;
            cout<<"Settings have been synchronised"<<endl;
        }

    } // Waiting bool
  
}

/*
 ********************************************************************************************** End Sync settings
 */




/*
 ********************************************************************************************** Run tests
 */

// Fill the test frame with some junk data
int junk;
for (junk = 0; junk < fSize; junk++)
{
    txData[junk] = (char)((int) 65); // ASCII 65 = A;
    //(255.0*rand()/(RAND_MAX+1.0)));
}


timeNow = time(0);
localtm = localtime(&timeNow);
cout << endl << "Starting test on " << asctime(localtm) << endl;
ss.precision(36); // ??????????????????????????????????????????????????????????? WHY IS THIS 36?
cout << fixed << setprecision(6);

testing = 1;

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

    // ETH_FRAME_LEN - Go through and check where this is used to transmit/receive a frame
    // If we are sending less that 1500 bytes for example, it should be payLoadSize+headersLength

    // The displaid TX rate (19Mbps) on the TX host when testing with 20 byte frames is less than the RX host displaid RX rate (40Mbps)

    // No time out feature; for example, after 30 seconds the test should finish on the TX host (actually, this does happen sometimes?)

    // Display post test details; min max avg for Mbps and FrmRX/S

    // A max speed has been set
    if(bTXSpeed!=bTXSpeedDef)
      {

        // Get a clock time for the speed limit
        clock_gettime(CLOCK_MONOTONIC_RAW, &txLimit);
        // Get another for the initial starting time
        clock_gettime(CLOCK_MONOTONIC_RAW, &durTimer1);

        // Main TX loop
        while (*testBase<=*testMax)
        {

            // Get the current time
            clock_gettime(CLOCK_MONOTONIC_RAW, &durTimer2);

            // One second has passed
            if((durTimer2.tv_sec-durTimer1.tv_sec)>=1)
            {
                sElapsed+=1;
                bSpeed = (((float)bTX-(float)bTXlast)*8)/1000/1000;
                bTXlast = bTX;
                cout << sElapsed << "\t\t" << bSpeed << "\t" << (bTX/1000)/1000 << "\t\t"
                     << (fTX-fTXlast) << "\t\t" << fTX << endl;
                fTXlast = fTX;
                durTimer1.tv_sec = durTimer2.tv_sec;
                durTimer1.tv_nsec = durTimer2.tv_nsec;
            }

            // If it hasn't been 1 second yet
            if (durTimer2.tv_sec-txLimit.tv_sec<1)
            {
                if((bTXSpeedLast+fSize)<=bTXSpeed)
                {

                    //If sending another frame keeps us under speed
                    ss.clear();
                    ss.str("");
                    ss << "etheratetest" << (fTX+1);
                    param = ss.str();
                    strncpy(txData,param.c_str(), param.length());
                    sendResult = sendto(sockFD, txBuffer, fSizeTotal, 0, 
               	          (struct sockaddr*)&socket_address, sizeof(socket_address));
                    fTX+=1;
                    bTX+=fSize;
                    bTXSpeedLast+=fSize;
                    if(ACKMode)
                    {
                        ss.clear();
                        ss.str("");
                        ss << "etherateack" << fTX;
                        param = ss.str();
                        while(true)
                        {
                            rxLength = recvfrom(sockFD, rxBuffer, fSizeTotal, 0, NULL, NULL);
                            if(rxLength>0)
                            {
                                if(strncmp(rxData,param.c_str(),param.length())==0)
                                {
                                    fRX++;
                                    break;
                                }
                            }
                        }
                    } 

                } // End of <=bTXSpeed

            } else { // 1 second has passed
              bTXSpeedLast = 0;
              clock_gettime(CLOCK_MONOTONIC_RAW, &txLimit);

            } // End of txLimit.tv_sec<1

        }


    } else { // Else, if we aren't testing with a speed restriction:


        // Get the initial starting time
        clock_gettime(CLOCK_MONOTONIC_RAW, &durTimer1);

        // Main TX loop
        while (*testBase<=*testMax)
        {

            // Get the current time
            clock_gettime(CLOCK_MONOTONIC_RAW, &durTimer2);

            // One second has passed
            if((durTimer2.tv_sec-durTimer1.tv_sec)>=1)
            {
                sElapsed+=1;
                bSpeed = (((float)bTX-(float)bTXlast)*8)/1000/1000;
                bTXlast = bTX;
                cout << sElapsed << "\t\t" << bSpeed << "\t" << (bTX/1000)/1000 << "\t\t"
                     << (fTX-fTXlast) << "\t\t"<< fTX << endl;
                fTXlast = fTX;
                durTimer1.tv_sec = durTimer2.tv_sec;
                durTimer1.tv_nsec = durTimer2.tv_nsec;
            }

            ss.clear();
            ss.str("");
            ss << "etheratetest" << (fTX+1);
            param = ss.str();
            strncpy(txData,param.c_str(), param.length());
            sendResult = sendto(sockFD, txBuffer, fSize+headersLength, 0, 
       	          (struct sockaddr*)&socket_address, sizeof(socket_address));
            fTX+=1;
            bTX+=fSize;
            if(ACKMode)
            {
                ss.clear();
                ss.str("");
                ss << "etherateack" << fTX;
                param = ss.str();
                while(true)
                {
                    rxLength = recvfrom(sockFD, rxBuffer, fSizeTotal, 0, NULL, NULL);
                    if(rxLength>0)
                    {
                        if(strncmp(rxData,param.c_str(),param.length())==0)
                        {
                            fRX++;
                            break;
                        }
                    }
                }
            }

        } // End of non-speed-restricted test


    } // End of max test-speed If


    cout << sElapsed << "\t\t" << bSpeed << "\t" << (bTX/1000)/1000 << "\t\t" << (fTX-fTXlast)
         << "\t\t" << fTX << endl;

    timeNow = time(0);
    localtm = localtime(&timeNow);
    cout << endl << "Ending test on " << asctime(localtm) << endl;


// Else, we are in RX mode
} else {

    cout << "Seconds\t\tMbps RX\t\tMBs Rx\t\tFrmRX/s\t\tFrames RX" << endl;

    long long *testBase, *testMax;                     //////// DOUBLE CHECK PORTABILITY OF LONG LONG AND x86 COMPATABILITY

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
    clock_gettime(CLOCK_MONOTONIC_RAW, &durTimer1);
    // Main RX loop
    while (*testBase<=*testMax)
    {

        clock_gettime(CLOCK_MONOTONIC_RAW, &durTimer2);
        // One second has passed
        if((durTimer2.tv_sec-durTimer1.tv_sec)>=1) {
            sElapsed+=1;
            bSpeed = ((bRX-bRXlast)*8)/1000/1000;
            bRXlast = bRX;
            cout << sElapsed << "\t\t" << bSpeed << "\t" << (bRX/1000)/1000 << "\t\t"
                 << (fRX-fRXlast) << "f/s\t\t" << fRX << endl;
            fRXlast = fRX;
            durTimer1.tv_sec = durTimer2.tv_sec;
            durTimer1.tv_nsec = durTimer2.tv_nsec;
        }

        // Receive incoming frames
        rxLength = recvfrom(sockFD, rxBuffer, fSizeTotal, 0, NULL, NULL);
        if(rxLength>0)
        {

            // Check if this is an etherate test frame
            param = "etheratetest";
            if(strncmp(rxData,param.c_str(),param.length())==0)
            {

                // Update our stats
                fRX+=1;
                bRX+= (rxLength-14);

                // If running in ACK mode we need to ACK to TX host
                if(ACKMode)
                {
                  ss.clear();
                  ss.str("");
                  ss << "etherateack" << fRX;
                  param = ss.str();
                  strncpy(txData,param.c_str(), param.length());
                  sendResult = sendto(sockFD, txBuffer, fSizeTotal, 0, 
         	               (struct sockaddr*)&socket_address, sizeof(socket_address));
                }

                rxLength = 0;
            }

            // Check if the other end has quit;
            param = "etheratedeath";
            if(strncmp(rxData,param.c_str(),param.length())==0)
            {
                timeNow = time(0);
                localtm = localtime(&timeNow);
                cout << "TX host is going down. Ending test and resetting on " << asctime(localtm) << endl << endl;
                goto start;
            }

        }

        if (rxLength == -1) { 
            cout << "Error receiving frame" << endl;
            perror("recvfrom() ");
        }

    }

    cout << sElapsed << "\t\t" << bSpeed << "\t" << (bRX/1000)/1000 << "\t\t" << (fRX-fRXlast)
         << "\t\t" << fRX << endl;
    timeNow = time(0);
    localtm = localtime(&timeNow);
    cout << endl << "Ending test on " << asctime(localtm) << endl;
    close(sockFD);
    goto start;

}

cout << "Leaving promiscuous mode" << endl;

strncpy(ethreq.ifr_name,ifName,IFNAMSIZ);

if (ioctl(sockFD,SIOCGIFFLAGS,&ethreq)==-1)
{
    cout << "Error getting socket flags, entering promiscuous mode failed." << endl;
    perror("ioctl() ");
    close(sockFD);
    return 70;
}

ethreq.ifr_flags &= ~IFF_PROMISC;

if (ioctl(sockFD,SIOCSIFFLAGS,&ethreq)==-1)
{
    cout << "Error setting socket flags, promiscuous mode failed." << endl;
    perror("ioctl() ");
    close(sockFD);
    return 70;
}

close(sockFD);


/*
********************************************************************************************** End tests
*/


 return EXIT_SUCCESS;

} // End of main()


