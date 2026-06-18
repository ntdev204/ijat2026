#ifndef RAI_CCANMPC_CONTROLLER__CCANMPC_CORE_HPP_
#define RAI_CCANMPC_CONTROLLER__CCANMPC_CORE_HPP_

#include <string>
#include <vector>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "nav_msgs/msg/path.hpp"
#include "rclcpp/time.hpp"

namespace rai_ccanmpc_controller
{

struct HumanState
{
  int id{-1};
  double x{0.0};
  double y{0.0};
  double vx{0.0};
  double vy{0.0};
  double confidence{1.0};
  double age_sec{0.0};
  double covariance_x{0.0};
  double covariance_y{0.0};
  double covariance_vx{0.0};
  double covariance_vy{0.0};
};

struct CcanmpcParameters
{
  int horizon_steps{20};
  double sample_time{0.05};
  double max_reference_length{3.0};
  double default_v_ref{0.35};
  int vx_samples{7};
  int vy_samples{7};
  int omega_samples{9};
  double min_speed_xy{0.03};
  double acc_lim_x{0.8};
  double acc_lim_y{0.8};
  double acc_lim_theta{1.2};

  double beta{3.0};
  double d0{2.5};
  double d_safe_0{0.5};
  double d_safe_max{0.8};
  double vx_max_0{0.45};
  double vy_max_0{0.35};
  double omega_max_0{1.0};
  double vx_max_min{0.08};
  double vy_max_min{0.06};
  double omega_max_min{0.15};

  double q_x{10.0};
  double q_y{10.0};
  double q_theta{5.0};
  double q_human_x{35.0};
  double q_human_y{35.0};
  double q_human_theta{10.0};
  double r_vx{0.1};
  double r_vy{0.1};
  double r_omega{0.05};
  double rd_vx{1.0};
  double rd_vy{1.0};
  double rd_omega{0.5};
  double human_cost_weight{8.0};
  double costmap_cost_weight{0.02};
  double terminal_cost_weight{12.0};
  double w_slack{100000.0};
  double max_compute_time_ms{50.0};
  double max_human_age_sec{0.5};
};

struct ContextState
{
  double phi_h{0.0};
  double d_safe{0.0};
  double vx_max{0.0};
  double vy_max{0.0};
  double omega_max{0.0};
};

struct SolveResult
{
  geometry_msgs::msg::Twist command;
  nav_msgs::msg::Path local_reference_path;
  nav_msgs::msg::Path predicted_trajectory;
  double solve_time_ms{0.0};
  int sample_count{0};
  std::string status{"NO_PLAN"};
  bool timeout{false};
  bool collision{false};
  double phi_h{0.0};
  double d_safe{0.0};
  double vx_max{0.0};
  double vy_max{0.0};
  double omega_max{0.0};
  double q_scale{1.0};
};

class CcanmpcCore
{
public:
  explicit CcanmpcCore(CcanmpcParameters parameters);

  // Precondition: robot_pose, reference_path, human states, and occupancy grid
  // are expressed in the same global/control frame before calling the core.
  SolveResult solve(
    const geometry_msgs::msg::PoseStamped & robot_pose,
    const geometry_msgs::msg::Twist & robot_velocity,
    const nav_msgs::msg::Path & reference_path,
    const std::vector<HumanState> & humans,
    const ContextState & context,
    const nav_msgs::msg::OccupancyGrid * costmap);

  void reset();

private:
  CcanmpcParameters params_;
  geometry_msgs::msg::Twist last_command_;

  nav_msgs::msg::Path pruneAndCropReference(
    const geometry_msgs::msg::PoseStamped & robot_pose,
    const nav_msgs::msg::Path & reference_path) const;

  std::vector<geometry_msgs::msg::PoseStamped> resampleReference(
    const geometry_msgs::msg::PoseStamped & robot_pose,
    const nav_msgs::msg::Path & local_path,
    double v_ref) const;

  double nearestHumanDistance(
    const std::vector<HumanState> & humans,
    double x,
    double y,
    double time_from_now) const;

  double occupancyCost(const nav_msgs::msg::OccupancyGrid * costmap, double x, double y) const;
  double computePhi(double human_distance) const;
  ContextState computeAdaptiveParameters(double phi_h) const;
  ContextState mergeExternalLimits(
    const ContextState & adaptive,
    const ContextState & external_context) const;
  void rolloutMecanumStep(
    double & x,
    double & y,
    double & yaw,
    double vx,
    double vy,
    double omega,
    double dt) const;
  double scoreTrackingError(
    double x,
    double y,
    double yaw,
    const geometry_msgs::msg::PoseStamped & reference,
    double phi_h) const;
};

double normalizeAngle(double angle);
double yawFromPose(const geometry_msgs::msg::PoseStamped & pose);
geometry_msgs::msg::PoseStamped makePose(
  const std::string & frame_id,
  const rclcpp::Time & stamp,
  double x,
  double y,
  double yaw);

}  // namespace rai_ccanmpc_controller

#endif  // RAI_CCANMPC_CONTROLLER__CCANMPC_CORE_HPP_
