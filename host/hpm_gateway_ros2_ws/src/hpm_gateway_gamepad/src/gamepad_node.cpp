/*
 * gamepad_node.cpp - event-driven gamepad node implementation.
 *
 * Author: BearKing
 */
#include "hpm_gateway_gamepad/gamepad_node.hpp"

#include <fcntl.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <utility>

namespace hpm_gateway_gamepad {

namespace {
constexpr int kMaxEpollEvents = 16;

/* Arm a periodic timer (it_interval == it_value) at the given period, or
 * disarm when period_seconds <= 0. */
void setPeriodicTimerSeconds(int fd, double period_seconds)
{
  struct itimerspec spec = {};
  if (period_seconds > 0.0) {
    const long sec = static_cast<long>(period_seconds);
    spec.it_value.tv_sec = sec;
    spec.it_value.tv_nsec = static_cast<long>((period_seconds - sec) * 1e9);
    spec.it_interval = spec.it_value;
  }
  (void)timerfd_settime(fd, 0, &spec, nullptr);
}

void setPeriodicTimer(int fd, int period_ms)
{
  struct itimerspec spec = {};
  spec.it_value.tv_sec = period_ms / 1000;
  spec.it_value.tv_nsec = (period_ms % 1000) * 1000000L;
  spec.it_interval = spec.it_value;
  (void)timerfd_settime(fd, 0, &spec, nullptr);
}
}  // namespace

GamepadNode::GamepadNode() : Node("gamepad")
{
  declare_parameter<std::string>("dev", "");
  declare_parameter<double>("deadzone", 0.05);
  declare_parameter<double>("autorepeat_rate", 20.0);
  declare_parameter<int>("heartbeat_period_ms", 0);
  declare_parameter<std::string>("frame_id", "gamepad");
  declare_parameter<std::string>("topic", "/gamepad/joy");

  dev_ = get_parameter("dev").as_string();
  deadzone_ = static_cast<float>(get_parameter("deadzone").as_double());
  autorepeat_rate_ = get_parameter("autorepeat_rate").as_double();
  heartbeat_period_ms_ = get_parameter("heartbeat_period_ms").as_int();
  frame_id_ = get_parameter("frame_id").as_string();
  topic_ = get_parameter("topic").as_string();

  joy_pub_ = create_publisher<sensor_msgs::msg::Joy>(topic_, 10);

  RCLCPP_INFO(get_logger(),
              "topic=%s dev=%s deadzone=%.2f autorepeat=%.1fHz heartbeat=%dms",
              topic_.c_str(), dev_.empty() ? "(auto)" : dev_.c_str(),
              deadzone_, autorepeat_rate_, heartbeat_period_ms_);
}

GamepadNode::~GamepadNode()
{
  closeFds();
}

int GamepadNode::run()
{
  /* --- signal handling via signalfd (event-driven shutdown) ----------- */
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  if (sigprocmask(SIG_BLOCK, &mask, nullptr) != 0) {
    RCLCPP_ERROR(get_logger(), "sigprocmask failed: %s", std::strerror(errno));
    return EXIT_FAILURE;
  }
  signal_fd_ = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
  if (signal_fd_ < 0) {
    RCLCPP_ERROR(get_logger(), "signalfd failed: %s", std::strerror(errno));
    return EXIT_FAILURE;
  }

  if (!monitor_.init()) {
    RCLCPP_ERROR(get_logger(), "udev monitor init failed");
    return EXIT_FAILURE;
  }

  epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd_ < 0) {
    RCLCPP_ERROR(get_logger(), "epoll_create1 failed: %s", std::strerror(errno));
    return EXIT_FAILURE;
  }
  addToEpoll(signal_fd_);
  addToEpoll(monitor_.fd());

