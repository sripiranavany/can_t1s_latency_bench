# Verifying SPI on the STM32F439ZI + Click Shield for Nucleo-144

How we confirmed the SPI1 bus works correctly between the Nucleo-F439ZI and
the MikroE Click Shield for Nucleo-144, using Zephyr and a hardware loopback
test. Firmware lives in
[`host-tools/with-click-shield/`](../host-tools/with-click-shield/).

---

## Background

The click shield exposes 3 mikroBUS™ sockets. SCK, CIPO (MISO) and COPI
(MOSI) are a **shared bus** wired identically to all three sockets — only
the CS line is unique per socket. Socket signals route through the shield's
`TXS0108E` level shifters to the Nucleo's Morpho connectors (CN11/CN12),
which need shield-side USB-C power to function.

We cross-checked the shield's pinout diagram
([`click-shield.png`](click-shield.png)) against ST's official Morpho
connector table ([`stm32439-morphopins-assignment.png`](stm32439-morphopins-assignment.png),
UM1974 Table 21) to map mikroBUS signals to actual STM32F439ZI pins:

| Signal | STM32F439 pin | Notes |
|---|---|---|
| SCK | PB3 | shared across all 3 sockets |
| CIPO (MISO) | PB4 | shared across all 3 sockets |
| COPI (MOSI) | PB5 | shared across all 3 sockets |
| CS1 (socket 1) | PA1 | |
| CS2 (socket 2) | PB12 | |
| CS3 (socket 3) | PG8 | |

Confidence in this mapping came from two things: PB3/PB4/PB5 were
independently identified by direct pin inspection, and other signals we
derived from the diagram (e.g. TX1→PA9, RX1→PA10) landed exactly on
STM32's real default USART1 pins — a strong sanity check that the
diagram-to-table row mapping was read correctly.

---

## Test method: hardware SPI loopback

Rather than trying to verify SPI indirectly (e.g. via a specific Click
board), we used a direct hardware loopback: physically jumper **COPI
(MOSI) to CIPO (MISO)** on the socket header. Firmware then transmits a
known byte pattern and checks whether it reads back exactly what it sent.
This isolates the SPI peripheral, pin routing, and shield level-shifters
from any dependency on a specific Click board.

### Wiring

On socket 1's mikroBUS header, jumper the CIPO pin directly to the COPI pin
with a single wire. No resistor or external component needed — this is a
pure digital loopback.

### Devicetree overlay

[`host-tools/with-click-shield/boards/nucleo_f439zi.overlay`](../host-tools/with-click-shield/boards/nucleo_f439zi.overlay):

```dts
#include <zephyr/dt-bindings/gpio/gpio.h>

&spi1 {
	status = "okay";
	pinctrl-0 = <&spi1_sck_pb3 &spi1_miso_pb4 &spi1_mosi_pb5>;
	pinctrl-names = "default";

	cs-gpios = <&gpioa 1 GPIO_ACTIVE_LOW>;
};
```

### Firmware

[`host-tools/with-click-shield/src/main.c`](../host-tools/with-click-shield/src/main.c)
sends 4 incrementing bytes over `spi_transceive()` once a second and prints
TX vs RX:

```c
int ret = spi_transceive(spi_dev, &spi_cfg, &tx_set, &rx_set);
bool match = memcmp(tx_buf, rx_buf, sizeof(tx_buf)) == 0;

printf("TX: %02x %02x %02x %02x  RX: %02x %02x %02x %02x  %s\n",
       tx_buf[0], tx_buf[1], tx_buf[2], tx_buf[3],
       rx_buf[0], rx_buf[1], rx_buf[2], rx_buf[3],
       match ? "LOOPBACK OK" : "MISMATCH");
```

### Build & flash

```bash
cd host-tools/with-click-shield
west build -b nucleo_f439zi -p always .
west flash
```

---

## What went wrong first, and how we found the fix

The first attempt used **PA5/PA6/PA7** (the standard Arduino-header SPI1
pins on any Nucleo-144 board) with the loopback jumper on the click
shield's socket header. Result: `RX` was always `00 00 00 00` regardless
of what was sent — no error from `spi_transceive()`, just silence on MISO.

To isolate the cause, we moved the same jumper to bypass the shield
entirely — straight across the Nucleo's own Arduino header pins D11/D12
(PA7/PA6) — and reflashed the same firmware unchanged. That loopback
**worked**, which proved the SPI1 peripheral and pin configuration were
fine; the problem was specific to the shield.

That result pointed at pin routing rather than shield power, since the
shield's USB-C was already connected. Direct inspection then showed the
click shield's SPI bus is actually wired to **PB3/PB4/PB5**, not
PA5/PA6/PA7 — the shield does not reuse the Arduino header's default SPI
pins. Updating the overlay's `pinctrl-0` and `cs-gpios` accordingly (see
above) fixed it.

---

## Result

```
*** Booting Zephyr OS build v4.4.0-11884-g5058917ea61b ***
TX: 01 02 03 04  RX: 01 02 03 04  LOOPBACK OK
TX: 02 03 04 05  RX: 02 03 04 05  LOOPBACK OK
TX: 03 04 05 06  RX: 03 04 05 06  LOOPBACK OK
TX: 04 05 06 07  RX: 04 05 06 07  LOOPBACK OK
```

TX and RX matching on every transfer confirms: SPI1 is correctly
configured on PB3/PB4/PB5, CS1 (PA1) is wired correctly, and the shield's
level shifters are powered and passing signal through socket 1 cleanly.

To test socket 2 or socket 3, only `cs-gpios` needs to change (PB12 or
PG8 respectively) — SCK/CIPO/COPI are the same shared bus, so no other
change is required.

> **Correction (post-publish):** CS3 was initially derived from a
> hard-to-read screenshot as PG5. Re-verified against the literal text of
> ST's official UM1974 PDF (Table 17), the correct value is **PG8**. The
> table text also gave the exact value for socket 1's INT1 pin (PF13),
> which the screenshot-based reading couldn't reliably resolve.
