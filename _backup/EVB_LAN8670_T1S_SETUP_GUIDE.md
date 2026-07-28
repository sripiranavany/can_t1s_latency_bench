# EVB-LAN8670 10BASE-T1S: Driver Build, PLCA Automation & Sanity Testing

Consolidated, corrected setup guide for bringing up the Microchip EVB-LAN8670-USB
10BASE-T1S adapters on Debian/Raspberry Pi OS (Trixie), automating PLCA config on
boot/hotplug, and validating throughput/collision behavior.

---

## Prerequisites

- **OS:** Debian 13 (Trixie) / Raspberry Pi OS
- **Hardware:** EVB-LAN8670-USB 10BASE-T1S adapter (uses `smsc95xx` USB-to-MII
  bridge + LAN8670 PHY)
- **Packages:** `build-essential`, matching `linux-headers-$(uname -r)`,
  `ethtool` ≥ 6.7 (for PLCA netlink support)

Check headers package name — on Trixie it is **not** `raspberrypi-kernel-headers`
(that name is gone). Use the `rpi-v8` (Pi 4/CM4, 64-bit) or `rpi-2712` (Pi 5)
meta-package instead:

```bash
sudo apt update
sudo apt install linux-headers-rpi-v8 build-essential
```

Confirm headers match the running kernel:

```bash
ls /usr/src/ | grep linux-headers
uname -r
```

---

## Step 1: Check whether the driver is already present

`microchip_t1s` is mainlined in upstream Linux (`drivers/net/phy/microchip_t1s.c`,
config option `CONFIG_MICROCHIP_T1S_PHY`), so check before building anything:

```bash
zcat /proc/config.gz 2>/dev/null | grep -i MICROCHIP_T1S \
  || grep -i MICROCHIP_T1S /boot/config-$(uname -r)

find /lib/modules/$(uname -r)/kernel/drivers/net/phy/ -iname "*t1s*"
find /lib/modules/$(uname -r)/kernel/drivers/net/phy/ -iname "*microchip*"
```

- `CONFIG_MICROCHIP_T1S_PHY=m` and a matching `.ko` present → skip to Step 3,
  just `modprobe` it.
- `# CONFIG_MICROCHIP_T1S_PHY is not set` and nothing found → build it
  out-of-tree (Step 2).

---

## Step 2: Build the driver out-of-tree

```bash
mkdir -p ~/t1s_driver && cd ~/t1s_driver
wget https://raw.githubusercontent.com/torvalds/linux/master/drivers/net/phy/microchip_t1s.c
```

**On newer kernels (6.18+)** the struct may reference cable-test/SQI ops not
present in this driver revision. If the build fails on those, comment them out
(keep the PLCA ops):

```c
    .get_plca_cfg    = genphy_c45_plca_get_cfg,
    .set_plca_cfg    = lan86xx_plca_set_cfg,
    .get_plca_status = genphy_c45_plca_get_status,
    // .cable_test_start     = genphy_c45_oatc14_cable_test_start,
    // .cable_test_get_status = genphy_c45_oatc14_cable_test_get_status,
    // .get_sqi              = genphy_c45_oatc14_get_sqi,
    // .get_sqi_max          = genphy_c45_oatc14_get_sqi_max,
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

Force the USB bridge to re-probe so the correct PHY driver binds (get the bus
ID from `dmesg | grep "Product: 10BASE-T1S"` if unsure):

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

> `smsc95xx` (netdev/MDIO-bridge driver) is correct and expected on `ethtool -i
> eth1` — PLCA support comes from the **PHY** driver bound underneath it, not
> from `smsc95xx` itself. Check the PHY driver specifically, not just the
> netdev driver name.

---

## Step 4: Manual PLCA testing

Correct flag names — there is no `--show-plca-cfg`; the get/set/status
commands are:

```bash
sudo ethtool --set-plca-cfg eth1 enable on node-id 1 node-cnt 2 to-tmr 32 burst-tmr 128
ethtool --get-plca-cfg eth1
ethtool --get-plca-status eth1
```

`--get-plca-cfg` returning `netlink error: Operation not supported` means the
op reached the kernel but no PLCA-capable PHY driver is bound (i.e. still
Generic PHY) — go back to Step 3.

---

## Step 5: Automate PLCA config for boot & hot-plug

### Why this needs two separate mechanisms

- A cold power-on races USB enumeration against module loading. If
  `smsc95xx` probes the adapter before `microchip_t1s` is loaded, the kernel
  falls back to **Generic PHY** and does not retry later on its own — the
  interface stays stuck with Generic PHY until something forces a fresh
  probe (unbind/bind, or physical replug).
- A `RUN+=` udev script (synchronous, short timeout) is the right tool for
  the rebind step. A systemd oneshot service (proper logging via
  `journalctl`, no timeout pressure) is the right tool for applying PLCA
  settings. Both are triggered from udev, in sequence.

### 5.1 Rebind script

`/usr/local/bin/t1s-force-rebind.sh`:

```sh
#!/bin/sh
# /usr/local/bin/t1s-force-rebind.sh
IFACE="${1:-eth1}"

