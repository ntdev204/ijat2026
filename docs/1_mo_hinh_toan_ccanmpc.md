# 1. Mô hình Toán học CCA-NMPC (Continuous Context-Aware NMPC)

Tài liệu này trình bày chi tiết mô hình toán học nâng cấp của bộ điều khiển **CCA-NMPC** cho robot Mecanum di động, tích hợp bộ dự báo chuyển động người tách biệt (`HumanPredictor`), tích phân sai phân Runge-Kutta bậc 4 (RK4), hiệp phương sai Kalman, và gom tụ đa người (multi-human aggregation).

---

## 1. Khử nhiễu và Tiền xử lý dữ liệu (Kalman Filtering)

Trước khi đưa vào mô hình tối ưu hóa, các luồng dữ liệu cảm biến được lọc qua 3 tầng bộ lọc:

1. **Lọc góc nghiêng và góc quay IMU mức thấp (STM32 MCU)**:
   Chạy bộ lọc Kalman cơ bản trên vi điều khiển (file `filter.c`) để lọc nhiễu tần số cao của gia tốc và vận tốc góc từ IMU thô.

2. **Dung hợp định vị robot (Extended Kalman Filter - EKF)**:
   Gói EKF (`robot_localization` hoặc `robot_pose_ekf`) trên ROS 2 dung hợp dữ liệu Odom bánh xe và IMU để ước lượng Pose robot ổn định:
   $$X_{r,k} = [x_{r,k}, y_{r,k}, \theta_{r,k}]^T$$

3. **Bám vết người đi bộ (Linear Kalman Filter)**:
   Dữ liệu từ YOLO26m (`best.pt`) phát hiện người trên camera RGB-D được đưa qua **Linear Kalman Filter** (`KalmanTracker` trong gói `rai_human_perception`) để ước lượng vector trạng thái:
   $$S_{h,j} = [x_{h,j}, y_{h,j}, v_{hx,j}, v_{hy,j}]^T$$

### Tích hợp Hiệp phương sai Kalman (Covariance-Aware Confidence)
Độ tin cậy hiệu dụng $c_h$ của mỗi người được điều biến bằng trace của ma trận hiệp phương sai sai số $P$ từ bộ lọc Kalman để giảm thiểu sai số khi theo dấu bị mất hoặc nhiễu lớn:
$$c_{\text{cov}} = \text{clamp}\left( e^{-\text{trace}(P)}, 0.0, 1.0 \right)$$
$$\text{trace}(P) = P_{xx} + P_{yy} + P_{v_x v_x} + P_{v_y v_y}$$
$$c_h = c_{\text{detector}} \times c_{\text{cov}}$$
Trong đó $c_{\text{detector}}$ là độ tin cậy nhận diện từ YOLO26m.

---

## 2. Mô hình Động học Robot Mecanum tích phân RK4 (Runge-Kutta 4th Order)

Robot di động sử dụng cơ cấu bánh xe Mecanum 3 DOF với vector tín hiệu điều khiển vận tốc dọc, vận tốc ngang và vận tốc góc:
$$u_k = [v_{x,k}, v_{y,k}, \omega_k]^T$$

Để tăng độ chính xác tích phân sai phân quỹ đạo trong chân trời dự báo so với phương pháp Euler truyền thống, thuật toán sử dụng tích phân **Runge-Kutta bậc 4 (RK4)**. Định nghĩa hàm đạo hàm trạng thái theo góc hướng $\theta$:
$$f(\theta) = \begin{bmatrix} v_x \cos\theta - v_y \sin\theta \\ v_x \sin\theta + v_y \cos\theta \\ \omega \end{bmatrix}$$

Tại mỗi bước dự báo với chu kỳ trích mẫu $T_s$:
$$k_1 = f(\theta_k)$$
$$k_2 = f(\theta_k + 0.5 T_s k_{1,\theta})$$
$$k_3 = f(\theta_k + 0.5 T_s k_{2,\theta})$$
$$k_4 = f(\theta_k + T_s k_{3,\theta})$$

Cập nhật trạng thái robot mới:
$$x_{k+1} = x_k + \frac{T_s}{6} (k_{1,x} + 2 k_{2,x} + 2 k_{3,x} + k_{4,x})$$
$$y_{k+1} = y_k + \frac{T_s}{6} (k_{1,y} + 2 k_{2,y} + 2 k_{3,y} + k_{4,y})$$
$$\theta_{k+1} = \text{normalizeAngle}\left( \theta_k + \frac{T_s}{6} (k_{1,\theta} + 2 k_{2,\theta} + 2 k_{3,\theta} + k_{4,\theta}) \right)$$

---

## 3. Kiến trúc Ước lượng Ngữ cảnh Độc lập (HumanPredictor)

