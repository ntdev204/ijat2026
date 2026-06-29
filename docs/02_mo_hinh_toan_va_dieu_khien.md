# 02 - Mô hình toán và điều khiển

Tài liệu này tóm tắt mô hình toán đang được dùng cho controller CCA-NMPC trong hệ thống RAI Robot. Mục tiêu là giúp người vận hành hiểu các biến quan trọng khi đọc telemetry, cấu hình navigation và phân tích dataset.

## 1. Trạng thái robot và lệnh điều khiển

Robot Mecanum được mô hình hóa trên mặt phẳng với trạng thái:

$$
X_{r,k} = \begin{bmatrix} x_{r,k} & y_{r,k} & \theta_{r,k} \end{bmatrix}^{T}
$$

và lệnh điều khiển:

$$
u_k = \begin{bmatrix} v_{x,k} & v_{y,k} & \omega_k \end{bmatrix}^{T}
$$

Trong đó:

| Ký hiệu | Ý nghĩa |
| --- | --- |
| $x_r$, $y_r$ | vị trí robot trong frame điều khiển/map |
| $\theta_r$ | góc yaw của robot |
| $v_x$ | vận tốc tiến/lùi theo thân robot |
| $v_y$ | vận tốc ngang theo thân robot |
| $\omega$ | vận tốc góc yaw |

Giới hạn lệnh web API đang dùng:

$$
v_x, v_y \in [-1.5, 1.5]\ \text{m/s}
$$

$$
\omega \in [-3.0, 3.0]\ \text{rad/s}
$$

## 2. Động học Mecanum 3-DOF

Với $v_x$, $v_y$ trong body frame và $\theta$ trong world/control frame:

$$
\begin{aligned}
x_{k+1} &= x_k + \left(v_x \cos\theta_k - v_y \sin\theta_k\right)T_s \\
y_{k+1} &= y_k + \left(v_x \sin\theta_k + v_y \cos\theta_k\right)T_s \\
\theta_{k+1} &= \operatorname{wrap}\left(\theta_k + \omega_k T_s\right)
\end{aligned}
$$

Trong rollout NMPC, implementation dùng tích phân Runge-Kutta bậc 4 trên cùng mô hình động học để giảm sai số dự báo quỹ đạo.

## 3. Chân trời NMPC

Controller sinh chuỗi điều khiển trong chân trời dự báo:

$$
X_{r,k+i+1\mid k} = f_{\mathrm{mec}}\left(X_{r,k+i\mid k}, u_{k+i}, T_s\right),\quad i = 0,\ldots,N-1
$$

Cấu hình mặc định trong `rai_controller.yaml`:

| Tham số | Giá trị |
| --- | --- |
| `control_frequency` | `25.0 Hz` |
| `horizon_steps` | `20` |
| `sample_time` | `0.04 s` |
| `max_solver_time_ms` | `35.0 ms` |
| `default_v_ref` | `0.35 m/s` |
| `goal_tolerance_xy` | `0.25 m` |
| `goal_tolerance_yaw` | `0.35 rad` |

## 4. Trạng thái người và dự báo chuyển động

Mỗi human track được mô tả bằng:

$$
S_{h,j,k} = \begin{bmatrix} p_{h,j,k}^{T} & v_{h,j,k}^{T} & c_{h,j,k} & P_{h,j,k} \end{bmatrix}^{T}
$$

$$
p_{h,j,k} = \begin{bmatrix} x_{h,j,k} & y_{h,j,k} \end{bmatrix}^{T}
$$

$$
v_{h,j,k} = \begin{bmatrix} v_{hx,j,k} & v_{hy,j,k} \end{bmatrix}^{T}
$$

Dự báo ngắn hạn dùng mô hình vận tốc không đổi:

$$
\hat{p}_{h,j,k+i\mid k} = p_{h,j,k} + iT_s v_{h,j,k}
$$

$$
\hat{c}_{h,j,k+i\mid k} = c_{\mathrm{det},j,k}\exp\left(-\operatorname{trace}\left(P_{h,j,k}\right)\right)
$$

Khi covariance tăng, confidence hiệu dụng giảm để controller thận trọng hơn.

## 5. Chỉ số ngữ cảnh liên tục $\phi_h$

Mỗi người tạo một điểm rủi ro:

