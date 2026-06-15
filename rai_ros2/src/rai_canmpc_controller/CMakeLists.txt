cmake_minimum_required(VERSION 3.8)
project(rai_canmpc_controller)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -O3)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(nav2_core REQUIRED)
find_package(nav2_costmap_2d REQUIRED)
find_package(nav2_util REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(pluginlib REQUIRED)
find_package(canmpc_msgs REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(casadi REQUIRED)
find_package(Eigen3 REQUIRED)

# Include directories
include_directories(
  include
  ${casadi_INCLUDE_DIRS}
  ${Eigen3_INCLUDE_DIRS}
)

# Build the generated solver as a shared library
add_library(canmpc_solver SHARED generated/canmpc_solver.c)
set_target_properties(canmpc_solver PROPERTIES LINKER_LANGUAGE C)

# Define main controller library
add_library(${PROJECT_NAME} SHARED
  src/canmpc_controller.cpp
  src/mecanum_model.cpp
  src/local_plan_handler.cpp
  src/solver_wrapper.cpp
)
add_dependencies(${PROJECT_NAME} canmpc_solver)

target_link_libraries(${PROJECT_NAME}
  rclcpp::rclcpp
  rclcpp_lifecycle::rclcpp_lifecycle
  nav2_core::nav2_core
  nav2_costmap_2d::nav2_costmap_2d
  nav2_util::nav2_util
  geometry_msgs::geometry_msgs
  nav_msgs::nav_msgs
  tf2_ros::tf2_ros
  pluginlib::pluginlib
  canmpc_msgs::canmpc_msgs
  ament_index_cpp::ament_index_cpp
  ${casadi_LIBRARIES}
)

# Export the plugin XML
pluginlib_export_plugin_description_file(nav2_core rai_canmpc_controller.xml)

# Declare human_estimator_node executable
add_executable(human_estimator_node
  src/human_estimator_node.cpp
)

# Link dependencies for human_estimator_node
ament_target_dependencies(human_estimator_node
  rclcpp
  geometry_msgs
  canmpc_msgs
  tf2
  tf2_ros
  tf2_geometry_msgs
)

# Install targets
install(
  TARGETS ${PROJECT_NAME} canmpc_solver human_estimator_node
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(
  DIRECTORY include/
  DESTINATION include/
)

install(
  FILES rai_canmpc_controller.xml
  DESTINATION share/${PROJECT_NAME}
)

# Testing
if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  
  # Test Mecanum Model
  ament_add_gtest(test_mecanum_model test/test_mecanum_model.cpp)
  target_link_libraries(test_mecanum_model
    ${PROJECT_NAME}
    rclcpp::rclcpp
  )
  
  # Test Local Plan Handler
  ament_add_gtest(test_local_plan_handler test/test_local_plan_handler.cpp)
  target_link_libraries(test_local_plan_handler
    ${PROJECT_NAME}
    rclcpp::rclcpp
  )

  # Test Solver Wrapper
  ament_add_gtest(test_solver_wrapper test/test_solver_wrapper.cpp)
  target_link_libraries(test_solver_wrapper
    ${PROJECT_NAME}
    rclcpp::rclcpp
    ament_index_cpp::ament_index_cpp
    ${casadi_LIBRARIES}
  )

  # Test Costmap Evaluator
  ament_add_gtest(test_costmap_evaluator test/test_costmap_evaluator.cpp)
  target_link_libraries(test_costmap_evaluator
    ${PROJECT_NAME}
    rclcpp::rclcpp
  )
endif()

ament_package()
