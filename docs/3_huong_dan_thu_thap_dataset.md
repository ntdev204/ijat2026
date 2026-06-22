# 3. Hướng dẫn Thu thập Dataset và Thực nghiệm (Dataset & Experiments)

Tài liệu này trình bày các chỉ dẫn thực nghiệm thu thập dataset, các nhóm dữ liệu cấu trúc, các kịch bản chạy thử (S1 - S6) và kế hoạch phân tích thống kê phục vụ cho nghiên cứu CCA-NMPC.

---

## 1. Nguyên lý Thiết kế Dataset

Hệ thống lưu giữ dữ liệu dưới dạng thô (rosbag) và tự động xử lý chuyển đổi thành các file dữ liệu dạng bảng phục vụ trực tiếp cho việc vẽ biểu đồ và phân tích thống kê. Các nguyên lý gồm:
* Khai báo chính xác mã kịch bản (Scenario ID), bộ điều khiển (Controller ID), và môi trường thực thi (`real` hoặc `sim`) khi bắt đầu ghi.
* Không phụ thuộc vào bản đồ cố định hay waypoint tự động; robot hoạt động theo tương tác thời gian thực.
* Hỗ trợ lưu trữ rosbag và xuất telemetry trực quan hóa ngay trên giao diện Web Dashboard.

### Cấu trúc `dataset/` chuẩn mới nhất

Theo triển khai hiện tại trong `rai_web_api`, thư mục `dataset/` được chuẩn hóa như sau:

```text
dataset/
├── raw/
│   ├── sim/
│   └── real/
├── derived/
│   ├── human_motion_dataset/
│   │   ├── human.csv
│   │   └── predicted_human.csv
│   ├── context_dataset/
│   │   ├── context.csv
│   │   └── adaptive_constraints.csv
│   ├── navigation_dataset/
│   │   ├── robot.csv
│   │   └── solver.csv
│   ├── baseline_comparison_dataset/
│   │   └── run_summary.csv
│   └── aggregates/
│       ├── controller_timeseries.csv
│       ├── human_states.csv
│       ├── human_prediction.csv
│       ├── adaptation_timeseries.csv
│       ├── metrics_per_run.csv
│       └── metrics_summary.csv
├── metadata/
│   ├── calibration/
│   ├── splits/
│   │   ├── train_runs.txt
│   │   ├── val_runs.txt
│   │   └── test_runs.txt
│   ├── scenarios/
│   ├── controllers/
│   ├── videos/
│   │   └── video_metadata.csv
│   ├── run_index.csv
│   ├── sensors.yaml
│   ├── robot_mecanum.yaml
│   ├── controllers.yaml
│   ├── ablation.yaml
│   ├── dataset_groups.yaml
│   ├── statistical_validation.yaml
│   ├── required_figures.yaml
│   └── scenarios.yaml
├── figures/
│   ├── system/
│   ├── trajectories/
│   ├── timeseries/
│   ├── boxplots/
│   ├── latency/
│   ├── ablation/
│   ├── context/
│   ├── prediction/
│   └── benchmark/
├── tables/
│   ├── csv/
│   ├── latex/
│   └── ablation/
└── videos/
```

### Cấu trúc của một run thô trong `raw/`

Mỗi run được ghi dưới dạng:

```text
dataset/raw/<environment>/<scenario_name>/<controller_id>/<run_id>/
├── rosbag2/
├── rosbag_record.log
└── metadata.json
```

Ví dụ:

```text
dataset/raw/real/S6_human_approaching/CCA_NMPC/run_000/
├── rosbag2/
├── rosbag_record.log
└── metadata.json
```

---

## 2. Thiết kế 6 Kịch bản Thực nghiệm (S1 - S6)

Hệ thống yêu cầu chạy thực nghiệm thu thập dữ liệu trên 6 kịch bản cụ thể:

| Kịch bản | Tên kịch bản | Mục tiêu khoa học | Cách thực hiện | Số run tối thiểu | Chỉ số chính |
|---|---|---|---|---|---|
| **S1** | `S1_open_zone` | Đánh giá bám đường danh nghĩa và hoạt động khi không có nguy cơ từ người. | Di chuyển trên bãi rộng, không có người cản trở. | 30 runs | `tracking rmse_xy` |
| **S2** | `S2_narrow_corridor` | Đánh giá hành vi di chuyển ngang của Mecanum và giới hạn thích ứng trong hành lang hẹp tĩnh. | Lái robot dọc theo lối đi hẹp. | 30 runs | `minimum_human_distance` |
| **S3** | `S3_human_proximate` | Đánh giá đáp ứng tăng dần của $\phi_h$ và $d_{\text{safe}}$ thích ứng khi robot tiến gần người. | Một hoặc hai người đứng yên hoặc đi rất chậm sát đường đi. | 40 runs | `mean_context_phi_h` |
| **S4** | `S4_dynamic_crossing` | Đánh giá dự báo vị trí người và đáp ứng giảm tốc khi có người đi cắt ngang vuông góc. | Người đi bộ băng ngang quỹ đạo robot ở tốc độ $0.4 - 1.2\text{ m/s}$. | 50 runs | `prediction_rmse` |
| **S5** | `S5_occlusion` | Đánh giá độ tin cậy cảm biến, hiệp phương sai và hành vi an toàn dự phòng khi có người xuất hiện bất ngờ. | Người đột ngột bước ra từ sau góc tường/tủ che khuất tầm nhìn camera. | 50 runs | `context_response_time` |
| **S6** | `S6_human_approaching` | Đánh giá độ nhạy của thành phần góc di chuyển tương đối $\cos(\Delta\theta)$ bằng cách đối chiếu người đi đối diện và đi cùng chiều. | Người đi trực diện đối mặt robot, sau đó quay lưng đi cùng chiều ở cùng khoảng cách. | 60 runs | `relative_heading_vs_phi_h` |

---

## 3. Các Nhóm Dataset Trích xuất (Dataset Groups)

Mỗi run thực nghiệm sau khi xử lý sẽ trích xuất ra các file dữ liệu dạng bảng phục vụ cho 4 nhóm nghiên cứu khoa học chính:

1. **Dataset 1: Human Motion Dataset (`human_motion_dataset`)**
   * *Mục tiêu*: Đánh giá chất lượng bám vết Kalman và dự báo chuyển động người.
   * *File yêu cầu*: `human.csv`, `predicted_human.csv`.
2. **Dataset 2: Context Dataset (`context_dataset`)**
   * *Mục tiêu*: Kiểm chứng đáp ứng của chỉ số ngữ cảnh liên tục $\phi_h$ và thích ứng các ràng buộc.
   * *File yêu cầu*: `context.csv`, `adaptive_constraints.csv`.
3. **Dataset 3: Navigation Dataset (`navigation_dataset`)**
   * *Mục tiêu*: Đánh giá chất lượng định hướng robot, tính an toàn và hành vi điều khiển.
   * *File yêu cầu*: `robot.csv`, `context.csv`, `human.csv`, `solver.csv`.
4. **Dataset 4: Baseline Comparison Dataset (`baseline_comparison_dataset`)**
   * *Mục tiêu*: So sánh hiệu năng của `CCA_NMPC` với biến thể `NMPC` danh nghĩa nội bộ trên cùng kịch bản với cùng chu kỳ điều khiển `25 Hz`.
   * *File yêu cầu*: `run_summary.csv`.

### Các file tổng hợp bổ sung ngoài 4 dataset group

Ngoài 4 nhóm dataset chính, hệ thống hiện còn sinh thêm các file tổng hợp mức cao hơn để phục vụ pipeline đánh giá, web dashboard và script hậu xử lý:

* `derived/aggregates/controller_timeseries.csv`: chuỗi thời gian điều khiển, theo dõi `phi_h`, `d_safe`, adaptive bounds và solver stats theo từng mẫu.
* `derived/aggregates/human_states.csv`: chuỗi trạng thái người theo thời gian, kèm confidence và covariance.
* `derived/aggregates/human_prediction.csv`: dữ liệu dự báo người theo horizon để tính sai số prediction.
* `derived/aggregates/adaptation_timeseries.csv`: chuỗi thích ứng của `phi_h -> d_safe -> v_max`.
* `derived/aggregates/metrics_per_run.csv`: metric đầy đủ cho từng run.
* `derived/aggregates/metrics_summary.csv`: metric tổng hợp theo scenario/controller/environment.

---

## 4. Kế hoạch Phân tích Thống kê (Statistical Plan)