$$
\hat{\phi}_{j,k+i\mid k} = \sigma\left(z_{j,k+i\mid k}\right)
$$

$$
\sigma(z) = \frac{1}{1 + \exp(-z)}
$$

Với:

$$
\begin{aligned}
z &= w_d\frac{d_0 - d_h}{d_0}
+ w_v\frac{\lVert v_h \rVert}{v_{h,\max}}
+ w_{\theta}\cos(\Delta\theta)
+ w_c c_h
+ b
\end{aligned}
$$

Ý nghĩa vận hành:

| Thành phần | Khi tăng thì |
| --- | --- |
| Người gần robot ($d_h$ nhỏ) | $\phi_h$ tăng |
| Người di chuyển nhanh | $\phi_h$ tăng |
| Người tiến về phía robot | $\phi_h$ tăng |
| Detector confidence cao | tín hiệu context đáng tin hơn |

Với nhiều người:

$$
\hat{\phi}_h = \max_j \hat{\phi}_j
$$

$$
\hat{d}_h = \min_j \hat{d}_j
$$

Hệ thống lấy rủi ro lớn nhất và khoảng cách nhỏ nhất để ưu tiên an toàn.

## 6. Ràng buộc thích ứng

Khoảng cách an toàn:

$$
d_{\mathrm{safe}}(\phi_h) = d_{\mathrm{safe},0} + k_d\phi_h
$$

Cấu hình mặc định:

$$
d_{\mathrm{safe},0} = 0.5,\quad k_d = 0.3,\quad d_{\mathrm{safe},\max} = 0.8
$$

Giới hạn vận tốc thích ứng:

$$
\begin{aligned}
v_{x,\max}(\phi_h) &= v_{x,\max,0} - k_{vx}\phi_h \\
v_{y,\max}(\phi_h) &= v_{y,\max,0} - k_{vy}\phi_h \\
\omega_{\max}(\phi_h) &= \omega_{\max,0} - k_{\omega}\phi_h
\end{aligned}
$$

Cấu hình mặc định:

| Tham số | Giá trị |
| --- | --- |
| $v_{x,\max,0}$ | `0.45` |
| $v_{y,\max,0}$ | `0.35` |
| $\omega_{\max,0}$ | `1.0` |
| $v_{x,\min}$ | `0.08` |
| $v_{y,\min}$ | `0.06` |
| $\omega_{\min}$ | `0.15` |

Khi $\phi_h$ cao, robot tự giảm vận tốc tuyến tính/ngang/góc và tăng khoảng cách an toàn.

## 7. Hàm mục tiêu ở mức vận hành

Controller cân bằng các mục tiêu:

- bám đường đi toàn cục;
- giảm sai lệch hướng;
- giảm dao động lệnh điều khiển;
- tránh costmap obstacle;
- tránh người với `human_cost_weight`;
- giữ ràng buộc an toàn $d_h \ge d_{\mathrm{safe}}(\phi_h)$;
- dừng nếu dữ liệu odom/IMU/plan quá cũ.

Các biến thường cần xem khi debug:

| Topic | Ý nghĩa |
| --- | --- |
| `/canmpc/context` | $\phi_h$, $d_h$, $d_{\mathrm{safe}}$ |
| `/canmpc/humans` | trạng thái người đã track |
| `/canmpc/adaptive_bounds` | giới hạn vận tốc thích ứng |
| `/canmpc/local_reference_path` | đường tham chiếu cục bộ |
| `/canmpc/predicted_trajectory` | quỹ đạo robot dự báo |
| `/canmpc/solver_stats` | thời gian giải, trạng thái solver |
| `/rai_navigation/status` | trạng thái điều hướng |

## 8. Controller và planner

| Thành phần | Mặc định | Ghi chú |
| --- | --- | --- |
| Local controller | `CCA_NMPC` | controller đề xuất, có context người |
| Ablation controller | `NMPC` | biến thể nominal không adaptation theo người |
| Global planner | `A_STAR` | có thể đổi qua runtime config |
| Frame global | `map` | trong `rai_controller.yaml` |
| Frame control | `odom_combined` | dùng cho pose/velocity control |
| Base frame | `base_footprint` | robot base |

Khi so sánh thực nghiệm, chạy cùng kịch bản với `CCA_NMPC` và `NMPC`, cùng start/goal, cùng environment và run index tương ứng.
