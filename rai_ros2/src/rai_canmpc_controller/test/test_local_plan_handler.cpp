#include <gtest/gtest.h>
#include "rai_canmpc_controller/local_plan_handler.hpp"
#include <tf2_ros/buffer.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>
#include <nav2_costmap_2d/costmap_2d.hpp>
#include <rclcpp/rclcpp.hpp>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

using rai_canmpc_controller::LocalPlanHandler;

TEST(LocalPlanHandlerTest, SetAndPrunePlan)
{
  LocalPlanHandler handler;

  // Define global path: (0,0), (1,0), (2,0), (3,0), (4,0)
  nav_msgs::msg::Path path;
  path.header.frame_id = "map";
  for (int i = 0; i < 5; ++i) {
    geometry_msgs::msg::PoseStamped pose;
    pose.header.frame_id = "map";
    pose.pose.position.x = static_cast<double>(i);
    pose.pose.position.y = 0.0;
    pose.pose.orientation.w = 1.0;
    path.poses.push_back(pose);
  }

  handler.setGlobalPlan(path);
  EXPECT_EQ(handler.getGlobalPlan().poses.size(), 5U);

  // 1. Robot is close to (2.0, 0.0) -> closest point should be (2.0, 0.0) at index 2
  geometry_msgs::msg::PoseStamped robot_pose;
  robot_pose.header.frame_id = "map";
  robot_pose.pose.position.x = 1.9;
  robot_pose.pose.position.y = 0.1;
  robot_pose.pose.orientation.w = 1.0;

  size_t remaining = handler.prunePlan(robot_pose);
  EXPECT_EQ(remaining, 3U); // Index 0 and 1 are removed, indexes 2, 3, 4 remain (size = 3)
  EXPECT_NEAR(handler.getGlobalPlan().poses[0].pose.position.x, 2.0, 1e-6);

  // 2. Robot is far past the end of the path -> closest point should be last point (4.0, 0.0) at index 2 of current plan
  robot_pose.pose.position.x = 10.0;
  robot_pose.pose.position.y = 0.0;

  remaining = handler.prunePlan(robot_pose);
  EXPECT_EQ(remaining, 1U); // Only the last pose remains
  EXPECT_NEAR(handler.getGlobalPlan().poses[0].pose.position.x, 4.0, 1e-6);
}

