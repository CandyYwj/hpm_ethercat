#!/usr/bin/env bash
# ethercat_port_linkonly.sh - 把 EtherCAT 口设为"只亮不配"（无 IP、无路由）。
#
# 现象（实测第二台机器 wzsu）：管理口走 Wi-Fi（wlo1，192.168.111.70/22），
# EtherCAT 口是 enp2s0。只要把 EtherCAT 网线插进 enp2s0，SSH 就断；拔线即恢复。
#
# 原因：enp2s0 起来后被 NetworkManager 自动配置 IP/路由（DHCP 或遗留静态配置），
# 抢走回包路径 -> 非对称路由，SSH 应答从 EtherCAT 口出去，表现为"连不上"。
# EtherCAT 主站（SOEM）用 raw socket，只需要链路 UP，**不需要 IP、不能有默认路由**。
#
# 本脚本把指定接口标记为 NetworkManager "unmanaged"，令其只有链路、不配地址。
# 若本机不用 NetworkManager（用 netplan/networkd），请改用 docs/29 §7.2 的 netplan
# 写法（dhcp4: false + addresses: []）。
#
# 用法（需 root）：
#   sudo ./ethercat_port_linkonly.sh [接口名]     # 默认 enp2s0
#
# 验证：脚本最后会打印该接口的 ip -br addr，应无 IPv4 地址。
set -euo pipefail

IFACE="${1:-enp2s0}"
CONF="/etc/NetworkManager/conf.d/99-ethercat-unmanaged.conf"

if ! ip link show "$IFACE" >/dev/null 2>&1; then
    echo "error: interface '$IFACE' not found" >&2
    echo "check with: ip -br link" >&2
    exit 1
fi

# 1) 立即让 NetworkManager 放弃该接口（若 NM 在运行）
if command -v nmcli >/dev/null 2>&1 && \
   systemctl is-active NetworkManager >/dev/null 2>&1; then
    echo "[1] nmcli device set $IFACE managed no"
    nmcli device set "$IFACE" managed no || echo "    (nmcli set failed, conf 仍会生效)"
else
    echo "[1] NetworkManager 未运行/未安装，跳过 nmcli（请改用 netplan，见 docs/29 §7.2）"
fi

# 2) 写永久配置：重启 NetworkManager 后依然不受管
echo "[2] write $CONF"
printf '[keyfile]\nunmanaged-devices=interface-name:%s\n' "$IFACE" > "$CONF"
chmod 0644 "$CONF"
cat "$CONF"

if command -v systemctl >/dev/null 2>&1 && \
   systemctl is-active NetworkManager >/dev/null 2>&1; then
    echo "[3] systemctl reload NetworkManager"
    systemctl reload NetworkManager || true
fi

# 3) 验证：该接口不应有 IPv4 地址
echo "--- $IFACE address ---"
ip -br addr show "$IFACE"
echo "[OK] 现在插 EtherCAT 网线不会再影响 SSH（该口只有链路、无 IP/路由）。"
