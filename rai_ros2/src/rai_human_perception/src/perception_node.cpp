#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <numeric>
#include <string>
#include <vector>

#include <cv_bridge/cv_bridge.h>
#include <opencv2/imgproc.hpp>

#include "geometry_msgs/msg/point_stamped.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/camera_info.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "std_msgs/msg/float64.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"

#include "rai_controller/msg/human_state.hpp"
#include "rai_controller/msg/human_states.hpp"
#include "rai_human_perception/depth_association.hpp"
#include "rai_human_perception/human_state_estimator.hpp"
#include "rai_human_perception/kalman_tracker.hpp"
#include "rai_human_perception/msg/context_input.hpp"
#include "rai_human_perception/msg/human_array.hpp"
#include "rai_human_perception/msg/human_state.hpp"
#include "rai_human_perception/types.hpp"
#include "rai_human_perception/yolo_detector.hpp"

namespace rai_human_perception
{
namespace
{

rai_human_perception::msg::HumanState toPerceptionMsg(const HumanTrack & track)
{
  rai_human_perception::msg::HumanState msg;
  msg.id = track.id;
  msg.x = track.x;
  msg.y = track.y;
  msg.vx = track.vx;
  msg.vy = track.vy;
  msg.confidence = track.confidence;
  msg.age_sec = track.age_sec;
  msg.covariance_x = track.covariance_x;
  msg.covariance_y = track.covariance_y;
  msg.covariance_vx = track.covariance_vx;
  msg.covariance_vy = track.covariance_vy;
  return msg;
}

rai_controller::msg::HumanState toControllerMsg(const HumanTrack & track)
{
  rai_controller::msg::HumanState msg;
  msg.id = track.id;
  msg.x = track.x;
  msg.y = track.y;
  msg.vx = track.vx;
  msg.vy = track.vy;
  msg.confidence = track.confidence;
  msg.age_sec = track.age_sec;
  msg.covariance_x = track.covariance_x;
  msg.covariance_y = track.covariance_y;
  msg.covariance_vx = track.covariance_vx;
  msg.covariance_vy = track.covariance_vy;
  return msg;
}

}  

class PerceptionNode : public rclcpp::Node
{
public:
  PerceptionNode()
  : Node("rai_human_perception"),
    tf_buffer_(get_clock()),
    tf_listener_(tf_buffer_),
    detector_(loadDetectorOptions()),
    depth_association_(
      declare_parameter<double>("depth_center_ratio", 0.45),
      declare_parameter<double>("min_depth_m", 0.25),
      declare_parameter<double>("max_depth_m", 6.0)),
    tracker_(loadKalmanOptions()),
    target_frame_(declare_parameter<std::string>("target_frame", "map")),
    estimator_(target_frame_)
  {
    rgb_topic_ = declare_parameter<std::string>("rgb_topic", "/camera/color/image_raw");
    depth_topic_ = declare_parameter<std::string>("depth_topic", "/camera/depth/image_raw");
    camera_info_topic_ = declare_parameter<std::string>("camera_info_topic", "/camera/color/camera_info");
    odom_topic_ = declare_parameter<std::string>("odom_topic", "/odom");
    humans_topic_ = declare_parameter<std::string>("humans_topic", "/cca_nmpc/humans");
    controller_humans_topic_ = declare_parameter<std::string>("controller_humans_topic", "/canmpc/humans");
    context_topic_ = declare_parameter<std::string>("context_input_topic", "/cca_nmpc/context_input");
    debug_image_topic_ = declare_parameter<std::string>("debug_image_topic", "/perception/debug/image");
    debug_depth_topic_ = declare_parameter<std::string>("debug_depth_topic", "/perception/debug/depth");
    debug_tracks_topic_ = declare_parameter<std::string>("debug_tracks_topic", "/perception/debug/tracks");
    debug_latency_topic_ = declare_parameter<std::string>("debug_latency_topic", "/perception/debug/latency");
    depth_scale_ = declare_parameter<double>("depth_scale", 0.001);
    tf_timeout_sec_ = declare_parameter<double>("tf_timeout_sec", 0.05);
    csv_path_ = declare_parameter<std::string>("csv_path", "/tmp/rai_human_perception.csv");
    enable_csv_logging_ = declare_parameter<bool>("enable_csv_logging", true);

    if (!detector_.initialize()) {
      RCLCPP_WARN(get_logger(), "YOLO detector did not initialize; node will publish empty tracks.");
    } else {
      RCLCPP_INFO(
        get_logger(),
        "Human detector initialized (%s)",
        detector_.usingTensorRt() ? "TensorRT engine" : "OpenCV DNN fallback");
    }

    rgb_sub_ = create_subscription<sensor_msgs::msg::Image>(
      rgb_topic_, rclcpp::SensorDataQoS(),
      [this](sensor_msgs::msg::Image::ConstSharedPtr msg) {rgbCallback(msg);});
    depth_sub_ = create_subscription<sensor_msgs::msg::Image>(
      depth_topic_, rclcpp::SensorDataQoS(),
      [this](sensor_msgs::msg::Image::ConstSharedPtr msg) {depthCallback(msg);});
    camera_info_sub_ = create_subscription<sensor_msgs::msg::CameraInfo>(
      camera_info_topic_, rclcpp::QoS(10),
      [this](sensor_msgs::msg::CameraInfo::ConstSharedPtr msg) {cameraInfoCallback(msg);});
    odom_sub_ = create_subscription<nav_msgs::msg::Odometry>(
      odom_topic_, rclcpp::QoS(20),
      [this](nav_msgs::msg::Odometry::ConstSharedPtr msg) {odomCallback(msg);});

    humans_pub_ = create_publisher<rai_human_perception::msg::HumanArray>(humans_topic_, 10);
    controller_humans_pub_ =
      create_publisher<rai_controller::msg::HumanStates>(controller_humans_topic_, 10);
    context_pub_ = create_publisher<rai_human_perception::msg::ContextInput>(context_topic_, 10);
    debug_image_pub_ = create_publisher<sensor_msgs::msg::Image>(debug_image_topic_, 10);
    debug_depth_pub_ = create_publisher<sensor_msgs::msg::Image>(debug_depth_topic_, 10);
    debug_tracks_pub_ = create_publisher<rai_human_perception::msg::HumanArray>(debug_tracks_topic_, 10);
    debug_latency_pub_ = create_publisher<std_msgs::msg::Float64>(debug_latency_topic_, 10);

    if (enable_csv_logging_) {
      const auto csv_parent = std::filesystem::path(csv_path_).parent_path();
      if (!csv_parent.empty()) {
        std::filesystem::create_directories(csv_parent);
      }
      csv_.open(csv_path_, std::ios::app);
      if (csv_.tellp() == 0) {
        csv_ << "timestamp,human_id,x_h,y_h,vx_h,vy_h,confidence,depth,latency_ms\n";
      }
    }
  }

private:
  YoloDetectorOptions loadDetectorOptions()
  {
    YoloDetectorOptions options;
    options.engine_path = declare_parameter<std::string>("engine_path", "best.engine");
    options.onnx_path = declare_parameter<std::string>("onnx_path", "best.onnx");
    options.weights_path = declare_parameter<std::string>("weights_path", "best.pt");
    options.export_script = declare_parameter<std::string>("export_script", "");
    options.image_size = declare_parameter<int>("imgsz", 640);
    options.confidence_threshold = declare_parameter<double>("confidence_threshold", 0.35);
    options.nms_threshold = declare_parameter<double>("nms_threshold", 0.45);
    options.fp16 = declare_parameter<bool>("fp16", true);
    options.auto_export_engine = declare_parameter<bool>("auto_export_engine", true);
    return options;
  }

