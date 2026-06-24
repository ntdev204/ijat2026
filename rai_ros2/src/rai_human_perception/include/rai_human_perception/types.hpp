#ifndef RAI_HUMAN_PERCEPTION__TYPES_HPP_
#define RAI_HUMAN_PERCEPTION__TYPES_HPP_

#include <opencv2/core.hpp>

#include <vector>

#include "geometry_msgs/msg/point_stamped.hpp"

namespace rai_human_perception
{

struct Detection2D
{
  int class_id{0};
  int track_id{-1};
  double confidence{0.0};
  cv::Rect bbox;
};

struct HumanMeasurement
{
  int id{-1};
  double confidence{0.0};
  double depth_m{0.0};
  geometry_msgs::msg::PointStamped camera_point;
  geometry_msgs::msg::PointStamped target_point;
};

struct HumanTrack
{
  int id{-1};
  double x{0.0};
  double y{0.0};
  double vx{0.0};
  double vy{0.0};
  double confidence{0.0};
  double age_sec{0.0};
  double covariance_x{0.0};
  double covariance_y{0.0};
  double covariance_vx{0.0};
  double covariance_vy{0.0};
  double depth_m{0.0};
};

struct CameraIntrinsics
{
  double fx{0.0};
  double fy{0.0};
  double cx{0.0};
  double cy{0.0};
  bool valid{false};
};

}  

#endif  
