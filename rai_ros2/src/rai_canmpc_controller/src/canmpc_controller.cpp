#include "rai_canmpc_controller/canmpc_controller.hpp"
#include "nav2_costmap_2d/cost_values.hpp"
#include "nav2_util/node_utils.hpp"
#include "pluginlib/class_list_macros.hpp"
#include "ament_index_cpp/get_package_share_directory.hpp"
#include "ament_index_cpp/get_package_prefix.hpp"
#include "tf2/utils.h"

#include <chrono>
#include <algorithm>
#include <cmath>

namespace rai_canmpc_controller
{
namespace
{
double finite_or(double value, double fallback)
{
  return std::isfinite(value) ? value : fallback;
}

bool pose_is_finite(const geometry_msgs::msg::PoseStamped & pose)
{
  return std::isfinite(pose.pose.position.x) &&
         std::isfinite(pose.pose.position.y) &&
         std::isfinite(pose.pose.orientation.x) &&
         std::isfinite(pose.pose.orientation.y) &&
         std::isfinite(pose.pose.orientation.z) &&
         std::isfinite(pose.pose.orientation.w);
}

bool vector_is_finite(const std::vector<double> & values)
{
  return std::all_of(values.begin(), values.end(), [](double value) { return std::isfinite(value); });
}
}  // namespace

void CANMPCController::configure(
  const rclcpp_lifecycle::LifecycleNode::WeakPtr & parent,
  std::string name,
  std::shared_ptr<tf2_ros::Buffer> tf,
  std::shared_ptr<nav2_costmap_2d::Costmap2DROS> costmap_ros)
{
  parent_node_ = parent;
  name_ = name;
  tf_ = tf;
  costmap_ros_ = costmap_ros;
  
  auto node = parent.lock();
  if (!node) {
    throw std::runtime_error("Unable to lock parent node during configure!");
  }

  logger_ = node->get_logger();
  RCLCPP_INFO(logger_, "Configuring CANMPCController plugin: %s", name_.c_str());

  // Declare/Get standard parameters
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".horizon_steps", rclcpp::ParameterValue(20));
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".model_dt", rclcpp::ParameterValue(0.05));
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".max_solver_time_ms", rclcpp::ParameterValue(50.0));
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".max_path_length", rclcpp::ParameterValue(3.0));
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".default_v_ref", rclcpp::ParameterValue(0.5));

  node->get_parameter(name_ + ".horizon_steps", horizon_steps_);
  node->get_parameter(name_ + ".model_dt", model_dt_);
  node->get_parameter(name_ + ".max_solver_time_ms", max_solver_time_ms_);
  node->get_parameter(name_ + ".max_path_length", max_path_length_);
  node->get_parameter(name_ + ".default_v_ref", default_v_ref_);

  // Declare/Get NMPC hyperparameters
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".beta", rclcpp::ParameterValue(3.0));
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".d0", rclcpp::ParameterValue(1.5));
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".d_safe_0", rclcpp::ParameterValue(0.5));
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".d_safe_max", rclcpp::ParameterValue(1.2));
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".v_max_min", rclcpp::ParameterValue(0.1));
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".v_y_max_min", rclcpp::ParameterValue(0.1));
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".omega_max_min", rclcpp::ParameterValue(0.2));

  node->get_parameter(name_ + ".beta", beta_);
  node->get_parameter(name_ + ".d0", d0_);
  node->get_parameter(name_ + ".d_safe_0", d_safe_0_);
  node->get_parameter(name_ + ".d_safe_max", d_safe_max_);
  node->get_parameter(name_ + ".v_max_min", v_max_min_);
  node->get_parameter(name_ + ".v_y_max_min", v_y_max_min_);
  node->get_parameter(name_ + ".omega_max_min", omega_max_min_);

  // Declare/Get NMPC cost weights
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".q_x", rclcpp::ParameterValue(10.0));
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".q_y", rclcpp::ParameterValue(10.0));
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".q_theta", rclcpp::ParameterValue(5.0));
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".r_vx", rclcpp::ParameterValue(0.1));
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".r_vy", rclcpp::ParameterValue(0.1));
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".r_omega", rclcpp::ParameterValue(0.05));
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".rd_vx", rclcpp::ParameterValue(1.0));
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".rd_vy", rclcpp::ParameterValue(1.0));
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".rd_omega", rclcpp::ParameterValue(0.5));
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".q_active_factor", rclcpp::ParameterValue(0.1));
  nav2_util::declare_parameter_if_not_declared(node, name_ + ".w_slack", rclcpp::ParameterValue(100000.0));

  node->get_parameter(name_ + ".q_x", q_x_);
  node->get_parameter(name_ + ".q_y", q_y_);
  node->get_parameter(name_ + ".q_theta", q_theta_);
  node->get_parameter(name_ + ".r_vx", r_vx_);
  node->get_parameter(name_ + ".r_vy", r_vy_);
  node->get_parameter(name_ + ".r_omega", r_omega_);
  node->get_parameter(name_ + ".rd_vx", rd_vx_);
  node->get_parameter(name_ + ".rd_vy", rd_vy_);
  node->get_parameter(name_ + ".rd_omega", rd_omega_);
  node->get_parameter(name_ + ".q_active_factor", q_active_factor_);
  node->get_parameter(name_ + ".w_slack", w_slack_);

  // Create subscriptions
  human_states_sub_ = node->create_subscription<canmpc_msgs::msg::HumanStates>(
    "/canmpc/humans", 10,
    std::bind(&CANMPCController::humanStatesCallback, this, std::placeholders::_1));

  context_sub_ = node->create_subscription<canmpc_msgs::msg::Context>(
    "/canmpc/context", 10,
    std::bind(&CANMPCController::contextCallback, this, std::placeholders::_1));

  // Create publishers
  local_reference_path_pub_ = node->create_publisher<nav_msgs::msg::Path>("~/local_reference_path", 1);
  predicted_trajectory_pub_ = node->create_publisher<nav_msgs::msg::Path>("~/predicted_trajectory", 1);
  solver_stats_pub_ = node->create_publisher<canmpc_msgs::msg::SolverStats>("~/solver_stats", 1);
  adaptive_bounds_pub_ = node->create_publisher<canmpc_msgs::msg::AdaptiveBounds>("~/adaptive_bounds", 1);

  // Initialize solver wrapper (using N=20 and N_humans=3)
  solver_wrapper_ = std::make_unique<SolverWrapper>(horizon_steps_, 3);

  // Resolve solver shared library path and load it
  std::string package_prefix = ament_index_cpp::get_package_prefix("rai_canmpc_controller");
  std::vector<std::string> search_paths = {
    package_prefix + "/lib/libcanmpc_solver.so",
    package_prefix + "/bin/canmpc_solver.dll",
    package_prefix + "/lib/canmpc_solver.dll",
    package_prefix + "/bin/libcanmpc_solver.dll",
    package_prefix + "/lib/libcanmpc_solver.dll"
  };

  bool loaded = false;
  std::string tried_paths = "";
  for (const auto & path : search_paths) {
    RCLCPP_INFO(logger_, "Attempting to load solver from: %s", path.c_str());
    if (solver_wrapper_->init(path)) {
      RCLCPP_INFO(logger_, "Successfully loaded CasADi solver from: %s", path.c_str());
      loaded = true;
      break;
    }
    tried_paths += "\n - " + path;
  }

  if (!loaded) {
    RCLCPP_ERROR(logger_, "Failed to initialize CasADi solver wrapper. Tried paths: %s", tried_paths.c_str());
    throw std::runtime_error("CasADi solver wrapper initialization failed!");
  }

  // Create collision checker
  collision_checker_ = std::make_unique<nav2_costmap_2d::FootprintCollisionChecker<nav2_costmap_2d::Costmap2D *>>(
    costmap_ros_->getCostmap());

  RCLCPP_INFO(logger_, "CANMPCController initialized successfully.");
}

