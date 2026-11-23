# CAN SPE CARRIER – PCB Routing Guide (for KiCad beginners)

This file explains **how the board was routed**, **what you are looking at when you open it in KiCad**,
and **what to check before sending it to a manufacturer**.

> **Fixed parts:** J3, J4, J5 and J6 are the click-shield / mainboard headers. Their positions were
> **not allowed to change**, so everything else was placed and routed around them.

---

## 1. Files in this folder

| File | What it is |
|---|---|
| `CAN SPE CARRIER.kicad_pro` | Project file (design rules, net classes) |
| `CAN SPE CARRIER.kicad_sch` | Schematic |
| `CAN SPE CARRIER.kicad_pcb` | The routed board – **open this one** |
| `CAN SPE CARRIER-backups/` | Automatic zip backups. `…-before-routing.kicad_pcb` is your board **before** the re-route |
| `_autosave-*.kicad_pcb` | KiCad autosave, safe to ignore/delete |
| `~*.lck` | Lock files from an earlier session. KiCad may warn "file is open" – if KiCad is not running, click *Ignore/Open anyway* or delete the `.lck` files |

---

## 2. Quick glossary

- **Net** – a set of pads that must be electrically connected (e.g. `GND`, `+3V3`, `/SPI_SCK`).
- **Track / trace** – a copper line that carries a net.
- **Via** – a plated hole that moves a track from one copper layer to the other.
- **Pad** – the copper spot on a part where you solder.
- **Zone / pour** – a big filled copper area (here: ground).
- **Net class** – a group of nets that share rules (track width, clearance). Here: `Default` and `Power`.
- **Ratsnest** – thin straight lines in KiCad showing connections that are *not yet routed*. A finished board has none.
- **DRC** – Design Rule Check; KiCad's automatic error checker.

---

## 3. Why the colours look different in KiCad

KiCad draws each copper layer in its own colour. If you opened the board and everything "changed colour", this is normal:

| Colour (default theme) | Layer | Meaning |
|---|---|---|
| **Red** | `F.Cu` | Front copper (top side) |
| **Green/Blue** | `B.Cu` | Back copper (bottom side) |
| Light red/green haze | GND zones | The ground pour on each layer |
| Yellow lines | `Edge.Cuts` | Board outline |

Tips:
- **Active layer is bright, the other layers are dimmed.** Press `PgUp` / `PgDn`, or click a layer in the
  right-hand *Appearance* panel, and the colours swap emphasis. That is why the board looks different each time.
- *Appearance* panel → **Layers** → *Colour* swatch lets you change any colour.
- **Highlight a net:** press `` ` `` (backtick) or use *Route → Highlight Net* (shortcut `` ` ``)
  and click a track. Everything on that net lights up, and everything else dims. This is the **best way to
  understand the routing**.
- *Appearance* → **Objects** → untick *Zones* (or set *Zone fill* opacity low) to hide the ground pour so you
  can see the tracks clearly.
- *Appearance* → **Nets** lets you show/hide or recolour individual nets.
- Press `B` to refill zones after any change; `Ctrl+B` clears the fill.
- **Show the whole board:** `Home`. **Zoom:** scroll wheel. **Measure:** `Ctrl+Shift+M`.

---

## 4. Board overview

```
                    TOP EDGE  (y ≈ 46…50 mm)
   J5 (left, x≈22)  ─────────────────────────  J6 (right, x≈85)
   click-shield side                              mainboard side
        │   J3 (x≈59, y≈73)    J4 (x≈59, y≈50)    │
        │   TP1 D1 R3 R2 C4  ← 3V3 loads           │
        │   C3  TP2          ← 5V cap              │
        │                                          │
        │   C2 C1  U2 (MCP2562 CAN transceiver)    │
        │                       D2  R1  J2   J1    │
        └──────────── BOTTOM (y ≈ 140 mm) ─────────┘
```

Roughly 69 × 93 mm board, 2 copper layers.

Signal flow in one sentence: **mainboard (J6→J4) and click shield (J5→J3) feed the CAN transceiver U2, whose CANH/CANL
outputs go through the protection diode (D1 / D2) and termination (R1 + J4, R2 + J6) to the screw terminals J1 and J2.**

### 4.1 mikroBUS socket (J3 / J5)

