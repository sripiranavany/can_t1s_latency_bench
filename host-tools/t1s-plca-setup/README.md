# t1s-plca-setup

Scripts to automatically get a Microchip LAN8670-based 10BASE-T1S USB
adapter (e.g. EVB-LAN8670-USB) into a correctly-bound, PLCA-configured state
on every boot and hot-plug event — plus sanity tests to validate
throughput/collision behavior once it's up.

For general background on the driver itself (building `microchip_t1s.ko`,
manual `ethtool` PLCA commands, why "Generic PHY" shows up), see
[`docs/t1s-driver-build.md`](../../docs/t1s-driver-build.md). This README
assumes that part is already done — i.e. `microchip_t1s.ko` is built and
`modprobe`-able, and you've confirmed manual `ethtool --set/get-plca-cfg`
works at least once.

---

## What's in this folder

| File | Purpose |
|---|---|
| `t1s-force-rebind.sh` | Detects Generic-PHY binding, forces `smsc95xx` unbind/rebind to fix it |
| `t1s-plca-config.sh` | Applies PLCA config via `ethtool`, reading node parameters from env files |
| `98-t1s-force-rebind.rules` | udev rule: runs the rebind script synchronously on adapter add |
| `99-t1s-hotplug.rules` | udev rule: starts the PLCA systemd service on adapter add |
| `t1s-plca@.service` | systemd oneshot template unit that applies PLCA config |

---

## Why two separate mechanisms (not one script/service)

A cold power-on races USB enumeration against kernel module loading. If
`smsc95xx` probes the adapter before `microchip_t1s` is loaded, the PHY
binds as **Generic PHY** and stays that way — the kernel doesn't retry on
its own. Fixing this needs an unbind/bind, which itself generates a fresh
`add` event.

- **`RUN+=` udev rule** (`98-...`) is the right tool for the rebind check:
  synchronous, runs inline as udev processes the event, no dependency on
  systemd being ready yet.
- **systemd oneshot service** (`99-...` → `t1s-plca@.service`) is the right
  tool for applying PLCA config: proper logging via `journalctl`, no
  udev-event timeout pressure.

**Execution order:** `98` sorts/runs before `99`. `RUN+=` scripts complete
synchronously before udev hands off `SYSTEMD_WANTS` units to systemd, so the
rebind always finishes before PLCA config runs — no explicit `After=`/
`Before=` needed between them.

**Expected (harmless) double-fire:** if the rebind script actually
triggers, the unbind/bind generates a second `add` event. Both rules fire
again — the rebind script becomes a no-op (driver's already correct), and
PLCA config gets re-applied a second time (idempotent). You'll see two log
entries in `journalctl` on boots where the race was actually hit; this is
expected, not a bug.

---

## Install

1. **Copy the config files:**

   Shared parameters — identical on every node/Pi:
   ```bash
   sudo tee /etc/t1s-plca-common.env <<'EOF'
   NODE_COUNT=2
   TO_TMR=32
   BURST_TMR=128
   EOF
   ```

   Per-node parameter — **only this differs across Pis**:
   ```bash
   sudo tee /etc/t1s-plca-node.env <<'EOF'
   NODE_ID=0
   EOF
   ```
   (second Pi gets `NODE_ID=1`, third gets `2`, etc.)

2. **Install the scripts:**
   ```bash
   sudo cp t1s-force-rebind.sh t1s-plca-config.sh /usr/local/bin/
   sudo chmod +x /usr/local/bin/t1s-force-rebind.sh /usr/local/bin/t1s-plca-config.sh
   ```

3. **Install the systemd unit:**
   ```bash
   sudo cp t1s-plca@.service /etc/systemd/system/
   sudo systemctl daemon-reload
   ```

4. **Install the udev rules:**
   ```bash
   sudo cp 98-t1s-force-rebind.rules 99-t1s-hotplug.rules /etc/udev/rules.d/
   sudo udevadm control --reload-rules
   ```

5. **Edit `99-t1s-hotplug.rules`** to match this Pi's adapter MAC address
   (`ATTR{address}=="..."`) — this must be set per Pi, it can't be shared.

---

## Validate

**Always test with a real cold power-off/power-on, not just `reboot` or
`systemctl restart`** — the race this setup fixes is specifically an
enumeration-timing issue that's most visible on a genuine cold boot.

```bash
readlink -f /sys/class/net/eth1/phydev/driver
ethtool --get-plca-cfg eth1
ethtool --get-plca-status eth1
journalctl -u t1s-plca@eth1.service -b
```

Expected: PHY driver shows Microchip's driver (not Generic PHY), PLCA
config matches your `.env` files, and `Status: on`.

---

## Throughput / collision sanity tests

Once two nodes are both up with PLCA configured, validate the bus.

**Background:** 10BASE-T1S is 10 Mbps half-duplex, shared bus. With PLCA
correctly arbitrating, classic CSMA/CD collisions shouldn't occur at all —
so the real sanity checks are (a) frame loss and (b) PLCA falling back to
CSMA/CD due to lost beacon sync, not "collision counters" in the
traditional sense.

### Raw L2 count/loss test (`packETHcli`)

Avoids the IP stack — closest to the eventual latency-measurement use case
this rig is built for.

```bash
sudo apt install -y packeth python3-scapy
```

Build a small pcap (packETHcli replays from a `.pcap` file, it doesn't
build packets from inline CLI flags):

```bash
python3 -c "
from scapy.all import Ether, Raw, wrpcap
pkt = Ether(dst='9c:95:6e:b5:88:4c', src='9c:95:6e:b5:64:78', type=0x88b5)/Raw(load=b'A'*46)
wrpcap('p1.pcap', pkt)
"
```

**Receiver Pi:**
```bash
sudo packETHcli -i eth1 -m 9 -x
```

**Sender Pi:**
```bash
sudo packETHcli -i eth1 -m 2 -B 5 -n 1000 -f p1.pcap -x
```

Compare sent (1000) vs. received count on the other Pi. Any gap = dropped
frames somewhere in the chain. Repeat at a few rates (`-B 1`, `-B 5`,
`-B 9`) looking for where received count starts falling behind.

### Bandwidth/saturation test (`iperf3`)

Goes through the IP stack; gives direct loss %/jitter.

```bash
iperf3 -s
# "Address already in use"? sudo ss -tlnp | grep 5201, then pkill iperf3
# or use a different port on both ends: iperf3 -s -p 5301
```

```bash
iperf3 -c <receiver-ip> -u -b 2M   -t 20 -i 5
iperf3 -c <receiver-ip> -u -b 5M   -t 20 -i 5
iperf3 -c <receiver-ip> -u -b 8M   -t 20 -i 5
iperf3 -c <receiver-ip> -u -b 9.5M -t 20 -i 5
```

Loss should stay ~0% until close to the practical ceiling (below 10 Mbps
once PLCA arbitration overhead and Ethernet framing/IFG are accounted for),
then climb sharply — that knee is the real achievable throughput.

### Watch for PLCA fallback during either test

```bash
watch -n 1 'ethtool --get-plca-status eth1; echo; ip -s link show eth1'
```

- `Status: on` should hold steady throughout. A flip to `off` means the
  node lost beacon sync and fell back to CSMA/CD — real collisions become
  possible at that point, and should show up in `TX: ... carrier collsns`.
- `errors`, `dropped`, `carrier`, `collsns` should stay at 0 regardless of
  load. Any nonzero value, even under stress, is worth investigating.

---
