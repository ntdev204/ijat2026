cmake_minimum_required(VERSION 3.8)
project(canmpc_costmap_layers)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -O3)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(nav2_costmap_2d REQUIRED)
find_package(pluginlib REQUIRED)
find_package(canmpc_msgs REQUIRED)

# Include directories
include_directories(include)

# Define library
add_library(${PROJECT_NAME} SHARED
  src/human_semantic_layer.cpp
)

target_link_libraries(${PROJECT_NAME}
  rclcpp::rclcpp
  nav2_costmap_2d::nav2_costmap_2d
  pluginlib::pluginlib
  canmpc_msgs::canmpc_msgs
)

# Export the plugin XML
pluginlib_export_plugin_description_file(nav2_costmap_2d canmpc_costmap_layers.xml)

# Install
install(
  TARGETS ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(
  DIRECTORY include/
  DESTINATION include/
)

install(
  FILES canmpc_costmap_layers.xml
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
