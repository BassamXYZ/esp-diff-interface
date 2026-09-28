cmake_minimum_required(VERSION 3.8)
project(esp_diff_interface)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(controller_interface REQUIRED)
find_package(hardware_interface REQUIRED)
find_package(pluginlib REQUIRED)

include_directories(include)   # if you keep headers in include/ folder

# Build the shared library
add_library(${PROJECT_NAME} SHARED
  src/esp_diff_interface.cpp
)

# Specify dependencies
target_include_directories(${PROJECT_NAME} PRIVATE
  ${rclcpp_INCLUDE_DIRS}
  ${controller_interface_INCLUDE_DIRS}
  ${hardware_interface_INCLUDE_DIRS}
  ${pluginlib_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
  ${rclcpp_LIBRARIES}
  ${controller_interface_LIBRARIES}
  ${hardware_interface_LIBRARIES}
  ${pluginlib_LIBRARIES}
)

# Export the plugin description file
pluginlib_export_plugin_description_file(hardware_interface esp_diff_interface.xml)

# Install the library and the plugin description
install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(FILES esp_diff_interface.xml
  DESTINATION share/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
