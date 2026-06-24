#include "rai_planner_straight_line/straight_line_planner.hpp"

#include <memory>
#include <string>

#include "rai_planner/grid_planner_utils.hpp"

namespace rai_planner_straight_line
{
namespace
{

class StraightLinePlanner : public rai_planner::Planner
{
public:
  std::string id() const override {return "STRAIGHT_LINE";}

  nav_msgs::msg::Path plan(
    const geometry_msgs::msg::PoseStamped & start,
    const geometry_msgs::msg::PoseStamped & goal,
    const nav_msgs::msg::OccupancyGrid *,
    const rai_planner::PlannerConfig & config,
    const rclcpp::Time & stamp) const override
  {
    return rai_planner::makeStraightPath(start, goal, config, stamp);
  }
};

std::unique_ptr<rai_planner::Planner> create()
{
  return std::make_unique<StraightLinePlanner>();
}

}  

void registerStraightLinePlanner()
{
  rai_planner::registerPlanner("STRAIGHT_LINE", create);
}

}  