Để đảm bảo ý nghĩa thống kê của các kết quả nghiên cứu công bố trên bài báo:
* **Quy mô mẫu**: Tối thiểu 30 runs cho mỗi điều kiện, đề xuất 50 runs cho các kịch bản cốt lõi và 60 runs cho kịch bản S6.
* **Báo cáo số liệu**: Định dạng $Mean \pm STD$ (Độ lệch chuẩn) và Khoảng tin cậy 95% (95% Confidence Interval).
* **Kiểm định giả thuyết ý nghĩa thống kê**:
  * Sử dụng kiểm định *paired t-test* đối với phân phối chuẩn.
  * Sử dụng kiểm định *Wilcoxon signed-rank* đối với dữ liệu phân phối phi chuẩn.
* **Đánh giá kích cỡ ảnh hưởng (Effect Size)**: Tính toán hệ số *Cohen d* hoặc *Cliff delta* để chứng minh mức độ cải thiện hiệu năng vượt trội.

---

## 5. Danh mục 11 Biểu đồ Nghiên cứu Bắt buộc (Required Figures)

Hệ thống yêu cầu xuất ra 11 biểu đồ phục vụ cho bài báo khoa học:

1. `trajectory_comparison`: So sánh quỹ đạo di chuyển thực tế của các bộ điều khiển trên cùng một kịch bản.
2. `phi_h_timeseries`: Biến thiên thời gian của chỉ số ngữ cảnh $\phi_h$.
3. `d_safe_timeseries`: Biến thiên thời gian của khoảng cách an toàn thích ứng $d_{\text{safe}}$.
4. `v_max_timeseries`: Biến thiên thời gian của giới hạn vận tốc tuyến tính tối đa thích ứng.
5. `human_distance_vs_phi_h`: Biểu đồ phân tán thể hiện mối quan hệ phi tuyến giữa khoảng cách người và chỉ số $\phi_h$.
6. `prediction_error`: Sai số dự báo chuyển động người theo chân trời thời gian.
7. `solver_distribution`: Phân bố thời gian giải tối ưu của solver (tính bằng ms).
8. `safety_comparison`: So sánh khoảng cách an toàn nhỏ nhất giữa các bộ điều khiển.
9. `rmse_comparison`: So sánh sai số bám đường RMSE giữa các bộ điều khiển.
10. `adaptive_constraint_behaviour`: Minh họa hành vi kích hoạt ràng buộc thích ứng.
11. `ablation_results`: Kết quả đối chiếu `CCA_NMPC` với `NMPC` danh nghĩa để làm rõ phần đóng góp của continuous context adaptation trong môi trường Nav2-free.

---

## 6. Quy trình Thực thi Hậu xử lý (CLI Workflow)

Người vận hành có thể chạy tuần tự các công cụ trích xuất dữ liệu và vẽ biểu đồ sau khi hoàn tất trials:
```bash
cd ~/ijat2026

# Xuất dữ liệu thô rosbag ra CSV/Parquet theo đặc tả
python3 scripts/dataset/bag_to_csv.py --dataset dataset --bag dataset/raw/real/S6/cca_nmpc/run_000/rosbag2

# Lập danh mục run_index
python3 scripts/dataset/build_run_index.py --dataset dataset

# Trích xuất số liệu đánh giá
python3 scripts/dataset/extract_metrics.py --dataset dataset

# Tự động vẽ các biểu đồ báo cáo
python3 scripts/plot/plot_trajectories.py --dataset dataset
python3 scripts/plot/plot_boxplots.py --dataset dataset
```

---

## 7. Ghi chú Đồng bộ với Web Dashboard

Giao diện `rai_website` hiện thao tác trực tiếp với đúng layout `dataset/` ở trên:

* Trang Dataset đọc danh sách run từ `raw/<environment>/<scenario>/<controller>/<run_id>/`.
* Nút tải artifact sẽ tải trực tiếp:
  * `derived/robot.csv`
  * `derived/human.csv`
  * `derived/context.csv`
  * `derived/adaptive_constraints.csv`
  * `derived/solver.csv`
  * `derived/predicted_human.csv`
  * `derived/run_summary.csv`
  * `metadata.json`
* Các thư mục `figures/`, `tables/`, `videos/` được giữ sẵn để pipeline hậu xử lý và supplementary material ghi trực tiếp vào đúng vị trí.
