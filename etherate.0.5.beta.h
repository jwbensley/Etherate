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
 * File: Etherate Global Definitions
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
 * GLOBAL FUNCTIONS
 * GLOBAL CONSTANTS
 * GLOBAL VARIABLES
 *
 */


/*
 ************************************************************* GLOBAL FUNCTIONS
 */
 

// Build the Ethernet headers for sending frames
void build_headers(char* &TX_BUFFER, unsigned char (&DESTINATION_MAC)[6], 
     unsigned char (&SOURCE_MAC)[6], int &ETHERTYPE, short &PCP,
     short &VLAN_ID, short &QINQ_ID, short &QINQ_PCP, int &ETH_HEADERS_LEN);

// Get the MTU of the interface used for the test
int get_interface_mtu_by_name(int &SOCKET_FD, char &IF_NAME);

// Try to automatically chose an interface to run the test on
int get_sock_interface(int &SOCKET_FD);

// List interfaces and hardware (MAC) address
void list_interfaces();

// Print common CLI examples
void print_examples ();

// Print CLI args and usage
void print_usage();

// Try to open the passed socket on a user specified interface by index
int set_sock_interface_index(int &SOCKET_FD, int &IF_INDEX);

// Try to open the passed socket on a user specified interface by name
int set_sock_interface_name(int &SOCKET_FD, char &IF_NAME);

// Signal handler to notify remote host of local application termiantion
void signal_handler(int signal);

// Explode a string into an array using a seperator value
void string_explode(string str, string separator, vector<string>* results);



/*
 ************************************************************* GLOBAL FUNCTIONS
 */




/*
 ************************************************************* GLOBAL CONSTANTS
 */


const string version = "0.5.beta 2015-07";

// Maximum frame size on the wire (payload + 22 octets for QinQ headers)
// Etherate will support, this is hard coded here because we have to allocate
// send and receive buffers. 
const int F_SIZE_MAX = 10000;

// Default frame payload size in bytes
const int F_SIZE_DEF = 1500;

// Default duration in seconds
const long F_DURATION_DEF = 30;

// Default total number of frames to transmit
const long F_COUNT_DEF = 0;

// Default amount of data to transmit in bytes
const long F_BYTES_DEF = 0;

// Default max speed in bytes, 0 == no limit
const long B_TX_SPEED_MAX_DEF = 0;

// Default PCP value
const short PCP_DEF = 0;

// Default VLAN ID
const short VLAN_ID_DEF = 0;

// Default QinQ VLAN ID
const short QINQ_ID_DEF = 0;

// Default QinQ PCP value
const short QINQ_PCP_DEF = 0;

// Default frame headers length
const int HEADERS_LEN_DEF = 14;

// Default ETHERTYPE (0x0800 == IPv4)
const int ETHERTYPE_DEF = 2048;

// Default interface index number
const int IF_INDEX_DEF = -1;


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

unsigned char SOURCE_MAC[6];
unsigned char DESTINATION_MAC[6];
int ETHERTYPE;
char* TX_BUFFER;
char* TX_DATA;
unsigned char* TX_ETHERNET_HEADER;
struct ifreq ethreq;
int ETH_HEADERS_LEN;
int SOCKET_FD;
struct sockaddr_ll socket_address;
char IF_NAME[IFNAMSIZ];
int TX_REV_VAL;


/*
 * Optional Tx/Rx variables
 */

// Default 802.1p PCP/CoS value = 0
short PCP;

// Default 802.1q VLAN ID = 0
short VLAN_ID;

// Default 802.1ad VLAN ID of QinQ outer frame = 0
short QINQ_ID;

// Default 802.1p PCP/CoS value of outer frame = 0
short QINQ_PCP;


/*
 ************************************************************* GLOBAL VARIABLES
 */
