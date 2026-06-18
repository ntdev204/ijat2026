#!/bin/bash
# ==============================================================================
# RAI Project - Jetson Orin Nano Autostart Script
# ==============================================================================
set -euo pipefail

# 1. Environment Setup
source /opt/ros/humble/setup.bash
source /home/rai/ijat2026/rai_ros2/install/setup.bash

# 2. Core ROS 2 & RMW Configurations
export ROS_DOMAIN_ID=30
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp

# 3. Device Specific Roles
export RAI_DEVICE_ROLE=jetson
export RAI_DEVICE_LABEL=jetson_orin_nano

# 4. Optional: Wait for network to be fully up
sleep 2

# 5. Execute ROS 2 Launch
# Launches perception/estimator stack and the CCA-NMPC controller
ros2 launch rai_ccanmpc_controller jetson_runtime.launch.py
