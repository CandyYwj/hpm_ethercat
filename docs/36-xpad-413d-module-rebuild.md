<!-- Author: BearKing -->
# RDK 重编 xpad 内核模块：原生支持山寨 XBOX360 接收器（0x413d）

> 更新日期：2026-08-20
>
> 目标：给 RDK 6.1.83 内核的 `xpad` 驱动补上 `0x413d` 两条设备表，**out-of-tree 重编
> 单个 xpad.ko 并替换**，让内核**原生**认领 `0x413d:0x2104` 山寨接收器——
> 出 `/dev/input/js0`、原生热拔插，从而**彻底移除 xboxdrv + 看门狗 + udev 规则**这套补丁方案。
>
> 关联：[`docs/35-rdk-gamepad-event-driven-deployment.md`](35-rdk-gamepad-event-driven-deployment.md)

---

## 1. 为什么加两行就够

`xpad` 靠两张表匹配设备：

```c
/* ① 设备表：运行时查名字/类型（xpad_device[]） */
{ 0x413d, 0x2104, "Black Shark Green Ghost Gamepad", 0, XTYPE_XBOX360 },

/* ② 接口表：USB 核心按 厂商+class 0xff/subclass 93/protocol 1 匹配（xpad_table[]） */
XPAD_XBOX360_VENDOR(0x413d),   /* = { class 0xff, subclass 93, protocol 1|129 } */
```

你的接收器接口 0 是 `ff5d01`（class `0xff` / subclass `0x5d`(93) / protocol `0x01`），
正好命中 ②。这两行就是主机 7.0 内核里已有的内容（6.2+ 才加入，6.1 LTS 没回灌）。
补上后内核行为与主机**完全一致**。

---

## 2. 前置检查（在 RDK 上）

```bash
uname -r                                    # 6.1.83
ls -d /lib/modules/$(uname -r)/build        # 内核头必须存在（关键前提）
apt-cache search linux-headers-$(uname -r)  # 若上一条失败，用这个装

# 编译工具
command -v make gcc || sudo apt install -y build-essential
command -v curl || sudo apt install -y curl
```

> ⚠️ 如果 `/lib/modules/$(uname -r)/build` 不存在且装不了头文件，此路不通——
> 只能退回 xboxdrv 或换受支持的手柄。

---

## 3. 方案一：一键脚本（推荐）

仓库已提供 `scripts/build_xpad_413d.sh`（补丁逻辑已在真实 v6.1 源码验证过）：

```bash
# 把脚本传到 RDK 后以 root 运行：
sudo bash scripts/build_xpad_413d.sh
```

脚本自动完成：取源码（本地优先，否则从 kernel.org 下 v6.1.83/v6.1）→ 打补丁 →
编 `xpad.ko` → 停掉 xboxdrv 服务/进程、删 udev 规则 → 安装到
`/lib/modules/$(uname -r)/extra/`（优先级高于系统模块）→ `depmod` + `modprobe`。

跑完后**重新插拔接收器**，验证：

```bash
ls -l /dev/input/js0
sudo apt install -y joystick && jstest /dev/input/js0
```

---

## 4. 方案二：手工逐步（原理与兜底）

### 4.1 获取源码

```bash
mkdir -p ~/xpad-413d && cd ~/xpad-413d

# 优先用 RDK 头里自带的（若有源码）
find /usr/src /lib/modules/$(uname -r) -name xpad.c 2>/dev/null

# 否则下载 v6.1.83（RDK 无外网时，在 PC 下载后 scp 过来）
curl -fsSL -o xpad.c \
  "https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/plain/drivers/input/joystick/xpad.c?h=v6.1.83"
# 或退回 v6.1：
# curl -fsSL -o xpad.c "...?h=v6.1"
```

### 4.2 打补丁（两处）

用编辑器打开 `xpad.c`，做两处插入：

**① 设备表**：找到 `"Chinese-made Xbox Controller"` 那一行，在它**上一行**插入：

```c
	{ 0x413d, 0x2104, "Black Shark Green Ghost Gamepad", 0, XTYPE_XBOX360 },
```

**② 接口表**：找到最后一行的 `XPAD_XBOX360_VENDOR(...)`，在它**下一行**、`{ }` 结束符之前插入：