J3 and J5 are the standard **mikroBUS socket**: two 1×8 female headers, 22.86 mm (900 mil) apart, with pin 1 of both
rows at the same end. A click board plugs in directly. No cable and no flipping is needed.

| Row | Pin 1 → pin 8 |
|---|---|
| **J3** (left row) | AN *(not connected)*, RST, CS, SCK, MISO, MOSI, 3V3, GND |
| **J5** (right row) | PWM *(not connected)*, INT, RX *(not connected)*, TX *(not connected)*, SCL, SDA, 5V, GND |

The numbering of J5 runs in the same direction as J3. mikroBUS itself numbers the right-hand pins 16 down to 9, but
that is only a naming convention. Physically PWM sits opposite AN, as on a real click board.

**Connected pins:** RST, CS, SCK, MISO, MOSI, 3V3, GND (J3) and INT, SCL, SDA, 5V, GND (J5). These cover the
**Two-Wire ETH Click (LAN8651)**, which talks to the host over SPI and uses INT and RST. It does not use a UART.

**Not connected:** AN (J3 pin 1) and PWM, RX, TX (J5 pins 1, 3, 4). Their silkscreen labels follow the standard
mikroBUS pin names, but nothing is routed to them. A click board that needs an analog input, a PWM output or a UART
will **not** work in this socket.

---

## 5. Routing, net by net

Track widths: signals **0.25 mm**, power **0.5 mm**, short stubs near pads 0.3 mm.
Clearance everywhere: **0.2 mm**. Vias: **0.6 mm pad / 0.3 mm drill**.

### 5.1 SPI bus (J5 → J3)
| Net | Layer | How |
|---|---|---|
| `/SPI_SCK` | F.Cu (top) | Part of a parallel bundle |
| `/SPI_MISO` | F.Cu (top) | Part of a parallel bundle |
| `/SPI_CS` | F.Cu (top) | Part of a parallel bundle |
| `/SPI_MOSI` | **B.Cu (bottom)** | Runs underneath the bundle |

- The three top-layer signals run side by side using **45° bends** (no sharp 90° corners), keeping the same spacing.
- MOSI was put on the **other layer** so it never has to cross the others → **no vias needed** for SPI.

### 5.2 Mainboard signals (J6 → J4): `/MB_INT`, `/MB_SCL`, `/MB_SDA`
- All on **F.Cu**, running along the **top edge** (above the headers).
- Each one sits at a **different height ("staggered levels")** so the lines never cross each other.
- In KiCad: highlight `/MB_INT` and you will see a long horizontal line at y ≈ 47.2 mm joining J6 to J4.

### 5.3 `/MB_RST`
- On **B.Cu** (bottom). It goes **around the right end of J3** and then up into pin **J3.2**.
- Using the back layer avoids all the top-layer signals.

### 5.4 +3V3 power
- A **0.5 mm trunk on F.Cu** from **J5.16** to **J3.7**, with short branches to the nearby 3.3 V parts
  (R2, R3, D1/LED resistor, C4, TP1).
- **One via** drops the net to **B.Cu**, and a short bottom run feeds **U2** (CAN transceiver VIO) and **C1** (100 nF decoupling).

### 5.5 +5V power
- A **0.5 mm trunk on F.Cu** to **J4.7** and **C3** (5 V bulk cap, placed right beside the trunk).
- A **B.Cu run goes straight down** to **U2** (VCC) and **C2** (100 nF decoupling).
- One via connects the top trunk to the bottom run.

### 5.6 CAN logic: `/CAN_TX`, `/CAN_RX`
- Direct, short **F.Cu** tracks from the J5 header down to U2 (TXD / RXD pins). No vias.

### 5.7 CAN bus: CANH / CANL
The board has two identical CAN pairs, one per screw terminal. Each pair is named in KiCad after its terminal
(no custom label): `Net-(J1-Pin_1)` / `Net-(J1-Pin_2)` (CANH / CANL on J1) and `Net-(J2-Pin_1)` / `Net-(J2-Pin_2)` (J2).
- Each pair is routed **entirely on F.Cu**, with no layer change, so it is suitable for CAN FD tests.
- **Not length-matched, on purpose:** CANH is the outer track and CANL the inner one, so CANH is longer (about 45.3 mm
  against 34.6 mm for CANL, a difference of about 10.7 mm for each pair). The earlier CANL serpentine was removed because CAN
  does not need meanders: the difference is about 70 ps of skew, against a bit time of 200 ns even at 5 Mbit/s in CAN FD.
