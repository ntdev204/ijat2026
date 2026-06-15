# Manual S1-S5 Scenarios - CCA-NMPC Dataset

No validated map is available yet, so these scenarios are executed manually. The scenario labels are still mandatory for dataset organization.

## S1_open_zone

Open space, no close human.

Goal: keep `phi_h` low and collect nominal Mecanum tracking.

## S2_narrow_corridor

Drive through a corridor-like aisle or constrained passage.

Goal: collect clearance behavior and lateral Mecanum corrections.

## S3_human_proximate

Drive near one standing or slow-moving human while keeping safe distance.

Goal: collect smooth increase of `phi_h`, `d_safe`, and reduced adaptive velocity bounds.

## S4_dynamic_crossing

A human crosses the robot path.

Goal: collect human velocity estimation and robot slowdown/side-step response.

## S5_occlusion_sudden_appearance

A human appears from behind an occluding object or corner.

Goal: collect reaction time, occlusion flag, fallback stop/slow behavior.

## Manual Start Example

```bash
ros2 launch rai_dataset_collection dataset_collection.launch.py \
  scenario:=S4_dynamic_crossing \
  controller:=CCA_NMPC \
  environment:=real \
  run_id:=run_000 \
  auto_start:=true
```

## Per-Run Notes

Record in notes or lab log:

- Scenario label S1-S5.
- Controller id.
- Human actor behavior and approximate speed.
- Any intervention or emergency stop.
- Any tracking loss, occlusion, near miss or collision.
