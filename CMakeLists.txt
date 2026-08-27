cmake_minimum_required(VERSION 3.8)
project(spirob_diff_drive)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(diff_drive_teleop src/diff_drive_teleop.cpp)
target_include_directories(diff_drive_teleop PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_compile_features(diff_drive_teleop PUBLIC c_std_99 cxx_std_17)
ament_target_dependencies(diff_drive_teleop
  rclcpp
  geometry_msgs
  std_msgs)

install(TARGETS diff_drive_teleop
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
