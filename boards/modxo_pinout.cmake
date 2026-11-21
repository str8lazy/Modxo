#
#SPDX short identifier: BSD-2-Clause
#BSD 2-Clause License
#Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
#

#Default pin values, so the pinout files can skip them if not used
set(LED_STATUS_PIN PICO_DEFAULT_LED_PIN)

set(LED_STRIP1_PWR 31) #Not Used
set(LED_STRIP1 31) #Not Used
set(LED_STRIP2 31) #Not Used
set(BOARD_FLASH_SIZE 16777216)

# Xbox SMBus OOB defaults (overridden per board after include if needed).
# Placeholder until board file sets EXPANSION_PORT pins; finalized below.

set(SD_CARD_USE_SDIO false) #Not Used
set(SD_CARD_SDIO_CMD 31) #Not Used
set(SD_CARD_SDIO_D0 31) #Not Used

#pinout selector
set(BOARD_PINOUT_FILE "${CMAKE_CURRENT_LIST_DIR}/${MODXO_PINOUT}.cmake")
if( EXISTS ${BOARD_PINOUT_FILE})
    include(${BOARD_PINOUT_FILE})
else()
    message(FATAL_ERROR "Pinout file not found: ${BOARD_PINOUT_FILE}")
endif()

message(STATUS "Building for ${MODXO_PINOUT}.")

# Default Xbox SMBus pins to the expansion I2C port (GP12/13 on Zero/Tiny).
if(NOT DEFINED XBOX_SMBUS_SDA)
    set(XBOX_SMBUS_SDA ${EXPANSION_PORT_I2C_SDA})
endif()
if(NOT DEFINED XBOX_SMBUS_SCL)
    set(XBOX_SMBUS_SCL ${EXPANSION_PORT_I2C_SCL})
endif()

configure_file(${CMAKE_CURRENT_LIST_DIR}/modxo_pinout.h.in modxo_pinout.h @ONLY)
add_compile_definitions(PICO_FLASH_SIZE_BYTES=${BOARD_FLASH_SIZE})

#Output filename
set(PROJECT_NAME "modxo_${MODXO_PINOUT}")
