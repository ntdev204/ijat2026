#!/bin/bash

safe_source() {
  set +u
  source "$1"
  set -u
}

setup_rai_ros_env() {
  safe_source "${ROS_SETUP_PATH:-/opt/ros/humble/setup.bash}"
  safe_source "${RAI_ROS_SETUP_PATH:-/home/rai/ijat2026/rai_ros2/install/setup.bash}"

  export ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-30}
  export RMW_IMPLEMENTATION=${RMW_IMPLEMENTATION:-rmw_cyclonedds_cpp}
  export ROS_LOCALHOST_ONLY=${ROS_LOCALHOST_ONLY:-0}
}

setup_rai_network_env() {
  export RAI_API_HOST=${RAI_API_HOST:-0.0.0.0}
  export RAI_API_PORT=${RAI_API_PORT:-8080}
  export RAI_SERVER_HOST=${RAI_SERVER_HOST:-100.116.199.115}
  export RAI_PI_AGENT_URL=${RAI_PI_AGENT_URL:-http://100.120.77.81:8080}
  export RAI_JETSON_AGENT_URL=${RAI_JETSON_AGENT_URL:-http://100.69.39.18:8080}
  export RAI_LAN_HOST=${RAI_LAN_HOST:-$RAI_SERVER_HOST}
  export RAI_API_CORS=${RAI_API_CORS:-http://localhost:3000,http://${RAI_SERVER_HOST}:3000}
}
