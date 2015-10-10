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
 * File: Etherate Global Code
 *
 * File Contents:
 * Global Constants
 * Global Definitions
 *
 */



/*
 ************************************************************* GLOBAL CONSTANTS
 */

const char VERSION[20] = "0.7.beta 2015-10";

// Maximum frame size on the wire (payload + all headers headers) Etherate
// will support, this is hard coded here because send and receive buffers
// have to allocated
const unsigned int       F_SIZE_MAX         = 10000;
const unsigned int       F_SIZE_DEF         = 1500;  // Default frame payload size in bytes
const unsigned long long F_DURATION_DEF     = 30;    // Default test duration in seconds
const unsigned long      F_COUNT_DEF        = 0;     // Default total number of frames to transmit
const unsigned long      F_BYTES_DEF        = 0;     // Default amount of data to transmit in bytes
const unsigned long      B_TX_SPEED_MAX_DEF = 0;     // Default max speed in bytes, 0 == no limit
const unsigned short     PCP_DEF            = 0;     // Default PCP value
const unsigned short     VLAN_ID_DEF        = 0;     // Default VLAN ID
const unsigned short     QINQ_ID_DEF        = 0;     // Default QinQ VLAN ID
const unsigned short     QINQ_PCP_DEF       = 0;     // Default QinQ PCP value
const unsigned int       HEADERS_LEN_DEF    = 14;    // Default frame headers length
const unsigned short     ETHERTYPE_DEF      = 2048;  // Default ETHERTYPE (0x0800 == IPv4)
const unsigned int       IF_INDEX_DEF       = -1;    // Default interface index number
const unsigned char      TX_DELAY_DEF       = true;  // Default TX to RX delay check



/*
 *********************************************************** GLOBAL DEFINITIONS
 */



#define TYPE_APPLICATION         1
#define TYPE_APPLICATION_SUB_TLV 11
#define VALUE_DYINGGASP          101
#define VALUE_DUMMY              102

#define TYPE_BROADCAST           2
#define VALUE_PRESENCE           21

#define TYPE_SETTING             3
#define VALUE_SETTING_SUB_TLV    30
#define VALUE_SETTING_END        31
#define TYPE_ETHERTYPE           301
#define TYPE_FRAMESIZE           302
#define TYPE_DURATION            303
#define TYPE_FRAMECOUNT          304
#define TYPE_BYTECOUNT           305
#define TYPE_MAXSPEED            306
#define TYPE_VLANPCP             307
#define TYPE_QINQPCP             308
#define TYPE_ACKMODE             309
#define TYPE_MTUTEST             310
#define TYPE_MTUMIN              311
#define TYPE_MTUMAX              312
#define TYPE_QMTEST              313
#define TYPE_QMINTERVAL          314
#define TYPE_QMTIMEOUT           315
#define TYPE_TXDELAY             316

#define TYPE_TESTFRAME           4
#define VALUE_TEST_SUB_TLV       40
#define VALUE_DELAY_END          41
#define TYPE_FRAMEINDEX          401
#define TYPE_ACKINDEX            402
#define TYPE_DELAY               403
#define TYPE_DELAY1              404
#define TYPE_DELAY2              405
#define TYPE_PING                406
#define TYPE_PONG                407


struct APP_PARAMS              // General application parameters
{

    char         TX_MODE;               // Default, mode is TX
    char         TX_SYNC;               // Default, Sync settings between hosts
    char         TX_DELAY;              // Default, measure one way delay TX > RX
    time_t       TS_NOW;                // Current date and time
    tm*          TM_LOCAL;              // For breaking down the above
    unsigned int STATS_DELAY;           // SCreen update frequently in seconds

};


struct FRAME_HEADERS           // Frame header settings
{

    unsigned char       SOURCE_MAC[6];
    unsigned char       DESTINATION_MAC[6];
    unsigned short      ETHERTYPE;
    unsigned short      LENGTH;
    unsigned short      PCP;
    unsigned short      VLAN_ID;
    unsigned short      QINQ_ID;
    unsigned short      QINQ_PCP;
    char*               RX_BUFFER;
    char*               RX_DATA;
    char*               TX_BUFFER;
    char*               TX_DATA;
    unsigned int        TLV_SIZE;
    unsigned short*     RX_TLV_TYPE;
    unsigned long*      RX_TLV_VALUE;
    unsigned int        SUB_TLV_SIZE;
    unsigned short*     RX_SUB_TLV_TYPE;
    unsigned long long* RX_SUB_TLV_VALUE;
};


