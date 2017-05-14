/*
 * License: MIT
 *
 * Copyright (c) 2012-2017 James Bensley.
 *
 * Permission is hereby granted, free of uint8_tge, to any person obtaining
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
 * GLOBAL MACROS
 * GLOBAL CONSTANTS
 * GLOBAL DEFINITIONS
 *
 */



/*
 **************************************************************** GLOBAL MACROS
 */

#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)       (x)
#define unlikely(x)     (x)
#endif

// HtoN and NtoH methods for ull values
#if (__BYTE_ORDER == __BIG_ENDIAN)
#define ntohll(val) return (val)
#define htonll(val) return (val)
#else
#define ntohll(val) __bswap_64(val)
#define htonll(val) __bswap_64(val)
#endif



/*
 ************************************************************* GLOBAL CONSTANTS
 */

#define APP_VERSION        "0.12.beta 2017-05"
#define F_SIZE_MAX         10000 // Maximum frame size on the wire (payload+headers)
#define F_SIZE_DEF         1500  // Default frame payload size in bytes
#define F_DURATION_DEF     30    // Default test duration in seconds
#define F_COUNT_DEF        0     // Default total number of frames to transmit
#define F_BYTES_DEF        0     // Default amount of data to transmit in bytes
#define B_TX_SPEED_MAX_DEF 0     // Default max speed in bytes, 0 == no limit
#define F_TX_SPEED_MAX_DEF 0     // Default max frames per second, 0 == no limit
#define PCP_DEF            0     // Default PCP value
#define VLAN_ID_DEF        0     // Default VLAN ID
#define QINQ_ID_DEF        0     // Default QinQ VLAN ID
#define QINQ_PCP_DEF       0     // Default QinQ PCP value
#define MPLS_LABELS_MAX    10    // Maximum number of MPLS labels
#define HEADERS_LEN_DEF    14    // Default frame headers length
#define ETHERTYPE_DEF      13107 // Default Ethertype (0x3333)
#define IF_INDEX_DEF       -1    // Default interface index number
#define SOCKET_FD_DEF      -1    // Default socket fd
#define TX_DELAY_DEF       1     // Default TX to RX delay check
#define RET_EXIT_APP       -2    // Used to exit the app even though success
#define RET_EXIT_FAILURE   -1    // EXIT_FAILURE but a negative value


/*
 *********************************************************** GLOBAL DEFINITIONS
 */

// TLV and sub-TLV types/values
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


struct app_params              // General application parameters
{

    uint8_t BROADCAST;    // Default, broadcast local source MAC before start
    uint8_t TX_MODE;      // Default, mode is TX
    uint8_t TX_SYNC;      // Default, sync settings between hosts
    uint8_t TX_DELAY;     // Default, measure one way delay TX > RX
    time_t  TS_NOW;       // Current date and time
    struct  tm* TM_LOCAL; // For breaking down the above

};


struct frame_headers           // Frame header settings
{

    uint8_t   SOURCE_MAC[6];
    uint8_t   DEST_MAC[6];
    uint16_t  ETHERTYPE;
    uint16_t  LENGTH;
    uint16_t  PCP;
    uint16_t  VLAN_ID;
    uint8_t   VLAN_DEI;
    uint16_t  QINQ_ID;
    uint16_t  QINQ_PCP;
    uint8_t   QINQ_DEI;
    uint8_t   LSP_SOURCE_MAC[6];
    uint8_t   LSP_DEST_MAC[6];
    uint16_t  MPLS_LABELS;
    uint32_t  MPLS_LABEL[MPLS_LABELS_MAX];
    uint16_t  MPLS_EXP[MPLS_LABELS_MAX];
    uint16_t  MPLS_TTL[MPLS_LABELS_MAX];
    uint8_t   PWE_CONTROL_WORD;
    
    uint8_t*  RX_BUFFER;      // Full frame including headers
    uint8_t*  RX_DATA;        // Pointer to frame payload after the headers
    uint8_t*  TX_BUFFER;      // Full frame including headers
    uint8_t*  TX_DATA;        // Pointer to frame payload after the headers
    uint32_t  TLV_SIZE;
    uint16_t* RX_TLV_TYPE;    // TLV type within frame headers
    uint32_t* RX_TLV_VALUE;   // TLV value within the frame header
    uint32_t  SUB_TLV_SIZE;
    uint16_t* RX_SUB_TLV_TYPE;
    uint64_t* RX_SUB_TLV_VALUE;

};


struct test_interface          // Settings for the physical test interface
{

    int     IF_INDEX;
    uint8_t IF_NAME[IFNAMSIZ];
    struct  sockaddr_ll SOCKET_ADDRESS;
    int     SOCKET_FD;                  // Used for sending frames
    int     SELECT_RET_VAL;
    struct  timeval TV_SELECT_DELAY;    // Elapsed time struct for polling the socket FD

};


