/*
 * gamepad_node.hpp - ROS 2 gamepad node, fully event-driven.
 *
 * A single epoll loop multiplexes:
 *   - udev netlink fd  -> gamepad hotplug (add/remove)
 *   - evdev fds        -> joystick input events
 *   - signalfd         -> SIGINT/SIGTERM graceful shutdown
 *   - timerfd          -> optional autorepeat / heartbeat
 *
 * Nothing polls: the thread sleeps in epoll_wait() and only wakes on a real
 * event, then publishes sensor_msgs/msg/Joy immediately.
 *
 * Author: BearKing
 */
#ifndef HPM_GATEWAY_GAMEPAD__GAMEPAD_NODE_HPP_
#define HPM_GATEWAY_GAMEPAD__GAMEPAD_NODE_HPP_

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joy.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "hpm_gateway_gamepad/evdev_joystick.hpp"
#include "hpm_gateway_gamepad/gamepad_monitor.hpp"

namespace hpm_gateway_gamepad {

class GamepadNode : public rclcpp::Node {
 public:
  GamepadNode();
  ~GamepadNode() override;

  /* Runs the event loop; returns the process exit code. */
  int run();

 private:
  void handleSignal();
  void handleMonitorEvent();
  void handleJoystick(int fd);
  void handleRepeatTimer();
  void handleHeartbeat();

  bool addJoystick(const std::string &devnode, const std::string &sysname);
  void removeJoystick(const std::string &devnode);
  void publishState();
  void updateRepeatTimer();

  bool addToEpoll(int fd);
  void removeFromEpoll(int fd);
  void closeFds();

  rclcpp::Publisher<sensor_msgs::msg::Joy>::SharedPtr joy_pub_;

  std::string dev_;                 /* fixed device path; "" = auto via udev */
  float deadzone_;
  double autorepeat_rate_;          /* Hz; 0 disables */
  int heartbeat_period_ms_;         /* 0 disables */
  std::string frame_id_;
  std::string topic_;

  GamepadMonitor monitor_;
  /* devnode -> joystick */
  std::map<std::string, std::unique_ptr<EvdevJoystick>> joysticks_;
  /* fd -> devnode, for epoll dispatch */
  std::map<int, std::string> fd_to_devnode_;
  /* sysname -> devnode, for matching remove events that lack a devnode */
  std::map<std::string, std::string> sysname_to_devnode_;

  int epoll_fd_ = -1;
  int signal_fd_ = -1;
  int repeat_fd_ = -1;
  int heartbeat_fd_ = -1;
  bool running_ = true;

  /* Last known axis/button shape so a disconnect can publish zeroed state. */
  std::vector<float> last_axes_;
  std::vector<int32_t> last_buttons_;
};

}  // namespace hpm_gateway_gamepad

#endif  // HPM_GATEWAY_GAMEPAD__GAMEPAD_NODE_HPP_
