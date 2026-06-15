# Setup Guide - CCA-NMPC Dataset Collection

## Purpose

Collect datasets for **Continuous Context-Adaptive CA-NMPC** on a Rai Mecanum robot.

The active flow records continuous context and Mecanum commands:

- `phi_h`, `d_h`, `d_safe`.
- `vx_max`, `vy_max`, `omega_max`.
- Human state `[x, y, vx, vy]`.
- Robot command `[linear.x, linear.y, angular.z]`.

## Build

```bash
cd ~/rai_ros2
colcon build --packages-select rai_dataset_collection rai_web_api
source install/setup.bash
```

## Launch Collection

```bash
ros2 launch rai_dataset_collection dataset_collection.launch.py \
  scenario:=S1_open_zone \
  controller:=CCA_NMPC \
  environment:=real \
  run_id:=run_000 \
  auto_start:=true
```

## Check Runtime Topics

```bash
ros2 topic echo /canmpc/context
ros2 topic echo /canmpc/humans
ros2 topic echo /canmpc/adaptive_bounds
ros2 topic echo /cmd_vel
```

## Verify Dataset

```bash
python3 src/rai_dataset_collection/scripts/verify_dataset.py \
  --path ~/rai_datasets/canmpc \
  --environment real \
  --min-runs 20
```

## Web API

Start API:

```bash
ros2 run rai_web_api web_api
```

Useful endpoints:

- `GET /api/health`
- `GET /api/dataset/scenarios`
- `GET /api/telemetry/current`
- `GET /api/dataset/active`
- `POST /api/dataset/start`
- `POST /api/dataset/stop`
- `GET /api/dataset/runs`
- `WS /api/ws/telemetry`
- `WS /api/ws/dataset`
- `GET /api/dataset/download/{run_id}`
