#include <chrono>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "geometry_msgs/msg/point_stamped.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "nav_msgs/msg/path.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/imu.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"
#include "std_msgs/msg/empty.hpp"
#include "std_msgs/msg/header.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/empty.hpp"
#include "std_srvs/srv/trigger.hpp"
#include "tf2/LinearMath/Matrix3x3.h"
#include "tf2/LinearMath/Quaternion.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"

#include "rai_controller/controller_algorithm.hpp"
#include "rai_controller_cca_nmpc/ccanmpc_core.hpp"
#include "rai_controller/msg/adaptive_bounds.hpp"
#include "rai_controller/msg/context.hpp"
#include "rai_controller/msg/human_states.hpp"
#include "rai_controller/msg/solver_stats.hpp"
#include "rai_planner_a_star/a_star_planner.hpp"
#include "rai_planner_dijkstra/dijkstra_planner.hpp"
#include "rai_planner/planner.hpp"
#include "rai_planner_straight_line/straight_line_planner.hpp"
#include "rai_controller/srv/get_costmap.hpp"
#include "rai_controller/srv/get_navigation_status.hpp"
#include "rai_controller/srv/get_path.hpp"
#include "rai_controller/srv/list_algorithms.hpp"
#include "rai_controller/srv/navigate_to_pose.hpp"
#include "rai_controller/srv/set_controller.hpp"
#include "rai_controller/srv/set_planner.hpp"

using namespace std::chrono_literals;

namespace rai_controller
{

using HumanState = ControllerHumanState;
using CcanmpcParameters = ControllerConfig;
using ContextState = ControllerContext;
using SolveResult = ControllerResult;

namespace
{

double yawFromPose(const geometry_msgs::msg::PoseStamped & pose)
{
  tf2::Quaternion q;
  tf2::fromMsg(pose.pose.orientation, q);
  double roll = 0.0;
  double pitch = 0.0;
  double yaw = 0.0;
  tf2::Matrix3x3(q).getRPY(roll, pitch, yaw);
  return yaw;
}

class CcaNmpcAlgorithm : public ControllerAlgorithm
{
public:
  explicit CcaNmpcAlgorithm(const ControllerConfig & config)
  : core_(toParameters(config))
  {
  }

  std::string id() const override {return "CCA_NMPC";}

  ControllerResult solve(
    const geometry_msgs::msg::PoseStamped & robot_pose,
    const geometry_msgs::msg::Twist & robot_velocity,
    const nav_msgs::msg::Path & reference_path,
    const std::vector<ControllerHumanState> & humans,
    const ControllerContext & context,
    const nav_msgs::msg::OccupancyGrid * costmap) override
  {
    return toResult(core_.solve(
      robot_pose,
      robot_velocity,
      reference_path,
      toHumans(humans),
      toContext(context),
      costmap));
  }

  void reset() override
  {
    core_.reset();
  }

private:
  static rai_controller_cca_nmpc::CcanmpcParameters toParameters(const ControllerConfig & config)
  {
    rai_controller_cca_nmpc::CcanmpcParameters out;
    out.horizon_steps = config.horizon_steps;
    out.sample_time = config.sample_time;
    out.max_reference_length = config.max_reference_length;
    out.default_v_ref = config.default_v_ref;
    out.vx_samples = config.vx_samples;
    out.vy_samples = config.vy_samples;
    out.omega_samples = config.omega_samples;
    out.adaptive_sampling = config.adaptive_sampling;
    out.refinement_samples = config.refinement_samples;
    out.max_control_samples = config.max_control_samples;
    out.refinement_radius_fraction = config.refinement_radius_fraction;
    out.timeout_margin_ms = config.timeout_margin_ms;
    out.min_speed_xy = config.min_speed_xy;
    out.acc_lim_x = config.acc_lim_x;
    out.acc_lim_y = config.acc_lim_y;
    out.acc_lim_theta = config.acc_lim_theta;
    out.beta = config.beta;
    out.d0 = config.d0;
    out.context_distance_weight = config.context_distance_weight;
    out.context_velocity_weight = config.context_velocity_weight;
    out.context_direction_weight = config.context_direction_weight;
    out.context_confidence_weight = config.context_confidence_weight;
    out.context_bias = config.context_bias;
    out.human_velocity_max = config.human_velocity_max;
    out.d_safe_0 = config.d_safe_0;
    out.d_safe_max = config.d_safe_max;
    out.k_d = config.k_d;
    out.vx_max_0 = config.vx_max_0;
    out.vy_max_0 = config.vy_max_0;
    out.omega_max_0 = config.omega_max_0;
    out.vx_max_min = config.vx_max_min;
    out.vy_max_min = config.vy_max_min;
    out.omega_max_min = config.omega_max_min;
    out.k_vx = config.k_vx;
    out.k_vy = config.k_vy;
    out.k_omega = config.k_omega;
    out.q_x = config.q_x;
    out.q_y = config.q_y;
    out.q_theta = config.q_theta;
    out.q_human_x = config.q_human_x;
    out.q_human_y = config.q_human_y;
    out.q_human_theta = config.q_human_theta;
    out.r_vx = config.r_vx;
    out.r_vy = config.r_vy;
    out.r_omega = config.r_omega;
    out.rd_vx = config.rd_vx;
    out.rd_vy = config.rd_vy;
    out.rd_omega = config.rd_omega;
    out.human_cost_weight = config.human_cost_weight;
    out.costmap_cost_weight = config.costmap_cost_weight;
    out.terminal_cost_weight = config.terminal_cost_weight;
    out.w_slack = config.w_slack;
    out.max_compute_time_ms = config.max_compute_time_ms;
    out.max_human_age_sec = config.max_human_age_sec;
    out.costmap_collision_threshold = config.costmap_collision_threshold;
    return out;
  }

  static rai_controller_cca_nmpc::ContextState toContext(const ControllerContext & context)
  {
    rai_controller_cca_nmpc::ContextState out;
    out.phi_h = context.phi_h;
    out.d_safe = context.d_safe;
    out.vx_max = context.vx_max;
    out.vy_max = context.vy_max;
    out.omega_max = context.omega_max;
    return out;
  }

  static std::vector<rai_controller_cca_nmpc::HumanState> toHumans(
    const std::vector<ControllerHumanState> & humans)
  {
    std::vector<rai_controller_cca_nmpc::HumanState> out;
    out.reserve(humans.size());
    for (const auto & human : humans) {
      rai_controller_cca_nmpc::HumanState converted;
      converted.id = human.id;
      converted.x = human.x;
      converted.y = human.y;
      converted.vx = human.vx;
      converted.vy = human.vy;
      converted.confidence = human.confidence;
      converted.age_sec = human.age_sec;
      converted.covariance_x = human.covariance_x;
      converted.covariance_y = human.covariance_y;
      converted.covariance_vx = human.covariance_vx;
      converted.covariance_vy = human.covariance_vy;
      out.push_back(converted);
    }
    return out;
  }

  static ControllerResult toResult(const rai_controller_cca_nmpc::SolveResult & result)
  {
    ControllerResult out;
    out.command = result.command;
    out.local_reference_path = result.local_reference_path;
    out.predicted_trajectory = result.predicted_trajectory;
    out.solve_time_ms = result.solve_time_ms;
    out.sample_count = result.sample_count;
    out.status = result.status;
    out.timeout = result.timeout;
    out.collision = result.collision;
    out.phi_h = result.phi_h;
    out.d_safe = result.d_safe;
    out.vx_max = result.vx_max;
    out.vy_max = result.vy_max;
    out.omega_max = result.omega_max;
    out.q_scale = result.q_scale;
    return out;
  }

