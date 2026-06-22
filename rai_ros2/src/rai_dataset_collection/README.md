# rai_dataset_collection

Manual dataset collection package for **Continuous Context-Aware NMPC (CCA-NMPC)** on the Rai Mecanum robot.

There is no validated map yet, so scenario execution is **manual**, not automatic. The dataset protocol still requires the six S1-S6 scenarios.

The current perception stack uses a fine-tuned `YOLO26m` model from local
weights such as `best.pt`, then bridges tracked humans into the controller.

## Required Manual Scenarios

| Scenario | Manual intent |
|---|---|
| `S1_open_zone` | Open area, no close human, low `phi_h` |
| `S2_narrow_corridor` | Corridor/aisle-like run, clearance and lateral behavior |
| `S3_human_proximate` | Pass near a standing or slow person |
| `S4_dynamic_crossing` | A person crosses the robot path |
| `S5_occlusion` | A person appears from an occluded area |
| `S6_human_approaching` | A person approaches then recedes to expose directional context effects |

Target:

- `S1_open_zone`: 30 runs
- `S2_narrow_corridor`: 30 runs
- `S3_human_proximate`: 40 runs
- `S4_dynamic_crossing`: 50 runs
- `S5_occlusion`: 50 runs
- `S6_human_approaching`: 60 runs

## What It Records

- Mecanum command data: `linear.x`, `linear.y`, `angular.z`.
- Continuous context: `phi_h`, `d_h`, `d_safe`.
- Adaptive bounds: `vx_max`, `vy_max`, `omega_max`.
- Human state estimates: `x`, `y`, `vx`, `vy`, detector confidence, covariance trace.
- Perception context input: nearest human distance, relative speed, human count, tracking quality.
- Raw sensors, odom, TF, local costmap, reference path, predicted trajectory, solver stats.
- Perception debug streams: image, depth, tracks, latency.

## Start One Manual Run

```bash
ros2 launch rai_dataset_collection dataset_collection.launch.py \
  scenario:=S4_dynamic_crossing \
  controller:=CCA_NMPC \
  environment:=real \
  run_id:=run_000 \
  auto_start:=true
```

## Dataset Layout

```text
~/rai_datasets/canmpc/
└── raw/
    └── real/
        └── S4_dynamic_crossing/
            └── CCA_NMPC/
                └── run_000/
                    ├── rosbag2/
                    └── metadata.json
```

## Verify

```bash
python3 scripts/verify_dataset.py \
  --path ~/rai_datasets/canmpc \
  --environment real
```
