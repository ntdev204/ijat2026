# 04 - Chạy dataset theo kịch bản

Tài liệu này mô tả cách chạy dataset từ dashboard hoặc API cho các kịch bản S1-S6. Mục tiêu là tạo rosbag, metadata và artefact phân tích đủ để so sánh CCA-NMPC với NMPC.

## 1. Dataset ghi những gì

Một run dataset ghi:

- rosbag2 của các topic sensor/robot/context;
- metadata JSON của run;
- index CSV tổng hợp run;
- trạng thái controller, human context, adaptive bounds, solver stats;
- artefact hậu xử lý: CSV, metrics, plots, tables.

Các topic quan trọng:

| Nhóm | Topic |
| --- | --- |
| Robot | `/odom`, `/odom_combined`, `/cmd_vel`, `/joint_states`, `/wheel_encoders`, `/voltage` |
| Sensor | `/scan`, `/scan_filtered`, `/imu/data_raw`, camera RGB-D topics |
| CCA-NMPC | `/canmpc/context`, `/canmpc/humans`, `/canmpc/adaptive_bounds`, `/canmpc/local_reference_path`, `/canmpc/predicted_trajectory`, `/canmpc/solver_stats` |
| Perception | `/cca_nmpc/context_input`, `/human_perception/context_input`, `/cca_nmpc/humans` |
| Map/costmap | `/local_costmap/costmap`, `/local_costmap/published_footprint` |

## 2. Cấu trúc thư mục dataset

Mặc định `RAI_DATASET_PATH` là `~/ijat2026/dataset`. Metadata/DB do `rai_web_api` trên hub quản lý, còn process `ros2 bag record` chạy qua `rai_runtime_bridge` trên Pi.

Trong robot thật, nên mount/sync `RAI_DATASET_PATH` để hub và Pi cùng nhìn thấy cùng một cây thư mục. Nếu không dùng shared path, rosbag sẽ được ghi trên Pi theo path hub gửi xuống, còn bước zip/download trên hub cần đồng bộ dữ liệu về sau run.

```text
dataset/
├── raw/<environment>/<scenario>/<controller>/<run_id>/
│   ├── rosbag2/
│   ├── rosbag_record.log
│   └── metadata.json
├── derived/
├── metadata/
│   └── run_index.csv
├── figures/
├── tables/
└── videos/
```

Một run thực tế có dạng:

```text
dataset/raw/real/S4_dynamic_crossing/CCA_NMPC/run_001/
```

## 3. Controllers cần so sánh

| ID | Ý nghĩa | Nhóm |
| --- | --- | --- |
| `CCA_NMPC` | Continuous Context-Aware NMPC, dùng $\phi_h$ và adaptive bounds | proposed |
| `NMPC` | Nominal Mecanum NMPC không adaptation theo human context | ablation |

Khi thu thập dữ liệu so sánh, giữ cùng scenario, start/goal, environment và human protocol; chỉ đổi controller.

## 4. Danh sách kịch bản S1-S6

| Scenario | Tên | Mục tiêu | Số run tối thiểu |
| --- | --- | --- | --- |
| `S1_open_zone` | Open Zone | Tracking trong môi trường ít rủi ro người | 30 |
| `S2_narrow_corridor` | Narrow Corridor | Kiểm tra hành vi ngang Mecanum và clearance hẹp | 30 |
| `S3_human_proximate` | Human Proximate | Đo $\phi_h$ và $d_{\mathrm{safe}}$ khi người đứng/gần robot | 40 |
| `S4_dynamic_crossing` | Dynamic Crossing | Người cắt ngang đường robot, kiểm tra slowdown dự báo | 50 |
| `S5_occlusion` | Occlusion | Người bị che khuất rồi xuất hiện muộn, kiểm tra confidence/covariance | 50 |
| `S6_human_approaching` | Human Approaching | Người tiến gần/đi xa robot để kiểm tra hướng tương đối | 60 |

## 5. Metadata cần nhập trước mỗi run

Trên trang Dataset của dashboard:

| Field | Cách dùng |
| --- | --- |
| Scenario | Chọn `S1_open_zone` ... `S6_human_approaching` |
| Controller | `CCA_NMPC` hoặc `NMPC` |
| Environment | `real` hoặc `sim` |
| Split | `train`, `val`, `test` hoặc `unsplit` |
| Run index | nên dùng `001`, `002`, ... để dễ đối chiếu |
| Random seed | dùng khi chạy sim hoặc cần tái lập |
| Human behavior | mô tả người đi ngang, đứng gần, approaching, occlusion |
| Start/Goal | nhập pose bắt đầu và đích nếu kịch bản yêu cầu |
| Record RGB-D topics | bật khi cần camera topics trong rosbag |
| Intervention | đánh dấu nếu operator can thiệp |
| Notes | ghi lỗi, lệch protocol, vật cản, pin, ánh sáng |