TEST(LocalPlanHandlerTest, TransformAndCropPlan)
{
  LocalPlanHandler handler;
  
  // Set up TF buffer
  auto clock = std::make_shared<rclcpp::Clock>(RCL_SYSTEM_TIME);
  auto tf_buffer = std::make_shared<tf2_ros::Buffer>(clock);

  // Set up transform: map -> odom (translation x=1.0, y=2.0)
  geometry_msgs::msg::TransformStamped tf_msg;
  tf_msg.header.frame_id = "map";
  tf_msg.header.stamp = rclcpp::Time(0);
  tf_msg.child_frame_id = "odom";
  tf_msg.transform.translation.x = 1.0;
  tf_msg.transform.translation.y = 2.0;
  tf_msg.transform.translation.z = 0.0;
  tf_msg.transform.rotation.w = 1.0; // Identity
  tf_buffer->setTransform(tf_msg, "test_authority", true);

  // Create global path in "map" frame:
  // Points: (0,0), (1,0), (2,0), (3,0), (4,0)
  // Under transform, in "odom" frame they will map to:
  // (0-1, 0-2) = (-1, -2)
  // (1-1, 0-2) = (0, -2)
  // (2-1, 0-2) = (1, -2)
  // (3-1, 0-2) = (2, -2)
  // (4-1, 0-2) = (3, -2)
  nav_msgs::msg::Path path;
  path.header.frame_id = "map";
  path.header.stamp = rclcpp::Time(0);
  for (int i = 0; i < 5; ++i) {
    geometry_msgs::msg::PoseStamped pose;
    pose.header.frame_id = "map";
    pose.header.stamp = rclcpp::Time(0);
    pose.pose.position.x = static_cast<double>(i);
    pose.pose.position.y = 0.0;
    pose.pose.orientation.w = 1.0;
    path.poses.push_back(pose);
  }
  handler.setGlobalPlan(path);

  // Robot pose in "map" frame (close to first point)
  geometry_msgs::msg::PoseStamped robot_pose;
  robot_pose.header.frame_id = "map";
  robot_pose.header.stamp = rclcpp::Time(0);
  robot_pose.pose.position.x = 0.0;
  robot_pose.pose.position.y = 0.0;
  robot_pose.pose.orientation.w = 1.0;

  // Create a 2D costmap in "odom" frame
  // Cover bounds: x in [-2.0, 2.0], y in [-3.0, 3.0]
  // Size = 40 cells x 60 cells, resolution = 0.1, origin = (-2.0, -3.0)
  nav2_costmap_2d::Costmap2D costmap(40, 60, 0.1, -2.0, -3.0, 0);

  // 1. Test cropping based on costmap bounds:
  // Poses transformed:
  // (-1, -2) -> inside
  // (0, -2) -> inside
  // (1, -2) -> inside
  // (2, -2) -> inside (boundary cell 40 -> false for worldToMap)
  // (3, -2) -> outside
  // Thus, cropped path should stop before the out-of-bounds pose.
  nav_msgs::msg::Path cropped = handler.transformAndCropPlan(
    robot_pose, tf_buffer, "odom", costmap, 10.0);

  // Expected remaining points in "odom": (-1,-2), (0,-2), (1,-2)
  EXPECT_EQ(cropped.header.frame_id, "odom");
  ASSERT_EQ(cropped.poses.size(), 3U);
  EXPECT_NEAR(cropped.poses[0].pose.position.x, -1.0, 1e-6);
  EXPECT_NEAR(cropped.poses[1].pose.position.x, 0.0, 1e-6);
  EXPECT_NEAR(cropped.poses[2].pose.position.x, 1.0, 1e-6);

  // 2. Test cropping based on max_path_length:
  // Cumulative distance:
  // Pose 0 -> Pose 1 = 1.0m
  // Pose 1 -> Pose 2 = 1.0m (cumulative = 2.0m)
  // Set max_path_length = 1.5m -> should break before Pose 2.
  // Expected remaining points in "odom": (-1,-2), (0,-2)
  handler.setGlobalPlan(path); // Reset pruned plan
  nav_msgs::msg::Path length_cropped = handler.transformAndCropPlan(
    robot_pose, tf_buffer, "odom", costmap, 1.5);

  ASSERT_EQ(length_cropped.poses.size(), 2U);
  EXPECT_NEAR(length_cropped.poses[0].pose.position.x, -1.0, 1e-6);
  EXPECT_NEAR(length_cropped.poses[1].pose.position.x, 0.0, 1e-6);
}

