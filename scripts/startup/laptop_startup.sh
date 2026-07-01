#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/common_env.sh"

setup_rai_ros_env
setup_rai_network_env

export RAI_DEVICE_ROLE=${RAI_DEVICE_ROLE:-hub}
export RAI_DEVICE_LABEL=${RAI_DEVICE_LABEL:-laptop_hub}

ros2 launch rai_web_api web_api.launch.py \
  host:=${RAI_API_HOST} \
  lan_host:=${RAI_LAN_HOST} \
  port:=${RAI_API_PORT}
