#ifndef CCA_NMPC_CONTROLLER__HUMAN_ESTIMATOR_NODE_HPP_
#define CCA_NMPC_CONTROLLER__HUMAN_ESTIMATOR_NODE_HPP_

#include <memory>
#include <string>
#include <vector>
#include <Eigen/Dense>

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/pose_array.hpp"
#include "ccanmpc_msgs/msg/human_states.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"

namespace cca_nmpc_controller
{

struct Track
{
  int id;
  Eigen::Vector4d x;  // State: [px, py, vx, vy]^T
  Eigen::Matrix4d P;  // Covariance
  int missed_frames;
  rclcpp::Time last_update_time;
};

class HumanEstimatorNode : public rclcpp::Node
{
public:
  explicit HumanEstimatorNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
  virtual ~HumanEstimatorNode() = default;

private:
  void detectionCallback(const geometry_msgs::msg::PoseArray::SharedPtr msg);
  void predictTracks(double dt);
  void updateTracks(const std::vector<Eigen::Vector2d> & detections, const rclcpp::Time & stamp);
  void publishTracks(const rclcpp::Time & stamp);

  // ROS 2 Communication
  rclcpp::Subscription<geometry_msgs::msg::PoseArray>::SharedPtr detection_sub_;
  rclcpp::Publisher<ccanmpc_msgs::msg::HumanStates>::SharedPtr human_states_pub_;

  // TF2 Buffer & Listener
  std::shared_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> tf_listener_;

  // Tracking Parameters
  std::string target_frame_;
  std::string detection_topic_;
  int max_missed_frames_;
  double gating_threshold_;
  double q_pos_noise_;
  double q_vel_noise_;
  double r_pos_noise_;

  // Tracking state
  std::vector<Track> tracks_;
  int next_track_id_;
  rclcpp::Time last_predict_time_;
  bool is_first_predict_;
};

}  // namespace cca_nmpc_controller

#endif  // CCA_NMPC_CONTROLLER__HUMAN_ESTIMATOR_NODE_HPP_
