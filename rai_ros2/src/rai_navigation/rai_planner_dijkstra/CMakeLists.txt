cmake_minimum_required(VERSION 3.8)
project(rai_planner_dijkstra)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -O3)
endif()

find_package(ament_cmake REQUIRED)
find_package(rai_planner REQUIRED)

add_library(rai_planner_dijkstra
  src/dijkstra_planner.cpp
)

target_include_directories(rai_planner_dijkstra PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)

ament_target_dependencies(rai_planner_dijkstra rai_planner)

install(
  TARGETS rai_planner_dijkstra
  EXPORT rai_planner_dijkstraTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

ament_export_targets(rai_planner_dijkstraTargets HAS_LIBRARY_TARGET)
ament_export_dependencies(rai_planner)
ament_export_include_directories(include)
ament_export_libraries(rai_planner_dijkstra)

ament_package()
