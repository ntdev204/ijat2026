#!/bin/bash
# ==============================================================================
# RAI Project - Raspberry Pi 4 Autostart Script
# ==============================================================================
set -euo pipefail

# 1. Environment Setup
source /opt/ros/humble/setup.bash
source /home/rai/ijat2026/rai_ros2/install/setup.bash

# 2. Core ROS 2 & RMW Configurations
export ROS_DOMAIN_ID=30
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp

# 3. Device Specific Roles & Paths
export RAI_DEVICE_ROLE=pi
export RAI_DEVICE_LABEL=raspberry_pi_4
export RAI_DATASET_PATH=/home/rai/ijat2026/dataset

# 4. Optional: Wait for network to be fully up (if interface bonding is needed)
sleep 2

# 5. Execute ROS 2 Launch
# Launches hardware layer, lidar, IMU, EKF, twist_mux, web API, and Nav2
ros2 launch turn_on_rai_robot pi_runtime.launch.py with_nav2:=true
