# RAI CCA-NMPC Dataset, Metrics, and Experiment Guide

Hướng dẫn này mô tả cách thu thập rosbag, tạo cấu trúc dataset, chạy pipeline số liệu, và tổ chức thực nghiệm cho bộ điều khiển `rai_ccanmpc_controller`.

Tài liệu companion cho phân vai thiết bị và rule web/API:

- [rai_device_runtime_and_web_routing.md](/home/rai/ijat2026/docs/rai_device_runtime_and_web_routing.md)

Địa chỉ đang dùng trong triển khai hiện tại:

- Pi: `100.120.77.81`
- Jetson: `100.69.39.18`
- Laptop: `100.93.83.87`
- Pi Ethernet nội bộ: `25.12.4.101`
- Jetson Ethernet nội bộ: `25.12.4.100`

## 1. Mục tiêu hệ thống

Kiến trúc đang dùng:

- `Nav2` chỉ giữ vai trò `map`, `TF`, `localization`, `global planner/path`, `costmap`.
- `rai_ccanmpc_controller` là controller độc lập, không chạy bên trong `controller_server`.
- `rai_web_api` cung cấp web/API để start-stop dataset run, record rosbag, và gọi pipeline hậu xử lý.

Node controller:

- Package: `rai_ccanmpc_controller`
- Executable: `rai_ccanmpc_controller_node`
- Launch file: [rai_ccanmpc_controller.launch.py](/home/rai/ijat2026/rai_ros2/src/rai_ccanmpc_controller/launch/rai_ccanmpc_controller.launch.py)
- Tên node: `rai_ccanmpc_controller`

Arbitration cho lệnh vận tốc:

- `twist_mux` là node duy nhất xuất `/cmd_vel` cho base driver.
- `rai_web_api` publish vào `/cmd_vel_web`
- `rai_ccanmpc_controller` publish vào `/cca_nmpc/cmd_vel`
- `twist_mux` chọn nguồn và xuất ra `/cmd_vel`

## 2. Topic và precondition

Controller publish/subscribe theo cấu hình hiện tại tại [rai_ccanmpc_controller.yaml](/home/rai/ijat2026/rai_ros2/src/rai_ccanmpc_controller/config/rai_ccanmpc_controller.yaml):

- `odom_topic`: `/odom_combined`
- `costmap_topic`: `/local_costmap/costmap`
- `goal_topic`: `/goal_pose`
- `external_path_topic`: `/canmpc/global_path`
- `plan_topic`: `/plan`
- `cmd_vel_topic`: `/cca_nmpc/cmd_vel`

Topic dataset bắt buộc:

- `/scan`
- `/scan_filtered`
- `/odom`
- `/imu/data_raw`
- `/tf`
- `/tf_static`
- `/cmd_vel`
- `/cmd_vel_web`
- `/cca_nmpc/cmd_vel`
- `/canmpc/context`
- `/canmpc/humans`
- `/canmpc/adaptive_bounds`
- `/canmpc/predicted_trajectory`
- `/canmpc/local_reference_path`
- `/canmpc/solver_stats`
- `/local_costmap/costmap`
- `/local_costmap/published_footprint`

Topic camera tùy chọn:

- `/camera/color/image_raw`
- `/camera/depth/image_rect_raw`
- `/camera/aligned_depth_to_color/image_raw`
- `/camera/camera_info`

Precondition quan trọng:

- Khi đánh giá controller, pose robot, human state, reference path và costmap phải được quy về cùng một frame điều khiển trước khi trích metric.
- Trong cấu hình hiện tại, `control_frame` là `odom_combined`.

## 3. Cấu trúc dataset

Root mặc định:

```text
/home/rai/ijat2026/dataset
```

Có thể override bằng biến môi trường:

```bash
export RAI_DATASET_PATH=/path/to/dataset
```

Cấu trúc đang được `rai_web_api` và scripts tạo tự động:

```text
dataset/
  raw/{sim,real}/...
  derived/
  metadata/
  figures/
  tables/
```

Các file schema và metadata mẫu đã có sẵn:

- [dataset/metadata/sensors.yaml](/home/rai/ijat2026/dataset/metadata/sensors.yaml)
- [dataset/metadata/robot_mecanum.yaml](/home/rai/ijat2026/dataset/metadata/robot_mecanum.yaml)
- [dataset/metadata/controllers.yaml](/home/rai/ijat2026/dataset/metadata/controllers.yaml)
- [dataset/metadata/scenarios.yaml](/home/rai/ijat2026/dataset/metadata/scenarios.yaml)
- [dataset/metadata/run_index.csv](/home/rai/ijat2026/dataset/metadata/run_index.csv)

## 4. Build và bringup

Build workspace:

```bash
cd /home/rai/ijat2026/rai_ros2
colcon build --symlink-install
source install/setup.bash
```

Bringup tối thiểu để chạy controller:

Trong vận hành thực tế nên ưu tiên wrapper theo thiết bị:

