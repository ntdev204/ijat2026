#include "ccanmpc_costmap_layers/human_semantic_layer.hpp"
#include "nav2_costmap_2d/costmap_math.hpp"
#include "pluginlib/class_list_macros.hpp"

namespace ccanmpc_costmap_layers
{

void HumanSemanticLayer::onInitialize()
{
  auto node = node_.lock();
  if (!node) {
    throw std::runtime_error("Unable to lock parent node in costmap layer!");
  }

  // Declare parameters
  declareParameter("lethal_radius", rclcpp::ParameterValue(0.4));
  declareParameter("warning_radius", rclcpp::ParameterValue(1.2));
  declareParameter("warning_cost", rclcpp::ParameterValue(180));
  declareParameter("topic_name", rclcpp::ParameterValue("/canmpc/humans"));

  // Get parameters
  node->get_parameter(name_ + ".lethal_radius", lethal_radius_);
  node->get_parameter(name_ + ".warning_radius", warning_radius_);
  node->get_parameter(name_ + ".warning_cost", warning_cost_);
  node->get_parameter(name_ + ".topic_name", topic_name_);

  // Create subscriber in the costmap node
  human_states_sub_ = node->create_subscription<ccanmpc_msgs::msg::HumanStates>(
    topic_name_, 10,
    std::bind(&HumanSemanticLayer::humanStatesCallback, this, std::placeholders::_1));

  current_ = true;
  default_value_ = nav2_costmap_2d::FREE_SPACE;
  matchSize();

  RCLCPP_INFO(node->get_logger(), "HumanSemanticLayer initialized: lethal_r=%.2fm, warning_r=%.2fm, topic=%s",
              lethal_radius_, warning_radius_, topic_name_.c_str());
}

void HumanSemanticLayer::matchSize()
{
  Costmap2D * master = layered_costmap_->getCostmap();
  resizeMap(master->getSizeInCellsX(), master->getSizeInCellsY(),
            master->getResolution(), master->getOriginX(), master->getOriginY());
}

void HumanSemanticLayer::reset()
{
  std::lock_guard<std::mutex> lock(data_mutex_);
  latest_human_states_ = ccanmpc_msgs::msg::HumanStates();
  resetMaps();
  current_ = true;
}

bool HumanSemanticLayer::isClearable()
{
  return true;
}

void HumanSemanticLayer::humanStatesCallback(const ccanmpc_msgs::msg::HumanStates::SharedPtr msg)
{
  std::lock_guard<std::mutex> lock(data_mutex_);
  latest_human_states_ = *msg;
}

void HumanSemanticLayer::updateBounds(
  double robot_x, double robot_y, double robot_yaw,
  double * min_x, double * min_y, double * max_x, double * max_y)
{
  (void)robot_x;
  (void)robot_y;
  (void)robot_yaw;

  std::lock_guard<std::mutex> lock(data_mutex_);
  if (latest_human_states_.humans.empty()) {
    return;
  }

  // Expand bounds to include warning zones around all humans
  for (const auto & human : latest_human_states_.humans) {
    double hx = human.pose.position.x;
    double hy = human.pose.position.y;
    *min_x = std::min(*min_x, hx - warning_radius_);
    *min_y = std::min(*min_y, hy - warning_radius_);
    *max_x = std::max(*max_x, hx + warning_radius_);
    *max_y = std::max(*max_y, hy + warning_radius_);
  }
}

void HumanSemanticLayer::updateCosts(
  nav2_costmap_2d::Costmap2D & master_grid,
  int min_i, int min_j, int max_i, int max_j)
{
  if (!enabled_) {
    return;
  }

  // Clear internal costmap bounds
  for (int j = min_j; j < max_j; ++j) {
    for (int i = min_i; i < max_i; ++i) {
      setCost(i, j, nav2_costmap_2d::FREE_SPACE);
    }
  }

  std::lock_guard<std::mutex> lock(data_mutex_);
  if (latest_human_states_.humans.empty()) {
    return;
  }

  // Ingest costs around each human
  for (const auto & human : latest_human_states_.humans) {
    double hx = human.pose.position.x;
    double hy = human.pose.position.y;

    // Convert warning radius to cell range
    int warning_cells = std::ceil(warning_radius_ / resolution_);
    
    unsigned int mx, my;
    if (!worldToMap(hx, hy, mx, my)) {
      continue;
    }

    int start_x = std::max(min_i, static_cast<int>(mx) - warning_cells);
    int start_y = std::max(min_j, static_cast<int>(my) - warning_cells);
    int end_x = std::min(max_i, static_cast<int>(mx) + warning_cells + 1);
    int end_y = std::min(max_j, static_cast<int>(my) + warning_cells + 1);

    for (int y = start_y; y < end_y; ++y) {
      for (int x = start_x; x < end_x; ++x) {
        double cell_x, cell_y;
        mapToWorld(x, y, cell_x, cell_y);

        double dist = std::sqrt((cell_x - hx) * (cell_x - hx) + (cell_y - hy) * (cell_y - hy));
        unsigned char cost = nav2_costmap_2d::FREE_SPACE;

        if (dist <= lethal_radius_) {
          cost = nav2_costmap_2d::LETHAL_OBSTACLE;
        } else if (dist <= warning_radius_) {
          // Linear cost decay from warning_cost_ to FREE_SPACE
          double ratio = (dist - lethal_radius_) / (warning_radius_ - lethal_radius_);
          cost = static_cast<unsigned char>(warning_cost_ * (1.0 - ratio));
        }

        // Write to internal costmap
        unsigned char existing_cost = getCost(x, y);
        if (cost > existing_cost) {
          setCost(x, y, cost);
        }
      }
    }
  }

  // Merge internal costmap with master grid
  updateWithMax(master_grid, min_i, min_j, max_i, max_j);
}

}  // namespace ccanmpc_costmap_layers

// Register as a Costmap2D Layer plugin
PLUGINLIB_EXPORT_CLASS(ccanmpc_costmap_layers::HumanSemanticLayer, nav2_costmap_2d::Layer)
