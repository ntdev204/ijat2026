# Dataset Requirements cho Continuous Context-Adaptive CA-NMPC

**Muc tieu:** thu thap dataset thuc va mo phong cho robot RAI Mecanum de danh gia Continuous Context-Adaptive Nonlinear Model Predictive Control (CCA-NMPC) trong dieu huong co nhan thuc con nguoi.

Tai lieu nay thay the ban cu dung `OZ/NC/HPZ` lam nhan chinh. Huong hien tai khong phu thuoc map va khong chay scenario tu dong bang file waypoint/scenario automation. Moi run duoc thuc hien thu cong theo 5 kich ban S1-S5.

---

## 1. Nguyen tac dataset

- **S1-S5 la dataset/scenario class o cap run**: moi lan record phai gan dung mot class S1-S5 trong metadata.
- **Khong can gan nhan thu cong tung frame**: nhan chinh duoc lay tu run metadata, cac tin hieu lien tuc duoc tinh tu sensor/controller log.
- **Thu thap thu cong, khong tu dong hoa theo map**: hien chua co map hop le, nen khong dung waypoint/scenario automation.
- **Backend phai tra telemetry realtime** de quan sat tren UI sau nay, dong thoi cho phep tai dataset `.zip`.
- **Discrete `OZ/NC/HPZ` chi la legacy/debug label tuy chon**, khong phai nhan chinh cua dataset va khong phai thuat toan chinh.

---

## 2. Kich ban bat buoc S1-S5

| Class | Ten metadata | Muc dich | Cach chay |
|---|---|---|---|
| S1 | `S1_open_zone` | Khong gian mo, it vat can, it tuong tac nguoi | Dieu khien robot di tu diem A den B trong vung rong |
| S2 | `S2_narrow_corridor` | Hanh lang/hep, rang buoc vat can tinh | Dieu khien qua loi hep bang tay hoac teleop co kiem soat |
| S3 | `S3_human_proximate` | Robot di gan nguoi dung yen/cham | Actor dung/ngoi/di cham gan duong di robot |
| S4 | `S4_dynamic_crossing` | Nguoi cat ngang duong robot | Actor di cat ngang voi toc do khac nhau |
| S5 | `S5_occlusion_sudden_appearance` | Nguoi xuat hien bat ngo sau che khuat | Actor xuat hien tu sau vat che/nga re |

Moi run phai co:

- `scenario_id`: `S1` den `S5`
- `scenario_class`: mot trong 5 ten tren
- `environment`: `real` hoac `sim`
- `controller`: `dwa`, `teb`, `nmpc`, `discrete_canmpc`, `cca_nmpc`
- `operator`, `actor_count`, `notes`
- `start_time`, `stop_time`, `duration_s`

---

## 3. Mo hinh du lieu dieu khien

### 3.1 Robot state va control

Robot Mecanum can du lieu theo khong gian van toc 3 bac tu do:

```text
P_r = [x_r, y_r, theta_r]^T
u   = [v_x, v_y, omega]^T
```

Trong do:

- `x_r, y_r`: vi tri robot trong frame odom/map khi co.
- `theta_r`: heading.
- `v_x, v_y`: van toc tinh tien theo frame robot.
- `omega`: van toc quay yaw.

### 3.2 Human state va prediction

Moi human detection/track can luu:

```text
S_h = [x_h, y_h, v_hx, v_hy]^T
```

Du bao tuyen tinh ngan han:

```text
P_hat_h,k+i = [x_hat_h,k, y_hat_h,k]^T + i T_s [v_hat_hx,k, v_hat_hy,k]^T
```

Dataset can luu duoc:

- human position trong frame robot/odom.
- human velocity estimate.
- track id neu co.
- confidence/detection source.
- predicted trajectory neu controller publish.

### 3.3 Continuous context score

Context score chinh la tin hieu lien tuc:

```text
phi_hat_h = 1 / (1 + exp(beta (d_h - d0)))
```

Trong do:

- `d_h`: khoang cach robot-human gan nhat.
- `d0`: nguong chuyen tiep.
- `beta`: do doc cua ham sigmoid.

Thong so adaptive can log:

```text
Q(phi)          = Q0 + phi Qh
d_safe(phi)     = d_min + k_d phi
vx_max(phi)     = vx0 - kx phi
vy_max(phi)     = vy0 - ky phi
omega_max(phi)  = omega0 - komega phi
```

---

## 4. ROS2 topics bat buoc record

### 4.1 Sensor va robot feedback

- `/scan`
- `/scan_filtered`
- `/camera/color/image_raw` hoac topic RGB tuong ung
- `/camera/depth/image_raw` hoac topic depth tuong ung
- `/camera/color/camera_info`
- `/tf`
- `/tf_static`
- `/odom`
- `/imu/data_raw`
- `/cmd_vel`
- `/joint_states`
- `/wheel_encoders`
- `/voltage`

### 4.2 CANMPC/CCA-NMPC debug topics

- `/canmpc/context`
- `/canmpc/humans`
- `/canmpc/adaptive_bounds`
- `/canmpc/local_reference_path`
- `/canmpc/predicted_trajectory`
- `/canmpc/solver_stats`

### 4.3 Costmap/local planning topics

- `/local_costmap/costmap`
- `/local_costmap/costmap_updates`
- `/local_costmap/published_footprint`

Neu topic ten khac theo launch thuc te, metadata phai ghi alias ro rang.

