# 用 nmcli 管理 RDK 的 WLAN（Wi-Fi）

本文记录在 RDK X5（TROS / Ubuntu，NetworkManager 管理网卡）上用 `nmcli`
控制 Wi-Fi 的常用操作：扫描、连接、断开、开关无线电、配置优先级与静态 IP。
面向“联调时临时把 RDK 从办公室 Wi-Fi 切到有线/断网”以及“重新连回 Wi-Fi”的场景。

> 工具名确认：RDK 上管理网络的命令就是 **`nmcli`**（NetworkManager CLI）。
> 同类命令还有 `nmtui`（交互式菜单，同一套 NetworkManager）、`iw`/`iwconfig`
> （底层，不适合建连接）。只有使用 ConnMan 的板子才用 `connmanctl`，RDK 不是。

## 1. 背景与约定

- RDK 默认带一块 Wi-Fi `wlan0`，此前 `192.168.108.178` 是它的 Wi-Fi 地址。
- 联调时为了减少干扰，常按文档 26/27 把 Wi-Fi 断开、走有线直连；本文补齐
  “怎么干净地断开、怎么重新连回、怎么改配置”的完整命令。
- NetworkManager 会接管网卡，**临时 `ip addr add` 会被回滚**；Wi-Fi 的连接、
  密码、优先级等持久配置都应走 `nmcli`（或 `nmtui`）。

## 2. 前置检查

```bash
# 1. NetworkManager 是否在跑（RDK 上是 systemd 服务）
systemctl status NetworkManager | head -5

# 2. 有哪些网络设备，找 wlan0 和它的 connection 名
nmcli device status
nmcli device show wlan0 | head -20

# 3. 当前生效的 Wi-Fi 连接（SSID / 频率 / 速率 / IP）
nmcli -f IN-USE,SSID,FREQ,RATE,SIGNAL,SECURITY device wifi list | grep -E '^\*|IN-USE'
nmcli -f GENERAL.CONNECTION,GENERAL.DEVICE,IP4.ADDRESS,IP4.GATEWAY device show wlan0
```

输出里 `IN-USE` 为 `*` 的那一行就是当前连上的网络；`GENERAL.CONNECTION` 是
connection 名（NetworkManager 的“连接配置”概念，一个 SSID 对应一个连接）。

## 3. 常用操作

### 3.1 扫描可用网络

```bash
nmcli radio wifi on            # 确保 Wi-Fi 无线电打开
nmcli device wifi rescan       # 强制重新扫描（部分系统需 sudo）
nmcli device wifi list         # 列出可见网络
nmcli -f SSID,SIGNAL,BARS,SECURITY device wifi list   # 只看关键列
```

- `SIGNAL` 是百分比强度，`BARS` 是图形信号格。
- 扫不到目标 SSID 时：先 `nmcli radio wifi on`，再 `rescan`，或走近路由器。

### 3.2 连接 WPA/WPA2 网络（一键式）

```bash
# 输入密码直连（会自动创建/复用名为 SSID 的连接并激活）
nmcli device wifi connect "MyWiFi" password "MyPassword"

# 不想把密码写进 shell 历史，用交互式提示（--ask）
nmcli --ask device wifi connect "MyWiFi"
```

成功后会自动生成一条 connection（名字通常就是 SSID），可用第 4 节命令管理。

### 3.3 连接 WPA3（SAE）或企业网

一键式只覆盖常见 WPA-PSK。特殊加密先建连接再激活：

```bash
# 建连接骨架（只建不连）
nmcli connection add type wifi con-name my-wpa3 ifname wlan0 ssid "MyWPA3"

# WPA3-SAE：key-mgmt 设为 sae
nmcli connection modify my-wpa3 wifi-sec.key-mgmt sae wifi-sec.psk "MyPassword"

nmcli connection up my-wpa3
```

> 企业网（802.1X，如 eduroam）参数多、易错，建议用 `nmtui` 或桌面 GUI 配置，
> `nmcli` 需逐项指定 `802-1x.*`，这里不展开。

### 3.4 连接隐藏 SSID 或指定频段

```bash
nmcli connection modify my-wpa3 802-11-wireless.hidden yes          # 隐藏网络
nmcli connection modify my-wpa3 802-11-wireless.band bg              # 限定 2.4 GHz（bg）/ 5 GHz（a）
nmcli connection up my-wpa3
```

### 3.5 断开 / 重连

```bash
nmcli device disconnect wlan0      # 只断开当前连接（推荐，配置保留，方便恢复）
nmcli connection down "MyWiFi"     # 等价：按连接名断开

nmcli device connect wlan0         # 重新激活默认/最近连接
nmcli connection up "MyWiFi"       # 按连接名重新激活
```

### 3.6 彻底开关 Wi-Fi 无线电

```bash
nmcli radio wifi off               # 关闭射频（省电/断网，且不再扫描）
nmcli radio wifi on                # 打开射频

nmcli radio                        # 查看各射频（wifi/wwan）开关状态
nmcli networking on|off            # 整体开关所有网络（更彻底，谨慎）
```

## 4. 连接（connection）管理

NetworkManager 的持久配置单位是 **connection**，每个连接有名字（`con-name`）、
SSID、密码、优先级、IP 方式等属性。

