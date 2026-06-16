#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <string>

#include "geometry_msgs/msg/point_stamped.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"
#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"

class ScanBoxFilterNode : public rclcpp::Node
{
public:
  ScanBoxFilterNode()
  : Node("scan_box_filter_node"),
    tf_buffer_(this->get_clock()),
    tf_listener_(tf_buffer_)
  {
    input_topic_ = declare_parameter<std::string>("input_topic", "/scan");
    output_topic_ = declare_parameter<std::string>("output_topic", "/scan_filtered");
    box_frame_ = declare_parameter<std::string>("box_frame", "base_footprint");
    min_x_ = declare_parameter<double>("min_x", -0.50);
    max_x_ = declare_parameter<double>("max_x", 0.30);
    min_y_ = declare_parameter<double>("min_y", -0.35);
    max_y_ = declare_parameter<double>("max_y", 0.35);
    min_z_ = declare_parameter<double>("min_z", -0.20);
    max_z_ = declare_parameter<double>("max_z", 0.35);
    invert_ = declare_parameter<bool>("invert", false);

    pub_ = create_publisher<sensor_msgs::msg::LaserScan>(output_topic_, rclcpp::SensorDataQoS());
    sub_ = create_subscription<sensor_msgs::msg::LaserScan>(
      input_topic_,
      rclcpp::SensorDataQoS(),
      std::bind(&ScanBoxFilterNode::scanCallback, this, std::placeholders::_1));

    RCLCPP_INFO(
      get_logger(),
      "Filtering %s -> %s in %s box x[%.2f, %.2f] y[%.2f, %.2f] z[%.2f, %.2f]",
      input_topic_.c_str(), output_topic_.c_str(), box_frame_.c_str(),
      min_x_, max_x_, min_y_, max_y_, min_z_, max_z_);
  }

private:
  void scanCallback(const sensor_msgs::msg::LaserScan::SharedPtr scan)
  {
    auto filtered = *scan;
    filtered.header.stamp = now();

    geometry_msgs::msg::TransformStamped transform;
    try {
      transform = tf_buffer_.lookupTransform(
        box_frame_, scan->header.frame_id, tf2::TimePointZero);
    } catch (const tf2::TransformException & ex) {
      RCLCPP_WARN_THROTTLE(
        get_logger(), *get_clock(), 2000,
        "Cannot transform scan from %s to %s: %s",
        scan->header.frame_id.c_str(), box_frame_.c_str(), ex.what());
      pub_->publish(filtered);
      return;
    }

    for (size_t i = 0; i < filtered.ranges.size(); ++i) {
      const float range = filtered.ranges[i];
      if (!std::isfinite(range) || range < scan->range_min || range > scan->range_max) {
        continue;
      }

      const double angle = scan->angle_min + static_cast<double>(i) * scan->angle_increment;
      geometry_msgs::msg::PointStamped point_in;
      point_in.header = scan->header;
      point_in.point.x = range * std::cos(angle);
      point_in.point.y = range * std::sin(angle);
      point_in.point.z = 0.0;

      geometry_msgs::msg::PointStamped point_out;
      tf2::doTransform(point_in, point_out, transform);

      const bool inside =
        point_out.point.x >= min_x_ && point_out.point.x <= max_x_ &&
        point_out.point.y >= min_y_ && point_out.point.y <= max_y_ &&
        point_out.point.z >= min_z_ && point_out.point.z <= max_z_;

      if (inside != invert_) {
        filtered.ranges[i] = std::numeric_limits<float>::infinity();
      }
    }

    pub_->publish(filtered);
  }

  std::string input_topic_;
  std::string output_topic_;
  std::string box_frame_;
  double min_x_;
  double max_x_;
  double min_y_;
  double max_y_;
  double min_z_;
  double max_z_;
  bool invert_;

  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;
  rclcpp::Publisher<sensor_msgs::msg::LaserScan>::SharedPtr pub_;
  rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr sub_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<ScanBoxFilterNode>());
  rclcpp::shutdown();
  return 0;
}