void CANMPCController::cleanup()
{
  RCLCPP_INFO(logger_, "Cleaning up CANMPCController plugin: %s", name_.c_str());
  
  human_states_sub_.reset();
  context_sub_.reset();
  
  local_reference_path_pub_.reset();
  predicted_trajectory_pub_.reset();
  solver_stats_pub_.reset();
  adaptive_bounds_pub_.reset();

  solver_wrapper_.reset();
  collision_checker_.reset();
}

void CANMPCController::activate()
{
  RCLCPP_INFO(logger_, "Activating CANMPCController plugin: %s", name_.c_str());
  
  local_reference_path_pub_->on_activate();
  predicted_trajectory_pub_->on_activate();
  solver_stats_pub_->on_activate();
  adaptive_bounds_pub_->on_activate();

  if (solver_wrapper_) {
    solver_wrapper_->resetCache();
  }

  last_cmd_vel_.linear.x = 0.0;
  last_cmd_vel_.linear.y = 0.0;
  last_cmd_vel_.linear.z = 0.0;
  last_cmd_vel_.angular.x = 0.0;
  last_cmd_vel_.angular.y = 0.0;
  last_cmd_vel_.angular.z = 0.0;
}

void CANMPCController::deactivate()
{
  RCLCPP_INFO(logger_, "Deactivating CANMPCController plugin: %s", name_.c_str());
  
  local_reference_path_pub_->on_deactivate();
  predicted_trajectory_pub_->on_deactivate();
  solver_stats_pub_->on_deactivate();
  adaptive_bounds_pub_->on_deactivate();
}

