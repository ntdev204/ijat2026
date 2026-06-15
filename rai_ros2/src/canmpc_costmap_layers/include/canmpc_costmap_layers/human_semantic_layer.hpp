#ifndef CANMPC_COSTMAP_LAYERS__HUMAN_SEMANTIC_LAYER_HPP_
#define CANMPC_COSTMAP_LAYERS__HUMAN_SEMANTIC_LAYER_HPP_

#include <mutex>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "nav2_costmap_2d/layer.hpp"
#include "nav2_costmap_2d/layered_costmap.hpp"
#include "nav2_costmap_2d/costmap_layer.hpp"
#include "canmpc_msgs/msg/human_states.hpp"

namespace canmpc_costmap_layers
{

/**
 * @class HumanSemanticLayer
 * @brief Custom Nav2 Costmap Layer that injects lethal/high-cost zones around tracked human positions.
 */
class HumanSemanticLayer : public nav2_costmap_2d::CostmapLayer
{
public:
  HumanSemanticLayer() = default;
  ~HumanSemanticLayer() override = default;

  // Layer lifecycle overrides
  void onInitialize() override;
  void updateBounds(
    double robot_x, double robot_y, double robot_yaw,
    double * min_x, double * min_y, double * max_x, double * max_y) override;
  void updateCosts(
    nav2_costmap_2d::Costmap2D & master_grid,
    int min_i, int min_j, int max_i, int max_j) override;
  void reset() override;
  bool isClearable() override;

  // Handle costmap resizing
  void matchSize() override;

protected:
  // ROS 2 parameters
  double lethal_radius_{0.4};
  double warning_radius_{1.2};
  int warning_cost_{180};
  std::string topic_name_{"/canmpc/humans"};

  // Subscriber
  rclcpp::Subscription<canmpc_msgs::msg::HumanStates>::SharedPtr human_states_sub_;
  canmpc_msgs::msg::HumanStates latest_human_states_;
  std::mutex data_mutex_;

  // Callback
  void humanStatesCallback(const canmpc_msgs::msg::HumanStates::SharedPtr msg);
};

}  // namespace canmpc_costmap_layers

#endif  // CANMPC_COSTMAP_LAYERS__HUMAN_SEMANTIC_LAYER_HPP_
