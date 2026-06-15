# Implementation Plan: Continuous Context-Adaptive CA-NMPC for Mecanum Human-Aware Navigation

## Goal
Implement a Continuous Context-Adaptive Nonlinear Model Predictive Control (CCA-NMPC) navigation stack for Mecanum robots as a native Nav2 controller plugin, tested in Gazebo Fortress, and optimized for deployment on the NVIDIA Jetson Orin Nano.

---

## 🛠️ Dependency Verification & Setup

Before developing the ROS 2 packages, we must ensure CasADi is correctly set up for both Python (offline solver generation) and C++ (online wrapper, if needed) on the Jetson Orin Nano.

### Setup Check & Installation Steps (to be documented in `docs/casadi_setup.md`):
1. **Python CasADi Verification**:
   - Check if CasADi Python package is available: `python3 -c "import casadi; print(casadi.__version__)"`.
   - If missing, install via pip: `pip3 install casadi` or build from source if on ARM64 Jetson without pre-built wheels.
2. **C++ CasADi Verification**:
   - Check for CasADi headers: `/usr/local/include/casadi/` or `/usr/include/casadi/`.
   - Check for shared libraries: `libcasadi.so`.
   - If missing, compile from source for ARM64:
     ```bash
     git clone https://github.com/casadi/casadi.git -b 3.6.3
     cd casadi && mkdir build && cd build
     cmake -DWITH_PYTHON=ON -DWITH_COPASI=OFF -DWITH_CLANG=OFF ..
     make -j$(nproc)
     sudo make install
     ```

---

## 📋 Phased Implementation Tasks

### Task 0: Documentation & Dependency Setup
- [ ] **Task 0.1**: Create `docs/casadi_setup.md` detailing the verification and installation instructions for Python and C++ CasADi on Jetson Orin Nano.
  - *Verify*: Read the file and ensure commands match Jetson Orin Nano (ARM64) architecture.

### Phase 1: Custom Message Definition (`canmpc_msgs`)
- [ ] **Task 1.1**: Create package `canmpc_msgs` with standard ROS 2 message generation scaffolding (`CMakeLists.txt`, `package.xml`).
  - *Verify*: The package builds successfully using `colcon build --packages-select canmpc_msgs`.
- [ ] **Task 1.2**: Define `HumanState.msg` and `HumanStates.msg` for tracking dynamic obstacle positions and velocities.
  - `HumanState.msg` structure:
    ```text
    int32 id
    geometry_msgs/Pose pose
    geometry_msgs/Twist velocity
    float64 confidence
    ```
  - `HumanStates.msg` structure:
    ```text
    std_msgs/Header header
    canmpc_msgs/HumanState[] humans
    ```
  - *Verify*: `ros2 interface show canmpc_msgs/msg/HumanStates` runs successfully.
- [ ] **Task 1.3**: Define `Context.msg` to carry the continuous human-proximity context score and estimated parameters.
  - `Context.msg` structure:
    ```text
    std_msgs/Header header
    float64 phi_h
    float64 nearest_human_dist
    float64 d_safe
    float64 vx_max
    float64 vy_max
    float64 omega_max
    bool occlusion_flag
    ```
  - *Verify*: `ros2 interface show canmpc_msgs/msg/Context` runs successfully.
- [ ] **Task 1.4**: Define `SolverStats.msg` and `AdaptiveBounds.msg` for optimization telemetry and parameter tracking.
  - `SolverStats.msg` structure:
    ```text
    std_msgs/Header header
    float64 solve_time_ms
    int32 iter_count
    string status
    bool timeout_flag
    bool collision_flag
    ```
  - `AdaptiveBounds.msg` structure:
    ```text
    std_msgs/Header header
    float64 vx_max
    float64 vy_max
    float64 omega_max
    float64 d_safe
    ```
  - *Verify*: `ros2 interface show canmpc_msgs/msg/SolverStats` runs successfully.

### Phase 2: Controller Skeleton (`nav2_canmpc_controller`)
- [ ] **Task 2.1**: Scaffolding of the `nav2_canmpc_controller` package:
  - Create package directories: `include/nav2_canmpc_controller/`, `src/`, `generated/`, `scripts/`.
  - Create `package.xml` specifying dependencies: `rclcpp`, `nav2_core`, `nav2_costmap_2d`, `geometry_msgs`, `nav_msgs`, `tf2_ros`, `pluginlib`, `canmpc_msgs`.
  - Create `CMakeLists.txt` exporting the library and declaring plugin registration.
  - Create plugin declaration file `nav2_canmpc_controller.xml`.
  - *Verify*: Library skeleton builds with `colcon build --packages-select nav2_canmpc_controller` and is detected as a `nav2_core` plugin via `ros2 pkg plugins --package nav2_canmpc_controller nav2_core`.
