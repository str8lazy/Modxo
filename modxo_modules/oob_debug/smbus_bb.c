/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Bit-banged open-drain SMBus/I2C multi-master for the Xbox SMBus.

Why bit-bang instead of the RP2040 hardware I2C block:
- The Xbox MCPX is already the primary SMBus master; we are a second master.
- HW I2C does not give clean, portable arbitration-loss detection for this case.
- We must never enable on-chip pull-ups (Xbox already has 3.3V pull-ups) and
  must idle high-Z. Bit-bang makes that explicit.
- PIO SMs are already claimed (pio0 all 4 for LPC; pio1 for serirq/ws2812).
- Transactions are rare (CLI commands); bit timing cost is fine on core1.

Pins: XBOX_SMBUS_SDA / XBOX_SMBUS_SCL from pinout (GP12/GP13 on RP2040-Zero/Tiny
Modxo V3 expansion header — same nets as LPC header pins 14/13).
*/
#include <oob_debug.h>

#include <hardware/gpio.h>
#include <hardware/timer.h>
#include <pico/stdlib.h>
#include <modxo_pinout.h>

#ifndef XBOX_SMBUS_SDA
#define XBOX_SMBUS_SDA EXPANSION_PORT_I2C_SDA
#endif
#ifndef XBOX_SMBUS_SCL
#define XBOX_SMBUS_SCL EXPANSION_PORT_I2C_SCL
#endif

static bool smbus_ready = false;

/* Open-drain: drive low by output-0; release by switching to input (high-Z). */
static inline void sda_low(void)
{
    gpio_set_dir(XBOX_SMBUS_SDA, GPIO_OUT);
    gpio_put(XBOX_SMBUS_SDA, 0);
}

static inline void sda_release(void)
{
    gpio_set_dir(XBOX_SMBUS_SDA, GPIO_IN);
}

static inline void scl_low(void)
{
    gpio_set_dir(XBOX_SMBUS_SCL, GPIO_OUT);
    gpio_put(XBOX_SMBUS_SCL, 0);
}

static inline void scl_release(void)
{
    gpio_set_dir(XBOX_SMBUS_SCL, GPIO_IN);
}

static inline bool sda_read(void)
{
    return gpio_get(XBOX_SMBUS_SDA);
}

static inline bool scl_read(void)
{
    return gpio_get(XBOX_SMBUS_SCL);
}

static void delay_half(void)
{
    busy_wait_us_32(SMBUS_BIT_US);
}

/* Wait for SCL high (clock stretching / bus settle). Never block forever. */
static bool wait_scl_high(uint32_t timeout_us)
{
    absolute_time_t deadline = make_timeout_time_us(timeout_us);
    while (!scl_read()) {
        if (absolute_time_diff_us(get_absolute_time(), deadline) <= 0) {
            return false;
        }
    }
    return true;
}

static bool bus_is_idle(void)
{
    return sda_read() && scl_read();
}

static bool wait_bus_idle(uint32_t timeout_us)
{
    absolute_time_t deadline = make_timeout_time_us(timeout_us);
    while (!bus_is_idle()) {
        if (absolute_time_diff_us(get_absolute_time(), deadline) <= 0) {
            return false;
        }
        busy_wait_us_32(50);
    }
    return true;
}

/* Release both lines — safe idle / recovery. */
static void bus_release(void)
{
    sda_release();
    scl_release();
}

static smbus_result_t start_cond(void)
{
    if (!wait_bus_idle(SMBUS_TIMEOUT_US / 2)) {
        return SMBUS_ERR_BUS_BUSY;
    }
    /* START: SDA falls while SCL high */
    sda_low();
    delay_half();
    scl_low();
    delay_half();
    return SMBUS_OK;
}

static smbus_result_t repeated_start(void)
{
    sda_release();
    delay_half();
    scl_release();
    if (!wait_scl_high(SMBUS_TIMEOUT_US / 4)) {
        bus_release();
        return SMBUS_ERR_TIMEOUT;
    }
    delay_half();
    if (!sda_read()) {
        /* Another master won arbitration during our released SDA */
        bus_release();
        return SMBUS_ERR_ARB_LOST;
    }
    sda_low();
    delay_half();
    scl_low();
    delay_half();
    return SMBUS_OK;
}

