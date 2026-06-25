# 2. Hệ thống và Định tuyến Thiết bị (Device Runtime & Routing)

Tài liệu này chuẩn hóa cấu hình mạng hiện tại của hệ thống:

- **Server / Hub**: `100.116.199.115`
- **Raspberry Pi 4**: `100.120.77.81` hoặc `25.12.4.101`
- **Jetson Orin Nano**: `100.69.39.18` hoặc `25.12.4.100`

Server là điểm vào chính cho `rai_website` và `rai_web_api` dạng hub. Pi vẫn là nơi chạy runtime robot, SLAM, navigation và dataset. Jetson vẫn là nơi chạy camera/perception.

---

## 1. Vai trò thiết bị

### Raspberry Pi 4

- Driver phần cứng base.
- LiDAR, IMU, odometry, EKF, TF.
- `twist_mux`.
- `rai_web_api` local cho robot runtime.
- SLAM, navigation, map server, dataset recording.

### Jetson Orin Nano

- Camera / RGB-D processing.
- `rai_human_perception`.
- YOLO + TensorRT + Kalman tracking.
- `rai_web_api` local cho camera/perception.

### Server / Hub

- `rai_web_api` ở vai trò `hub`.
- `rai_website` ở cổng `3000`.
- PostgreSQL cho metadata dataset.
- RViz / teleop / tooling nếu cần.

---

## 2. Định tuyến mạng

- Website người dùng truy cập: `http://100.116.199.115:3000`
- Hub API: `http://100.116.199.115:8080`
- Pi API: `http://100.120.77.81:8080`
- Jetson API: `http://100.69.39.18:8080`

Đường Ethernet nội bộ `25.12.4.10x` được dùng cho ROS 2 tốc độ cao giữa Pi và Jetson. Hub không cần nằm trên cùng Ethernet nội bộ, nhưng phải truy cập được HTTP tới Pi và Jetson.

---

## 3. ROS 2 network chuẩn

Tất cả thiết bị phải dùng cùng cấu hình DDS:

```bash
export ROS_DOMAIN_ID=30
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_LOCALHOST_ONLY=0
```

Nếu một máy đặt `ROS_LOCALHOST_ONLY=1` thì nó sẽ không giao tiếp DDS với robot.

---

## 4. Bringup theo thiết bị

### 4.1 Raspberry Pi 4

```bash
source /opt/ros/humble/setup.bash
source ~/rai_ros2/install/setup.bash

export ROS_DOMAIN_ID=30
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_LOCALHOST_ONLY=0
export RAI_DEVICE_ROLE=pi
export RAI_DEVICE_LABEL=raspberry_pi_4
export RAI_JETSON_API_URL=http://25.12.4.100:8080

ros2 launch rai_web_api web_api.launch.py host:=0.0.0.0 port:=8080
```

Sau khi Pi API lên:

- `turn_on_rai_robot.launch.py`
- `rai_lidar.launch.py`
- `rai_camera.launch.py`

có thể được bật từ trang `System` trên website.

### 4.2 Jetson Orin Nano

```bash
source /opt/ros/humble/setup.bash
source ~/rai_ros2/install/setup.bash

export ROS_DOMAIN_ID=30
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_LOCALHOST_ONLY=0
export RAI_DEVICE_ROLE=jetson
export RAI_DEVICE_LABEL=jetson_orin_nano
export RAI_PI_API_URL=http://25.12.4.101:8080

ros2 launch rai_web_api web_api.launch.py host:=0.0.0.0 port:=8080
```

### 4.3 Server / Hub

```bash
source /opt/ros/humble/setup.bash
source ~/ijat2026/rai_ros2/install/setup.bash

export ROS_DOMAIN_ID=30
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_LOCALHOST_ONLY=0
export RAI_DEVICE_ROLE=hub
export RAI_DEVICE_LABEL=ubuntu_server_hub
export RAI_PI_API_URL=http://100.120.77.81:8080
export RAI_JETSON_API_URL=http://100.69.39.18:8080

ros2 launch rai_web_api web_api.launch.py host:=0.0.0.0 port:=8080
```

Website trên server dùng:

```bash
NEXT_PUBLIC_API_URL=http://100.116.199.115:8080
NEXT_PUBLIC_PI_API_URL=http://100.120.77.81:8080
NEXT_PUBLIC_JETSON_API_URL=http://100.69.39.18:8080
```

---

## 5. Định tuyến lệnh vận tốc

- `/cmd_vel_web`: teleop từ web, ưu tiên cao nhất.
- `/cca_nmpc/cmd_vel`: lệnh điều khiển tự động.
- `twist_mux` xuất `/cmd_vel`: lệnh cuối cùng đi xuống base driver.

---

## 6. Thứ tự khởi động khuyến nghị

1. Server: PostgreSQL.
2. Server: `rai_web_api` hub.
3. Server: `rai_website`.
4. Jetson: `rai_web_api`.
5. Pi: `rai_web_api`.
6. Từ website: bật robot base, LiDAR, camera, perception, SLAM hoặc navigation.

Hướng dẫn thao tác chi tiết xem tại `docs/device_launch_guide.md`.