  if (autorepeat_rate_ > 0.0) {
    repeat_fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (repeat_fd_ >= 0) {
      addToEpoll(repeat_fd_);
    }
  }
  if (heartbeat_period_ms_ > 0) {
    heartbeat_fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (heartbeat_fd_ >= 0) {
      addToEpoll(heartbeat_fd_);
      setPeriodicTimer(heartbeat_fd_, heartbeat_period_ms_);
    }
  }

  /* --- initial device population ------------------------------------- */
  if (!dev_.empty()) {
    addJoystick(dev_, "");
  } else {
    for (const std::string &path : monitor_.enumerateJoysticks()) {
      addJoystick(path, "");
    }
  }

  /* --- event loop: sleep until something actually happens ------------- */
  std::vector<epoll_event> events(kMaxEpollEvents);
  while (running_) {
    const int count = epoll_wait(epoll_fd_, events.data(),
                                 static_cast<int>(events.size()), -1);
    if (count < 0) {
      if (errno == EINTR) {
        continue;
      }
      RCLCPP_ERROR(get_logger(), "epoll_wait failed: %s", std::strerror(errno));
      break;
    }
    for (int i = 0; i < count; ++i) {
      const int fd = events[i].data.fd;
      if (fd == signal_fd_) {
        handleSignal();
      } else if (fd == monitor_.fd()) {
        handleMonitorEvent();
      } else if (fd == repeat_fd_) {
        handleRepeatTimer();
      } else if (fd == heartbeat_fd_) {
        handleHeartbeat();
      } else {
        handleJoystick(fd);
      }
      if (!running_) {
        break;
      }
    }
  }

  joysticks_.clear();
  return EXIT_SUCCESS;
}

void GamepadNode::handleSignal()
{
  struct signalfd_siginfo info;
  const ssize_t n = ::read(signal_fd_, &info, sizeof(info));
  if (n == static_cast<ssize_t>(sizeof(info))) {
    RCLCPP_INFO(get_logger(), "signal %u received, shutting down",
                info.ssi_signo);
  }
  running_ = false;
}

void GamepadNode::handleMonitorEvent()
{
  for (const GamepadDeviceEvent &event : monitor_.poll()) {
    if (event.action == DeviceAction::Add) {
      addJoystick(event.devnode, event.sysname);
    } else {
      /* On removal the devnode may be empty; fall back to sysname match. */
      if (!event.devnode.empty() && joysticks_.count(event.devnode) != 0U) {
        removeJoystick(event.devnode);
      } else if (!event.sysname.empty()) {
        auto it = sysname_to_devnode_.find(event.sysname);
        if (it != sysname_to_devnode_.end()) {
          removeJoystick(it->second);
        }
      }
    }
  }
}

void GamepadNode::handleJoystick(int fd)
{
  auto fd_it = fd_to_devnode_.find(fd);
  if (fd_it == fd_to_devnode_.end()) {
    return;
  }
  auto joy_it = joysticks_.find(fd_it->second);
  if (joy_it == joysticks_.end()) {
    return;
  }
  EvdevJoystick &joystick = *joy_it->second;
  if (joystick.drain() > 0) {
    publishState();
  }
  updateRepeatTimer();
}

void GamepadNode::handleRepeatTimer()
{
  uint64_t expirations = 0;
  (void)::read(repeat_fd_, &expirations, sizeof(expirations));
  publishState();
  updateRepeatTimer();
}

void GamepadNode::handleHeartbeat()
{
  uint64_t expirations = 0;
  (void)::read(heartbeat_fd_, &expirations, sizeof(expirations));
  publishState();
}

