#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "geometry_msgs/msg/point_stamped.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "nav2_msgs/action/compute_path_to_pose.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "nav_msgs/msg/path.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "std_msgs/msg/header.hpp"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"

#include "rai_ccanmpc_controller/ccanmpc_core.hpp"
#include "rai_ccanmpc_controller/msg/adaptive_bounds.hpp"
#include "rai_ccanmpc_controller/msg/context.hpp"
#include "rai_ccanmpc_controller/msg/human_states.hpp"
#include "rai_ccanmpc_controller/msg/solver_stats.hpp"

using namespace std::chrono_literals;

namespace rai_ccanmpc_controller
{

class RaiCcanmpcControllerNode : public rclcpp::Node
{
public:
  using ComputePathToPose = nav2_msgs::action::ComputePathToPose;
  using GoalHandleComputePath = rclcpp_action::ClientGoalHandle<ComputePathToPose>;

  RaiCcanmpcControllerNode()
  : Node("rai_ccanmpc_controller"),
    tf_buffer_(this->get_clock()),
    tf_listener_(tf_buffer_),
    core_(loadCoreParameters())
  {
    global_frame_ = declare_parameter<std::string>("global_frame", "map");
    control_frame_ = declare_parameter<std::string>("control_frame", "odom_combined");
    base_frame_ = declare_parameter<std::string>("base_frame", "base_footprint");
    goal_topic_ = declare_parameter<std::string>("goal_topic", "/goal_pose");
    external_path_topic_ = declare_parameter<std::string>("external_path_topic", "/canmpc/global_path");
    plan_topic_ = declare_parameter<std::string>("plan_topic", "/plan");
    odom_topic_ = declare_parameter<std::string>("odom_topic", "/odom_combined");
    costmap_topic_ = declare_parameter<std::string>("costmap_topic", "/local_costmap/costmap");
    cmd_vel_topic_ = declare_parameter<std::string>("cmd_vel_topic", "/cca_nmpc/cmd_vel");
    planner_action_name_ = declare_parameter<std::string>("planner_action_name", "compute_path_to_pose");
    control_frequency_ = declare_parameter<double>("control_frequency", 20.0);
    max_solver_time_ms_ = declare_parameter<double>("max_solver_time_ms", 50.0);
    stop_on_stale_odom_sec_ = declare_parameter<double>("stop_on_stale_odom_sec", 0.5);
    stop_on_missing_plan_ = declare_parameter<bool>("stop_on_missing_plan", true);
    planner_id_ = declare_parameter<std::string>("planner_id", "");

    cmd_pub_ = create_publisher<geometry_msgs::msg::Twist>(cmd_vel_topic_, 10);
    local_reference_pub_ = create_publisher<nav_msgs::msg::Path>("/canmpc/local_reference_path", 10);
    predicted_pub_ = create_publisher<nav_msgs::msg::Path>("/canmpc/predicted_trajectory", 10);
    solver_stats_pub_ = create_publisher<msg::SolverStats>("/canmpc/solver_stats", 10);
    adaptive_bounds_pub_ = create_publisher<msg::AdaptiveBounds>("/canmpc/adaptive_bounds", 10);

    auto reliable_qos = rclcpp::QoS(10).reliable();
    auto costmap_qos = rclcpp::QoS(rclcpp::KeepLast(1)).reliable().transient_local();

    odom_sub_ = create_subscription<nav_msgs::msg::Odometry>(
      odom_topic_, reliable_qos,
      [this](nav_msgs::msg::Odometry::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        latest_odom_ = *msg;
        have_odom_ = true;
      });

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

    costmap_sub_ = create_subscription<nav_msgs::msg::OccupancyGrid>(
      costmap_topic_, costmap_qos,
      [this](nav_msgs::msg::OccupancyGrid::SharedPtr msg) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        latest_costmap_ = *msg;
        have_costmap_ = true;
      });

    external_path_sub_ = create_subscription<nav_msgs::msg::Path>(
      external_path_topic_, reliable_qos,
      [this](nav_msgs::msg::Path::SharedPtr msg) {
        setPath(*msg);
      });
    plan_sub_ = create_subscription<nav_msgs::msg::Path>(
      plan_topic_, reliable_qos,
      [this](nav_msgs::msg::Path::SharedPtr msg) {
        setPath(*msg);
      });

    goal_sub_ = create_subscription<geometry_msgs::msg::PoseStamped>(
      goal_topic_, reliable_qos,
      [this](geometry_msgs::msg::PoseStamped::SharedPtr msg) {
        requestGlobalPath(*msg);
      });

    planner_client_ = rclcpp_action::create_client<ComputePathToPose>(this, planner_action_name_);

    const auto period = std::chrono::duration<double>(1.0 / std::max(1.0, control_frequency_));
    control_timer_ = create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(period),
      std::bind(&RaiCcanmpcControllerNode::controlLoop, this));

    RCLCPP_INFO(
      get_logger(),
      "rai_ccanmpc_controller ready: goal=%s planner_action=%s output=%s",
      goal_topic_.c_str(), planner_action_name_.c_str(), cmd_vel_topic_.c_str());
  }

