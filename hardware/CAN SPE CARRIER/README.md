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
outputs go through the protection diode D2 and termination R1 to the screw terminal J1 (and jumper J2).**

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
These are named `Net-(J1-Pin_1)` and `Net-(J1-Pin_2)` in KiCad (no custom label).
- Routed as a **matched pair** on F.Cu above J1: same layer, same length-ish, constant spacing.
- They are arranged so they **do not cross** each other on the way to D2 (ESD protection), R1 (120 Ω termination) and J1.
- Good practice for CAN: keep the pair together and symmetrical – that is what was done here.

### 5.8 Ground (GND)
- A **ground pour (zone) on both F.Cu and B.Cu** covering the whole board.
- Every GND pad connects into the pour (thermal-relief spokes: 0.4 mm bridges, 0.3 mm gap).
- **Vias join the top and bottom pours** so both are one ground. (The board currently has only 2 GND vias – see §8.)
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
- **Stitching vias:** only 2 GND vias exist. Adding a few more near the edges and next to the signal vias
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
