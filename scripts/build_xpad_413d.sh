#!/usr/bin/env bash
# Author: BearKing
#
# build_xpad_413d.sh - Build & install a patched xpad kernel module for the
# RDK (kernel 6.1.83) so it natively recognizes the 2.4G XBOX360 clone
# receiver (vendor 0x413d / product 0x2104) as /dev/input/js0.
#
# Why: the stock 6.1 xpad driver lacks the 0x413d entry (added upstream in
# 6.2+ and not backported to the LTS branch). This adds the two upstream
# lines and builds ONLY the xpad module out-of-tree against the running
# kernel headers, then installs it into /lib/modules/$(uname -r)/extra
# (which takes precedence over the stock module).
#
# Run on the RDK as root:
#   sudo bash scripts/build_xpad_413d.sh
#
# Requirements:
#   - kernel headers: /lib/modules/$(uname -r)/build must exist
#   - build-essential (gcc, make)
#   - optional: curl / internet to fetch xpad.c (or drop xpad.c into $WORK)
#
# After success: unplug/replug the receiver -> js0 should appear, and the
# xboxdrv hack (service + udev rule) is disabled automatically.
set -euo pipefail

KV="$(uname -r)"
KSRC="/lib/modules/$KV/build"
WORK="${XPAD_WORK:-$HOME/xpad-413d}"
KERNEL_DIR="/lib/modules/$KV/kernel/drivers/input/joystick"
EXTRA_DIR="/lib/modules/$KV/extra"

say() { printf '\n==> %s\n' "$*"; }

# ---- 0. prerequisites -------------------------------------------------
if [ ! -d "$KSRC" ]; then
  echo "error: kernel headers not found at $KSRC"
  echo "       install them, e.g.:  sudo apt install -y linux-headers-$(uname -r)"
  exit 1
fi
for tool in make gcc awk sed; do
  command -v "$tool" >/dev/null 2>&1 || { echo "error: missing tool: $tool"; exit 1; }
done

mkdir -p "$WORK"
cd "$WORK"

# ---- 1. obtain xpad.c -------------------------------------------------
if [ ! -f xpad.c ]; then
  local_found="$(find /usr/src "$KSRC" -name xpad.c 2>/dev/null | head -n 1 || true)"
  if [ -n "$local_found" ]; then
    say "using local xpad.c: $local_found"
    cp "$local_found" xpad.c
  elif command -v curl >/dev/null 2>&1; then
    say "downloading xpad.c (kernel v6.1.83) from kernel.org..."
    if ! curl -fsSL --connect-timeout 20 -o xpad.c \
        "https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/plain/drivers/input/joystick/xpad.c?h=v6.1.83"; then
      say "v6.1.83 tag unavailable, trying v6.1..."
      curl -fsSL --connect-timeout 20 -o xpad.c \
        "https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/plain/drivers/input/joystick/xpad.c?h=v6.1" \
        || { echo "error: failed to download xpad.c (no internet?)."; exit 1; }
    fi
  else
    echo "error: no local xpad.c and curl missing."
    echo "       Download it manually into $WORK/xpad.c, then re-run."
    exit 1
  fi
fi

# sanity: must look like the kernel xpad driver source
grep -q 'MODULE_DESCRIPTION' xpad.c && grep -q 'XPAD_XBOX360_VENDOR' xpad.c \
  || { echo "error: $WORK/xpad.c does not look like xpad.c"; exit 1; }

# ---- 2. patch (idempotent) -------------------------------------------
if grep -q '0x413d' xpad.c; then
  say "xpad.c already contains 0x413d, skipping patch"
else
  say "patching xpad.c (adding Black Shark Green Ghost 0x413d entries)..."
  awk '
    { lines[NR] = $0 }
    /^[[:space:]]*XPAD_[A-Z0-9_]*_VENDOR\(/ { last_vendor = NR }
    /^[[:space:]]*\{ 0xffff, 0xffff, "Chinese-made Xbox Controller"/ { dev_anchor = NR }
    END {
      for (i = 1; i <= NR; i++) {
        if (i == dev_anchor)
          print "\t{ 0x413d, 0x2104, \"Black Shark Green Ghost Gamepad\", 0, XTYPE_XBOX360 },"
        print lines[i]
        if (i == last_vendor)
          print "\tXPAD_XBOX360_VENDOR(0x413d),\t/* Black Shark Green Ghost Controller */"
      }
    }
  ' xpad.c > xpad.c.patched
  mv xpad.c.patched xpad.c
  grep -q '0x413d' xpad.c || { echo "error: patch did not apply"; exit 1; }
fi
say "patched lines:"
grep -n '0x413d' xpad.c

# ---- 3. build ---------------------------------------------------------
cat > Makefile <<'EOF'
obj-m += xpad.o
KDIR := /lib/modules/$(shell uname -r)/build
all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules
clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean
EOF

# Vendor header trees often ship the source without the compiled build
# helpers (scripts/basic/fixdep etc.). Prepare them once so `make` works.
if [ ! -x "$KSRC/scripts/basic/fixdep" ]; then
  say "building kernel build helpers (modules_prepare) in $KSRC"
  make -C "$KSRC" modules_prepare
fi

say "building xpad.ko against $KV..."
make -j"$(nproc 2>/dev/null || echo 2)"

[ -f xpad.ko ] || { echo "error: build did not produce xpad.ko"; exit 1; }

# ---- 4. stop the xboxdrv hack (kernel must own the device now) --------
if systemctl list-unit-files 2>/dev/null | grep -q 'rdk_xboxdrv.service'; then
  say "disabling rdk_xboxdrv.service (kernel xpad will take over)"
  systemctl disable --now rdk_xboxdrv.service 2>/dev/null || true
fi
if command -v xboxdrv >/dev/null 2>&1 && pgrep -x xboxdrv >/dev/null; then
  say "killing running xboxdrv"
  pkill -x xboxdrv 2>/dev/null || true
fi
if [ -f /etc/udev/rules.d/90-rdk-xbox360.rules ]; then
  say "removing 90-rdk-xbox360.rules"
  rm -f /etc/udev/rules.d/90-rdk-xbox360.rules
  udevadm control --reload 2>/dev/null || true
fi

# ---- 5. install (extra/ takes precedence over kernel/) ----------------
say "backing up stock xpad module"
if ls "$KERNEL_DIR"/xpad.ko* >/dev/null 2>&1; then
  mkdir -p "$WORK/stock"
  cp "$KERNEL_DIR"/xpad.ko* "$WORK/stock/" 2>/dev/null || true
fi

say "unloading old xpad (if any)"
rmmod xpad 2>/dev/null || true

mkdir -p "$EXTRA_DIR"
say "installing to $EXTRA_DIR/xpad.ko"
install -m 644 xpad.ko "$EXTRA_DIR/xpad.ko"
depmod -a

say "loading patched xpad"
modprobe xpad
lsmod | grep xpad || true

say "done. Now (re)plug the receiver and check:"
echo "  ls -l /dev/input/js*"
echo "  sudo apt install -y joystick && jstest /dev/input/js0"
echo
echo "If js0 does not appear:  dmesg | tail -20"
echo
echo "Rollback:"
echo "  rm -f $EXTRA_DIR/xpad.ko && depmod -a && rmmod xpad && modprobe xpad"