- [ ] **Task 2.2**: Implement `CANMPCController` skeleton class:
  - Inherit from `nav2_core::Controller`.
  - Define method overrides:
    ```cpp
    void configure(const rclcpp_lifecycle::LifecycleNode::WeakPtr & parent, std::string name, std::shared_ptr<tf2_ros::Buffer> tf, std::shared_ptr<nav2_costmap_2d::Costmap2DROS> costmap_ros) override;
    void cleanup() override;
    void activate() override;
    void deactivate() override;
    void setPlan(const nav_msgs::msg::Path & path) override;
    geometry_msgs::msg::TwistStamped computeVelocityCommands(const geometry_msgs::msg::PoseStamped & pose, const geometry_msgs::msg::Twist & velocity, nav2_core::GoalChecker * goal_checker) override;
    void setSpeedLimit(const double & speed_limit, const bool & percentage) override;
    ```
  - Add ROS 2 parameter declarations (`horizon_steps`, `model_dt`, `max_solver_time_ms`, etc.).
  - *Verify*: The plugin skeleton initializes within the Nav2 controller server lifecycle without crashes.

### Phase 3: Mecanum Model and Local Reference
- [ ] **Task 3.1**: Implement `MecanumModel` kinematic integration class (`mecanum_model.hpp`/`.cpp`):
  - Predict next state $P_{r,k+1} = f_d(P_{r,k}, u_k)$ using the 4th-order Runge-Kutta (RK4) integration scheme.
  - Kinematics equations:
    $$\dot{x}_r = v_x \cos\theta_r - v_y \sin\theta_r$$
    $$\dot{y}_r = v_x \sin\theta_r + v_y \cos\theta_r$$
    $$\dot{\theta}_r = \omega$$
  - *Verify*: Unit test verifies state prediction matches analytical integration for straight, lateral, and rotational motions.
- [ ] **Task 3.2**: Implement `LocalPlanHandler` path manager class (`local_plan_handler.hpp`/`.cpp`):
  - Ingest global path from `setPlan()`.
  - Find the closest point to current robot pose and prune historical path nodes.
  - Crop global path to local costmap bounding box.
  - Resample plan based on target velocity $v_{ref} \cdot T_s$ or arclength.
  - Compute heading (yaw) values from path tangent if heading is omitted in the global path.
  - *Verify*: Unit test verifies correct cropping, index matching, and resampling on synthetic straight and curved paths.

### Phase 4: CasADi Solver C Generation & Wrapper
- [ ] **Task 4.1**: Create `scripts/generate_solver.py` for offline CasADi NLP generation:
  - Formulate the optimization variables: states $P_r$, inputs $u$, slacks $s$.
  - Define the prediction model constraints using the RK4 Mecanum kinematics.
  - Define the cost function tracking error relative to $\mathcal{P}_{ref}$:
    $$J = \sum_{i=0}^{N-1} [ e_{k+i}^T Q(\hat{\phi}_{h,k+i}) e_{k+i} + u_{k+i}^T R u_{k+i} + \Delta u_{k+i}^T R_d \Delta u_{k+i} ] + e_{k+N}^T Q_f e_{k+N}$$
  - Define continuous context score sigmoid:
    $$\hat{\phi}_{h,k+i} = \frac{1}{1 + \exp(\beta (d_{h,k+i} - d_0))}$$
  - Define adaptive bounds and parameters: $Q(\hat{\phi})$, $d_{safe}(\hat{\phi})$, and bounds on inputs.
  - Call CasADi code generator to output C source files `generated/canmpc_solver.c` (and headers).
  - *Verify*: Running `python3 scripts/generate_solver.py` generates compilable C code.
- [ ] **Task 4.2**: Implement `SolverWrapper` C++ interface:
  - Include and load the compiled `canmpc_solver.c` symbols.
  - Map variables between C++ structures and C solver arrays (states, parameters, references, constraints, slacks).
  - Manage warm-starting by caching the previous optimal control and state sequence.
  - *Verify*: C++ wrapper compiles successfully and executes a single dummy optimization run.

### Phase 5: Full NMPC Controller Integration
- [ ] **Task 5.1**: Integrate perception and estimation feeds:
  - Subscribe to `/canmpc/humans` and `/canmpc/context` inside the controller node.
  - Extract local costmap representation and implement `CostmapEvaluator` to compute obstacle cost maps or distance fields.
  - *Verify*: Callback metrics show correct parsing of humans and context topics.
