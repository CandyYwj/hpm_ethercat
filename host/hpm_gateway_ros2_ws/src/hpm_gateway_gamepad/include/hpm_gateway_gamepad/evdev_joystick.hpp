/*
 * evdev_joystick.hpp - RAII wrapper around a raw evdev joystick node
 * (/dev/input/eventX). Reads events on demand (driven by epoll) and keeps a
 * normalized snapshot that can be published as sensor_msgs/msg/Joy.
 *
 * No polling threads, no busy waits: the caller registers fd() with epoll
 * and calls drain() when the fd is readable.
 *
 * Author: BearKing
 */
#ifndef HPM_GATEWAY_GAMEPAD__EVDEV_JOYSTICK_HPP_
#define HPM_GATEWAY_GAMEPAD__EVDEV_JOYSTICK_HPP_

#include <cstdint>
#include <string>
#include <vector>

namespace hpm_gateway_gamepad {

struct JoystickSnapshot {
  std::vector<float> axes;        /* normalized [-1,1], deadzone applied */
  std::vector<int32_t> buttons;   /* 0/1 */
  std::string name;               /* device name from EVIOCGNAME */
};

class EvdevJoystick {
 public:
  EvdevJoystick() = default;
  ~EvdevJoystick() { close(); }

  EvdevJoystick(const EvdevJoystick &) = delete;
  EvdevJoystick &operator=(const EvdevJoystick &) = delete;

  /* Open /dev/input/eventX non-blocking and probe axis/button capabilities. */
  bool open(const std::string &path);
  void close();

  bool isOpen() const { return fd_ >= 0; }
  int fd() const { return fd_; }
  const std::string &path() const { return path_; }
  const std::string &name() const { return name_; }

  /* Consume all pending input events. Returns number of events consumed. */
  int drain();

  JoystickSnapshot snapshot() const;
  void setDeadzone(float deadzone) { deadzone_ = deadzone; }

  bool anyButtonPressed() const;

 private:
  bool queryCapabilities();

  int fd_ = -1;
  std::string path_;
  std::string name_;
  float deadzone_ = 0.05f;

  /* abs code -> (axis index, center, scale) for raw->normalized mapping. */
  struct AxisMap {
    int axis_index = -1;
    float center = 0.0f;
    float scale = 0.0f;  /* 2/(max-min); 0 when the axis is not usable */
  };
  std::vector<AxisMap> abs_map_;  /* sized ABS_CNT */
  std::vector<int> key_map_;      /* key code -> button index, -1 if none */
  std::vector<float> axes_;
  std::vector<int32_t> buttons_;
};

}  // namespace hpm_gateway_gamepad

#endif  // HPM_GATEWAY_GAMEPAD__EVDEV_JOYSTICK_HPP_
