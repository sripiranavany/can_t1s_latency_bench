From the Pi

```
python3 -c "
from scapy.all import Ether, Raw, wrpcap
import struct
pkts = [Ether(dst='02:00:00:00:00:01', src='9c:95:6e:b5:88:4c', type=0x88b5)
        / Raw(load=struct.pack('!I', i) + b'A'*42) for i in range(1000)]
wrpcap('p1.pcap', pkts)
"
```

Run

```
sudo packETHcli -i eth1 -m 2 -B 5 -n 10 -f p1.pcap -x
```

Make sure the Coordinator is running
