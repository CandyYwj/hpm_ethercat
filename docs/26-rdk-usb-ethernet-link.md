# 通过 USB 转网口直连 RDK（地瓜派）

本文记录如何用一根网线（PC 侧通过 USB 转网口）与地瓜派 RDK 建立通信，
包含排查过程、最终方案、验证与持久化配置。适用于"想走网线连 RDK"的联调场景。

## 1. 背景与目标

- 目标设备：地瓜派 RDK（默认管理 IP `192.168.108.178`，用户 `root`，SSH 22 端口）。
- 前提：PC 上插了一个 USB 转网口（本机识别为 `enxec1ac300e94a`，RTL8153）。
- 目标：让 PC 通过这根 USB 网线直连 RDK，实现 ping / SSH / 部署 host 代码 / 跑 SOEM 主站。

## 2. 网络拓扑与关键事实（实测结论）

```text
PC (Ubuntu 24.04)
  ├─ eno1 (板载网口)         192.168.108.156/22   ← DHCP，默认路由走这里（办公室局域网）
  └─ enxec1ac300e94a (USB网口) 192.168.108.155/24  ← 直连 RDK（需要自己配置）
                │  网线
                ▼
RDK eth0 (唯一有线口)         192.168.108.178/24  ← 需手动加；默认是 192.168.127.10/24
RDK wlan0                     （可选，联调时可断开）
```

排查中确认的 4 个关键事实：

1. **RDK 的 `192.168.108.178` 之前是 Wi-Fi 地址**：通过办公室局域网到达 PC 的 `eno1`。
   断开 RDK 的 WLAN 后，这个地址消失，USB 网线直连链路尚未建立，所以 ping 不通。
2. **RDK 只有一块有线网口 `eth0`**，默认 `192.168.127.10/24`，网线插入时 `carrier=1`。
3. **PC 的 USB 网口链路已 UP（网线已插），但默认没有 IP**，且 NetworkManager 管理该网卡，
   临时 `ip addr add` 会被回滚，必须用 `nmcli` 建持久连接。
4. **网段必须对齐**：PC USB 网口 `192.168.108.155/24` 与 RDK eth0 `192.168.108.178/24`
   同网段后即可互通；`/24` 比 `eno1` 的 `/22` 前缀更长，内核自动优先走 USB 网线，
   **不需要**再手动加 `/32` 路由。

## 3. 完整步骤

### 3.1 PC 端：用 nmcli 给 USB 网口创建固定连接

> 不要用 `sudo ip addr add ...`，NetworkManager 会接管并回滚。

```bash
# 1. 确认 USB 网口名称和当前状态
ip -br link
nmcli device status | grep enxec1ac300e94a

# 2. 创建固定静态 IP 连接（/24 即可，前缀更长会自动优先走 USB 网线）
sudo nmcli connection add type ethernet ifname enxec1ac300e94a con-name usb-rdk \
  ipv4.method manual ipv4.addresses 192.168.108.155/24 \
  connection.autoconnect yes

# 3. 激活
sudo nmcli connection up usb-rdk

# 4. 确认
ip -4 addr show enxec1ac300e94a      # 期望：inet 192.168.108.155/24
ip route get 192.168.108.178         # 期望：dev enxec1ac300e94a
```

如果某一步想快速临时验证（重启后失效），可临时加：

```bash
sudo ip addr add 192.168.108.155/24 dev enxec1ac300e94a
ip route get 192.168.108.178
ping -c 4 192.168.108.178
```

### 3.2 RDK 端：给 eth0 加同网段 IP（可通过串口）

> 先分清两块口：**EtherCAT 口**（跑 SOEM 主站、接 HPM 网关，走原始帧）与**管理口**
> （接 PC 的 USB 网线）。网线插哪块就配哪块；**不要**给 EtherCAT 口配管理 IP。

在 RDK 上先确认：

```bash
ip -br addr
nmcli device status
# 看哪块网卡插了网线（carrier=1 的就是连 PC 网线的那块）
for i in $(ls /sys/class/net | grep -E '^(eth|en)'); do
  echo -n "$i: "; cat /sys/class/net/$i/carrier 2>/dev/null || echo down
done
```

