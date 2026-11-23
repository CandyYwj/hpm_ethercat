<!-- Author: BearKing -->
# systemd 详解（RDK 开机自启服务）

> 目标读者：需要理解并编写 RDK（或任何 systemd Linux）开机自启服务的人。
> 全文以本仓库两个真实服务为例子：
> - `scripts/rdk_rt_setup.service` —— NIC 中断绑核（一次性任务）
> - `scripts/hpm_gateway_node.service` —— 开机自启 ROS2 EtherCAT 节点（常驻进程）
>
> 更新日期：2026-08-20

## 1. systemd 是什么

systemd 是 Linux 上 PID=1 的 init 系统，开机后第一个进程，负责启动/停止/跟踪
系统里所有服务与资源。它把"一个被管理的对象"称为 **unit**，常见的 unit 类型：

| 类型 | 作用 |
| --- | --- |
| `service` | 守护进程/服务（本文主角） |
| `target` | 一组 unit 的集合点，如 `multi-user.target`（多用户运行级别） |
| `socket` | 套接字激活 |
| `timer` | 定时任务（cron 的替代） |
| `path` | 文件/目录变化触发 |
| `mount` / `swap` / `device` | 文件系统与设备 |

开机顺序本质是：`default.target` → 依赖的 `target` → 依赖的 `service`，由
`[Install]` 里的 `WantedBy` 决定挂在哪个 target 下。

## 2. unit 文件位置与优先级

| 路径 | 说明 |
| --- | --- |
| `/etc/systemd/system/` | 管理员放置/覆盖（**优先**），本项目服务装这里 |
| `/usr/lib/systemd/system/` | 发行版自带的 unit（ROS/TROS 的包也装这里） |
| `/lib/systemd/system/` | 旧路径，通常是 `/usr/lib` 的软链 |

同名 unit 以 `/etc` 优先。想不改原文件做微调，用 **drop-in**：
`/etc/systemd/system/xxx.service.d/override.conf`，比覆盖整个文件更安全。

## 3. 单元文件的三大段

一个 `.service` 通常有三段：`[Unit]`、`[Service]`、`[Install]`。

### 3.1 `[Unit]` —— 元信息与依赖顺序

```ini
[Unit]
Description=...
After=...        # 只决定"先后顺序"，不创建依赖
Before=...
Requires=...     # 硬依赖：它没起来，本 unit 也起不来/失败
Wants=...        # 软依赖：尽量拉起它，但它失败不影响本 unit
PartOf=...       # 从属：主 unit 停，它也停
```

**核心概念：`After/Before` 只管顺序，`Requires/Wants` 只管依赖**。两者常组合用：

```ini
After=rdk_rt_setup.service
Wants=rdk_rt_setup.service
```

意思是"等它启动完成再启动我；若它没装/失败，我不受影响"。

### 3.2 `[Service]` —— 怎么跑、怎么重启、怎么停

```ini
[Service]
Type=...           # 启动类型（见 3.3）
User=...           # 以哪个用户跑（root / 普通用户）
Environment=...    # 注入环境变量
ExecStart=...      # 主命令（必须绝对路径）
ExecStartPre=...   # 启动前执行（可多条）
ExecStartPost=...  # 启动后执行
ExecReload=...     # systemctl reload 时执行
ExecStop=...       # 停止时额外执行
Restart=...        # 退出后是否自动重启（见 5）
RestartSec=...     # 重启间隔
TimeoutStartSec=...  # 启动超时
TimeoutStopSec=...   # 停止超时
KillMode=...       # 停止时杀哪些进程（见 9）
KillSignal=...     # 先发什么信号
WorkingDirectory=... # 工作目录
StandardOutput=... / StandardError=...  # 输出去向（journal）
SyslogIdentifier=...  # 日志里显示的标识
```

> ⚠️ **ExecStart 不走 shell**：不能用管道 `|`、重定向 `>`、`&&`、`$VAR` 展开。
> 需要这些就去写个 wrapper 脚本（`hpm_gateway_node.sh` 就是这么做的），
> 或者用 `ExecStart=/bin/bash -lc '...'`。

### 3.3 `Type` 详解（最常选错的一项）

| Type | 语义 | 何时用 |
| --- | --- | --- |
| `simple` | ExecStart 的进程**就是主进程**，fork 成功即视为"已启动" | 前台常驻进程（ROS 节点、web 服务） |
| `exec` | 同 simple，但要等 `execve` 成功才算启动 | 更严格地检测可执行文件存在 |
| `oneshot` | 执行完即退出 | 一次性脚本（绑核、初始化） |
| `forking` | 主进程 fork 出子进程后退出，需 `PIDFile=` | 传统 daemon（自行 daemonize 的老程序） |
| `notify` | 进程用 `sd_notify` 通知"我准备好了" | 需要就绪确认的服务 |
| `idle` | 等其它任务跑完再启动 | 输出不想被冲掉的脚本 |

