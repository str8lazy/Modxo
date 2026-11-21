/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Out-of-band debug/control channel for the original Xbox via SMBus + USB CDC.
Gives the host PC freeze detection and remote reset/cycle/off even when the
Xbox CPU is hung. LPC servicing stays on core0; SMBus/control runs on core1.
*/
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifndef OOB_HOST_TEST
#include <modxo.h>
#endif

// Xbox PIC / SMC SMBus 7-bit address (write address 0x20)
#define XBOX_PIC_ADDR 0x10

// PIC SMC registers (same as PrometheOS/Cerbios HalWriteSMBusValue targets)
#define PIC_REG_VERSION 0x01
#define PIC_REG_POWER_CTRL 0x02
#define PIC_REG_TEMP_CPU 0x09
#define PIC_REG_TEMP_AMB 0x0A

// PIC power control command bytes
#define PIC_CMD_RESET 0x01
#define PIC_CMD_CYCLE 0x40
#define PIC_CMD_OFF 0x80

// ADM1032 / focus video encoder common Xbox SMBus addresses (for docs/scan)
#define XBOX_ADM1032_ADDR 0x4C
#define XBOX_VIDEO_ENCODER_ADDR 0x45

/*
 * LPC I/O heartbeat port.
 * 0xDEA8/0xDEA9 are taken by legacy_display LCD; 0xDEA4/0xDEA5 by NVM;
 * 0xDEAD/0xDEAF by chip/variant ID. 0xDEA0 is free.
 */
#define LPC_HEARTBEAT_PORT 0xDEA0

// Confirm token required for destructive SMBus power commands
#define OOB_CONFIRM_TOKEN "CONFIRM"

// Default ms without heartbeat write before unsolicited HB-STALE (0 = disabled)
#define OOB_HB_STALE_DEFAULT_MS 5000

// SMBus timing (shared bus with MCPX — stay short, never hold forever)
#define SMBUS_TIMEOUT_US 50000
#define SMBUS_RETRY_DELAY_US 5000
#define SMBUS_MAX_RETRIES 3
#define SMBUS_BIT_US 5

typedef enum {
    SMBUS_OK = 0,
    SMBUS_ERR_TIMEOUT,
    SMBUS_ERR_NACK,
    SMBUS_ERR_ARB_LOST,
    SMBUS_ERR_BUS_BUSY,
    SMBUS_ERR_INVALID
} smbus_result_t;

typedef struct {
    uint8_t last_value;
    uint32_t last_write_us;
    uint32_t last_lpc_activity_us;
    uint32_t stale_timeout_ms;
    bool have_write;
} heartbeat_state_t;

typedef struct {
    char reply[256];
    bool ok;
    bool needs_smbus;
} oob_cmd_result_t;

/* Returns true if line was recognized (reply filled). execute=false for dry parse. */
bool oob_parse_and_dispatch(const char *line, oob_cmd_result_t *out, bool execute);

#ifndef OOB_HOST_TEST
extern MODXO_TASK oob_debug_hdlr;

/* Called from core1 main loop; must stay light and never block LPC on core0. */
void oob_debug_poll(void);

void smbus_bb_init(void);
bool smbus_bb_owns_expansion_pins(void);
smbus_result_t smbus_write_byte(uint8_t addr7, uint8_t reg, uint8_t value);
smbus_result_t smbus_read_byte(uint8_t addr7, uint8_t reg, uint8_t *value);
smbus_result_t smbus_probe(uint8_t addr7);

const heartbeat_state_t *heartbeat_get_state(void);
void heartbeat_set_stale_timeout_ms(uint32_t ms);
void heartbeat_init(void);
void oob_control_init(void);
#endif
