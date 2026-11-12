# Adding the Nucleo 3D model under the carrier (KiCad)

This guide shows how to put the **NUCLEO-F439ZI** 3D model under the CAN SPE
Carrier in KiCad's 3D viewer. It is useful for checking whether the carrier
fits on the Nucleo, for example whether the RJ45 Ethernet connector touches the
bottom of the carrier.

No special knowledge is needed. Just follow the steps.

---

## Quick answer

The Nucleo model is already added to this project. To see it:

1. Open the PCB in KiCad (`CAN SPE CARRIER.kicad_pcb`).
2. Open **View → 3D Viewer** (shortcut `Alt+3`).
3. In the 3D viewer, open **Preferences** and turn on **Show Footprints → Virtual** (also called "board-only").
4. Flip the board with **View → Flip Board** (`Shift+Y`) to look at the underside.

The model is on a helper footprint called `MECH1` (it is not a real part, only a 3D picture).

---

## Files involved

| File | What it is |
|---|---|
| `hardware/kicad-files/stepfile/NUCLEO-F439ZI--3DModel-STEP-56544.STEP` | The original model from ST. Y is "up" and the origin is the board centre. |
| `hardware/kicad-files/stepfile/NUCLEO-F439ZI-carrier-aligned.step` | A copy that is already rotated and moved into the right place under the carrier. |
| `CAN SPE CARRIER.kicad_pcb` | Contains the helper footprint `MECH1` that points at the aligned model. |

---

## How the model is attached

KiCad can only show a 3D model if it belongs to a **footprint**. You cannot add
a loose model to the board. So the trick is:

1. Make a footprint with **no pads** (it does nothing electrically).
2. Attach the Nucleo STEP file to it.
3. Mark it as **board-only** and exclude it from the BOM, so it never appears in your parts list.

---

## Option A: Add it yourself in the KiCad window (step by step)

Use this if you want to do it by hand, or if you use a different 3D model.

### Step 1: Place a footprint

1. In the PCB editor press **A** (Place → Add Footprint).
2. Choose any simple footprint, for example a `TestPoint`, and click on the board.
3. This footprint carries the 3D model.

### Step 2: Open its properties

Double-click the footprint, then open the **3D Models** tab.

### Step 3: Add the STEP file

1. Click the **folder icon** (or the + button).
2. Browse to `hardware/kicad-files/stepfile/` and pick the `.STEP` file.

### Step 4: Rotate it flat

The ST model has Y as its up axis, but KiCad expects Z. In the **Rotation** boxes:

| Try | Result |
|---|---|
| X = **−90** | Usually lies flat, correct way up. |
| X = **+90** | Use this if the model appears upside down. |

### Step 5: Move it under the carrier

Use the **Offset** boxes (values are in millimetres):

| Box | Meaning | Typical value |
|---|---|---|
| X | Move left or right | Adjust until the headers line up |
| Y | Move up or down the board | Adjust until the headers line up |
| Z | Height | About **−10.1** (carrier thickness 1.6 + socket height 8.5) |

A negative Z puts the Nucleo **below** the carrier. If you use male header
pins with a plastic base, the gap is bigger (about **−12.6**).

### Step 6: Check

Click **Preview** in the dialog or open the 3D viewer. If you cannot see the
model, see **Troubleshooting** below.

---

## Option B: Use the aligned file (already set up)

This is what the project uses. The aligned file
`NUCLEO-F439ZI-carrier-aligned.step` was rotated and moved **before** it was
loaded into KiCad. That means the footprint needs:

- **Rotation:** 0, 0, 0
- **Offset:** X 0, Y 0, **Z −10.1**

So you only need to change **Z** if the stack height changes.

To change it: double-click `MECH1` in the PCB → **3D Models** tab → change the
**Z** offset.

---

## What the stack height means

```
   Carrier PCB          (1.6 mm thick)
   ------------------
   Socket body          (8.5 mm tall)
   ------------------
   Nucleo PCB top surface
```

The Z offset is the distance from the top of the carrier down to the top of the
Nucleo: **1.6 + 8.5 = 10.1 mm**.

If the Nucleo has male morpho headers with a 2.5 mm plastic base, the sockets sit on that base, so use about **12.6 mm** instead.

---

## Troubleshooting

| Problem | What to do |
|---|---|
| I cannot see the Nucleo in the 3D viewer | Reload the PCB (File → Revert), then turn on **Show Footprints → Virtual** in the 3D viewer preferences. |
| It is very slow to open | The file is about 40 MB. Give it a few seconds the first time. |
| The model is upside down | Change the rotation from X = −90 to X = +90. |
| It is not under the carrier | Change the X and Y offsets. Check the headers `J7` and `J8` line up with the model's pin rows. |
| The model is there but invisible from above | The carrier hides it. Flip the board (`Shift+Y`) or use a side view. |
| The footprint shows up in my parts list | Open `MECH1` → Fabrication Attributes → tick **Exclude from bill of materials** and **Exclude from position files**. |

---

## What to look for in the 3D viewer

Check the end of the carrier near the **RJ45 Ethernet connector** and the two
taller blocks next to it.

- If the RJ45 or those blocks touch the underside of the carrier, you need a cutout, taller sockets, or a shorter board.
- If there is clear space between them, the carrier fits.

Useful measurements taken from the model (Nucleo PCB top = 0):

| Part | Height above the Nucleo PCB |
|---|---|
| RJ45 | about 7.3 mm |
| Two taller blocks near the RJ45 | about 10.7 mm |
| Arduino (Zio) headers | 8.5 mm |

---

## Notes

- The Nucleo model is for **checking only**. It is not part of the circuit, and it does not change the schematic or the manufactured board.
- If you move the carrier or change its outline, re-check the fit.
- Keep the original ST `.STEP` file in the repo if you want to make a new aligned copy later.