bool GamepadNode::addJoystick(const std::string &devnode,
                              const std::string &sysname)
{
  if (devnode.empty() || joysticks_.count(devnode) != 0U) {
    return true;  /* already open */
  }
  auto joystick = std::make_unique<EvdevJoystick>();
  joystick->setDeadzone(deadzone_);
  if (!joystick->open(devnode)) {
    RCLCPP_WARN(get_logger(), "cannot open %s: %s", devnode.c_str(),
                std::strerror(errno));
    return false;
  }

  const int fd = joystick->fd();
  if (!addToEpoll(fd)) {
    RCLCPP_WARN(get_logger(), "epoll add failed for %s", devnode.c_str());
    return false;
  }
  fd_to_devnode_[fd] = devnode;
  if (!sysname.empty()) {
    sysname_to_devnode_[sysname] = devnode;
  }
  joysticks_[devnode] = std::move(joystick);

  RCLCPP_INFO(get_logger(), "gamepad connected: %s (%s)", devnode.c_str(),
              joysticks_[devnode]->name().c_str());
  publishState();
  updateRepeatTimer();
  return true;
}

void GamepadNode::removeJoystick(const std::string &devnode)
{
  auto joy_it = joysticks_.find(devnode);
  if (joy_it == joysticks_.end()) {
    return;
  }
  const int fd = joy_it->second->fd();
  removeFromEpoll(fd);
  fd_to_devnode_.erase(fd);
  for (auto it = sysname_to_devnode_.begin(); it != sysname_to_devnode_.end();
       ++it) {
    if (it->second == devnode) {
      sysname_to_devnode_.erase(it);
      break;
    }
  }
  joysticks_.erase(joy_it);
  RCLCPP_INFO(get_logger(), "gamepad disconnected: %s", devnode.c_str());

  /* Publish a zeroed state so downstream control can stop immediately. */
  publishState();
  updateRepeatTimer();
}

void GamepadNode::publishState()
{
  sensor_msgs::msg::Joy message;
  message.header.stamp = now();
  message.header.frame_id = frame_id_;

  if (!joysticks_.empty()) {
    const EvdevJoystick &joystick = *joysticks_.begin()->second;
    const JoystickSnapshot snapshot = joystick.snapshot();
    last_axes_ = snapshot.axes;
    last_buttons_ = snapshot.buttons;
  } else {
    /* Disconnect or never-connected: publish zeros with the last shape. */
    last_axes_.assign(last_axes_.size(), 0.0f);
    last_buttons_.assign(last_buttons_.size(), 0);
  }

  message.axes.assign(last_axes_.begin(), last_axes_.end());
  message.buttons.assign(last_buttons_.begin(), last_buttons_.end());
  joy_pub_->publish(message);
}

void GamepadNode::updateRepeatTimer()
{
  if (repeat_fd_ < 0) {
    return;
  }
  if (joysticks_.empty()) {
    setPeriodicTimerSeconds(repeat_fd_, 0.0);  /* disarm */
    return;
  }
  const EvdevJoystick &joystick = *joysticks_.begin()->second;
  if (joystick.anyButtonPressed() && autorepeat_rate_ > 0.0) {
    setPeriodicTimerSeconds(repeat_fd_, 1.0 / autorepeat_rate_);
  } else {
    setPeriodicTimerSeconds(repeat_fd_, 0.0);  /* disarm */
  }
}

bool GamepadNode::addToEpoll(int fd)
{
  if (fd < 0) {
    return false;
  }
  epoll_event event = {};
  event.events = EPOLLIN;
  event.data.fd = fd;
  return epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) == 0;
}

void GamepadNode::removeFromEpoll(int fd)
{
  if (fd >= 0) {
    (void)epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
  }
}

void GamepadNode::closeFds()
{
  if (epoll_fd_ >= 0) {
    ::close(epoll_fd_);
    epoll_fd_ = -1;
  }
  if (signal_fd_ >= 0) {
    ::close(signal_fd_);
    signal_fd_ = -1;
  }
  if (repeat_fd_ >= 0) {
    ::close(repeat_fd_);
    repeat_fd_ = -1;
  }
  if (heartbeat_fd_ >= 0) {
    ::close(heartbeat_fd_);
    heartbeat_fd_ = -1;
  }
}

}  // namespace hpm_gateway_gamepad
