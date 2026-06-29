cmake_minimum_required(VERSION 3.8)
project(rai_controller_cca_nmpc)

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

add_library(rai_controller_cca_nmpc
  src/ccanmpc_core.cpp
  src/human_predictor.cpp
)

target_include_directories(rai_controller_cca_nmpc PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)

ament_target_dependencies(rai_controller_cca_nmpc
  geometry_msgs
  nav_msgs
  rclcpp
)

install(
  TARGETS rai_controller_cca_nmpc
  EXPORT rai_controller_cca_nmpcTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

ament_export_targets(rai_controller_cca_nmpcTargets HAS_LIBRARY_TARGET)
ament_export_dependencies(geometry_msgs nav_msgs rclcpp)
ament_export_include_directories(include)
ament_export_libraries(rai_controller_cca_nmpc)

ament_package()