  rai_controller_cca_nmpc::CcanmpcCore core_;
};

std::unique_ptr<ControllerAlgorithm> createCcaNmpcAlgorithm(const ControllerConfig & config)
{
  return std::make_unique<CcaNmpcAlgorithm>(config);
}

}  

class RaiControllerServerNode : public rclcpp::Node
{
public:
  RaiControllerServerNode()
  : Node("rai_controller"),
    tf_buffer_(this->get_clock()),
    tf_listener_(tf_buffer_)
  {
    registerAlgorithms();
    global_frame_ = declare_parameter<std::string>("global_frame", "map");
    control_frame_ = declare_parameter<std::string>("control_frame", "odom_combined");
    base_frame_ = declare_parameter<std::string>("base_frame", "base_footprint");
    goal_topic_ = declare_parameter<std::string>("goal_topic", "/goal_pose");
    cancel_topic_ = declare_parameter<std::string>("cancel_topic", "/rai_navigation/cancel_topic");
    clear_costmap_topic_ = declare_parameter<std::string>(
      "clear_costmap_topic", "/rai_navigation/clear_costmap_topic");
    replan_topic_ = declare_parameter<std::string>("replan_topic", "/rai_navigation/replan_topic");
    status_topic_ = declare_parameter<std::string>("status_topic", "/rai_navigation/status");
    navigate_to_pose_service_ = declare_parameter<std::string>(
      "navigate_to_pose_service", "/rai_navigation/navigate_to_pose");
    cancel_service_ = declare_parameter<std::string>("cancel_service", "/rai_navigation/cancel");
    clear_costmap_service_ = declare_parameter<std::string>(
      "clear_costmap_service", "/rai_navigation/clear_costmap");
    clear_local_costmap_service_ = declare_parameter<std::string>(
      "clear_local_costmap_service", "/rai_navigation/clear_local_costmap");
    clear_global_costmap_service_ = declare_parameter<std::string>(
      "clear_global_costmap_service", "/rai_navigation/clear_global_costmap");
    replan_service_ = declare_parameter<std::string>("replan_service", "/rai_navigation/replan");
    pause_service_ = declare_parameter<std::string>("pause_service", "/rai_navigation/pause");
    resume_service_ = declare_parameter<std::string>("resume_service", "/rai_navigation/resume");
    get_status_service_ = declare_parameter<std::string>(
      "get_status_service", "/rai_navigation/get_status");
    get_path_service_ = declare_parameter<std::string>("get_path_service", "/rai_navigation/get_path");
    get_costmap_service_ = declare_parameter<std::string>(
      "get_costmap_service", "/rai_navigation/get_local_costmap");
    list_algorithms_service_ = declare_parameter<std::string>(
      "list_algorithms_service", "/rai_navigation/list_algorithms");
    set_controller_service_ = declare_parameter<std::string>(
      "set_controller_service", "/rai_navigation/set_controller");
    set_planner_service_ = declare_parameter<std::string>(
      "set_planner_service", "/rai_navigation/set_planner");
    active_controller_id_ = declare_parameter<std::string>("controller_id", "CCA_NMPC");
    external_path_topic_ = declare_parameter<std::string>(
      "external_path_topic", "/rai_navigation/external_path");
    global_path_topic_ = declare_parameter<std::string>(
      "global_path_topic", "/rai_navigation/global_path");
    plan_topic_ = declare_parameter<std::string>("plan_topic", "");
    odom_topic_ = declare_parameter<std::string>("odom_topic", "/odom_combined");
    laser_scan_topic_ = declare_parameter<std::string>("laser_scan_topic", "/scan_filtered");
    imu_topic_ = declare_parameter<std::string>("imu_topic", "/imu/data");
    map_topic_ = declare_parameter<std::string>("map_topic", "/map");
    costmap_topic_ = declare_parameter<std::string>(
      "costmap_topic", "/rai_navigation/local_costmap");
    local_grid_topic_ = declare_parameter<std::string>(
      "local_grid_topic", "/rai_navigation/local_costmap");
    cmd_vel_topic_ = declare_parameter<std::string>("cmd_vel_topic", "/rai_navigation/cmd_vel");
    generated_path_spacing_ = declare_parameter<double>("generated_path_spacing", 0.05);
    global_planner_algorithm_ = declare_parameter<std::string>("global_planner_algorithm", "A_STAR");
    global_path_smoothing_passes_ = declare_parameter<int>("global_path_smoothing_passes", 1);
    global_planner_inflation_radius_ = declare_parameter<double>("global_planner_inflation_radius", 0.22);
    local_grid_resolution_ = declare_parameter<double>("local_grid_resolution", 0.05);
    local_grid_width_m_ = declare_parameter<double>("local_grid_width_m", 4.0);
    local_grid_height_m_ = declare_parameter<double>("local_grid_height_m", 4.0);
    
    
    robot_radius_ = declare_parameter<double>("robot_radius", 0.22);
    
    global_planner_inflation_radius_ = std::max(global_planner_inflation_radius_, robot_radius_);
    obstacle_inflation_radius_ = declare_parameter<double>("obstacle_inflation_radius", 0.22);
    obstacle_inflation_radius_ = std::max(obstacle_inflation_radius_, robot_radius_);
    
    obstacle_lethal_radius_ = declare_parameter<double>("obstacle_lethal_radius", robot_radius_);
    obstacle_lethal_radius_ = std::max(obstacle_lethal_radius_, robot_radius_);
    slowdown_inflation_radius_ = declare_parameter<double>("slowdown_inflation_radius", 0.55);
    slowdown_inflation_radius_ = std::max(slowdown_inflation_radius_, obstacle_inflation_radius_);
    slowdown_cost_ = declare_parameter<int>("slowdown_cost", 35);
    inscribed_cost_ = declare_parameter<int>("inscribed_cost", 85);
    map_occupied_threshold_ = declare_parameter<int>("map_occupied_threshold", 65);
    treat_unknown_as_obstacle_ = declare_parameter<bool>("treat_unknown_as_obstacle", false);
    max_laser_age_sec_ = declare_parameter<double>("max_laser_age_sec", 0.5);
    use_imu_angular_velocity_ = declare_parameter<bool>("use_imu_angular_velocity", true);
    max_imu_age_sec_ = declare_parameter<double>("max_imu_age_sec", 0.5);
    control_frequency_ = declare_parameter<double>("control_frequency", 25.0);
    max_solver_time_ms_ = declare_parameter<double>("max_solver_time_ms", 35.0);
    active_controller_ = rai_controller::createController(active_controller_id_, loadCoreParameters());
    stop_on_stale_odom_sec_ = declare_parameter<double>("stop_on_stale_odom_sec", 0.5);
    stop_on_missing_plan_ = declare_parameter<bool>("stop_on_missing_plan", true);
    goal_tolerance_xy_ = declare_parameter<double>("goal_tolerance_xy", 0.25);
    goal_tolerance_yaw_ = declare_parameter<double>("goal_tolerance_yaw", 0.35);
    recovery_enabled_ = declare_parameter<bool>("recovery_enabled", true);
    recovery_max_attempts_ = declare_parameter<int>("recovery_max_attempts", 3);
    recovery_backup_speed_ = declare_parameter<double>("recovery_backup_speed", 0.08);
    recovery_backup_duration_sec_ = declare_parameter<double>("recovery_backup_duration_sec", 0.8);
    recovery_spin_speed_ = declare_parameter<double>("recovery_spin_speed", 0.35);
    recovery_spin_duration_sec_ = declare_parameter<double>("recovery_spin_duration_sec", 1.2);

    cmd_pub_ = create_publisher<geometry_msgs::msg::Twist>(cmd_vel_topic_, 10);
    global_path_pub_ = create_publisher<nav_msgs::msg::Path>(global_path_topic_, 10);
    local_reference_pub_ = create_publisher<nav_msgs::msg::Path>("/canmpc/local_reference_path", 10);
    predicted_pub_ = create_publisher<nav_msgs::msg::Path>("/canmpc/predicted_trajectory", 10);
    local_grid_pub_ = create_publisher<nav_msgs::msg::OccupancyGrid>(local_grid_topic_, 1);
    solver_stats_pub_ = create_publisher<msg::SolverStats>("/canmpc/solver_stats", 10);
    adaptive_bounds_pub_ = create_publisher<msg::AdaptiveBounds>("/canmpc/adaptive_bounds", 10);
    status_pub_ = create_publisher<std_msgs::msg::String>(status_topic_, 10);

    auto reliable_qos = rclcpp::QoS(10).reliable();
    auto sensor_qos = rclcpp::SensorDataQoS();
    auto costmap_qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();

    odom_sub_ = create_subscription<nav_msgs::msg::Odometry>(
      odom_topic_, reliable_qos,
      [this](nav_msgs::msg::Odometry::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        latest_odom_ = *msg;
        have_odom_ = true;
      });

    if (!laser_scan_topic_.empty()) {
      laser_sub_ = create_subscription<sensor_msgs::msg::LaserScan>(
        laser_scan_topic_, sensor_qos,
        [this](sensor_msgs::msg::LaserScan::SharedPtr msg) {
          ingestLaserScan(*msg);
        });
    }

    if (!imu_topic_.empty()) {
      imu_sub_ = create_subscription<sensor_msgs::msg::Imu>(
        imu_topic_, sensor_qos,
        [this](sensor_msgs::msg::Imu::SharedPtr msg) {
          std::lock_guard<std::mutex> lock(data_mutex_);
          latest_imu_ = *msg;
          have_imu_ = true;
        });
    }

    if (!map_topic_.empty()) {
      map_sub_ = create_subscription<nav_msgs::msg::OccupancyGrid>(
        map_topic_, costmap_qos,
        [this](nav_msgs::msg::OccupancyGrid::SharedPtr msg) {
          std::lock_guard<std::mutex> lock(data_mutex_);
          latest_map_ = *msg;
          have_map_ = true;
        });
    }

    humans_sub_ = create_subscription<msg::HumanStates>(
      "/canmpc/humans", reliable_qos,
      [this](msg::HumanStates::SharedPtr msg) {
        std::vector<HumanState> humans;
        humans.reserve(msg->humans.size());
        const std::string human_frame = msg->header.frame_id.empty() ? control_frame_ : msg->header.frame_id;
        for (const auto & human : msg->humans) {
          HumanState state;
          state.id = human.id;
          state.x = human.x;
          state.y = human.y;
          state.vx = human.vx;
          state.vy = human.vy;
          state.confidence = human.confidence;
          state.age_sec = human.age_sec;
          state.covariance_x = human.covariance_x;
          state.covariance_y = human.covariance_y;
          state.covariance_vx = human.covariance_vx;
          state.covariance_vy = human.covariance_vy;

          if (human_frame != control_frame_) {
            geometry_msgs::msg::PointStamped point_in;
            point_in.header = msg->header;
            point_in.header.frame_id = human_frame;
            point_in.point.x = human.x;
            point_in.point.y = human.y;
            geometry_msgs::msg::PointStamped velocity_tip_in = point_in;
            velocity_tip_in.point.x = human.x + human.vx;
            velocity_tip_in.point.y = human.y + human.vy;
            try {
              const auto point_out = tf_buffer_.transform(point_in, control_frame_, 50ms);
              const auto velocity_tip_out = tf_buffer_.transform(velocity_tip_in, control_frame_, 50ms);
              state.x = point_out.point.x;
              state.y = point_out.point.y;
              state.vx = velocity_tip_out.point.x - point_out.point.x;
              state.vy = velocity_tip_out.point.y - point_out.point.y;
            } catch (const tf2::TransformException & ex) {
              RCLCPP_WARN_THROTTLE(
                get_logger(), *get_clock(), 2000,
                "Human transform %s -> %s failed: %s",
                human_frame.c_str(), control_frame_.c_str(), ex.what());
              continue;
            }
          }
          humans.push_back(state);
        }

        std::lock_guard<std::mutex> lock(data_mutex_);
        latest_humans_ = humans;
      });

    context_sub_ = create_subscription<msg::Context>(
      "/canmpc/context", reliable_qos,
      [this](msg::Context::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        latest_context_.phi_h = msg->phi_h;
        latest_context_.d_safe = msg->d_safe;
        latest_context_.vx_max = msg->vx_max;
        latest_context_.vy_max = msg->vy_max;
        latest_context_.omega_max = msg->omega_max;
      });

    if (!costmap_topic_.empty()) {
      costmap_sub_ = create_subscription<nav_msgs::msg::OccupancyGrid>(
        costmap_topic_, costmap_qos,
        [this](nav_msgs::msg::OccupancyGrid::SharedPtr msg) {
          std::lock_guard<std::mutex> lock(data_mutex_);
          latest_costmap_ = *msg;
          have_costmap_ = true;
        });
    }

    external_path_sub_ = create_subscription<nav_msgs::msg::Path>(
      external_path_topic_, reliable_qos,
      [this](nav_msgs::msg::Path::SharedPtr msg) {
        setPath(*msg);
      });
    if (!plan_topic_.empty()) {
      plan_sub_ = create_subscription<nav_msgs::msg::Path>(
        plan_topic_, reliable_qos,
        [this](nav_msgs::msg::Path::SharedPtr msg) {
          setPath(*msg);
        });
    }

    goal_sub_ = create_subscription<geometry_msgs::msg::PoseStamped>(
      goal_topic_, reliable_qos,
      [this](geometry_msgs::msg::PoseStamped::SharedPtr msg) {
        setGoalPath(*msg);
      });

    if (!cancel_topic_.empty()) {
      cancel_sub_ = create_subscription<std_msgs::msg::Empty>(
        cancel_topic_, reliable_qos,
        [this](std_msgs::msg::Empty::SharedPtr) {
          cancelNavigation();
        });
    }
    if (!clear_costmap_topic_.empty()) {
      clear_costmap_sub_ = create_subscription<std_msgs::msg::Empty>(
        clear_costmap_topic_, reliable_qos,
        [this](std_msgs::msg::Empty::SharedPtr) {
          clearLocalObstacles();
        });
    }
    if (!replan_topic_.empty()) {
      replan_sub_ = create_subscription<std_msgs::msg::Empty>(
        replan_topic_, reliable_qos,
        [this](std_msgs::msg::Empty::SharedPtr) {
          requestReplan();
        });
    }

    createNavigationServices();

    const auto period = std::chrono::duration<double>(1.0 / std::max(1.0, control_frequency_));
    control_timer_ = create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(period),
      std::bind(&RaiControllerServerNode::controlLoop, this));

