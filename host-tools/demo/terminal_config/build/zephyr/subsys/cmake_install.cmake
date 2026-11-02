# Install script for directory: /sripiranavan/installs/Embedded/RTOS/zephyrproject/zephyr/subsys

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/home/sripiranavan/zephyr-sdk-1.0.1/gnu/arm-zephyr-eabi/bin/arm-zephyr-eabi-objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/authentication/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/canbus/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/debug/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/fs/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/gnss/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/instrumentation/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/ipc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/kvss/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/logging/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/mem_mgmt/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/mgmt/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/pm/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/pmci/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/portability/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/random/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/sd/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/stats/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/storage/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/testsuite/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/tracing/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/usb/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/crc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/settings/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/shell/cmake_install.cmake")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/sripiranavan/development/germany/sm3/research/can_t1s_latency_bench/host-tools/demo/terminal_config/build/zephyr/subsys/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
