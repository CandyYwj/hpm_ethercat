/*
 * gamepad_monitor.cpp - udev hotplug monitor implementation.
 *
 * Detection is dual-path so untagged gamepads (e.g. devices udev classifies
 * as keyboards because they only get a joystick interface once the controller
 * pairs) are still found:
 *   1. udev tag ID_INPUT_JOYSTICK=1 / ID_INPUT_GAMEPAD=1, or
 *   2. capability probe: the devnode reports gamepad stick axes + buttons.
 *
 * Author: BearKing
 */
#include "hpm_gateway_gamepad/gamepad_monitor.hpp"

#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <utility>

namespace hpm_gateway_gamepad {

namespace {

bool testBit(unsigned long bit, const unsigned long *bits)
{
  return (bits[bit / (8UL * sizeof(unsigned long))] &
          (1UL << (bit % (8UL * sizeof(unsigned long))))) != 0UL;
}

/* Open the devnode briefly and probe capabilities to decide whether it looks
 * like a gamepad: >=2 stick axes (or a dpad) AND either a joystick-range
 * button or >=4 buttons. Mice/keyboards are excluded by these criteria. */
bool hasGamepadCapabilities(const char *devnode)
{
  if (devnode == nullptr) {
    return false;
  }
  const int fd = ::open(devnode, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
  if (fd < 0) {
    return false;
  }

  unsigned long abs_bits[ABS_CNT / 8UL / sizeof(unsigned long) + 1UL] = {0};
  unsigned long key_bits[KEY_CNT / 8UL / sizeof(unsigned long) + 1UL] = {0};
  if (::ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits) < 0 ||
      ::ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) < 0) {
    ::close(fd);
    return false;
  }

  static const int kStickAxes[] = {ABS_X, ABS_Y, ABS_Z, ABS_RX, ABS_RY,
                                   ABS_RZ, ABS_GAS, ABS_BRAKE, ABS_THROTTLE,
                                   ABS_RUDDER, ABS_WHEEL};
  int stick_axes = 0;
  for (const int code : kStickAxes) {
    if (testBit(static_cast<unsigned long>(code), abs_bits)) {
      ++stick_axes;
    }
  }
  const bool has_dpad = testBit(ABS_HAT0X, abs_bits) ||
                        testBit(ABS_HAT0Y, abs_bits);

  bool has_joystick_button = false;
  int buttons = 0;
  for (int code = BTN_MISC; code < KEY_MAX; ++code) {
    if (testBit(static_cast<unsigned long>(code), key_bits)) {
      ++buttons;
      if (code >= BTN_JOYSTICK && code <= BTN_THUMBR) {
        has_joystick_button = true;
      }
    }
  }

  ::close(fd);
  return (stick_axes >= 2 || has_dpad) &&
         (has_joystick_button || buttons >= 4);
}

/* A device is a gamepad when udev tags it (checked on the device and its
 * input parent) or when capability probing says it looks like one. */
bool deviceIsJoystick(udev_device *device)
{
  if (device == nullptr) {
    return false;
  }
  const char *props[] = {"ID_INPUT_JOYSTICK", "ID_INPUT_GAMEPAD"};
  for (const char *prop_name : props) {
    const char *prop = udev_device_get_property_value(device, prop_name);
    if (prop != nullptr && std::strcmp(prop, "1") == 0) {
      return true;
    }
    udev_device *parent = udev_device_get_parent_with_subsystem_devtype(
        device, "input", nullptr);
    if (parent != nullptr) {
      prop = udev_device_get_property_value(parent, prop_name);
      if (prop != nullptr && std::strcmp(prop, "1") == 0) {
        return true;
      }
    }
  }
  return hasGamepadCapabilities(udev_device_get_devnode(device));
}

}  // namespace

bool GamepadMonitor::init()
{
  close();

  udev_ = udev_new();
  if (udev_ == nullptr) {
    return false;
  }
  monitor_ = udev_monitor_new_from_netlink(udev_, "udev");
  if (monitor_ == nullptr) {
    close();
    return false;
  }
  udev_monitor_filter_add_match_subsystem_devtype(monitor_, "input", nullptr);
  udev_monitor_enable_receiving(monitor_);
  return true;
}

void GamepadMonitor::close()
{
  if (monitor_ != nullptr) {
    udev_monitor_unref(monitor_);
    monitor_ = nullptr;
  }
  if (udev_ != nullptr) {
    udev_unref(udev_);
    udev_ = nullptr;
  }
}

std::vector<GamepadDeviceEvent> GamepadMonitor::poll()
{
  std::vector<GamepadDeviceEvent> events;
  for (;;) {
    udev_device *device = udev_monitor_receive_device(monitor_);
    if (device == nullptr) {
      break;
    }

    const char *action = udev_device_get_action(device);
    const char *devnode = udev_device_get_devnode(device);
    const char *sysname = udev_device_get_sysname(device);
    const bool is_joystick = deviceIsJoystick(device);
    udev_device_unref(device);

    if (!is_joystick) {
      continue;
    }

    GamepadDeviceEvent event;
    if (action != nullptr && std::strcmp(action, "remove") == 0) {
      event.action = DeviceAction::Remove;
    } else if (action != nullptr && std::strcmp(action, "add") == 0) {
      event.action = DeviceAction::Add;
    } else {
      continue;  /* "change"/"bind" etc. are not relevant */
    }
    if (sysname != nullptr) {
      event.sysname = sysname;
    }
    if (devnode != nullptr) {
      event.devnode = devnode;
    }
    events.push_back(std::move(event));
  }
  return events;
}

std::vector<std::string> GamepadMonitor::enumerateJoysticks() const
{
  std::vector<std::string> result;
  if (udev_ == nullptr) {
    return result;
  }

  udev_enumerate *enumerate = udev_enumerate_new(udev_);
  if (enumerate == nullptr) {
    return result;
  }
  /* Scan every input device (not only udev-tagged ones) and let the combined
   * tag-or-capability check decide, so untagged gamepads are found too. */
  udev_enumerate_add_match_subsystem(enumerate, "input");
  udev_enumerate_scan_devices(enumerate);

  udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
  udev_list_entry *entry = nullptr;
  udev_list_entry_foreach(entry, devices) {
    const char *syspath = udev_list_entry_get_name(entry);
    udev_device *device = udev_device_new_from_syspath(udev_, syspath);
    if (device == nullptr) {
      continue;
    }
    if (deviceIsJoystick(device)) {
      const char *devnode = udev_device_get_devnode(device);
      if (devnode != nullptr) {
        result.emplace_back(devnode);
      }
    }
    udev_device_unref(device);
  }
  udev_enumerate_unref(enumerate);
  return result;
}

}  // namespace hpm_gateway_gamepad
