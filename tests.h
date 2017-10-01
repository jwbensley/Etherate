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
 * File: Etherate Test Functions
 *
 * File Contents:
 * tests.c prototypes
 *
 */



// Calculate the one-way delay from Tx to Rx
void delay_test(struct app_params *app_params,
                struct frame_headers *frame_headers,
                struct test_interface *test_interface,
                struct test_params * test_params,
                struct qm_test *qm_test);

// Run an MTU sweep test from Tx to Rx
void mtu_sweep_test(struct app_params *app_params,
                    struct frame_headers *frame_headers,
                    struct test_interface *test_interface,
                    struct test_params *test_params,
                    struct mtu_test *mtu_test);

// Run some quality measurements from Tx to Rx
void latency_test(struct app_params *app_params,
                  struct frame_headers *frame_headers,
                  struct test_interface *test_interface,
                  struct test_params *test_params,
                  struct qm_test *qm_test);

// Run a speedtest (which is the default test operation if none is specified)
void speed_test_default(struct app_params *app_params,
                        struct frame_headers *frame_headers,
                        struct speed_test *speed_test,
                        struct test_interface *test_interface,
                        struct test_params *test_params);

// Tx a custom frame loaded from file
void send_custom_frame(struct app_params *app_params,
                       struct frame_headers *frame_headers,
                       struct speed_test *speed_test,
                       struct test_interface *test_interface,
                       struct test_params *test_params);