struct TEST_INTERFACE          // Settings for the physical test interface
{

	int    IF_INDEX;
    char   IF_NAME[IFNAMSIZ];
    struct sockaddr_ll SOCKET_ADDRESS;
    int    SOCKET_FD;                   // Used for sending frames
    fd_set FD_READS;                    // Socket file descriptors for polling with select()
    int    SOCKET_FD_COUNT;             // FD count and ret val for polling with select()
    int    SELECT_RET_VAL;
    struct timeval TV_SELECT_DELAY;     // Elapsed time struct for polling the socket FD

};


struct TEST_PARAMS             // Gerneral testing parameters
{

    unsigned short     F_SIZE;          // Frame payload in bytes
    unsigned short     F_SIZE_TOTAL;    // Total frame size including headers
    unsigned long long F_DURATION;      // Maximum duration in seconds
    unsigned long long F_COUNT;         // Maximum number of frames to send
    unsigned long long F_BYTES;         // Maximum amount of data to transmit in bytes
    unsigned long      B_TX_SPEED_MAX;  // Maximum transmit speed in bytes per second
    unsigned long      B_TX_SPEED_PREV; // Transmission speed for the previous second
    timespec           TS_TX_LIMIT;     // Timer used for rate limiting the TX host
    unsigned long long S_ELAPSED;       // Seconds the test has been running
    unsigned long long F_TX_COUNT;      // Total number of frames transmitted
    unsigned long long F_TX_COUNT_PREV; // Total number of frames sent one second ago
    unsigned long long B_TX;            // Total number of bytes transmitted
    unsigned long long B_TX_PREV;       // Bytes sent up to one second ago
    unsigned long long F_RX_COUNT;      // Total number of frames received
    unsigned long long F_RX_COUNT_PREV; // Total number of frames received one second ago
    unsigned long long F_RX_OTHER;      // Number of non test frames received
    unsigned long long B_RX;            // Total number of bytes received
    unsigned long long B_RX_PREV;       // Bytes received one second ago
    unsigned long long F_INDEX_PREV;    // Index of the last test frame sent/received
    unsigned long long F_RX_ONTIME;     // Frames received on time
    unsigned long long F_RX_EARLY;      // Frames received out of order that are early
    unsigned long long F_RX_LATE;       // Frames received out of order that are late        
    double             B_SPEED;         // Current speed
    double             B_SPEED_MAX;     // The maximum speed achieved during the test
    long double        B_SPEED_AVG;     // The average speed achieved during the test
    char               F_ACK;           // Testing in ACK mode during transmition
    char               F_WAITING_ACK;   // Test is waiting for a frame to be ACK'ed
    timespec           TS_CURRENT_TIME; // Two timers for timing a test and calculating stats
    timespec           TS_ELAPSED_TIME;

};


struct MTU_TEST {              // Settings specific to the MTU sweep test

    char          ENABLED;              // Enable the MTU sweep test mode
    unsigned int  MTU_TX_MIN;           // Default minmum MTU size
    unsigned int  MTU_TX_MAX;           // Default maximum MTU size

};


struct QM_TEST {               // Settings specific to the quality measurement test

    char          ENABLED;              // Enable the quality measurement tests
    unsigned int  INTERVAL;             // Default echo interval in milliseconds
    unsigned long INTERVAL_SEC;
    unsigned long INTERVAL_NSEC;
    long double   INTERVAL_MIN;
    long double   INTERVAL_MAX;
    unsigned int  TIMEOUT;              // Default timeout in milliseconds
    unsigned long TIMEOUT_NSEC;
    unsigned long TIMEOUT_SEC;
    unsigned long TIMEOUT_COUNT;
    unsigned int  DELAY_TEST_COUNT;     // Number of one way delay measurements
    long double   RTT_MIN;
    long double   RTT_MAX;
    long double   JITTER_MIN;
    long double   JITTER_MAX;
    double        *pDELAY_RESULTS;      // Store Tx to Rx test results
    timespec      TS_RTT;               // Timespec used for calculating delay/RTT
    timespec      TS_START;             // Time the test was started

};


// These need to be global so that signal_handler() can send a dying gasp
// and the allocated buffers can be free()'d
struct ifreq ethreq;
struct QM_TEST *pQM_TEST;
struct TEST_INTERFACE *pTEST_INTERFACE;
struct FRAME_HEADERS *pFRAME_HEADERS;