TEST(LocalPlanHandlerTest, ResamplePath)
{
  nav_msgs::msg::Path path;
  path.header.frame_id = "map";

  // Create path: (0,0) -> (4,0) (5 points spaced by 1.0m)
  for (int i = 0; i <= 4; ++i) {
    geometry_msgs::msg::PoseStamped pose;
    pose.header.frame_id = "map";
    pose.pose.position.x = static_cast<double>(i);
    pose.pose.position.y = 0.0;
    pose.pose.orientation.w = 1.0;
    path.poses.push_back(pose);
  }

  geometry_msgs::msg::PoseStamped robot_pose;
  robot_pose.header.frame_id = "map";
  robot_pose.pose.position.x = 0.0;
  robot_pose.pose.position.y = 0.0;
  robot_pose.pose.orientation.w = 1.0;

  // 1. Test Upsampling:
  // Resample with N = 8 steps, dt = 0.5s, v_ref = 1.0 m/s -> ds = 0.5m
  // Target coordinates should be: 0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0
  auto upsampled = LocalPlanHandler::resamplePath(path, robot_pose, 8, 0.5, 1.0);
  EXPECT_EQ(upsampled.size(), 9U);
  for (size_t i = 0; i < upsampled.size(); ++i) {
    EXPECT_NEAR(upsampled[i].pose.position.x, static_cast<double>(i) * 0.5, 1e-6);
    EXPECT_NEAR(upsampled[i].pose.position.y, 0.0, 1e-6);
  }

  // 2. Test Downsampling:
  // Resample with N = 2 steps, dt = 1.0s, v_ref = 2.0 m/s -> ds = 2.0m
  // Target coordinates should be: 0.0, 2.0, 4.0
  auto downsampled = LocalPlanHandler::resamplePath(path, robot_pose, 2, 1.0, 2.0);
  EXPECT_EQ(downsampled.size(), 3U);
  EXPECT_NEAR(downsampled[0].pose.position.x, 0.0, 1e-6);
  EXPECT_NEAR(downsampled[1].pose.position.x, 2.0, 1e-6);
  EXPECT_NEAR(downsampled[2].pose.position.x, 4.0, 1e-6);

  // 3. Test Yaw Angle Interpolation (shortest angular path):
  // Let's create a path with changing orientation.
  // Point 0: yaw = 0.0 -> q = (0, 0, 0, 1)
  // Point 1: yaw = pi/2 -> q = (0, 0, sin(pi/4), cos(pi/4)) = (0, 0, 0.7071, 0.7071)
  nav_msgs::msg::Path orient_path;
  orient_path.header.frame_id = "map";

  geometry_msgs::msg::PoseStamped p0;
  p0.pose.position.x = 0.0;
  p0.pose.position.y = 0.0;
  p0.pose.orientation.x = 0.0;
  p0.pose.orientation.y = 0.0;
  p0.pose.orientation.z = 0.0;
  p0.pose.orientation.w = 1.0;
  orient_path.poses.push_back(p0);

  geometry_msgs::msg::PoseStamped p1;
  p1.pose.position.x = 2.0;
  p1.pose.position.y = 0.0;
  p1.pose.orientation.x = 0.0;
  p1.pose.orientation.y = 0.0;
  p1.pose.orientation.z = std::sin(M_PI / 4.0);
  p1.pose.orientation.w = std::cos(M_PI / 4.0);
  orient_path.poses.push_back(p1);

  // Resample halfway: N = 2, dt = 0.5s, v_ref = 2.0 m/s -> ds = 1.0m (halfway)
  auto interp = LocalPlanHandler::resamplePath(orient_path, robot_pose, 2, 0.5, 2.0);
  ASSERT_EQ(interp.size(), 3U);

  // Halfway pose at index 1: position should be (1.0, 0.0), yaw should be M_PI / 4.0
  EXPECT_NEAR(interp[1].pose.position.x, 1.0, 1e-6);
  EXPECT_NEAR(interp[1].pose.position.y, 0.0, 1e-6);
  EXPECT_NEAR(interp[1].pose.orientation.z, std::sin(M_PI / 8.0), 1e-4);
  EXPECT_NEAR(interp[1].pose.orientation.w, std::cos(M_PI / 8.0), 1e-4);
}

