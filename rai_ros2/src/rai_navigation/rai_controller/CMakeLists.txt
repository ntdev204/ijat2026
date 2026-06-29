cmake_minimum_required(VERSION 3.8)
project(rai_controller)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -O3)
endif()

find_package(ament_cmake REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(rai_controller_cca_nmpc REQUIRED)
find_package(rai_planner REQUIRED)
find_package(rai_planner_a_star REQUIRED)
find_package(rai_planner_dijkstra REQUIRED)
find_package(rai_planner_straight_line REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(tf2_ros REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/AdaptiveBounds.msg"
  "msg/Context.msg"
  "msg/HumanState.msg"
  "msg/HumanStates.msg"
  "msg/SolverStats.msg"
  "srv/GetCostmap.srv"
  "srv/GetNavigationStatus.srv"
  "srv/GetPath.srv"
  "srv/ListAlgorithms.srv"
  "srv/NavigateToPose.srv"
  "srv/SetController.srv"
  "srv/SetPlanner.srv"
  DEPENDENCIES std_msgs geometry_msgs nav_msgs
)
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")

include_directories(include)

add_library(rai_controller_core
  src/controller_algorithm.cpp
)
ament_target_dependencies(rai_controller_core
  geometry_msgs
  nav_msgs
  rai_controller_cca_nmpc
  rclcpp
)

add_executable(rai_controller_server
  src/rai_controller_server.cpp
)
add_dependencies(rai_controller_server ${PROJECT_NAME}__rosidl_generator_cpp)
ament_target_dependencies(rai_controller_server
  geometry_msgs
  nav_msgs
  rai_controller_cca_nmpc
  rai_planner
  rai_planner_a_star
  rai_planner_dijkstra
  rai_planner_straight_line
  rclcpp
  sensor_msgs
  std_msgs
  std_srvs
  tf2
  tf2_geometry_msgs
  tf2_ros
)
target_link_libraries(rai_controller_server
  rai_controller_core
  "${cpp_typesupport_target}"
)

install(
  TARGETS rai_controller_core rai_controller_server
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
ament_export_libraries(rai_controller_core)

ament_package()