struct test_params             // Gerneral testing parameters
{

    uint16_t    F_SIZE;          // Frame payload in bytes
    uint16_t    F_SIZE_TOTAL;    // Total frame size including headers
    uint64_t    F_DURATION;      // Maximum duration in seconds
    uint64_t    F_COUNT;         // Maximum number of frames to send
    uint64_t    F_BYTES;         // Maximum amount of data to transmit in bytes
    uint8_t*    F_PAYLOAD;       // Custom payload loaded from file
    uint16_t    F_PAYLOAD_SIZE;  // Size of the custom payload  from file
    uint32_t    B_TX_SPEED_MAX;  // Maximum transmit speed in bytes per second
    uint32_t    B_TX_SPEED_PREV; // Transmission speed for the previous second
    uint64_t    S_ELAPSED;       // Seconds the test has been running
    uint64_t    F_TX_COUNT;      // Total number of frames transmitted
    uint64_t    F_TX_COUNT_PREV; // Total number of frames sent one second ago
    uint32_t    F_TX_SPEED_MAX;  // Maximum transmit speed in frames per second
    uint64_t    F_SPEED;         // Current frames per second during a test
    uint64_t    F_SPEED_AVG;     // Average frames per second during a test
    uint64_t    F_SPEED_MAX;     // Max frames per second achieved during the test
    uint64_t    B_TX;            // Total number of bytes transmitted
    uint64_t    B_TX_PREV;       // Bytes sent up to one second ago
    uint64_t    F_RX_COUNT;      // Total number of frames received
    uint64_t    F_RX_COUNT_PREV; // Total number of frames received one second ago
    uint64_t    F_RX_OTHER;      // Number of non test frames received
    uint64_t    B_RX;            // Total number of bytes received
    uint64_t    B_RX_PREV;       // Bytes received one second ago
    uint64_t    F_INDEX_PREV;    // Index of the last test frame sent/received
    uint64_t    F_RX_ONTIME;     // Frames received on time
    uint64_t    F_RX_EARLY;      // Frames received out of order that are early
    uint64_t    F_RX_LATE;       // Frames received out of order that are late
    double      B_SPEED;         // Current speed
    double      B_SPEED_MAX;     // Maximum speed achieved during the test
    long double B_SPEED_AVG;     // Average speed achieved during the test
    uint8_t     F_ACK;           // Testing in ACK mode during transmition
    uint8_t     F_WAITING_ACK;   // Test is waiting for a frame to be ACK'ed
    struct timespec TS_CURRENT_TIME; // Two timers for timing a test and calculating stats
    struct timespec TS_ELAPSED_TIME;

};


struct mtu_test {              // Settings specific to the MTU sweep test

    uint8_t  ENABLED;     // Enable the MTU sweep test mode
    uint16_t MTU_TX_MIN;  // Default minmum MTU size
    uint16_t MTU_TX_MAX;  // Default maximum MTU size

};


struct qm_test {               // Settings specific to the quality measurement test

    uint8_t       ENABLED;          // Enable the quality measurement tests
    uint32_t      DELAY_TEST_COUNT; // Number of one way delay measurements
    uint32_t      INTERVAL;         // Default echo interval in milliseconds
    signed long   INTERVAL_SEC;
    signed long   INTERVAL_NSEC;
    long double   INTERVAL_MIN;
    long double   INTERVAL_MAX;
    long double   JITTER_MIN;
    long double   JITTER_MAX;
    long double   RTT_MIN;
    long double   RTT_MAX;
    uint32_t      TEST_COUNT;       // Number of latency tests made
    double        *TIME_RX_1;       // These values are used to calculate the
    double        *TIME_RX_2;       // delay between TX and RX hosts
    double        *TIME_RX_DIFF;
    double        *TIME_TX_1;
    double        *TIME_TX_2;
    double        *TIME_TX_DIFF;
    uint32_t      TIMEOUT;          // Default timeout in milliseconds
    signed long   TIMEOUT_NSEC;
    signed long   TIMEOUT_SEC;
    uint32_t      TIMEOUT_COUNT;
    double        *pDELAY_RESULTS;  // Store Tx to Rx test results
    struct timespec TS_RTT;           // Timespec used for calculating delay/RTT
    struct timespec TS_START;         // Time the test was started

};


// These need to be global so that signal_handler() can send a dying gasp
// and the allocated buffers can be free()'d
struct ifreq ethreq;
struct qm_test *pqm_test;
struct test_interface *ptest_interface;
struct test_params *ptest_params;
struct frame_headers *pframe_headers;