void CANMPCController::setPlan(const nav_msgs::msg::Path & path)
{
  auto normalized_path = path;

  if (normalized_path.header.frame_id.empty()) {
    normalized_path.header.frame_id = costmap_ros_->getGlobalFrameID();
    RCLCPP_WARN(
      logger_,
      "Received global plan with empty header frame_id. Falling back to %s.",
      normalized_path.header.frame_id.c_str());
  }

  for (auto & pose : normalized_path.poses) {
    if (pose.header.frame_id.empty()) {
      pose.header.frame_id = normalized_path.header.frame_id;
    }
    if (pose.header.stamp.sec == 0 && pose.header.stamp.nanosec == 0) {
      pose.header.stamp = normalized_path.header.stamp;
    }
  }

  current_global_plan_ = normalized_path;
  plan_handler_.setGlobalPlan(normalized_path);
}

void CANMPCController::getSpeedLimits(
  double context_vx, double context_vy, double context_omega,
  double & max_vx, double & max_vy, double & max_omega)
{
  // 1. Nominal maximum limits
  max_vx = (context_vx > 0.0) ? context_vx : default_v_ref_;
  max_vy = (context_vy > 0.0) ? context_vy : default_v_ref_;
  max_omega = (context_omega > 0.0) ? context_omega : 1.0;

  // 2. Apply external speed limit (set via setSpeedLimit)
  if (external_speed_limit_vx_ > 0.0) {
    if (speed_limit_percentage_) {
      max_vx *= external_speed_limit_vx_;
      max_vy *= external_speed_limit_vy_;
      max_omega *= external_speed_limit_omega_;
    } else {
      max_vx = std::min(max_vx, external_speed_limit_vx_);
      max_vy = std::min(max_vy, external_speed_limit_vy_);
      max_omega = std::min(max_omega, external_speed_limit_omega_);
    }
  }
}