给 eth0 临时加 IP（不删除原有 192.168.127.10，测试完可移除）：

```bash
sudo ip addr add 192.168.108.178/24 dev eth0
ip -4 addr show eth0
```

### 3.3 验证

```bash
# PC 端
ping -c 4 192.168.108.178
ssh root@192.168.108.178
```

## 4. 持久化配置（可选，测试通过后）

### 4.1 RDK 端：写入 netplan

RDK 用 netplan（`/etc/netplan/`）。先看现有配置：

```bash
cat /etc/netplan/*.yaml
```

在 eth0 的地址列表中加入 `192.168.108.178/24`，例如：

```yaml
network:
  version: 2
  ethernets:
    eth0:
      addresses:
        - 192.168.127.10/24
        - 192.168.108.178/24
```

应用：

```bash
sudo netplan apply
```

### 4.2 PC 端

上面的 `nmcli connection add` 已持久化（`autoconnect yes`），重启后自动生效。

## 5. 断开 RDK 的 WLAN（可选，联调时减少干扰）

在 RDK 上：

```bash
nmcli device status            # 找到 wlan0 的 connection 名
nmcli device disconnect wlan0  # 只断开当前连接（推荐，方便恢复）
# 或
nmcli radio wifi off           # 彻底关掉 Wi-Fi 无线电
```

恢复：

```bash
nmcli device connect wlan0
# 或
nmcli radio wifi on
```

> 警告：如果 SSH 会话本身是通过 Wi-Fi 建立的，断开 WLAN 会立刻掉线。
> 请先确认 SSH 走有线（`ssh root@192.168.108.178` 通过网线可达）再断开。
> 断开 WLAN 不影响 EtherCAT（那是 RDK 网口与 HPM 网关之间的数据面）。

## 6. 常见问题排查

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| `ip -4 addr show enxec1ac300e94a` 为空 | USB 网口没配 IP，或 NM 回滚了临时配置 | 用 `nmcli connection add` 建持久连接 |
| `ip route get 192.168.108.178` 显示 `dev eno1` | USB 网口无 IP/路由，包走板载网口 | 给 USB 网口配 `192.168.108.155/24`，确认 route 显示 `dev enxec1ac300e94a` |
| ping 源地址是 `192.168.108.156`（eno1 的 IP） | 源地址从别的网卡选，说明 USB 网口没配好 | 重新配好 USB 网口 IP 后重试 |
| RDK 上 `ip -br addr` 没有有线 IP 或网段不对 | RDK eth0 默认 `192.168.127.10/24`，与 PC 不同网段 | 给 eth0 加 `192.168.108.178/24` |
| ping 不通但两端 carrier=1 | 网段未对齐 或 中间隔了交换机但没配网关 | 确认两端同网段；RDK 若是交换机接入则直接走 `eno1` 即可 |
| 断 WLAN 后 ping 不通 | 178 原先是 Wi-Fi 地址，USB 直连链路未建立 | 按本文 3.1 + 3.2 配置后验证 |

## 7. 相关脚本与仓库约定

- 仓库提供辅助脚本 `scripts/rdk_link_setup.sh {up|down|status} [iface]`，可自动找 `enx*`
  网口、配 `192.168.108.155/24` 并加 `/32` 路由；但脚本用的是临时 `ip` 命令，
  **持久化请优先用 nmcli**（见 3.1）。脚本执行需 sudo。
- 部署 host 代码到 RDK：`scripts/deploy_host_to_rdk.ps1 -RdkIp 192.168.108.178 -UserName root`
- 在 RDK 上跑 SOEM 主站：
  `sudo ./ecat_motor_rt enp4s0 --arm-all --cycle-us 1000 --rx-timeout-us 500`
- RDK 默认管理 IP `192.168.108.178` 与仓库部署脚本默认值一致。

## 8. 要点速记

- PC USB 网口：`192.168.108.155/24`（nmcli 持久化）。
- RDK eth0：`192.168.108.178/24`（临时 `ip addr add` 或 netplan 持久化）。
- 同网段即互通，`/24` 优先于 `eno1` 的 `/22`，无需额外路由。
- 网线两端 `carrier=1` 只代表链路 UP，不代表网段可达；**网段对齐是前提**。
