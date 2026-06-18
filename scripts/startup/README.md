# Hướng dẫn Cài đặt Chạy Tự động (Autostart) ROS 2 cho RAI Robot

Thư mục này chứa các file cấu hình và shell script để khởi động hệ thống ROS 2 tự động khi bật nguồn cho cả 2 thiết bị: **Raspberry Pi 4** (đóng vai trò `pi`) và **Jetson Orin Nano** (đóng vai trò `jetson`).

Cả hai thiết bị đều chạy trên phiên bản **ROS 2 Humble**, chia sẻ chung cấu hình mạng với `ROS_DOMAIN_ID=30` và Middleware `rmw_cyclonedds_cpp`.

---

## 📂 Danh sách các file

1. **Phía Raspberry Pi 4 (`pi`):**
   - [pi_startup.sh](file:///home/rai/ijat2026/scripts/startup/pi_startup.sh): Shell script thiết lập biến môi trường và chạy `pi_runtime.launch.py`.
   - [pi_startup.service](file:///home/rai/ijat2026/scripts/startup/pi_startup.service): File cấu hình systemd service chạy ngầm.

2. **Phía Jetson Orin Nano (`jetson`):**
   - [jetson_startup.sh](file:///home/rai/ijat2026/scripts/startup/jetson_startup.sh): Shell script thiết lập biến môi trường và chạy `jetson_runtime.launch.py`.
   - [jetson_startup.service](file:///home/rai/ijat2026/scripts/startup/jetson_startup.service): File cấu hình systemd service chạy ngầm.

---

## 🚀 Các bước cài đặt trên từng thiết bị

Thực hiện các bước sau trên từng thiết bị tương ứng (Pi hoặc Jetson):

### Bước 1: Phân quyền thực thi cho file script `.sh`

Đảm bảo script khởi động có quyền chạy:

```bash
chmod +x /home/rai/ijat2026/scripts/startup/pi_startup.sh       # (Chạy trên Pi)
# hoặc
chmod +x /home/rai/ijat2026/scripts/startup/jetson_startup.sh   # (Chạy trên Jetson)
```

### Bước 2: Sao chép file `.service` vào thư mục hệ thống systemd

Sao chép file service vào thư mục cấu hình của hệ thống (`/etc/systemd/system/`):

```bash
sudo cp /home/rai/ijat2026/scripts/startup/pi_startup.service /etc/systemd/system/       # (Chạy trên Pi)
# hoặc
sudo cp /home/rai/ijat2026/scripts/startup/jetson_startup.service /etc/systemd/system/   # (Chạy trên Jetson)
```

### Bước 3: Nạp lại cấu hình daemon và kích hoạt Service

Chạy các lệnh sau để đăng ký service chạy cùng hệ thống:

```bash
# Nạp lại systemd daemon
sudo systemctl daemon-reload

# Kích hoạt service tự khởi động khi bật máy
sudo systemctl enable pi_startup.service       # (Trên Pi)
# hoặc
sudo systemctl enable jetson_startup.service   # (Trên Jetson)

# Khởi động service ngay lập tức (không cần reset máy)
sudo systemctl start pi_startup.service        # (Trên Pi)
# hoặc
sudo systemctl start jetson_startup.service    # (Trên Jetson)
```

---

## 🔍 Kiểm tra trạng thái hoạt động

Bạn có thể kiểm tra xem service của mình hoạt động bình thường hay không qua các lệnh sau:

### 1. Kiểm tra trạng thái (Status)
```bash
sudo systemctl status pi_startup.service       # (Trên Pi)
sudo systemctl status jetson_startup.service   # (Trên Jetson)
```

### 2. Xem log chi tiết thời gian thực
Lệnh này giúp bạn xem toàn bộ `stdout` và `stderr` được xuất ra từ ROS 2:
```bash
journalctl -u pi_startup.service -f            # (Trên Pi)
journalctl -u jetson_startup.service -f        # (Trên Jetson)
```

### 3. Dừng hoặc khởi động lại thủ công
Nếu bạn đang lập trình hoặc nâng cấp code và muốn tắt/bật lại service:
```bash
# Dừng service
sudo systemctl stop pi_startup.service

# Restart service
sudo systemctl restart pi_startup.service
```

## Sửa lỗi thường gặp

### Lỗi `AMENT_TRACE_SETUP_FILES: unbound variable`

Lỗi này xảy ra khi script chạy với `set -u` và source file setup của ROS 2 trong systemd. Hai script startup hiện đã dùng `safe_source` để tạm tắt `nounset` khi source:

```bash
safe_source /opt/ros/humble/setup.bash
safe_source /home/rai/ijat2026/rai_ros2/install/setup.bash
```

Sau khi cập nhật script, restart service:

```bash
sudo systemctl daemon-reload
sudo systemctl restart jetson_startup.service
sudo journalctl -u jetson_startup.service -f
```

Nếu service đang restart liên tục, dừng nó trước khi sửa/chạy lại:

```bash
sudo systemctl stop jetson_startup.service
sudo systemctl reset-failed jetson_startup.service
```

### Lỗi `package 'twist_mux' not found`

`twist_mux` cần có trên Pi để hợp nhất `/cmd_vel_web` và `/cca_nmpc/cmd_vel` thành `/cmd_vel`.

Cài trên Pi:

```bash
sudo apt update
sudo apt install ros-humble-twist-mux
```

Nếu package chưa được cài, `twist_mux.launch.py` sẽ bỏ qua node mux để không làm sập toàn bộ bringup. Tuy vậy, khi thiếu `twist_mux`, base driver vẫn nghe `/cmd_vel` nhưng web và controller đang publish vào topic nguồn riêng, nên robot sẽ chưa nhận được lệnh vận tốc qua mux.

Sau khi cài:

```bash
source /opt/ros/humble/setup.bash
source /home/rai/ijat2026/rai_ros2/install/setup.bash
ros2 launch turn_on_rai_robot twist_mux.launch.py
```
