#ifndef NAV2_CCA_NMPC_CONTROLLER__LOCAL_PLAN_HANDLER_HPP_
#define NAV2_CCA_NMPC_CONTROLLER__LOCAL_PLAN_HANDLER_HPP_

#include <vector>
#include <string>
#include <memory>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <nav_msgs/msg/path.hpp>
#include <tf2_ros/buffer.h>
#include <nav2_costmap_2d/costmap_2d.hpp>
#include <mutex>

namespace cca_nmpc_controller
{

/**
 * @class LocalPlanHandler
 * @brief Manages the global reference plan, including pruning, frame transformations, 
 * cropping, and resampling to generate reference trajectories for MPC.
 */
class LocalPlanHandler
{
public:
  LocalPlanHandler() = default;
  ~LocalPlanHandler() = default;

  /**
   * @brief Set the global plan received from the planner
   * @param path The global plan
   */
  void setGlobalPlan(const nav_msgs::msg::Path & path);

  /**
   * @brief Prunes the plan up to the closest point to the robot's current pose.
   * @param robot_pose Current pose of the robot.
   * @return Index of the closest point in the current stored global plan.
   */
  size_t prunePlan(const geometry_msgs::msg::PoseStamped & robot_pose);

  /**
   * @brief Transforms, crops, and prunes the global plan to the local costmap frame and bounds.
   * @param robot_pose Current robot pose.
   * @param tf TF buffer for frame transformations.
   * @param local_frame Frame of the local costmap/controller.
   * @param costmap Reference to the local costmap to check bounds.
   * @param max_path_length Maximum path distance to keep in the local plan (meters).
   * @return Path transformed and cropped to local limits.
   */
  nav_msgs::msg::Path transformAndCropPlan(
    const geometry_msgs::msg::PoseStamped & robot_pose,
    const std::shared_ptr<tf2_ros::Buffer> & tf,
    const std::string & local_frame,
    const nav2_costmap_2d::Costmap2D & costmap,
    double max_path_length);

  /**
   * @brief Resamples the path to generate a reference trajectory of N steps with time step dt,
   * matching a reference velocity.
   * @param path Input pruned/cropped path.
   * @param robot_pose Current robot pose.
   * @param num_steps Horizon length N.
   * @param dt Time step.
   * @param v_ref Reference tracking velocity (m/s).
   * @return A vector of poses representing the reference trajectory of size N + 1.
   */
  static std::vector<geometry_msgs::msg::PoseStamped> resamplePath(
    const nav_msgs::msg::Path & path,
    const geometry_msgs::msg::PoseStamped & robot_pose,
    size_t num_steps,
    double dt,
    double v_ref);

  /**
   * @brief Computes headings (yaw) for path points based on the tangent direction.
   * @param path Path to update in-place.
   */
  static void computePathHeadings(nav_msgs::msg::Path & path);

  /**
   * @brief Get the stored global plan
   */
  nav_msgs::msg::Path getGlobalPlan() const
  {
    std::lock_guard<std::recursive_mutex> lock(plan_mutex_);
    return global_plan_;
  }

private:
  nav_msgs::msg::Path global_plan_;
  mutable std::recursive_mutex plan_mutex_;
};

}  // namespace cca_nmpc_controller

#endif  // NAV2_CCA_NMPC_CONTROLLER__LOCAL_PLAN_HANDLER_HPP_