    RCLCPP_INFO(
      get_logger(),
      "rai_controller ready: goal=%s map=%s scan=%s imu=%s output=%s",
      goal_topic_.c_str(), map_topic_.c_str(), laser_scan_topic_.c_str(), imu_topic_.c_str(),
      cmd_vel_topic_.c_str());
    setNavigationState("IDLE", "ready");
  }

private:
  void registerAlgorithms()
  {
    rai_planner_a_star::registerAStarPlanner();
    rai_planner_dijkstra::registerDijkstraPlanner();
    rai_planner_straight_line::registerStraightLinePlanner();
    registerController("CCA_NMPC", createCcaNmpcAlgorithm);
  }

  CcanmpcParameters loadCoreParameters()
  {
    CcanmpcParameters p;
    p.horizon_steps = declare_parameter<int>("horizon_steps", p.horizon_steps);
    p.sample_time = declare_parameter<double>("sample_time", p.sample_time);
    p.max_reference_length = declare_parameter<double>("max_reference_length", p.max_reference_length);
    p.default_v_ref = declare_parameter<double>("default_v_ref", p.default_v_ref);
    p.vx_samples = declare_parameter<int>("vx_samples", p.vx_samples);
    p.vy_samples = declare_parameter<int>("vy_samples", p.vy_samples);
    p.omega_samples = declare_parameter<int>("omega_samples", p.omega_samples);
    p.adaptive_sampling = declare_parameter<bool>("adaptive_sampling", p.adaptive_sampling);
    p.refinement_samples = declare_parameter<int>("refinement_samples", p.refinement_samples);
    p.max_control_samples = declare_parameter<int>("max_control_samples", p.max_control_samples);
    p.refinement_radius_fraction = declare_parameter<double>(
      "refinement_radius_fraction", p.refinement_radius_fraction);
    p.timeout_margin_ms = declare_parameter<double>("timeout_margin_ms", p.timeout_margin_ms);
    p.min_speed_xy = declare_parameter<double>("min_speed_xy", p.min_speed_xy);
    p.acc_lim_x = declare_parameter<double>("acc_lim_x", p.acc_lim_x);
    p.acc_lim_y = declare_parameter<double>("acc_lim_y", p.acc_lim_y);
    p.acc_lim_theta = declare_parameter<double>("acc_lim_theta", p.acc_lim_theta);
    p.beta = declare_parameter<double>("beta", p.beta);
    p.d0 = declare_parameter<double>("d0", p.d0);
    p.context_distance_weight = declare_parameter<double>(
      "context_distance_weight", p.context_distance_weight);
    p.context_velocity_weight = declare_parameter<double>(
      "context_velocity_weight", p.context_velocity_weight);
    p.context_direction_weight = declare_parameter<double>(
      "context_direction_weight", p.context_direction_weight);
    p.context_confidence_weight = declare_parameter<double>(
      "context_confidence_weight", p.context_confidence_weight);
    p.context_bias = declare_parameter<double>("context_bias", p.context_bias);
    p.human_velocity_max = declare_parameter<double>("human_velocity_max", p.human_velocity_max);
    p.d_safe_0 = declare_parameter<double>("d_safe_0", p.d_safe_0);
    p.d_safe_max = declare_parameter<double>("d_safe_max", p.d_safe_max);
    p.k_d = declare_parameter<double>("k_d", p.k_d);
    p.vx_max_0 = declare_parameter<double>("vx_max_0", p.vx_max_0);
    p.vy_max_0 = declare_parameter<double>("vy_max_0", p.vy_max_0);
    p.omega_max_0 = declare_parameter<double>("omega_max_0", p.omega_max_0);
    p.vx_max_min = declare_parameter<double>("vx_max_min", p.vx_max_min);
    p.vy_max_min = declare_parameter<double>("vy_max_min", p.vy_max_min);
    p.omega_max_min = declare_parameter<double>("omega_max_min", p.omega_max_min);
    p.k_vx = declare_parameter<double>("k_vx", p.k_vx);
    p.k_vy = declare_parameter<double>("k_vy", p.k_vy);
    p.k_omega = declare_parameter<double>("k_omega", p.k_omega);
    p.q_x = declare_parameter<double>("q_x", p.q_x);
    p.q_y = declare_parameter<double>("q_y", p.q_y);
    p.q_theta = declare_parameter<double>("q_theta", p.q_theta);
    p.q_human_x = declare_parameter<double>("q_human_x", p.q_human_x);
    p.q_human_y = declare_parameter<double>("q_human_y", p.q_human_y);
    p.q_human_theta = declare_parameter<double>("q_human_theta", p.q_human_theta);
    p.r_vx = declare_parameter<double>("r_vx", p.r_vx);
    p.r_vy = declare_parameter<double>("r_vy", p.r_vy);
    p.r_omega = declare_parameter<double>("r_omega", p.r_omega);
    p.rd_vx = declare_parameter<double>("rd_vx", p.rd_vx);
    p.rd_vy = declare_parameter<double>("rd_vy", p.rd_vy);
    p.rd_omega = declare_parameter<double>("rd_omega", p.rd_omega);
    p.human_cost_weight = declare_parameter<double>("human_cost_weight", p.human_cost_weight);
    p.costmap_cost_weight = declare_parameter<double>("costmap_cost_weight", p.costmap_cost_weight);
    p.terminal_cost_weight = declare_parameter<double>("terminal_cost_weight", p.terminal_cost_weight);
    p.w_slack = declare_parameter<double>("w_slack", p.w_slack);
    p.max_compute_time_ms = declare_parameter<double>("max_compute_time_ms", p.max_compute_time_ms);
    p.max_human_age_sec = declare_parameter<double>("max_human_age_sec", p.max_human_age_sec);
    p.costmap_collision_threshold = declare_parameter<double>(
      "costmap_collision_threshold", p.costmap_collision_threshold);
    return p;
  }

