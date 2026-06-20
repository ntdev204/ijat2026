# RAI Controller Server Model

This package owns the RAI controller server. It is not a Nav2 controller plugin and does not depend on Nav2 actions, Nav2 lifecycle nodes, Nav2 controller server, or Nav2 MPPI.

`CCA_NMPC` is the first controller algorithm package and lives in `rai_controller_cca_nmpc`. `rai_controller` owns the server contract and registry wiring, not the algorithm implementation.

Controller selection flow:

```text
rai_navigation launch / navigate_to_pose / set_controller
        -> rai_controller_server
        -> active controller implementation package, e.g. rai_controller_cca_nmpc
        -> cmd_vel_topic
```

## Runtime Contract

Inputs:
- Robot pose from TF: `control_frame -> base_frame`
- Robot velocity from `odom_topic`
- Scanned occupancy map from `map_topic`
- Laser scan from `laser_scan_topic`
- IMU from `imu_topic`
- Goal from `goal_topic`, planned by the selected standalone global planner
- Optional external reference from `external_path_topic`
- Optional context from `/canmpc/context`
- Optional humans from `/canmpc/humans`
- Optional occupancy grid from `costmap_topic`
- Cancel command from `cancel_topic` using `std_msgs/Empty`
- Clear local obstacle command from `clear_costmap_topic` using `std_msgs/Empty`
- Replan command from `replan_topic` using `std_msgs/Empty`
- Navigate service from `navigate_to_pose_service`
- Algorithm selection services from `set_controller_service` and `set_planner_service`

Outputs:
- Velocity command on `cmd_vel_topic`
- Global path on `global_path_topic`
- Local reference path on `/canmpc/local_reference_path`
- Predicted trajectory on `/canmpc/predicted_trajectory`
- Local LiDAR obstacle grid on `local_grid_topic`
- Solver stats on `/canmpc/solver_stats`
- Adaptive bounds on `/canmpc/adaptive_bounds`
- Navigation state on `status_topic` using `std_msgs/String`

Services:
- `navigate_to_pose_service`: `rai_controller/srv/NavigateToPose`
- `cancel_service`: `std_srvs/srv/Trigger`
- `clear_costmap_service`: `std_srvs/srv/Trigger`
- `clear_local_costmap_service`: `std_srvs/srv/Trigger`
- `clear_global_costmap_service`: `std_srvs/srv/Trigger`
- `replan_service`: `std_srvs/srv/Trigger`
- `pause_service`: `std_srvs/srv/Trigger`
- `resume_service`: `std_srvs/srv/Trigger`
- `get_status_service`: `rai_controller/srv/GetNavigationStatus`
- `get_path_service`: `rai_controller/srv/GetPath`
- `get_costmap_service`: `rai_controller/srv/GetCostmap`
- `list_algorithms_service`: `rai_controller/srv/ListAlgorithms`
- `set_controller_service`: `rai_controller/srv/SetController`
- `set_planner_service`: `rai_controller/srv/SetPlanner`

## Nav2-Like Standalone Behavior

The node intentionally mirrors the operating shape of Nav2 without linking against Nav2. It is an independent ROS2 controller/navigation server made from:

```text
goal receiver -> global planner -> local CCA-NMPC controller -> cmd_vel
                    ^                    |
                    |                    v
             map / replan       scan + map + IMU fusion
```

Runtime states are published as strings:

```text
IDLE         ready or waiting for a path
PLANNING     a new goal is being converted into a global path
CONTROLLING  the local CCA-NMPC controller is tracking the active path
SUCCEEDED    robot pose is within goal_tolerance_xy and goal_tolerance_yaw
FAILED       required pose/path/solver condition failed
CANCELED     cancel_topic was received and the robot was stopped
```

Control topics:

```text
goal_topic            geometry_msgs/PoseStamped
cancel_topic          std_msgs/Empty
clear_costmap_topic   std_msgs/Empty
replan_topic          std_msgs/Empty
status_topic          std_msgs/String
```

`cancel_topic` clears the active path and goal, resets the CCA-NMPC core, publishes zero velocity, and reports `CANCELED`.
`clear_costmap_topic` clears only the rolling external/LiDAR obstacle grids; it does not erase the scanned map from `map_topic`.
`replan_topic` runs the selected global planner again from the current robot pose to the active goal.
When the robot reaches the active goal, the node stops the robot, clears the active path, and reports `SUCCEEDED`.

## State And Control

The robot state is:

```text
x_k = [p_x, p_y, theta]^T
u_k = [v_x, v_y, omega]^T
```

The mecanum rollout model is:

```text
theta_{k+1} = wrap(theta_k + omega_k dt)
p_x{k+1} = p_x{k} + (v_x cos(theta_k + 0.5 omega_k dt) - v_y sin(theta_k + 0.5 omega_k dt)) dt
p_y{k+1} = p_y{k} + (v_x sin(theta_k + 0.5 omega_k dt) + v_y cos(theta_k + 0.5 omega_k dt)) dt
```

## Human Context Adaptation

For each human `i`, constant-velocity prediction is:

```text
h_i(t) = [h_x + v_x t, h_y + v_y t]^T
d_h(x, t) = min_i || [p_x, p_y]^T - h_i(t) ||
phi_h = 1 / (1 + exp(beta (d_h - d0)))
```

`phi_h` is clamped to `[0, 1]`. Larger `phi_h` means higher human proximity risk.

Adaptive limits are:

```text
d_safe(phi_h) = d_safe_0 + (d_safe_max - d_safe_0) phi_h
v_x_max(phi_h) = v_x_min + (v_x_0 - v_x_min) (1 - phi_h)
v_y_max(phi_h) = v_y_min + (v_y_0 - v_y_min) (1 - phi_h)
omega_max(phi_h) = omega_min + (omega_0 - omega_min) (1 - phi_h)
```