  KalmanOptions loadKalmanOptions()
  {
    KalmanOptions options;
    options.process_noise_position = declare_parameter<double>("process_noise_position", 0.08);
    options.process_noise_velocity = declare_parameter<double>("process_noise_velocity", 0.8);
    options.measurement_noise_position = declare_parameter<double>("measurement_noise_position", 0.05);
    options.association_gate_m = declare_parameter<double>("association_gate_m", 0.8);
    options.max_track_age_sec = declare_parameter<double>("max_track_age_sec", 0.7);
    return options;
  }

  void rgbCallback(sensor_msgs::msg::Image::ConstSharedPtr msg)
  {
    sensor_msgs::msg::Image::ConstSharedPtr depth_msg;
    CameraIntrinsics intrinsics;
    {
      std::lock_guard<std::mutex> lock(data_mutex_);
      depth_msg = latest_depth_;
      intrinsics = intrinsics_;
    }
    if (!depth_msg || !intrinsics.valid) {
      return;
    }

    const auto start = std::chrono::steady_clock::now();
    cv_bridge::CvImageConstPtr rgb_cv;
    cv_bridge::CvImageConstPtr depth_cv;
    try {
      rgb_cv = cv_bridge::toCvShare(msg, "bgr8");
      depth_cv = cv_bridge::toCvShare(depth_msg);
    } catch (const cv_bridge::Exception & ex) {
      RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 2000, "cv_bridge failed: %s", ex.what());
      return;
    }