  void setPath(const nav_msgs::msg::Path & path)
  {
    nav_msgs::msg::Path transformed = transformPathToControlFrame(path);
    const bool has_path = !transformed.poses.empty();
    {
      std::lock_guard<std::mutex> lock(data_mutex_);
      latest_path_ = transformed;
      have_path_ = has_path;
      if (has_path) {
        active_goal_ = latest_path_.poses.back();
        have_active_goal_ = true;
      } else {
        have_active_goal_ = false;
      }
      resetActiveController();
      if (!suppress_recovery_reset_) {
        resetRecovery();
      }
    }
    setNavigationState(
      has_path ? "CONTROLLING" : "FAILED",
      has_path ? "external path accepted" : "external path is empty");
  }

  void setGoalPath(const geometry_msgs::msg::PoseStamped & goal)
  {
    paused_ = false;
    setNavigationState("PLANNING", "goal received");

    geometry_msgs::msg::PoseStamped robot_pose;
    if (!lookupRobotPose(robot_pose)) {
      publishStop();
      setNavigationState("FAILED", "robot pose is unavailable");
      return;
    }

    geometry_msgs::msg::PoseStamped goal_in = goal;
    if (goal_in.header.frame_id.empty()) {
      goal_in.header.frame_id = global_frame_;
    }
    goal_in.header.stamp = rclcpp::Time(0);

    geometry_msgs::msg::PoseStamped goal_out;
    try {
      goal_out = tf_buffer_.transform(goal_in, control_frame_, 100ms);
    } catch (const tf2::TransformException & ex) {
      RCLCPP_WARN(get_logger(), "Goal transform failed: %s", ex.what());
      publishStop();
      setNavigationState("FAILED", "goal transform failed");
      return;
    }

    nav_msgs::msg::OccupancyGrid map;
    bool have_map = false;
    {
      std::lock_guard<std::mutex> lock(data_mutex_);
      map = latest_map_;
      have_map = have_map_;
    }

    nav_msgs::msg::Path global_path;
    nav_msgs::msg::Path local_path;
    if (have_map) {
      global_path = planGlobalPath(robot_pose, goal_out, map);
      local_path = transformPathToControlFrame(global_path);
    }
    if (local_path.poses.empty()) {
      local_path = makeStraightGoalPath(robot_pose, goal_out);
      global_path = local_path;
    }

    {
      std::lock_guard<std::mutex> lock(data_mutex_);
      latest_path_ = local_path;
      have_path_ = !latest_path_.poses.empty();
      active_goal_ = goal_out;
      have_active_goal_ = have_path_;
      resetActiveController();
      if (!suppress_recovery_reset_) {
        resetRecovery();
      }
    }

    RCLCPP_INFO(
      get_logger(),
      "Generated %s global reference path with %zu poses.",
      global_planner_algorithm_.c_str(), local_path.poses.size());
    global_path_pub_->publish(global_path);
    setNavigationState(
      local_path.poses.empty() ? "FAILED" : "CONTROLLING",
      local_path.poses.empty() ? "planner returned no path" : "path accepted");
  }

  bool setControllerId(const std::string & controller_id, std::string & message)
  {
    const std::string normalized = rai_controller::normalizeControllerId(controller_id);
    if (!isControllerAvailable(normalized)) {
      message = "unknown controller_id: " + controller_id;
      return false;
    }
    active_controller_id_ = normalized;
    active_controller_ = createController(active_controller_id_, loadCoreParameters());
    message = "controller selected: " + normalized;
    return true;
  }

  bool setPlannerId(const std::string & planner_id, std::string & message)
  {
    const std::string normalized = rai_planner::normalizePlannerId(planner_id);
    if (!isPlannerAvailable(normalized)) {
      message = "unknown planner_id: " + planner_id;
      return false;
    }
    global_planner_algorithm_ = normalized;
    message = "planner selected: " + normalized;
    return true;
  }

  std::vector<std::string> controllerIds() const
  {
    return availableControllerIds();
  }

  bool isControllerAvailable(const std::string & controller_id) const
  {
    for (const auto & id : controllerIds()) {
      if (controller_id == id) {
        return true;
      }
    }
    return false;
  }

