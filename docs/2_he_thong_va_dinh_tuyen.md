# 2. Hệ thống và Định tuyến Thiết bị (Device Runtime & Routing)

Tài liệu này định nghĩa vai trò thiết bị, cách khởi chạy hệ thống trên **Raspberry Pi 4** và **Jetson Orin Nano**, các bộ điều khiển được đăng ký hỗ trợ, và luồng định tuyến lệnh vận tốc thông qua `twist_mux`.

---

## 1. Phân vai Thiết bị (Device Roles)

Hệ thống được thiết kế theo source tree chung nhưng phân chia chức năng chạy thực tế giữa hai máy tính nhúng:

* **Raspberry Pi 4 (Pi)**:
  * Trình điều khiển phần cứng (base driver).
  * Thu nhận dữ liệu cảm biến Lidar và IMU gốc.
  * Chạy Odometry, bộ lọc EKF, phát hành TF.
  * Chạy `rai_map_server`, định vị và các dịch vụ runtime cho `rai_navigation`.
  * Chạy node hợp nhất lệnh vận tốc `twist_mux`.
  * Chạy `rai_web_api` và quản lý việc ghi rosbag (dataset recording).

* **Jetson Orin Nano (Jetson)**:
  * Chạy mô hình camera RGB-D và YOLO26m (`best.pt`) phát hiện người.
  * Bộ ước lượng vết người tuyến tính (`KalmanTracker`).
  * Chạy bộ điều khiển tối ưu hóa chính `rai_controller_cca_nmpc` và bộ dự báo người `HumanPredictor`.

---

## 2. Bản đồ Mạng và Địa chỉ IP tham chiếu

* **Raspberry Pi 4**: `100.120.77.81` (mạng chung) hoặc `25.12.4.101` (mạng nội bộ).
* **Jetson Orin Nano**: `100.69.39.18` (mạng chung) hoặc `25.12.4.100` (mạng nội bộ).
* **Laptop người dùng**: `100.93.83.87` (mạng chung).

*Quy ước kết nối*: 
* Frontend của người dùng luôn gọi tới API chạy trên Pi (`http://100.120.77.81:8080`) làm đích chính.
* Đường truyền Ethernet nội bộ (`25.12.4.10x`) chỉ dùng cho truyền thông điệp ROS 2 tốc độ cao và trao đổi API nội bộ giữa Pi và Jetson.

---

## 3. Các bộ điều khiển được đăng ký trên hệ thống

Mã nguồn đăng ký các bộ điều khiển cho hai mục đích so sánh chính:

### 3.1 Bộ điều khiển so sánh chuẩn (Benchmark Controllers)
* **`CCA_NMPC`**: Thuật toán đề xuất với predictive continuous context adaptation đầy đủ.
* **`NMPC`**: Bộ điều khiển NMPC danh nghĩa cho xe Mecanum, không thích ứng ngữ cảnh.

Các bộ điều khiển dựa trên Nav2 như `DWA`, `DWB`, `TEB`, `MPPI` đã được gỡ khỏi workspace để tránh xung đột kiến trúc với `rai_navigation`.

### 3.2 Chuẩn tần số runtime
Toàn bộ pipeline benchmark được đồng bộ về cùng một nhịp thời gian để tránh xung đột giữa bộ điều khiển, perception và logging:
* **Control cycle**: `25 Hz`
* **CCA-NMPC sample time**: `T_s = 0.04 s`
* **EKF / odometry fusion**: `25 Hz`
* **RGB-D camera publish rate**: `25 Hz`
* **TF publish rate**: `25 Hz`
* **Planner expectation**: `25 Hz`

---

## 4. Quy trình Khởi chạy Hệ thống (Bringup)

### 4.1 Khởi động phía Raspberry Pi 4 (Thiết bị Pi)
Khởi động web runtime trên Pi. Theo kiến trúc mới, robot có thể đã bật nguồn nhưng **không tự chạy launch/node phần cứng**. Pi chỉ dựng `rai_web_api` để website điều khiển các thành phần độc lập:
```bash
source /opt/ros/humble/setup.bash
source ~/rai_ros2/install/setup.bash
export ROS_DOMAIN_ID=30
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export RAI_DEVICE_ROLE=pi
export RAI_DEVICE_LABEL=raspberry_pi_4

# Chỉ khởi chạy web/runtime stack
ros2 launch rai_web_api web_api.launch.py host:=0.0.0.0 port:=8080
```

Sau khi API lên, toàn bộ thao tác bật/tắt:
* Robot base: `turn_on_rai_robot.launch.py`
* LiDAR: `rai_lidar.launch.py`
* Camera: `rai_camera.launch.py`

được thực hiện từ trang `System` trên `rai_website`, không cần chạy tay trong terminal.

### 4.2 Khởi động phía Jetson Orin Nano (Thiết bị Jetson)
Jetson cũng chỉ dựng `rai_web_api` cục bộ để nhận lệnh điều khiển camera/perception từ website hoặc từ Pi thông qua proxy nội bộ:
```bash
source /opt/ros/humble/setup.bash
source ~/rai_ros2/install/setup.bash
export ROS_DOMAIN_ID=30
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export RAI_DEVICE_ROLE=jetson
export RAI_DEVICE_LABEL=jetson_orin_nano

# Khởi chạy web/API control surface trên Jetson
ros2 launch rai_web_api web_api.launch.py host:=0.0.0.0 port:=8080
```

---

## 5. Định tuyến Lệnh vận tốc qua `twist_mux`

Để đảm bảo robot không bị xung đột lệnh vận tốc giữa các nguồn điều khiển khác nhau:

* **`/cmd_vel_web` (độ ưu tiên cao)**: Lệnh lái thủ công từ Web Dashboard (teleop).
* **`/cca_nmpc/cmd_vel` (độ ưu tiên trung bình)**: Lệnh vận tốc tự động tính từ thuật toán CCA-NMPC trên Jetson.
* **`/cmd_vel`**: Lệnh vận tốc đầu ra duy nhất sau khi mux chọn lọc để truyền trực tiếp xuống bộ điều khiển động cơ base driver.
