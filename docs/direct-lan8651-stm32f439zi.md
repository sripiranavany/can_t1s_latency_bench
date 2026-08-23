# NUCLEO-F439ZI + Two-Wire ETH Click (LAN8651), wired directly

Bringing up the MikroE Two-Wire ETH Click (LAN8651, 10BASE-T1S MAC-PHY) on a
NUCLEO-F439ZI under Zephyr, with the click board jumpered straight to the
Nucleo with **no Click Shield in between**.

This is the configuration that works. Firmware lives in
[`host-tools/with-click-shield/`](../host-tools/with-click-shield/) and this is
its default build.

> For the Click Shield route, see [Why not the Click Shield](#why-not-the-click-shield)
> at the end. It is not currently working.

---

## Wiring

Eight jumpers between the click board's mikroBUS pins and the Nucleo. Two
pinouts are supported; both are verified working.

### Default — ST morpho headers (CN11/CN12)

| Click mikroBUS pin | Function | Nucleo morpho | STM32 |
| --- | --- | --- | --- |
| 4 | SCK | CN12 pin 31 | PB3 |
| 5 | MISO (SDO) | CN12 pin 27 | PB4 |
| 6 | MOSI (SDI) | CN12 pin 29 | PB5 |
| 3 | CS | CN12 pin 66 | PG8 |
| 15 | INT (IRQ) | CN12 pin 69 | PG4 |
| 2 | RST | CN11 pin 31 | PH1 |
| 7 | 3.3V | 3V3 | — |
| 8 | GND | GND | — |

This is deliberately the same pinout the Click Shield for Nucleo-144 routes to
its socket 3, so moving to the shield later needs no devicetree change.

Its one cost is RST on **PH1 = OSC_OUT**, which forces the HSE off and the
system clock onto the HSI — see
[Why PH1 forces the HSI](#why-ph1-forces-the-hsi).

### Alternative — Arduino headers (CN7/CN10)

`-DEXTRA_DTC_OVERLAY_FILE=arduino-header.overlay`

| Click mikroBUS pin | Function | Nucleo Arduino pin | STM32 |
| --- | --- | --- | --- |
| 4 | SCK | D13 | PA5 |
| 5 | MISO (SDO) | D12 | PA6 |
| 6 | MOSI (SDI) | D11 | PA7 |
| 3 | CS | D10 | PD14 |
| 2 | RST | D8 | PF12 |
| 15 | INT (IRQ) | D7 | PF13 |
| 7 | 3.3V | 3V3 | — |
| 8 | GND | GND | — |

**Prefer this one for latency work.** It keeps RST off PH1, so the HSE stays
enabled and the clock remains crystal-derived instead of running off the HSI's
±1% RC oscillator.

Keep the leads short — this runs SPI at 20 MHz on flying wires. If control
reads come back unreliable, drop `spi-max-frequency` to `<1000000>` in the
overlay before suspecting anything else.

Powering the click from the Nucleo's own 3V3 also means both chips power up
together, which matters — see [The IRQ_N power-ordering trap](#the-irq_n-power-ordering-trap).

Two 100 Ω termination jumpers (J1, J2) are populated by default on the click
board. Leave both fitted on the two boards at the physical ends of a 10BASE-T1S
segment, and remove them on any node in the middle.

---

## Why PH1 forces the HSI

PH1 is also OSC_OUT. The board runs the HSE in bypass off the ST-LINK's 8 MHz
MCO, and on STM32F4 the oscillator block keeps the OSC_OUT pad **even in bypass
mode** — driving PH1 high still reads back low, so the LAN8651 never sees its
reset pulse and you get `LAN865x reset timeout reached!`.

Moving the PLL onto the HSI releases PH0/PH1 to the GPIO block. HSI is 16 MHz
against the HSE's 8 MHz, so `div-m`/`mul-n` change to keep the same 2 MHz PLL
input, 336 MHz VCO, 168 MHz SYSCLK and 48 MHz PLL48.

The tradeoff is accuracy: HSI is a ±1% RC oscillator that drifts with
temperature. Moving the **RST lead alone** to PF12 (Arduino D8) avoids the whole
problem — that is what `arduino-header.overlay` does.

---

## Devicetree

[`boards/nucleo_f439zi.overlay`](../host-tools/with-click-shield/boards/nucleo_f439zi.overlay).
Three things in it are non-obvious:

**Disable the STM32's own MAC/MDIO/PHY.** Otherwise the on-chip Ethernet
controller claims iface 0 and `net_if_get_default()` hands you the wrong
interface.

```dts
&mac    { status = "disabled"; };
&mdio   { status = "disabled"; };
&eth_phy { status = "disabled"; };
```

**Set a MAC address.** The LAN8651 has no MAC in OTP. Without this,
`net_eth_mac_load()` yields `00:00:00:00:00:00`, `net_if` logs
`Invalid MAC address for iface 1` and the interface never comes up — the device
initialises fine and the carrier reports on, but `net_if_is_up()` stays 0. The
upstream `mikroe_two_wire_eth_click` shield overlay does not set one.

```dts
local-mac-address = [02 00 00 00 00 01];
```

Locally-administered unicast (bit 1 of the first octet set, bit 0 clear). The
last octet matches `plca-node-id`; give every node on the bus its own.

**PLCA settings** live on the `ethernet-phy@0` child node. `plca-node-id = <1>`
here, with `plca-node-count = <4>`. Node 0 is the PLCA coordinator — exactly one
node on the segment must be `plca-node-id = <0>`.

---

## Build and flash

```bash
source /sripiranavan/development/germany/sm3/research/.zephyr/bin/activate
cd host-tools/with-click-shield
west build -b nucleo_f439zi -p always .
west flash
```

`CONFIG_ETH_LAN865X` is selected automatically from the devicetree node.
[`prj.conf`](../host-tools/with-click-shield/prj.conf) enables networking, the
Ethernet L2, and `CONFIG_NET_SHELL` so you can poke at the interface with
`net iface` over the console.

---

## Expected output

```text
main() started, Timestamp: 11 ms
LAN865x device ready, Timestamp: 86 ms
MAC: 02:00:00:00:00:01
Entering status loop...
iface up=1 carrier_ok=1 oper_state=6,  Timestamp: 97 ms

[00:00:00.010,000] <inf> phy_mc_t1s: PHY (0) Link is up, speed 10 Mbps, half duplex
```

`oper_state=6` is `NET_IF_OPER_UP`. `up=1 carrier_ok=1` is the pair to look for.

---

## The IRQ_N power-ordering trap

Worth knowing, because it produces a confusing failure that looks like broken
wiring.

`lan865x_init()` in `drivers/ethernet/eth_lan865x.c` does this:

1. arms the INT pin with `GPIO_INT_EDGE_TO_ACTIVE` — a **falling** edge
2. pulses RESET_N low for 10 µs
3. blocks up to `LAN865X_RESET_TIMEOUT` (250 ms) waiting for the resulting edge

The LAN8651 asserts IRQ_N (active low) at power-on and holds it until the host
clears `STATUS0.RESETC`. So if the click board is **already powered** when the
STM32 boots, IRQ_N has been low for a long time before step 1 runs, the reset
in step 2 does not raise an already-asserted IRQ_N, no edge ever arrives, and
you get:

```text
<err> eth_lan865x: LAN865x reset timeout reached!
<err> net_if: Iface 0x20000c98 device not ready
```

Powering the click from the Nucleo's 3V3 avoids this — both come up together,
so the POR assertion lands after the interrupt is armed.

If you ever power the click board independently (a shield with its own USB-C,
for instance), the driver will trip over this. The workaround is an init hook
ordered between SPI and the Ethernet driver that clears `STATUS0` and reads a
status chunk to negate IRQ_N before `lan865x_init()` runs, so the driver's own
reset pulse produces a genuine deassert→assert edge. That needs
`CONFIG_ETH_LAN865X_INIT_PRIORITY` raised above `CONFIG_SPI_INIT_PRIORITY` (both
default to 50) to leave a slot for it. It is not in the current firmware because
the direct wiring does not need it.

This is arguably an upstream driver bug: arming an edge-triggered interrupt on a
line that may already be asserted is wrong on any board where the MAC-PHY is
independently powered.

---

## Why not the Click Shield

The MikroE Click Shield for Nucleo-144 route is **unproven** — it never worked
in testing, but the same pinout works fine on direct jumpers, so the fault is in
the shield path rather than the configuration.

The socket-3 pinout is already the default overlay's pinout, so seating the
click in socket 3 and building normally is enough.
[`click-shield.overlay`](../host-tools/with-click-shield/click-shield.overlay)
re-asserts those same pins and carries the shield-specific notes, for when you
want the configuration stated explicitly:

```bash
west build -b nucleo_f439zi -p always . -DEXTRA_DTC_OVERLAY_FILE=click-shield.overlay
```

What was established while trying:

**Socket 3 is the only usable socket.** Sockets 1 and 2 put RST on CN11 pins 13
and 15 — PA13/PA14, i.e. SWDIO/SWCLK, wired to the onboard ST-LINK.

**The socket-3 pin mapping is correct**, verified against the Click Shield for
Nucleo-144 v102 schematic cross-referenced with UM1974 Table 21 (which covers
NUCLEO-F439ZI): SCK/MISO/MOSI = PB3/PB4/PB5, CS3 = PG8, INT3 = PG4, RST3 = PH1.

**RST3 needs the HSE disabled.** PH1 is also OSC_OUT. The board runs the HSE in
bypass off the ST-LINK's 8 MHz MCO, and on STM32F4 the oscillator block keeps
the OSC_OUT pad even in bypass — driving PH1 high reads back low until the HSE
is turned off. Moving the PLL to the HSI frees it, at the cost of ±1% RC
accuracy instead of crystal-derived, which is a real downside for latency
measurements.

**SPI through the shield was never reliable.** Control reads succeeded
intermittently with `oa_tc6: Header transmission error!` throughout, then
stopped responding altogether — on two separate shields. Every SPI line passes
through the shield's TXS0108E auto-direction translators.

**Check the SEL3 switch before retrying.** The shield has per-socket logic-level
switches selecting VLS1–VLS4 between 3V3 and 5V; socket 3's translators (U8 for
SPI/CS/RST, U9 for INT) run off VLS3. The Two-Wire ETH Click is a 3.3 V board.
If SEL3 is on 5V, the translators drive 5 V into the LAN8651's 3.3 V I/O.

**Do not drive RST low for long periods.** MikroE's pinout table labels the
click's RST pin "Reset / ID SEL". A multi-millisecond assertion may latch the
chip into a different configuration at reset release.

The same click board works perfectly on direct jumpers using the **identical
pinout**, so both the click board and the devicetree are fine. Whatever is wrong
is in the shield's signal path.

**Expect the IRQ_N power-ordering trap to reappear.** With direct jumpers the
click is powered from the Nucleo's 3V3, so both come up together. A shield with
its own USB-C can power the click first, which is exactly the case the driver
mishandles.
