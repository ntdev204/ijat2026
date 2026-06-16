#include <gtest/gtest.h>
#include "cca_nmpc_controller/costmap_evaluator.hpp"
#include "nav2_costmap_2d/costmap_2d.hpp"
#include <cmath>
#include <limits>

using cca_nmpc_controller::CostmapEvaluator;

TEST(CostmapEvaluatorTest, EvaluateCostAtPose)
{
  // Create a 2D costmap: width=10, height=10, resolution=0.1, origin=(-0.5, -0.5)
  nav2_costmap_2d::Costmap2D costmap(10, 10, 0.1, -0.5, -0.5, 0);

  // Set up specific costs in the costmap
  // Set origin cell (0, 0) in world coords -> map coords (5, 5) to FREE_SPACE (0)
  unsigned int mx, my;
  ASSERT_TRUE(costmap.worldToMap(0.0, 0.0, mx, my));
  EXPECT_EQ(mx, 5U);
  EXPECT_EQ(my, 5U);
  costmap.setCost(mx, my, nav2_costmap_2d::FREE_SPACE);

  // Set (0.1, 0.0) -> map coords (6, 5) to an inflated cost (e.g. 127)
  ASSERT_TRUE(costmap.worldToMap(0.1, 0.0, mx, my));
  costmap.setCost(mx, my, 127);

  // Set (0.2, 0.0) -> map coords (7, 5) to LETHAL_OBSTACLE (254)
  ASSERT_TRUE(costmap.worldToMap(0.2, 0.0, mx, my));
  costmap.setCost(mx, my, nav2_costmap_2d::LETHAL_OBSTACLE);

  // Set (0.3, 0.0) -> map coords (8, 5) to NO_INFORMATION (255)
  ASSERT_TRUE(costmap.worldToMap(0.3, 0.0, mx, my));
  costmap.setCost(mx, my, nav2_costmap_2d::NO_INFORMATION);

  // Instantiate CostmapEvaluator and set costmap
  CostmapEvaluator evaluator;
  
  // 1. Test before setting costmap (should return 1.0)
  EXPECT_DOUBLE_EQ(evaluator.evaluateCostAtPose(0.0, 0.0), 1.0);

  evaluator.setCostmap(&costmap);

  // 2. Test FREE_SPACE
  EXPECT_DOUBLE_EQ(evaluator.evaluateCostAtPose(0.0, 0.0), 0.0);

  // 3. Test Inflated Cost (127 / 254.0)^2 = 0.5^2 = 0.25
  EXPECT_NEAR(evaluator.evaluateCostAtPose(0.1, 0.0), 0.25, 1e-6);

  // 4. Test LETHAL_OBSTACLE
  EXPECT_DOUBLE_EQ(evaluator.evaluateCostAtPose(0.2, 0.0), 1.0);

  // 5. Test NO_INFORMATION
  EXPECT_DOUBLE_EQ(evaluator.evaluateCostAtPose(0.3, 0.0), 1.0);

  // 6. Test Outside Map (far away)
  EXPECT_DOUBLE_EQ(evaluator.evaluateCostAtPose(10.0, 10.0), 1.0);
  EXPECT_DOUBLE_EQ(evaluator.evaluateCostAtPose(-10.0, -10.0), 1.0);

  // 7. Test edge cases with NaN input
  double nan_val = std::numeric_limits<double>::quiet_NaN();
  EXPECT_DOUBLE_EQ(evaluator.evaluateCostAtPose(nan_val, 0.0), 1.0);
  EXPECT_DOUBLE_EQ(evaluator.evaluateCostAtPose(0.0, nan_val), 1.0);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
