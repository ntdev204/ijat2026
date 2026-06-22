#!/bin/bash
# ==============================================================================
# RAI Project - Jetson Orin Nano Autostart Script
# ==============================================================================
set -euo pipefail

# 1. Core ROS 2 & RMW Configurations
export ROS_DOMAIN_ID=30
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp

# ROS setup scripts read optional variables that may be unset under systemd.
safe_source() {
  set +u
  source "$1"
  set -u
}

# 2. Environment Setup
safe_source /opt/ros/humble/setup.bash
safe_source /home/rai/ijat2026/rai_ros2/install/setup.bash

# 3. Device Specific Roles
export RAI_DEVICE_ROLE=jetson
export RAI_DEVICE_LABEL=jetson_orin_nano
export RAI_PI_API_URL=${RAI_PI_API_URL:-http://25.12.4.101:8080}

# 4. Optional: Wait for network to be fully up
sleep 2

# 5. Execute ROS 2 Launch
# Only start the Jetson-side web/API control surface. Camera and perception launch from the website.
ros2 launch rai_web_api web_api.launch.py host:=0.0.0.0 port:=8080
