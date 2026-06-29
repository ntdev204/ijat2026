# RAI ROS2 — Build Guide

> Workspace: `~/ijat2026/rai_ros2`  
> ROS2 distro: **Humble**  
> Shell: run từ thư mục workspace sau khi `source /opt/ros/humble/setup.bash`

---

## Package Dependency Graph (Local → Local)

```
rai_planner                  ← rai_planner_a_star
                             ← rai_planner_dijkstra
                             ← rai_planner_straight_line
                             ← rai_controller

rai_controller_cca_nmpc      ← rai_controller

rai_controller               ← rai_navigation
                             ← rai_robot_navigation
                             ← rai_human_perception
                             ← rai_dataset_collection
                             ← rai_web_api
                             ← turn_on_rai_robot

rai_navigation               ← rai_robot_navigation
                             ← rai_web_api
                             ← turn_on_rai_robot

rai_robot_msg                ← rai_web_api
                             ← turn_on_rai_robot

rai_slam_toolbox             ← rai_dataset_collection
                             ← turn_on_rai_robot

rai_human_perception         ← rai_dataset_collection

lslidar_msgs                 ← lslidar_driver
lslidar_driver               ← turn_on_rai_robot

astra_camera_msgs            ← astra_camera

serial                       ← turn_on_rai_robot
ackermann_msgs               ← turn_on_rai_robot
turn_on_rai_robot            ← rai_dataset_collection
```

---

## Ghi chú cấu trúc

| Thư mục | Ghi chú |
|---------|---------|
| `rai_navigation/` | Thư mục nhóm chứa 7 sub-packages (rai_planner, rai_controller, ...) |
| `rai_robot_slam/` | Chứa `rai_slam_toolbox/` và `rai_cartographer/` — không có package.xml gốc |
| `rai_robot_urdf/` | `package.xml` nằm ở `rai_robot_urdf/rai_robot_urdf/package.xml` |
| `rai_robot_rtab/` | ⚠️ Không có `package.xml` — chỉ là thư mục `params/` (stub) |
| `rai_scada_bridge/` | ⚠️ Không có `package.xml` — chỉ có `launch/`, `resource/` (config folder) |
| `rai_simulation/` | Chứa 4 sub-packages; `rai_human_actors/` và `rai_scenario_runner/` không có `package.xml` |

---

## 🍓 Raspberry Pi — Robot Base

**Role:** Hardware driver, SLAM, navigation, web API.  
**Không có CUDA** — không build `rai_human_perception`.

```bash
cd ~/ijat2026/rai_ros2

colcon build --symlink-install --packages-select \
  lslidar_msgs \
  lslidar_driver \
  serial \
  ackermann_msgs \
  rai_planner \
  rai_planner_a_star \
  rai_planner_dijkstra \
  rai_planner_straight_line \
  rai_controller_cca_nmpc \
  rai_controller \
  rai_navigation \
  rai_robot_msg \
  rai_robot_urdf \
  rai_slam_toolbox \
  turn_on_rai_robot \
  rai_robot_navigation \
  rai_robot_keyboard \
  rai_web_api
```

**Thứ tự build (colcon tự sắp xếp):**
1. `lslidar_msgs`, `serial`, `ackermann_msgs`, `rai_planner`, `rai_controller_cca_nmpc`, `rai_robot_msg`, `rai_robot_urdf`, `rai_slam_toolbox`
2. `lslidar_driver`, `rai_planner_a_star`, `rai_planner_dijkstra`, `rai_planner_straight_line`
3. `rai_controller`
4. `rai_navigation`, `turn_on_rai_robot`
5. `rai_robot_navigation`, `rai_robot_keyboard`, `rai_web_api`

---

## ⚡ Jetson — AI / Perception

**Role:** YOLO detection, depth association, Kalman tracking, dataset collection.  
**Yêu cầu:** CUDA + TensorRT.

```bash
cd ~/ijat2026/rai_ros2

colcon build --symlink-install --packages-select \
  lslidar_msgs \
  lslidar_driver \
  serial \
  ackermann_msgs \
  rai_planner \
  rai_planner_a_star \
  rai_planner_dijkstra \
  rai_planner_straight_line \
  rai_controller_cca_nmpc \
  rai_controller \
  rai_navigation \
  rai_robot_msg \
  rai_slam_toolbox \
  turn_on_rai_robot \
  rai_human_perception \
  rai_dataset_collection \
  rai_web_api
```

