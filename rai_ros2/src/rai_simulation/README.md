# RAI Simulation Automation & Repeatable Scenarios (Phases 8 & 9)

This package contains the simulation worlds, actor definitions, and the automated batch runner for evaluating the **Continuous Context-Adaptive CA-NMPC (CCA-NMPC)** algorithm.

## Package Structure

```text
rai_ros2/src/rai_simulation/
├── rai_gazebo_worlds/
│   └── worlds/
│       ├── s1_open_zone.world         # Scenario 1: Open zone area
│       ├── s2_narrow_corridor.world   # Scenario 2: Narrow corridor boundary walls
│       ├── s3_human_proximity.world  # Scenario 3: Human proximity zone (static human)
│       ├── s4_crossing_path.world     # Scenario 4: Dynamic human crossing path
│       └── s5_occlusion.world         # Scenario 5: Occlusion/sudden appearance area
│
├── rai_human_actors/
│   ├── s4_crossing_human.sdf          # Standing/walking actor with pre-programmed loop
│   └── s5_occluded_human.sdf          # Controlled actor cylinder with velocity control
│
└── rai_scenario_runner/
    └── scenario_runner.py             # Automated scenario batch execution runner script
```

## Running the Automated Batch trials

To run the batch simulation trials automatically (20 runs per scenario, 100 runs total):

```bash
python3 rai_ros2/src/rai_simulation/rai_scenario_runner/scenario_runner.py --controller CCA_NMPC --runs 20
```

### Script Execution Flow

For each run:
1. **Cleanup**: Kills any leftover simulation or ROS 2 nodes to ensure a fresh, clean execution environment.
2. **Jitter Generation**: Generates a randomized starting pose and goal pose using Gaussian noise ($\sigma_{pos} = 0.05\text{ m}$, $\sigma_{yaw} = 0.03\text{ rad}$) for evaluation repeatability.
3. **Simulation Launch**: Launches Gazebo Fortress with the scenario-specific world file.
4. **Robot Spawning**: Spawns the `mini_mec_robot` model from `rai_robot_urdf` at the jittered start pose.
5. **Bridge Launch**: Launches the `ros_gz_bridge` mapping topics (`/clock`, `/scan`, `/odom_combined`, `/cmd_vel`, `/tf`, `/tf_static`).
6. **Context Monitor**: Launches the context estimator that publishes `/canmpc/context` and `/canmpc/humans`.
7. **RAI Navigation**: Launches `rai_navigation`, which consumes `/goal_pose` directly and publishes commands without Nav2.
8. **Telemetry Recording**: Launches `ros2 bag record` to record all target signals:
   - `/scan`
   - `/tf`
   - `/tf_static`
   - `/odom_combined`
   - `/cmd_vel`
   - `/canmpc/context`
   - `/canmpc/humans`
   - `/canmpc/solver_stats`
   - `/canmpc/adaptive_bounds`
   - `/rai_navigation/local_costmap`
9. **Goal Dispatch**: Sends the randomized goal pose to the standalone CCA-NMPC controller.
10. **Dynamic Actor Trigger**: For Scenario 5 (Occlusion), monitors the robot's odometry. When the robot crosses $x \ge 2.0$, it publishes a velocity command to `/model/s5_human/cmd_vel` to move the actor cylinder out from behind the wall at $1.2\text{ m/s}$ towards $y = 5.0$.
11. **Termination**: Monitors the distance to the goal. Terminates and marks as SUCCESS if the robot reaches the goal within $35.0\text{ s}$, or TIMEOUT otherwise.
12. **Metadata Logging**: Saves a `metadata.json` for each trial and appends the trial summary into a global `run_index.csv`.
