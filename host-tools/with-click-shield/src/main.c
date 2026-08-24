#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>
#include <zephyr/net/phy.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/promiscuous.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>
#include <errno.h>

static struct net_mgmt_event_callback carrier_cb;

static void carrier_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
				   struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_ETHERNET_CARRIER_ON) {
		printf("Carrier ON  (link up),   Timestamp: %lld ms\n", k_uptime_get());
	} else if (mgmt_event == NET_EVENT_ETHERNET_CARRIER_OFF) {
		printf("Carrier OFF (link down), Timestamp: %lld ms\n", k_uptime_get());
	}
}

static const struct device *const eth_dev = DEVICE_DT_GET(DT_NODELABEL(eth0));

/* Read back what the PHY actually has in its PLCA registers, rather than what
 * devicetree asked for - the two differ. phy_mc_t1s applies the DT properties
 * at init, but forces node_count to 0 on any node whose plca-node-id is not 0,
 * because only the coordinator (node 0) advertises the count.
 */
static struct net_mgmt_event_callback ipv4_cb;

static void ipv4_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
			       struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}

	for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
		char buf[NET_IPV4_ADDR_LEN];

		if (iface->config.ip.ipv4->unicast[i].ipv4.addr_type != NET_ADDR_DHCP) {
			continue;
		}

		printf("DHCPv4 address: %s\n",
		       net_addr_ntop(NET_AF_INET,
				     &iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr, buf,
				     sizeof(buf)));
		printf("DHCPv4 subnet:  %s\n",
		       net_addr_ntop(NET_AF_INET, &iface->config.ip.ipv4->unicast[i].netmask, buf,
				     sizeof(buf)));
		printf("DHCPv4 router:  %s\n",
		       net_addr_ntop(NET_AF_INET, &iface->config.ip.ipv4->gw, buf, sizeof(buf)));
		printf("DHCPv4 lease:   %u seconds, Timestamp: %lld ms\n",
		       iface->config.dhcpv4.lease_time, k_uptime_get());
	}
}

static uint32_t rx_frames;

/* One line per received frame. Note the cost: printf() over the console UART is
 * far slower than a 10BASE-T1S frame, so leaving this enabled will distort any
 * latency measurement and will drop frames under load. Fine for bring-up and
 * for confirming who is on the bus; turn it off before taking numbers.
 */
static void rx_frame_report(const uint8_t *buf, size_t len)
{
	const struct net_eth_hdr *hdr = (const struct net_eth_hdr *)buf;

	rx_frames++;

	if (len < sizeof(*hdr)) {
		printf("RX #%u: runt, %zu bytes, Timestamp: %lld ms\n", rx_frames, len,
		       k_uptime_get());
		return;
	}

	printf("RX #%u: %02x:%02x:%02x:%02x:%02x:%02x -> %02x:%02x:%02x:%02x:%02x:%02x "
	       "type=0x%04x len=%zu, Timestamp: %lld ms\n",
	       rx_frames, hdr->src.addr[0], hdr->src.addr[1], hdr->src.addr[2], hdr->src.addr[3],
	       hdr->src.addr[4], hdr->src.addr[5], hdr->dst.addr[0], hdr->dst.addr[1],
	       hdr->dst.addr[2], hdr->dst.addr[3], hdr->dst.addr[4], hdr->dst.addr[5],
	       ntohs(hdr->type), len, k_uptime_get());
}

/* Raw AF_PACKET socket bound to the interface.
 *
 * Needed because EtherType 0x88b5 (and anything else non-IP) has no handler in
 * the IP stack - such frames are dropped at L2 and never surface to the app.
 * A packet socket sees them regardless of protocol.
 *
 * It does NOT bypass the MAC's address filter: the LAN8651 only passes frames
 * addressed to our MAC or to broadcast/multicast unless promiscuous mode is on.
 * Address your test traffic to this node's MAC, or re-enable
 * CONFIG_NET_PROMISCUOUS_MODE to sniff everything.
 */
static int rx_socket_open(struct net_if *iface)
{
	struct net_sockaddr_ll addr = {0};
	int sock;
	int ret;

	sock = zsock_socket(NET_AF_PACKET, NET_SOCK_RAW, htons(ETH_P_ALL));
	if (sock < 0) {
		printf("RX socket: create failed, errno %d\n", errno);
		return -1;
	}

	addr.sll_family = NET_AF_PACKET;
	addr.sll_ifindex = net_if_get_by_iface(iface);

	ret = zsock_bind(sock, (const struct net_sockaddr *)&addr, sizeof(addr));
	if (ret < 0) {
		printf("RX socket: bind failed, errno %d\n", errno);
		zsock_close(sock);
		return -1;
	}

	return sock;
}

