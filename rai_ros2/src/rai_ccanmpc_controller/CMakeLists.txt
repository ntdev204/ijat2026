cmake_minimum_required(VERSION 3.8)
project(rai_ccanmpc_controller)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -O3)
endif()

find_package(ament_cmake REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav2_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(std_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(tf2_ros REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/AdaptiveBounds.msg"
  "msg/Context.msg"
  "msg/HumanState.msg"
  "msg/HumanStates.msg"
  "msg/SolverStats.msg"
  DEPENDENCIES std_msgs geometry_msgs
)
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")

include_directories(include)

add_library(rai_ccanmpc_core
  src/ccanmpc_core.cpp
)
ament_target_dependencies(rai_ccanmpc_core
  geometry_msgs
  nav_msgs
  rclcpp
)

add_executable(rai_ccanmpc_controller_node
  src/rai_ccanmpc_controller_node.cpp
)
add_dependencies(rai_ccanmpc_controller_node ${PROJECT_NAME}__rosidl_generator_cpp)
ament_target_dependencies(rai_ccanmpc_controller_node
  geometry_msgs
  nav2_msgs
  nav_msgs
  rclcpp
  rclcpp_action
  std_msgs
  tf2
  tf2_geometry_msgs
  tf2_ros
)
target_link_libraries(rai_ccanmpc_controller_node
  rai_ccanmpc_core
  "${cpp_typesupport_target}"
)

install(
  TARGETS rai_ccanmpc_core rai_ccanmpc_controller_node
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY include/
  DESTINATION include
)

install(
  DIRECTORY config launch
  DESTINATION share/${PROJECT_NAME}
)

ament_export_dependencies(rosidl_default_runtime)
ament_export_include_directories(include)
ament_export_libraries(rai_ccanmpc_core)

ament_package()
