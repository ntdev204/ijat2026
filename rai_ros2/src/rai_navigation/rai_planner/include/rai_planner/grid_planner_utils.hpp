#ifndef RAI_PLANNER__GRID_PLANNER_UTILS_HPP_
#define RAI_PLANNER__GRID_PLANNER_UTILS_HPP_

#include <utility>
#include <vector>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "nav_msgs/msg/path.hpp"
#include "rai_planner/planner.hpp"

namespace rai_planner
{

bool worldToMap(
  const nav_msgs::msg::OccupancyGrid & map,
  double x,
  double y,
  int & mx,
  int & my);

std::vector<std::pair<int, int>> searchGridPath(
  const nav_msgs::msg::OccupancyGrid & map,
  int start_x,
  int start_y,
  int goal_x,
  int goal_y,
  bool use_heuristic,
  const PlannerConfig & config);

nav_msgs::msg::Path cellsToPath(
  const nav_msgs::msg::OccupancyGrid & map,
  const std::vector<std::pair<int, int>> & cells,
  const geometry_msgs::msg::PoseStamped & goal,
  const rclcpp::Time & stamp,
  const PlannerConfig & config);

nav_msgs::msg::Path makeStraightPath(
  const geometry_msgs::msg::PoseStamped & start,
  const geometry_msgs::msg::PoseStamped & goal,
  const PlannerConfig & config,
  const rclcpp::Time & stamp);

}  

#endif  
