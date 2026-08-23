#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_mgmt.h>

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

	printf("Entering status loop...\n");

	while (1) {
		printf("iface up=%d carrier_ok=%d oper_state=%d,  Timestamp: %lld ms\n",
		       net_if_is_up(iface), net_if_is_carrier_ok(iface),
		       net_if_oper_state(iface), k_uptime_get());
		k_msleep(2000);
	}

	return 0;
}
