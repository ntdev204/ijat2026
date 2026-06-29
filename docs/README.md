# Tài liệu vận hành RAI Robot

Bộ tài liệu này thay thế các ghi chú cũ và tập trung vào cách vận hành hệ thống robot RAI trong repo `ijat2026`: kiến trúc thiết bị, mô hình điều khiển CCA-NMPC, dashboard, startup service và quy trình chạy dataset theo kịch bản.

## Đọc nhanh theo nhu cầu

| Nhu cầu | Tài liệu |
| --- | --- |
| Hiểu hệ thống chạy trên máy nào, role nào, IP nào | [01 - Kiến trúc hệ thống](./01_kien_truc_he_thong.md) |
| Hiểu mô hình toán CCA-NMPC và các biến được ghi dataset | [02 - Mô hình toán và điều khiển](./02_mo_hinh_toan_va_dieu_khien.md) |
| Khởi động hub, Pi, Jetson, website và systemd service | [03 - Vận hành hệ thống](./03_van_hanh_he_thong.md) |
| Chạy dataset S1-S6, ghi bag, xử lý pipeline | [04 - Chạy dataset theo kịch bản](./04_chay_dataset_theo_kich_ban.md) |
| Cấu hình dashboard, REST API, WebSocket | [05 - Web dashboard và API](./05_web_dashboard_va_api.md) |
| Sửa lỗi thường gặp khi chạy robot/dataset | [06 - Troubleshooting](./06_troubleshooting.md) |

## Luồng vận hành chuẩn

1. Bật mạng giữa hub, Pi và Jetson.
2. Trên cả ba máy, dùng cùng `ROS_DOMAIN_ID=30`, `RMW_IMPLEMENTATION=rmw_cyclonedds_cpp`, `ROS_LOCALHOST_ONLY=0`.
3. Trên hub, bật PostgreSQL bằng `make db-up`.
4. Bật `rai_web_api` trên hub với `RAI_DEVICE_ROLE=hub`.
5. Bật `rai_runtime_bridge` trên Pi với `RAI_DEVICE_ROLE=pi`.
6. Bật `rai_runtime_bridge` trên Jetson với `RAI_DEVICE_ROLE=jetson`.
7. Bật `rai_website` ở cổng `3000`.
8. Vào dashboard để kiểm tra `/api/health`, runtime components, telemetry và dataset state.

## Cấu hình IP mặc định

| Thiết bị | Role | URL mặc định |
| --- | --- | --- |
| Server / Laptop hub | `hub` hoặc `laptop` | `http://100.116.199.115:8080` |
| Raspberry Pi 4 bridge | `pi` | `http://100.120.77.81:8090` |
| Jetson Orin Nano bridge | `jetson` | `http://100.69.39.18:8090` |
| Website | frontend | `http://100.116.199.115:3000` |

Nếu đổi IP, cập nhật trong `scripts/startup/common_env.sh`, `.env.local` của `rai_website`, hoặc biến môi trường tương ứng trước khi launch.

## Cây thành phần chính

```text
ijat2026/
├── rai_ros2/                 # ROS 2 workspace: robot, navigation, perception, web API
├── rai_website/              # Next.js dashboard vận hành
├── scripts/startup/          # startup scripts và systemd services
├── docs/                     # bộ tài liệu vận hành mới
├── docker-compose.yml        # PostgreSQL cho metadata dataset
└── Makefile                  # lệnh db-up/db-down/db-logs
```

## Nguyên tắc vận hành

- Hub là điểm vào cho người vận hành và website.
- Pi giữ runtime robot: base, LiDAR, SLAM, navigation, dataset recording.
- Jetson giữ runtime camera/perception.
- Dataset chỉ có giá trị khi metadata, scenario, controller, start/goal và topic ROS 2 được ghi nhất quán.
- Không chạy nhiều instance cùng role trên cùng port `8080` nếu không có lý do rõ ràng.