Mô hình dự báo hành vi người được tách thành một lớp riêng biệt `HumanPredictor`, trả về cấu trúc ngữ cảnh tích hợp `PredictedHumanContext`.

### 3.1 Dự báo quỹ đạo người
Với mỗi người $j$, vị trí tương lai được dự báo dựa trên vận tốc ước lượng Kalman:
$$\hat{p}_{h,j,k+i|k} = p_{h,j,k} + i T_s v_{h,j,k}$$

### 3.2 Gom tụ đa người (Multi-Human Aggregation)
Thay vì chỉ chọn người gần nhất, thuật toán duyệt qua toàn bộ danh sách để tính toán chỉ số $\phi_{h,j}$ cho từng người, sau đó gom tụ lại:
* **Khoảng cách vật lý**: Lấy khoảng cách nhỏ nhất đến người gần nhất để làm biên phòng thủ va chạm:
  $$\hat{d}_{h,k+i|k} = \min_j \| \hat{p}_{r,k+i|k} - \hat{p}_{h,j,k+i|k} \|_2$$
* **Chỉ số rủi ro ngữ cảnh**: Gom tụ bằng toán tử `max` để phản ánh rủi ro lớn nhất từ đám đông:
  $$\phi_{h,k+i|k} = \max_j \phi_{h,j,k+i|k}$$

---

## 4. Chỉ số Ngữ cảnh Đa biến $\phi_h$ và Thích ứng Tham số

Với mỗi người $j$, chỉ số ngữ cảnh $\phi_{h,j}$ được xác định bằng hàm Sigmoid đa biến:
$$\phi_{h,j} = \sigma(z_j)$$
$$z_j = w_d \frac{d_0 - d_{h,j}}{d_0} + w_v \frac{\|v_{h,j}\|_2}{v_{h,\text{max}}} + w_{\theta} \cos(\Delta\theta_j) + w_c c_{h,j} + b_{\phi}$$
Trong đó $\cos(\Delta\theta_j)$ là cosine góc hướng di chuyển tương đối của người so với robot:
$$\cos(\Delta\theta_j) = \text{clamp}\left( \frac{(v_{h,j} - v_{r}) \cdot e_{rh,j}}{\max(\|v_{h,j} - v_{r}\|_2, \epsilon)}, -1.0, 1.0 \right)$$

### Thích ứng thông số điều khiển:
* **Khoảng cách an toàn thích ứng**: $d_{\text{safe}}(\phi_h) = d_{\text{safe},0} + k_d \phi_h$ (mặc định: $d_{\text{safe}} = 0.5 + 0.3 \phi_h$).
* **Ma trận trọng số thích ứng**: $Q(\phi_h) = Q_0 + \phi_h Q_h$.

---

## 5. Bài toán Tối ưu hóa và các Ràng buộc (NMPC Formulation)

Bộ tối ưu cực tiểu hóa hàm mục tiêu trong chân trời $N$:

$$\min_{U_k} J_k = \sum_{i=0}^{N-1} \left[ \|X_{r,k+i|k} - X_{\text{ref},k+i}\|^2_{Q(\phi_{h,k+i|k})} + \|u_{k+i|k}\|^2_R + \|\Delta u_{k+i|k}\|^2_{R_{\delta}} + J_{\text{human}}(k+i|k) + J_{\text{costmap}}(k+i|k) \right] + J_{\text{terminal}}$$

Với chi phí tránh người đặc trưng:
$$J_{\text{human}}(k+i|k) = w_{\text{human}} \phi_{h,k+i|k} \max(0, d_0 - d_{h,k+i|k})^2$$

### Các Ràng buộc tối ưu hóa (Constraints):
1. **Động lực học RK4**: $X_{r,k+i+1|k} = f_{\text{RK4}}(X_{r,k+i|k}, u_{k+i|k})$
2. **Ràng buộc Vận tốc Thích ứng**:
   $$|v_{x,k+i|k}| \le v_{x,\text{max},0} - k_{v,x} \phi_{h,k+i|k}$$
   $$|v_{y,k+i|k}| \le v_{y,\text{max},0} - k_{v,y} \phi_{h,k+i|k}$$
   $$|\omega_{k+i|k}| \le \omega_{\text{max},0} - k_{\omega} \phi_{h,k+i|k}$$
3. **Ràng buộc khoảng cách an toàn**:
   $$d_{h,k+i|k} \ge d_{\text{safe}}(\phi_{h,k+i|k})$$
4. **Tránh vật cản Costmap**:
   $$\text{occupancy}(p_{x,k+i|k}, p_{y,k+i|k}) < C_{\text{threshold}}$$
Vi phạm các ràng buộc (2), (3) và (4) sẽ bị phạt nặng thông qua hệ số biến phạt lỏng $w_{\text{slack}} = 100,000.0$.
