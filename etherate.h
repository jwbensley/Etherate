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

#define APP_VERSION        "1.15 2017-10"
#define F_SIZE_MAX         10000 // Maximum frame size on the wire (payload+headers)
#define F_SIZE_DEF         1500  // Default frame payload size in bytes
#define F_DURATION_DEF     30    // Default test duration in seconds
#define F_COUNT_DEF        0     // Default total number of frames to transmit
#define F_BYTES_DEF        0     // Default amount of data to transmit in bytes
#define B_TX_SPEED_MAX_DEF 0     // Default max speed in bytes, 0 == no limit
#define F_TX_COUNT_MAX_DEF 0     // Default max frames per second, 0 == no limit
#define PCP_DEF            0     // Default pcp value
#define VLAN_ID_DEF        0     // Default VLAN ID
#define QINQ_ID_DEF        0     // Default QinQ VLAN ID
#define QINQ_PCP_DEF       0     // Default QinQ pcp value
#define MPLS_LABELS_MAX    10    // Maximum number of MPLS labels
#define HEADERS_LEN_DEF    14    // Default frame headers length
#define ETYPE_DEF          13107 // Default Ethertype (0x3333)
#define IF_INDEX_DEF       -1    // Default interface index number
#define SOCK_FD_DEF        -1    // Default socket fd
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
#define TYPE_ETYPE               301
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

    uint8_t broadcast;               // Default, broadcast local source MAC before start
    uint8_t tx_mode;                 // Default, mode is TX
    uint8_t tx_sync;                 // Default, sync settings between hosts
    uint8_t tx_delay;                // Default, measure one way delay TX > RX
    time_t  ts_now;                  // Current date and time
    struct  tm* tm_local;            // For breaking down the above

};


struct frame_headers           // Frame header settings
{

    uint8_t   src_mac[6];
    uint8_t   dst_mac[6];
    uint16_t  etype;
    uint16_t  length;
    uint16_t  pcp;
    uint16_t  vlan_id;
    uint8_t   vlan_dei;
    uint16_t  qinq_id;
    uint16_t  qinq_pcp;
    uint8_t   qinq_dei;
    uint8_t   lsp_src_mac[6];
    uint8_t   lsp_dst_mac[6];
    uint16_t  mpls_labels;
    uint32_t  mpls_label[MPLS_LABELS_MAX];
    uint16_t  mpls_exp[MPLS_LABELS_MAX];
    uint16_t  mpls_ttl[MPLS_LABELS_MAX];
    uint8_t   pwe_ctrl_word;
    
    uint8_t*  rx_buffer;             // Full frame including headers
    uint8_t*  rx_data;               // Pointer to frame payload after the headers
    uint8_t*  tx_buffer;             // Full frame including headers
    uint8_t*  tx_data;               // Pointer to frame payload after the headers
    uint32_t  tlv_size;              // TLV size is currently fixed!
    uint16_t* rx_tlv_type;           // TLV type within frame headers
    uint32_t* rx_tlv_value;          // TLV value within the frame header
    uint32_t  sub_tlv_size;          // Sub-TLV size is also currently fixed!
    uint16_t* rx_sub_tlv_type;
    uint64_t* rx_sub_tlv_value;

};


struct mtu_test {              // Settings specific to the MTU sweep test

    uint8_t  enabled;                // Enable the MTU sweep test mode
    uint16_t largest;                // Largest MTU ACK'ed at Tx or recieved by Rx
    uint16_t mtu_tx_min;             // Default minmum MTU size
    uint16_t mtu_tx_max;             // Default maximum MTU size

};


struct qm_test {               // Settings specific to the quality measurement test

