#ifndef NAV2_CANMPC_CONTROLLER__COSTMAP_EVALUATOR_HPP_
#define NAV2_CANMPC_CONTROLLER__COSTMAP_EVALUATOR_HPP_

#include "nav2_costmap_2d/costmap_2d.hpp"
#include "nav2_costmap_2d/cost_values.hpp"

namespace rai_canmpc_controller
{

class CostmapEvaluator
{
public:
  CostmapEvaluator() = default;
  ~CostmapEvaluator() = default;

  void setCostmap(nav2_costmap_2d::Costmap2D * costmap)
  {
    costmap_ = costmap;
  }

  // Returns normalized penalty in [0.0, 1.0]
  double evaluateCostAtPose(double x, double y) const
  {
    if (!costmap_) {
      return 1.0;
    }

    unsigned int mx, my;
    if (!costmap_->worldToMap(x, y, mx, my)) {
      return 1.0; // Outside map boundary is treated as lethal
    }

    unsigned char cost = costmap_->getCost(mx, my);
    if (cost == nav2_costmap_2d::LETHAL_OBSTACLE) {
      return 1.0;
    }
    if (cost == nav2_costmap_2d::NO_INFORMATION) {
      return 1.0; // Treat unknown/no information as lethal
    }
    if (cost == nav2_costmap_2d::FREE_SPACE) {
      return 0.0;
    }
    
    // Inflated cost: return normalized cost squared as per Option A in plan
    double normalized = static_cast<double>(cost) / 254.0;
    return normalized * normalized;
  }

private:
  nav2_costmap_2d::Costmap2D * costmap_{nullptr};
};

}  // namespace rai_canmpc_controller

#endif  // NAV2_CANMPC_CONTROLLER__COSTMAP_EVALUATOR_HPP_
