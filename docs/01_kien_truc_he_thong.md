# 01 - Kiến trúc hệ thống

Tài liệu này mô tả cách các thiết bị trong hệ thống RAI Robot phối hợp với nhau khi chạy web dashboard, ROS 2 runtime, CCA-NMPC navigation và dataset collection.

## 1. Vai trò thiết bị

| Thiết bị | Role (`RAI_DEVICE_ROLE`) | Trách nhiệm chính |
| --- | --- | --- |
| Server / laptop hub | `hub` hoặc `laptop` | Điểm vào của website, API điều phối, PostgreSQL metadata, proxy lệnh xuống Pi/Jetson |
| Raspberry Pi 4 | `pi` | Base driver, LiDAR, IMU/odom, SLAM, navigation, dataset recording |
| Jetson Orin Nano | `jetson` | Camera RGB-D, perception, camera runtime |
| Simulation machine | `sim` | Chạy mô phỏng Gazebo/RViz khi không dùng robot thật |

Trong triển khai hiện tại, cùng package `rai_web_api` chạy trên nhiều thiết bị. Hành vi của API phụ thuộc vào `RAI_DEVICE_ROLE`.

## 2. Địa chỉ mạng mặc định

| Thành phần | URL |
| --- | --- |
| Website | `http://100.116.199.115:3000` |
| Hub API | `http://100.116.199.115:8080` |
| Pi API agent | `http://100.120.77.81:8080` |
| Jetson API agent | `http://100.69.39.18:8080` |

Các địa chỉ này được đặt trong `scripts/startup/common_env.sh`:

```bash
export RAI_SERVER_HOST=${RAI_SERVER_HOST:-100.116.199.115}
export RAI_PI_AGENT_URL=${RAI_PI_AGENT_URL:-http://100.120.77.81:8080}
export RAI_JETSON_AGENT_URL=${RAI_JETSON_AGENT_URL:-http://100.69.39.18:8080}
export RAI_API_CORS=${RAI_API_CORS:-http://localhost:3000,http://${RAI_SERVER_HOST}:3000}
```

## 3. ROS 2 network chuẩn

Tất cả máy cần cùng domain DDS:

```bash
export ROS_DOMAIN_ID=30
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
export ROS_LOCALHOST_ONLY=0
```

Nếu một máy đặt `ROS_LOCALHOST_ONLY=1`, máy đó chỉ thấy node ROS 2 local và sẽ mất liên lạc DDS với robot.

## 4. Luồng request từ website

```text
Browser
  -> rai_website:3000
  -> rai_web_api hub:8080
       -> Pi rai_web_api:8080      robot / lidar / slam / navigation / dataset
       -> Jetson rai_web_api:8080  camera / perception
```

REST API chung đi qua `NEXT_PUBLIC_API_URL`. WebSocket có thể đi trực tiếp về Pi cho telemetry/map/control stream nếu `NEXT_PUBLIC_PI_API_URL` được cấu hình.

## 5. Role và quyền hành động

`rai_web_api` giới hạn hành động theo role:

| Role | Nhóm hành động |
| --- | --- |
| `pi` | `teleop`, `navigation`, `slam`, `dataset`, `maps`, `simulation`, `system`, `hardware`, `lidar` |
| `jetson` | `controller`, `system`, `camera` |
| `hub` / `laptop` | Điều phối đầy đủ qua agent URL xuống Pi/Jetson |
| `sim` | `teleop`, `navigation`, `slam`, `maps`, `simulation`, `system` |

Khi một action không thuộc role hiện tại, API trả lỗi 403 để tránh launch nhầm phần cứng trên sai máy.

## 6. Thành phần ROS 2 chính

| Thành phần | Package/launch | Máy thường chạy |
| --- | --- | --- |
| Robot base | `turn_on_rai_robot.launch.py` | Pi |
| LiDAR | `rai_lidar.launch.py` | Pi |
| Camera | `rai_camera.launch.py` | Jetson |
| SLAM | `online_async_launch.py` | Pi hoặc sim |
| Navigation | `rai_navigation.launch.py` | Pi hoặc sim |
| Dataset launch stack | `dataset_collection.launch.py` | Pi |
| Web API | `rai_web_api web_api.launch.py` | Hub, Pi, Jetson |
| Website | `npm run dev` / `npm run start` | Hub |

## 7. Dữ liệu và lưu trữ

| Dữ liệu | Mặc định |
| --- | --- |
| Workspace | `~/ijat2026` hoặc `RAI_WORKSPACE_ROOT` |
| ROS 2 map | `rai_ros2/data/map/` |
| Simulation worlds | `rai_ros2/data/worlds/` |
| Dataset | `~/ijat2026/dataset` hoặc `RAI_DATASET_PATH` |
| DB metadata | PostgreSQL trong `docker-compose.yml` |

## 8. Thứ tự bật khuyến nghị

1. PostgreSQL trên hub.
2. Hub API.
3. Website.
4. Jetson API agent.
5. Pi API agent.
6. Runtime components từ dashboard: camera, robot base, LiDAR, SLAM/navigation, dataset stack.
