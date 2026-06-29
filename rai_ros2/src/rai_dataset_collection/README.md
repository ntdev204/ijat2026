# rai_dataset_collection

Package này ghi dataset cho thực nghiệm CCA-NMPC: rosbag2, metadata run, context samples, human states, adaptive bounds và solver stats.

## Chạy qua dashboard

Khuyến nghị vận hành qua dashboard ở trang Dataset. Quy trình chi tiết nằm trong:

- [docs/04_chay_dataset_theo_kich_ban.md](../../../docs/04_chay_dataset_theo_kich_ban.md)

## Chạy trực tiếp bằng ROS 2

```bash
source /opt/ros/humble/setup.bash
source ~/ijat2026/rai_ros2/install/setup.bash

ros2 launch rai_dataset_collection dataset_collection.launch.py \
  scenario:=S4_dynamic_crossing \
  controller:=CCA_NMPC \
  environment:=real \
  split:=unsplit \
  run_id:=run_001 \
  auto_start:=true
```

## Scenario IDs

- `S1_open_zone`
- `S2_narrow_corridor`
- `S3_human_proximate`
- `S4_dynamic_crossing`
- `S5_occlusion`
- `S6_human_approaching`

## Controllers

- `CCA_NMPC`: controller đề xuất có human context adaptation.
- `NMPC`: nominal ablation không adaptation theo human context.
