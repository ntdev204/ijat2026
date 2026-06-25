cmake_minimum_required(VERSION 3.8)
project(rai_human_perception)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -O3)
endif()

find_package(ament_cmake REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(image_transport REQUIRED)
find_package(nav_msgs REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core dnn imgproc video)
find_package(CUDAToolkit REQUIRED)
find_package(rai_controller REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_geometry_msgs REQUIRED)
find_package(tf2_ros REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/HumanState.msg"
  "msg/HumanArray.msg"
  "msg/ContextInput.msg"
  DEPENDENCIES std_msgs
)
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")

include_directories(include)

find_path(TENSORRT_INCLUDE_DIR NvInfer.h
  HINTS /usr/include /usr/local/include /usr/include/aarch64-linux-gnu)
find_library(TENSORRT_LIBRARY nvinfer
  HINTS /usr/lib /usr/local/lib /usr/lib/aarch64-linux-gnu)
find_library(CUDART_LIBRARY cudart
  HINTS /usr/local/cuda/lib64 /usr/lib/aarch64-linux-gnu)

add_library(${PROJECT_NAME}_core
  src/yolo_detector.cpp
  src/depth_association.cpp
  src/kalman_tracker.cpp
  src/human_state_estimator.cpp
)
target_include_directories(${PROJECT_NAME}_core PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
  ${OpenCV_INCLUDE_DIRS}
  ${CUDAToolkit_INCLUDE_DIRS}
)
ament_target_dependencies(${PROJECT_NAME}_core
  cv_bridge
  geometry_msgs
  nav_msgs
  rai_controller
  rclcpp
  sensor_msgs
  std_msgs
  tf2
  tf2_geometry_msgs
  tf2_ros
)
target_link_libraries(${PROJECT_NAME}_core
  ${OpenCV_LIBRARIES}
  "${cpp_typesupport_target}"
  CUDA::cudart
)
if(TENSORRT_INCLUDE_DIR AND TENSORRT_LIBRARY)
  target_compile_definitions(${PROJECT_NAME}_core PUBLIC RAI_HUMAN_PERCEPTION_HAS_TENSORRT=1)
  target_include_directories(${PROJECT_NAME}_core PUBLIC ${TENSORRT_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME}_core ${TENSORRT_LIBRARY})
  if(CUDART_LIBRARY)
    target_link_libraries(${PROJECT_NAME}_core ${CUDART_LIBRARY})
  endif()
endif()

add_executable(perception_node src/perception_node.cpp)
add_dependencies(perception_node ${PROJECT_NAME}__rosidl_generator_cpp)
ament_target_dependencies(perception_node
  cv_bridge
  geometry_msgs
  image_transport
  nav_msgs
  rai_controller
  rclcpp
  sensor_msgs
  std_msgs
  tf2
  tf2_geometry_msgs
  tf2_ros
)
target_link_libraries(perception_node
  ${PROJECT_NAME}_core
  "${cpp_typesupport_target}"
)

install(
  TARGETS ${PROJECT_NAME}_core perception_node
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY config launch scripts DESTINATION share/${PROJECT_NAME})
install(FILES README.md DESTINATION share/${PROJECT_NAME})

ament_export_dependencies(rosidl_default_runtime)
ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME}_core)

ament_package()
