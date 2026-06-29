#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/common_env.sh"

setup_rai_ros_env
setup_rai_network_env

export RAI_DEVICE_ROLE=${RAI_DEVICE_ROLE:-pi}
export RAI_DEVICE_LABEL=${RAI_DEVICE_LABEL:-raspberry_pi_4}

sleep 2

ros2 launch rai_runtime_bridge bridge.launch.py \
  role:=${RAI_DEVICE_ROLE} \
  host:=${RAI_BRIDGE_HOST} \
  port:=${RAI_BRIDGE_PORT}