---

## 5. Cau truc dataset

```text
~/rai_datasets/canmpc/
  raw/
    real/
      S1/
        cca_nmpc/
          run_000/
            rosbag2/
            metadata.json
      S2/
      S3/
      S4/
      S5/
    sim/
      S1/
      S2/
      S3/
      S4/
      S5/
  derived/
    real/
      runs.parquet
      samples.parquet
      metrics.csv
    sim/
      runs.parquet
      samples.parquet
      metrics.csv
```

Moi `metadata.json` toi thieu:

```json
{
  "schema_version": "canmpc_dataset_v2",
  "robot_brand": "rai",
  "environment": "real",
  "scenario_id": "S3",
  "scenario_class": "S3_human_proximate",
  "controller": "cca_nmpc",
  "manual_collection": true,
  "map_required": false,
  "topics": [],
  "metrics": {}
}
```

---

## 6. Realtime backend va tai dataset

`rai_web_api` phai ho tro quan sat realtime trong luc record:

- `GET /api/telemetry/current`: snapshot telemetry moi nhat.
- `GET /api/dataset/active`: trang thai run dang record.
- `WS /api/ws/telemetry`: stream robot/sensor/controller telemetry.
- `WS /api/ws/dataset`: stream dataset collection event va sample preview.
- `GET /api/dataset/runs/{run_id}/metrics`: metric dinh luong cua run.
- `GET /api/dataset/download/{run_id}`: tai mot run dang `.zip`.

Yeu cau zip:

- Giu nguyen `metadata.json`.
- Giu rosbag2 raw.
- Co the kem derived CSV/Parquet neu da tao.
- Khong doi ten class S1-S5 khi export.

---

## 7. Metric dinh luong bat buoc

Moi run can tinh va luu cac nhom metric sau. Neu metric nao chua tinh duoc realtime, de `null` va bo sung o buoc derived dataset.

### 7.1 Tracking

- `rmse_xy`: RMSE theo vi tri so voi reference path.
- `rmse_theta`: RMSE heading.
- `max_lateral_error`: sai so ngang lon nhat.

Tracking co the `null` trong live collection neu chua co reference trajectory.

### 7.2 Safety

- `d_min`: khoang cach human-robot nho nhat.
- `d_avg`: khoang cach trung binh.
- `d_5percentile`: phan vi 5% cua khoang cach.
- `violation_count`: so lan `d_h < d_safe`.
- `violation_duration`: tong thoi gian vi pham khoang cach an toan.
- `collision_count`: so va cham/gan va cham duoc annotate hoac detect.

### 7.3 Smoothness

- `jerk_mean`: jerk trung binh.
- `jerk_max`: jerk lon nhat.
- `mean_abs_delta_u`: trung binh `|Delta u|`.
- `max_abs_delta_u`: lon nhat `|Delta u|`.

### 7.4 Control effort

- `control_effort`: tich phan/rms cua `v_x^2 + v_y^2 + omega^2`.
- `mean_abs_vx`
- `mean_abs_vy`
- `mean_abs_omega`

### 7.5 Realtime solver

- `solve_time_mean_ms`
- `solve_time_median_ms`
- `solve_time_p95_ms`
- `solve_time_max_ms`
- `timeout_rate`

---

## 8. Quy mo du lieu toi thieu

### 8.1 Real robot

Cho controller de xuat `cca_nmpc`:

```text
5 scenarios x 20 runs/scenario = 100 real runs toi thieu
```

Neu co baseline, uu tien:

1. `cca_nmpc`: full raw sensor + metrics.
2. `discrete_canmpc`: full metrics, raw sensor neu du dung luong.
3. `nmpc`, `teb`, `dwa`: metrics + rosbag toi thieu; camera raw co the giam tan so.

### 8.2 Gazebo replay/simulation

Gazebo khong thay the du lieu robot that. No dung de:

- Mo phong lai cung flow S1-S5 dua tren du lieu/kich ban thu that.
- So sanh controller trong dieu kien lap lai duoc.
- Tao them ablation va stress cases.

Muc tieu:

```text
5 scenarios x 20 runs/scenario/controller neu kha thi
```

Neu gioi han thoi gian, giu so run Gazebo khop voi real runs cua `cca_nmpc`.

---

## 9. Checklist thu thap

- [ ] Chay dung mot trong 5 class S1-S5 cho moi run.
- [ ] Metadata co `scenario_id`, `scenario_class`, `environment`, `controller`.
- [ ] Record du cac topic sensor, robot feedback, CCA-NMPC debug.
- [ ] Backend tra realtime telemetry trong luc record.
- [ ] Stop run sinh `metadata.json` co metrics nested schema.
- [ ] Co the tai run thanh `.zip`.
- [ ] Derived tooling tao duoc CSV/Parquet.
- [ ] Metric safety/control/smoothness/solve time khong rong voi run hop le.
- [ ] Tracking metric duoc tinh khi co reference path.
- [ ] Khong phu thuoc map hoac file waypoint/scenario automation.

---

## 10. Lien ket voi bai bao

Dataset nay dung de chung minh:

- Continuous context adaptation tot hon discrete CA-NMPC.
- Robot that va Gazebo co cung protocol S1-S5.
- CCA-NMPC cai thien safety, smoothness, tracking va realtime feasibility.
- Ket qua co metric dinh luong ro: tracking error, minimum human-robot distance, violation count, smoothness, control effort, solve time.
