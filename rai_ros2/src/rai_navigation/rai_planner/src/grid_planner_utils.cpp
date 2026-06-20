#include "rai_planner/grid_planner_utils.hpp"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <functional>
#include <limits>
#include <queue>
#include <string>

#include "tf2/LinearMath/Matrix3x3.h"
#include "tf2/LinearMath/Quaternion.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"

namespace rai_planner
{
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

geometry_msgs::msg::PoseStamped makePose(
  const std::string & frame,
  const rclcpp::Time & stamp,
  double x,
  double y,
  double yaw)
{
  geometry_msgs::msg::PoseStamped pose;
  pose.header.frame_id = frame;
  pose.header.stamp = stamp;
  pose.pose.position.x = x;
  pose.pose.position.y = y;
  pose.pose.orientation.z = std::sin(yaw * 0.5);
  pose.pose.orientation.w = std::cos(yaw * 0.5);
  return pose;
}

bool isRawMapCellBlocked(
  const nav_msgs::msg::OccupancyGrid & map,
  int x,
  int y,
  const PlannerConfig & config)
{
  if (x < 0 || y < 0 || x >= static_cast<int>(map.info.width) ||
    y >= static_cast<int>(map.info.height))
  {
    return true;
  }
  const size_t idx = static_cast<size_t>(y) * map.info.width + static_cast<size_t>(x);
  const int value = static_cast<int>(map.data[idx]);
  if (value < 0) {
    return config.treat_unknown_as_obstacle;
  }
  return value >= config.occupied_threshold;
}

bool isMapCellBlocked(
  const nav_msgs::msg::OccupancyGrid & map,
  int x,
  int y,
  const PlannerConfig & config)
{
  const int radius_cells = std::max(
    0,
    static_cast<int>(std::ceil(config.inflation_radius / std::max(0.01, map.info.resolution))));
  for (int dy = -radius_cells; dy <= radius_cells; ++dy) {
    for (int dx = -radius_cells; dx <= radius_cells; ++dx) {
      if (isRawMapCellBlocked(map, x + dx, y + dy, config)) {
        return true;
      }
    }
  }
  return false;
}

bool hasLineOfSight(
  const nav_msgs::msg::OccupancyGrid & map,
  const std::pair<int, int> & a,
  const std::pair<int, int> & b,
  const PlannerConfig & config)
{
  int x0 = a.first;
  int y0 = a.second;
  const int x1 = b.first;
  const int y1 = b.second;
  const int dx = std::abs(x1 - x0);
  const int sx = x0 < x1 ? 1 : -1;
  const int dy = -std::abs(y1 - y0);
  const int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;

  while (true) {
    if (isMapCellBlocked(map, x0, y0, config)) {
      return false;
    }
    if (x0 == x1 && y0 == y1) {
      return true;
    }
    const int e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

std::vector<std::pair<int, int>> smoothGridPath(
  const nav_msgs::msg::OccupancyGrid & map,
  std::vector<std::pair<int, int>> cells,
  const PlannerConfig & config)
{
  for (int pass = 0; pass < std::max(0, config.smoothing_passes); ++pass) {
    if (cells.size() < 3) {
      return cells;
    }
    std::vector<std::pair<int, int>> smoothed;
    size_t anchor = 0;
    smoothed.push_back(cells.front());
    while (anchor + 1 < cells.size()) {
      size_t next = cells.size() - 1;
      while (next > anchor + 1 && !hasLineOfSight(map, cells[anchor], cells[next], config)) {
        --next;
      }
      smoothed.push_back(cells[next]);
      anchor = next;
    }
    cells = smoothed;
  }
  return cells;
}

}  // namespace

bool worldToMap(
  const nav_msgs::msg::OccupancyGrid & map,
  double x,
  double y,
  int & mx,
  int & my)
{
  if (map.info.resolution <= 0.0) {
    return false;
  }
  mx = static_cast<int>(std::floor((x - map.info.origin.position.x) / map.info.resolution));
  my = static_cast<int>(std::floor((y - map.info.origin.position.y) / map.info.resolution));
  return mx >= 0 && my >= 0 && mx < static_cast<int>(map.info.width) &&
    my < static_cast<int>(map.info.height);
}

std::vector<std::pair<int, int>> searchGridPath(
  const nav_msgs::msg::OccupancyGrid & map,
  int start_x,
  int start_y,
  int goal_x,
  int goal_y,
  bool use_heuristic,
  const PlannerConfig & config)
{
  const int width = static_cast<int>(map.info.width);
  const int height = static_cast<int>(map.info.height);
  const int cell_count = width * height;
  if (cell_count <= 0 || isMapCellBlocked(map, start_x, start_y, config) ||
    isMapCellBlocked(map, goal_x, goal_y, config))
  {
    return {};
  }

  struct QueueNode
  {
    double priority;
    int x;
    int y;
    bool operator>(const QueueNode & other) const {return priority > other.priority;}
  };

  auto index = [width](int x, int y) {return y * width + x;};
  auto heuristic = [goal_x, goal_y](int x, int y) {
      return std::hypot(static_cast<double>(x - goal_x), static_cast<double>(y - goal_y));
    };

  std::priority_queue<QueueNode, std::vector<QueueNode>, std::greater<QueueNode>> open;
  std::vector<double> cost(static_cast<size_t>(cell_count), std::numeric_limits<double>::infinity());
  std::vector<int> parent(static_cast<size_t>(cell_count), -1);

  const int start_idx = index(start_x, start_y);
  cost[start_idx] = 0.0;
  open.push({use_heuristic ? heuristic(start_x, start_y) : 0.0, start_x, start_y});

  const std::vector<std::pair<int, int>> neighbors = {
    {-1, 0}, {1, 0}, {0, -1}, {0, 1},
    {-1, -1}, {-1, 1}, {1, -1}, {1, 1},
  };

  while (!open.empty()) {
    const auto current = open.top();
    open.pop();
    if (current.x == goal_x && current.y == goal_y) {
      break;
    }

    const int current_idx = index(current.x, current.y);
    for (const auto & delta : neighbors) {
      const int nx = current.x + delta.first;
      const int ny = current.y + delta.second;
      if (nx < 0 || ny < 0 || nx >= width || ny >= height ||
        isMapCellBlocked(map, nx, ny, config))
      {
        continue;
      }

      const double step = (delta.first != 0 && delta.second != 0) ? std::sqrt(2.0) : 1.0;
      const int next_idx = index(nx, ny);
      const double next_cost = cost[current_idx] + step;
      if (next_cost >= cost[next_idx]) {
        continue;
      }

      cost[next_idx] = next_cost;
      parent[next_idx] = current_idx;
      const double priority = next_cost + (use_heuristic ? heuristic(nx, ny) : 0.0);
      open.push({priority, nx, ny});
    }
  }

  const int goal_idx = index(goal_x, goal_y);
  if (!std::isfinite(cost[goal_idx])) {
    return {};
  }

  std::vector<std::pair<int, int>> cells;
  for (int idx = goal_idx; idx >= 0; idx = parent[idx]) {
    cells.emplace_back(idx % width, idx / width);
    if (idx == start_idx) {
      break;
    }
  }
  std::reverse(cells.begin(), cells.end());
  return smoothGridPath(map, cells, config);
}

nav_msgs::msg::Path cellsToPath(
  const nav_msgs::msg::OccupancyGrid & map,
  const std::vector<std::pair<int, int>> & cells,
  const geometry_msgs::msg::PoseStamped & goal,
  const rclcpp::Time & stamp,
  const PlannerConfig & config)
{
  nav_msgs::msg::Path path;
  path.header.frame_id = map.header.frame_id.empty() ? config.fallback_frame : map.header.frame_id;
  path.header.stamp = stamp;
  path.poses.reserve(cells.size());

  for (size_t i = 0; i < cells.size(); ++i) {
    const auto & cell = cells[i];
    const double x = map.info.origin.position.x +
      (static_cast<double>(cell.first) + 0.5) * map.info.resolution;
    const double y = map.info.origin.position.y +
      (static_cast<double>(cell.second) + 0.5) * map.info.resolution;
    double yaw = yawFromPose(goal);
    if (i + 1 < cells.size()) {
      const auto & next = cells[i + 1];
      yaw = std::atan2(
        static_cast<double>(next.second - cell.second),
        static_cast<double>(next.first - cell.first));
    }
    path.poses.push_back(makePose(path.header.frame_id, stamp, x, y, yaw));
  }

  if (!path.poses.empty()) {
    path.poses.back().pose.orientation = goal.pose.orientation;
  }
  return path;
}

nav_msgs::msg::Path makeStraightPath(
  const geometry_msgs::msg::PoseStamped & start,
  const geometry_msgs::msg::PoseStamped & goal,
  const PlannerConfig & config,
  const rclcpp::Time & stamp)
{
  nav_msgs::msg::Path path;
  path.header.frame_id = start.header.frame_id.empty() ? config.fallback_frame : start.header.frame_id;
  path.header.stamp = stamp;

  const double sx = start.pose.position.x;
  const double sy = start.pose.position.y;
  const double gx = goal.pose.position.x;
  const double gy = goal.pose.position.y;
  const double distance = std::hypot(gx - sx, gy - sy);
  const double spacing = std::max(0.01, config.path_spacing);
  const int steps = std::max(1, static_cast<int>(std::ceil(distance / spacing)));
  const double line_yaw = distance > 1e-3 ? std::atan2(gy - sy, gx - sx) : yawFromPose(goal);
  const double final_yaw = yawFromPose(goal);

  path.poses.reserve(static_cast<size_t>(steps + 1));
  for (int i = 0; i <= steps; ++i) {
    const double ratio = static_cast<double>(i) / static_cast<double>(steps);
    const double yaw = i == steps ? final_yaw : line_yaw;
    path.poses.push_back(makePose(
      path.header.frame_id,
      stamp,
      sx + ratio * (gx - sx),
      sy + ratio * (gy - sy),
      yaw));
  }
  return path;
}

}  // namespace rai_planner
