/*
 * Dual-CDC USB descriptors for Modxo OOB debug.
 * CDC0: SuperIO / COM1 bridge
 * CDC1: OOB control port
 *
 * Replaces pico_stdio_usb default descriptors
 * (PICO_STDIO_USB_USE_DEFAULT_DESCRIPTORS=0).
 *
 * Deliberately does NOT use the 1200-baud BOOTSEL reset path for Xbox power
 * control — that is done via SMBus to the PIC. Baud-rate BOOTSEL is disabled
 * at compile time (PICO_STDIO_USB_ENABLE_RESET_VIA_BAUD_RATE=0).
 */
#include "pico/stdio_usb.h"
#include "pico/stdio_usb/reset_interface.h"
#include "pico/unique_id.h"
#include "tusb.h"

#ifndef USBD_VID
#define USBD_VID (0x2E8A)
#endif

#ifndef USBD_PID
#if PICO_RP2040
#define USBD_PID (0x000A)
#else
#define USBD_PID (0x0009)
#endif
#endif

#ifndef USBD_MANUFACTURER
#define USBD_MANUFACTURER "Team Resurgent"
#endif

#ifndef USBD_PRODUCT
#define USBD_PRODUCT "Modxo"
#endif

#define TUD_RPI_RESET_DESC_LEN 9

#if !PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE
#define USBD_DESC_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_CDC_DESC_LEN)
#define USBD_ITF_MAX 4
#else
#define USBD_DESC_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_CDC_DESC_LEN + TUD_RPI_RESET_DESC_LEN)
#define USBD_ITF_RPI_RESET 4
#define USBD_ITF_MAX 5
#endif

#if !PICO_STDIO_USB_DEVICE_SELF_POWERED
#define USBD_CONFIGURATION_DESCRIPTOR_ATTRIBUTE (0)
#define USBD_MAX_POWER_MA (250)
#else
#define USBD_CONFIGURATION_DESCRIPTOR_ATTRIBUTE TUSB_DESC_CONFIG_ATT_SELF_POWERED
#define USBD_MAX_POWER_MA (1)
#endif

/* Interface numbers: each CDC uses two interfaces (comm + data) */
#define USBD_ITF_CDC0 0
#define USBD_ITF_CDC1 2

#define USBD_CDC0_EP_CMD 0x81
#define USBD_CDC0_EP_OUT 0x02
#define USBD_CDC0_EP_IN 0x82
#define USBD_CDC1_EP_CMD 0x83
#define USBD_CDC1_EP_OUT 0x04
#define USBD_CDC1_EP_IN 0x84

#define USBD_CDC_CMD_MAX_SIZE 8
#define USBD_CDC_IN_OUT_MAX_SIZE 64

#define USBD_STR_0 0x00
#define USBD_STR_MANUF 0x01
#define USBD_STR_PRODUCT 0x02
#define USBD_STR_SERIAL 0x03
#define USBD_STR_CDC0 0x04
#define USBD_STR_CDC1 0x05
#define USBD_STR_RPI_RESET 0x06

static const tusb_desc_device_t usbd_desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
#if PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE && PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR
    .bcdUSB = 0x0210,
#else
    .bcdUSB = 0x0200,
#endif
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USBD_VID,
    .idProduct = USBD_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = USBD_STR_MANUF,
    .iProduct = USBD_STR_PRODUCT,
    .iSerialNumber = USBD_STR_SERIAL,
    .bNumConfigurations = 1,
};

#define TUD_RPI_RESET_DESCRIPTOR(_itfnum, _stridx) \
    9, TUSB_DESC_INTERFACE, _itfnum, 0, 0, TUSB_CLASS_VENDOR_SPECIFIC, RESET_INTERFACE_SUBCLASS, RESET_INTERFACE_PROTOCOL, _stridx,

static const uint8_t usbd_desc_cfg[USBD_DESC_LEN] = {
    TUD_CONFIG_DESCRIPTOR(1, USBD_ITF_MAX, USBD_STR_0, USBD_DESC_LEN,
                          USBD_CONFIGURATION_DESCRIPTOR_ATTRIBUTE, USBD_MAX_POWER_MA),

    TUD_CDC_DESCRIPTOR(USBD_ITF_CDC0, USBD_STR_CDC0, USBD_CDC0_EP_CMD,
                       USBD_CDC_CMD_MAX_SIZE, USBD_CDC0_EP_OUT, USBD_CDC0_EP_IN,
                       USBD_CDC_IN_OUT_MAX_SIZE),

    TUD_CDC_DESCRIPTOR(USBD_ITF_CDC1, USBD_STR_CDC1, USBD_CDC1_EP_CMD,
                       USBD_CDC_CMD_MAX_SIZE, USBD_CDC1_EP_OUT, USBD_CDC1_EP_IN,
                       USBD_CDC_IN_OUT_MAX_SIZE),

#if PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE
    TUD_RPI_RESET_DESCRIPTOR(USBD_ITF_RPI_RESET, USBD_STR_RPI_RESET)
#endif
};

static char usbd_serial_str[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];

static const char *const usbd_desc_str[] = {
    [USBD_STR_MANUF] = USBD_MANUFACTURER,
    [USBD_STR_PRODUCT] = USBD_PRODUCT,
    [USBD_STR_SERIAL] = usbd_serial_str,
    [USBD_STR_CDC0] = "Modxo COM1",
    [USBD_STR_CDC1] = "Modxo Control",
#if PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE
    [USBD_STR_RPI_RESET] = "Reset",
#endif
};

const uint8_t *tud_descriptor_device_cb(void)
{
    return (const uint8_t *)&usbd_desc_device;
}

const uint8_t *tud_descriptor_configuration_cb(__unused uint8_t index)
{
    return usbd_desc_cfg;
}

const uint16_t *tud_descriptor_string_cb(uint8_t index, __unused uint16_t langid)
{
#ifndef USBD_DESC_STR_MAX
#define USBD_DESC_STR_MAX (20)
#endif
    static uint16_t desc_str[USBD_DESC_STR_MAX];

    if (!usbd_serial_str[0]) {
        pico_get_unique_board_id_string(usbd_serial_str, sizeof(usbd_serial_str));
    }

    uint8_t len;
    if (index == 0) {
        desc_str[1] = 0x0409;
        len = 1;
    } else {
        if (index >= sizeof(usbd_desc_str) / sizeof(usbd_desc_str[0])) {
            return NULL;
        }
        const char *str = usbd_desc_str[index];
        if (!str) {
            return NULL;
        }
        for (len = 0; len < USBD_DESC_STR_MAX - 1 && str[len]; ++len) {
            desc_str[1 + len] = str[len];
        }
    }

    desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * len + 2));
    return desc_str;
}