- [ ] **Task 5.2**: Implement full control loop in `CANMPCController::computeVelocityCommands()`:
  - Prune and resample reference trajectory using `LocalPlanHandler`.
  - Extrapolate human states along horizon using constant-velocity model.
  - Compute adaptive constraints and weights based on dynamic human positions.
  - Evaluate obstacle proximity and populate solver inputs.
  - Invoke `SolverWrapper` with warm start.
  - Fallback logic: reject solutions violating footprint constraints or exceeding max solver time.
  - Package the first optimal input command $u_k^*$ as `geometry_msgs::msg::TwistStamped`.
  - Publish debugging information: `/canmpc/local_reference_path`, `/canmpc/predicted_trajectory`, `/canmpc/solver_stats`.
  - *Verify*: Build runs, robot follows path in simulation, and debug topics display properly in RViz.

### Phase 6: Human Semantic Costmap Layer
- [ ] **Task 6.1**: Implement `HumanSemanticLayer` plugin (`human_semantic_layer.hpp`/`.cpp`):
  - Inherit from `nav2_costmap_2d::CostmapLayer`.
  - Subscribe to `/canmpc/humans`.
  - Render dynamic high-cost zones around current and predicted human coordinates.
  - Add occlusion/sudden-appearance buffers to the costmap values.
  - *Verify*: Colcon compiles the layer. Register and load it in local costmap plugins, confirming RViz visualizes dynamic cost rings around human positions.

### Phase 7: Fallback and Safety Behavior
- [ ] **Task 7.1**: Implement safety and recovery strategies:
  - Verify computed velocities are finite and non-NaN.
  - If solver fails or times out, execute the previous safe control input (if valid) or step down to a zero-velocity halt.
  - Raise `nav2_core::ControllerException` if solver failure persists, delegating recovery to Nav2 tree.
  - Integrate `setSpeedLimit()` limits with the adaptive bounds (stricter limit prevails).
  - *Verify*: Simulated failure injection (mocking solver timeouts or sudden close obstacles) triggers correct safety limits and zero-velocity halts.

### Phase 8: Gazebo Fortress Experimental Design
- [ ] **Task 8.1**: Create warehouse simulation world:
  - Define world model `rai_ros2/src/rai_simulation/rai_gazebo_worlds/worlds/warehouse_rai.world`.
  - Include structural zones: Open Area (S1), Narrow Corridor (S2), Human Proximity Zone (S3).
  - Program repeatable pedestrian behaviors: constant velocity crosser (S4), corner-hidden sudden appearance actor (S5).
  - *Verify*: Launch world in Gazebo and check that actors follow pre-programmed paths with correct timing.

### Phase 9: Data Collection & Simulation Trials
- [ ] **Task 9.1**: Program batch runner script for automation:
  - Automate 20 runs for each scenario S1-S5 (100 runs total).
  - Support comparisons: DWA, TEB, standard NMPC, Discrete CA-NMPC, and Continuous Context-Adaptive CA-NMPC.
  - Jitter starting poses, goal poses, and pedestrian appearance timings to ensure robust testing.
  - Record data to rosbag2 using optimized topic recording profile.
  - *Verify*: Dry run for a single scenario runs successfully and saves bag.
- [ ] **Task 9.2**: Implement dataset compilation scripts (`rai_dataset_tools`):
  - Extract telemetry data from rosbag2 files.
  - Compile data into derived schema tabular parquet files: tracking error, proximity context, solver stats, etc.
  - Split dataset by run identifier into: Train (70%), Val (15%), Test (15%).
  - *Verify*: Parquet extraction runs without NaNs/errors and files pass quality check.

---

## 🎯 Verification Criteria & Success Definition

1. **Compilation**: Standard `colcon build` builds all packages successfully.
2. **Real-time Performance**: Average solver time stays $< 35 \text{ ms}$ (hard limit at $50 \text{ ms}$) on Jetson Orin Nano.
3. **Mecanum Motion**: Controller makes full use of 3-DOF MIMO outputs (including lateral motion $v_y$).
4. **Context Adaptation**: $\hat{\phi}_{h}(t)$ behaves continuously in $[0, 1]$ and adapts $Q$, $d_{safe}$, and $v_{max}$ values dynamically.
5. **No Collisions**: Zero collisions occur over 100 runs in simulated scenarios S1-S4.
6. **Data Output**: Tabular derived dataset matches the required schema and contains 100 runs of simulation history.

---

## ⚠️ Key Risks and Mitigations

| Risk | Impact | Mitigation |
| :--- | :--- | :--- |
| NLP solver infeasibility near sudden obstacles | Controller crash / collision | Add slack variables to human distance constraints; default to soft constraints with high penalties and include a kinematic fallback halt. |
| Solver computation exceeds cycle time (100ms) | Loop lag / control delay | Offline C code generation (removes CasADi runtime overhead), warm-starting solver, and tuning horizon length ($N$). |
| Noisy pedestrian state estimation | Erratic adaptation and jerky commands | Apply Kalman filtering to human position measurements, filter velocity estimation, and clamp adaptive bound variations. |
| Discontinuous costmap queries | Poor solver convergence | Use smooth distance approximation functions or potential fields rather than raw cell costs directly in the solver optimization. |