- `Pi`: [pi_runtime.launch.py](/home/rai/ijat2026/rai_ros2/src/turn_on_rai_robot/launch/pi_runtime.launch.py)
- `Jetson`: [jetson_runtime.launch.py](/home/rai/ijat2026/rai_ros2/src/rai_ccanmpc_controller/launch/jetson_runtime.launch.py)

1. Bringup robot sensor/odom/TF.
2. Chạy localization + map server + planner + costmap của Nav2.
3. Chạy controller độc lập:

```bash
ros2 launch rai_ccanmpc_controller rai_ccanmpc_controller.launch.py
```

4. Chạy web API:

```bash
ros2 launch rai_web_api web_api.launch.py
```

Ghi chú:

- Launch của `rai_web_api` đã thêm `PYTHONPATH` và `LD_LIBRARY_PATH` cho package `rai_ccanmpc_controller`.
- `rai_web_api` mặc định chạy ở `0.0.0.0:8080`.
- `turn_on_rai_robot` hiện launch thêm `twist_mux` để hợp nhất `/cmd_vel_web` và `/cca_nmpc/cmd_vel`.
- Trên máy Pi cần cài `twist_mux` nếu hệ thống chưa có:

```bash
sudo apt install ros-humble-twist-mux
```

## 5. Web workflow

Màn dataset UI hiện nằm trong web dashboard của `rai_website`.

Luồng dùng chuẩn:

1. Mở trang `Dataset Collection`.
2. Bấm `Prepare folders`.
3. Nhập metadata run:
   - `Scenario`
   - `Controller`
   - `Environment`
   - `Split`
   - `Run index`
   - `Random seed`
   - `Human behavior`
   - `Start x/y/theta`
   - `Goal x/y/theta`
   - `Intervention`
   - `Record RGB-D topics` nếu cần camera
4. Bấm `Start bag`.
5. Thực hiện trial.
6. Bấm `Stop`.
7. Chạy pipeline:
   - `Validate`
   - `Bag to CSV`
   - `Metrics`
   - `Figures`
   - `Tables`
   - hoặc `Run all`

Khi start run, `rai_web_api` sẽ:

- tạo thư mục run dưới `dataset/raw/{environment}/{scenario}/{controller}/{run_id}`
- tạo `metadata.json`
- cập nhật `dataset/metadata/run_index.csv`
- chạy `ros2 bag record`

Lưu ý về command topics khi record:

- `/cmd_vel` là lệnh sau mux, đi thật xuống base
- `/cmd_vel_web` là lệnh teleop từ web
- `/cca_nmpc/cmd_vel` là lệnh nguồn từ controller

Khi stop run, `rai_web_api` sẽ:

- dừng `ros2 bag record` bằng `SIGINT`
- cập nhật `end_time`, `duration_sec`
- zip thư mục run ở background

## 6. API workflow

Các endpoint chính:

- `GET /api/dataset/artifacts`
- `POST /api/dataset/prepare`
- `POST /api/dataset/start`
- `GET /api/dataset/active`
- `POST /api/dataset/stop`
- `GET /api/dataset/runs`
- `POST /api/dataset/pipeline`
- `GET /api/dataset/download/{run_id}`

Ví dụ start run:

```bash
curl -X POST http://localhost:8080/api/dataset/start \
  -H 'Content-Type: application/json' \
  -d '{
    "scenario_name": "S2_crossing_human",
    "controller_id": "CCA_NMPC",
    "environment": "real",
    "split": "test",
    "run_index": 1,
    "random_seed": 42,
    "robot_start_x": 0.0,
    "robot_start_y": 0.0,
    "robot_start_theta": 0.0,
    "goal_x": 4.0,
    "goal_y": 0.0,
    "goal_theta": 0.0,
    "human_behavior": "crossing",
    "intervention": false,
    "record_camera": false,
    "notes": "baseline trial"
  }'
```

Ví dụ chạy full pipeline:

```bash
curl -X POST http://localhost:8080/api/dataset/pipeline \
  -H 'Content-Type: application/json' \
  -d '{"action":"all","run_id":1}'
```

## 7. CLI workflow

Nếu không dùng web, có thể chạy scripts trực tiếp:

```bash
cd /home/rai/ijat2026
python3 scripts/dataset/validate_bag.py --dataset dataset --bag dataset/raw/real/.../rosbag2
python3 scripts/dataset/bag_to_csv.py --dataset dataset --bag dataset/raw/real/.../rosbag2
python3 scripts/dataset/build_run_index.py --dataset dataset
python3 scripts/dataset/extract_metrics.py --dataset dataset
python3 scripts/plot/plot_timeseries.py --dataset dataset
python3 scripts/plot/plot_trajectories.py --dataset dataset
python3 scripts/plot/plot_boxplots.py --dataset dataset
python3 scripts/plot/plot_latency.py --dataset dataset
python3 scripts/tables/make_latex_tables.py --dataset dataset
```

## 8. Thực trạng pipeline hiện tại

Những phần đã hoạt động:

- Tạo cấu trúc dataset
- Tạo schema CSV/metadata
- Start-stop `ros2 bag record` từ web/API
- Lưu `metadata.json`
- Cập nhật `run_index.csv`
- Validate thư mục bag và `metadata.yaml`
- Tạo output placeholder cho figures/tables

Những phần hiện mới là scaffold, chưa phải extractor hoàn chỉnh:

- `bag_to_csv.py`
- `extract_controller_timeseries.py`
- `extract_human_states.py`
- `extract_metrics.py`
- các script plotting

Điều đó có nghĩa là:

- Flow thu thập run đã dùng được.
- Flow hậu xử lý đã có khung lệnh và output path chuẩn.
- Muốn có metric paper thật từ rosbag thì cần nối parser `rosbag2_py` hoặc converter tương đương vào các script extractor.

## 9. Thiết kế thực nghiệm khuyến nghị

Các scenario đang seed trong metadata:

- `S1_open_zone`
- `S2_crossing_human`
- `S3_corridor`
- `S4_occlusion`
- `S5_dense_dynamic`

Controller nên so sánh:

- `CCA_NMPC`
- `DWB`
- `MPPI`
- `TEB`
- `MANUAL`

Khuyến nghị tối thiểu cho mỗi scenario:

- `sim`: 10-20 run/controller
- `real`: 5-10 run/controller

Metadata nên cố định cho từng trial:

- `random_seed`
- `robot_start_*`
- `goal_*`
- `human_behavior`
- `environment`
- `notes` nếu có bất thường

Định nghĩa outcome mỗi run:

- `success`: tới goal trong điều kiện chấp nhận
- `collision`: có va chạm người/vật cản
- `timeout`: không đạt goal trong thời gian cho phép
- `intervention`: có tác động tay người vận hành

## 10. Metric paper nên báo cáo

Từ schema hiện tại, bộ metric mục tiêu gồm:

- Tracking:
  - `rmse_xy`
  - `rmse_theta`
  - `max_lateral_error`
- Safety:
  - `d_min`
  - `d_avg`
  - `d_5percentile`
  - `violation_count`
  - `violation_duration`
  - `collision_count`
- Smoothness:
  - `jerk_mean`
  - `jerk_max`
  - `mean_abs_delta_u`
  - `max_abs_delta_u`
- Control effort:
  - `control_effort`
  - `mean_abs_vx`
  - `mean_abs_vy`
  - `mean_abs_omega`
- Real-time:
  - `solve_time_mean_ms`
  - `solve_time_median_ms`
  - `solve_time_p95_ms`
  - `solve_time_max_ms`
  - `timeout_rate`

Đối với CCA predictive control, nên plot thêm:

- `phi_h`
- `d_h`
- `d_safe`
- `vx_max_adaptive`
- `vy_max_adaptive`
- `omega_max_adaptive`
- trạng thái solver và timeout

## 11. Checklist trước khi record

Trước mỗi batch run:

1. Xác nhận `source install/setup.bash`.
2. Kiểm tra `ros2 topic list` có đủ topic controller và context.
3. Kiểm tra `/canmpc/context`, `/canmpc/humans`, `/canmpc/solver_stats` có dữ liệu.
4. Kiểm tra controller đang publish `/cca_nmpc/cmd_vel`.
5. Kiểm tra `dataset/metadata/run_index.csv` ghi được.
6. Nếu record camera, kiểm tra tốc độ ghi bag và dung lượng lưu trữ.

## 12. Troubleshooting

`Start bag` lỗi ngay:

- thường do `rai_web_api` không chạy trong ROS environment đã source
- kiểm tra lệnh `ros2 bag record` có chạy được bằng tay không

Web hiện được nhưng không record:

- xem file `rosbag_record.log` trong thư mục run
- kiểm tra quyền ghi tại `RAI_DATASET_PATH`

Có run nhưng metrics trống:

- hiện tại extractor mới là scaffold
- cần bổ sung parser rosbag2 để đổ dữ liệu thật vào `derived/*.csv`

Controller có path nhưng robot không đi:

- kiểm tra `/odom_combined`, `/local_costmap/costmap`, `/plan` hoặc `/canmpc/global_path`
- kiểm tra `control_frame` và frame transform

## 13. File liên quan

- Web/API runtime: [main.py](/home/rai/ijat2026/rai_ros2/src/rai_web_api/rai_web_api/main.py)
- Web dataset page: [page.tsx](/home/rai/ijat2026/rai_website/src/app/(dashboard)/dataset/page.tsx)
- Controller launch/config: [rai_ccanmpc_controller.launch.py](/home/rai/ijat2026/rai_ros2/src/rai_ccanmpc_controller/launch/rai_ccanmpc_controller.launch.py), [rai_ccanmpc_controller.yaml](/home/rai/ijat2026/rai_ros2/src/rai_ccanmpc_controller/config/rai_ccanmpc_controller.yaml)
- Dataset scripts: [scripts/dataset](/home/rai/ijat2026/scripts/dataset), [scripts/plot](/home/rai/ijat2026/scripts/plot), [scripts/tables](/home/rai/ijat2026/scripts/tables)
