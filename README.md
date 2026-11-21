![Modchip + Axolotl = Modxo](images/logo.png) <img src="images/Shalx-TR.png" height="176">

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://github.com/Team-Resurgent/Repackinator/blob/main/LICENSE.md)
[![.NET](https://github.com/Team-Resurgent/Modxo/actions/workflows/BundleModxo.yml/badge.svg)](https://github.com/Team-Resurgent/Modxo/actions/workflows/BundleModxo.yml)
[![Discord](https://img.shields.io/badge/chat-on%20discord-7289da.svg?logo=discord)](https://discord.gg/VcdSfajQGK)
<!--Translated by: Emmanuelito18-->

# Translated versions

> [!NOTE]
> Some translations could be outdated and may take a while to be updated than english version.

- [English](https://github.com/Team-Resurgent/Modxo/blob/main/README.md)
- [Español](https://github.com/Team-Resurgent/Modxo/blob/main/README%20ES-MX.md)
- [Portuguese](https://github.com/Team-Resurgent/Modxo/blob/main/README%20PT-BR.md)
- [Nederlands](https://github.com/Team-Resurgent/Modxo/blob/main/README%20NL.md)
- More translations coming soon

# Modxo

Modxo (pronnounced "Modsho") is an RP2040 firmware that converts a Raspberry Pi Pico (or similar RP2040-based device) into an Original Xbox-compatible LPC peripheral device.

Modxo can be used for loading an Xbox BIOS image from the LPC port, as well as for interfacing compatible Xbox software with peripheral devices such as HD44780 displays or addressable RGB LEDs.

Modxo is *not* a modchip. Whereas legacy modchips rely on largely obsolete hardware like LPC flash storage chips or expensive programmable logic ICs, Modxo is the first fully software-defined implementation of an LPC peripheral device. It is open source software, mostly written in C, developed using the official Raspberry Pi Pico SDK and designed to run on RP2040-based hardware.

No specialized hardware or complicated tooling is needed to load Modxo on a compatible device -- in most cased just a USB cable is all that is necessary. And installation works much like legacy devices -- all that is needed for installation is a compatible RP2040-based device, a few resistors, wire and basic soldering equipment. Custom PCBs exist to simplify the installation process even further.

## PrometheOS

If you want to upgrade the Modxo part of PrometheOS, see Upgrading PrometheOS's Modxo steps below.
However if you just want to install PrometheOS you can disregard the instructions below as PrometheOS already contains a working version of Modxo so all you need to do is:

1) Use Pandora
2) Download the relevant .uf2 for your pi variant e.g. RP2040 Zero & Tiny Prometheos VX.X.X.uf2 

## How to Install

### Requirements

- An Xbox (any revision) with a working LPC Port. 1.6 Xboxes will need an LPC rebuild.
- A RP2040 development board. There may be some clone boards that are not compatible. The following boards are known to work with Modxo:
- - Official Raspberry Pi Pico
  - Official Raspberry Pi Pico 2
  - YD-RP2040
  - RP2040 Zero/Tiny
  - XIAO RP2040
- 4 100 Ohm resistors (tested with 1/4 W resistors)
- 1 1N4148 Diode (technically optional but highly recommended with certain development boards, see board-specific installation instructions below for more information)

### Flashing firmware

#### Packing Bios

1. Go to [https://team-resurgent.github.io/modxo/](https://team-resurgent.github.io/Modxo/)
2. Drag and Drop your bios file
3. UF2 File with bios image will be downloaded

#### Flashing steps (Single Bios)

> [!IMPORTANT]
> The following steps are not required if you intend to use PrometheOS, and just flash PrometheOS-{board-variant}.uf2 directly...

1. Connect Raspberry Pi Pico to a PC with BOOTSEL button (or equivalent button on compatible RP2040 hardware) pressed and a new drive will become available.
2. Copy Modxo-{board variant}.uf2 into the Raspberry Pi Pico Drive.
3. Reconnect Raspberry Pi Pico with BOOTSEL button pressed, so the previous drive will showup again.
4. Copy your bios UF2 file into the drive

#### Upgrading PrometheOS's Modxo steps

> [!IMPORTANT]
> The following steps are only needed if wanting to upgrade PrometheOS with Modxo updates/fixes...

1. Assuming PrometheOS-{board-variant}.uf2 is already flashed.
2. Connect Raspberry Pi Pico to a PC with BOOTSEL button (or equivalent button on compatible RP2040 hardware) pressed and a new drive will become available.
3. Copy Modxo-{board variant}.uf2 into the Raspberry Pi Pico Drive.

### Wiring diagrams

---

#### LPC Header

   ![LPC Header wiring diagram](images/lpc_header_wiring.png)

  > [!NOTE]
  > - D0 is required for versions 1.0 - 1.5 unless it is grounded.
  > - LFrame connection is required by version 1.6.
  > - LPC 3.3V is required for all versions
  > - LPC Rebuild is required for version 1.6
  >
---

#### Official Raspberry Pi Pico

   ![LPC Header wiring diagram](images/official_pinout_pico1.png)

   > [!NOTE]
   > Please add the diode if connecting the Pico to USB. This avoid powering the LPC 5V Pin from the USB cable which could have unintended consequences.
   > LPC 3.3V is required for all versions
---

#### Official Raspberry Pi Pico 2

   ![LPC Header wiring diagram](images/official_pinout_pico2.png)

   > [!NOTE]
   > Please add the diode if connecting the Pico to USB. This avoid powering the LPC 5V Pin from the USB cable which could have unintended consequences.
   > LPC 3.3V is required for all versions
---

#### YD-RP2040

   ![LPC Header wiring diagram](images/YD_RP2040_pinout.png)

   > [!NOTE]
   > Dont forget to add solder to jumper R68 if using the onboard RGB Led.
   > LPC 3.3V is required for all versions
---

#### RP2040-Zero/Tiny

   ![LPC Header wiring diagram](images/RP2040_Zero_pinout.png)

   > [!NOTE]
   > Please add the diode if connecting the Pico to USB. This avoid powering the LPC 5V Pin from the USB cable which could have unintended consequences.
   > LPC 3.3V is required for all versions
---

#### XIAO-RP2040

   ![LPC Header wiring diagram](images/XIAO-RP2040_pnout.png)

---

## Firmware Build Instructions

Clone the repository with submodules:

```bash
git clone --recurse-submodules https://github.com/Team-Resurgent/Modxo.git
# or, if already cloned:
git submodule update --init --recursive
```

### Visual Studio Code (Windows/Linux/Mac)

1.- Download and Install [Visual Studio Code](https://code.visualstudio.com/download)

2.- Install extension: "Raspberry Pi Pico"

![Pico Extension](images/extension.png)

3.- Ensure SDK 2.0.0 is selected as below...

![SDK 2.0.0](images/sdk.png)

4.- Go to Raspberry Pi Pico Tab and click "Configure CMake"

5.- Go to Run and Debug Tab and select Build for your board

6.- Click "Start Debugging" (Green arrow)

7.- UF2 File will be generated on Build folder

### Docker

   > [!WARNING]
   > The Docker method is currently not working. Consider using VScode.

#### Setup

Build your base docker image with

```
docker build -t modxo-builder .
```

#### Firmware Build

```
docker compose run --rm builder
```

Output will be `out/modxo_[pinout].uf2`

There are also some extra parameters that can be passed to the build script:

- MODXO_PINOUT=`official_pico` | `yd_rp2040` | `rp2040_zero` | `xiao_rp2040` - Default is `official_pico`.

- CLEAN=`y`: triggers a clean build. Default is disabled.

- BUILD_TYPE=`Release` | `Debug` - Default is `Debug`.

*Examples:*

```
MODXO_PINOUT=rp2040_zero BUILD_TYPE=Release docker compose run --rm builder
```

```
CLEAN MODXO_PINOUT=yd_rp2040 docker compose run --rm builder
```

#### Packing Bios locally

Place your bios file named `bios.bin` in this directory or place any bios files (regardless of their name) in the bios directory

```
docker compose run --rm bios2uf2
```

---

## Out-of-band debug / control (SMBus + dual CDC)

Modxo exposes a second USB CDC interface as an always-available control port.
Even if the Xbox CPU is hung, a Linux host can still probe SMBus, watch LPC
heartbeat age, and ask the PIC/SMC to reset, power-cycle, or power-off.

This does **not** drive `LRESET#` and does **not** use the Pico 1200-baud
BOOTSEL trick for Xbox power control. Power commands go to the Xbox PIC over
SMBus (same register values PrometheOS/Cerbios use via `HalWriteSMBusValue`).

### Wiring (Modxo V3 / RP2040-Tiny style)

Owner-wired (already present on the target hardware this feature targets):

| Xbox LPC header | Signal        | RP2040 GPIO | Notes                                      |
|-----------------|---------------|-------------|--------------------------------------------|
| Pin 14          | SMBus SDA     | GP12        | i2c0 SDA / Modxo expansion port            |
| Pin 13          | SMBus SCL     | GP13        | i2c0 SCL / Modxo expansion port            |
| GND             | Ground        | header GND  | Shared ground                              |

- Do **not** enable pull-ups on the Pico; the Xbox SMBus already has pull-ups to 3.3V.
- Firmware uses a **bit-banged open-drain multi-master** (not the HW I2C block): checks bus idle before START, detects arbitration loss / bus errors, retries with backoff, and always releases the lines. Hardware I2C on RP2040 is a poor fit for polite second-master use on a live MCPX bus; PIO SMs are already consumed by LPC/serirq/WS2812.
- On boards where GP12/GP13 are the expansion I2C port, the expansion HW I2C driver refuses to claim those pins while OOB owns them (LCD on GP14/15 i2c1 is unaffected).

Default pin mapping follows each board's `EXPANSION_PORT_I2C_*` pins (overridable via `XBOX_SMBUS_SDA` / `XBOX_SMBUS_SCL` in the board cmake).

### USB interfaces

| Interface | Role                         | udev symlink   |
|-----------|------------------------------|----------------|
| CDC0 (IF 0) | SuperIO / COM1 bridge (unchanged) | `/dev/modxo`     |
| CDC1 (IF 2) | OOB control port             | `/dev/modxo-ctl` |

Sample udev rules: [`udev/99-modxo.rules`](udev/99-modxo.rules) (`GROUP=uucp`, `MODE=0660`, `ENV{ID_MM_DEVICE_IGNORE}=1`).

### Control protocol (line-based text on CDC1)

```
help
status
heartbeat
hb-stale <ms>              # 0 disables unsolicited HB-STALE
smbus scan
smbus read <addr7> <reg>
smbus write <addr7> <reg> <val>
reset CONFIRM              # PIC 0x10 reg 0x02 = 0x01
cycle CONFIRM              # PIC 0x10 reg 0x02 = 0x40
off CONFIRM                # PIC 0x10 reg 0x02 = 0x80
```

Destructive commands require an explicit `CONFIRM` or `--confirm` token so a
stray line cannot reboot the box.

### LPC heartbeat

Guest (or a BIOS/app) may write a counter to I/O port **`0xDEA0`**
(`LPC_HEARTBEAT_PORT`). `status` / `heartbeat` report the last value, ms since
that write, and ms since any LPC cycle was seen (hang detection without guest
cooperation). If `hb-stale` is non-zero, the control port emits an unsolicited
`HB-STALE` line when the heartbeat is older than that timeout.

### Safety caveats

- Shared bus with the MCPX: keep transactions short; the firmware never holds SCL/SDA indefinitely.
- SMBus power commands only work while the Xbox standby / SMBus rail is powered. You **cannot power ON a fully-off box** via SMBus from Modxo.
- Do not confuse Pico BOOTSEL / picotool reset with Xbox reset.
- No Microsoft/retail Xbox BIOS binaries are bundled with this firmware.

### Host protocol unit test

```bash
cmake -S host -B build-host && cmake --build build-host && ./build-host/oob_protocol_test
```

### Hardware test checklist (not run in CI)

1. Flash `modxo_rp2040_zero_tiny.uf2` (or your board variant); confirm two CDC ports appear.
2. Install udev rules; check `/dev/modxo` and `/dev/modxo-ctl`.
3. `picocom /dev/modxo-ctl` → `help` → expect command list.
4. `smbus scan` → expect **0x10** (PIC). Often also **0x45** (video encoder) and/or **0x4C** / **0x54** (temp / other). Exact set varies by revision.
5. `smbus read 0x10 0x01` → PIC version (do this **before** any reset/cycle/off).
6. `status` → PIC version / temps if reachable; heartbeat + LPC age fields present.
7. With Xbox running, confirm `lpc_age_ms` stays small; freeze the box (or pull LPC activity) and watch age grow / optional `HB-STALE`.
8. Only after scan+version look sane: `reset CONFIRM` (or `cycle` / `off`) and verify Xbox reacts. `reset` without `CONFIRM` must be rejected.
9. Confirm COM1 (`/dev/modxo`) SuperIO bridge still works independently of the control port.

**Untested on real Xbox hardware in this PR** (no LPC/SMBus attached in CI). Bit-bang timing, arbitration under MCPX traffic, and temperature register layouts should be validated on the owner's Modxo V3 + RP2040-Tiny wiring.

## Known bugs

- Windbg get stuck sometimes when connected to Modxo SuperIO's serial port

## Notes

> [!IMPORTANT]
> Currently, Modxo uses the ID 0xAF. Any derivative hardware with significant changes should ideally use a different ID. This is so that software like PrometheOS can target features appropriately.

## Attribution Requirement

     a) **Attribution:**  
       If you distribute or modify this work, you must provide proper 
       attribution to the original authors. This includes:
       - Mentioning the original project name: `Modxo`.
       - Listing the original authors: `Shalx / Team Resurgent`.
       - Including a link to the original project repository: 
           `https://github.com/Team-Resurgent/Modxo`.
       - Clearly stating any modifications made.

    b) **Logo and Branding:**  
       Any derivative work or distribution must include the logos provided by
       the original authors in accordance with the branding guidelines. The 
       logos must be displayed prominently in any interface or documentation 
       where the original project is referenced or attributed.

    c) **Branding Guidelines:**  
       You can find the logos and detailed branding guidelines in the 
       `BRANDING.md` file provided with this project. The logos must not be 
       altered in any way that would distort or misrepresent the original 
       branding.