DEV_PATH=$(readlink -f /sys/class/net/"$IFACE"/device 2>/dev/null)
if [ -z "$DEV_PATH" ]; then
    echo "Error: Network interface '$IFACE' device path not found."
    exit 1
fi
BUSID=$(basename "$DEV_PATH")

# Wait for phydev to actually attach before judging which driver bound
# (avoids reacting to the device appearing before the PHY has probed)
for i in 1 2 3 4 5 6 7 8 9 10; do
    [ -e /sys/class/net/"$IFACE"/phydev ] && break
    sleep 0.5
done

# Check if ethtool PLCA works; if it fails with 'not supported', rebind
if ethtool --get-plca-cfg "$IFACE" 2>&1 | grep -q "not supported"; then
    echo "Generic PHY or unbound driver detected on $IFACE ($BUSID). Re-binding smsc95xx..."
    echo "$BUSID" > /sys/bus/usb/drivers/smsc95xx/unbind 2>/dev/null
    sleep 1
    echo "$BUSID" > /sys/bus/usb/drivers/smsc95xx/bind 2>/dev/null

    # Wait again for phydev after the rebind
    for i in 1 2 3 4 5 6 7 8 9 10; do
        [ -e /sys/class/net/"$IFACE"/phydev ] && break
        sleep 0.5
    done
    echo "Rebind triggered for $BUSID."
else
    echo "Interface $IFACE is already bound and supporting PLCA commands."
fi
```

```bash
sudo chmod +x /usr/local/bin/t1s-force-rebind.sh
```

### 5.2 PLCA config systemd template unit

`/etc/systemd/system/t1s-plca@.service`:

```ini
[Unit]
Description=Apply 10BASE-T1S PLCA Configuration on %I
After=sys-subsystem-net-devices-%i.device
BindsTo=sys-subsystem-net-devices-%i.device