geometry_msgs::msg::TwistStamped CANMPCController::computeVelocityCommands(
  const geometry_msgs::msg::PoseStamped & pose,
  const geometry_msgs::msg::Twist & velocity,
  nav2_core::GoalChecker * goal_checker)
{
  geometry_msgs::msg::TwistStamped cmd_vel;
  cmd_vel.header.frame_id = pose.header.frame_id;
  cmd_vel.header.stamp = pose.header.stamp;
  try {
    (void)velocity;
    (void)goal_checker;

    auto start_time = std::chrono::high_resolution_clock::now();

    if (!pose_is_finite(pose)) {
      RCLCPP_ERROR(logger_, "Robot pose contains NaN/Inf. Returning zero velocity.");
      return cmd_vel;
    }

    // 1. Thread-safe copy of latest dynamic callback data
    canmpc_msgs::msg::Context context_copy;
    canmpc_msgs::msg::HumanStates humans_copy;
    {
      std::lock_guard<std::mutex> lock(data_mutex_);
      context_copy = latest_context_;
      humans_copy = latest_human_states_;
    }

    // 2. Transform and crop global plan
    auto costmap = costmap_ros_->getCostmap();
    std::string global_frame = costmap_ros_->getGlobalFrameID();

    nav_msgs::msg::Path local_cropped_path = plan_handler_.transformAndCropPlan(
      pose, tf_, global_frame, *costmap, max_path_length_);

    if (local_cropped_path.poses.empty()) {
      RCLCPP_WARN(logger_, "Local cropped path is empty!");
      return cmd_vel; // Zero velocity command
    }

    // Determine current speed limits
    double max_vx, max_vy, max_omega;
    getSpeedLimits(context_copy.vx_max, context_copy.vy_max, context_copy.omega_max, max_vx, max_vy, max_omega);
    max_vx = finite_or(max_vx, default_v_ref_);
    max_vy = finite_or(max_vy, default_v_ref_);
    max_omega = finite_or(max_omega, 1.0);

    // 3. Resample path to get exact reference trajectory for the prediction horizon
    double v_ref = (context_copy.vx_max > 0.0 && std::isfinite(context_copy.vx_max)) ? context_copy.vx_max : default_v_ref_;
    std::vector<geometry_msgs::msg::PoseStamped> ref_traj = plan_handler_.resamplePath(
      local_cropped_path, pose, horizon_steps_, model_dt_, v_ref);

    if (ref_traj.size() < static_cast<size_t>(horizon_steps_ + 1)) {
      RCLCPP_ERROR(logger_, "Reference trajectory is shorter than horizon. Returning zero velocity.");
      return cmd_vel;
    }
    if (!std::all_of(ref_traj.begin(), ref_traj.end(), pose_is_finite)) {
      RCLCPP_ERROR(logger_, "Reference trajectory contains NaN/Inf. Returning zero velocity.");
      return cmd_vel;
    }

    // Publish reference path for visualization
    nav_msgs::msg::Path ref_path_msg;
    ref_path_msg.header.frame_id = global_frame;
    ref_path_msg.header.stamp = pose.header.stamp;
    ref_path_msg.poses = ref_traj;
    local_reference_path_pub_->publish(ref_path_msg);

    // Extract current robot state
    double rx = pose.pose.position.x;
    double ry = pose.pose.position.y;
    double r_yaw = finite_or(tf2::getYaw(pose.pose.orientation), 0.0);

    // 4. Populate solver input parameters
    SolveInput solver_input;
    solver_input.x_init = {rx, ry, r_yaw};

    // Populate reference states and unwrap heading angles to avoid wrapping discontinuities
    solver_input.x_ref.resize(3 * (horizon_steps_ + 1));
    double prev_yaw = r_yaw;
    for (int i = 0; i <= horizon_steps_; ++i) {
      double target_yaw = finite_or(tf2::getYaw(ref_traj[i].pose.orientation), prev_yaw);
      double diff = MecanumModel::normalizeAngle(target_yaw - prev_yaw);
      target_yaw = prev_yaw + diff;
      solver_input.x_ref[3 * i] = finite_or(ref_traj[i].pose.position.x, rx);
      solver_input.x_ref[3 * i + 1] = finite_or(ref_traj[i].pose.position.y, ry);
      solver_input.x_ref[3 * i + 2] = target_yaw;
      prev_yaw = target_yaw;
    }

    // Populate reference controls (defaulting to tracking the reference forward speed)
    solver_input.u_ref.assign(3 * horizon_steps_, 0.0);
    for (int i = 0; i < horizon_steps_; ++i) {
      solver_input.u_ref[3 * i] = v_ref;
    }

    // Find 3 closest humans and populate human data parameter vector
    solver_input.human_data.assign(4 * 3, 0.0); // Size 12 for 3 humans

    struct HumanDist {
      canmpc_msgs::msg::HumanState state;
      double dist;
    };
    std::vector<HumanDist> sorted_humans;
    for (const auto & human : humans_copy.humans) {
      const double hx = human.pose.position.x;
      const double hy = human.pose.position.y;
      const double hvx = human.velocity.linear.x;
      const double hvy = human.velocity.linear.y;
      if (!std::isfinite(hx) || !std::isfinite(hy) || !std::isfinite(hvx) || !std::isfinite(hvy)) {
        continue;
      }
      double dx = hx - rx;
      double dy = hy - ry;
      double dist = std::sqrt(dx * dx + dy * dy);
      sorted_humans.push_back({human, dist});
    }

    std::sort(sorted_humans.begin(), sorted_humans.end(), [](const HumanDist & a, const HumanDist & b) {
      return a.dist < b.dist;
    });

    for (size_t j = 0; j < 3; ++j) {
      if (j < sorted_humans.size()) {
        solver_input.human_data[4 * j] = sorted_humans[j].state.pose.position.x;
        solver_input.human_data[4 * j + 1] = sorted_humans[j].state.pose.position.y;
        solver_input.human_data[4 * j + 2] = sorted_humans[j].state.velocity.linear.x;
        solver_input.human_data[4 * j + 3] = sorted_humans[j].state.velocity.linear.y;
      } else {
        solver_input.human_data[4 * j] = 999.0;
        solver_input.human_data[4 * j + 1] = 999.0;
        solver_input.human_data[4 * j + 2] = 0.0;
        solver_input.human_data[4 * j + 3] = 0.0;
      }
    }

    // Populate scalar solver hyperparameters
    solver_input.params = {
      model_dt_,
      beta_,
      d0_,
      d_safe_0_,
      d_safe_max_,
      v_ref,
      max_vx,
      v_max_min_,
      max_vy,
      v_y_max_min_,
      max_omega,
      omega_max_min_,
      q_x_,
      q_y_,
      q_theta_,
      r_vx_,
      r_vy_,
      r_omega_,
      rd_vx_,
      rd_vy_,
      rd_omega_,
      q_active_factor_,
      w_slack_
    };

    if (!vector_is_finite(solver_input.x_init) ||
        !vector_is_finite(solver_input.x_ref) ||
        !vector_is_finite(solver_input.u_ref) ||
        !vector_is_finite(solver_input.human_data) ||
        !vector_is_finite(solver_input.params)) {
      RCLCPP_ERROR(logger_, "Solver input contains NaN/Inf. Returning zero velocity.");
      return cmd_vel;
    }

    // 5. Invoke MPC Solver
    SolveOutput solver_output = solver_wrapper_->solve(solver_input);

    auto end_solve_time = std::chrono::high_resolution_clock::now();
    double solve_time_ms = std::chrono::duration<double, std::milli>(end_solve_time - start_time).count();
    bool timeout_detected = (solve_time_ms > max_solver_time_ms_);

    // 6. Evaluate solution validity and collision safety
    bool collision_detected = false;
    bool nan_detected = false;
    bool size_invalid = solver_output.u_opt.size() < 3 || solver_output.x_opt.size() < static_cast<size_t>(3 * (horizon_steps_ + 1));

    if (solver_output.success && !size_invalid) {
      for (double val : solver_output.u_opt) {
        if (std::isnan(val) || std::isinf(val)) {
          nan_detected = true;
          break;
        }
      }
      for (double val : solver_output.x_opt) {
        if (std::isnan(val) || std::isinf(val)) {
          nan_detected = true;
          break;
        }
      }

      auto footprint = costmap_ros_->getRobotFootprint();
      if (collision_checker_ && !nan_detected) {
        for (int i = 0; i <= horizon_steps_; ++i) {
          double px = solver_output.x_opt[3 * i];
          double py = solver_output.x_opt[3 * i + 1];
          double pyaw = solver_output.x_opt[3 * i + 2];
          double cost = collision_checker_->footprintCostAtPose(px, py, pyaw, footprint);
          if (cost >= nav2_costmap_2d::LETHAL_OBSTACLE) {
            collision_detected = true;
            RCLCPP_WARN(logger_, "Predicted state %d is in LETHAL collision! Cost: %.1f", i, cost);
            break;
          }
        }
      }
    }

    double cmd_vx = 0.0;
    double cmd_vy = 0.0;
    double cmd_omega = 0.0;

    bool fallback_triggered = !solver_output.success || size_invalid || nan_detected || collision_detected || timeout_detected;

    if (fallback_triggered) {
      RCLCPP_WARN(logger_, "MPC Solver failure or safety violation detected! Status: %s, SizeInvalid: %d, NaN: %d, Collision: %d, Timeout: %d. Activating safety fallback.",
                   solver_output.status.c_str(), size_invalid, nan_detected, collision_detected, timeout_detected);

      solver_wrapper_->resetCache();

      double decay_factor = 0.8;
      double candidate_vx = last_cmd_vel_.linear.x * decay_factor;
      double candidate_vy = last_cmd_vel_.linear.y * decay_factor;
      double candidate_omega = last_cmd_vel_.angular.z * decay_factor;

      bool candidate_safe = true;
      if (collision_checker_) {
        double dx = (candidate_vx * std::cos(r_yaw) - candidate_vy * std::sin(r_yaw)) * model_dt_;
        double dy = (candidate_vx * std::sin(r_yaw) + candidate_vy * std::cos(r_yaw)) * model_dt_;
        double dyaw = candidate_omega * model_dt_;

        double next_x = rx + dx;
        double next_y = ry + dy;
        double next_yaw = MecanumModel::normalizeAngle(r_yaw + dyaw);

        auto footprint = costmap_ros_->getRobotFootprint();
        double cost = collision_checker_->footprintCostAtPose(next_x, next_y, next_yaw, footprint);
        if (cost >= nav2_costmap_2d::LETHAL_OBSTACLE) {
          candidate_safe = false;
          RCLCPP_WARN(logger_, "Decayed previous command candidate is unsafe! Footprint cost: %.1f", cost);
        }
      }

      if (candidate_safe && (std::abs(candidate_vx) > 0.01 || std::abs(candidate_vy) > 0.01 || std::abs(candidate_omega) > 0.01)) {
        RCLCPP_INFO(logger_, "Executing safe decayed previous command: vx=%.3f, vy=%.3f, omega=%.3f", candidate_vx, candidate_vy, candidate_omega);
        cmd_vx = candidate_vx;
        cmd_vy = candidate_vy;
        cmd_omega = candidate_omega;
      } else {
        RCLCPP_WARN(logger_, "Executing zero-velocity stop fallback.");
        cmd_vx = 0.0;
        cmd_vy = 0.0;
        cmd_omega = 0.0;
      }
    } else {
      cmd_vx = solver_output.u_opt[0];
      cmd_vy = solver_output.u_opt[1];
      cmd_omega = solver_output.u_opt[2];
    }

    cmd_vx = std::clamp(cmd_vx, -max_vx, max_vx);
    cmd_vy = std::clamp(cmd_vy, -max_vy, max_vy);
    cmd_omega = std::clamp(cmd_omega, -max_omega, max_omega);

    cmd_vel.twist.linear.x = cmd_vx;
    cmd_vel.twist.linear.y = cmd_vy;
    cmd_vel.twist.angular.z = cmd_omega;

    last_cmd_vel_.linear.x = cmd_vx;
    last_cmd_vel_.linear.y = cmd_vy;
    last_cmd_vel_.angular.z = cmd_omega;

    if (solver_output.success && !fallback_triggered) {
      nav_msgs::msg::Path pred_path_msg;
      pred_path_msg.header.frame_id = global_frame;
      pred_path_msg.header.stamp = pose.header.stamp;
      for (int i = 0; i <= horizon_steps_; ++i) {
        geometry_msgs::msg::PoseStamped pred_pose;
        pred_pose.header.frame_id = global_frame;
        pred_pose.header.stamp = pose.header.stamp;
        pred_pose.pose.position.x = solver_output.x_opt[3 * i];
        pred_pose.pose.position.y = solver_output.x_opt[3 * i + 1];
        pred_pose.pose.position.z = 0.0;
        pred_pose.pose.orientation.x = 0.0;
        pred_pose.pose.orientation.y = 0.0;
        pred_pose.pose.orientation.z = std::sin(solver_output.x_opt[3 * i + 2] / 2.0);
        pred_pose.pose.orientation.w = std::cos(solver_output.x_opt[3 * i + 2] / 2.0);
        pred_path_msg.poses.push_back(pred_pose);
      }
      predicted_trajectory_pub_->publish(pred_path_msg);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double solve_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    canmpc_msgs::msg::SolverStats stats_msg;
    stats_msg.header.stamp = pose.header.stamp;
    stats_msg.solve_time_ms = solve_time;
    stats_msg.iter_count = solver_output.iter_count;
    stats_msg.status = solver_output.status;
    stats_msg.timeout_flag = (solve_time > max_solver_time_ms_);
    stats_msg.collision_flag = collision_detected;
    solver_stats_pub_->publish(stats_msg);

    canmpc_msgs::msg::AdaptiveBounds bounds_msg;
    bounds_msg.header.stamp = pose.header.stamp;
    bounds_msg.vx_max = max_vx;
    bounds_msg.vy_max = max_vy;
    bounds_msg.omega_max = max_omega;
    bounds_msg.d_safe = (context_copy.d_safe > 0.0 && std::isfinite(context_copy.d_safe)) ? context_copy.d_safe : d_safe_0_;
    adaptive_bounds_pub_->publish(bounds_msg);

    return cmd_vel;
  } catch (const std::exception & ex) {
    RCLCPP_ERROR(logger_, "CANMPCController exception: %s", ex.what());
    return cmd_vel;
  } catch (...) {
    RCLCPP_ERROR(logger_, "CANMPCController unknown exception.");
    return cmd_vel;
  }
}

void CANMPCController::setSpeedLimit(const double & speed_limit, const bool & percentage)
{
  speed_limit_percentage_ = percentage;
  
  if (percentage) {
    external_speed_limit_vx_ = speed_limit / 100.0;
    external_speed_limit_vy_ = speed_limit / 100.0;
    external_speed_limit_omega_ = speed_limit / 100.0;
  } else {
    external_speed_limit_vx_ = speed_limit;
    external_speed_limit_vy_ = speed_limit;
    external_speed_limit_omega_ = speed_limit;
  }
  
  RCLCPP_INFO(logger_, "Speed limit updated: %.2f (percentage: %d)", speed_limit, percentage);
}

void CANMPCController::humanStatesCallback(const canmpc_msgs::msg::HumanStates::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(data_mutex_);
  latest_human_states_ = *msg;
}

void CANMPCController::contextCallback(const canmpc_msgs::msg::Context::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(data_mutex_);
  latest_context_ = *msg;
}

}  // namespace rai_canmpc_controller

// Register as a Controller plugin
PLUGINLIB_EXPORT_CLASS(rai_canmpc_controller::CANMPCController, nav2_core::Controller)