private:
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
    p.min_speed_xy = declare_parameter<double>("min_speed_xy", p.min_speed_xy);
    p.acc_lim_x = declare_parameter<double>("acc_lim_x", p.acc_lim_x);
    p.acc_lim_y = declare_parameter<double>("acc_lim_y", p.acc_lim_y);
    p.acc_lim_theta = declare_parameter<double>("acc_lim_theta", p.acc_lim_theta);
    p.beta = declare_parameter<double>("beta", p.beta);
    p.d0 = declare_parameter<double>("d0", p.d0);
    p.d_safe_0 = declare_parameter<double>("d_safe_0", p.d_safe_0);
    p.d_safe_max = declare_parameter<double>("d_safe_max", p.d_safe_max);
    p.vx_max_0 = declare_parameter<double>("vx_max_0", p.vx_max_0);
    p.vy_max_0 = declare_parameter<double>("vy_max_0", p.vy_max_0);
    p.omega_max_0 = declare_parameter<double>("omega_max_0", p.omega_max_0);
    p.vx_max_min = declare_parameter<double>("vx_max_min", p.vx_max_min);
    p.vy_max_min = declare_parameter<double>("vy_max_min", p.vy_max_min);
    p.omega_max_min = declare_parameter<double>("omega_max_min", p.omega_max_min);
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
    return p;
  }

  void requestGlobalPath(const geometry_msgs::msg::PoseStamped & goal)
  {
    if (!planner_client_->wait_for_action_server(200ms)) {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 2000,
        "Planner action %s is not available.", planner_action_name_.c_str());
      return;
    }

    ComputePathToPose::Goal path_goal;
    path_goal.goal = goal;
    path_goal.planner_id = planner_id_;

    auto options = rclcpp_action::Client<ComputePathToPose>::SendGoalOptions();
    options.result_callback =
      [this](const GoalHandleComputePath::WrappedResult & result) {
        if (result.code != rclcpp_action::ResultCode::SUCCEEDED) {
          RCLCPP_WARN(get_logger(), "Global planner failed with action result code %d.", static_cast<int>(result.code));
          publishStop();
          return;
        }
        setPath(result.result->path);
      };

    planner_client_->async_send_goal(path_goal, options);
  }

  void setPath(const nav_msgs::msg::Path & path)
  {
    nav_msgs::msg::Path transformed = transformPathToControlFrame(path);
    std::lock_guard<std::mutex> lock(data_mutex_);
    latest_path_ = transformed;
    have_path_ = !latest_path_.poses.empty();
    core_.reset();
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
      return;
    }

    nav_msgs::msg::Odometry odom;
    nav_msgs::msg::Path path;
    std::vector<HumanState> humans;
    ContextState context;
    nav_msgs::msg::OccupancyGrid costmap;
    bool have_costmap = false;
    {
      std::lock_guard<std::mutex> lock(data_mutex_);
      if (!have_odom_) {
        publishStop();
        return;
      }
      odom = latest_odom_;
      path = latest_path_;
      humans = latest_humans_;
      context = latest_context_;
      costmap = latest_costmap_;
      have_costmap = have_costmap_;
    }

    if (have_costmap && !costmap.header.frame_id.empty() && costmap.header.frame_id != control_frame_) {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 2000,
        "Ignoring costmap in frame %s; CCA core expects %s.",
        costmap.header.frame_id.c_str(), control_frame_.c_str());
      have_costmap = false;
    }

    if ((now() - odom.header.stamp).seconds() > stop_on_stale_odom_sec_) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000, "Odom is stale. Publishing stop.");
      publishStop();
      return;
    }

    if (path.poses.empty()) {
      if (stop_on_missing_plan_) {
        publishStop();
      }
      return;
    }

    const nav_msgs::msg::OccupancyGrid * costmap_ptr = have_costmap ? &costmap : nullptr;
    auto result = core_.solve(robot_pose, odom.twist.twist, path, humans, context, costmap_ptr);
    result.timeout = result.solve_time_ms > max_solver_time_ms_;

    cmd_pub_->publish(result.command);
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
  }

  void publishStop()
  {
    cmd_pub_->publish(geometry_msgs::msg::Twist());
  }

  std::mutex data_mutex_;
  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;
  CcanmpcCore core_;

  std::string global_frame_;
  std::string control_frame_;
  std::string base_frame_;
  std::string goal_topic_;
  std::string external_path_topic_;
  std::string plan_topic_;
  std::string odom_topic_;
  std::string costmap_topic_;
  std::string cmd_vel_topic_;
  std::string planner_action_name_;
  std::string planner_id_;
  double control_frequency_{20.0};
  double max_solver_time_ms_{50.0};
  double stop_on_stale_odom_sec_{0.5};
  bool stop_on_missing_plan_{true};

  nav_msgs::msg::Odometry latest_odom_;
  nav_msgs::msg::Path latest_path_;
  std::vector<HumanState> latest_humans_;
  ContextState latest_context_;
  nav_msgs::msg::OccupancyGrid latest_costmap_;
  bool have_odom_{false};
  bool have_path_{false};
  bool have_costmap_{false};

  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_pub_;
  rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr local_reference_pub_;
  rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr predicted_pub_;
  rclcpp::Publisher<msg::SolverStats>::SharedPtr solver_stats_pub_;
  rclcpp::Publisher<msg::AdaptiveBounds>::SharedPtr adaptive_bounds_pub_;

  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  rclcpp::Subscription<msg::HumanStates>::SharedPtr humans_sub_;
  rclcpp::Subscription<msg::Context>::SharedPtr context_sub_;
  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr costmap_sub_;
  rclcpp::Subscription<nav_msgs::msg::Path>::SharedPtr external_path_sub_;
  rclcpp::Subscription<nav_msgs::msg::Path>::SharedPtr plan_sub_;
  rclcpp::Subscription<geometry_msgs::msg::PoseStamped>::SharedPtr goal_sub_;
  rclcpp_action::Client<ComputePathToPose>::SharedPtr planner_client_;
  rclcpp::TimerBase::SharedPtr control_timer_;
};

}  // namespace rai_ccanmpc_controller

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<rai_ccanmpc_controller::RaiCcanmpcControllerNode>());
  rclcpp::shutdown();
  return 0;
}