- **Corners:** there are no 90° corners on the CAN pairs or on the termination links. Every turn is a pair of 45° bends
  (a 1 mm chamfer), which keeps the impedance step small and avoids reflections.
- **Spacing:** the pair runs at 0.25 mm track width and 0.5 mm pitch, apart from where CANH fans out to the ESD diode pad.
- **ESD diodes:** D1 (J1 pair) and D2 (J2 pair) sit in-line on the pair at the corner of the terminal block, about 4 mm
  from the GND pin and about 14 mm of track from pin 1. They cannot go closer because the terminal body covers the area
  below the pins. The diode GND pad has a short trace to a GND via between its two signal pads.
- **Routing:** the pair leaves the transceiver, goes down past the ESD diode and passes under the
  terminal block. It enters the J1 / J2 pins from the south, which is what keeps CANH on the correct side without a via.
- **Termination:** the 120 Ω resistor and its jumper sit next to the pair, with stubs of about 2 mm each
  (R1 + J4 for the J1 pair, R2 + J6 for the J2 pair). The only layer change is the resistor-to-jumper link
  (`Net-(J4-Pin_2)` / `Net-(J6-Pin_2)`), which crosses under the pair on B.Cu through one via.
- **GND stitching vias:** one for each diode GND pad (see above) and four more beside the transceivers at (45.2, 123.9), (50.4, 124.3), (75.2, 123.9) and (80.4, 124.3), so the GND pour pockets enclosed by
  the pair stay connected to the B.Cu pour.

### 5.8 Pull-ups: `/MB_SCL`, `/MB_SDA`, `/MB_INT`, `/MB_RST`
The carrier acts as the mikroBUS socket (host side), so it provides the pull-ups. All four go to **+3V3**:

| Part | Value | Net | Why |
|---|---|---|---|
| R5 | 4.7 kΩ | `/MB_SCL` | I²C clock (open-drain) |
| R6 | 4.7 kΩ | `/MB_SDA` | I²C data (open-drain) |
| R7 | 10 kΩ | `/MB_INT` | Interrupt from the click board (open-drain, active low) |
| R8 | 10 kΩ | `/MB_RST` | Keeps reset released while nothing drives it |

- **Schematic:** the four resistors are at the bottom of the sheet, each with a `+3V3` label on one pin and the net label on the other.
- **PCB:** R5–R7 sit just west of the via column at x ≈ 64 mm / y ≈ 69–74 mm, where `/MB_INT`, `/MB_SCL` and `/MB_SDA` come up to F.Cu. Each has a 1.5 mm stub from its via. A short +3V3 rail at x = 59.2 mm joins the +3V3 trunk at y = 80 mm.
- R8 sits below them. `/MB_RST` runs on B.Cu there, so one extra via at (64.8, 75.1) brings it to F.Cu for R8.
- **Silkscreen:** the value text is hidden on R5–R8 and their reference designators are on the F.Fab layer, because the 2.5 mm spacing leaves no room for silkscreen text.
- If the Nucleo or the click board already has pull-ups on these lines, the extra ones are harmless (4.7 kΩ in parallel is about 2.35 kΩ). Remove R5–R8 only if the combined value would be too strong for your bus speed.

### 5.9 Orientation marks (silkscreen)
The board is a symmetric rectangle with two 2×35 sockets, so it could be plugged onto the Nucleo rotated by 180°. That would put
5 V / 3.3 V on the wrong pins. To prevent this, both silkscreen layers carry orientation text:

| Mark | Where | Meaning |
|---|---|---|
| ▲ + `NUCLEO ST-LINK USB END` | top edge, centred (y ≈ 47–50 mm) | This end goes over the Nucleo's ST-LINK USB connector. Pin 1 of J7 and J8 is here. |
| `NUCLEO ETH / USB END` | bottom edge, centred (y ≈ 139 mm), bottom silk only | This end goes over the Nucleo's RJ45 Ethernet connector and user USB. The screw terminals J1 and J2 are on this side. |
| `J7 PIN 1` / `J8 PIN 1` with arrows | next to the top corners | The arrow points at pin 1 of each 2×35 socket. |

