cmake_minimum_required(VERSION 3.8)
project(ca_nmpc_controller)

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
find_package(Eigen3 REQUIRED)

find_package(casadi QUIET)
if(casadi_FOUND)
  message(STATUS "Found CasADi via CMake package")
  if(TARGET casadi::casadi)
    set(casadi_LIBRARIES casadi::casadi)
  elseif(TARGET CasADi::casadi)
    set(casadi_LIBRARIES CasADi::casadi)
  elseif(TARGET casadi)
    set(casadi_LIBRARIES casadi)
  endif()
else()
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if(Python3_Interpreter_FOUND)
    execute_process(
      COMMAND ${Python3_EXECUTABLE} -c "import casadi, os; print(os.path.dirname(casadi.__file__))"
      OUTPUT_VARIABLE CASADI_PYTHON_PACKAGE_DIR
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )
  endif()

  find_path(casadi_INCLUDE_DIR
    NAMES casadi/casadi.hpp
    HINTS
      "${CASADI_PYTHON_PACKAGE_DIR}/include"
      /usr/local/include
      /usr/include
  )

  find_library(casadi_LIBRARY
    NAMES casadi libcasadi
    HINTS
      "${CASADI_PYTHON_PACKAGE_DIR}"
      "${CASADI_PYTHON_PACKAGE_DIR}/lib"
      /usr/local/lib
      /usr/lib
      /usr/lib/aarch64-linux-gnu
      /usr/lib/x86_64-linux-gnu
  )

  if(NOT casadi_INCLUDE_DIR OR NOT casadi_LIBRARY)
    message(FATAL_ERROR
      "CasADi C++ headers/library were not found. Install CasADi C++ runtime "
      "or install the Python wheel with C++ artifacts, then rebuild. Checked "
      "Python package path='${CASADI_PYTHON_PACKAGE_DIR}', /usr/local, and system lib paths."
    )
  endif()

  set(casadi_INCLUDE_DIRS ${casadi_INCLUDE_DIR})
  set(casadi_LIBRARIES ${casadi_LIBRARY})
  message(STATUS "Found CasADi include: ${casadi_INCLUDE_DIRS}")
  message(STATUS "Found CasADi library: ${casadi_LIBRARIES}")
endif()

set(CASADI_RUNTIME_DIRS "")
foreach(casadi_lib IN LISTS casadi_LIBRARIES)
  if(TARGET ${casadi_lib})
    get_target_property(casadi_imported_location ${casadi_lib} IMPORTED_LOCATION_RELEASE)
    if(NOT casadi_imported_location)
      get_target_property(casadi_imported_location ${casadi_lib} IMPORTED_LOCATION)
    endif()
    if(casadi_imported_location)
      get_filename_component(casadi_runtime_dir "${casadi_imported_location}" DIRECTORY)
      list(APPEND CASADI_RUNTIME_DIRS "${casadi_runtime_dir}")
    endif()
  elseif(IS_ABSOLUTE "${casadi_lib}")
    get_filename_component(casadi_runtime_dir "${casadi_lib}" DIRECTORY)
    list(APPEND CASADI_RUNTIME_DIRS "${casadi_runtime_dir}")
  endif()
endforeach()
list(REMOVE_DUPLICATES CASADI_RUNTIME_DIRS)

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

ament_target_dependencies(${PROJECT_NAME}
  rclcpp
  rclcpp_lifecycle
  nav2_core
  nav2_costmap_2d
  nav2_util
  geometry_msgs
  nav_msgs
  tf2
  tf2_ros
  tf2_geometry_msgs
  pluginlib
  canmpc_msgs
  ament_index_cpp
)

target_link_libraries(${PROJECT_NAME}
  ${casadi_LIBRARIES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  BUILD_RPATH "${CASADI_RUNTIME_DIRS}"
  INSTALL_RPATH "${CASADI_RUNTIME_DIRS}"
  INSTALL_RPATH_USE_LINK_PATH TRUE
)

# Export the plugin XML
pluginlib_export_plugin_description_file(nav2_core ca_nmpc_controller.xml)

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
  FILES ca_nmpc_controller.xml
  DESTINATION share/${PROJECT_NAME}
)

# Testing
if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  
  # Test Mecanum Model
  ament_add_gtest(test_mecanum_model test/test_mecanum_model.cpp)
  target_link_libraries(test_mecanum_model
    ${PROJECT_NAME}
  )
  ament_target_dependencies(test_mecanum_model rclcpp)
  
  # Test Local Plan Handler
  ament_add_gtest(test_local_plan_handler test/test_local_plan_handler.cpp)
  target_link_libraries(test_local_plan_handler
    ${PROJECT_NAME}
  )
  ament_target_dependencies(test_local_plan_handler rclcpp)

  # Test Solver Wrapper
  ament_add_gtest(test_solver_wrapper test/test_solver_wrapper.cpp)
  target_link_libraries(test_solver_wrapper
    ${PROJECT_NAME}
    ${casadi_LIBRARIES}
  )
  ament_target_dependencies(test_solver_wrapper rclcpp ament_index_cpp)

  # Test Costmap Evaluator
  ament_add_gtest(test_costmap_evaluator test/test_costmap_evaluator.cpp)
  target_link_libraries(test_costmap_evaluator
    ${PROJECT_NAME}
  )
  ament_target_dependencies(test_costmap_evaluator rclcpp)
endif()

ament_package()
