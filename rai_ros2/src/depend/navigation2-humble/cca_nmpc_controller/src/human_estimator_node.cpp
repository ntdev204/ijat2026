#include "cca_nmpc_controller/human_estimator_node.hpp"

#include <chrono>
#include <limits>
#include <vector>

using namespace std::chrono_literals;

namespace cca_nmpc_controller
{

HumanEstimatorNode::HumanEstimatorNode(const rclcpp::NodeOptions & options)
: Node("human_estimator_node", options),
  next_track_id_(1),
  is_first_predict_(true)
{
  // Declare parameters
  target_frame_ = this->declare_parameter<std::string>("target_frame", "map");
  detection_topic_ = this->declare_parameter<std::string>("detection_topic", "/detector/human_poses");
  max_missed_frames_ = this->declare_parameter<int>("max_missed_frames", 10);
  gating_threshold_ = this->declare_parameter<double>("gating_threshold", 1.5);
  q_pos_noise_ = this->declare_parameter<double>("q_pos_noise", 0.05);
  q_vel_noise_ = this->declare_parameter<double>("q_vel_noise", 0.2);
  r_pos_noise_ = this->declare_parameter<double>("r_pos_noise", 0.1);

  // Set up TF2 buffer and listener
  tf_buffer_ = std::make_shared<tf2_ros::Buffer>(this->get_clock());
  tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

  // Create Publisher
  human_states_pub_ = this->create_publisher<ccanmpc_msgs::msg::HumanStates>(
    "/canmpc/humans", 10);

  // Create Subscriber
  detection_sub_ = this->create_subscription<geometry_msgs::msg::PoseArray>(
    detection_topic_, 10,
    std::bind(&HumanEstimatorNode::detectionCallback, this, std::placeholders::_1));

  RCLCPP_INFO(this->get_logger(), "Human Estimator Node initialized.");
  RCLCPP_INFO(this->get_logger(), "Subscribing to: %s, Publishing to: /canmpc/humans in frame: %s",
    detection_topic_.c_str(), target_frame_.c_str());
}

void HumanEstimatorNode::detectionCallback(const geometry_msgs::msg::PoseArray::SharedPtr msg)
{
  rclcpp::Time stamp = msg->header.stamp;
  
  // Initialize prediction clock on first message
  if (is_first_predict_) {
    last_predict_time_ = stamp;
    is_first_predict_ = false;
  }

  // Calculate dt
  double dt = (stamp - last_predict_time_).seconds();
  if (dt > 1e-4) {
    predictTracks(dt);
    last_predict_time_ = stamp;
  }

  // Transform detections to target frame
  std::vector<Eigen::Vector2d> transformed_detections;
  
  for (const auto & pose : msg->poses) {
    geometry_msgs::msg::PoseStamped pose_in;
    pose_in.header = msg->header;
    pose_in.pose = pose;

    geometry_msgs::msg::PoseStamped pose_out;
    try {
      // Look up transform and transform pose
      pose_out = tf_buffer_->transform(pose_in, target_frame_);
      transformed_detections.push_back(
        Eigen::Vector2d(pose_out.pose.position.x, pose_out.pose.position.y));
    } catch (const tf2::TransformException & ex) {
      RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000,
        "TF transform failed from %s to %s: %s",
        msg->header.frame_id.c_str(), target_frame_.c_str(), ex.what());
      continue;
    }
  }

  // Update tracking filters
  updateTracks(transformed_detections, stamp);

  // Publish tracks
  publishTracks(stamp);
}

void HumanEstimatorNode::predictTracks(double dt)
{
  // Transition Matrix A (Constant Velocity model)
  Eigen::Matrix4d A = Eigen::Matrix4d::Identity();
  A(0, 2) = dt;
  A(1, 3) = dt;

  // Process Noise Covariance Matrix Q
  Eigen::Matrix4d Q = Eigen::Matrix4d::Zero();
  double q_pos_var = q_pos_noise_ * q_pos_noise_;
  double q_vel_var = q_vel_noise_ * q_vel_noise_;
  Q(0, 0) = q_pos_var * dt;
  Q(1, 1) = q_pos_var * dt;
  Q(2, 2) = q_vel_var * dt;
  Q(3, 3) = q_vel_var * dt;

  for (auto & track : tracks_) {
    track.x = A * track.x;
    track.P = A * track.P * A.transpose() + Q;
  }
}