## 6. Quy trình chạy bằng dashboard

1. Bật hệ thống theo [03 - Vận hành hệ thống](./03_van_hanh_he_thong.md).
2. Vào trang Dataset.
3. Chọn metadata run.
4. Bấm `Launch dataset stack` nếu stack chưa chạy.
5. Bấm `Prepare folders`.
6. Đưa robot/người vào đúng vị trí đầu kịch bản.
7. Bấm `Start run`.
8. Thực hiện kịch bản theo protocol S1-S6.
9. Bấm `Stop run` khi hoàn tất.
10. Chạy pipeline `all` hoặc từng bước `validate`, `bag_to_csv`, `metrics`, `plots`, `tables`.
11. Ghi nhận nếu run lỗi và không dùng run đó cho thống kê chính.

## 7. Quy trình chạy bằng API

Chuẩn bị thư mục:

```bash
curl -X POST http://100.116.199.115:8080/api/dataset/prepare
```

Bật launch stack:

```bash
curl -X POST http://100.116.199.115:8080/api/dataset/launch/start \
  -H 'Content-Type: application/json' \
  -d '{
    "scenario_name": "S4_dynamic_crossing",
    "controller_id": "CCA_NMPC",
    "environment": "real",
    "split": "unsplit",
    "run_id": "run_001",
    "auto_start": true
  }'
```

Bắt đầu run:

```bash
curl -X POST http://100.116.199.115:8080/api/dataset/start \
  -H 'Content-Type: application/json' \
  -d '{
    "scenario_name": "S4_dynamic_crossing",
    "controller_id": "CCA_NMPC",
    "environment": "real",
    "run_index": 1,
    "split": "unsplit",
    "human_behavior": "left_to_right_crossing",
    "record_camera": true,
    "notes": "baseline lighting, no intervention"
  }'
```

Dừng run:

```bash
curl -X POST http://100.116.199.115:8080/api/dataset/stop
```

Chạy pipeline:

```bash
curl -X POST http://100.116.199.115:8080/api/dataset/pipeline \
  -H 'Content-Type: application/json' \
  -d '{"action":"all","run_id":1}'
```

## 8. Launch dataset bằng ROS 2 trực tiếp

Trên Pi:

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

Launch này bật hardware layer, context monitor và dataset collector theo thứ tự delay.

## 9. Protocol thực nghiệm theo kịch bản

### S1 - Open Zone

- Không có người hoặc người ở xa hành lang robot.
- Dùng để kiểm tra tracking nominal, sai số RMSE và ổn định solver.
- Chạy trước để xác nhận robot, odom, map và controller hoạt động.

### S2 - Narrow Corridor

- Robot chạy trong corridor/hành lang hẹp.
- Có thể có người đứng bên cạnh hoặc di chuyển song song chậm.
- Quan sát $v_y$, $d_h$, $d_{\mathrm{safe}}$, costmap và va chạm cạnh.

### S3 - Human Proximate

- Một hoặc hai người đứng/gần đường đi robot.
- Mục tiêu là $\phi_h$ tăng mượt khi robot lại gần.
- Không để người đổi hướng quá nhanh; giữ protocol lặp lại được.

### S4 - Dynamic Crossing

- Người cắt ngang đường robot từ trái sang phải hoặc ngược lại.
- Ghi rõ hướng crossing trong `human_behavior`.
- Mục tiêu là robot giảm tốc trước điểm giao cắt.

### S5 - Occlusion

- Người bị che bởi góc tường/kệ/vật cản rồi xuất hiện muộn.
- Ghi rõ loại occluder và thời điểm xuất hiện.
- Dùng để kiểm tra confidence, covariance và fallback an toàn.

### S6 - Human Approaching

- Người đi trực diện về phía robot rồi đi ra xa ở khoảng cách tương tự.
- Mục tiêu là chứng minh ảnh hưởng của hướng tương đối qua $\cos(\Delta\theta)$.
- Chạy nhiều lần nhất vì đây là scenario critical.

## 10. Kiểm tra chất lượng run

Một run dùng được khi:

- metadata có scenario/controller/environment/run_id đúng;
- rosbag2 tồn tại và có kích thước tăng trong lúc ghi;
- `GET /api/dataset/launch/status` có `record.running=true` trong lúc ghi;
- `/canmpc/context` có mẫu $\phi_h$, $d_h$, $d_{\mathrm{safe}}$;
- `/canmpc/solver_stats` có solve time;
- `metadata/run_index.csv` có dòng tương ứng;
- không có intervention ngoài kế hoạch, hoặc đã đánh dấu `intervention=true`.

Loại hoặc đánh dấu run nếu:

- robot không chạy đúng kịch bản;
- người thực hiện sai protocol;
- pin yếu làm robot giảm hiệu năng;
- camera/LiDAR mất dữ liệu;
- Web/API lỗi làm start/stop không khớp thời gian thực tế.
