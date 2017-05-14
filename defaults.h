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
 * File: Etherate Setup Functions
 *
 * File Contents:
 * defaults.c prototypes
 *
 */



// Restore all test and interface values to default at the start of each RX loop
void set_default_values(struct app_params *app_params,
                        struct frame_headers *frame_headers,
                        struct mtu_test *mtu_test,
                        struct test_interface *test_interface,
                        struct test_params * test_params,
                        struct qm_test *qm_test);

// Set up Etherate test frame
int16_t setup_frame(struct app_params * app_params,
                    struct frame_headers *frame_headers,
                    struct test_interface *test_interface,
                    struct test_params *test_params);

// Set up an OS socket
int16_t setup_socket(struct test_interface *test_interface);

// Set up the physical interface and bind to socket
int16_t setup_socket_interface(struct frame_headers *frame_headers,
                               struct test_interface *test_interface,
                               struct test_params *test_params);