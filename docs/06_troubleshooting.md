# 06 - Troubleshooting

Tài liệu này liệt kê các lỗi thường gặp khi chạy RAI Robot, dashboard và dataset collection.

## 1. Không thấy node ROS 2 giữa các máy

Triệu chứng:

- `ros2 node list` chỉ thấy node local.
- Dashboard không có telemetry/map.
- Navigation báo thiếu odom/map/scan.

Kiểm tra trên mọi máy:

```bash
echo $ROS_DOMAIN_ID
echo $RMW_IMPLEMENTATION
echo $ROS_LOCALHOST_ONLY
```

Giá trị đúng:

```bash
ROS_DOMAIN_ID=30
RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
ROS_LOCALHOST_ONLY=0
```

Nếu vẫn lỗi:

- kiểm tra cùng mạng/VPN/Tailscale;
- kiểm tra firewall UDP DDS;
- đảm bảo đã source đúng `install/setup.bash`;
- không chạy nhiều domain ROS 2 khác nhau.

## 2. API hub không gọi được runtime bridge Pi/Jetson

Triệu chứng:

- `/api/system/components` thiếu component từ Pi/Jetson.
- Start robot/camera từ dashboard trả lỗi không reach runtime bridge.

Kiểm tra từ hub:

```bash
curl http://100.120.77.81:8090/api/health
curl http://100.69.39.18:8090/api/health
```

Kiểm tra biến môi trường hub:

```bash
echo $RAI_PI_BRIDGE_URL
echo $RAI_JETSON_BRIDGE_URL
```

Giá trị đúng theo cấu hình mặc định:

```bash
RAI_PI_BRIDGE_URL=http://100.120.77.81:8090
RAI_JETSON_BRIDGE_URL=http://100.69.39.18:8090
```

## 3. Dashboard bị CORS

Triệu chứng:

- Browser console báo CORS.
- `curl` API được nhưng browser không gọi được.

Sửa trên máy chạy `rai_web_api`:

```bash
export RAI_API_CORS=http://localhost:3000,http://100.116.199.115:3000
```

Nếu dashboard chạy host khác, thêm origin đó vào danh sách, phân tách bằng dấu phẩy.

## 4. WebSocket disconnected

Triệu chứng:

- REST API OK nhưng telemetry/map/control trên dashboard disconnected.
- Trang Monitor không cập nhật realtime.

Kiểm tra `.env.local` của `rai_website`:

```bash
NEXT_PUBLIC_API_URL=http://100.116.199.115:8080
```

Kiểm tra trực tiếp từ browser/máy vận hành:

```text
ws://100.116.199.115:8080/api/ws/telemetry
```

Nếu dùng reverse proxy riêng cho WebSocket, set `NEXT_PUBLIC_WS_URL`.

## 5. Systemd service không chạy

Kiểm tra trạng thái:

```bash
sudo systemctl status pi_startup.service
sudo systemctl status jetson_startup.service
```

Xem log:

```bash
journalctl -u pi_startup.service -f
journalctl -u jetson_startup.service -f
```

Lỗi thường gặp:

| Lỗi | Cách xử lý |
| --- | --- |
| `setup.bash: No such file` | kiểm tra `ROS_SETUP_PATH`, `RAI_ROS_SETUP_PATH` trong `common_env.sh` |
| `permission denied` | `chmod +x scripts/startup/*.sh` |
| API/bridge bind fail | kiểm tra port `8080` trên hub hoặc `8090` trên Pi/Jetson đã bị process khác chiếm chưa |
| node crash loop | đọc log journal, chạy script thủ công để thấy stacktrace |

## 6. PostgreSQL không lên

Kiểm tra:

```bash
make db-ps
make db-logs
```

Restart:

```bash
make db-restart
```

Nếu schema/dataset API lỗi DB, đảm bảo `make db-up` đã chạy trước hub API.

## 7. Dataset không start được

Triệu chứng:

- `Start run` trả 409.
- Run trước vẫn active.
- `ros2 bag record is already running`.

Kiểm tra:

```bash
curl http://100.116.199.115:8080/api/dataset/active
curl http://100.116.199.115:8080/api/dataset/launch/status
curl http://100.120.77.81:8090/api/dataset/record/status
```

Dừng run đang active:

```bash
curl -X POST http://100.116.199.115:8080/api/dataset/stop
```

Dừng launch stack:

```bash
curl -X POST http://100.116.199.115:8080/api/dataset/launch/stop
```

Nếu vẫn lỗi, kiểm tra process `ros2 bag` hoặc `dataset_collector` trên Pi. Nếu hub báo download/zip lỗi sau khi record thành công, kiểm tra `RAI_DATASET_PATH` có phải shared path giữa hub và Pi không, hoặc đồng bộ thư mục run từ Pi về hub trước khi tải artefact.

## 8. Dataset thiếu topic

Triệu chứng:

- Pipeline validate fail.
- CSV/metrics thiếu dữ liệu.
- $\phi_h$, solver stats hoặc camera topic trống.

Kiểm tra topic trong lúc ghi:

```bash
ros2 topic list | grep canmpc
ros2 topic hz /canmpc/context
ros2 topic hz /canmpc/solver_stats
ros2 topic hz /scan_filtered
```

Nếu bật camera:

```bash
ros2 topic list | grep camera
```

Không đánh dấu run là hợp lệ nếu topic bắt buộc bị thiếu.

## 9. Navigation không chạy

Kiểm tra config:

```bash
curl http://100.116.199.115:8080/api/rai-navigation/config
```

Các lỗi phổ biến:

| Lỗi | Cách xử lý |
| --- | --- |
| Không có map | chọn hoặc save map trước khi start navigation |
| Params file không tồn tại | kiểm tra `RAI_NAVIGATION_PARAMS` |
| Odom stale | kiểm tra `/odom_combined` và base driver |
| Missing plan | kiểm tra global planner, map, start/goal |
| Solver timeout | giảm tốc, kiểm tra costmap/human context quá nhiễu |

## 10. Robot không phản hồi teleop

Kiểm tra:

```bash
ros2 topic echo /cmd_vel_web
ros2 topic echo /cmd_vel
ros2 topic echo /rai_navigation/cmd_vel
```

Nếu dashboard gửi được `/cmd_vel_web` nhưng robot không chạy:

- kiểm tra `twist_mux`;
- kiểm tra base serial;
- kiểm tra emergency stop/power;
- kiểm tra quyền action của role Pi;
- kiểm tra robot base component đã start chưa.

## 11. Camera/Jetson không hoạt động

Kiểm tra Jetson runtime bridge:

```bash
curl http://100.69.39.18:8090/api/health
curl http://100.69.39.18:8090/api/system/components
```

Kiểm tra camera topics:

```bash
ros2 topic list | grep camera
```

Nếu depth không có, kiểm tra option `enable_depth` khi start camera trên dashboard.

## 12. Khi nào nên restart toàn bộ

Restart theo thứ tự này nếu hệ thống bị trạng thái lẫn lộn:

1. Stop dataset run nếu đang active.
2. Stop dataset launch stack.
3. Stop navigation/SLAM.
4. Stop robot base/LiDAR/camera.
5. Restart Pi/Jetson runtime bridge.
6. Restart hub API.
7. Restart website.
8. Refresh dashboard.
