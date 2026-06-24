#!/bin/bash



set -euo pipefail


export ROS_DOMAIN_ID=30
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp


safe_source() {
  set +u
  source "$1"
  set -u
}


safe_source /opt/ros/humble/setup.bash
safe_source /home/rai/ijat2026/rai_ros2/install/setup.bash


export RAI_DEVICE_ROLE=jetson
export RAI_DEVICE_LABEL=jetson_orin_nano
export RAI_PI_API_URL=${RAI_PI_API_URL:-http://25.12.4.101:8080}
export RAI_API_CORS=${RAI_API_CORS:-http://100.77.136.102:3000,http://localhost:3000}


sleep 2



ros2 launch rai_web_api web_api.launch.py host:=0.0.0.0 port:=8080
