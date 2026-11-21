/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

USB CDC1 control port: line-based text protocol.
CDC0 remains the SuperIO / COM1 bridge (unchanged).
*/
#include <oob_debug.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "tusb.h"
#include <pico/stdlib.h>
#include <pico/time.h>
#include <hardware/sync.h>

/* Control port is the second CDC interface */
#define OOB_CDC_ITF 1

#define LINE_MAX 160
static char line_buf[LINE_MAX];
static size_t line_len = 0;
static bool stale_announced = false;
static repeating_timer_t poll_timer;
static volatile bool poll_kick = false;

static void cdc_write_str(const char *s)
{
    if (!tud_cdc_n_connected(OOB_CDC_ITF)) {
        return;
    }
    size_t n = strlen(s);
    while (n) {
        uint32_t avail = tud_cdc_n_write_available(OOB_CDC_ITF);
        if (!avail) {
            tud_task();
            avail = tud_cdc_n_write_available(OOB_CDC_ITF);
            if (!avail) {
                break;
            }
        }
        uint32_t chunk = (uint32_t)n < avail ? (uint32_t)n : avail;
        tud_cdc_n_write(OOB_CDC_ITF, s, chunk);
        s += chunk;
        n -= chunk;
    }
    tud_cdc_n_write_flush(OOB_CDC_ITF);
}

static void handle_line(char *line)
{
    /* Trim trailing CR/LF/space */
    size_t n = strlen(line);
    while (n > 0 && (line[n - 1] == '\r' || line[n - 1] == '\n' || isspace((unsigned char)line[n - 1]))) {
        line[--n] = '\0';
    }
    if (n == 0) {
        return;
    }

    oob_cmd_result_t result;
    if (!oob_parse_and_dispatch(line, &result, true)) {
        return;
    }
    cdc_write_str(result.reply);
}

static void feed_rx(void)
{
    if (!tud_cdc_n_connected(OOB_CDC_ITF)) {
        line_len = 0;
        return;
    }

    while (tud_cdc_n_available(OOB_CDC_ITF)) {
        char c;
        if (tud_cdc_n_read(OOB_CDC_ITF, &c, 1) != 1) {
            break;
        }
        if (c == '\n' || c == '\r') {
            line_buf[line_len] = '\0';
            handle_line(line_buf);
            line_len = 0;
            continue;
        }
        if (line_len + 1 < LINE_MAX) {
            line_buf[line_len++] = c;
        } else {
            /* Overflow — drop line */
            line_len = 0;
            cdc_write_str("ERR line too long\r\n");
        }
    }
}

static void check_hb_stale(void)
{
    const heartbeat_state_t *hb = heartbeat_get_state();
    if (!hb->stale_timeout_ms || !hb->have_write) {
        stale_announced = false;
        return;
    }
    if (!tud_cdc_n_connected(OOB_CDC_ITF)) {
        return;
    }

    uint32_t age_ms = (time_us_32() - hb->last_write_us) / 1000u;
    if (age_ms >= hb->stale_timeout_ms) {
        if (!stale_announced) {
            char msg[80];
            snprintf(msg, sizeof(msg), "HB-STALE age_ms=%u last=0x%02X\r\n",
                     age_ms, hb->last_value);
            cdc_write_str(msg);
            stale_announced = true;
        }
    } else {
        stale_announced = false;
    }
}

static bool poll_timer_cb(repeating_timer_t *rt)
{
    (void)rt;
    poll_kick = true;
    __sev();
    return true;
}

void oob_control_init(void)
{
    line_len = 0;
    stale_announced = false;
    /* Wake core1 every 10ms so CDC/SMBus work even when LPC is quiet. */
    add_repeating_timer_ms(10, poll_timer_cb, NULL, &poll_timer);
}

void oob_debug_poll(void)
{
    if (!poll_kick && !tud_cdc_n_available(OOB_CDC_ITF)) {
        /* Still check stale occasionally via timer kick */
        return;
    }
    poll_kick = false;
    feed_rx();
    check_hb_stale();
}

static void oob_init(void)
{
    smbus_bb_init();
    heartbeat_init();
    oob_control_init();
}

static void oob_core1_init(void)
{
    /* Timer already started in init; nothing else required on core1. */
}

MODXO_TASK oob_debug_hdlr = {
    .init = oob_init,
    .core1_init = oob_core1_init,
};
