#ifndef RAI_PLANNER__PLANNER_HPP_
#define RAI_PLANNER__PLANNER_HPP_

#include <memory>
#include <string>
#include <vector>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "nav_msgs/msg/path.hpp"
#include "rclcpp/time.hpp"

namespace rai_planner
{

struct PlannerConfig
{
  double path_spacing{0.05};
  double inflation_radius{0.22};
  int smoothing_passes{1};
  int occupied_threshold{65};
  bool treat_unknown_as_obstacle{false};
  std::string fallback_frame{"map"};
};

class Planner
{
public:
  virtual ~Planner() = default;
  virtual std::string id() const = 0;
  virtual nav_msgs::msg::Path plan(
    const geometry_msgs::msg::PoseStamped & start,
    const geometry_msgs::msg::PoseStamped & goal,
    const nav_msgs::msg::OccupancyGrid * map,
    const PlannerConfig & config,
    const rclcpp::Time & stamp) const = 0;
};

using PlannerFactory = std::unique_ptr<Planner> (*)();

std::string normalizePlannerId(std::string value);
const std::vector<std::string> & availablePlannerIds();
std::unique_ptr<Planner> createPlanner(const std::string & id);
void registerPlanner(const std::string & id, PlannerFactory factory);

}  // namespace rai_planner

#endif  // RAI_PLANNER__PLANNER_HPP_
