#include "cca_nmpc_controller/local_plan_handler.hpp"
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <cmath>
#include <algorithm>
#include <limits>
#include <rclcpp/logging.hpp>

namespace cca_nmpc_controller
{

void LocalPlanHandler::setGlobalPlan(const nav_msgs::msg::Path & path)
{
  std::lock_guard<std::recursive_mutex> lock(plan_mutex_);
  global_plan_ = path;
}

size_t LocalPlanHandler::prunePlan(const geometry_msgs::msg::PoseStamped & robot_pose)
{
  std::lock_guard<std::recursive_mutex> lock(plan_mutex_);
  if (global_plan_.poses.empty()) {
    return 0;
  }

  double min_dist = std::numeric_limits<double>::max();
  size_t closest_idx = 0;

  for (size_t i = 0; i < global_plan_.poses.size(); ++i) {
    double dx = global_plan_.poses[i].pose.position.x - robot_pose.pose.position.x;
    double dy = global_plan_.poses[i].pose.position.y - robot_pose.pose.position.y;
    double dist = dx * dx + dy * dy;
    if (dist < min_dist) {
      min_dist = dist;
      closest_idx = i;
    }
  }

  // Prune points before the closest one
  if (closest_idx > 0) {
    global_plan_.poses.erase(global_plan_.poses.begin(), global_plan_.poses.begin() + closest_idx);
  }

  return global_plan_.poses.size();
}

nav_msgs::msg::Path LocalPlanHandler::transformAndCropPlan(
  const geometry_msgs::msg::PoseStamped & robot_pose,
  const std::shared_ptr<tf2_ros::Buffer> & tf,
  const std::string & local_frame,
  const nav2_costmap_2d::Costmap2D & costmap,
  double max_path_length)
{
  std::lock_guard<std::recursive_mutex> lock(plan_mutex_);
  nav_msgs::msg::Path local_path;
  local_path.header.frame_id = local_frame;
  local_path.header.stamp = robot_pose.header.stamp;

  if (global_plan_.poses.empty()) {
    return local_path;
  }

  // 1. Prune plan first in the global plan frame.
  const std::string plan_frame = global_plan_.header.frame_id;
  if (plan_frame.empty() || robot_pose.header.frame_id == plan_frame) {
    prunePlan(robot_pose);
  } else {
    geometry_msgs::msg::PoseStamped prune_pose = robot_pose;
    prune_pose.header.stamp.sec = 0;
    prune_pose.header.stamp.nanosec = 0;
    try {
      prune_pose = tf->transform(prune_pose, plan_frame);
      prunePlan(prune_pose);
    } catch (const tf2::TransformException & ex) {
      RCLCPP_WARN(
        rclcpp::get_logger("LocalPlanHandler"),
        "Could not transform robot pose to plan frame for pruning: %s", ex.what());
    }
  }

  // 2. Transform poses to local costmap frame and check bounds
  double cum_dist = 0.0;
  geometry_msgs::msg::PoseStamped prev_pose;
  bool first = true;

  for (const auto & global_pose : global_plan_.poses) {
    geometry_msgs::msg::PoseStamped transformed_pose;
    try {
      auto pose_to_transform = global_pose;
      if (pose_to_transform.header.frame_id.empty()) {
        pose_to_transform.header.frame_id = plan_frame;
      }

      // Global plan stamps can be older than the TF cache; use latest TF for cropping.
      pose_to_transform.header.stamp.sec = 0;
      pose_to_transform.header.stamp.nanosec = 0;

      tf->transform(pose_to_transform, transformed_pose, local_frame);
      transformed_pose.header.stamp = robot_pose.header.stamp;
    }
    catch (const tf2::TransformException & ex) {
      RCLCPP_ERROR(rclcpp::get_logger("LocalPlanHandler"), "Could not transform pose: %s", ex.what());
      continue;
    }

    // Check costmap bounds
    unsigned int mx, my;
    if (!costmap.worldToMap(transformed_pose.pose.position.x, transformed_pose.pose.position.y, mx, my)) {
      // Target pose is outside local costmap, stop cropping here
      break;
    }

    // Check cumulative distance
    if (!first) {
      double dx = transformed_pose.pose.position.x - prev_pose.pose.position.x;
      double dy = transformed_pose.pose.position.y - prev_pose.pose.position.y;
      cum_dist += std::sqrt(dx * dx + dy * dy);
      if (cum_dist > max_path_length) {
        break;
      }
    }

    local_path.poses.push_back(transformed_pose);
    prev_pose = transformed_pose;
    first = false;
  }

  // If headings are not computed properly, compute them
  computePathHeadings(local_path);

  return local_path;
}

std::vector<geometry_msgs::msg::PoseStamped> LocalPlanHandler::resamplePath(
  const nav_msgs::msg::Path & path,
  const geometry_msgs::msg::PoseStamped & robot_pose,
  size_t num_steps,
  double dt,
  double v_ref)
{
  std::vector<geometry_msgs::msg::PoseStamped> trajectory;
  trajectory.reserve(num_steps + 1);

  if (path.poses.empty()) {
    // Fallback: fill with robot pose
    for (size_t i = 0; i <= num_steps; ++i) {
      trajectory.push_back(robot_pose);
    }
    return trajectory;
  }

  // Compute cumulative distances along the path
  std::vector<double> path_dist;
  path_dist.push_back(0.0);
  for (size_t i = 1; i < path.poses.size(); ++i) {
    double dx = path.poses[i].pose.position.x - path.poses[i - 1].pose.position.x;
    double dy = path.poses[i].pose.position.y - path.poses[i - 1].pose.position.y;
    double step = std::sqrt(dx * dx + dy * dy);
    path_dist.push_back(path_dist.back() + step);
  }

  double ds = v_ref * dt;

  for (size_t i = 0; i <= num_steps; ++i) {
    double target_s = i * ds;

    // Find the segment containing target_s
    auto it = std::lower_bound(path_dist.begin(), path_dist.end(), target_s);
    size_t idx = std::distance(path_dist.begin(), it);

    if (idx == 0) {
      trajectory.push_back(path.poses.front());
    } else if (idx >= path.poses.size()) {
      // Extrapolate or clamp to end
      trajectory.push_back(path.poses.back());
    } else {
      // Interpolate between idx - 1 and idx
      double s0 = path_dist[idx - 1];
      double s1 = path_dist[idx];
      double t = (target_s - s0) / (s1 - s0);

      const auto & p0 = path.poses[idx - 1].pose;
      const auto & p1 = path.poses[idx].pose;

      geometry_msgs::msg::PoseStamped interpolated_pose;
      interpolated_pose.header = path.header;
      interpolated_pose.pose.position.x = p0.position.x + t * (p1.position.x - p0.position.x);
      interpolated_pose.pose.position.y = p0.position.y + t * (p1.position.y - p0.position.y);
      interpolated_pose.pose.position.z = p0.position.z + t * (p1.position.z - p0.position.z);

      // Simple quaternion interpolation (lerp + normalize)
      double qx = p0.orientation.x + t * (p1.orientation.x - p0.orientation.x);
      double qy = p0.orientation.y + t * (p1.orientation.y - p0.orientation.y);
      double qz = p0.orientation.z + t * (p1.orientation.z - p0.orientation.z);
      double qw = p0.orientation.w + t * (p1.orientation.w - p0.orientation.w);
      double q_norm = std::sqrt(qx * qx + qy * qy + qz * qz + qw * qw);

      if (q_norm > 1e-4) {
        interpolated_pose.pose.orientation.x = qx / q_norm;
        interpolated_pose.pose.orientation.y = qy / q_norm;
        interpolated_pose.pose.orientation.z = qz / q_norm;
        interpolated_pose.pose.orientation.w = qw / q_norm;
      } else {
        interpolated_pose.pose.orientation = p0.orientation;
      }

      trajectory.push_back(interpolated_pose);
    }
  }

  return trajectory;
}

void LocalPlanHandler::computePathHeadings(nav_msgs::msg::Path & path)
{
  if (path.poses.size() < 2) {
    return;
  }

  for (size_t i = 0; i < path.poses.size() - 1; ++i) {
    double dx = path.poses[i + 1].pose.position.x - path.poses[i].pose.position.x;
    double dy = path.poses[i + 1].pose.position.y - path.poses[i].pose.position.y;
    double yaw = std::atan2(dy, dx);

    // If orientation is empty/identity or needs update, set it
    path.poses[i].pose.orientation.x = 0.0;
    path.poses[i].pose.orientation.y = 0.0;
    path.poses[i].pose.orientation.z = std::sin(yaw / 2.0);
    path.poses[i].pose.orientation.w = std::cos(yaw / 2.0);
  }

  // For the last pose, copy the second-to-last pose's orientation
  path.poses.back().pose.orientation = path.poses[path.poses.size() - 2].pose.orientation;
}

}  // namespace cca_nmpc_controller
