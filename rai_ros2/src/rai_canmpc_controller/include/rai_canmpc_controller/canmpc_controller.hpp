#ifndef NAV2_CANMPC_CONTROLLER__CANMPC_CONTROLLER_HPP_
#define NAV2_CANMPC_CONTROLLER__CANMPC_CONTROLLER_HPP_

#include <string>
#include <memory>
#include <vector>
#include <mutex>

#include "rclcpp/rclcpp.hpp"
#include "nav2_core/controller.hpp"
#include "nav2_costmap_2d/costmap_2d_ros.hpp"
#include "nav2_costmap_2d/footprint_collision_checker.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "nav_msgs/msg/path.hpp"
#include "tf2_ros/buffer.h"

// Custom message definitions
#include "canmpc_msgs/msg/human_states.hpp"
#include "canmpc_msgs/msg/context.hpp"
#include "canmpc_msgs/msg/solver_stats.hpp"
#include "canmpc_msgs/msg/adaptive_bounds.hpp"

// Project headers
#include "rai_canmpc_controller/local_plan_handler.hpp"
#include "rai_canmpc_controller/mecanum_model.hpp"
#include "rai_canmpc_controller/solver_wrapper.hpp"

namespace rai_canmpc_controller
{

/**
 * @class CANMPCController
 * @brief Continuous Context-Adaptive Nonlinear Model Predictive Control plugin for Nav2.
 */
class CANMPCController : public nav2_core::Controller
{
public:
  CANMPCController() = default;
  ~CANMPCController() override = default;

  // Lifecycle methods
  void configure(
    const rclcpp_lifecycle::LifecycleNode::WeakPtr & parent,
    std::string name,
    std::shared_ptr<tf2_ros::Buffer> tf,
    std::shared_ptr<nav2_costmap_2d::Costmap2DROS> costmap_ros) override;

  void cleanup() override;
  void activate() override;
  void deactivate() override;

  // Plan management
  void setPlan(const nav_msgs::msg::Path & path) override;

  // Velocity computation
  geometry_msgs::msg::TwistStamped computeVelocityCommands(
    const geometry_msgs::msg::PoseStamped & pose,
    const geometry_msgs::msg::Twist & velocity,
    nav2_core::GoalChecker * goal_checker) override;

  // Speed limit callback
  void setSpeedLimit(const double & speed_limit, const bool & percentage) override;

protected:
  // ROS 2 parameters
  int horizon_steps_{20};
  double model_dt_{0.05};
  double max_solver_time_ms_{50.0};
  double max_path_length_{3.0};
  double default_v_ref_{0.5};

  // NMPC solver hyperparameters
  double beta_{3.0};
  double d0_{1.5};
  double d_safe_0_{0.5};
  double d_safe_max_{1.2};
  double v_max_min_{0.1};
  double v_y_max_min_{0.1};
  double omega_max_min_{0.2};

  // Base MPC weights
  double q_x_{10.0};
  double q_y_{10.0};
  double q_theta_{5.0};
  double r_vx_{0.1};
  double r_vy_{0.1};
  double r_omega_{0.05};
  double rd_vx_{1.0};
  double rd_vy_{1.0};
  double rd_omega_{0.5};
  double q_active_factor_{0.1};
  double w_slack_{100000.0};

  // Node handles and structures
  rclcpp_lifecycle::LifecycleNode::WeakPtr parent_node_;
  std::string name_;
  std::shared_ptr<tf2_ros::Buffer> tf_;
  std::shared_ptr<nav2_costmap_2d::Costmap2DROS> costmap_ros_;
  rclcpp::Logger logger_{rclcpp::get_logger("CANMPCController")};

  // Local Path Handler
  LocalPlanHandler plan_handler_;

  // Solver Wrapper
  std::unique_ptr<SolverWrapper> solver_wrapper_;

  // Collision Checker
  std::unique_ptr<nav2_costmap_2d::FootprintCollisionChecker<nav2_costmap_2d::Costmap2D *>> collision_checker_;

  // Speed limits from Nav2 server
  double external_speed_limit_vx_{0.0};
  double external_speed_limit_vy_{0.0};
  double external_speed_limit_omega_{0.0};
  bool speed_limit_percentage_{false};

  // Trajectory tracking structures
  nav_msgs::msg::Path current_global_plan_;
  geometry_msgs::msg::Twist last_cmd_vel_;

  // Dynamic subscribers
  rclcpp::Subscription<canmpc_msgs::msg::HumanStates>::SharedPtr human_states_sub_;
  rclcpp::Subscription<canmpc_msgs::msg::Context>::SharedPtr context_sub_;

  // Debug publishers
  rclcpp_lifecycle::LifecyclePublisher<nav_msgs::msg::Path>::SharedPtr local_reference_path_pub_;
  rclcpp_lifecycle::LifecyclePublisher<nav_msgs::msg::Path>::SharedPtr predicted_trajectory_pub_;
  rclcpp_lifecycle::LifecyclePublisher<canmpc_msgs::msg::SolverStats>::SharedPtr solver_stats_pub_;
  rclcpp_lifecycle::LifecyclePublisher<canmpc_msgs::msg::AdaptiveBounds>::SharedPtr adaptive_bounds_pub_;

  // Callback storage
  canmpc_msgs::msg::HumanStates latest_human_states_;
  canmpc_msgs::msg::Context latest_context_;
  std::mutex data_mutex_;

  // Callbacks
  void humanStatesCallback(const canmpc_msgs::msg::HumanStates::SharedPtr msg);
  void contextCallback(const canmpc_msgs::msg::Context::SharedPtr msg);

  // Helper method to resolve dynamic speed limits
  void getSpeedLimits(
    double context_vx, double context_vy, double context_omega,
    double & max_vx, double & max_vy, double & max_omega);
};

}  // namespace rai_canmpc_controller

#endif  // NAV2_CANMPC_CONTROLLER__CANMPC_CONTROLLER_HPP_
