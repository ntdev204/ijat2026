# 03 - Vận hành hệ thống

Tài liệu này là checklist chạy hệ thống từ terminal hoặc systemd. Các lệnh giả định repo nằm tại `~/ijat2026` trên Linux devices.

## 1. Điều kiện trước khi chạy

Trên các máy ROS 2:

```bash
source /opt/ros/humble/setup.bash
source ~/ijat2026/rai_ros2/install/setup.bash

export ROS_DOMAIN_ID=30
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_LOCALHOST_ONLY=0
```

Trên hub cần có Node.js dependencies cho `rai_website` và Docker/Compose cho PostgreSQL.
Nếu ghi dataset robot thật, cấu hình `RAI_DATASET_PATH` sao cho hub và Pi cùng truy cập được hoặc có bước sync thư mục run từ Pi về hub sau khi ghi.

## 2. Bật PostgreSQL trên hub

```bash
cd ~/ijat2026
make db-up
make db-ps
```

Xem log DB:

```bash
make db-logs
```

Mở shell PostgreSQL:

```bash
make db-shell
```

## 3. Chạy hub API

```bash
cd ~/ijat2026
source /opt/ros/humble/setup.bash
source ~/ijat2026/rai_ros2/install/setup.bash

export ROS_DOMAIN_ID=30
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_LOCALHOST_ONLY=0

export RAI_DEVICE_ROLE=hub
export RAI_DEVICE_LABEL=laptop_hub
export RAI_API_HOST=0.0.0.0
export RAI_API_PORT=8080
export RAI_SERVER_HOST=100.116.199.115
export RAI_LAN_HOST=$RAI_SERVER_HOST
export RAI_PI_BRIDGE_URL=http://100.120.77.81:8090
export RAI_JETSON_BRIDGE_URL=http://100.69.39.18:8090
export RAI_API_CORS=http://localhost:3000,http://${RAI_SERVER_HOST}:3000
export RAI_DATASET_PATH=${RAI_DATASET_PATH:-$HOME/ijat2026/dataset}

ros2 launch rai_web_api web_api.launch.py host:=${RAI_API_HOST} lan_host:=${RAI_LAN_HOST} port:=${RAI_API_PORT}
```

Kiểm tra:

```bash
curl http://100.116.199.115:8080/api/health
curl http://100.116.199.115:8080/api/system/runtime
```

## 4. Chạy website

```bash
cd ~/ijat2026/rai_website
npm install
cat > .env.local <<'EOF'
NEXT_PUBLIC_API_URL=http://100.116.199.115:8080
NEXT_PUBLIC_ALLOWED_DEV_ORIGINS=100.116.199.115
EOF
npm run dev
```

Mở:

```text
http://100.116.199.115:3000
```

Production:

```bash
npm run build
npm run start -- --hostname 0.0.0.0 --port 3000
```

## 5. Chạy Pi runtime bridge

```bash
cd ~/ijat2026
source /opt/ros/humble/setup.bash
source ~/ijat2026/rai_ros2/install/setup.bash

export ROS_DOMAIN_ID=30
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_LOCALHOST_ONLY=0

export RAI_DEVICE_ROLE=pi
export RAI_DEVICE_LABEL=raspberry_pi_4
export RAI_BRIDGE_HOST=0.0.0.0
export RAI_BRIDGE_PORT=8090
export RAI_SERVER_HOST=100.116.199.115
export RAI_LAN_HOST=$RAI_SERVER_HOST
export RAI_DATASET_PATH=${RAI_DATASET_PATH:-$HOME/ijat2026/dataset}

ros2 launch rai_runtime_bridge bridge.launch.py role:=pi host:=${RAI_BRIDGE_HOST} port:=${RAI_BRIDGE_PORT}
```

Pi chịu trách nhiệm launch:

- robot base;
- LiDAR;
- SLAM;
- navigation;
- map save/load;
- dataset launch/recording.

## 6. Chạy Jetson runtime bridge

```bash
cd ~/ijat2026
source /opt/ros/humble/setup.bash
source ~/ijat2026/rai_ros2/install/setup.bash

export ROS_DOMAIN_ID=30
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_LOCALHOST_ONLY=0

export RAI_DEVICE_ROLE=jetson
export RAI_DEVICE_LABEL=jetson_orin_nano
export RAI_BRIDGE_HOST=0.0.0.0
export RAI_BRIDGE_PORT=8090
export RAI_SERVER_HOST=100.116.199.115
export RAI_LAN_HOST=$RAI_SERVER_HOST

ros2 launch rai_runtime_bridge bridge.launch.py role:=jetson host:=${RAI_BRIDGE_HOST} port:=${RAI_BRIDGE_PORT}
```

Jetson chịu trách nhiệm camera/perception runtime.

## 7. Dùng scripts startup có sẵn

Các script trong `scripts/startup/` tự source `common_env.sh` và set role:

```bash
# Hub
bash scripts/startup/laptop_startup.sh

# Pi
bash scripts/startup/pi_startup.sh

# Jetson
bash scripts/startup/jetson_startup.sh
```

Sửa IP mặc định trong `scripts/startup/common_env.sh` hoặc override bằng biến môi trường trước khi chạy.

## 8. Cài systemd service cho Pi/Jetson

Trên Pi:

```bash
chmod +x /home/rai/ijat2026/scripts/startup/pi_startup.sh
sudo cp /home/rai/ijat2026/scripts/startup/pi_startup.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable pi_startup.service
sudo systemctl start pi_startup.service
sudo systemctl status pi_startup.service
journalctl -u pi_startup.service -f
```

Trên Jetson:

```bash
chmod +x /home/rai/ijat2026/scripts/startup/jetson_startup.sh
sudo cp /home/rai/ijat2026/scripts/startup/jetson_startup.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable jetson_startup.service
sudo systemctl start jetson_startup.service
sudo systemctl status jetson_startup.service
journalctl -u jetson_startup.service -f
```

## 9. Khởi động runtime từ dashboard

Vào trang System hoặc runtime controls trên website:

1. Kiểm tra API health.
2. Bật Camera trên Jetson nếu cần RGB-D/perception.
3. Bật Robot Base trên Pi.
4. Bật LiDAR trên Pi.
5. Bật SLAM hoặc chọn map/navigation.
6. Bật Dataset Launch Stack khi chuẩn bị ghi dataset.

## 10. Kiểm tra nhanh bằng API

```bash
curl http://100.116.199.115:8080/api/system/components
curl http://100.116.199.115:8080/api/dataset/launch/status
curl http://100.116.199.115:8080/api/rai-navigation/config
```

Nếu gọi trực tiếp Pi/Jetson:

```bash
curl http://100.120.77.81:8090/api/health
curl http://100.69.39.18:8090/api/health
```
