/*
 * License: MIT
 *
 * Copyright (c) 2012-2017 James Bensley.
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
 * File: Etherate Gerneral Functions
 *
 * File Contents:
 * functions.cpp
 *
 */



// Send gratuitus Etherate broadcasts
int16_t broadcast_etherate(struct FRAME_HEADERS *FRAME_HEADERS,
                           struct TEST_INTERFACE *TEST_INTERFACE);

// Build the Ethernet headers for sending frames
void build_headers(struct FRAME_HEADERS *FRAME_HEADERS);

// Build the Etherate TLV header
void build_tlv(struct FRAME_HEADERS *FRAME_HEADERS, uint16_t TLV_TYPE,
               uint32_t TLV_VALUE);

// Build a sub-TLV value after the TLV header
void build_sub_tlv(struct FRAME_HEADERS *FRAME_HEADERS, uint16_t SUB_TLV_TYPE,
                   uint64_t SUB_TLV_VALUE);

// Process the CLI arguments
int16_t cli_args(int argc, char *argv[], struct APP_PARAMS *APP_PARAMS,
                 struct FRAME_HEADERS *FRAME_HEADERS,
                 struct TEST_INTERFACE *TEST_INTERFACE,
                 struct TEST_PARAMS *TEST_PARAMS, struct MTU_TEST *MTU_TEST,
                 struct QM_TEST *QM_TEST);

// Explode a char string based on the passed field delimiter
uint8_t explode_char(char *string, char *delim, char *tokens[]);

// Get the MTU of the interface used for the test
int32_t get_interface_mtu_by_name(struct TEST_INTERFACE *TEST_INTERFACE);

// Try to automatically chose an interface to run the test on
int16_t get_sock_interface(struct TEST_INTERFACE *TEST_INTERFACE);

// HtoN method for ull values
uint64_t htonll(uint64_t val);

// List interfaces and hardware (MAC) address
void list_interfaces();

// NtoH method for ull values
uint64_t ntohll(uint64_t val);

// Print CLI args and usage
void print_usage(struct APP_PARAMS *APP_PARAMS,
                 struct FRAME_HEADERS *FRAME_HEADERS);

// Remove the interface from promiscuous mode
int16_t remove_interface_promisc(struct TEST_INTERFACE *TEST_INTERFACE);

// Free's allocated memory and closes sockets etc
void reset_app(struct FRAME_HEADERS *FRAME_HEADERS,
               struct TEST_INTERFACE *TEST_INTERFACE,
               struct TEST_PARAMS *TEST_PARAMS, struct QM_TEST *QM_TEST);

// Set the interface into promiscuous mode
int16_t set_interface_promisc(struct TEST_INTERFACE *TEST_INTERFACE);

// Try to open the passed socket on a user specified interface by index
int16_t set_sock_interface_index(struct TEST_INTERFACE *TEST_INTERFACE);

// Try to open the passed socket on a user specified interface by name
int16_t set_sock_interface_name(struct TEST_INTERFACE *TEST_INTERFACE);

// Signal handler to notify remote host of local application termiantion
void signal_handler(int signal);

// Send the settings from TX to RX
void sync_settings(struct APP_PARAMS *APP_PARAMS,
                   struct FRAME_HEADERS *FRAME_HEADERS,
                   struct TEST_INTERFACE *TEST_INTERFACE,
                   struct TEST_PARAMS * TEST_PARAMS,
                   struct MTU_TEST *MTU_TEST, struct QM_TEST *QM_TEST);

