# Server Robot Runtime Guide

Tài liệu này mô tả cách chạy toàn bộ hệ thống với **server/hub** tại `100.116.199.115` làm điểm vào chính.

---

## 1. Mục tiêu

- Bật website trên server.
- Bật hub API trên server.
- Cấu hình `ROS_DOMAIN_ID` để DDS thấy được Pi và Jetson.
- Cấu hình các endpoint HTTP giữa server, Pi, và Jetson.
- Cho phép điều khiển robot từ website.

---

## 2. Địa chỉ mặc định

- Server / Hub: `100.116.199.115`
- Pi: `100.120.77.81`
- Jetson: `100.69.39.18`

---

## 3. Cấu hình ROS 2

Trên tất cả máy:

```bash
export ROS_DOMAIN_ID=30
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_LOCALHOST_ONLY=0
```

Nếu dùng shell đăng nhập tự động, nên đưa các dòng này vào `~/.bashrc`.

---

## 4. Khởi động server / hub

### 4.1 PostgreSQL

```bash
cd ~/ijat2026
make db-up
```

### 4.2 Hub API

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

### 4.3 Website

```bash
cd ~/ijat2026/rai_website
npm install
npm run dev
```

Mở:

```text
http://100.116.199.115:3000
```

Nếu chạy production:

```bash
npm run build
npm run start -- --hostname 0.0.0.0 --port 3000
```

`.env.local` phải chứa:

```bash
NEXT_PUBLIC_API_URL=http://100.116.199.115:8080
NEXT_PUBLIC_PI_API_URL=http://100.120.77.81:8080
NEXT_PUBLIC_JETSON_API_URL=http://100.69.39.18:8080
```

---

## 5. Khởi động Pi

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

Sau đó vào website để bật:

- Robot base
- LiDAR
- Camera
- SLAM hoặc navigation

---

## 6. Khởi động Jetson

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

---

## 7. Trình tự chạy khuyến nghị

1. Bật PostgreSQL trên server.
2. Bật hub API trên server.
3. Bật website trên server.
4. Bật API trên Jetson.
5. Bật API trên Pi.
6. Từ website, điều khiển robot base, LiDAR, camera, SLAM, navigation.

---

## 8. Kiểm tra kết nối

```bash
ros2 topic list
ros2 node list
```

Nếu website không thấy dữ liệu:

- Kiểm tra `ROS_DOMAIN_ID=30` trên cả 3 máy.
- Kiểm tra `ROS_LOCALHOST_ONLY=0`.
- Kiểm tra Pi và Jetson có ping HTTP được từ server.
- Kiểm tra Pi API và Jetson API đều đang listen cổng `8080`.

---

## 9. Ghi chú vận hành

- Server là điểm vào duy nhất cho người dùng.
- Pi và Jetson vẫn cần chạy `rai_web_api` riêng để website proxy được đúng chức năng.
- Không nên hardcode IP mới ở nhiều nơi; ưu tiên biến môi trường `NEXT_PUBLIC_*` và `RAI_*`.
