# 4. Giao diện và Web Dashboard (Web Controls & API)

Tài liệu này mô tả thiết kế giao diện ứng dụng web giám sát (`rai_website`) và bộ API điều phối (`rai_web_api`) chạy trực tiếp trên robot, hỗ trợ thu thập dữ liệu và tương tác trực quan.

---

## 1. Các trang chức năng chính (Next.js Frontend)

Ứng dụng web được xây dựng bằng framework Next.js và Tailwind CSS. Theo bản cập nhật mới nhất, giao diện đã được tinh giản, loại bỏ các thành phần Nav2 dư thừa và trang training để tập trung vào mục tiêu nghiên cứu CCA-NMPC:

* **Monitor (Trang Giám sát chính)**:
  * Theo dõi luồng dữ liệu thời gian thực của robot (pose, velocity, costmap).
  * Vẽ biểu đồ động cho chỉ số ngữ cảnh $\phi_h$, khoảng cách người $d_h$, và khoảng cách an toàn thích ứng $d_{\text{safe}}$.
  * Cho phép thiết lập đích đến (`goal_pose`) và chọn giải thuật điều khiển trực tiếp trên bản đồ.
* **Map (Trang Bản đồ)**:
  * Quản lý trạng thái SLAM, vẽ bản đồ Occupancy Grid động phục vụ định vị.
* **Dataset Collection (Trang Thu thập dữ liệu)**:
  * Quản lý trạng thái của các run thử nghiệm.
  * Tích hợp cấu hình metadata theo chuẩn `research_dataset_spec.py` bao gồm chọn kịch bản (S1-S6), bộ điều khiển native (`CCA_NMPC`, `NMPC`), cấu hình start/goal, và ghi chú trial.
  * Cung cấp các nút chuẩn bị thư mục (`Prepare folders`), bắt đầu ghi bag (`Start bag`), dừng ghi và nén file (`Stop`).
  * Gọi trực tiếp pipeline hậu xử lý số liệu thống kê.

Trang này là giao diện vận hành chính cho dataset: bảng run có phân trang, lọc theo scenario/controller/environment, xem trạng thái pipeline, tải trực tiếp CSV/ZIP/metadata, và kích hoạt `launch` hoặc `node` từ chính website thay vì thao tác rời ở terminal.

* **System (Trang Điều khiển Thiết bị)**:
  * Điều khiển bật/tắt độc lập từng thành phần phần cứng và runtime nền.
  * Hiển thị trạng thái trực quan theo thiết bị:
    * **Pi**: Robot base (`turn_on_rai_robot.launch.py`), LiDAR (`rai_lidar.launch.py`)
    * **Jetson**: Camera (`rai_camera.launch.py`)
  * Hỗ trợ gọi proxy từ Pi sang Jetson để người vận hành điều khiển toàn hệ thống từ một website duy nhất.
  * Camera có tuỳ chọn bật depth stream trực tiếp trên giao diện trước khi launch.

---

## 2. API Role Guard (Bảo vệ vai trò thiết bị)

Vì `rai_web_api` chạy trực tiếp trên thiết bị (ở cổng mặc định `8080`), hệ thống tích hợp bộ lọc vai trò (`device_role`) để tránh các thao tác sai thiết bị:

* Khi API chạy với vai trò `jetson`:
  * Chỉ cho phép các API liên quan đến cấu hình thuật toán điều khiển và truy xuất thông tin từ `HumanPredictor`.
  * Tự động chặn và trả về lỗi `403 Forbidden` đối với các API liên quan đến phần cứng của Pi (như `/api/dataset/*`, `/api/robot/slam/*`).
* Khi API chạy với vai trò `pi`:
  * Cho phép đầy đủ quyền lập lịch trình, SLAM, record dữ liệu, và điều khiển từ xa.
  * Cho phép điều khiển trang `System`, bao gồm start/stop robot base, LiDAR, và proxy camera sang Jetson nếu `RAI_JETSON_API_URL` đã được cấu hình.
* Khi API chạy với vai trò `jetson`:
  * Cho phép trang `System` điều khiển camera cục bộ.

---

## 3. Các WebSocket Telemetry Streams

Để truyền tải dữ liệu tần số cao mượt mà lên trình duyệt của người dùng mà không gây tắc nghẽn, backend sử dụng WebSocket:

* **`ws://<ip_pi>:8080/api/ws/telemetry`**:
  * Stream dữ liệu trạng thái chuyển động của robot ($v_x, v_y, \omega$).
  * Stream trạng thái bộ giải solver stats (thời gian tính toán, số lượng mẫu).
* **`ws://<ip_pi>:8080/api/ws/dataset`**:
  * Stream tiến trình ghi file rosbag, dung lượng ghi và thời gian trial.
* **`ws://<ip_pi>:8080/api/ws/map`**:
  * Stream bản đồ lưới cục bộ (local occupancy grid) và các cập nhật.

---

## 4. Các API Endpoint thu thập Dataset chính

* **`POST /api/dataset/prepare`**: Chuẩn bị cấu trúc thư mục lưu trữ cho run mới.
* **`POST /api/dataset/start`**: Truyền thông tin metadata, kích hoạt `ros2 bag record` ở chế độ background trên Pi.
* **`POST /api/dataset/stop`**: Gửi tín hiệu dừng `ros2 bag record`, đóng tệp rosbag và tự động nén thư mục run thành tệp `.zip`.
* **`POST /api/dataset/pipeline`**: Gọi kịch bản python hậu xử lý để tự động tính toán các chỉ số RMSE, an toàn, JERK và xuất biểu đồ ngay sau khi trial kết thúc.
