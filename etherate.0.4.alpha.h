/*
 * Etherate Global Definitions
 * 
 *
 * File Contents:
 *
 * GLOBAL FUNCTIONS
 * GLOBAL CONSTANTS
 * GLOBAL VARIABLES
 *
 */


/*
 ********************************************************************************************** Global functions
 */

// Signal handler to notify remote host of local application termiantion
void signal_handler(int signal);

// Print CLI args and usage
void PrintUsage();

// Explode a string into an array using a seperator value
void StringExplode(string str, string separator, vector<string>* results);

// Try to open the passed socket on an interface
int GetSockInterface(int &sockFD);

// List interfaces and hardware (MAC) address
void ListInterfaces();

// Build the Ethernet headers for sending frames
void BuildHeaders(char* &txBuffer, unsigned char (&destMAC)[6], 
                  unsigned char (&sourceMAC)[6], short &PCP, short &vlanID,
                  short &qinqID, short &qinqPCP, int &headersLength);


/*
 ********************************************************************************************** Global constants
 */

const string version = "0.4.alpha 2014-02";

// Maximum frame size on the with (payload + headers)
const int fSizeMax = 9020;

// Default frame size in bytes
const int fSizeDef = 1500;

// Default duration in seconds
const long fDurationDef = 30;

// Default total number of frames to transmit
const long fCountDef = 0; // 83.3k frames at 1Gbps is 1 second

// Default amount of data to transmit in bytes
const long fBytesDef = 0;

// Default max speed, 0 == no limit
const long bTXSpeedDef = 0;

// Default PCP value
const int PCPDef = 0;

// Default VLAN ID
const int vlanIDDef = 0;

// Default QinQ VLAN ID
const int qinqIDDef = 0;

// Default QinQ PCP value
const int qinqPCPDef = 0;


/*
 ********************************************************************************************** Global variables
 */


/*
 * These are the minimun declarations required to send a frame; 
 * They have been moved into the global variable space. This is
 * because they are being used by the signal handler function
 * signal_handler() to send one last "dying gasp" frame upon
 * program exit().
 */

unsigned char sourceMAC[6];
unsigned char destMAC[6];
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
 * Optional TX/Rx variables
 */

// Default 802.1p PCP/CoS value = 0
short PCP = 0;

// Default 802.1q VLAN ID = 0
short vlanID = 0;

// Default 802.1ad VLAN ID of QinQ outer frame = 0
short qinqID = 0;

// Default 802.1p PCP/CoS value of outer frame = 0
short qinqPCP = 0;

// Is the test running
bool testing = 0;