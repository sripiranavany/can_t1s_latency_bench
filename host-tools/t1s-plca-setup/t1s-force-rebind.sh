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