```bash
nmcli connection show               # 列出所有已保存连接
nmcli connection show --active      # 只看当前激活的
nmcli connection show "MyWiFi"      # 查看某连接完整属性（含密码掩码、IP 设置）

nmcli connection delete "MyWiFi"    # 删除连接（同时“忘记”该网络）
```

### 4.1 开机自动连接

```bash
nmcli connection modify "MyWiFi" connection.autoconnect yes     # 允许自动连
nmcli connection modify "MyWiFi" connection.autoconnect no      # 禁止自动连
```

### 4.2 多网络优先级

`connection.autoconnect-priority` 数值越大越优先（有负值时注意：默认 0）。
家有/公司多个 Wi-Fi 时，设置后开机自动选更优者：

```bash
nmcli connection modify "HomeWiFi"  connection.autoconnect-priority 100
nmcli connection modify "OfficeWiFi" connection.autoconnect-priority 50
```

### 4.3 给 Wi-Fi 配静态 IP

```bash
nmcli connection modify "MyWiFi" \
  ipv4.method manual \
  ipv4.addresses 192.168.108.178/24 \
  ipv4.gateway 192.168.108.1 \
  ipv4.dns "8.8.8.8 8.8.4.4"

nmcli connection up "MyWiFi"        # 重新激活生效

# 改回 DHCP
nmcli connection modify "MyWiFi" ipv4.method auto
nmcli connection up "MyWiFi"
```

> 联调提示：若这块 Wi-Fi 地址就是 SSH 管理地址，改 IP 或断开会导致 SSH 掉线，
> 请先确保有第二条可达路径（有线直连，见文档 26/27）。

## 5. 与联调流程的关系（RDK 场景）

```text
RDK wlan0  (Wi-Fi，办公室局域网)  ← 本文管理对象
RDK eth0   (有线管理口)           ← 网线直连 PC，见 26/27
RDK 另一网口 (EtherCAT raw socket) ← 接 HPM 网关，不配 IP，见 29
```

- 断开 Wi-Fi 前先确认 SSH 已走有线：`ssh root@192.168.108.178`（有线地址）可达再断。
- 断开 Wi-Fi **不影响** EtherCAT：那是另一块网口与 HPM 网关之间的数据面，
  与 WLAN 无关。
- `nmcli radio wifi off` 比 `nmcli device disconnect wlan0` 更彻底：前者连射频都关，
  后者只是断开当前连接、配置保留。

## 6. 常见问题排查

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| `nmcli device wifi list` 为空 | Wi-Fi 射频关闭，或驱动未加载 | `nmcli radio wifi on`；`ip link show wlan0` 看是否 UP |
| 扫不到目标 SSID | 信号弱 / 隐藏网络 / 5 GHz 频段问题 | 靠近路由；隐藏网络按 3.4 配 `hidden yes`；确认 `802-11-wireless.band` |
| `nmcli device wifi connect` 报 `No suitable security` | 网络加密与默认参数不匹配 | 按 3.3 显式建连接并指定 `wifi-sec.key-mgmt`（wpa-psk / sae / wpa-eap） |
| 密码含特殊字符连接失败 | shell 展开/转义问题 | 用引号包住：`password "P@ss w0rd!"`，或 `nmcli --ask` 交互输入 |
| 连接成功但无 IP | DHCP 失败或该连接被配成 manual | `nmcli device show wlan0` 看 IP；必要时 `ipv4.method auto` 后 `connection up` |
| 开机不自动连 | `connection.autoconnect no` 或优先级被别的压过 | 4.1 设 yes；4.2 调 `autoconnect-priority` |
| 断开 Wi-Fi 后 SSH 掉线 | SSH 之前走 Wi-Fi | 先 `ssh root@<有线地址>` 确认有线可达，再断开 |
| `nmcli` 报 `NetworkManager is not running` | NM 服务没起（极少见） | `sudo systemctl start NetworkManager` |

## 7. 要点速记

- 命令就是 `nmcli`；同族交互工具 `nmtui`，底层 `iw`，非 NM 板才用 `connmanctl`。
- 扫描：`nmcli device wifi list`；重扫：`nmcli device wifi rescan`。
- 直连：`nmcli device wifi connect "SSID" password "PASS"`；不落密码用 `--ask`。
- 断开：`nmcli device disconnect wlan0`；关射频：`nmcli radio wifi off`。
- 持久配置单位是 connection：`nmcli connection show/modify/up/down/delete`。
- 静态 IP：`connection modify <名> ipv4.method manual ipv4.addresses ...`。
- 断 Wi-Fi 前先确保 SSH 走有线；断 Wi-Fi 不影响 EtherCAT 数据面。

## 8. 相关文件

```text
docs/26-rdk-usb-ethernet-link.md   USB 网口在 PC 侧，网线直连 RDK（含断 WLAN）
docs/27-rdk-usb-eth-to-pc.md       USB 网口在 RDK 侧，网线直连 PC（含断 WLAN）
docs/29-rdkx5-ros2-deployment.md   RDK 部署电机控制环境（管理口/EtherCAT 口区分）
scripts/rdk_link_setup.sh          PC 侧 USB 网口直连辅助脚本（临时 ip 命令）
```