static void plca_report(struct net_if *iface)
{
	const struct device *phy = net_eth_get_phy(iface);
	struct phy_plca_cfg cfg = {0};
	bool sts = false;
	int ret;

	if (phy == NULL) {
		printf("PLCA: no PHY bound to this interface\n");
		return;
	}

	ret = phy_get_plca_cfg(phy, &cfg);
	if (ret < 0) {
		printf("PLCA: cfg read failed (%d)\n", ret);
		return;
	}

	printf("PLCA: enable=%d node_id=%u node_count=%u burst_count=%u burst_timer=0x%02x "
	       "to_timer=0x%02x\n",
	       cfg.enable, cfg.node_id, cfg.node_count, cfg.burst_count, cfg.burst_timer,
	       cfg.to_timer);

	ret = phy_get_plca_sts(phy, &sts);
	if (ret < 0) {
		printf("PLCA: status read failed (%d)\n", ret);
		return;
	}

	printf("PLCA: status=%d (%s)\n", sts,
	       sts ? "operating - beacon seen" : "NOT operating - no beacon on the segment");

	if (cfg.enable && !sts) {
		printf("PLCA: exactly one node on the segment must have plca-node-id = <0>;\n");
		printf("      it is the coordinator that sends the BEACON. Without it PLCA\n");
		printf("      stays idle and the segment falls back to CSMA/CD.\n");
	}
}

int main(void)
{
	printf("main() started, Timestamp: %lld ms\n", k_uptime_get());

	if (!device_is_ready(eth_dev)) {
		printf("Error: LAN865x device not ready (init failed), Timestamp: %lld ms\n",
		       k_uptime_get());
		return 0;
	}

	printf("LAN865x device ready, Timestamp: %lld ms\n", k_uptime_get());

	struct net_if *iface = net_if_get_default();

	if (!iface) {
		printf("Error: no network interface found\n");
		return 0;
	}

	uint8_t *mac = net_if_get_link_addr(iface)->addr;

	printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4],
	       mac[5]);

	net_mgmt_init_event_callback(&carrier_cb, carrier_event_handler,
				      NET_EVENT_ETHERNET_CARRIER_ON |
					      NET_EVENT_ETHERNET_CARRIER_OFF);
	net_mgmt_add_event_callback(&carrier_cb);

	plca_report(iface);

	/* DHCPv4 does not start itself - the stack only reacts once the app asks
	 * for it. Needs CONFIG_NET_IPV4 as well as CONFIG_NET_DHCPV4; without
	 * IPV4 the DHCPV4 symbol is silently dropped at Kconfig time.
	 */
	net_mgmt_init_event_callback(&ipv4_cb, ipv4_event_handler, NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&ipv4_cb);

	printf("Starting DHCPv4...\n");
	net_dhcpv4_start(iface);

	/* Promiscuous mode so we see every frame on the segment, not just the
	 * ones addressed to our MAC or to broadcast. The clone handed to us by
	 * net_if_recv_data() is taken before L2 processing, so the Ethernet
	 * header is still intact.
	 */
	if (IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE)) {
		int ret = net_promisc_mode_on(iface);

		if (ret < 0 && ret != -EALREADY) {
			printf("Warning: promiscuous mode failed (%d)\n", ret);
		} else {
			printf("Promiscuous mode on - every frame on the segment is visible\n");
		}
	} else {
		printf("Promiscuous mode off - only frames addressed to %02x:%02x:%02x:%02x:%02x:"
		       "%02x, broadcast or multicast will arrive\n",
		       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}

	int sock = rx_socket_open(iface);

	if (sock < 0) {
		printf("No RX socket - received frames will not be printed\n");
	} else {
		printf("Raw AF_PACKET socket open - printing every frame received\n");
	}

	/* Poll, but only print status when something actually changes - carrier
	 * transitions already arrive through carrier_event_handler, so a
	 * periodic reprint of an unchanged state is just console noise.
	 */
	int prev_up = -1, prev_carrier = -1, prev_oper = -1;

	static uint8_t rx_buf[NET_ETH_MAX_FRAME_SIZE];

	while (1) {
		if (sock >= 0) {
			/* Wait up to 500 ms for a frame, so the status check below
			 * still runs on a quiet bus.
			 */
			struct zsock_pollfd fds = {.fd = sock, .events = ZSOCK_POLLIN};

			if (zsock_poll(&fds, 1, 500) > 0) {
				int n = zsock_recv(sock, rx_buf, sizeof(rx_buf), 0);

				if (n > 0) {
					rx_frame_report(rx_buf, (size_t)n);
				} else if (n < 0) {
					printf("RX socket: recv error, errno %d\n", errno);
				}
			}
		} else {
			k_msleep(500);
		}

		int up = net_if_is_up(iface);
		int carrier = net_if_is_carrier_ok(iface);
		int oper = net_if_oper_state(iface);

		if (up != prev_up || carrier != prev_carrier || oper != prev_oper) {
			printf("iface up=%d carrier_ok=%d oper_state=%d,  Timestamp: %lld ms\n", up,
			       carrier, oper, k_uptime_get());
			prev_up = up;
			prev_carrier = carrier;
			prev_oper = oper;
		}
	}

	return 0;
}
