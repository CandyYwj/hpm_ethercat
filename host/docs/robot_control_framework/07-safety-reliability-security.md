# 安全、可靠性与安全防护

## 适用边界

本文是软件架构和验证要求，不是 ISO 10218、ISO 13849、IEC 61508 或其他法规认证文件。急停、STO、限位、过流/过温、制动和电源切断必须由已评估的硬件/固件安全链路提供；`robot_rt` 负责配合、监测和进入定义的安全状态。

## 安全状态机

```text
BOOT -> SAFE -> STANDBY -> ARMED -> ACTIVE
          |       |         |       |
          +-------+---------+-------+--> DEGRADED
                                           |
                                     SAFE_STOP/FAULT
                                           |
                                  RESET (显式授权) -> SAFE
```

- `SAFE`：输出被禁能或置安全值，允许只读探测。
- `STANDBY`：设备已识别、参数/限位校验通过，但尚未允许扭矩。
- `ARMED`：安全许可、power domain、lease 和硬件 ready 均成立；等待有效周期命令。
- `ACTIVE`：按 profile 控制；每周期重新检查命令有效期、反馈新鲜度、限幅、斜率和故障。
- `DEGRADED`：只保留明确允许的低风险能力（例如降低速度/保持姿态）；禁止隐式继续全功率。
- `SAFE_STOP/FAULT`：撤销所有运动 lease，执行 profile fail-action；不会因 DDS 重连自动恢复。

节点 lifecycle（unconfigured/inactive/active/finalized）只表示资源状态，不能单独代表 `ARMED/ACTIVE`。两套状态必须在诊断中同时显示。

## 独立保护与看门狗

至少建立三层 watchdog：

1. **驱动器/MCU watchdog**：失去周期帧、通信或心跳时禁能/切 STO；不依赖 Linux 或 ROS。
2. **`robot_rt` watchdog**：监视周期 deadline、命令 age、反馈 age、总线状态和安全输入；连续违约触发安全动作。
3. **systemd/process watchdog**：监视进程存活和启动失败；进程退出后硬件 watchdog 仍负责切断输出。

命令 timeout 是通信保护，不是急停替代品。默认运动命令带 `valid_until`，过期后下一周期即清零；保持最后值必须是设备 profile 明确声明且仍受总时长/斜率限制。

## 故障分类与恢复

| 故障 | 默认动作 | 是否自动恢复 |
| --- | --- | --- |
| 参数/协议版本不匹配 | `QUARANTINED`，只读 | 否，需修正 profile |
| CRC/格式错误偶发 | 丢弃帧、计数 | 可继续，超过阈值降级 |
| 反馈 stale/非法 | 停止依赖该反馈的控制器 | 否，待反馈恢复后显式 reset |
| CAN error-passive/bus-off | 停止该 bus 轴、保存 trace | 恢复总线后仍需人工 re-arm |
| EtherCAT WKC/状态异常 | 受影响从站安全停机 | 重新扫描/确认后恢复 |
| RT deadline miss | 撤销 lease，按连续次数停机 | 只允许显式恢复 |
| ROS/DDS/IPC 断开 | 本地 watchdog 进入 SAFE | 不能自动重新上扭矩 |
| 温度/电压超限 | 按设备 profile 降额或停机 | 需冷却/电源确认 |

每种故障必须定义检测阈值、动作时限、告警 ID、原始证据、复位条件和回滚路径。重试只用于幂等、短暂的管理操作；非幂等参数写入超时不得盲重发。

## 输入校验与权限

- ROS topic/service/action、CLI、配置和设备探测都视为不可信输入；边界校验 finite、范围、单位、序号、source lease、大小和速率。
- `command_mux` 是唯一运动命令入口，按 source、优先级、租约、deadman 和模式做授权；应用节点不直接访问 raw socket、`/dev` 或 EtherCAT 网卡。
- `arm`、清错、模式切换、标定、参数持久化和固件升级需要显式身份、设备 allowlist、当前状态和审计记录。操作员确认不等同于硬件安全许可。
- RT 层再次校验限幅、斜率、状态和有效期；不能信任 ROS/URDF 已经校验过。

## 主机与网络安全

- 生产 ROS 2 使用 SROS2 governance/permissions 和最小 topic/service allowlist；证书、私钥不进仓库，轮换和失效行为要演练。
- 控制网、管理网、开发/录包网分区；EtherCAT 专用接口不配置管理 IP；跨 VLAN 用静态 peer/discovery server 或受控 router，防止公网 discovery。
- `robot_rt` 只授予必要的 `CAP_NET_RAW`、`CAP_SYS_NICE`、`CAP_IPC_LOCK`；systemd 设置 `NoNewPrivileges=true`、设备白名单、只读系统目录和受限写目录。
- 插件、配置、固件和生成物校验签名/hash；禁止从 ROS 参数加载任意路径 `.so` 或执行脚本。容器使用只读 rootfs、固定 image digest、host network 仅在确有需要时开启。
- 录包和 trace 可能含设备序列号、位置或环境数据，按保留期限、访问控制和脱敏规则管理。

## 威胁模型（首版）

| 资产 | 威胁 | 缓解 | 证据 |
| --- | --- | --- | --- |
| 执行器扭矩/位置 | 未授权命令、重放、注入 | SROS2、lease/sequence/expiry、RT限幅、硬件 watchdog | 越权/重放测试 |
| 总线配置 | 错误 bitrate/CAN ID/PDO | 签名 profile、离线 lint、allowlist、停机切换 | 配置拒绝与回滚 |
| 固件/插件 | 篡改或恶意代码 | 签名、hash pin、只在启动加载、最小权限 | 供应链/签名测试 |
| 诊断数据 | 泄露或伪造 | ACL、脱敏、事件序号和来源 | 访问审计 |
| 可用性 | discovery 洪泛、队列耗尽 | 网络隔离、有界队列、速率限制、背压策略 | 压力/DoS测试 |

## 可靠性与恢复

- 所有服务具备超时、取消、幂等键和明确错误码；启动失败快速报错，不能静默使用默认总线地址。
- 配置/插件采用 A/B slot；激活前停机并保存 last-known-good，健康门失败自动回退，之后仍需人工 re-arm。
- 断电恢复以 SAFE 启动，重新探测、校验身份/版本/校准和安全输入后才允许 STANDBY；不能因为上次是 ACTIVE 就自动上扭矩。
- 对系统崩溃、网络断开、设备重启、总线恢复、磁盘满、时钟跳变和升级中断做定期恢复演练。