```c
	XPAD_XBOX360_VENDOR(0x413d),	/* Black Shark Green Ghost Controller */
```

> 等价的一行 sed（GNU sed）：
> ```bash
> sed -i '/0xffff, 0xffff, "Chinese-made Xbox Controller"/i\\t{ 0x413d, 0x2104, "Black Shark Green Ghost Gamepad", 0, XTYPE_XBOX360 },' xpad.c
> sed -i '0,/^\t{ }$/{/^\t{ }$/i\\tXPAD_XBOX360_VENDOR(0x413d),\t/* Black Shark Green Ghost Controller */}' xpad.c
> ```

验证：`grep -n 413d xpad.c` 应看到两行。

### 4.3 编译

```bash
cat > Makefile <<'EOF'
obj-m += xpad.o
KDIR := /lib/modules/$(shell uname -r)/build
all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules
clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
EOF

make
ls -l xpad.ko    # 应生成模块
```

> 内核头从 `KCONFIG` 提供 `CONFIG_JOYSTICK_XPAD[_FF|_LEDS]` 与 vermagic，out-of-tree 编译
> 自动对齐，`ff_memless`/`led_class` 符号在加载时由已加载模块解析。

### 4.4 停掉 xboxdrv（关键：内核才能拿到设备）

```bash
sudo systemctl disable --now rdk_xboxdrv.service 2>/dev/null || true
sudo pkill -x xboxdrv 2>/dev/null || true
sudo rm -f /etc/udev/rules.d/90-rdk-xbox360.rules
sudo udevadm control --reload
```

### 4.5 安装模块（extra/ 优先于 kernel/）

```bash
# 备份原模块
sudo mkdir -p ~/xpad-413d/stock
sudo cp /lib/modules/$(uname -r)/kernel/drivers/input/joystick/xpad.ko* ~/xpad-413d/stock/ 2>/dev/null || true

# 卸载旧的、装新的
sudo rmmod xpad 2>/dev/null || true
sudo mkdir -p /lib/modules/$(uname -r)/extra
sudo install -m 644 xpad.ko /lib/modules/$(uname -r)/extra/xpad.ko
sudo depmod -a
sudo modprobe xpad
lsmod | grep xpad
```

> `extra/` 目录在 depmod 里优先级高于 `kernel/`，所以 `modprobe xpad` 会加载我们的新模块。

### 4.6 验证

```bash
# 重新插拔接收器，然后：
ls -l /dev/input/js0          # 应出现（原生）
dmesg | tail -20              # 应看到 xpad 认领日志
sudo apt install -y joystick && jstest /dev/input/js0
```

热拔插测试：拔掉 → `js0` 消失、手柄节点 `gamepad disconnected`；插回 → `js0` 自动回来、`connected`。全程无 xboxdrv。

### 4.7 回滚

```bash
sudo rm -f /lib/modules/$(uname -r)/extra/xpad.ko
sudo depmod -a
sudo rmmod xpad && sudo modprobe xpad    # 或重启
# 若想恢复 xboxdrv 方案：重新启用 rdk_xboxdrv.service + udev 规则
```

---

## 5. 常见问题

| 现象 | 处理 |
| --- | --- |
| `make` 报 `vermagic` 不匹配 | 头文件与运行内核版本不一致；确认 `/lib/modules/$(uname -r)/build` |
| `make` 报缺头文件/符号 | 检查是否装了 `linux-headers-$(uname -r)` 和 `build-essential` |
| `modprobe xpad` 后仍无 `js0` | `dmesg \| tail`；确认接收器在 `0x2104` 模式（不是 `0x2106`），重新插拔 |
| 系统里 `xpad` 被 built-in（非模块） | 无法替换模块，退回 xboxdrv 或换硬件 |
| 内核升级后失效 | 用 `dkms` 把 `scripts/build_xpad_413d.sh` 的产物包成 DKMS 源，或升级后再跑一次脚本 |

---

## 6. 结论

- 优雅度：✅ 内核原生认领，热拔插零维护，**xboxdrv/看门狗/udev 全部下线**。
- 前提：RDK 有当前内核头文件（`/lib/modules/$(uname -r)/build`）。
- 与主机体验一致：`/dev/input/js0`、`jstest`、拔插自动恢复。
