# CAN over Ethernet Latency Bench

A testbed for measuring latency when **CAN traffic is tunnelled over automotive
Ethernet**, built on Zephyr RTOS and STM32.

The aim is to quantify what it costs, in latency and jitter, to carry CAN frames
across single-pair Ethernet using IEEE 1722 / [Open1722](https://github.com/COVESA/Open1722)
(AVTP CAN encapsulation), compared with a native CAN bus. The matrix of
configurations to compare:

| | Classic CAN | CAN FD |
| --- | --- | --- |
| **10BASE-T1S** (PLCA multidrop) | Current focus | Planned |
| **100BASE-T1** (point-to-point) | Planned | Planned |

```
 CAN node ──CAN──▶ STM32 (Zephyr) ──SPI──▶ LAN8651 ══10BASE-T1S (PLCA)══ LAN8670 ──USB──▶ Linux host
                   bxCAN + OA-TC6           MAC-PHY                         EVB-LAN8670-USB   timestamps / analysis
```

## Technology stack

| Layer | Choice |
| --- | --- |
| RTOS | Zephyr |
| MCU | STM32 NUCLEO-F439ZI (bxCAN, classic CAN 2.0 only). A CAN FD capable MCU (FDCAN) is needed for the FD tests |
| Ethernet | 10BASE-T1S with PLCA, Microchip LAN8651 (MAC-PHY over SPI, OA TC6). 100BASE-T1 planned |
| CAN transport | IEEE 1722 AVTP via Open1722 |
| CAN PHY | MCP2562FD transceiver, PESD2CAN protection |
| Host | Linux (Raspberry Pi / PC) with EVB-LAN8670-USB, `microchip_t1s` driver |

## Status

| Item | State |
| --- | --- |
| SPI link STM32 ↔ Click Shield verified | Done |
| LAN8651 bring-up on Zephyr (direct wiring and Click Shield socket 3) | Done |
| 10BASE-T1S frames received on STM32 and bridged out on CAN1 | Working prototype (fixed CAN ID `0x123`) |
| Linux host PLCA setup (driver build, hotplug automation) | Done |
| Custom CAN SPE carrier PCB | Schematic and layout in progress |
| Open1722 (AVTP CAN) integration | Not started |
| CAN FD tests | Planned, needs an FDCAN-capable board |
| 100BASE-T1 tests | Planned, needs a 100BASE-T1 PHY / board |
| Latency measurement and analysis | Not started |

## Repository layout

| Path | Contents |
| --- | --- |
| [`host-tools/with-click-shield/`](host-tools/with-click-shield/) | Zephyr firmware for the NUCLEO-F439ZI: LAN8651 bring-up, raw-frame RX, CAN bridge. Wiring variants are selected with devicetree overlays |
| [`host-tools/t1s-plca-setup/`](host-tools/t1s-plca-setup/README.md) | udev rules, systemd unit and scripts that put an EVB-LAN8670-USB into a PLCA-configured state on boot and hotplug |
| [`hardware/CAN SPE CARRIER/`](hardware/CAN%20SPE%20CARRIER/README.md) | KiCad project for the carrier board (CAN transceiver + 10BASE-T1S on a Nucleo-144 / click-shield footprint) |
| [`hardware/kicad-files/`](hardware/kicad-files/) | Third-party KiCad parts (MCP2562FD, PESD2CAN) |
| [`docs/`](docs/) | Bring-up notes, see below |

## Documentation

- [Direct LAN8651 ↔ STM32F439ZI wiring](docs/direct-lan8651-stm32f439zi.md): the known-good hardware setup, plus the Click Shield route
- [SPI verification on the Click Shield](docs/spi-verification-click-shield.md): loopback test and pin mapping
- [10BASE-T1S on Linux](docs/t1s-driver-build.md): building the PHY driver and manual PLCA testing
- [Useful links](docs/links): Click Shield and Nucleo-144 datasheets

## Getting started

Prerequisites: a Zephyr SDK and `west` workspace, a NUCLEO-F439ZI, a MikroE
Two-Wire ETH Click (LAN8651), and a Linux host with an EVB-LAN8670-USB.

1. **Host side.** Build the `microchip_t1s` driver and set up PLCA, following
   [`docs/t1s-driver-build.md`](docs/t1s-driver-build.md) and then
   [`host-tools/t1s-plca-setup`](host-tools/t1s-plca-setup/README.md).
2. **Wire the Click board** to the Nucleo as described in
   [`docs/direct-lan8651-stm32f439zi.md`](docs/direct-lan8651-stm32f439zi.md).
3. **Build and flash the firmware:**

   ```bash
   cd host-tools/with-click-shield
   west build -b nucleo_f439zi -p always .
   west flash
   ```

   To use the Click Shield and/or the CAN bridge, stack overlays:

   ```bash
   west build -b nucleo_f439zi -p always . \
       -DEXTRA_DTC_OVERLAY_FILE="click-shield.overlay;can.overlay"
   ```

   The CAN overlay uses PD0 (RX) / PD1 (TX) and needs an external CAN
   transceiver. See the header of [`can.overlay`](host-tools/with-click-shield/can.overlay).

## Notes

- bxCAN on the STM32F4 is classic CAN 2.0 only (8-byte payload, 1 Mbit/s max). There is no CAN FD on this part, so the CAN FD runs will need a different MCU with FDCAN.
- Click Shield socket 3 needs its logic-level switch set to 3V3.
- `_backup/` holds an older EVB-LAN8670 setup guide kept for reference.
