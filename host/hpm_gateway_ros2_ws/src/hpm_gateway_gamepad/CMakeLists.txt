cmake_minimum_required(VERSION 3.8)
project(hpm_gateway_gamepad)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(Threads REQUIRED)

# libudev provides the netlink hotplug monitor (system dependency).
find_path(UDEV_INCLUDE_DIR
  NAMES libudev.h
  PATHS /usr/include /usr/local/include
)
find_library(UDEV_LIBRARY
  NAMES udev
  PATHS /usr/lib /usr/lib64 /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu
)

if(NOT UDEV_INCLUDE_DIR OR NOT UDEV_LIBRARY)
  message(FATAL_ERROR
    "libudev was not found. Install it on the RDK with: apt install libudev-dev")
endif()

add_executable(gamepad_node
  src/evdev_joystick.cpp
  src/gamepad_monitor.cpp
  src/gamepad_node.cpp
  src/gamepad_main.cpp
)

target_include_directories(gamepad_node PRIVATE
  include
  ${UDEV_INCLUDE_DIR}
)

ament_target_dependencies(gamepad_node
  rclcpp
  sensor_msgs
)

target_link_libraries(gamepad_node
  ${UDEV_LIBRARY}
  Threads::Threads
  m
)

install(TARGETS gamepad_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