static void stop_cond(void)
{
    sda_low();
    delay_half();
    scl_release();
    wait_scl_high(SMBUS_TIMEOUT_US / 4);
    delay_half();
    sda_release();
    delay_half();
}

/* Write one bit; detect arbitration loss when releasing SDA high. */
static smbus_result_t write_bit(bool bit)
{
    if (bit) {
        sda_release();
    } else {
        sda_low();
    }
    delay_half();
    scl_release();
    if (!wait_scl_high(SMBUS_TIMEOUT_US / 4)) {
        bus_release();
        return SMBUS_ERR_TIMEOUT;
    }
    /* Arbitration: we wanted high but bus is low */
    if (bit && !sda_read()) {
        bus_release();
        return SMBUS_ERR_ARB_LOST;
    }
    delay_half();
    scl_low();
    delay_half();
    return SMBUS_OK;
}

static smbus_result_t read_bit(bool *bit_out)
{
    sda_release();
    delay_half();
    scl_release();
    if (!wait_scl_high(SMBUS_TIMEOUT_US / 4)) {
        bus_release();
        return SMBUS_ERR_TIMEOUT;
    }
    delay_half();
    *bit_out = sda_read();
    scl_low();
    delay_half();
    return SMBUS_OK;
}

static smbus_result_t write_byte(uint8_t byte, bool *acked)
{
    for (int i = 7; i >= 0; i--) {
        smbus_result_t r = write_bit((byte >> i) & 1);
        if (r != SMBUS_OK) {
            return r;
        }
    }
    bool ack_bit = true;
    smbus_result_t r = read_bit(&ack_bit);
    if (r != SMBUS_OK) {
        return r;
    }
    *acked = !ack_bit; /* ACK is SDA low */
    return SMBUS_OK;
}

static smbus_result_t read_byte(uint8_t *byte_out, bool send_ack)
{
    uint8_t v = 0;
    for (int i = 7; i >= 0; i--) {
        bool bit = false;
        smbus_result_t r = read_bit(&bit);
        if (r != SMBUS_OK) {
            return r;
        }
        if (bit) {
            v |= (uint8_t)(1u << i);
        }
    }
    smbus_result_t r = write_bit(!send_ack); /* ACK=0, NACK=1 */
    if (r != SMBUS_OK) {
        return r;
    }
    *byte_out = v;
    return SMBUS_OK;
}

void smbus_bb_init(void)
{
    if (smbus_ready) {
        return;
    }
    /* Input + no pulls: idle high-Z; Xbox bus supplies pull-ups to 3.3V. */
    gpio_init(XBOX_SMBUS_SDA);
    gpio_init(XBOX_SMBUS_SCL);
    gpio_set_dir(XBOX_SMBUS_SDA, GPIO_IN);
    gpio_set_dir(XBOX_SMBUS_SCL, GPIO_IN);
    gpio_disable_pulls(XBOX_SMBUS_SDA);
    gpio_disable_pulls(XBOX_SMBUS_SCL);
    /* Ensure output value is 0 so switching to OUT drives low (open-drain). */
    gpio_put(XBOX_SMBUS_SDA, 0);
    gpio_put(XBOX_SMBUS_SCL, 0);
    smbus_ready = true;
}

bool smbus_bb_owns_expansion_pins(void)
{
#if (XBOX_SMBUS_SDA == EXPANSION_PORT_I2C_SDA) && (XBOX_SMBUS_SCL == EXPANSION_PORT_I2C_SCL)
    return true;
#else
    return false;
#endif
}

static smbus_result_t xfer_write_byte(uint8_t addr7, uint8_t reg, uint8_t value)
{
    smbus_result_t r;
    bool ack = false;

    r = start_cond();
    if (r != SMBUS_OK) {
        return r;
    }

    r = write_byte((uint8_t)(addr7 << 1), &ack);
    if (r != SMBUS_OK) {
        return r;
    }
    if (!ack) {
        stop_cond();
        return SMBUS_ERR_NACK;
    }

    r = write_byte(reg, &ack);
    if (r != SMBUS_OK) {
        return r;
    }
    if (!ack) {
        stop_cond();
        return SMBUS_ERR_NACK;
    }

    r = write_byte(value, &ack);
    if (r != SMBUS_OK) {
        return r;
    }
    if (!ack) {
        stop_cond();
        return SMBUS_ERR_NACK;
    }

    stop_cond();
    return SMBUS_OK;
}

