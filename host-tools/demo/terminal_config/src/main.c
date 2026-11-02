#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>

struct app_config {
    uint32_t can_bitrate;
    uint32_t tx_period_ms;
    bool     loopback;
};

static struct app_config cfg = {
    .can_bitrate  = 500000,
    .tx_period_ms = 100,
    .loopback     = false,
};
static K_MUTEX_DEFINE(cfg_lock);

static int cmd_set_bitrate(const struct shell *sh, size_t argc, char **argv)
{
    char *end;
    unsigned long v = strtoul(argv[1], &end, 10);

    if (*end != '\0' || (v != 125000 && v != 250000 && v != 500000 && v != 1000000)) {
        shell_error(sh, "bitrate must be 125000/250000/500000/1000000");
        return -EINVAL;
    }
    k_mutex_lock(&cfg_lock, K_FOREVER);
    cfg.can_bitrate = v;
    k_mutex_unlock(&cfg_lock);
    shell_print(sh, "CAN bitrate = %lu (takes effect on next 'can restart')", v);
    return 0;
}

static int cmd_set_period(const struct shell *sh, size_t argc, char **argv)
{
    uint32_t v = strtoul(argv[1], NULL, 10);

    if (v < 1 || v > 10000) {
        shell_error(sh, "period 1..10000 ms");
        return -EINVAL;
    }
    k_mutex_lock(&cfg_lock, K_FOREVER);
    cfg.tx_period_ms = v;
    k_mutex_unlock(&cfg_lock);
    return 0;
}

static int cmd_show(const struct shell *sh, size_t argc, char **argv)
{
    k_mutex_lock(&cfg_lock, K_FOREVER);
    shell_print(sh, "can_bitrate  : %u", cfg.can_bitrate);
    shell_print(sh, "tx_period_ms : %u", cfg.tx_period_ms);
    shell_print(sh, "loopback     : %s", cfg.loopback ? "on" : "off");
    k_mutex_unlock(&cfg_lock);
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(cfg_cmds,
    SHELL_CMD_ARG(bitrate, NULL, "Set CAN bitrate <bps>", cmd_set_bitrate, 2, 0),
    SHELL_CMD_ARG(period,  NULL, "Set TX period <ms>",    cmd_set_period,  2, 0),
    SHELL_CMD(show, NULL, "Show config", cmd_show),
    SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(cfg, &cfg_cmds, "Application configuration", NULL);

int main(void)
{
    while (1) {
        k_mutex_lock(&cfg_lock, K_FOREVER);
        uint32_t period = cfg.tx_period_ms;
        k_mutex_unlock(&cfg_lock);
        /* ... send CAN frame ... */
        k_msleep(period);
    }
}