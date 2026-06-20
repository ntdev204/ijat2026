#ifndef RAI_CONTROLLER__CONTROLLER_ALGORITHM_HPP_
#define RAI_CONTROLLER__CONTROLLER_ALGORITHM_HPP_

#include <memory>
#include <string>
#include <vector>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "nav_msgs/msg/path.hpp"

namespace rai_controller
{

struct ControllerHumanState
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

struct ControllerConfig
{
  int horizon_steps{20};
  double sample_time{0.05};
  double max_reference_length{3.0};
  double default_v_ref{0.35};
  int vx_samples{7};
  int vy_samples{7};
  int omega_samples{9};
  bool adaptive_sampling{true};
  int refinement_samples{3};
  int max_control_samples{180};
  double refinement_radius_fraction{0.35};
  double timeout_margin_ms{3.0};
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
  double costmap_collision_threshold{98.0};
};

struct ControllerContext
{
  double phi_h{0.0};
  double d_safe{0.0};
  double vx_max{0.0};
  double vy_max{0.0};
  double omega_max{0.0};
};

struct ControllerResult
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

using HumanState = ControllerHumanState;
using CcanmpcParameters = ControllerConfig;
using ContextState = ControllerContext;
using SolveResult = ControllerResult;

class ControllerAlgorithm
{
public:
  virtual ~ControllerAlgorithm() = default;
  virtual std::string id() const = 0;
  virtual ControllerResult solve(
    const geometry_msgs::msg::PoseStamped & robot_pose,
    const geometry_msgs::msg::Twist & robot_velocity,
    const nav_msgs::msg::Path & reference_path,
    const std::vector<ControllerHumanState> & humans,
    const ControllerContext & context,
    const nav_msgs::msg::OccupancyGrid * costmap) = 0;
  virtual void reset() = 0;
};

using ControllerFactory = std::unique_ptr<ControllerAlgorithm> (*)(const ControllerConfig & config);

std::string normalizeControllerId(std::string value);
const std::vector<std::string> & availableControllerIds();
std::unique_ptr<ControllerAlgorithm> createController(
  const std::string & id,
  const ControllerConfig & config);
void registerController(const std::string & id, ControllerFactory factory);

}  // namespace rai_controller

#endif  // RAI_CONTROLLER__CONTROLLER_ALGORITHM_HPP_