static smbus_result_t xfer_read_byte(uint8_t addr7, uint8_t reg, uint8_t *value)
{
    smbus_result_t r;
    bool ack = false;

    r = start_cond();
    if (r != SMBUS_OK) {
        return r;
    }

    r = write_byte((uint8_t)(addr7 << 1), &ack);
    if (r != SMBUS_OK) {
        return r;
    }
    if (!ack) {
        stop_cond();
        return SMBUS_ERR_NACK;
    }

    r = write_byte(reg, &ack);
    if (r != SMBUS_OK) {
        return r;
    }
    if (!ack) {
        stop_cond();
        return SMBUS_ERR_NACK;
    }

    r = repeated_start();
    if (r != SMBUS_OK) {
        return r;
    }

    r = write_byte((uint8_t)((addr7 << 1) | 1u), &ack);
    if (r != SMBUS_OK) {
        return r;
    }
    if (!ack) {
        stop_cond();
        return SMBUS_ERR_NACK;
    }

    r = read_byte(value, false); /* NACK then STOP after single byte */
    if (r != SMBUS_OK) {
        return r;
    }

    stop_cond();
    return SMBUS_OK;
}

static smbus_result_t xfer_probe(uint8_t addr7)
{
    smbus_result_t r;
    bool ack = false;

    r = start_cond();
    if (r != SMBUS_OK) {
        return r;
    }
    r = write_byte((uint8_t)(addr7 << 1), &ack);
    if (r != SMBUS_OK) {
        return r;
    }
    stop_cond();
    return ack ? SMBUS_OK : SMBUS_ERR_NACK;
}

static smbus_result_t with_retries(smbus_result_t (*fn)(uint8_t, uint8_t, uint8_t *),
                                   uint8_t a, uint8_t b, uint8_t *c)
{
    smbus_result_t last = SMBUS_ERR_TIMEOUT;
    for (int i = 0; i < SMBUS_MAX_RETRIES; i++) {
        if (i > 0) {
            bus_release();
            busy_wait_us_32(SMBUS_RETRY_DELAY_US * (uint32_t)i);
        }
        last = fn(a, b, c);
        if (last == SMBUS_OK || last == SMBUS_ERR_NACK) {
            return last;
        }
        /* Retry on arb lost / busy / timeout */
    }
    bus_release();
    return last;
}

/* Wrappers so with_retries can share a signature for read; write uses its own. */
static smbus_result_t read_adapter(uint8_t addr7, uint8_t reg, uint8_t *value)
{
    return xfer_read_byte(addr7, reg, value);
}

smbus_result_t smbus_write_byte(uint8_t addr7, uint8_t reg, uint8_t value)
{
    smbus_bb_init();
    smbus_result_t last = SMBUS_ERR_TIMEOUT;
    for (int i = 0; i < SMBUS_MAX_RETRIES; i++) {
        if (i > 0) {
            bus_release();
            busy_wait_us_32(SMBUS_RETRY_DELAY_US * (uint32_t)i);
        }
        last = xfer_write_byte(addr7, reg, value);
        if (last == SMBUS_OK || last == SMBUS_ERR_NACK) {
            return last;
        }
    }
    bus_release();
    return last;
}

smbus_result_t smbus_read_byte(uint8_t addr7, uint8_t reg, uint8_t *value)
{
    if (!value) {
        return SMBUS_ERR_INVALID;
    }
    smbus_bb_init();
    return with_retries(read_adapter, addr7, reg, value);
}

smbus_result_t smbus_probe(uint8_t addr7)
{
    smbus_bb_init();
    smbus_result_t last = SMBUS_ERR_TIMEOUT;
    for (int i = 0; i < SMBUS_MAX_RETRIES; i++) {
        if (i > 0) {
            bus_release();
            busy_wait_us_32(SMBUS_RETRY_DELAY_US * (uint32_t)i);
        }
        last = xfer_probe(addr7);
        if (last == SMBUS_OK || last == SMBUS_ERR_NACK) {
            return last;
        }
    }
    bus_release();
    return last;
}