**Thứ tự build (colcon tự sắp xếp):**
1. `lslidar_msgs`, `serial`, `ackermann_msgs`, `rai_planner`, `rai_controller_cca_nmpc`, `rai_robot_msg`, `rai_slam_toolbox`
2. `lslidar_driver`, `rai_planner_a_star`, `rai_planner_dijkstra`, `rai_planner_straight_line`
3. `rai_controller`
4. `rai_navigation`, `turn_on_rai_robot`
5. `rai_human_perception`, `rai_web_api`
6. `rai_dataset_collection`

---

## 💻 Laptop — Dev / Visualization

**Role:** RViz2, simulation, giám sát. Không có hardware driver.

```bash
cd ~/ijat2026/rai_ros2

colcon build --symlink-install --packages-select \
  rai_planner \
  rai_planner_a_star \
  rai_planner_dijkstra \
  rai_planner_straight_line \
  rai_controller_cca_nmpc \
  rai_controller \
  rai_navigation \
  rai_robot_msg \
  rai_robot_urdf \
  rai_robot_navigation \
  rai_rviz2 \
  rai_web_api
```

**Thứ tự build (colcon tự sắp xếp):**
1. `rai_planner`, `rai_controller_cca_nmpc`, `rai_robot_msg`, `rai_robot_urdf`
2. `rai_planner_a_star`, `rai_planner_dijkstra`, `rai_planner_straight_line`
3. `rai_controller`
4. `rai_navigation`
5. `rai_robot_navigation`, `rai_web_api`, `rai_rviz2`

---

## Quick Reference — Package trên từng thiết bị

| Package                     | Pi | Jetson | Laptop | Local Deps |
|-----------------------------|----|--------|--------|------------|
| `lslidar_msgs`              | ✅ | ✅     | ❌     | — |
| `lslidar_driver`            | ✅ | ✅     | ❌     | lslidar_msgs |
| `serial`                    | ✅ | ✅     | ❌     | — |
| `ackermann_msgs`            | ✅ | ✅     | ❌     | — |
| `rai_planner`               | ✅ | ✅     | ✅     | — |
| `rai_planner_a_star`        | ✅ | ✅     | ✅     | rai_planner |
| `rai_planner_dijkstra`      | ✅ | ✅     | ✅     | rai_planner |
| `rai_planner_straight_line` | ✅ | ✅     | ✅     | rai_planner |
| `rai_controller_cca_nmpc`   | ✅ | ✅     | ✅     | — |
| `rai_controller`            | ✅ | ✅     | ✅     | rai_controller_cca_nmpc, rai_planner_* |
| `rai_navigation`            | ✅ | ✅     | ✅     | rai_controller |
| `rai_robot_msg`             | ✅ | ✅     | ✅     | — |
| `rai_robot_urdf`            | ✅ | ❌     | ✅     | — |
| `rai_slam_toolbox`          | ✅ | ✅     | ❌     | — |
| `rai_cartographer`          | ⚠️ | ❌    | ❌     | — |
| `turn_on_rai_robot`         | ✅ | ✅     | ❌     | rai_robot_msg, rai_navigation, rai_controller, lslidar_driver, serial, ackermann_msgs, rai_slam_toolbox |
| `rai_human_perception`      | ❌ | ✅     | ❌     | rai_controller (CUDA required) |
| `rai_robot_navigation`      | ✅ | ❌     | ✅     | rai_navigation, rai_controller |
| `rai_robot_keyboard`        | ✅ | ❌     | ❌     | — |
| `rai_web_api`               | ✅ | ✅     | ✅     | rai_robot_msg, rai_controller, rai_navigation |
| `rai_dataset_collection`    | ❌ | ✅     | ❌     | rai_controller, rai_human_perception, turn_on_rai_robot, rai_slam_toolbox |
| `rai_rviz2`                 | ❌ | ❌     | ✅     | — |

> ⚠️ `rai_cartographer` — có thể build trên Pi nếu cần, không có local deps.

---

## Mẹo colcon

```bash
# Source sau khi build
source install/setup.bash

# Build 1 pkg + TẤT CẢ dependencies của nó (an toàn nhất)
colcon build --symlink-install --packages-up-to <package_name>

# Chỉ build pkg vừa sửa (nhanh)
colcon build --symlink-install --packages-select <package_name>

# Xem đồ thị dependency
colcon graph --packages-select <pkg1> <pkg2>

# Clean toàn bộ
rm -rf build/ install/ log/

# Build song song (giới hạn CPU)
colcon build --symlink-install --parallel-workers 2
```
