cmake_minimum_required(VERSION 3.8)
project(rai_planner)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -O3)
endif()

find_package(ament_cmake REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)

add_library(rai_planner
  src/planner.cpp
  src/grid_planner_utils.cpp
)

target_include_directories(rai_planner PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)

ament_target_dependencies(rai_planner
  geometry_msgs
  nav_msgs
  rclcpp
  tf2
  tf2_geometry_msgs
)

install(
  TARGETS rai_planner
  EXPORT rai_plannerTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(
  DIRECTORY include/
  DESTINATION include
)

install(
  DIRECTORY docs
  DESTINATION share/${PROJECT_NAME}
)

ament_export_targets(rai_plannerTargets HAS_LIBRARY_TARGET)
ament_export_dependencies(geometry_msgs nav_msgs rclcpp tf2 tf2_geometry_msgs)
ament_export_include_directories(include)
ament_export_libraries(rai_planner)

ament_package()
