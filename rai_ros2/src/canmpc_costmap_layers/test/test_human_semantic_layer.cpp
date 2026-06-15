#include <gtest/gtest.h>
#include "canmpc_costmap_layers/human_semantic_layer.hpp"
#include "nav2_costmap_2d/costmap_2d.hpp"
#include <cmath>
#include <limits>

using nav2_canmpc_costmap_layers::HumanSemanticLayer;

TEST(HumanSemanticLayerTest, UpdateBoundsAndCosts)
{
  HumanSemanticLayer layer;
  layer.configureLayer(1.0, 0.5); // human_radius = 1.0, lethal_radius = 0.5

  // Prepare a single human state at position (1.0, 2.0)
  canmpc_msgs::msg::HumanStates msg;
  msg.header.frame_id = "map";
  
  canmpc_msgs::msg::HumanState human;
  human.id = 42;
  human.pose.position.x = 1.0;
  human.pose.position.y = 2.0;
  human.pose.position.z = 0.0;
  human.confidence = 0.95;
  msg.humans.push_back(human);

  layer.setHumans(msg);

  // 1. Test updateBounds
  double min_x = 0.0;
  double min_y = 0.0;
  double max_x = 0.0;
  double max_y = 0.0;

  // Since human is at (1.0, 2.0) and radius is 1.0:
  // Expected bounds:
  // min_x = min(0.0, 1.0 - 1.0) = 0.0
  // max_x = max(0.0, 1.0 + 1.0) = 2.0
  // min_y = min(0.0, 2.0 - 1.0) = 1.0
  // max_y = max(0.0, 2.0 + 1.0) = 3.0
  layer.updateBounds(0.0, 0.0, 0.0, &min_x, &min_y, &max_x, &max_y);

  EXPECT_DOUBLE_EQ(min_x, 0.0);
  EXPECT_DOUBLE_EQ(max_x, 2.0);
  EXPECT_DOUBLE_EQ(min_y, 1.0);
  EXPECT_DOUBLE_EQ(max_y, 3.0);

  // 2. Test updateCosts
  // Create master costmap: size=40x40, resolution=0.1, origin=(-1.0, -1.0)
  nav2_costmap_2d::Costmap2D master_grid(40, 40, 0.1, -1.0, -1.0, 0);

  // Initialize master grid to FREE_SPACE (0)
  for (unsigned int x = 0; x < 40; ++x) {
    for (unsigned int y = 0; y < 40; ++y) {
      master_grid.setCost(x, y, nav2_costmap_2d::FREE_SPACE);
    }
  }

  // Update costs in index bounds covering the human circle
  // human center (1.0, 2.0) is at map coords:
  // mx = (1.0 - (-1.0)) / 0.1 = 20
  // my = (2.0 - (-1.0)) / 0.1 = 30
  // Bounds around center with radius 1.0 (10 cells): [10, 30] and [20, 40]
  layer.updateCosts(master_grid, 10, 20, 30, 40);

  // Verify costs at different distances:
  // Center (1.0, 2.0) -> distance = 0.0 <= lethal_radius (0.5) -> LETHAL_OBSTACLE (254)
  unsigned int mx_center, my_center;
  ASSERT_TRUE(master_grid.worldToMap(1.0, 2.0, mx_center, my_center));
  EXPECT_EQ(master_grid.getCost(mx_center, my_center), nav2_costmap_2d::LETHAL_OBSTACLE);

  // Inside lethal zone: (1.2, 2.0) -> distance = 0.2 <= 0.5 -> LETHAL_OBSTACLE (254)
  unsigned int mx_lethal, my_lethal;
  ASSERT_TRUE(master_grid.worldToMap(1.2, 2.0, mx_lethal, my_lethal));
  EXPECT_EQ(master_grid.getCost(mx_lethal, my_lethal), nav2_costmap_2d::LETHAL_OBSTACLE);

  // Inside semantic transition zone: (1.75, 2.0) -> distance = 0.75
  // factor = (1.0 - 0.75) / (1.0 - 0.5) = 0.25 / 0.5 = 0.5
  // expected cost = 0.5 * 254 = 127
  unsigned int mx_transition, my_transition;
  ASSERT_TRUE(master_grid.worldToMap(1.75, 2.0, mx_transition, my_transition));
  unsigned char transition_cost = master_grid.getCost(mx_transition, my_transition);
  EXPECT_NEAR(static_cast<double>(transition_cost), 127.0, 5.0);

  // Outside semantic zone: (2.2, 2.0) -> distance = 1.2 > 1.0 -> FREE_SPACE (0)
  unsigned int mx_outside, my_outside;
  ASSERT_TRUE(master_grid.worldToMap(2.2, 2.0, mx_outside, my_outside));
  EXPECT_EQ(master_grid.getCost(mx_outside, my_outside), nav2_costmap_2d::FREE_SPACE);
}

TEST(HumanSemanticLayerTest, DisabledLayer)
{
  HumanSemanticLayer layer;
  layer.configureLayer(1.0, 0.5);

  canmpc_msgs::msg::HumanStates msg;
  canmpc_msgs::msg::HumanState human;
  human.pose.position.x = 1.0;
  human.pose.position.y = 2.0;
  msg.humans.push_back(human);
  layer.setHumans(msg);

  // Disable layer
  layer.setEnabled(false);

  // Bounds should not change
  double min_x = 0.0, min_y = 0.0, max_x = 0.0, max_y = 0.0;
  layer.updateBounds(0.0, 0.0, 0.0, &min_x, &min_y, &max_x, &max_y);
  EXPECT_DOUBLE_EQ(min_x, 0.0);
  EXPECT_DOUBLE_EQ(max_x, 0.0);

  // Costs should not change
  nav2_costmap_2d::Costmap2D master_grid(40, 40, 0.1, -1.0, -1.0, 0);
  layer.updateCosts(master_grid, 0, 0, 40, 40);
  
  unsigned int mx, my;
  ASSERT_TRUE(master_grid.worldToMap(1.0, 2.0, mx, my));
  EXPECT_EQ(master_grid.getCost(mx, my), 0); // remains 0 (free/default)
}

TEST(HumanSemanticLayerTest, ResetAndClear)
{
  HumanSemanticLayer layer;
  layer.configureLayer(1.0, 0.5);

  canmpc_msgs::msg::HumanStates msg;
  canmpc_msgs::msg::HumanState human;
  human.pose.position.x = 1.0;
  human.pose.position.y = 2.0;
  msg.humans.push_back(human);
  layer.setHumans(msg);

  // Call reset
  layer.reset();

  // Bounds should not be updated since humans are cleared
  double min_x = 0.0, min_y = 0.0, max_x = 0.0, max_y = 0.0;
  layer.updateBounds(0.0, 0.0, 0.0, &min_x, &min_y, &max_x, &max_y);
  EXPECT_DOUBLE_EQ(min_x, 0.0);
  EXPECT_DOUBLE_EQ(max_x, 0.0);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
