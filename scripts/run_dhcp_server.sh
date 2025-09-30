#!/usr/bin/env bash
set -euo pipefail

# === CONFIG ===
IFACE="enp46s0"                 # Primary interface (for single mode or bridge member)
BRIDGE_IFACE="enp0s20f0u1"      # Secondary interface (for bridge mode)
BR_NAME="br0"
CON_NAME="dhcp-server-$IFACE"
BR_CON_NAME="dhcp-server-$BR_NAME"
DHCP_CONF="/etc/dhcp/dhcpd.conf"
SYSCONF="/etc/sysconfig/dhcpd"
SUBNET="192.168.50.0"
RANGE_START="192.168.50.10"
RANGE_END="192.168.50.100"
ROUTER_IP="192.168.50.1"
NETMASK="255.255.255.0"
DNS="8.8.8.8"

# === FUNCTIONS ===

check_iface() {
    local iface="$1"
    if ! ip link show "$iface" &>/dev/null; then
        echo "Interface '$iface' does not exist, please update interface name."
        exit 1
    fi
}

enable_dhcp_single() {
    check_iface "$IFACE"

    echo "[*] Enabling DHCP server on $IFACE (single interface mode)"

    if ! rpm -q dhcp-server >/dev/null 2>&1; then
        sudo dnf install -y dhcp-server
    fi

    if ! nmcli connection show "$CON_NAME" >/dev/null 2>&1; then
        sudo nmcli connection add type ethernet ifname "$IFACE" con-name "$CON_NAME" ipv4.method manual ipv4.addresses "$ROUTER_IP/24"
    fi
    sudo nmcli connection up "$CON_NAME"

    write_dhcpd_conf

    echo "DHCPDARGS=$IFACE" | sudo tee $SYSCONF >/dev/null
    sudo systemctl enable --now dhcpd
    echo "[+] DHCP server enabled on $IFACE"
}

enable_dhcp_bridge() {
    check_iface "$IFACE"
    check_iface "$BRIDGE_IFACE"

    echo "[*] Enabling DHCP server on bridge $BR_NAME ($IFACE + $BRIDGE_IFACE)"

    if ! rpm -q dhcp-server >/dev/null 2>&1; then
        sudo dnf install -y dhcp-server
    fi

    # Create bridge
    if ! nmcli connection show "$BR_CON_NAME" >/dev/null 2>&1; then
        sudo nmcli connection add type bridge con-name "$BR_CON_NAME" ifname "$BR_NAME" ipv4.method manual ipv4.addresses "$ROUTER_IP/24"
        sudo nmcli connection add type bridge-slave ifname "$IFACE" master "$BR_NAME"
        sudo nmcli connection add type bridge-slave ifname "$BRIDGE_IFACE" master "$BR_NAME"
    fi
    sudo nmcli connection up "$BR_CON_NAME"

    write_dhcpd_conf

    echo "DHCPDARGS=$BR_NAME" | sudo tee $SYSCONF >/dev/null
    sudo systemctl enable --now dhcpd
    echo "[+] DHCP server enabled on $BR_NAME (bridged)"
}

disable_dhcp() {
    echo "[*] Disabling DHCP server"

    sudo systemctl disable --now dhcpd || true

    # Remove configs
    sudo nmcli connection delete "$CON_NAME" >/dev/null 2>&1 || true
    sudo nmcli connection delete "$BR_CON_NAME" >/dev/null 2>&1 || true
    sudo rm -f "$DHCP_CONF"
    sudo sed -i "/^DHCPDARGS=/d" "$SYSCONF" || true

    echo "[+] DHCP server disabled and cleaned up"
}

write_dhcpd_conf() {
    sudo bash -c "cat > $DHCP_CONF" <<EOF
default-lease-time 600;
max-lease-time 7200;
authoritative;

subnet $SUBNET netmask $NETMASK {
    range $RANGE_START $RANGE_END;
    option routers $ROUTER_IP;
    option subnet-mask $NETMASK;
    option domain-name-servers $DNS;
}
EOF
}

# === MAIN ===

case "${1:-}" in
    enable-single)
        enable_dhcp_single
        ;;
    enable-bridge)
        enable_dhcp_bridge
        ;;
    disable)
        disable_dhcp
        ;;
    *)
        echo "Usage: $0 {enable-single|enable-bridge|disable}"
        exit 1
        ;;
esac

