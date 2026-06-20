# RAI Navigation Contract

`rai_navigation` is the RAI navigation stack bringup package. It plays the same role as a small `navigation2-humble` style stack, but it is implemented from scratch and does not depend on Nav2 packages, Nav2 actions, Nav2 lifecycle nodes, Nav2 costmap, or Nav2 controller plugins.

For static scanned-map navigation, `rai_map_server.py` replaces `nav2_map_server`: it reads a ROS map `.yaml` plus image file and publishes `/map` as `nav_msgs/OccupancyGrid` with transient-local QoS.

## Packages

- `rai_navigation`: stack bringup, default parameters, public topic/service contract.
- `rai_controller`: navigation/controller server, topic/service contract, and algorithm selection.
- `rai_controller_cca_nmpc`: CCA-NMPC controller algorithm package.
- `rai_planner`: planner interface and shared utilities.
- `rai_planner_a_star`, `rai_planner_dijkstra`, `rai_planner_straight_line`: planner algorithm packages.

## Main Topics

```text
/goal_pose                       geometry_msgs/PoseStamped
/map                             nav_msgs/OccupancyGrid
/scan_filtered                   sensor_msgs/LaserScan
/imu/data                        sensor_msgs/Imu
/odom_combined                   nav_msgs/Odometry
/cmd_vel                         geometry_msgs/Twist
/rai_navigation/status           std_msgs/String
/rai_navigation/global_path      nav_msgs/Path
/rai_navigation/local_costmap    nav_msgs/OccupancyGrid
/canmpc/local_reference_path     nav_msgs/Path
/canmpc/predicted_trajectory     nav_msgs/Path
/canmpc/solver_stats             rai_controller/SolverStats
/canmpc/adaptive_bounds          rai_controller/AdaptiveBounds
```

## Main Services

```text
/rai_navigation/navigate_to_pose    rai_controller/srv/NavigateToPose
/rai_navigation/cancel              std_srvs/srv/Trigger
/rai_navigation/clear_costmap       std_srvs/srv/Trigger
/rai_navigation/clear_local_costmap std_srvs/srv/Trigger
/rai_navigation/clear_global_costmap std_srvs/srv/Trigger
/rai_navigation/replan              std_srvs/srv/Trigger
/rai_navigation/pause               std_srvs/srv/Trigger
/rai_navigation/resume              std_srvs/srv/Trigger
/rai_navigation/get_status          rai_controller/srv/GetNavigationStatus
/rai_navigation/get_path            rai_controller/srv/GetPath
/rai_navigation/get_local_costmap   rai_controller/srv/GetCostmap
/rai_navigation/list_algorithms     rai_controller/srv/ListAlgorithms
/rai_navigation/set_controller      rai_controller/srv/SetController
/rai_navigation/set_planner         rai_controller/srv/SetPlanner
```

## States

```text
IDLE
PLANNING
CONTROLLING
SUCCEEDED
FAILED
CANCELED
PAUSED
```

## Algorithms

Controllers:

```text
CCA_NMPC
```

Global planners:

```text
A_STAR
DIJKSTRA
STRAIGHT_LINE
```

Future controller algorithms should live in their own `rai_controller_<algorithm>` package and be exposed through `/rai_navigation/list_algorithms` and `/rai_navigation/set_controller`. Future planner algorithms should live in their own `rai_planner_<algorithm>` package and be exposed through `/rai_navigation/set_planner` or `global_planner_algorithm`.