TEST(LocalPlanHandlerTest, ComputePathHeadings)
{
  nav_msgs::msg::Path path;
  path.header.frame_id = "map";

  geometry_msgs::msg::PoseStamped p1;
  p1.pose.position.x = 0.0;
  p1.pose.position.y = 0.0;
  path.poses.push_back(p1);

  geometry_msgs::msg::PoseStamped p2;
  p2.pose.position.x = 1.0;
  p2.pose.position.y = 1.0; // Heading should be pi/4 (45 degrees)
  path.poses.push_back(p2);

  geometry_msgs::msg::PoseStamped p3;
  p3.pose.position.x = 1.0;
  p3.pose.position.y = 2.0; // Heading should be pi/2 (90 degrees)
  path.poses.push_back(p3);

  // Yaw is automatically generated when not provided
  LocalPlanHandler::computePathHeadings(path);

  // Check first point orientation (yaw = pi/4)
  double sin_pi_8 = std::sin(M_PI_2 / 4.0);
  double cos_pi_8 = std::cos(M_PI_2 / 4.0);
  EXPECT_NEAR(path.poses[0].pose.orientation.z, sin_pi_8, 1e-4);
  EXPECT_NEAR(path.poses[0].pose.orientation.w, cos_pi_8, 1e-4);

  // Check second point orientation (yaw = pi/2)
  double sin_pi_4 = std::sin(M_PI_2 / 2.0);
  double cos_pi_4 = std::cos(M_PI_2 / 2.0);
  EXPECT_NEAR(path.poses[1].pose.orientation.z, sin_pi_4, 1e-4);
  EXPECT_NEAR(path.poses[1].pose.orientation.w, cos_pi_4, 1e-4);

  // Check last point copies previous orientation
  EXPECT_NEAR(path.poses[2].pose.orientation.z, sin_pi_4, 1e-4);
  EXPECT_NEAR(path.poses[2].pose.orientation.w, cos_pi_4, 1e-4);
}

TEST(LocalPlanHandlerTest, EdgeCases)
{
  LocalPlanHandler handler;
  geometry_msgs::msg::PoseStamped robot_pose;
  robot_pose.header.frame_id = "map";
  robot_pose.pose.position.x = 0.0;
  robot_pose.pose.position.y = 0.0;
  robot_pose.pose.orientation.w = 1.0;

  // 1. Empty path in prunePlan
  nav_msgs::msg::Path empty_path;
  handler.setGlobalPlan(empty_path);
  EXPECT_EQ(handler.prunePlan(robot_pose), 0U);

  // 2. Empty path in transformAndCropPlan
  auto clock = std::make_shared<rclcpp::Clock>(RCL_SYSTEM_TIME);
  auto tf_buffer = std::make_shared<tf2_ros::Buffer>(clock);
  nav2_costmap_2d::Costmap2D costmap(10, 10, 0.1, 0.0, 0.0, 0);

  nav_msgs::msg::Path crop_res = handler.transformAndCropPlan(
    robot_pose, tf_buffer, "odom", costmap, 10.0);
  EXPECT_TRUE(crop_res.poses.empty());

  // 3. Empty path in resamplePath -> returns path of robot_pose
  auto resample_res = LocalPlanHandler::resamplePath(empty_path, robot_pose, 5, 0.1, 1.0);
  EXPECT_EQ(resample_res.size(), 6U);
  for (const auto & pose : resample_res) {
    EXPECT_DOUBLE_EQ(pose.pose.position.x, robot_pose.pose.position.x);
    EXPECT_DOUBLE_EQ(pose.pose.position.y, robot_pose.pose.position.y);
  }

  // 4. Single pose path in computePathHeadings -> should return immediately without crash
  nav_msgs::msg::Path single_pose_path;
  single_pose_path.poses.push_back(robot_pose);
  EXPECT_NO_THROW(LocalPlanHandler::computePathHeadings(single_pose_path));

  // 5. NaN coordinate handling in prunePlan
  nav_msgs::msg::Path nan_path;
  geometry_msgs::msg::PoseStamped nan_pose;
  nan_pose.pose.position.x = std::numeric_limits<double>::quiet_NaN();
  nan_pose.pose.position.y = 0.0;
  nan_path.poses.push_back(nan_pose);
  handler.setGlobalPlan(nan_path);

  // Should not crash and return size
  EXPECT_EQ(handler.prunePlan(robot_pose), 1U);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  rclcpp::init(argc, argv);
  int result = RUN_ALL_TESTS();
  rclcpp::shutdown();
  return result;
}
