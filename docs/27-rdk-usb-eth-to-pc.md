# RDK 侧插 USB 转网口，网线直连个人 PC

本文描述另一种接法：**把 USB 转网口插到 RDK 上**，然后用网线把 RDK 的这块
USB 网口直接连到个人 PC，实现 PC ⇄ RDK 通信。

> 与 `docs/26-rdk-usb-ethernet-link.md`（USB 网口在 PC 侧）相反。本文是
> "USB 网口在 RDK 侧"的方案，配置重点在 RDK。

## 1. 拓扑

```text
RDK (地瓜派)
  ├─ eth0    (板载有线口)  192.168.127.10/24   ← 原管理口（也可用作 EtherCAT）
  ├─ wlan0   (Wi-Fi)       （联调时可选断开）
  └─ 新 USB 转网口           192.168.108.178/24 ← 本文要配的，插上 USB 网卡后出现
              │  网线直连
              ▼
个人 PC (Ubuntu)
  └─ enp5s0  (板载网口)    192.168.108.156/24   ← 本机实测接口
```

目标网段：

- RDK USB 网口：`192.168.108.178/24`
- PC `enp5s0`：`192.168.108.156/24`（本机实测接口，使用静态地址）

PC 的 Wi-Fi 当前地址是 `192.168.110.6`，不能作为这条直连链路使用；
必须让有线接口 `enp5s0` 配置为 `192.168.108.156/24`，使路由明确走有线网卡。

## 2. 前置检查

### 2.1 确认 USB 网卡在 RDK 上被识别

把 USB 转网口插入 RDK，然后通过 RDK 串口（或现有网络）执行：

```bash
ip -br link
```

期望新增一块 `enx...`（或 `eth1`）接口，且 `carrier=1`（说明网线已插入）。

### 2.2 确认网线已连到 PC

PC 端：

```bash
ip -br link
ip -4 addr show enp5s0  # 期望 inet 192.168.108.156/24
```

PC 的 `enp5s0` 应显示 `UP`，并且网线已连接。

## 3. RDK 侧：给 USB 网口配 IP（关键步骤）

> ⚠️ 重要：**RDK 用 NetworkManager 管理网卡**（netplan renderer=NetworkManager，
> 见 `nmcli device status` 里的 `netplan-eth0` 连接）。手动 `ip addr add` 加的地址
> 会被 NM 在接管网卡时清掉（表现为"过一会儿 IP 就消失"）。
> **必须用 `nmcli` 建持久连接**，不要只用 `ip addr add`。

假设新接口叫 `enx<MAC>`（以 `ip -br link` 实际为准），例如 `enxec1ac300e94a`：

```bash
# 1. 确认接口名（找 carrier=1 的新 enx 接口）
ip -br link

# 2. 用 nmcli 创建固定静态连接（推荐，不会被 NM 清掉）
sudo nmcli connection add type ethernet ifname enxec1ac300e94a con-name rdk-usb-link \
  ipv4.method manual ipv4.addresses 192.168.108.178/24 \
  connection.autoconnect yes
sudo nmcli connection up rdk-usb-link

# 3. 确认（应稳定显示 inet 192.168.108.178/24，过一会再查一次仍存在）
ip -4 addr show enxec1ac300e94a
```

> 如果只想临时快速验证（不持久），可以用 `sudo ip addr add 192.168.108.178/24 dev enxec1ac300e94a`，
> 但要知道它可能被 NM 清掉；正式联调请用 nmcli 方案。

## 4. 首次部署与验证（按顺序执行）

首次部署建议严格按下面顺序操作，避免 PC 在有线连接尚未激活时把目标地址错误地交给 Wi-Fi：

### 4.1 RDK 侧先配置 USB 网口

在 RDK 上执行：

```bash
# 找到新插入的 USB 网口，接口名以实际输出为准
ip -br link
nmcli device status

# 假设接口名为 enxec1ac300e94a
sudo nmcli connection add type ethernet ifname enxec1ac300e94a \
  con-name rdk-usb-link \
  ipv4.method manual ipv4.addresses 192.168.108.178/24 \
  ipv4.never-default yes ipv6.method disabled \
  connection.autoconnect yes
sudo nmcli connection up rdk-usb-link

# 确认地址和链路
ip -4 addr show enxec1ac300e94a
ip -br link show enxec1ac300e94a
```