void HumanEstimatorNode::updateTracks(const std::vector<Eigen::Vector2d> & detections, const rclcpp::Time & stamp)
{
  std::vector<bool> track_matched(tracks_.size(), false);
  std::vector<bool> detection_matched(detections.size(), false);

  // Measurement matrix H (only position is measured)
  Eigen::Matrix<double, 2, 4> H;
  H << 1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0;

  // Measurement noise R
  Eigen::Matrix2d R = Eigen::Matrix2d::Identity() * (r_pos_noise_ * r_pos_noise_);

  // Greedy association
  for (size_t iter = 0; iter < std::min(tracks_.size(), detections.size()); ++iter) {
    double min_dist = gating_threshold_;
    int best_track_idx = -1;
    int best_det_idx = -1;

    for (size_t i = 0; i < tracks_.size(); ++i) {
      if (track_matched[i]) continue;
      for (size_t j = 0; j < detections.size(); ++j) {
        if (detection_matched[j]) continue;

        double dist = (tracks_[i].x.head<2>() - detections[j]).norm();
        if (dist < min_dist) {
          min_dist = dist;
          best_track_idx = i;
          best_det_idx = j;
        }
      }
    }

    // No close matches remaining under the gate threshold
    if (best_track_idx == -1) {
      break;
    }

    track_matched[best_track_idx] = true;
    detection_matched[best_det_idx] = true;

    // Kalman Filter Update
    auto & track = tracks_[best_track_idx];
    Eigen::Vector2d z = detections[best_det_idx];

    Eigen::Vector2d y = z - H * track.x;                         // Innovation
    Eigen::Matrix2d S = H * track.P * H.transpose() + R;        // Innovation Covariance
    Eigen::Matrix<double, 4, 2> K = track.P * H.transpose() * S.inverse(); // Kalman Gain

    track.x = track.x + K * y;
    track.P = (Eigen::Matrix4d::Identity() - K * H) * track.P;
    track.missed_frames = 0;
    track.last_update_time = stamp;
  }

  // Handle unmatched tracks
  for (size_t i = 0; i < tracks_.size(); ++i) {
    if (!track_matched[i]) {
      tracks_[i].missed_frames++;
    }
  }

  // Handle unmatched detections (Create new tracks)
  for (size_t j = 0; j < detections.size(); ++j) {
    if (!detection_matched[j]) {
      Track new_track;
      new_track.id = next_track_id_++;
      new_track.x << detections[j][0], detections[j][1], 0.0, 0.0; // Initial velocity = 0
      
      // Initialize covariance
      new_track.P = Eigen::Matrix4d::Zero();
      new_track.P(0, 0) = r_pos_noise_ * r_pos_noise_;
      new_track.P(1, 1) = r_pos_noise_ * r_pos_noise_;
      new_track.P(2, 2) = 1.0; // High initial velocity uncertainty
      new_track.P(3, 3) = 1.0;

      new_track.missed_frames = 0;
      new_track.last_update_time = stamp;

      tracks_.push_back(new_track);
      
      RCLCPP_DEBUG(this->get_logger(), "Created new track ID: %d at (%.2f, %.2f)",
        new_track.id, detections[j][0], detections[j][1]);
    }
  }

  // Prune dead tracks
  auto it = tracks_.begin();
  while (it != tracks_.end()) {
    if (it->missed_frames > max_missed_frames_) {
      RCLCPP_DEBUG(this->get_logger(), "Removing track ID: %d", it->id);
      it = tracks_.erase(it);
    } else {
      ++it;
    }
  }
}

void HumanEstimatorNode::publishTracks(const rclcpp::Time & stamp)
{
  ccanmpc_msgs::msg::HumanStates msg;
  msg.header.stamp = stamp;
  msg.header.frame_id = target_frame_;

  for (const auto & track : tracks_) {
    // Only publish active tracks that have been updated recently
    if (track.missed_frames > 2) {
      continue; // Skip publishing briefly lost tracks to avoid flicker
    }

    ccanmpc_msgs::msg::HumanState state;
    state.id = track.id;
    state.pose.position.x = track.x[0];
    state.pose.position.y = track.x[1];
    state.pose.position.z = 0.0;
    
    // Set orientation based on velocity heading
    double speed = track.x.tail<2>().norm();
    if (speed > 0.1) {
      double yaw = std::atan2(track.x[3], track.x[2]);
      tf2::Quaternion q;
      q.setRPY(0.0, 0.0, yaw);
      state.pose.orientation = tf2::toMsg(q);
    } else {
      state.pose.orientation.w = 1.0;
    }

    state.velocity.linear.x = track.x[2];
    state.velocity.linear.y = track.x[3];
    
    // Calculate confidence based on track history
    state.confidence = 1.0 - static_cast<double>(track.missed_frames) / max_missed_frames_;
    state.confidence = std::max(0.0, std::min(1.0, state.confidence));

    msg.humans.push_back(state);
  }

  human_states_pub_->publish(msg);
}

}  // namespace cca_nmpc_controller

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<cca_nmpc_controller::HumanEstimatorNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