    uint8_t         enabled;          // Enable the quality measurement tests
    uint32_t        delay_test_count; // Number of one way delay measurements
    uint32_t        interval;         // Default ping interval in milliseconds
    signed long     interval_sec;     // Delay between ping probes
    signed long     interval_nsec;
    long double     interval_min;     // Min interval during test period
    long double     interval_max;     // Max interval during test period
    long double     jitter_min;       // Min jitter during test period
    long double     jitter_max;       // Max jitter during test period
    long double     rtt_min;          // Min RTT during test period
    long double     rtt_max;          // Max RTT during test period
    uint32_t        test_count;       // Number of latency tests made
    double          *time_rx_1;       // These values are used to calculate the
    double          *time_rx_2;       // delay between TX and RX hosts:
    double          *time_rx_diff;
    double          *time_tx_1;
    double          *time_tx_2;
    double          *time_tx_diff;
    uint32_t        timeout;          // Default ping timeout in milliseconds
    signed long     timeout_nsec;     // Timeout period for ping probes
    signed long     timeout_sec;
    uint32_t        timeout_count;
    double          *delay_results;   // Store Tx to Rx test results
    struct timespec ts_rtt;           // Timespec used for calculating delay/RTT
    struct timespec ts_start;         // Time the test was started

};


struct test_interface          // Settings for the physical test interface
{

    int     if_index;
    uint8_t if_name[IFNAMSIZ];
    struct  sockaddr_ll sock_addr;   // Link-layer details
    int     sock_fd;                 // Socket file descriptor

};


struct speed_test             // Speed test parameters
{

    uint8_t     enabled;             // Enable the speed test
    uint64_t    b_rx;                // Total number of bytes received
    uint64_t    b_rx_prev;           // Bytes received one second ago
    uint64_t    b_tx;                // Total number of bytes transmitted
    uint64_t    b_tx_prev;           // Bytes sent up to one second ago
    uint64_t    b_tx_speed_max;      // Maximum transmit speed in bytes per second
    uint64_t    b_tx_speed_prev;     // Transmission speed for the previous second
    uint8_t*    f_payload;           // Custom payload loaded from file
    uint16_t    f_payload_size;      // Size of the custom payload from file
    uint64_t    f_index_prev;        // Index of the last test frame sent/received
    uint64_t    f_speed;             // Current frames per second during a test
    uint64_t    f_speed_avg;         // Average frames per second during a test
    uint64_t    f_speed_max;         // Max frames per second achieved during the test
    double      b_speed;             // Current speed
    double      b_speed_max;         // Maximum speed achieved during the test
    long double b_speed_avg;         // Average speed achieved during the test

};


struct test_params             // Gerneral testing parameters
{

    uint64_t    f_bytes;             // Maximum amount of data to transmit in bytes
    uint64_t    f_count;             // Maximum number of frames to send
    uint64_t    f_duration;          // Maximum duration in seconds
    uint16_t    f_size;              // Frame payload in bytes
    uint16_t    f_size_total;        // Total frame size including headers
    uint64_t    s_elapsed;           // Seconds the test has been running
    uint64_t    f_tx_count;          // Total number of frames transmitted
    uint64_t    f_tx_count_prev;     // Total number of frames sent one second ago
    uint32_t    f_tx_count_max;      // Maximum transmit speed in frames per second
    uint64_t    f_rx_count;          // Total number of frames received
    uint64_t    f_rx_count_prev;     // Total number of frames received one second ago
    uint64_t    f_rx_other;          // Number of non test frames received
    uint64_t    f_rx_ontime;         // Frames received on time
    uint64_t    f_rx_early;          // Frames received out of order that are early
    uint64_t    f_rx_late;           // Frames received out of order that are late
    uint8_t     f_ack;               // Testing in ACK mode during transmition
    uint8_t     f_waiting_ack;       // Test is waiting for a frame to be ACK'ed
    struct timespec ts_current_time; // Two timers for timing a test and calculating stats
    struct timespec ts_elapsed_time;

};


// These need to be global so that signal_handler() can send a dying gasp
// and the allocated buffers can be free()'d
struct ifreq ethreq;
struct app_params *papp_params;
struct mtu_test *pmtu_test;
struct qm_test *pqm_test;
struct speed_test *pspeed_test;
struct test_interface *ptest_interface;
struct test_params *ptest_params;
struct frame_headers *pframe_headers;