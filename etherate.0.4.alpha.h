/*
 * Etherate Global Definitions
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
 * GLOBAL FUNCTIONS
 * GLOBAL CONSTANTS
 * GLOBAL VARIABLES
 *
 */


/*
 ************************************************************* GLOBAL FUNCTIONS
 */
 

// Signal handler to notify remote host of local application termiantion
void signal_handler(int signal);

// Print CLI args and usage
void print_usage();

// Explode a string into an array using a seperator value
void string_explode(string str, string separator, vector<string>* results);

// Try to automatically chose an interface to run the test on
int get_sock_interface(int &sockFD);

// Try to open the passed socket on a user specified interface by index
int set_sock_interface_index(int &sockFD, int &ifIndex);

// Try to open the passed socket on a user specified interface by name
int set_sock_interface_name(int &sockFD, char &ifName);

// List interfaces and hardware (MAC) address
void list_interfaces();

// Build the Ethernet headers for sending frames
void build_headers(char* &txBuffer, unsigned char (&destMAC)[6], 
     unsigned char (&sourceMAC)[6], int &ethertype, short &PCP,
     short &vlanID, short &qinqID, short &qinqPCP, int &headersLength);


/*
 ************************************************************* GLOBAL FUNCTIONS
 */




/*
 ************************************************************* GLOBAL CONSTANTS
 */


const string version = "0.4.alpha 2014-08";

// Maximum frame size on the wire (payload + 22 octets for QinQ headers)
// Etherate will support, this is hard coded here because we have to allocate
// send and receive buffers. 
const int fSizeMax = 9022;

// Default frame payload size in bytes
const int fSizeDef = 1500;

// Default duration in seconds
const long fDurationDef = 30;

// Default total number of frames to transmit
const long fCountDef = 0;

// Default amount of data to transmit in bytes
const long fBytesDef = 0;

// Default max speed, 0 == no limit
const long bTXSpeedDef = 0;

// Default PCP value
const short PCPDef = 0;

// Default VLAN ID
const short vlanIDDef = 0;

// Default QinQ VLAN ID
const short qinqIDDef = 0;

// Default QinQ PCP value
const short qinqPCPDef = 0;

// Default frame headers length
const int headersLengthDefault = 14;

// Default ethertype (0x0800 == IPv4)
const int ethertypeDefault = 2048;

// Default interface index number
const int ifIndexDefault = -1;


/*
 ************************************************************* GLOBAL CONSTANTS
 */




/*
 ************************************************************* GLOBAL VARIABLES
 */


/*
 * These are the minimun declarations required to send a frame; 
 * They have been moved into this global variable space. This is because they
 * are being used by the signal handler function signal_handler() to send the
 * dying gasp frame upon program exit()
 */

unsigned char sourceMAC[6];
unsigned char destMAC[6];
int ethertype;
char* txBuffer;
unsigned char* txEtherhead;
struct ifreq ethreq;
int headersLength;
char* txData;
int sockFD;
struct sockaddr_ll socket_address;
char ifName[IFNAMSIZ];
int sendResult;


/*
 * Optional Tx/Rx variables
 */

// Default 802.1p PCP/CoS value = 0
short PCP;

// Default 802.1q VLAN ID = 0
short vlanID;

// Default 802.1ad VLAN ID of QinQ outer frame = 0
short qinqID;

// Default 802.1p PCP/CoS value of outer frame = 0
short qinqPCP;


/*
 ************************************************************* GLOBAL VARIABLES
 */