    auto detections = detector_.detect(rgb_cv->image);
    std::vector<HumanMeasurement> camera_measurements;
    camera_measurements.reserve(detections.size());
    for (const auto & detection : detections) {
      auto measurement = depth_association_.associate(
        detection,
        depth_cv->image,
        intrinsics,
        depth_msg->header,
        depth_scale_);
      if (measurement) {
        camera_measurements.push_back(*measurement);
      }
    }

    auto target_measurements = estimator_.transformMeasurements(
      camera_measurements,
      tf_buffer_,
      tf_timeout_sec_);
    auto tracks = tracker_.update(target_measurements, msg->header.stamp);

    const double latency_ms =
      std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
    publishOutputs(*msg, rgb_cv->image, depth_cv->image, detections, tracks, latency_ms);
  }

  void depthCallback(sensor_msgs::msg::Image::ConstSharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(data_mutex_);
    latest_depth_ = msg;
  }

  void cameraInfoCallback(sensor_msgs::msg::CameraInfo::ConstSharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(data_mutex_);
    intrinsics_.fx = msg->k[0];
    intrinsics_.fy = msg->k[4];
    intrinsics_.cx = msg->k[2];
    intrinsics_.cy = msg->k[5];
    intrinsics_.valid = intrinsics_.fx > 0.0 && intrinsics_.fy > 0.0;
  }

  void odomCallback(nav_msgs::msg::Odometry::ConstSharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(data_mutex_);
    robot_x_ = msg->pose.pose.position.x;
    robot_y_ = msg->pose.pose.position.y;
    robot_vx_ = msg->twist.twist.linear.x;
    robot_vy_ = msg->twist.twist.linear.y;
  }

  void publishOutputs(
    const sensor_msgs::msg::Image & rgb_msg,
    const cv::Mat & bgr_image,
    const cv::Mat & depth_image,
    const std::vector<Detection2D> & detections,
    const std::vector<HumanTrack> & tracks,
    double latency_ms)
  {
    rai_human_perception::msg::HumanArray humans_msg;
    humans_msg.header = rgb_msg.header;
    humans_msg.header.frame_id = target_frame_;

    rai_controller::msg::HumanStates controller_msg;
    controller_msg.header = humans_msg.header;

    for (const auto & track : tracks) {
      humans_msg.humans.push_back(toPerceptionMsg(track));
      controller_msg.humans.push_back(toControllerMsg(track));
      writeCsv(rgb_msg.header.stamp, track, latency_ms);
    }
    humans_pub_->publish(humans_msg);
    controller_humans_pub_->publish(controller_msg);
    debug_tracks_pub_->publish(humans_msg);

    rai_human_perception::msg::ContextInput context_msg;
    context_msg.header = humans_msg.header;
    context_msg.human_count = static_cast<int32_t>(tracks.size());
    context_msg.nearest_human_distance = nearestDistance(tracks);
    context_msg.relative_speed = nearestRelativeSpeed(tracks);
    context_msg.tracking_quality = trackingQuality(tracks);
    context_pub_->publish(context_msg);

    std_msgs::msg::Float64 latency_msg;
    latency_msg.data = latency_ms;
    debug_latency_pub_->publish(latency_msg);

    publishDebugImages(rgb_msg.header, bgr_image, depth_image, detections);
  }

  double nearestDistance(const std::vector<HumanTrack> & tracks) const
  {
    std::lock_guard<std::mutex> lock(data_mutex_);
    double nearest = 10.0;
    for (const auto & track : tracks) {
      nearest = std::min(nearest, std::hypot(track.x - robot_x_, track.y - robot_y_));
    }
    return nearest;
  }

  double nearestRelativeSpeed(const std::vector<HumanTrack> & tracks) const
  {
    if (tracks.empty()) {
      return 0.0;
    }
    std::lock_guard<std::mutex> lock(data_mutex_);
    const auto it = std::min_element(
      tracks.begin(), tracks.end(),
      [this](const auto & a, const auto & b) {
        return std::hypot(a.x - robot_x_, a.y - robot_y_) <
               std::hypot(b.x - robot_x_, b.y - robot_y_);
      });
    return std::hypot(it->vx - robot_vx_, it->vy - robot_vy_);
  }

