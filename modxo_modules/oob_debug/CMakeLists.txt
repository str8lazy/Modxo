add_library(oob_debug STATIC
    ${CMAKE_CURRENT_LIST_DIR}/smbus_bb.c
    ${CMAKE_CURRENT_LIST_DIR}/heartbeat.c
    ${CMAKE_CURRENT_LIST_DIR}/oob_protocol.c
    ${CMAKE_CURRENT_LIST_DIR}/oob_control.c
    ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
)

# Our tusb_config.h (CFG_TUD_CDC=2) must win over pico_stdio_usb's copy.
target_include_directories(oob_debug PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_definitions(oob_debug PUBLIC
    PICO_STDIO_USB_USE_DEFAULT_DESCRIPTORS=0
    PICO_STDIO_USB_ENABLE_RESET_VIA_BAUD_RATE=0
    MODXO_OOB_DEBUG=1
)

pico_enable_stdio_usb(oob_debug 1)
pico_enable_stdio_uart(oob_debug 0)

# Prefer unmarked TinyUSB (same as pico_stdio_usb) to avoid LIB_TINYUSB_DEVICE
# quirks; still need the device stack for descriptors/CDC APIs.
target_link_libraries(oob_debug PUBLIC
    modxo
    pico_stdlib
    pico_unique_id
    pico_stdio_usb
    hardware_gpio
    hardware_timer
)

# Force dual-CDC on every translation unit that pulls TinyUSB via this target.
target_compile_definitions(oob_debug PUBLIC CFG_TUD_CDC=2)