  void createNavigationServices()
  {
    if (!navigate_to_pose_service_.empty()) {
      navigate_to_pose_srv_ = create_service<srv::NavigateToPose>(
        navigate_to_pose_service_,
        [this](
          const std::shared_ptr<srv::NavigateToPose::Request> request,
          std::shared_ptr<srv::NavigateToPose::Response> response) {
          std::string message;
          if (!request->controller_id.empty() && !setControllerId(request->controller_id, message)) {
            response->accepted = false;
            response->message = message;
            return;
          }
          if (!request->planner_id.empty() && !setPlannerId(request->planner_id, message)) {
            response->accepted = false;
            response->message = message;
            return;
          }
          setGoalPath(request->goal);
          response->accepted = currentNavigationState() != "FAILED";
          response->message = response->accepted ? "goal accepted" : "goal rejected";
        });
    }
    if (!cancel_service_.empty()) {
      cancel_srv_ = create_service<std_srvs::srv::Trigger>(
        cancel_service_,
        [this](
          const std::shared_ptr<std_srvs::srv::Trigger::Request>,
          std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
          cancelNavigation();
          response->success = true;
          response->message = "navigation canceled";
        });
    }
    if (!clear_costmap_service_.empty()) {
      clear_costmap_srv_ = create_service<std_srvs::srv::Trigger>(
        clear_costmap_service_,
        [this](
          const std::shared_ptr<std_srvs::srv::Trigger::Request>,
          std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
          clearLocalObstacles();
          response->success = true;
          response->message = "local obstacle grids cleared";
        });
    }
    if (!clear_local_costmap_service_.empty()) {
      clear_local_costmap_srv_ = create_service<std_srvs::srv::Trigger>(
        clear_local_costmap_service_,
        [this](
          const std::shared_ptr<std_srvs::srv::Trigger::Request>,
          std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
          clearLocalObstacles();
          response->success = true;
          response->message = "local obstacle grids cleared";
        });
    }
    if (!clear_global_costmap_service_.empty()) {
      clear_global_costmap_srv_ = create_service<std_srvs::srv::Trigger>(
        clear_global_costmap_service_,
        [this](
          const std::shared_ptr<std_srvs::srv::Trigger::Request>,
          std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
          clearGlobalMap();
          response->success = true;
          response->message = "map snapshot cleared";
        });
    }
    if (!replan_service_.empty()) {
      replan_srv_ = create_service<std_srvs::srv::Trigger>(
        replan_service_,
        [this](
          const std::shared_ptr<std_srvs::srv::Trigger::Request>,
          std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
          requestReplan();
          response->success = currentNavigationState() != "FAILED";
          response->message = response->success ? "replan requested" : "replan rejected";
        });
    }
    if (!pause_service_.empty()) {
      pause_srv_ = create_service<std_srvs::srv::Trigger>(
        pause_service_,
        [this](
          const std::shared_ptr<std_srvs::srv::Trigger::Request>,
          std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
          pauseNavigation();
          response->success = true;
          response->message = "navigation paused";
        });
    }
    if (!resume_service_.empty()) {
      resume_srv_ = create_service<std_srvs::srv::Trigger>(
        resume_service_,
        [this](
          const std::shared_ptr<std_srvs::srv::Trigger::Request>,
          std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
          resumeNavigation();
          response->success = currentNavigationState() != "FAILED";
          response->message = response->success ? "navigation resumed" : "resume rejected";
        });
    }
    if (!get_status_service_.empty()) {
      get_status_srv_ = create_service<srv::GetNavigationStatus>(
        get_status_service_,
        [this](
          const std::shared_ptr<srv::GetNavigationStatus::Request>,
          std::shared_ptr<srv::GetNavigationStatus::Response> response) {
          response->state = currentNavigationState();
          response->detail = currentNavigationDetail();
          response->active_controller_id = active_controller_id_;
          response->active_planner_id = global_planner_algorithm_;
          std::lock_guard<std::mutex> lock(data_mutex_);
          response->has_active_goal = have_active_goal_;
        });
    }
    if (!list_algorithms_service_.empty()) {
      list_algorithms_srv_ = create_service<srv::ListAlgorithms>(
        list_algorithms_service_,
        [this](
          const std::shared_ptr<srv::ListAlgorithms::Request>,
          std::shared_ptr<srv::ListAlgorithms::Response> response) {
          response->controller_ids = controllerIds();
          response->planner_ids = rai_planner::availablePlannerIds();
          response->active_controller_id = active_controller_id_;
          response->active_planner_id = global_planner_algorithm_;
        });
    }
    if (!get_path_service_.empty()) {
      get_path_srv_ = create_service<srv::GetPath>(
        get_path_service_,
        [this](
          const std::shared_ptr<srv::GetPath::Request>,
          std::shared_ptr<srv::GetPath::Response> response) {
          std::lock_guard<std::mutex> lock(data_mutex_);
          response->path = latest_path_;
        });
    }
    if (!get_costmap_service_.empty()) {
      get_costmap_srv_ = create_service<srv::GetCostmap>(
        get_costmap_service_,
        [this](
          const std::shared_ptr<srv::GetCostmap::Request>,
          std::shared_ptr<srv::GetCostmap::Response> response) {
          std::lock_guard<std::mutex> lock(data_mutex_);
          response->costmap = have_costmap_ ? latest_costmap_ : latest_sensor_grid_;
        });
    }
    if (!set_controller_service_.empty()) {
      set_controller_srv_ = create_service<srv::SetController>(
        set_controller_service_,
        [this](
          const std::shared_ptr<srv::SetController::Request> request,
          std::shared_ptr<srv::SetController::Response> response) {
          response->success = setControllerId(request->controller_id, response->message);
        });
    }
    if (!set_planner_service_.empty()) {
      set_planner_srv_ = create_service<srv::SetPlanner>(
        set_planner_service_,
        [this](
          const std::shared_ptr<srv::SetPlanner::Request> request,
          std::shared_ptr<srv::SetPlanner::Response> response) {
          response->success = setPlannerId(request->planner_id, response->message);
        });
    }
  }

  void cancelNavigation()
  {
    paused_ = false;
    {
      std::lock_guard<std::mutex> lock(data_mutex_);
      latest_path_.poses.clear();
      have_path_ = false;
      have_active_goal_ = false;
      resetActiveController();
      resetRecovery();
    }
    publishStop();
    setNavigationState("CANCELED", "cancel requested");
  }

  void clearLocalObstacles()
  {
    {
      std::lock_guard<std::mutex> lock(data_mutex_);
      latest_costmap_.data.clear();
      latest_sensor_grid_.data.clear();
      have_costmap_ = false;
      have_sensor_grid_ = false;
    }
    setNavigationState(currentNavigationState(), "local obstacle grids cleared");
  }

  void clearGlobalMap()
  {
    {
      std::lock_guard<std::mutex> lock(data_mutex_);
      latest_map_.data.clear();
      have_map_ = false;
    }
    setNavigationState(currentNavigationState(), "map snapshot cleared");
  }

  void pauseNavigation()
  {
    paused_ = true;
    resetRecovery();
    publishStop();
    setNavigationState("PAUSED", "navigation paused");
  }

  void resumeNavigation()
  {
    paused_ = false;
    resetRecovery();
    if (hasActivePath()) {
      setNavigationState("CONTROLLING", "navigation resumed");
    } else {
      setNavigationState("IDLE", "navigation resumed without active path");
    }
  }

  bool hasActivePath()
  {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return have_path_ && !latest_path_.poses.empty();
  }

  void requestReplan()
  {
    geometry_msgs::msg::PoseStamped goal;
    {
      std::lock_guard<std::mutex> lock(data_mutex_);
      if (!have_active_goal_) {
        setNavigationState("FAILED", "replan requested without an active goal");
        return;
      }
      goal = active_goal_;
    }
    setGoalPath(goal);
  }

  nav_msgs::msg::Path makeStraightGoalPath(
    const geometry_msgs::msg::PoseStamped & start,
    const geometry_msgs::msg::PoseStamped & goal)
  {
    const auto planner = rai_planner::createPlanner("STRAIGHT_LINE");
    return planner->plan(start, goal, nullptr, plannerConfig(), now());
  }

  nav_msgs::msg::Path planGlobalPath(
    const geometry_msgs::msg::PoseStamped & start,
    const geometry_msgs::msg::PoseStamped & goal,
    const nav_msgs::msg::OccupancyGrid & map)
  {
    const std::string algorithm = normalizePlannerAlgorithm(global_planner_algorithm_);
    auto planner = rai_planner::createPlanner(algorithm);
    if (!planner) {
      return nav_msgs::msg::Path();
    }

    geometry_msgs::msg::PoseStamped start_for_planner = start;
    geometry_msgs::msg::PoseStamped goal_for_planner = goal;
    if (algorithm != "STRAIGHT_LINE") {
      if (!transformPoseToMap(start, map, start_for_planner) ||
        !transformPoseToMap(goal, map, goal_for_planner))
      {
        return nav_msgs::msg::Path();
      }
    }

    auto path = planner->plan(
      start_for_planner,
      goal_for_planner,
      algorithm == "STRAIGHT_LINE" ? nullptr : &map,
      plannerConfig(),
      now());
    if (path.poses.empty() && algorithm != "STRAIGHT_LINE") {
      RCLCPP_WARN(get_logger(), "Global planner %s found no path.", algorithm.c_str());
    }
    return path;
  }

  rai_planner::PlannerConfig plannerConfig() const
  {
    rai_planner::PlannerConfig config;
    config.path_spacing = generated_path_spacing_;
    config.inflation_radius = global_planner_inflation_radius_;
    config.smoothing_passes = global_path_smoothing_passes_;
    config.occupied_threshold = map_occupied_threshold_;
    config.treat_unknown_as_obstacle = treat_unknown_as_obstacle_;
    config.fallback_frame = global_frame_;
    return config;
  }

  std::string normalizePlannerAlgorithm(const std::string & value) const
  {
    const std::string normalized = rai_planner::normalizePlannerId(value);
    return isPlannerAvailable(normalized) ? normalized : "A_STAR";
  }

  bool isPlannerAvailable(const std::string & planner_id) const
  {
    const auto planners = rai_planner::availablePlannerIds();
    return std::find(planners.begin(), planners.end(), planner_id) != planners.end();
  }

  bool transformPoseToMap(
    const geometry_msgs::msg::PoseStamped & pose,
    const nav_msgs::msg::OccupancyGrid & map,
    geometry_msgs::msg::PoseStamped & out)
  {
    const std::string map_frame = map.header.frame_id.empty() ? global_frame_ : map.header.frame_id;
    try {
      auto pose_in = pose;
      pose_in.header.stamp = rclcpp::Time(0);
      out = tf_buffer_.transform(pose_in, map_frame, 100ms);
      return true;
    } catch (const tf2::TransformException & ex) {
      RCLCPP_WARN(get_logger(), "Global planner pose transform failed: %s", ex.what());
      return false;
    }
  }

