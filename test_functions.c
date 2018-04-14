/*
 * License: MIT
 *
 * Copyright (c) 2012-2018 James Bensley.
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
 * File: Speed Test Functions
 *
 */



#include "test_functions.h"



void latency_test_results(struct etherate *eth)
{
    if (eth->app.tx_mode) {

        printf("Test frames transmitted: %" PRIu64 "\n"
               "Test frames received: %" PRIu64 "\n"
               "Non test or out-of-order frames received: %" PRIu64 "\n"
               "Number of timeouts: %" PRIu32 "\n"
               "Min/Max rtt during test: %.9Lfs/%.9Lfs\n"
               "Min/Max jitter during test: %.9Lfs/%.9Lfs\n",
               eth->params.f_tx_count,
               eth->params.f_rx_count,
               eth->params.f_rx_other,
               eth->qm_test.timeout_count,
               eth->qm_test.rtt_min,
               eth->qm_test.rtt_max,
               eth->qm_test.jitter_min,
               eth->qm_test.jitter_max);

    } else {

        printf("Test frames transmitted: %" PRIu64 "\n"
               "Test frames received: %" PRIu64 "\n"
               "Non test frames received: %" PRIu64 "\n"
               "Number of timeouts: %" PRIu32 "\n"
               "Min/Max interval during test %.9Lfs/%.9Lfs\n",
               eth->params.f_tx_count,
               eth->params.f_rx_count,
               eth->params.f_rx_other,
               eth->qm_test.timeout_count,
               eth->qm_test.interval_min,
               eth->qm_test.interval_max);

    }

}



void mtu_sweep_test_results(struct etherate *eth)
{

    if (eth->app.tx_mode) {

        printf("Largest MTU ACK'ed from Rx is %" PRIu16 "\n\n"
               "Test frames transmitted: %" PRIu64 "\n"
               "Test frames received: %" PRIu64 "\n"
               "Non test frames received: %" PRIu64 "\n"
               "In order MTU ACK frames received: %" PRIu64 "\n"
               "Out of order MTU ACK frames received early: %" PRIu64 "\n"
               "Out of order MTU ACK frames received late: %" PRIu64 "\n",
               eth->mtu_test.largest,
               eth->params.f_tx_count,
               eth->params.f_rx_count,
               eth->params.f_rx_other,
               eth->params.f_rx_ontime,
               eth->params.f_rx_early,
               eth->params.f_rx_late);

    } else {

        printf("Largest MTU received was %" PRIu16 "\n\n"
               "Test frames transmitted: %" PRIu64 "\n"
               "Test frames received: %" PRIu64 "\n"
               "Non test frames received: %" PRIu64 "\n"
               "In order MTU frames received: %" PRIu64 "\n"
               "Out of order MTU frames received early: %" PRIu64 "\n"
               "Out of order MTU frames received late: %" PRIu64 "\n",
               eth->mtu_test.largest,
               eth->params.f_tx_count,
               eth->params.f_rx_count,
               eth->params.f_rx_other,
               eth->params.f_rx_ontime,
               eth->params.f_rx_early,
               eth->params.f_rx_late);

    }

}



void speed_test_results(struct etherate *eth)
{

    if (eth->app.tx_mode) {

        printf("Test frames transmitted: %" PRIu64 "\n"
               "Test frames received: %" PRIu64 "\n"
               "Non test frames received: %" PRIu64 "\n"
               "In order ACK frames received: %" PRIu64 "\n"
               "Out of order ACK frames received early: %" PRIu64 "\n"
               "Out of order ACK frames received late: %" PRIu64 "\n"
               "Maximum speed during test: %.2fMbps, %" PRIu64 "Fps\n"
               "Average speed during test: %.2LfMbps, %" PRIu64 "Fps\n"
               "Data transmitted during test: %" PRIu64 "MBs\n",
               eth->params.f_tx_count,
               eth->params.f_rx_count,
               eth->params.f_rx_other,
               eth->params.f_rx_ontime,
               eth->params.f_rx_early,
               eth->params.f_rx_late,
               eth->speed_test.b_speed_max,
               eth->speed_test.f_speed_max,
               eth->speed_test.b_speed_avg,
               eth->speed_test.f_speed_avg,
               (eth->speed_test.b_tx / 1024) / 1024);

    } else {

        printf("Test frames transmitted: %" PRIu64 "\n"
               "Test frames received: %" PRIu64 "\n"
               "Non test frames received: %" PRIu64 "\n"
               "In order test frames received: %" PRIu64 "\n"
               "Out of order test frames received early: %" PRIu64 "\n"
               "Out of order test frames received late: %" PRIu64 "\n"
               "Maximum speed during test: %.2fMbps, %" PRIu64 "Fps\n"
               "Average speed during test: %.2LfMbps, %" PRIu64 "Fps\n"
               "Data received during test: %" PRIu64 "MBs\n",
               eth->params.f_tx_count,
               eth->params.f_rx_count,
               eth->params.f_rx_other,
               eth->params.f_rx_ontime,
               eth->params.f_rx_early,
               eth->params.f_rx_late,
               eth->speed_test.b_speed_max,
               eth->speed_test.f_speed_max,
               eth->speed_test.b_speed_avg,
               eth->speed_test.f_speed_avg,
               (eth->speed_test.b_rx / 1024) / 1024);

    }

}
