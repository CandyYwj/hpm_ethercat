/*
 * gamepad_monitor.hpp - udev hotplug monitor for joystick devices.
 *
 * Uses the udev netlink monitor (not inotify polling of /dev/input), so the
 * node learns about gamepad insert/remove the instant the kernel emits the
 * event. Exposes:
 *   - fd()  -> register with epoll; call poll() when readable
 *   - enumerateJoysticks() -> devices already present at startup
 *
 * Author: BearKing
 */
#ifndef HPM_GATEWAY_GAMEPAD__GAMEPAD_MONITOR_HPP_
#define HPM_GATEWAY_GAMEPAD__GAMEPAD_MONITOR_HPP_

#include <libudev.h>

#include <string>
#include <vector>

namespace hpm_gateway_gamepad {

enum class DeviceAction { Add, Remove };

struct GamepadDeviceEvent {
  DeviceAction action;
  std::string devnode;  /* /dev/input/eventX */
  std::string sysname;  /* eventX */
};

class GamepadMonitor {
 public:
  GamepadMonitor() = default;
  ~GamepadMonitor() { close(); }

  GamepadMonitor(const GamepadMonitor &) = delete;
  GamepadMonitor &operator=(const GamepadMonitor &) = delete;

  bool init();
  void close();

  int fd() const { return monitor_ ? udev_monitor_get_fd(monitor_) : -1; }

  /* Drain pending udev events (call when fd() is readable). */
  std::vector<GamepadDeviceEvent> poll();

  /* Absolute /dev/input/event* paths of joysticks already connected. */
  std::vector<std::string> enumerateJoysticks() const;

 private:
  udev *udev_ = nullptr;
  udev_monitor *monitor_ = nullptr;
};

}  // namespace hpm_gateway_gamepad

#endif  // HPM_GATEWAY_GAMEPAD__GAMEPAD_MONITOR_HPP_
