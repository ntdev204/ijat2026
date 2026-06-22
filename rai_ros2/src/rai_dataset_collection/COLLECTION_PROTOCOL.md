# Dataset Collection Protocol - Manual S1-S6 CCA-NMPC

Map-based automatic execution is postponed until a validated map and reference scenario definitions exist. However, dataset collection still follows the required **S1-S6 protocol manually**.

## Required Scenarios

```text
S1_open_zone
S2_narrow_corridor
S3_human_proximate
S4_dynamic_crossing
S5_occlusion
S6_human_approaching
```

Target:

```text
S1_open_zone: 30 runs
S2_narrow_corridor: 30 runs
S3_human_proximate: 40 runs
S4_dynamic_crossing: 50 runs
S5_occlusion: 50 runs
S6_human_approaching: 60 runs
6 scenarios
260 runs/controller
```

## Required Realtime Signals

- `/canmpc/context`: JSON with `phi_h`, `d_h`, `d_safe`, `vx_max`, `vy_max`, `omega_max`.
- `/canmpc/humans`: JSON human states.
- `/cca_nmpc/context_input`: perception-side context input from the human predictor.
- `/cca_nmpc/humans`: tracked humans with confidence and covariance from the perception stack.
- `/canmpc/adaptive_bounds`: `[phi_h, d_safe, vx_max, vy_max, omega_max]`.
- `/perception/debug/tracks`, `/perception/debug/latency`: perception diagnostics.
- `/cmd_vel`: Mecanum command `[linear.x, linear.y, angular.z]`.
- `/odom`, `/tf`, `/scan_filtered`, camera/depth topics as available.

## Start One Run

```bash
./scripts/start_collection.sh \
  --scenario S4_dynamic_crossing \
  --controller CCA_NMPC \
  --environment real \
  --run-id run_000
```

## Observe While Recording

ROS:

```bash
ros2 topic echo /canmpc/context
ros2 topic echo /canmpc/humans
ros2 topic echo /cca_nmpc/context_input
ros2 topic echo /cca_nmpc/humans
ros2 topic echo /canmpc/adaptive_bounds
ros2 topic echo /perception/debug/tracks
```

Web API:

```text
GET /api/telemetry/current
GET /api/dataset/active
WS  /api/ws/telemetry
WS  /api/ws/dataset
```

## Stop and Download

```text
POST /api/dataset/stop
GET  /api/dataset/download/{run_id}
```

Download returns a `.zip` archive of the run directory. The archive can contain any bag storage format and derived files present in the run folder.

## Verify

```bash
python3 scripts/verify_dataset.py \
  --path ~/rai_datasets/canmpc \
  --environment real \
  --export
```
