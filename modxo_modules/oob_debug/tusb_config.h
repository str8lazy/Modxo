/*
 * TinyUSB config for Modxo with dual CDC:
 *   CDC0 — SuperIO / COM1 bridge (unchanged)
 *   CDC1 — OOB control port
 *
 * Must appear before pico_stdio_usb's tusb_config.h on the include path.
 * Uses the same include guard so a later include of the SDK copy is a no-op.
 */
#ifndef _PICO_STDIO_USB_TUSB_CONFIG_H
#define _PICO_STDIO_USB_TUSB_CONFIG_H

#include "pico/stdio_usb.h"

#if !defined(LIB_TINYUSB_HOST) && !defined(LIB_TINYUSB_DEVICE)
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE)

#define CFG_TUD_CDC (2)

#ifndef CFG_TUD_CDC_RX_BUFSIZE
#define CFG_TUD_CDC_RX_BUFSIZE (TUD_OPT_HIGH_SPEED ? 512 : 64)
#endif
#ifndef CFG_TUD_CDC_TX_BUFSIZE
#define CFG_TUD_CDC_TX_BUFSIZE (TUD_OPT_HIGH_SPEED ? 512 : 64)
#endif
#ifndef CFG_TUD_CDC_EP_BUFSIZE
#define CFG_TUD_CDC_EP_BUFSIZE (TUD_OPT_HIGH_SPEED ? 512 : 64)
#endif

#if !PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR
#define CFG_TUD_VENDOR (0)
#else
#define CFG_TUD_VENDOR (1)
#define CFG_TUD_VENDOR_RX_BUFSIZE (256)
#define CFG_TUD_VENDOR_TX_BUFSIZE (256)
#endif
#endif

#endif /* _PICO_STDIO_USB_TUSB_CONFIG_H */
