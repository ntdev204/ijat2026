# RAI Device Runtime, Launch Routing, and Web Control Rules

Tài liệu này định nghĩa cách chạy cùng một workspace ROS2 trên `Raspberry Pi 4` và `Jetson Orin Nano` mà không bị gọi nhầm launch, node, hay API.

## 0. Địa chỉ triển khai hiện tại

Bảng này chỉ để tham chiếu vận hành thực tế. Nó không thay thế rule phân vai thiết bị và không kéo theo yêu cầu setup IP thủ công trong tài liệu.

| Thiết bị | Network | IP |
|---|---|---|
| Raspberry Pi 4 | chung | `100.120.77.81` |
| Jetson Orin Nano | chung | `100.69.39.18` |
| Laptop | chung | `100.93.83.87` |
| Raspberry Pi 4 | Ethernet nội bộ Pi-Jetson | `25.12.4.101` |
| Jetson Orin Nano | Ethernet nội bộ Pi-Jetson | `25.12.4.100` |

Quy ước dùng trong runtime:

- `Pi-side API endpoint` cho frontend chạy trên laptop: `http://100.120.77.81:8080`
- `Pi-side API endpoint` cho giao tiếp nội bộ Pi-Jetson qua Ethernet: `http://25.12.4.101:8080`
- `Jetson-side runtime`: `25.12.4.100` là địa chỉ Ethernet nội bộ của Jetson để debug hoặc kiểm tra node phía Jetson khi cần.
- `Laptop`: dùng IP `100.93.83.87` ở mạng chung khi chạy frontend hoặc terminal giám sát từ máy người dùng.

## 1. Nguyên tắc chung

Repo là một source tree chung, nhưng runtime phải tách theo vai trò thiết bị:

- `Pi`:
  - base driver
  - lidar
  - IMU / odom / EKF / TF
  - localization
  - Nav2 map / planner / costmap
  - `twist_mux`
  - `rai_web_api`
  - dataset recording
- `Jetson`:
  - perception stack
  - human estimation
  - `rai_ccanmpc_controller`

Rule gốc:

- `Pi` không launch `rai_ccanmpc_controller`
- `Jetson` không launch `rai_nav2`
- web UI mặc định chỉ trỏ vào API trên `Pi`

## 2. Launch wrapper mới

### Pi wrapper

File:

- [pi_runtime.launch.py](/home/rai/ijat2026/rai_ros2/src/turn_on_rai_robot/launch/pi_runtime.launch.py)

Chức năng:

- set `RAI_DEVICE_ROLE=pi`
- set `RAI_DEVICE_LABEL=raspberry_pi_4`
- launch `prod_bringup.launch.py`
- tùy chọn launch thêm `rai_nav2.launch.py`

Chạy:

```bash
source /opt/ros/humble/setup.bash
source /home/rai/ijat2026/rai_ros2/install/setup.bash
export ROS_DOMAIN_ID=30
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
ros2 launch turn_on_rai_robot pi_runtime.launch.py with_nav2:=true
```

Nếu chỉ cần hardware + web API + twist mux, chưa cần Nav2:

```bash
ros2 launch turn_on_rai_robot pi_runtime.launch.py with_nav2:=false
```

### Jetson wrapper

File:

- [jetson_runtime.launch.py](/home/rai/ijat2026/rai_ros2/src/rai_ccanmpc_controller/launch/jetson_runtime.launch.py)

Chức năng:

- set `RAI_DEVICE_ROLE=jetson`
- set `RAI_DEVICE_LABEL=jetson_orin_nano`
- launch `rai_ccanmpc_controller.launch.py`

Chạy:

```bash
source /opt/ros/humble/setup.bash
source /home/rai/ijat2026/rai_ros2/install/setup.bash
export ROS_DOMAIN_ID=30
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
ros2 launch rai_ccanmpc_controller jetson_runtime.launch.py
```

## 3. Command routing sau khi thêm twist_mux

Luồng vận tốc hiện tại:

- web teleop: `/cmd_vel_web`
- controller CCA-NMPC: `/cca_nmpc/cmd_vel`
- `twist_mux`: chọn nguồn và xuất `/cmd_vel`
- base driver: chỉ nghe `/cmd_vel`

File liên quan:

- [twist_mux.yaml](/home/rai/ijat2026/rai_ros2/src/turn_on_rai_robot/config/twist_mux.yaml)
- [twist_mux.launch.py](/home/rai/ijat2026/rai_ros2/src/turn_on_rai_robot/launch/twist_mux.launch.py)

Pi cần có package:

```bash
sudo apt install ros-humble-twist-mux
```

## 4. Rule phân luồng web

### Rule điều hướng chính

Web UI trong vận hành bình thường phải dùng:

```text
Pi-side API endpoint
```

Trong mô hình hiện tại:

- nếu `rai_website` chạy trên laptop, endpoint đúng là `http://100.120.77.81:8080`
- nếu một tool chạy trực tiếp trong mạng Ethernet nội bộ Pi-Jetson, có thể dùng `http://25.12.4.101:8080`

Không dùng Jetson API làm API mặc định cho frontend.

Lý do:

- `Pi` là nơi giữ Nav2, map, slam, dataset, twist mux, base command
- `Jetson` chỉ giữ controller/perception
- nếu frontend trỏ nhầm Jetson thì các thao tác Pi-only sẽ sai ngữ cảnh

### Khi nào mới trỏ vào Jetson

Chỉ dùng Jetson API cho debug hoặc maintenance nếu sau này bạn chủ động chạy thêm `rai_web_api` ở Jetson.

Ngay cả khi trỏ nhầm, API bây giờ sẽ tự chặn các action không hợp role.

## 5. API role guard

`rai_web_api` bây giờ có runtime identity:

- `device_role`
- `device_label`
- `allowed_actions`

Endpoint:

```text
GET /api/system/runtime
GET /api/health
```

Role mặc định:

- `pi` cho phép:
  - `teleop`
  - `nav2`
  - `slam`
  - `dataset`
  - `maps`
- `jetson` cho phép:
  - `controller`

Các endpoint Pi-only hiện đã bị chặn cứng nếu API đang chạy với role `jetson`:

- `/api/nav2/*`
- `/api/robot/slam/*`
- `/api/map/*`
- `/api/dataset/*`
- `/api/robot/cmd_vel`
- `/api/ws/control`
- `/api/ws/map`
- `/api/ws/dataset`

Nếu gọi sai role, API trả về `403` với message rõ ràng.

## 6. Rule thao tác theo thiết bị

### Trên Pi

Được phép:

- launch `turn_on_rai_robot pi_runtime.launch.py`
- launch Nav2
- launch web API
- record dataset
- chạy SLAM
- teleop từ web

Không được phép:

- launch `jetson_runtime.launch.py`
- chạy `rai_ccanmpc_controller` trực tiếp để thay cho Jetson runtime chính thức

### Trên Jetson

Được phép:

- launch `rai_ccanmpc_controller jetson_runtime.launch.py`
- chạy perception / estimator stack

Không được phép:

- launch Nav2 runtime
- chạy SLAM runtime
- record dataset chính
- điều khiển base qua web API như API mặc định vận hành

## 7. Rule đặt terminal và shell

Nên tạo shell profile riêng cho từng máy:

### Pi shell

```bash
export ROS_DOMAIN_ID=30
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export RAI_DEVICE_ROLE=pi
export RAI_DEVICE_LABEL=raspberry_pi_4
export RAI_DATASET_PATH=/home/rai/ijat2026/dataset
source /opt/ros/humble/setup.bash
source /home/rai/ijat2026/rai_ros2/install/setup.bash
```

### Jetson shell

```bash
export ROS_DOMAIN_ID=30
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export RAI_DEVICE_ROLE=jetson
export RAI_DEVICE_LABEL=jetson_orin_nano
source /opt/ros/humble/setup.bash
source /home/rai/ijat2026/rai_ros2/install/setup.bash
```

## 8. Rule giao diện người dùng

Frontend bây giờ nên hiển thị rõ:

- `API target: raspberry_pi_4 (pi)` hoặc
- `API target: jetson_orin_nano (jetson)`

Các nút Pi-only sẽ tự disable nếu frontend đang nối tới API không đúng vai trò.

Hiện các trang bị ảnh hưởng:

- `Monitor`: Nav2 control
- `Map`: SLAM control
- `Dataset Collection`: dataset pipeline

## 9. Quy trình vận hành chuẩn

### Trường hợp chạy thật

1. Trên `Pi`:

```bash
ros2 launch turn_on_rai_robot pi_runtime.launch.py with_nav2:=true
```

2. Trên `Jetson`:

```bash
ros2 launch rai_ccanmpc_controller jetson_runtime.launch.py
```

3. Trên máy chạy frontend:

```bash
cd /home/rai/ijat2026/rai_website
npm run dev
```

Nếu frontend dev server chạy trên laptop như mô hình hiện tại, cấu hình nên là Pi-side API endpoint ở mạng chung:

```bash
NEXT_PUBLIC_API_URL=http://100.120.77.81:8080 npm run dev
```

Ethernet nội bộ `25.12.4.101` dành cho liên kết Pi-Jetson hoặc tool nội bộ, không phải endpoint mặc định cho laptop frontend.

4. Kiểm tra ở UI:

- banner role phải hiện `raspberry_pi_4 (pi)`
- trang `Map` cho phép SLAM
- trang `Monitor` cho phép Nav2
- trang `Dataset` cho phép record

Nếu banner hiện `jetson_orin_nano (jetson)` thì không dùng phiên web đó cho vận hành chính.

## 10. Rule tránh gọi nhầm

Đây là 3 rule bắt buộc nên giữ:

1. Chỉ có `Pi` mới là API mặc định cho frontend.
2. Chỉ launch bằng wrapper theo thiết bị, không gọi launch rời rạc trong vận hành thường ngày.
3. Chỉ có `twist_mux` mới được xuất `/cmd_vel` xuống base.

## 11. Checklist nhanh

Trước khi vận hành:

- `Pi`:
  - `ros2 node list` có `twist_mux`
  - `ros2 node list` có `rai_web_api`
  - nếu cần Nav2: có stack Nav2
- `Jetson`:
  - `ros2 node list` có `rai_ccanmpc_controller`
- frontend:
  - frontend phải trỏ tới Pi-side API endpoint, không trỏ Jetson
  - UI hiển thị role là `pi`

## 12. File liên quan

- Pi wrapper: [pi_runtime.launch.py](/home/rai/ijat2026/rai_ros2/src/turn_on_rai_robot/launch/pi_runtime.launch.py)
- Jetson wrapper: [jetson_runtime.launch.py](/home/rai/ijat2026/rai_ros2/src/rai_ccanmpc_controller/launch/jetson_runtime.launch.py)
- Web API launch: [web_api.launch.py](/home/rai/ijat2026/rai_ros2/src/rai_web_api/launch/web_api.launch.py)
- Role guard backend: [main.py](/home/rai/ijat2026/rai_ros2/src/rai_web_api/rai_web_api/main.py)
- Web cmd publisher: [node.py](/home/rai/ijat2026/rai_ros2/src/rai_web_api/rai_web_api/node.py)
- Dataset/experiment guide: [rai_ccanmpc_dataset_experiment_guide.md](/home/rai/ijat2026/docs/rai_ccanmpc_dataset_experiment_guide.md)
