#!/bin/bash



set -euo pipefail

safe_source() {
  set +u
  source "$1"
  set -u
}

safe_source /opt/ros/humble/setup.bash
safe_source /home/rai/ijat2026/rai_ros2/install/setup.bash

export ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-30}
export RMW_IMPLEMENTATION=${RMW_IMPLEMENTATION:-rmw_cyclonedds_cpp}
export RAI_DEVICE_ROLE=${RAI_DEVICE_ROLE:-hub}
export RAI_DEVICE_LABEL=${RAI_DEVICE_LABEL:-laptop_hub}
export RAI_PI_API_URL=${RAI_PI_API_URL:-http://100.120.77.81:8080}
export RAI_JETSON_API_URL=${RAI_JETSON_API_URL:-http://100.69.39.18:8080}
export RAI_API_CORS=${RAI_API_CORS:-http://100.77.136.102:3000,http://localhost:3000}
export RAI_LAN_HOST=${RAI_LAN_HOST:-100.77.136.102}

ros2 launch rai_web_api web_api.launch.py host:=0.0.0.0 port:=8080
