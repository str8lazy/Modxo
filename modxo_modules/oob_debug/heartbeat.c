/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

LPC I/O heartbeat at 0xDEA0 plus last-LPC-activity timestamp.
Guest writes a counter; host reads age via control CDC status/heartbeat.
*/
#include <oob_debug.h>

#include <hardware/sync.h>
#include <modxo/lpc_interface.h>
#include <pico/stdlib.h>
#include <stdatomic.h>

static heartbeat_state_t hb_state = {
    .last_value = 0,
    .last_write_us = 0,
    .last_lpc_activity_us = 0,
    .stale_timeout_ms = OOB_HB_STALE_DEFAULT_MS,
    .have_write = false,
};

/* Updated from LPC IRQ path — keep this cheap (single 32-bit store). */
void lpc_note_activity(void)
{
    hb_state.last_lpc_activity_us = time_us_32();
}

static void heartbeat_write_handler(uint16_t address, uint8_t *data)
{
    (void)address;
    uint32_t irq = save_and_disable_interrupts();
    hb_state.last_value = *data;
    hb_state.last_write_us = time_us_32();
    hb_state.have_write = true;
    restore_interrupts(irq);
}

static void heartbeat_read_handler(uint16_t address, uint8_t *data)
{
    (void)address;
    *data = hb_state.last_value;
}

void heartbeat_init(void)
{
    hb_state.stale_timeout_ms = OOB_HB_STALE_DEFAULT_MS;
    lpc_interface_add_io_handler(LPC_HEARTBEAT_PORT, 0xFFFF,
                                 heartbeat_read_handler, heartbeat_write_handler);
}

const heartbeat_state_t *heartbeat_get_state(void)
{
    return &hb_state;
}

void heartbeat_set_stale_timeout_ms(uint32_t ms)
{
    hb_state.stale_timeout_ms = ms;
}