本项目两个例子正好覆盖两种典型：

```ini
# rdk_rt_setup.service —— 一次性脚本，执行完就结束
Type=oneshot
RemainAfterExit=yes        # 退出后仍标记为 active（方便 status 显示绿色）
```

```ini
# hpm_gateway_node.service —— 常驻 ROS 节点
Type=simple                # ros2 launch 在前台跑，就是主进程
Restart=on-failure
```

### 3.4 `[Install]` —— 挂到哪个 target

```ini
[Install]
WantedBy=multi-user.target
```

`systemctl enable` 其实就是在
`/etc/systemd/system/multi-user.target.wants/` 下建一个指向本 unit 的符号链接，
让系统进入 `multi-user.target`（多用户模式，即正常开机）时把它拉起来。

## 4. 生命周期与常用命令

```bash
systemctl daemon-reload                 # ⚠️ 改过 .service 文件后必须先执行
systemctl enable --now xxx.service      # 开机自启 + 立即启动
systemctl disable --now xxx.service     # 取消自启 + 停止
systemctl start / stop / restart xxx.service
systemctl status xxx.service            # 状态 + 最近日志 + 主进程信息
systemctl is-enabled xxx.service        # 查是否开机自启
systemctl list-units --type=service     # 列出服务
systemd-analyze verify /etc/systemd/system/xxx.service  # 语法/依赖检查
```

`enable`（改 `[Install]`、建软链）和 `start`（真正跑进程）是**两件事**，
`--now` 只是把两者合成一步。

## 5. 重启策略（`Restart`）

| 值 | 行为 |
| --- | --- |
| `no` | 永不自动重启（默认） |
| `on-success` | 仅正常退出（exit 0）时重启 |
| `on-failure` | **非正常退出**（非零退出码、信号杀死、超时、启动失败）时重启 |
| `on-abnormal` | 被信号/超时/看门狗杀掉时重启 |
| `on-abort` | 被未捕获的信号杀时重启 |
| `always` | 无论如何都重启 |

配套参数：

```ini
Restart=on-failure
RestartSec=5
# 防"重启风暴"：默认 5 次/10s 内失败就放弃，改为 0 表示不限
StartLimitIntervalSec=0
```

> 本项目 `hpm_gateway_node.service` 用 `Restart=on-failure` + `RestartSec=5`：
> 节点崩了 5 秒后自动拉起；但若工作区没编译、EtherCAT 口绑错导致**反复失败**，
> 会一直重试（没有加 StartLimit），日志会刷屏——这是刻意的（等硬件/依赖就绪），
> 生产环境通常要加 `StartLimitIntervalSec` 限制。

## 6. 依赖与顺序（细讲 `After` / `Wants` / `Requires`）

- `After=bar.service`：只保证 bar **先启动**，bar 失败我照样启动（顺序）。
- `Requires=bar.service`：bar 失败/停止则我也失败/停止（依赖）。
- `Wants=bar.service`：尽量拉起 bar，但 bar 挂了我不受影响（软依赖）。
- 组合 `After` + `Wants` 是最常见的"先等它、但不强绑"写法。

本项目示例：

```ini
# hpm_gateway_node.service
After=network-online.target
Wants=network-online.target     # 等网络就绪（EtherCAT raw socket 需要网卡 up）
After=rdk_rt_setup.service
Wants=rdk_rt_setup.service      # 等中断先绑核，保证实时性
```

## 7. 环境变量

```ini
Environment=ROS_DOMAIN_ID=42          # 静态注入
EnvironmentFile=/etc/xxx.env          # 从文件读（每行 KEY=value）
```

> 本项目把 `ROS_DOMAIN_ID=42` 写在服务里，`hpm_gateway_node.sh` 里也有一份兜底，
> 双保险。注意 `ExecStart` 里 `$VAR` 不会被 shell 展开，所以环境变量只能通过
> `Environment=` 或 wrapper 脚本内部 `export` 传入。

## 8. 日志（journald）

默认标准输出/错误都进 journal（`StandardOutput=journal`）。本项目也显式写了：

```ini
StandardOutput=journal
StandardError=journal
SyslogIdentifier=hpm_gateway_node
```

查看：

```bash
journalctl -u hpm_gateway_node.service          # 全部
journalctl -u hpm_gateway_node.service -f       # 跟随（类似 tail -f）
journalctl -u hpm_gateway_node.service -n 100   # 最近 100 行
journalctl -u hpm_gateway_node.service --since "10 min ago"
journalctl -xe                                  # 系统级最近错误 + 提示
```

> 服务"看起来没起来"时，第一个动作就是 `journalctl -u xxx -f` 看它到底打印了啥。

## 9. 停止与杀进程（`KillMode` / `KillSignal`）

