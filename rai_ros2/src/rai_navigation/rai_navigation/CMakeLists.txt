cmake_minimum_required(VERSION 3.8)
project(rai_navigation)

find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED)

install(
  DIRECTORY config launch
  DESTINATION share/${PROJECT_NAME}
)

# Install Python module (rai_navigation/)
ament_python_install_package(${PROJECT_NAME})

# Install Python scripts as runnable executables
install(PROGRAMS
  rai_navigation/rai_map_server.py
  rai_navigation/waypoint_cycle.py
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
