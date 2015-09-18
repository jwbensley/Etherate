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
 * File Contents:
 * Global Constants
 * Global Variables
 *
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

// General application parameters
struct APP_PARAMS
{

    bool TX_MODE; // Default mode is TX
    bool TX_SYNC; // Sync settings between hosts by default

    time_t TS_NOW;     // Time type for holding the current date and time
    tm* TM_LOCAL;      // Time struct for breaking down the above time type

};

/*
 * These are the minimun declarations required to send a frame; 
 * They have been moved into this global variable space. This is because they
 * are being used by the signal handler function signal_handler() to send the
 * dying gasp frame upon program exit()
 */

/////unsigned char SOURCE_MAC[6];
/////unsigned char DESTINATION_MAC[6];
/////int ETHERTYPE;
/////char* TX_BUFFER;
/////char* TX_DATA;
/////unsigned char* TX_ETHERNET_HEADER;
struct ifreq ethreq;
/////int ETH_HEADERS_LEN;
/////int SOCKET_FD;
/////struct sockaddr_ll socket_address;
/////char IF_NAME[IFNAMSIZ];
/////int TX_RET_VAL;

// Frame header settings
struct FRAME_HEADERS
{
    unsigned char SOURCE_MAC[6];
    unsigned char DESTINATION_MAC[6];
    int ETHERTYPE;
    int LENGTH;
    short PCP;
    short VLAN_ID;
    short QINQ_ID;
    short QINQ_PCP;
    char* RX_BUFFER;
    char* RX_DATA;
    char* TX_BUFFER;
    char* TX_DATA;
};

// A global pointer is needed for the SIGINT catch, signal_handler()
struct FRAME_HEADERS *pFRAME_HEADERS;


// Settings for the physical interface used for testing
struct TEST_INTERFACE
{
	int IF_INDEX;
    char IF_NAME[IFNAMSIZ];
    struct sockaddr_ll SOCKET_ADDRESS;
    
    int SOCKET_FD;             // Used for sending frames
    fd_set FD_READS;           // A set of socket file descriptors for polling with select()
    int SOCKET_FD_COUNT;       // FD count and ret val for polling with select()
    int SELECT_RET_VAL;

    struct timeval TV_SELECT_DELAY; // Elapsed time struct for polling the socket file descriptor

};

// A global pointer is needed for the SIGINT catch, signal_handler()
struct TEST_INTERFACE *pTEST_INTERFACE;


// Gerneral testing parameters
struct TEST_PARAMS
{

    int F_SIZE;                // Frame payload in bytes
    int F_SIZE_TOTAL;          // Total frame size including headers
    long long F_DURATION;      // Maximum duration in seconds
    long long F_COUNT;         // Maximum number of frames to send
    long long F_BYTES;         // Maximum amount of data to transmit in bytes
    long B_TX_SPEED_MAX;       // Maximum speed to transmit at (max bytes per second)
    long B_TX_SPEED_PREV;      // Transmission speed for the previous second
    timespec TS_TX_LIMIT;      // Timer used for rate limiting the TX host
    long long S_ELAPSED;       // Seconds the test has been running

    long long F_TX_COUNT;      // Total number of frames transmitted
    long long F_TX_COUNT_PREV; // Total number of frames sent up to one second ago
    long long B_TX;            // Total number of bytes transmitted
    long long B_TX_PREV;       // Bytes sent up to one second ago
    long long F_RX_COUNT;      // Total number of frames received
    long long F_RX_COUNT_PREV; // Total number of frames received up to one second ago
    long F_RX_OTHER;           // Number of non test frames received
    long long B_RX;            // Total number of bytes received
    long long B_RX_PREV;       // Bytes received up to one second ago

    long long F_INDEX;         // Index of the current test frame sent/received
    long long F_INDEX_PREV;    // Index of the last test frame sent/received
    long long F_RX_ONTIME;     // Frames received on time
    long long F_RX_EARLY;      // Frames received out of order that are early
    long long F_RX_LATE;       // Frames received out of order that are late        
    
    float B_SPEED;             // Current speed

    bool F_ACK;                // Testing in ACK mode during transmition
    bool F_WAITING_ACK;        // Test is waiting for a frame to be ACK'ed
    timespec TS_ACK_TIMEOUT;   // Timeout to wait for a frame ACK
    
    timespec TS_CURRENT_TIME;  // Two timers for timing a test and calculating stats
    timespec TS_ELAPSED_TIME;

};


// Settings specific to the MTU sweet test
struct MTU_TEST {

    bool ENABLED;              // Enable the MTU sweep test mode
    
    int MTU_TX_MIN;            // Default minmum MTU size
    int MTU_TX_MAX;            // Default maximum MTU size

};


// Settings specific to the quality measurement test
struct QM_TEST {

    bool ENABLED;              // Enable the quality measurement tests

    int INTERVAL;              // Default echo interval in milliseconds
    long INTERVAL_SEC;
    long INTERVAL_NSEC;
    int TIMEOUT;               // Default timeout in milliseconds
    long TIMEOUT_NSEC;
    long TIMEOUT_SEC;

    int DELAY_TEST_COUNT;      // Number of one way delay measurements to account for interrupt coalescing
    double *pDELAY_RESULTS;    // Store Tx to Rx test results
    timespec TS_RTT;           // Timespec used for calculating delay/RTT

};