  double trackingQuality(const std::vector<HumanTrack> & tracks) const
  {
    if (tracks.empty()) {
      return 0.0;
    }
    double sum = 0.0;
    for (const auto & track : tracks) {
      sum += track.confidence;
    }
    return sum / static_cast<double>(tracks.size());
  }

  void publishDebugImages(
    const std_msgs::msg::Header & header,
    const cv::Mat & bgr_image,
    const cv::Mat & depth_image,
    const std::vector<Detection2D> & detections)
  {
    cv::Mat debug = bgr_image.clone();
    for (const auto & detection : detections) {
      cv::rectangle(debug, detection.bbox, cv::Scalar(0, 255, 0), 2);
      cv::putText(
        debug,
        "person " + std::to_string(static_cast<int>(detection.confidence * 100.0)) + "%",
        detection.bbox.tl() + cv::Point(0, -4),
        cv::FONT_HERSHEY_SIMPLEX,
        0.5,
        cv::Scalar(0, 255, 0),
        1);
    }
    auto debug_msg = cv_bridge::CvImage(header, "bgr8", debug).toImageMsg();
    debug_image_pub_->publish(*debug_msg);

    cv::Mat normalized_depth;
    if (depth_image.type() == CV_16UC1) {
      depth_image.convertTo(normalized_depth, CV_8UC1, 255.0 / 5000.0);
    } else if (depth_image.type() == CV_32FC1) {
      depth_image.convertTo(normalized_depth, CV_8UC1, 255.0 / 5.0);
    } else {
      return;
    }
    cv::Mat depth_color;
    cv::applyColorMap(normalized_depth, depth_color, cv::COLORMAP_TURBO);
    auto depth_msg = cv_bridge::CvImage(header, "bgr8", depth_color).toImageMsg();
    debug_depth_pub_->publish(*depth_msg);
  }

  void writeCsv(const rclcpp::Time & stamp, const HumanTrack & track, double latency_ms)
  {
    if (!csv_.is_open()) {
      return;
    }
    csv_ << stamp.seconds() << ','
         << track.id << ','
         << track.x << ','
         << track.y << ','
         << track.vx << ','
         << track.vy << ','
         << track.confidence << ','
         << track.depth_m << ','
         << latency_ms << '\n';
  }

  mutable std::mutex data_mutex_;
  sensor_msgs::msg::Image::ConstSharedPtr latest_depth_;
  CameraIntrinsics intrinsics_;

  std::string rgb_topic_;
  std::string depth_topic_;
  std::string camera_info_topic_;
  std::string odom_topic_;
  std::string humans_topic_;
  std::string controller_humans_topic_;
  std::string context_topic_;
  std::string debug_image_topic_;
  std::string debug_depth_topic_;
  std::string debug_tracks_topic_;
  std::string debug_latency_topic_;
  std::string csv_path_;
  std::string target_frame_;
  double depth_scale_{0.001};
  double tf_timeout_sec_{0.05};
  bool enable_csv_logging_{true};
  double robot_x_{0.0};
  double robot_y_{0.0};
  double robot_vx_{0.0};
  double robot_vy_{0.0};

  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;
  YoloDetector detector_;
  DepthAssociation depth_association_;
  KalmanTracker tracker_;
  HumanStateEstimator estimator_;
  std::ofstream csv_;

  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr rgb_sub_;
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr depth_sub_;
  rclcpp::Subscription<sensor_msgs::msg::CameraInfo>::SharedPtr camera_info_sub_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;
  rclcpp::Publisher<rai_human_perception::msg::HumanArray>::SharedPtr humans_pub_;
  rclcpp::Publisher<rai_controller::msg::HumanStates>::SharedPtr controller_humans_pub_;
  rclcpp::Publisher<rai_human_perception::msg::ContextInput>::SharedPtr context_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr debug_image_pub_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr debug_depth_pub_;
  rclcpp::Publisher<rai_human_perception::msg::HumanArray>::SharedPtr debug_tracks_pub_;
  rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr debug_latency_pub_;
};

}  

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<rai_human_perception::PerceptionNode>());
  rclcpp::shutdown();
  return 0;
}