- The texts are on **both** `F.SilkS` and `B.SilkS`. The bottom text is mirrored so it reads correctly when you flip the board over.
  The `NUCLEO ETH / USB END` text is bottom-only, because the top side of that end is covered by the terminal block silkscreen.
- **How the ends were determined:** from the Nucleo 3D model (`NUCLEO-F439ZI-carrier-aligned.step`). The ST-LINK USB connector (CN1)
  is at one end of the Nucleo and the RJ45 at the other. The carrier's pin-1 end (y ≈ 50 mm) is over the ST-LINK end. Please confirm this
  once against the real board before ordering.

### 5.10 Ground (GND)
- A **ground pour (zone) on both F.Cu and B.Cu** covering the whole board.
- Every GND pad connects into the pour (thermal-relief spokes: 0.4 mm bridges, 0.3 mm gap).
- **Vias join the top and bottom pours** so both are one ground. (The board currently has 6 GND vias: two at the ESD diodes and four near the CAN transceivers – see §8.)
- Because of the pour, you do not see individual GND tracks; ground is "everywhere there is no other signal".

---

## 6. Parts that were moved (none of J3–J6)

R1, R2, R3, C1, C3, C4, D1, D2, J2, TP1, TP2.

Why:
- **3.3 V loads** (R2, R3, C4, D1, C1) were moved next to the 3V3 trunk → shorter, simpler power tracks.
- **5 V cap** (C3) moved beside the 5V trunk.
- **D2, R1, J2** were moved **below J1**, because the body of the J1 screw terminal covered their old position.
- ⚠ **Check:** if the wires enter J1 from the **top** side, make sure D2, R1 and J2 don't block the wire entry.

---

## 7. Other changes made
- Added a **`Power` net class** in the project: **0.5 mm** tracks for `+5V` and `+3V3`
  (Board Setup → Design Rules → Net Classes).
- Old routing removed (it had 5 warnings, 8 unconnected items, dangling B.Cu tracks with no vias,
  long diagonal runs and no ground pour) and re-routed from scratch.
- Previous board saved as `CAN SPE CARRIER-backups/CAN SPE CARRIER-before-routing.kicad_pcb`
  (use *File → Restore Local Backup…* or just open that file).

---

## 8. How to review it yourself (step by step)

1. Open `CAN SPE CARRIER.kicad_pcb` (from the project, via the PCB Editor).
2. Press `B` to refill zones.
3. Run **Inspect → Design Rules Checker → Run DRC**. Expected: 0 errors, 0 unconnected items.
   (Tick *"Test for parity between PCB and schematic"* too.)
4. Walk through the nets: press `` ` `` on each track and read the list in §5.
5. Use **View → 3D Viewer** (`Alt+3`) to check that parts don't collide with J1.
6. Use *Appearance → Layers* to view only `F.Cu`, then only `B.Cu`.

### Things to improve / double-check
- **Stitching vias:** only 6 GND vias exist (two at the ESD diodes, four near the transceivers and the termination resistors). Adding a few more near the edges and next to the signal vias
  (*Place → Via*, set net to `GND`) improves return-current paths between the two ground pours.
- **Check the current rating:** 0.5 mm copper at 1 oz handles roughly 1 A; verify this is enough for your 3V3 and 5V loads.
- **Fab capabilities:** the design uses 0.2 mm clearance, 0.25 mm signal tracks and 0.6/0.3 mm vias.
  Confirm your manufacturer allows these (most standard 2-layer services do).
- **Lock files:** delete `~*.lck` if KiCad keeps warning you.

---

## 9. Handy KiCad shortcuts (PCB Editor)

| Key | Action |
|---|---|
| `` ` `` | Highlight net under cursor |
| `B` | Refill all zones |
| `X` | Start routing a track |
| `V` | Place a via while routing (also switches layer) |
| `D` | Drag a track |
| `M` / `G` | Move / Grab (keeps connections) |
| `Del` | Delete selected |
| `Ctrl+Z` | Undo |
| `PgUp/PgDn` | Switch active copper layer |
| `Alt+3` | 3D viewer |
| `Ctrl+Shift+M` | Measure tool |
| `Home` | Zoom to fit |
