/*
 * gamepad_main.cpp - entry point for the event-driven gamepad node.
 *
 * Author: BearKing
 */
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include "hpm_gateway_gamepad/gamepad_node.hpp"

int main(int argc, char **argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<hpm_gateway_gamepad::GamepadNode>();
  const int exit_code = node->run();
  rclcpp::shutdown();
  return exit_code;
}
