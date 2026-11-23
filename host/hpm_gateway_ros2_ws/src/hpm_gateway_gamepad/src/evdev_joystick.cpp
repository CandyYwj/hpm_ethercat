/*
 * evdev_joystick.cpp - raw evdev joystick implementation.
 *
 * Capabilities are probed once at open() with EVIOCGBIT/EVIOCGABS and mapped
 * to a compact table, so the hot path (drain()) is pure read() + table lookup
 * with no ioctl and no allocation.
 *
 * Author: BearKing
 */
#include "hpm_gateway_gamepad/evdev_joystick.hpp"

#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace hpm_gateway_gamepad {

namespace {
/* Bound each drain to keep the loop fair when a device floods events. */
constexpr int kMaxEventsPerDrain = 64;
}  // namespace

bool EvdevJoystick::open(const std::string &path)
{
  close();

  fd_ = ::open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
  if (fd_ < 0) {
    return false;
  }
  path_ = path;

  char name[256] = {0};
  if (::ioctl(fd_, EVIOCGNAME(sizeof(name) - 1U), name) >= 0) {
    name_ = name;
  }

  if (!queryCapabilities()) {
    close();
    return false;
  }
  return true;
}

void EvdevJoystick::close()
{
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
  path_.clear();
  name_.clear();
  abs_map_.clear();
  key_map_.clear();
  axes_.clear();
  buttons_.clear();
}

bool EvdevJoystick::queryCapabilities()
{
  abs_map_.assign(ABS_CNT, AxisMap{});
  key_map_.assign(KEY_CNT, -1);

  /* --- axes (EV_ABS) ------------------------------------------------- */
  unsigned long abs_bits[ABS_CNT / 8UL / sizeof(unsigned long) + 1UL] = {0};
  if (::ioctl(fd_, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits) < 0) {
    return false;
  }
  int axis_index = 0;
  for (int code = ABS_X; code < ABS_CNT; ++code) {
    const unsigned long bit =
        abs_bits[code / (8UL * sizeof(unsigned long))] &
        (1UL << (code % (8UL * sizeof(unsigned long))));
    if (bit == 0UL) {
      continue;
    }
    struct input_absinfo info;
    if (::ioctl(fd_, EVIOCGABS(code), &info) < 0) {
      continue;
    }
    AxisMap &m = abs_map_[code];
    m.axis_index = axis_index;
    if (info.maximum > info.minimum) {
      m.center = (static_cast<float>(info.maximum) +
                  static_cast<float>(info.minimum)) * 0.5f;
      m.scale = 2.0f / static_cast<float>(info.maximum - info.minimum);
    }
    axes_.push_back(0.0f);
    ++axis_index;
  }

  /* --- buttons (EV_KEY in the BTN_* range) --------------------------- */
  unsigned long key_bits[KEY_CNT / 8UL / sizeof(unsigned long) + 1UL] = {0};
  if (::ioctl(fd_, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0) {
    return false;
  }
  int button_index = 0;
  for (int code = BTN_MISC; code < KEY_MAX; ++code) {
    const unsigned long bit =
        key_bits[code / (8UL * sizeof(unsigned long))] &
        (1UL << (code % (8UL * sizeof(unsigned long))));
    if (bit == 0UL) {
      continue;
    }
    key_map_[code] = button_index;
    buttons_.push_back(0);
    ++button_index;
  }

  return (axis_index > 0) || (button_index > 0);
}

int EvdevJoystick::drain()
{
  int consumed = 0;
  for (;;) {
    struct input_event ev;
    const ssize_t n = ::read(fd_, &ev, sizeof(ev));
    if (n == static_cast<ssize_t>(sizeof(ev))) {
      ++consumed;
      switch (ev.type) {
        case EV_ABS:
          if (ev.code < static_cast<int>(abs_map_.size())) {
            const AxisMap &m = abs_map_[ev.code];
            if (m.axis_index >= 0 && m.scale > 0.0f) {
              float norm = (static_cast<float>(ev.value) - m.center) * m.scale;
              if (norm > 1.0f) {
                norm = 1.0f;
              } else if (norm < -1.0f) {
                norm = -1.0f;
              }
              if (norm >= -deadzone_ && norm <= deadzone_) {
                norm = 0.0f;
              }
              axes_[m.axis_index] = norm;
            }
          }
          break;
        case EV_KEY:
          if (ev.code < static_cast<int>(key_map_.size())) {
            const int index = key_map_[ev.code];
            if (index >= 0) {
              buttons_[index] = (ev.value == 1 || ev.value == 2) ? 1 : 0;
            }
          }
          break;
        default:
          break;  /* EV_SYN etc. carry no state change */
      }
    } else if (n == -1 && errno == EAGAIN) {
      break;  /* drained */
    } else if (n == -1 && errno == EINTR) {
      continue;
    } else {
      break;  /* read error or device removed */
    }

    if (consumed >= kMaxEventsPerDrain) {
      break;
    }
  }
  return consumed;
}

JoystickSnapshot EvdevJoystick::snapshot() const
{
  JoystickSnapshot snapshot;
  snapshot.name = name_;
  snapshot.axes = axes_;
  snapshot.buttons = buttons_;
  return snapshot;
}

bool EvdevJoystick::anyButtonPressed() const
{
  for (const int32_t button : buttons_) {
    if (button != 0) {
      return true;
    }
  }
  return false;
}

}  // namespace hpm_gateway_gamepad