External context can only tighten velocity limits or increase the safe distance.

## Sensor Obstacle Model

## Resource-Bounded Improvements

The controller avoids a full Nav2 dependency, but it no longer uses an all-or-nothing brute-force grid.

Sampling is resource bounded:

```text
seed controls: stop, previous command, path-following nominal command
coarse pass: small deterministic grid sorted by distance to nominal command
refinement pass: small local grids around previous and nominal commands
deadline: stop sampling before max_compute_time_ms - timeout_margin_ms
```

This keeps useful candidates near the front of the queue, so weak CPUs can return a feasible command before exhausting a large Cartesian grid. Setting `adaptive_sampling: false` restores the old full-grid behavior.

The local costmap is a lightweight layered grid:

```text
static layer   scanned map projected from map_topic
sensor layer   fresh LaserScan obstacle points
external layer optional costmap_topic
merge rule     max cell cost
```

Each obstacle marks `robot_radius` as the lethal body footprint, then adds an inscribed/inflated band and a wider slowdown band. Runtime clamps ensure `global_planner_inflation_radius`, `obstacle_lethal_radius`, and `obstacle_inflation_radius` cannot be smaller than the robot body radius. The CCA-NMPC core rejects cells above `costmap_collision_threshold` and treats lower values as soft costs. This gives a gradient around complex obstacles without implementing Nav2 Costmap2D plugins.

Recovery is a small state machine, not a Behavior Tree dependency:

```text
NO_VALID_TRAJECTORY
  -> clear local obstacle grid and replan
  -> short backup command
  -> short spin command
  -> FAILED after recovery_max_attempts
```

Recovery publishes `RECOVERING` and keeps the active goal/path, so a temporary local blockage does not immediately end navigation.

## Global Planning

CCA-NMPC is the predictive controller selected through `rai_controller_cca_nmpc`. Path planning lives in `rai_planner` plus per-algorithm packages (`rai_planner_a_star`, `rai_planner_dijkstra`, `rai_planner_straight_line`) instead of being embedded in `rai_controller_server.cpp`.

The selected algorithm is controlled by `global_planner_algorithm`:

```text
A_STAR        grid A* with Euclidean heuristic
DIJKSTRA      grid Dijkstra, equivalent to A* with zero heuristic
STRAIGHT_LINE direct reference segment, used as fallback
```

When a goal arrives, the controller transforms the robot pose and goal into the map frame, creates the selected planner from `rai_planner`, asks it to plan with `global_planner_inflation_radius`, publishes the result on `global_path_topic`, then transforms a copy into `control_frame` for the CCA-NMPC tracker. If no map is available or the selected planner fails, the controller falls back to `STRAIGHT_LINE` so the controller still has a reference.

For scanned-map navigation, the standalone node subscribes to `nav_msgs/OccupancyGrid` on `map_topic`. The map is normally expressed in `global_frame`; at each control tick, the node projects the map cells around the robot into a rolling local grid in `control_frame`:

```text
g_local = centered_grid(robot_pose, local_grid_width_m, local_grid_height_m)
p_map = T_map_control p_local
g_local(p_local) = occupied if map(p_map) >= map_occupied_threshold
```

Unknown map cells are treated as free by default and can be made blocking with `treat_unknown_as_obstacle`.

The standalone node subscribes directly to `sensor_msgs/LaserScan`. Each finite scan point is transformed from the scan frame into `control_frame`, then inserted into a rolling local occupancy grid centered on the current robot pose:

```text
z_i = [range_i cos(angle_i), range_i sin(angle_i)] in scan frame
o_i = T_control_scan z_i
grid(o_i) = occupied
lethal(grid, robot_radius)
inflate(grid, obstacle_inflation_radius)
slowdown(grid, slowdown_inflation_radius)
```

The LiDAR grid is merged with the projected map grid and published on `local_grid_topic`. During rollout, predicted states entering occupied or unknown/out-of-bounds cells are rejected with the same high slack penalty used for collision constraints.

The IMU is used as a sensor fallback for yaw-rate:

```text
omega_measured = imu.angular_velocity.z
```

when the IMU sample is fresh and enabled. This improves short-horizon prediction when odometry angular velocity is missing, delayed, or noisy.

## Objective

The controller samples feasible constant controls over the prediction horizon and minimizes:

```text
J = sum_k [
  q_x(phi_h) (p_x - r_x)^2
  + q_y(phi_h) (p_y - r_y)^2
  + q_theta(phi_h) wrap(theta - r_theta)^2
  + r_vx v_x^2 + r_vy v_y^2 + r_omega omega^2
  + rd_vx (v_x - v_x_prev)^2
  + rd_vy (v_y - v_y_prev)^2
  + rd_omega (omega - omega_prev)^2
  + human_cost
  + costmap_cost
] + terminal_cost
```

with:

```text
q_x(phi_h) = q_x + phi_h q_human_x
q_y(phi_h) = q_y + phi_h q_human_y
q_theta(phi_h) = q_theta + phi_h q_human_theta
human_cost = human_cost_weight (d0 - d_h)^2, when d_h < d0
costmap_cost = costmap_cost_weight * occupancy_value
terminal_cost = terminal_cost_weight * distance(predicted_terminal, reference_terminal)
```

Hard constraints are enforced with rejection or slack penalties:

```text
|v_x| <= v_x_max(phi_h)
|v_y| <= v_y_max(phi_h)
|omega| <= omega_max(phi_h)
d_h >= d_safe(phi_h)
occupancy_value < occupied_threshold
```

The selected first control sample is published, and the previous selected command is kept for smoothness in the next solve.
