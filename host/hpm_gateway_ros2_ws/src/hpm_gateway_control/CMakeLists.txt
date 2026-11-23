cmake_minimum_required(VERSION 3.8)
project(hpm_gateway_control)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(hpm_gateway_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(Threads REQUIRED)

if(DEFINED ENV{SOEM_DIR})
  set(HPM_GATEWAY_DEFAULT_SOEM_DIR "$ENV{SOEM_DIR}")
else()
  set(HPM_GATEWAY_DEFAULT_SOEM_DIR "/root/Tools/EtherCat/SOEM")
endif()

set(SOEM_DIR "${HPM_GATEWAY_DEFAULT_SOEM_DIR}"
  CACHE PATH "SOEM source/install directory")

find_path(SOEM_INCLUDE_DIR
  NAMES soem/soem.h
  PATHS
    "${SOEM_DIR}/include"
    "/usr/local/include"
    "/usr/include"
)

find_library(SOEM_LIBRARY
  NAMES soem
  PATHS
    "${SOEM_DIR}"
    "${SOEM_DIR}/lib"
    "${SOEM_DIR}/build"
    "${SOEM_DIR}/build/lib"
    "/usr/local/lib"
    "/usr/local/lib64"
    "/usr/lib"
)

if(NOT SOEM_INCLUDE_DIR OR NOT SOEM_LIBRARY)
  message(FATAL_ERROR
    "SOEM was not found. Set SOEM_DIR or install the SOEM headers/library.")
endif()

add_executable(hpm_gateway_control_node
  src/gateway_ecat_api.cpp
  src/hpm_gateway_control_node.cpp
)

target_include_directories(hpm_gateway_control_node PRIVATE
  include
  ${SOEM_INCLUDE_DIR}
)

ament_target_dependencies(hpm_gateway_control_node
  rclcpp
  hpm_gateway_msgs
  sensor_msgs
)

target_link_libraries(hpm_gateway_control_node
  ${SOEM_LIBRARY}
  Threads::Threads
  m
)

get_filename_component(SOEM_LIBRARY_DIR "${SOEM_LIBRARY}" DIRECTORY)
set_target_properties(hpm_gateway_control_node PROPERTIES
  BUILD_RPATH "${SOEM_LIBRARY_DIR}"
  INSTALL_RPATH "${SOEM_LIBRARY_DIR}"
)

install(TARGETS hpm_gateway_control_node
  DESTINATION lib/${PROJECT_NAME}
)

install(PROGRAMS
  scripts/send_reference_motor_commands.py
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
