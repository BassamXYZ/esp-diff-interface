cmake_minimum_required(VERSION 3.8)
project(esp32_diff_hw_interface)

# ── Find packages ─────────────────────────────────────────────
find_package(ament_cmake          REQUIRED)
find_package(hardware_interface   REQUIRED)
find_package(pluginlib            REQUIRED)
find_package(rclcpp               REQUIRED)
find_package(rclcpp_lifecycle     REQUIRED)
find_package(sensor_msgs          REQUIRED)
find_package(std_msgs             REQUIRED)

# ── Shared library (pluginlib requires a shared lib) ─────────
add_library(${PROJECT_NAME} SHARED
  src/esp32_diff_hw_interface.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

ament_target_dependencies(${PROJECT_NAME}
  hardware_interface
  pluginlib
  rclcpp
  rclcpp_lifecycle
  sensor_msgs
  std_msgs
)

# ── pluginlib XML ─────────────────────────────────────────────
pluginlib_export_plugin_description_file(
  hardware_interface
  esp32_diff_hw_interface.xml
)

# ── Install ───────────────────────────────────────────────────
install(TARGETS ${PROJECT_NAME}
  EXPORT  ${PROJECT_NAME}Targets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/
  DESTINATION include
)

install(FILES esp32_diff_hw_interface.xml
  DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY config launch
  DESTINATION share/${PROJECT_NAME}
)

ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME})
ament_export_targets(${PROJECT_NAME}Targets HAS_LIBRARY_TARGET)

ament_package()