输出应包含 `192.168.108.178/24`，且接口应为 `UP`、网线链路为 `LOWER_UP`。

### 4.2 PC 侧配置有线接口

在 PC 上执行。本文实际使用的有线接口是 `enp5s0`：

```bash
# 确认接口名和物理链路
nmcli device status
ip -br link

# 首次部署：创建持久连接，不设置默认路由，Wi-Fi 继续负责上网
sudo nmcli connection add type ethernet ifname enp5s0 \
  con-name rdk-usb-link \
  ipv4.method manual ipv4.addresses 192.168.108.156/24 \
  ipv4.never-default yes ipv6.method disabled \
  connection.autoconnect yes
sudo nmcli connection up rdk-usb-link
```

如果提示连接 `rdk-usb-link` 已存在，不要重复创建，改为：

```bash
sudo nmcli connection modify rdk-usb-link \
  connection.interface-name enp5s0 \
  ipv4.method manual ipv4.addresses 192.168.108.156/24 \
  ipv4.never-default yes ipv6.method disabled \
  connection.autoconnect yes
sudo nmcli connection up rdk-usb-link
```

### 4.3 PC 侧验证路由、网络和 SSH

先确认 PC 有到 192.168.108.0/24 的地址和路由（见下方"PC 侧无路由"说明）：

```bash
ip -4 addr show enp5s0        # 应有 inet 192.168.108.x
ip route get 192.168.108.178  # 应显示 dev enp5s0

# ping RDK 的 USB 网口地址
ping -c 4 192.168.108.178

# SSH 登录（这就是 RDK 的 root）
ssh root@192.168.108.178
```

如果 ping 通、SSH 能进，说明 **RDK 侧 USB 网口直连 PC 成功**。

> 注意：`192.168.108.178` 与仓库部署脚本默认 RDK 地址一致，后续
> `deploy_host_to_rdk.ps1`、SOEM 主站等都能直接用这个地址。

### 4.4 本次实测调试记录

本次 PC 检查结果如下：

```text
nmcli device status
enp5s0  ethernet  disconnected  --
wlp4s0  wifi       connected      重庆未至科技有限公司

ip -br link
enp5s0  UP  ... <BROADCAST,MULTICAST,UP,LOWER_UP>
wlp4s0  UP  ... <BROADCAST,MULTICAST,UP,LOWER_UP>
```

`enp5s0` 的物理网线链路已经连通，但 NetworkManager 没有活动连接，因此目标路由错误地走了 Wi-Fi：

```text
ip route get 192.168.108.178
192.168.108.178 dev wlp4s0 src 192.168.110.6
```

此时直接 ping 会得到 `Destination Host Unreachable`。完成第 4.2 节配置后，路由必须变为：

```text
192.168.108.178 dev enp5s0 src 192.168.108.156
```

若仍未变为 `dev enp5s0`，执行下面的检查，不要继续排查 SSH 或 ROS：

```bash
nmcli device status
ip -br link
ip -4 addr show enp5s0
ip route get 192.168.108.178
```

> 注意：`enp5s0` 使用纯静态地址不会影响 Wi-Fi 上网；直连配置通过
> `ipv4.never-default yes` 不设置默认路由。

## 5. 持久化与双地址（PC 需要保留上网时）

PC 保留 Wi-Fi 上网，同时通过 `enp5s0` 直达 RDK；直连连接使用
`ipv4.never-default yes`，不会抢占 Wi-Fi 默认路由。后续重启或重新插线时，
NetworkManager 会自动恢复该连接：

```bash
# 查看直连连接
nmcli connection show

nmcli connection show rdk-usb-link
nmcli device show enp5s0
```

RDK 侧若想用 netplan（而非 nmcli）持久化：先看现有配置 `cat /etc/netplan/*.yaml`，
在 `ethernets` 下新增：

