#ifndef RAI_CONTROLLER_CCA_NMPC__CCANMPC_CORE_HPP_
#define RAI_CONTROLLER_CCA_NMPC__CCANMPC_CORE_HPP_

#include <string>
#include <vector>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "rclcpp/time.hpp"
#include "rai_controller_cca_nmpc/ccanmpc_types.hpp"
#include "rai_controller_cca_nmpc/human_predictor.hpp"

namespace rai_controller_cca_nmpc
{

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
  HumanPredictor human_predictor_;
  geometry_msgs::msg::Twist last_command_;

  nav_msgs::msg::Path pruneAndCropReference(
    const geometry_msgs::msg::PoseStamped & robot_pose,
    const nav_msgs::msg::Path & reference_path) const;

  std::vector<geometry_msgs::msg::PoseStamped> resampleReference(
    const geometry_msgs::msg::PoseStamped & robot_pose,
    const nav_msgs::msg::Path & local_path,
    double v_ref) const;

  double occupancyCost(const nav_msgs::msg::OccupancyGrid * costmap, double x, double y) const;
  ContextState computeAdaptiveParameters(double phi_h) const;
  ContextState mergeExternalLimits(
    const ContextState & adaptive,
    const ContextState & external_context) const;
  double adaptiveVelocityConstraintPenalty(
    double vx,
    double vy,
    double omega,
    const ContextState & adaptive,
    bool & violated) const;
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

}  // namespace rai_controller_cca_nmpc

#endif  // RAI_CONTROLLER_CCA_NMPC__CCANMPC_CORE_HPP_
