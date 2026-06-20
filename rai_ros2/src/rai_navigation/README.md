# RAI Navigation Stack

This directory is the standalone RAI navigation stack, packaged like a small `navigation2-humble` source tree but without Nav2 dependencies.

Packages:

- `rai_navigation`: bringup, stack-level launch files, default parameters, public navigation contract.
- `rai_controller`: controller server, controller services/messages, and algorithm selection.
- `rai_controller_cca_nmpc`: CCA-NMPC controller algorithm package.
- `rai_planner`: planner interface and shared map-grid utilities.
- `rai_planner_a_star`: A* planner algorithm package.
- `rai_planner_dijkstra`: Dijkstra planner algorithm package.
- `rai_planner_straight_line`: straight-line planner algorithm package.

Build from the ROS2 workspace root:

```bash
colcon build --packages-select rai_navigation rai_controller rai_controller_cca_nmpc rai_planner rai_planner_a_star rai_planner_dijkstra rai_planner_straight_line
```

Launch the stack:

```bash
ros2 launch rai_navigation rai_navigation.launch.py controller_id:=CCA_NMPC
```