[Service]
Type=oneshot
ExecStartPre=/bin/sleep 1
ExecStart=/usr/sbin/ethtool --set-plca-cfg %i enable on node-id 1 node-cnt 2 to-tmr 32 burst-tmr 128
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
```

> `node-id` is the **one value that must differ per physical Pi** (e.g. `0`
> on one, `1` on the other). `node-cnt`/`to-tmr`/`burst-tmr` should match
> across all nodes on the same bus.

### 5.3 udev rules (two, different mechanisms — this is intentional)

`/etc/udev/rules.d/98-t1s-force-rebind.rules` — runs the rebind script
synchronously for any `smsc95xx`-based adapter:

```text
ACTION=="add", SUBSYSTEM=="net", DRIVERS=="smsc95xx", RUN+="/usr/local/bin/t1s-force-rebind.sh %k"
```

`/etc/udev/rules.d/99-t1s-hotplug.rules` — starts the PLCA systemd service,
matched on this specific adapter's MAC (replace per Pi):

```text
ACTION=="add", SUBSYSTEM=="net", ATTR{address}=="9c:95:6e:b5:64:78", TAG+="systemd", ENV{SYSTEMD_WANTS}="t1s-plca@%k.service"
```

**Ordering:** `98` sorts/executes before `99`. `RUN+=` scripts run
synchronously as udev finishes processing the event, before systemd is
handed any `SYSTEMD_WANTS` units to activate — so the rebind always
completes before PLCA config is applied, without needing explicit
`After=`/`Before=` between the two.

```bash
sudo systemctl daemon-reload
sudo udevadm control --reload-rules
```

**Expected (harmless) double-fire:** if the rebind script actually triggers
an unbind/bind (Generic PHY case), that generates a fresh `add` event for
the same interface. Both rules fire again — the rebind script becomes a
no-op, and PLCA config gets (re-)applied a second time. You'll see two log
lines in `journalctl -u t1s-plca@eth1.service -b` on boots where the race
was actually hit; this is expected and not a bug.

**Per-Pi differences to double check:**
- MAC address in `99-t1s-hotplug.rules`
- `node-id` in `t1s-plca@.service`

### 5.4 Validate with a real cold power cycle

Not just `systemctl restart` / `reboot` — physically power off and on, since
that's where the enumeration-timing race is most likely to bite:

```bash
readlink -f /sys/class/net/eth1/phydev/driver
ethtool --get-plca-cfg eth1
ethtool --get-plca-status eth1
journalctl -u t1s-plca@eth1.service -b
journalctl -t t1s-force-rebind -b   # if logging added to the script
```

---

## Step 6: Sanity-test throughput and collisions

Background: 10BASE-T1S is 10 Mbps half-duplex, shared bus. With PLCA active
and correctly arbitrating, classic CSMA/CD collisions should not occur at
all — so the real sanity check is (a) frame loss and (b) PLCA falling back
to CSMA/CD due to lost beacon sync, not "collision counters" in the
traditional sense.

### 6.1 Raw L2 count/loss test (packETHcli)

Avoids the IP stack — closest to the eventual latency-measurement use case.

```bash
sudo apt install -y packeth
```

Build a custom pcap (packETHcli replays from a `.pcap` file — it does not
build packets from inline CLI flags):

```bash
sudo apt install -y python3-scapy
python3 -c "
from scapy.all import Ether, Raw, wrpcap
pkt = Ether(dst='9c:95:6e:b5:88:4c', src='9c:95:6e:b5:64:78', type=0x88b5)/Raw(load=b'A'*46)
wrpcap('p1.pcap', pkt)
"
```

**Receiver Pi** (counts frames matching the embedded pattern):

```bash
sudo packETHcli -i eth1 -m 9 -x
```

**Sender Pi:**

```bash
sudo packETHcli -i eth1 -m 2 -B 5 -n 1000 -f p1.pcap -x
```

Sends 1000 frames at 5 Mbit/s with the pattern embedded (`-x`). Compare sent
vs. received count on the other Pi — any gap is a dropped frame at some
layer. Repeat at a few rates (e.g. `-B 1`, `-B 5`, `-B 9`) looking for where
received count starts falling behind sent count.

### 6.2 Bandwidth/saturation test (iperf3)

Goes through the IP stack; gives direct loss %/jitter.

```bash
iperf3 -s
# if "Address already in use":
sudo ss -tlnp | grep 5201     # find what's holding the port
pkill iperf3                 # or: sudo systemctl stop iperf3
# or just use a different port on both ends: iperf3 -s -p 5301
```

Sweep target bitrate toward the T1S ceiling from the sender:

```bash
iperf3 -c <receiver-ip> -u -b 2M   -t 20 -i 5
iperf3 -c <receiver-ip> -u -b 5M   -t 20 -i 5
iperf3 -c <receiver-ip> -u -b 8M   -t 20 -i 5
iperf3 -c <receiver-ip> -u -b 9.5M -t 20 -i 5
```

Watch `Lost/Total Datagrams` and jitter. Loss should stay ~0% until close to
the practical ceiling (below 10 Mbps once PLCA arbitration overhead and
Ethernet framing/IFG are accounted for), then climb sharply — that knee is
the real achievable throughput on this bus.

### 6.3 Watch for PLCA fallback during either test

On both Pis, in a separate terminal:

```bash
watch -n 1 'ethtool --get-plca-status eth1; echo; ip -s link show eth1'
```

- `PLCA status: on` should hold steady throughout. A flip to `off` mid-test
  means the node lost beacon sync and fell back to CSMA/CD — real
  collisions become possible at that point, and should show up in
  `TX: ... carrier collsns` in `ip -s link show`.
- Under healthy operation, `errors`, `dropped`, `carrier`, and `collsns` in
  `ip -s link show` should stay at 0 regardless of load. Any nonzero value,
  even under stress, is worth investigating rather than dismissing.

---