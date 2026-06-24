#include "rai_planner_a_star/a_star_planner.hpp"

#include <memory>
#include <string>

#include "rai_planner/grid_planner_utils.hpp"

namespace rai_planner_a_star
{
namespace
{

class AStarPlanner : public rai_planner::Planner
{
public:
  std::string id() const override {return "A_STAR";}

  nav_msgs::msg::Path plan(
    const geometry_msgs::msg::PoseStamped & start,
    const geometry_msgs::msg::PoseStamped & goal,
    const nav_msgs::msg::OccupancyGrid * map,
    const rai_planner::PlannerConfig & config,
    const rclcpp::Time & stamp) const override
  {
    if (!map) {
      return nav_msgs::msg::Path();
    }

    int start_x = 0;
    int start_y = 0;
    int goal_x = 0;
    int goal_y = 0;
    if (!rai_planner::worldToMap(*map, start.pose.position.x, start.pose.position.y, start_x, start_y) ||
      !rai_planner::worldToMap(*map, goal.pose.position.x, goal.pose.position.y, goal_x, goal_y))
    {
      return nav_msgs::msg::Path();
    }

    const auto cells = rai_planner::searchGridPath(
      *map, start_x, start_y, goal_x, goal_y, true, config);
    if (cells.empty()) {
      return nav_msgs::msg::Path();
    }
    return rai_planner::cellsToPath(*map, cells, goal, stamp, config);
  }
};

std::unique_ptr<rai_planner::Planner> create()
{
  return std::make_unique<AStarPlanner>();
}

}  

void registerAStarPlanner()
{
  rai_planner::registerPlanner("A_STAR", create);
}

}  
