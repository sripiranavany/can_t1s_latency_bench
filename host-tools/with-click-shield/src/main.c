#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/drivers/can.h>

LOG_MODULE_REGISTER(main_app, LOG_LEVEL_INF);

#define CAN_NODE DT_NODELABEL(can1)
static const struct device *can_dev = DEVICE_DT_GET(CAN_NODE);

int main(void) {
    LOG_INF("Starting Eth & CAN Initialization Test");

    /* Ethernet Status Check*/
    struct net_if *iface = net_if_get_default();
    if (!iface) {
        LOG_ERR("No default network interface found!");
    } else  {
        LOG_INF("Ethernet interface initialized successfully.");
    }

    /* CAN Controller Check*/
    if (!device_is_ready(can_dev)) {
        LOG_ERR("CAN device %s is not ready!", can_dev->name);
        return 0;
    }

    /* Start the CAN Controller*/
    int ret = can_start(can_dev);
    if (ret != 0) {
        LOG_ERR("Failed to start CAN controller: %d", ret);
        return 0;
    }

    LOG_INF("CAN1 controller started successfully.");

    /* Keep the main thread alive*/
    while (1) {
        k_sleep(K_SECONDS(1));
    }

    return 0;
}