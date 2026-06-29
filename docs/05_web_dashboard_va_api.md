# 05 - Web dashboard và API

Tài liệu này mô tả cách cấu hình `rai_website`, các endpoint vận hành chính của `rai_web_api`, và cách kiểm tra WebSocket/REST khi chạy robot.

## 1. Dashboard

`rai_website` là Next.js dashboard dùng để:

- xem trạng thái runtime robot;
- bật/tắt component trên Pi/Jetson;
- gửi teleop/navigation commands;
- xem telemetry, map, paths;
- quản lý dataset run và pipeline;
- cấu hình navigation/controller/planner.

Chạy development:

```bash
cd ~/ijat2026/rai_website
npm install
npm run dev
```

Chạy production:

```bash
npm run build
npm run start -- --hostname 0.0.0.0 --port 3000
```

## 2. Biến môi trường frontend

Tạo `rai_website/.env.local`:

```bash
NEXT_PUBLIC_API_URL=http://100.116.199.115:8080
NEXT_PUBLIC_ALLOWED_DEV_ORIGINS=100.116.199.115
```

| Biến | Ý nghĩa |
| --- | --- |
| `NEXT_PUBLIC_API_URL` | REST API chính, thường trỏ hub |
| `NEXT_PUBLIC_WS_URL` | override toàn bộ websocket base nếu có proxy riêng |
| `NEXT_PUBLIC_ALLOWED_DEV_ORIGINS` | origin dev được Next.js chấp nhận |

## 3. REST routing

Các request REST đi qua `resolveApiEndpoint()` và mặc định dùng `NEXT_PUBLIC_API_URL`. Hub API tự proxy xuống Pi/Jetson khi endpoint đó thuộc runtime device.

Ví dụ:

```bash
curl http://100.116.199.115:8080/api/health
curl http://100.116.199.115:8080/api/system/runtime
curl http://100.116.199.115:8080/api/system/components
```

## 4. WebSocket routing

Các stream tần số cao dùng WebSocket:

| Path | Backend khuyến nghị |
| --- | --- |
| `/api/ws/telemetry` | Pi |
| `/api/ws/map` | Pi |
| `/api/ws/paths` | Pi |
| `/api/ws/dataset` | Hub |
| `/api/ws/control` | Pi |

Frontend luôn dùng hub API origin cho các stream này. Hub API làm việc với runtime bridge ở Pi/Jetson khi cần điều khiển runtime.

## 5. Endpoint runtime chính

| Endpoint | Mục đích |
| --- | --- |
| `GET /api/health` | kiểm tra API sống |
| `GET /api/system/runtime` | role, label, actions, bridge URLs |
| `GET /api/system/components` | danh sách runtime components |
| `POST /api/system/components/robot/start|stop` | robot base trên Pi |
| `POST /api/system/components/lidar/start|stop` | LiDAR trên Pi |
| `POST /api/system/components/camera/start|stop` | camera trên Jetson |
| `POST /api/system/components/slam/start|stop` | SLAM |
| `POST /api/system/components/navigation/start|stop` | navigation stack |
| `POST /api/system/components/dataset/start|stop` | dataset launch stack |

## 6. Endpoint robot/navigation

| Endpoint | Mục đích |
| --- | --- |
| `POST /api/robot/cmd_vel` | gửi velocity command |
| `POST /api/robot/nav/goal` | gửi goal pose |
| `POST /api/robot/nav/route` | gửi route start-goal |
| `POST /api/robot/nav/cancel` | hủy goal |
| `GET /api/robot/anchors` | xem anchor/home pose |
| `POST /api/robot/initial_pose` | set initial pose |
| `POST /api/robot/home` | set home pose |
| `POST /api/robot/nav/home` | chạy về home |
| `GET /api/rai-navigation/config` | xem cấu hình navigation |
| `POST /api/rai-navigation/config` | đổi controller/planner/map |
| `POST /api/rai-navigation/start|stop` | bật/tắt navigation |

## 7. Endpoint dataset

| Endpoint | Mục đích |
| --- | --- |
| `POST /api/dataset/prepare` | tạo layout dataset |
| `GET /api/dataset/artifacts` | xem artefact sẵn có |
| `GET /api/dataset/launch/status` | trạng thái launch stack và recorder trên Pi bridge |
| `POST /api/dataset/launch/start|stop` | bật/tắt dataset launch stack trên Pi |
| `POST /api/dataset/start` | tạo metadata trên hub và bắt đầu recorder qua Pi bridge |
| `GET /api/dataset/active` | xem run đang ghi |
| `POST /api/dataset/stop` | dừng recorder trên Pi và hoàn tất metadata trên hub |
| `POST /api/dataset/pipeline` | chạy hậu xử lý |
| `GET /api/dataset/runs` | danh sách run |
| `GET /api/dataset/runs/{id}/download` | tải artefact run |

## 8. Kiểm tra WebSocket thủ công

Dùng browser devtools hoặc công cụ WebSocket bất kỳ để kiểm tra:

```text
ws://100.116.199.115:8080/api/ws/telemetry
ws://100.116.199.115:8080/api/ws/map
ws://100.116.199.115:8080/api/ws/control
```

Nếu dashboard hiện disconnected nhưng REST API vẫn OK, kiểm tra `NEXT_PUBLIC_API_URL`, CORS trên hub API và ROS 2 DDS giữa hub với Pi/Jetson.

## 9. CORS backend

`rai_web_api` lấy origin từ `RAI_API_CORS`. Mặc định trong startup:

```bash
RAI_API_CORS=http://localhost:3000,http://100.116.199.115:3000
```

Khi đổi host dashboard, thêm origin mới vào biến này.

## 10. Checklist trước khi vận hành bằng dashboard

- `GET /api/health` trả `status=ok`.
- `GET /api/system/runtime` đúng role `hub` trên server.
- `GET /api/system/components` thấy component Pi/Jetson.
- Website mở được từ browser vận hành.
- WebSocket telemetry connected.
- Nút start/stop component phản ánh đúng trạng thái runtime.
- Dataset page thấy launch status và active run status.
