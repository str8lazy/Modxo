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

target_link_libraries(oob_debug PUBLIC
    modxo
    pico_stdlib
    pico_unique_id
    hardware_gpio
    hardware_timer
)
