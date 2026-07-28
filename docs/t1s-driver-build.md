# 10BASE-T1S on Linux: Driver Build & Manual PLCA Testing

Background reference for bringing up Microchip LAN8670-based 10BASE-T1S USB
adapters (e.g. EVB-LAN8670-USB) on a Linux host. This covers the general
driver/kernel side — for this repo's ready-to-use boot/hotplug automation
scripts, see [`host-tools/t1s-plca-setup/README.md`](../host-tools/t1s-plca-setup/README.md).

---

## Background

The EVB-LAN8670-USB adapter is two chips in series:

- **`smsc95xx`** — a USB-to-MII bridge chip. This is the netdev/MDIO-bridge
  driver you'll see in `ethtool -i <iface>`, and it is correct and expected
  there.
- **LAN8670** — the actual 10BASE-T1S PHY, sitting on the MII bus behind the
  bridge. **PLCA support comes from the PHY driver bound to this chip**, not
  from `smsc95xx`.

`microchip_t1s` (`drivers/net/phy/microchip_t1s.c`) is the mainline Linux PHY
driver providing PLCA support for LAN8670/1/2 and LAN8650/1. It's a
`tristate` Kconfig option (`CONFIG_MICROCHIP_T1S_PHY`), so whether any given
distro's kernel ships it — built-in, as a module, or not at all — is that
distro's own packaging decision, not a kernel-wide default. Raspberry Pi
OS / Debian's `rpt-rpi` kernel, for example, does not ship it by default.

---

## Prerequisites

- Matching kernel headers for your **running** kernel (`uname -r`)
- `build-essential` (or equivalent toolchain)
- `ethtool` ≥ 6.7 (PLCA netlink support was added around this version)

On Debian/Raspberry Pi OS Trixie, the headers package name changed from the
older `raspberrypi-kernel-headers` to a per-board meta-package:

```bash
sudo apt update
sudo apt install linux-headers-rpi-v8 build-essential   # Pi 4 / CM4, 64-bit
# or: linux-headers-rpi-2712                             # Pi 5
```

Confirm headers match the running kernel:

```bash
ls /usr/src/ | grep linux-headers
uname -r
```

---

## Step 1: Check whether the driver is already present

Don't assume you need to build it — check first:

```bash
zcat /proc/config.gz 2>/dev/null | grep -i MICROCHIP_T1S \
  || grep -i MICROCHIP_T1S /boot/config-$(uname -r)

find /lib/modules/$(uname -r)/kernel/drivers/net/phy/ -iname "*t1s*"
find /lib/modules/$(uname -r)/kernel/drivers/net/phy/ -iname "*microchip*"
```

- `CONFIG_MICROCHIP_T1S_PHY=m` and a matching `.ko` found → skip to
  [Step 3](#step-3-install-and-bind-the-driver), just `modprobe` it.
- `# CONFIG_MICROCHIP_T1S_PHY is not set` and nothing found → build it
  out-of-tree ([Step 2](#step-2-build-the-driver-out-of-tree)).

---

## Step 2: Build the driver out-of-tree

**Get the driver source matching your kernel version** — not `master`.
Kernel modules are not source-portable like userspace programs: the
driver's calls into internal kernel functions (`phy_read_mmd`,
`mdiobus_read`, `genphy_c45_plca_get_cfg`, etc.) can change signature or
availability between releases. Pulling from `master` risks either a build
failure or an `insmod` failure with `Unknown symbol ...` errors if it
doesn't match your running kernel's ABI.

```bash
mkdir -p ~/t1s_driver && cd ~/t1s_driver
wget https://raw.githubusercontent.com/torvalds/linux/v6.12/drivers/net/phy/microchip_t1s.c
```

(Replace `v6.12` with the tag matching your kernel's major.minor — e.g. a
`6.12.75+rpt-rpi-v8` kernel should use tag `v6.12`. Distro-patched kernels
like Raspberry Pi's `+rpt` builds aren't a byte-identical match to the
matching mainline tag, but it's far closer than blindly using `master`.)

**On newer kernels** the driver struct may reference cable-test/SQI ops not
present in the version you pulled. If the build fails on those, comment
them out (keep the PLCA ops intact):

```c
    .get_plca_cfg    = genphy_c45_plca_get_cfg,
    .set_plca_cfg    = lan86xx_plca_set_cfg,
    .get_plca_status = genphy_c45_plca_get_status,
    // .cable_test_start      = genphy_c45_oatc14_cable_test_start,
    // .cable_test_get_status = genphy_c45_oatc14_cable_test_get_status,
    // .get_sqi               = genphy_c45_oatc14_get_sqi,
    // .get_sqi_max           = genphy_c45_oatc14_get_sqi_max,
```

**Makefile:**

```makefile
obj-m += microchip_t1s.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
```

**Build:**

```bash
make
```

---

## Step 3: Install and bind the driver

```bash
sudo cp microchip_t1s.ko /lib/modules/$(uname -r)/kernel/drivers/net/phy/
sudo depmod -a
sudo modprobe -r microchip_t1s 2>/dev/null
sudo modprobe microchip_t1s
```

Force the USB bridge to re-probe so the correct PHY driver binds (find the
bus ID from `dmesg | grep "Product: 10BASE-T1S"` if unsure):

```bash
sudo sh -c 'echo 1-1.3:1.0 > /sys/bus/usb/drivers/smsc95xx/unbind'
sudo sh -c 'echo 1-1.3:1.0 > /sys/bus/usb/drivers/smsc95xx/bind'
```

**Verify:**

```bash
readlink -f /sys/class/net/eth1/phydev/driver
# expect a path ending in .../Microchip LAN867x, NOT "Generic PHY"

dmesg | tail -n 20
```

### Why "Generic PHY" shows up in the first place

If `microchip_t1s` isn't loaded yet at the exact moment `smsc95xx` probes
the adapter and scans its MDIO bus, the kernel falls back to binding the
**Generic PHY** driver — and it does **not** retry automatically later just
because a better driver becomes available. This is a general Linux
USB-enumeration-vs-module-load race, not something specific to this
hardware or distro; it just shows up more often on boards with variable
USB/power-rail init timing (e.g. cold power-on on a Raspberry Pi) than it
does with a consistently-timed boot sequence.

The unbind/bind above forces a fresh probe and is the fix for this
condition whenever you see Generic PHY bound instead of Microchip's driver.
See `host-tools/t1s-plca-setup/README.md` for making this self-correcting
automatically on every boot and hot-plug, rather than a manual step.

---

## Step 4: Manual PLCA testing

Correct flag names — there is no `--show-plca-cfg`; the actual commands are:

```bash
sudo ethtool --set-plca-cfg eth1 enable on node-id 1 node-cnt 2 to-tmr 32 burst-tmr 128
ethtool --get-plca-cfg eth1
ethtool --get-plca-status eth1
```

`--get-plca-cfg` returning `netlink error: Operation not supported` means
the operation reached the kernel but no PLCA-capable PHY driver is
currently bound (i.e. still Generic PHY) — go back to Step 3.

---