  nav_msgs::msg::Path transformPathToControlFrame(const nav_msgs::msg::Path & path)
  {
    nav_msgs::msg::Path transformed;
    transformed.header.frame_id = control_frame_;
    transformed.header.stamp = now();

    const std::string source_frame = path.header.frame_id.empty() ? global_frame_ : path.header.frame_id;
    for (const auto & pose_in : path.poses) {
      geometry_msgs::msg::PoseStamped pose = pose_in;
      if (pose.header.frame_id.empty()) {
        pose.header.frame_id = source_frame;
      }
      try {
        pose.header.stamp = rclcpp::Time(0);
        auto out = tf_buffer_.transform(pose, control_frame_, 100ms);
        out.header.stamp = transformed.header.stamp;
        transformed.poses.push_back(out);
      } catch (const tf2::TransformException & ex) {
        RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000, "Path transform failed: %s", ex.what());
      }
    }
    return transformed;
  }

  void ingestLaserScan(const sensor_msgs::msg::LaserScan & scan)
  {
    geometry_msgs::msg::PoseStamped robot_pose;
    if (!lookupRobotPose(robot_pose)) {
      return;
    }

    const std::string scan_frame = scan.header.frame_id.empty() ? base_frame_ : scan.header.frame_id;
    const rclcpp::Time scan_time = scan.header.stamp;
    const rclcpp::Time grid_stamp = scan_time.nanoseconds() > 0 ? scan_time : now();
    nav_msgs::msg::OccupancyGrid grid = makeLocalObstacleGrid(robot_pose);
    grid.header.stamp = grid_stamp;

    for (size_t i = 0; i < scan.ranges.size(); ++i) {
      const float range = scan.ranges[i];
      if (!std::isfinite(range) || range < scan.range_min || range > scan.range_max) {
        continue;
      }

      const double angle = scan.angle_min + static_cast<double>(i) * scan.angle_increment;
      geometry_msgs::msg::PointStamped obstacle_in;
      obstacle_in.header.frame_id = scan_frame;
      obstacle_in.header.stamp = scan_time;
      obstacle_in.point.x = static_cast<double>(range) * std::cos(angle);
      obstacle_in.point.y = static_cast<double>(range) * std::sin(angle);

      try {
        const auto obstacle = tf_buffer_.transform(obstacle_in, control_frame_, 50ms);
        markObstacle(grid, obstacle.point.x, obstacle.point.y);
      } catch (const tf2::TransformException & ex) {
        RCLCPP_WARN_THROTTLE(
          get_logger(), *get_clock(), 2000,
          "Laser transform %s -> %s failed: %s",
          scan_frame.c_str(), control_frame_.c_str(), ex.what());
        return;
      }
    }

    {
      std::lock_guard<std::mutex> lock(data_mutex_);
      latest_sensor_grid_ = grid;
      have_sensor_grid_ = true;
      latest_laser_stamp_ = grid_stamp;
    }
    local_grid_pub_->publish(grid);
  }

  nav_msgs::msg::OccupancyGrid makeLocalObstacleGrid(
    const geometry_msgs::msg::PoseStamped & robot_pose) const
  {
    nav_msgs::msg::OccupancyGrid grid;
    grid.header.frame_id = control_frame_;
    grid.header.stamp = robot_pose.header.stamp;
    grid.info.resolution = std::max(0.01, local_grid_resolution_);
    grid.info.width = static_cast<uint32_t>(
      std::max(1.0, std::ceil(local_grid_width_m_ / grid.info.resolution)));
    grid.info.height = static_cast<uint32_t>(
      std::max(1.0, std::ceil(local_grid_height_m_ / grid.info.resolution)));
    grid.info.origin.position.x =
      robot_pose.pose.position.x - 0.5 * static_cast<double>(grid.info.width) * grid.info.resolution;
    grid.info.origin.position.y =
      robot_pose.pose.position.y - 0.5 * static_cast<double>(grid.info.height) * grid.info.resolution;
    grid.info.origin.orientation.w = 1.0;
    grid.data.assign(static_cast<size_t>(grid.info.width) * grid.info.height, 0);
    return grid;
  }

  void markObstacle(nav_msgs::msg::OccupancyGrid & grid, double x, double y) const
  {
    const double resolution = grid.info.resolution;
    const int center_x = static_cast<int>(std::floor((x - grid.info.origin.position.x) / resolution));
    const int center_y = static_cast<int>(std::floor((y - grid.info.origin.position.y) / resolution));
    const double max_radius = std::max(obstacle_inflation_radius_, slowdown_inflation_radius_);
    const int radius_cells = std::max(0, static_cast<int>(std::ceil(max_radius / resolution)));

    for (int dy = -radius_cells; dy <= radius_cells; ++dy) {
      for (int dx = -radius_cells; dx <= radius_cells; ++dx) {
        const int mx = center_x + dx;
        const int my = center_y + dy;
        if (mx < 0 || my < 0 || mx >= static_cast<int>(grid.info.width) ||
          my >= static_cast<int>(grid.info.height))
        {
          continue;
        }

        const double cell_dist = std::hypot(dx * resolution, dy * resolution);
        if (cell_dist > max_radius) {
          continue;
        }

        const size_t idx = static_cast<size_t>(my) * grid.info.width + static_cast<size_t>(mx);
        grid.data[idx] = static_cast<int8_t>(
          std::max(static_cast<int>(grid.data[idx]), obstacleCostForDistance(cell_dist)));
      }
    }
  }

  int obstacleCostForDistance(double distance) const
  {
    if (distance <= std::max(0.0, obstacle_lethal_radius_)) {
      return 100;
    }
    if (distance <= std::max(obstacle_lethal_radius_, obstacle_inflation_radius_)) {
      const double span = std::max(1e-3, obstacle_inflation_radius_ - obstacle_lethal_radius_);
      const double ratio = 1.0 - std::clamp((distance - obstacle_lethal_radius_) / span, 0.0, 1.0);
      return std::max(slowdown_cost_, static_cast<int>(std::round(
        slowdown_cost_ + ratio * (inscribed_cost_ - slowdown_cost_))));
    }
    if (distance <= slowdown_inflation_radius_) {
      const double span = std::max(1e-3, slowdown_inflation_radius_ - obstacle_inflation_radius_);
      const double ratio = 1.0 - std::clamp((distance - obstacle_inflation_radius_) / span, 0.0, 1.0);
      return std::max(1, static_cast<int>(std::round(ratio * slowdown_cost_)));
    }
    return 0;
  }

  nav_msgs::msg::OccupancyGrid projectMapToLocalGrid(
    const nav_msgs::msg::OccupancyGrid & map,
    const geometry_msgs::msg::PoseStamped & robot_pose)
  {
    nav_msgs::msg::OccupancyGrid local_grid = makeLocalObstacleGrid(robot_pose);
    local_grid.header.stamp = now();

    if (map.data.empty() || map.info.resolution <= 0.0) {
      return local_grid;
    }

    const std::string map_frame = map.header.frame_id.empty() ? global_frame_ : map.header.frame_id;
    for (uint32_t y = 0; y < local_grid.info.height; ++y) {
      for (uint32_t x = 0; x < local_grid.info.width; ++x) {
        const double wx = local_grid.info.origin.position.x +
          (static_cast<double>(x) + 0.5) * local_grid.info.resolution;
        const double wy = local_grid.info.origin.position.y +
          (static_cast<double>(y) + 0.5) * local_grid.info.resolution;

        geometry_msgs::msg::PointStamped point_in;
        point_in.header.frame_id = control_frame_;
        point_in.header.stamp = rclcpp::Time(0);
        point_in.point.x = wx;
        point_in.point.y = wy;

        geometry_msgs::msg::PointStamped point_on_map;
        try {
          point_on_map = tf_buffer_.transform(point_in, map_frame, 50ms);
        } catch (const tf2::TransformException & ex) {
          RCLCPP_WARN_THROTTLE(
            get_logger(), *get_clock(), 2000,
            "Map transform %s -> %s failed: %s",
            control_frame_.c_str(), map_frame.c_str(), ex.what());
          return local_grid;
        }

        const int raw = mapValueAt(map, point_on_map.point.x, point_on_map.point.y);
        if (isMapObstacle(raw)) {
          markObstacle(local_grid, wx, wy);
        }
      }
    }

    return local_grid;
  }

  int mapValueAt(const nav_msgs::msg::OccupancyGrid & map, double x, double y) const
  {
    const double resolution = map.info.resolution;
    const int mx = static_cast<int>(std::floor((x - map.info.origin.position.x) / resolution));
    const int my = static_cast<int>(std::floor((y - map.info.origin.position.y) / resolution));
    if (mx < 0 || my < 0 || mx >= static_cast<int>(map.info.width) ||
      my >= static_cast<int>(map.info.height))
    {
      return -1;
    }

    const size_t idx = static_cast<size_t>(my) * map.info.width + static_cast<size_t>(mx);
    return static_cast<int>(map.data[idx]);
  }

  bool isMapObstacle(int value) const
  {
    if (value < 0) {
      return treat_unknown_as_obstacle_;
    }
    return value >= map_occupied_threshold_;
  }

  void mergeObstacleGrid(
    nav_msgs::msg::OccupancyGrid & target,
    const nav_msgs::msg::OccupancyGrid & source) const
  {
    if (target.info.width != source.info.width || target.info.height != source.info.height ||
      target.info.resolution != source.info.resolution)
    {
      return;
    }

    const size_t count = std::min(target.data.size(), source.data.size());
    for (size_t i = 0; i < count; ++i) {
      target.data[i] = std::max(target.data[i], source.data[i]);
    }
  }

  bool lookupRobotPose(geometry_msgs::msg::PoseStamped & pose)
  {
    try {
      const auto tf = tf_buffer_.lookupTransform(control_frame_, base_frame_, tf2::TimePointZero, 100ms);
      pose.header.frame_id = control_frame_;
      pose.header.stamp = now();
      pose.pose.position.x = tf.transform.translation.x;
      pose.pose.position.y = tf.transform.translation.y;
      pose.pose.position.z = tf.transform.translation.z;
      pose.pose.orientation = tf.transform.rotation;
      return true;
    } catch (const tf2::TransformException & ex) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000, "Robot pose TF lookup failed: %s", ex.what());
      return false;
    }
  }

  void controlLoop()
  {
    geometry_msgs::msg::PoseStamped robot_pose;
    if (!lookupRobotPose(robot_pose)) {
      publishStop();
      setNavigationState("FAILED", "robot pose is unavailable");
      return;
    }

    nav_msgs::msg::Odometry odom;
    nav_msgs::msg::Path path;
    std::vector<HumanState> humans;
    ContextState context;
    nav_msgs::msg::OccupancyGrid costmap;
    nav_msgs::msg::OccupancyGrid map;
    nav_msgs::msg::OccupancyGrid sensor_grid;
    sensor_msgs::msg::Imu imu;
    geometry_msgs::msg::PoseStamped active_goal;
    bool have_costmap = false;
    bool have_map = false;
    bool have_sensor_grid = false;
    bool have_imu = false;
    bool have_active_goal = false;
    rclcpp::Time latest_laser_stamp;
    {
      std::lock_guard<std::mutex> lock(data_mutex_);
      if (!have_odom_) {
        publishStop();
        setNavigationState("FAILED", "odom is unavailable");
        return;
      }
      odom = latest_odom_;
      path = latest_path_;
      humans = latest_humans_;
      context = latest_context_;
      costmap = latest_costmap_;
      have_costmap = have_costmap_;
      map = latest_map_;
      have_map = have_map_;
      sensor_grid = latest_sensor_grid_;
      have_sensor_grid = have_sensor_grid_;
      latest_laser_stamp = latest_laser_stamp_;
      imu = latest_imu_;
      have_imu = have_imu_;
      active_goal = active_goal_;
      have_active_goal = have_active_goal_;
    }

    if (have_costmap && !costmap.header.frame_id.empty() && costmap.header.frame_id != control_frame_) {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 2000,
        "Ignoring costmap in frame %s; CCA core expects %s.",
        costmap.header.frame_id.c_str(), control_frame_.c_str());
      have_costmap = false;
    }

    if (!have_costmap && have_map) {
      costmap = projectMapToLocalGrid(map, robot_pose);
      have_costmap = true;
    }

    if (!have_costmap && have_sensor_grid) {
      if ((now() - latest_laser_stamp).seconds() <= max_laser_age_sec_) {
        costmap = sensor_grid;
        have_costmap = true;
      } else {
        RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000, "Laser grid is stale. Ignoring obstacles.");
      }
    }

    if (have_costmap && have_sensor_grid) {
      if ((now() - latest_laser_stamp).seconds() <= max_laser_age_sec_) {
        mergeObstacleGrid(costmap, sensor_grid);
      }
    }

    if ((now() - odom.header.stamp).seconds() > stop_on_stale_odom_sec_) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000, "Odom is stale. Publishing stop.");
      publishStop();
      setNavigationState("FAILED", "odom is stale");
      return;
    }

    if (use_imu_angular_velocity_ && have_imu && std::isfinite(imu.angular_velocity.z)) {
      const rclcpp::Time imu_stamp = imu.header.stamp;
      const bool is_fresh_imu = imu_stamp.nanoseconds() == 0 ||
        (now() - imu_stamp).seconds() <= max_imu_age_sec_;
      if (is_fresh_imu) {
        odom.twist.twist.angular.z = imu.angular_velocity.z;
      }
    }

    if (paused_) {
      publishStop();
      publishNavigationStatus();
      return;
    }

    if (path.poses.empty()) {
      if (stop_on_missing_plan_) {
        publishStop();
      }
      const std::string state = currentNavigationState();
      if (state == "PLANNING" || state == "CONTROLLING") {
        setNavigationState("IDLE", "waiting for path");
      } else {
        publishNavigationStatus();
      }
      return;
    }

    if (have_active_goal && isGoalReached(robot_pose, active_goal)) {
      {
        std::lock_guard<std::mutex> lock(data_mutex_);
        latest_path_.poses.clear();
        have_path_ = false;
        have_active_goal_ = false;
        resetActiveController();
      }
      publishStop();
      setNavigationState("SUCCEEDED", "goal reached");
      return;
    }

    if (have_costmap) {
      local_grid_pub_->publish(costmap);
    }

    const nav_msgs::msg::OccupancyGrid * costmap_ptr = have_costmap ? &costmap : nullptr;
    auto result = solveWithActiveController(
      robot_pose, odom.twist.twist, path, humans, context, costmap_ptr);
    result.timeout = result.solve_time_ms > max_solver_time_ms_;

    if (solverStatusIsFailure(result.status) && tryRecovery(result.status)) {
      publishSolverOutputs(result, false);
      return;
    }

    cmd_pub_->publish(result.command);
    recovery_attempts_ = 0;
    recovery_mode_ = "NONE";
    recovery_until_sec_ = 0.0;
    publishSolverOutputs(result, true);
  }

  void publishSolverOutputs(const SolveResult & result, bool update_state)
  {
    local_reference_pub_->publish(result.local_reference_path);
    predicted_pub_->publish(result.predicted_trajectory);

    msg::SolverStats stats;
    stats.header.stamp = now();
    stats.solve_time_ms = result.solve_time_ms;
    stats.iter_count = result.sample_count;
    stats.status = result.status;
    stats.timeout_flag = result.timeout;
    stats.collision_flag = result.collision;
    solver_stats_pub_->publish(stats);

    msg::AdaptiveBounds bounds;
    bounds.header.stamp = stats.header.stamp;
    bounds.vx_max = result.vx_max;
    bounds.vy_max = result.vy_max;
    bounds.omega_max = result.omega_max;
    bounds.d_safe = result.d_safe;
    bounds.q_scale = result.q_scale;
    adaptive_bounds_pub_->publish(bounds);

    if (update_state) {
      setNavigationState(solverStatusIsFailure(result.status) ? "FAILED" : "CONTROLLING", result.status);
    }
  }

  bool tryRecovery(const std::string & reason)
  {
    if (!recovery_enabled_ || recovery_attempts_ >= recovery_max_attempts_) {
      publishStop();
      setNavigationState("FAILED", reason);
      return false;
    }

    const double now_sec = now().seconds();
    if (recovery_mode_ != "NONE" && recovery_until_sec_ > 0.0 && now_sec < recovery_until_sec_) {
      geometry_msgs::msg::Twist cmd;
      if (recovery_mode_ == "BACKUP") {
        cmd.linear.x = -std::abs(recovery_backup_speed_);
      } else if (recovery_mode_ == "SPIN") {
        cmd.angular.z = recovery_spin_speed_;
      }
      cmd_pub_->publish(cmd);
      setNavigationState("RECOVERING", recovery_mode_ + ": " + reason);
      return true;
    }

    ++recovery_attempts_;
    if (recovery_attempts_ == 1) {
      clearLocalObstacles();
      suppress_recovery_reset_ = true;
      requestReplan();
      suppress_recovery_reset_ = false;
      publishStop();
      setNavigationState("RECOVERING", "clear_local_costmap_and_replan: " + reason);
      return true;
    }

    if (recovery_attempts_ == 2) {
      recovery_mode_ = "BACKUP";
      recovery_until_sec_ = now_sec + std::max(0.1, recovery_backup_duration_sec_);
      geometry_msgs::msg::Twist cmd;
      cmd.linear.x = -std::abs(recovery_backup_speed_);
      cmd_pub_->publish(cmd);
      setNavigationState("RECOVERING", "backup: " + reason);
      return true;
    }

    recovery_mode_ = "SPIN";
    recovery_until_sec_ = now_sec + std::max(0.1, recovery_spin_duration_sec_);
    geometry_msgs::msg::Twist cmd;
    cmd.angular.z = recovery_spin_speed_;
    cmd_pub_->publish(cmd);
    setNavigationState("RECOVERING", "spin: " + reason);
    return true;
  }

  void resetRecovery()
  {
    recovery_attempts_ = 0;
    recovery_mode_ = "NONE";
    recovery_until_sec_ = 0.0;
  }

  bool solverStatusIsFailure(const std::string & status) const
  {
    return status.find("NO_VALID") != std::string::npos ||
      status.find("INVALID") != std::string::npos ||
      status == "NO_REFERENCE_PATH";
  }

  void resetActiveController()
  {
    if (active_controller_) {
      active_controller_->reset();
    }
  }

  SolveResult solveWithActiveController(
    const geometry_msgs::msg::PoseStamped & robot_pose,
    const geometry_msgs::msg::Twist & robot_velocity,
    const nav_msgs::msg::Path & reference_path,
    const std::vector<HumanState> & humans,
    const ContextState & context,
    const nav_msgs::msg::OccupancyGrid * costmap)
  {
    if (active_controller_) {
      return active_controller_->solve(
        robot_pose, robot_velocity, reference_path, humans, context, costmap);
    }

    SolveResult result;
    result.status = "UNKNOWN_CONTROLLER:" + active_controller_id_;
    return result;
  }

  bool isGoalReached(
    const geometry_msgs::msg::PoseStamped & robot_pose,
    const geometry_msgs::msg::PoseStamped & goal) const
  {
    const double dx = robot_pose.pose.position.x - goal.pose.position.x;
    const double dy = robot_pose.pose.position.y - goal.pose.position.y;
    const double yaw_error = normalizeAngle(yawFromPose(robot_pose) - yawFromPose(goal));
    return std::hypot(dx, dy) <= goal_tolerance_xy_ && std::abs(yaw_error) <= goal_tolerance_yaw_;
  }

  double normalizeAngle(double angle) const
  {
    constexpr double pi = 3.14159265358979323846;
    while (angle > pi) {
      angle -= 2.0 * pi;
    }
    while (angle < -pi) {
      angle += 2.0 * pi;
    }
    return angle;
  }

  void setNavigationState(const std::string & state, const std::string & detail)
  {
    {
      std::lock_guard<std::mutex> lock(state_mutex_);
      navigation_state_ = state;
      navigation_detail_ = detail;
    }
    publishNavigationStatus();
  }

  std::string currentNavigationState()
  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return navigation_state_;
  }

  std::string currentNavigationDetail()
  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return navigation_detail_;
  }

  void publishNavigationStatus()
  {
    std::string state;
    std::string detail;
    {
      std::lock_guard<std::mutex> lock(state_mutex_);
      state = navigation_state_;
      detail = navigation_detail_;
    }

    std_msgs::msg::String status;
    status.data = state;
    if (!detail.empty()) {
      status.data += ": " + detail;
    }
    status_pub_->publish(status);
  }

  void publishStop()
  {
    cmd_pub_->publish(geometry_msgs::msg::Twist());
  }

  std::mutex data_mutex_;
  std::mutex state_mutex_;
  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;
  std::unique_ptr<ControllerAlgorithm> active_controller_;

  std::string global_frame_;
  std::string control_frame_;
  std::string base_frame_;
  std::string goal_topic_;
  std::string cancel_topic_;
  std::string clear_costmap_topic_;
  std::string replan_topic_;
  std::string status_topic_;
  std::string navigate_to_pose_service_;
  std::string cancel_service_;
  std::string clear_costmap_service_;
  std::string clear_local_costmap_service_;
  std::string clear_global_costmap_service_;
  std::string replan_service_;
  std::string pause_service_;
  std::string resume_service_;
  std::string get_status_service_;
  std::string get_path_service_;
  std::string get_costmap_service_;
  std::string list_algorithms_service_;
  std::string set_controller_service_;
  std::string set_planner_service_;
  std::string external_path_topic_;
  std::string global_path_topic_;
  std::string plan_topic_;
  std::string odom_topic_;
  std::string laser_scan_topic_;
  std::string imu_topic_;
  std::string map_topic_;
  std::string costmap_topic_;
  std::string local_grid_topic_;
  std::string cmd_vel_topic_;
  double generated_path_spacing_{0.05};
  std::string global_planner_algorithm_{"A_STAR"};
  int global_path_smoothing_passes_{1};
  double global_planner_inflation_radius_{0.22};
  double robot_radius_{0.22};              
  double local_grid_resolution_{0.05};
  double local_grid_width_m_{4.0};
  double local_grid_height_m_{4.0};
  double obstacle_inflation_radius_{0.22};
  double obstacle_lethal_radius_{0.22};
  double slowdown_inflation_radius_{0.55};
  int slowdown_cost_{35};
  int inscribed_cost_{85};
  int map_occupied_threshold_{65};
  bool treat_unknown_as_obstacle_{false};
  double max_laser_age_sec_{0.5};
  bool use_imu_angular_velocity_{true};
  double max_imu_age_sec_{0.5};
  double control_frequency_{25.0};
  double max_solver_time_ms_{35.0};
  double stop_on_stale_odom_sec_{0.5};
  bool stop_on_missing_plan_{true};
  double goal_tolerance_xy_{0.25};
  double goal_tolerance_yaw_{0.35};
  bool recovery_enabled_{true};
  int recovery_max_attempts_{3};
  double recovery_backup_speed_{0.08};
  double recovery_backup_duration_sec_{0.8};
  double recovery_spin_speed_{0.35};
  double recovery_spin_duration_sec_{1.2};
  std::string active_controller_id_{"CCA_NMPC"};
  std::string navigation_state_{"IDLE"};
  std::string navigation_detail_{"ready"};
  bool paused_{false};
  int recovery_attempts_{0};
  bool suppress_recovery_reset_{false};
  std::string recovery_mode_{"NONE"};
  double recovery_until_sec_{0.0};

  nav_msgs::msg::Odometry latest_odom_;
  nav_msgs::msg::Path latest_path_;
  geometry_msgs::msg::PoseStamped active_goal_;
  std::vector<HumanState> latest_humans_;
  ContextState latest_context_;
  nav_msgs::msg::OccupancyGrid latest_costmap_;
  nav_msgs::msg::OccupancyGrid latest_map_;
  nav_msgs::msg::OccupancyGrid latest_sensor_grid_;
  sensor_msgs::msg::Imu latest_imu_;
  rclcpp::Time latest_laser_stamp_;
  bool have_odom_{false};
  bool have_path_{false};
  bool have_active_goal_{false};
  bool have_costmap_{false};
  bool have_map_{false};
  bool have_sensor_grid_{false};
  bool have_imu_{false};

  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_pub_;
  rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr global_path_pub_;
  rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr local_reference_pub_;
  rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr predicted_pub_;
  rclcpp::Publisher<nav_msgs::msg::OccupancyGrid>::SharedPtr local_grid_pub_;
  rclcpp::Publisher<msg::SolverStats>::SharedPtr solver_stats_pub_;
  rclcpp::Publisher<msg::AdaptiveBounds>::SharedPtr adaptive_bounds_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;

  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr laser_sub_;
  rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr imu_sub_;
  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr map_sub_;
  rclcpp::Subscription<msg::HumanStates>::SharedPtr humans_sub_;
  rclcpp::Subscription<msg::Context>::SharedPtr context_sub_;
  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr costmap_sub_;
  rclcpp::Subscription<nav_msgs::msg::Path>::SharedPtr external_path_sub_;
  rclcpp::Subscription<nav_msgs::msg::Path>::SharedPtr plan_sub_;
  rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr goal_sub_;
  rclcpp::Subscription<std_msgs::msg::Empty>::SharedPtr cancel_sub_;
  rclcpp::Subscription<std_msgs::msg::Empty>::SharedPtr clear_costmap_sub_;
  rclcpp::Subscription<std_msgs::msg::Empty>::SharedPtr replan_sub_;
  rclcpp::Service<srv::NavigateToPose>::SharedPtr navigate_to_pose_srv_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr cancel_srv_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr clear_costmap_srv_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr clear_local_costmap_srv_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr clear_global_costmap_srv_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr replan_srv_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr pause_srv_;
  rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr resume_srv_;
  rclcpp::Service<srv::GetNavigationStatus>::SharedPtr get_status_srv_;
  rclcpp::Service<srv::GetPath>::SharedPtr get_path_srv_;
  rclcpp::Service<srv::GetCostmap>::SharedPtr get_costmap_srv_;
  rclcpp::Service<srv::ListAlgorithms>::SharedPtr list_algorithms_srv_;
  rclcpp::Service<srv::SetController>::SharedPtr set_controller_srv_;
  rclcpp::Service<srv::SetPlanner>::SharedPtr set_planner_srv_;
  rclcpp::TimerBase::SharedPtr control_timer_;
};

}  

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<rai_controller::RaiControllerServerNode>());
  rclcpp::shutdown();
  return 0;
}