| KillMode | 行为 |
| --- | --- |
| `control-group`（默认） | 杀整个 cgroup 里的**所有**进程（含子进程） |
| `process` | 只杀主进程，子进程不管 |
| `mixed` | 主进程发 SIGTERM，其余发 SIGKILL |
| `none` | 不杀（配合 ExecStop 手动清理） |

停止流程：先发 `KillSignal`（默认 SIGTERM）→ 等 `TimeoutStopSec`（默认约 90s）
→ 超时再 SIGKILL。

```ini
KillMode=control-group
KillSignal=SIGINT        # ros2 launch / ROS 节点对 SIGINT 做优雅停机
TimeoutStopSec=20        # 20s 内没退干净就 SIGKILL
```

> 用 SIGINT 而不是 SIGTERM 是刻意选择：ROS2 的 launch/节点把 SIGINT 当优雅退出，
> 这样 `systemctl stop` 能干净释放 EtherCAT raw socket 和 DDS。

## 10. 本仓库两个服务的逐行讲解

### 10.1 `scripts/rdk_rt_setup.service`（一次性绑核）

```ini
[Unit]
Description=Pin EtherCAT NIC IRQ to a dedicated CPU core (real-time setup)
After=network.target
Wants=network.target

[Service]
Type=oneshot                 # 一次性脚本
RemainAfterExit=yes          # 跑完仍视为 active，方便 systemctl status 看绿
Environment=CORE=3           # 脚本里可读 $CORE
Environment=IFACE=eth0
ExecStart=/usr/local/bin/rdk_rt_setup.sh ${CORE} ${IFACE}   # 参数来自 Environment

[Install]
WantedBy=multi-user.target   # 开机挂到多用户 target
```

### 10.2 `scripts/hpm_gateway_node.service`（常驻 ROS 节点）

```ini
[Unit]
Description=HPM Gateway EtherCAT control node (ROS2 Humble / TROS)
After=network-online.target
Wants=network-online.target
After=rdk_rt_setup.service
Wants=rdk_rt_setup.service

[Service]
Type=simple
User=root                    # raw socket 需要 root
Environment=ROS_DOMAIN_ID=42
ExecStart=/usr/local/bin/hpm_gateway_node.sh
Restart=on-failure
RestartSec=5
TimeoutStartSec=30
TimeoutStopSec=20
KillMode=control-group
KillSignal=SIGINT
StandardOutput=journal
StandardError=journal
SyslogIdentifier=hpm_gateway_node

[Install]
WantedBy=multi-user.target
```

## 11. 从零写一个服务（通用步骤）

1. **写启动脚本**（可选但推荐）：把 source 环境、`export`、`exec` 主程序放进
   一个 `.sh`，避免 systemd 的 ExecStart 不支持 shell 语法的坑。
2. **写 unit 文件**：按上面三段写，Type 选对（常驻=simple，一次性=oneshot）。
3. **安装**：
   ```bash
   install -m 0755 myapp.sh /usr/local/bin/myapp.sh
   install -m 0644 myapp.service /etc/systemd/system/
   systemctl daemon-reload
   ```
4. **试跑**：`systemctl start myapp.service` → `systemctl status` →
   `journalctl -u myapp.service -f`。
5. **开机自启**：`systemctl enable --now myapp.service`。
6. **验证重启后**：`systemctl reboot` 或 `systemctl isolate multi-user.target`
   后确认服务自动起来。

## 12. 排障速查

| 现象 | 排查 |
| --- | --- |
| 服务没启动 | `systemctl status xxx` + `journalctl -u xxx -e`；先 `daemon-reload` |
| 反复重启（风暴） | 看日志确认根因；临时 `systemctl stop`；生产加 `StartLimitIntervalSec` |
| `Exec format error` / 起不来 | 脚本缺 `#!/usr/bin/env bash` 或没 `chmod +x` |
| 环境变量/路径不对 | 记住 ExecStart 不走 shell；用 `Environment=` 或 wrapper 脚本 |
| 权限不足 | raw socket / 写 /proc 需要 `User=root`；普通用户加 group 权限 |
| 网络服务没就绪 | 用 `After=network-online.target`（配合 `systemd-networkd-wait-online` 或 NetworkManager） |
| 看不到输出 | 确认 `StandardOutput=journal`，用 `journalctl -u xxx -f` 而不是看终端 |
| 停不掉、卡住 | `TimeoutStopSec` 到点会 SIGKILL；确认 `KillMode` 覆盖了子进程 |

## 13. 相关文件

```text
scripts/rdk_rt_setup.sh / rdk_rt_setup.service        NIC 中断绑核（oneshot 示例）
scripts/hpm_gateway_node.sh / hpm_gateway_node.service  开机自启 ROS 节点（simple 示例）
docs/29-rdkx5-ros2-deployment.md                       8.3 开机自启 + 7 实时配置
scripts/README.md                                      RDK 自启服务安装说明
```