```yaml
network:
  version: 2
  ethernets:
    enxec1ac300e94a:
      addresses:
        - 192.168.108.178/24
```

应用 `sudo netplan apply`。

> 注意：USB 网口名称基于 MAC（`enx...`），换 USB 网卡/接口名会变，
> 持久化前先确认固定接口名。

## 6. 可选：断开 RDK 的 WLAN

联调时减少干扰可断开 Wi-Fi（在 RDK 上执行）：

```bash
nmcli device status
nmcli device disconnect wlan0     # 只断开连接，方便恢复
# 或
nmcli radio wifi off              # 彻底关闭 Wi-Fi 无线电
```

恢复：

```bash
nmcli device connect wlan0
# 或
nmcli radio wifi on
```

> 警告：如果 SSH 会话本身走 Wi-Fi，断开会掉线；请确认 SSH 走的是有线再断开。

## 7. 常见问题排查

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| RDK 上 `ip -br link` 没有新增 `enx*` | USB 网卡未识别 / 没插好 | 重新插拔 USB 网卡，或换一个 USB 口 |
| RDK 新接口 `carrier=0` | 网线没插或没插到 PC | 检查网线两端连接 |
| RDK 上 `ip addr add` 后 IP 过一会儿消失 | NetworkManager 接管网卡，清掉手动地址 | 用 `nmcli connection add` 建持久连接（第 3 节） |
| `ping 192.168.108.178` 报 `Network is unreachable` | PC 没有 192.168.108.0/24 的地址/路由 | 检查 `enp5s0`，按 4.2 配静态同网段地址 |
| `ip route get 192.168.108.178` 显示 `dev wlp4s0` | 有线连接未激活，目标流量走了 Wi-Fi | 激活 `rdk-usb-link`，确认路由显示 `dev enp5s0` |
| `ping 192.168.108.178` 超时（非 unreachable） | RDK USB 网口没配 IP，或网段不对 | RDK 上执行第 3 节步骤，确认 `ip -4 addr show` |
| PC `enp5s0` 没 IP 或不在 192.168.108.x | 有线连接未激活 | 用第 4.2 节的 `nmcli` 命令配置 `192.168.108.156/24` |
| SSH 断线（断开 WLAN 后） | SSH 之前走 Wi-Fi | 用有线地址 `ssh root@192.168.108.178`（RDK 的 USB 网口 IP）重新登录 |

## 8. 与"USB 网口在 PC 侧"方案的区别

| 项 | 26 号文档（USB 在 PC） | 本文（USB 在 RDK） |
| --- | --- | --- |
| USB 转网口插在哪 | 个人 PC | RDK |
| PC 连接方式 | PC USB 网口 ← 网线 → RDK eth0 | PC enp5s0 ← 网线 → RDK USB 网口 |
| 需要配置 IP 的一方 | PC 的 USB 网口 (`192.168.108.155/24`) | RDK 的 USB 网口 (`192.168.108.178/24`) |
| PC 侧额外配置 | 需要 nmcli 建持久连接 | `enp5s0` 用 nmcli 配 `192.168.108.156/24` |
| 风险点 | NetworkManager 回滚临时 IP | RDK netplan 接口名随 USB 变化 |

## 9. 后续联调

- 部署 host 代码：`scripts/deploy_host_to_rdk.ps1 -RdkIp 192.168.108.178 -UserName root`
- 跑 SOEM 主站（RDK 上）：
  `sudo ./ecat_motor_rt enp4s0 --arm-all --cycle-us 1000 --rx-timeout-us 500`
- 不要给 EtherCAT 口（若 eth0 用作 EtherCAT）配管理 IP，避免与 SOEM raw socket 冲突。

## 10. 要点速记

- RDK USB 网口：`192.168.108.178/24`；PC `enp5s0`：`192.168.108.156/24`。
- 接口名以 `ip -br link` 实测为准（新增 `enx*`）。
- 临时验证用 `ip addr add`；持久化用 netplan（RDK）或 nmcli（PC）。
- 网线两端 `carrier=1` 只代表链路 UP，**网段对齐才是连通前提**。
