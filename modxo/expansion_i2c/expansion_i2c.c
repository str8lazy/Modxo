#include <expansion_i2c.h>

#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <modxo.h>
#include <modxo_pinout.h>
#include <pico/time.h>
#include <stdbool.h>

static bool initialized = false;

/* Weak default: expansion I2C is available. oob_debug overrides when it owns pins. */
bool __attribute__((weak)) smbus_bb_owns_expansion_pins(void)
{
    return false;
}

void expansion_i2c_init()
{
    if (initialized) {
        return;
    }
    /*
     * On RP2040-Zero/Tiny Modxo V3, expansion I2C is GP12/GP13 — the same
     * pins used for Xbox SMBus OOB. Refuse HW I2C init so we do not enable
     * pico pull-ups or steal the open-drain bit-bang master.
     */
    if (smbus_bb_owns_expansion_pins()) {
        return;
    }
    initialized = true;
    i2c_init(EXPANSION_PORT_I2C_INST, 100 * 1000);
    gpio_set_function(EXPANSION_PORT_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(EXPANSION_PORT_I2C_SCL, GPIO_FUNC_I2C);
}

int expansion_i2c_read_timeout_us(uint8_t addr, uint8_t *dst, size_t len, bool nostop, uint timeout_us) {
    expansion_i2c_init();
    if (!initialized) {
        return PICO_ERROR_GENERIC;
    }
    return i2c_read_timeout_us(EXPANSION_PORT_I2C_INST, addr, dst, len, nostop, timeout_us);
}

int expansion_i2c_write_timeout_us(uint8_t addr, uint8_t *src, size_t len, bool nostop, uint timeout_us)
{
    expansion_i2c_init();
    if (!initialized) {
        return PICO_ERROR_GENERIC;
    }
    return i2c_write_timeout_us(EXPANSION_PORT_I2C_INST, addr, src, len, nostop, timeout_us);
}
