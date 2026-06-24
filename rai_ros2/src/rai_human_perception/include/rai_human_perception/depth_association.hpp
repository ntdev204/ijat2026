#ifndef RAI_HUMAN_PERCEPTION__DEPTH_ASSOCIATION_HPP_
#define RAI_HUMAN_PERCEPTION__DEPTH_ASSOCIATION_HPP_

#include <optional>
#include <string>

#include <opencv2/core.hpp>

#include "std_msgs/msg/header.hpp"

#include "rai_human_perception/types.hpp"

namespace rai_human_perception
{

class DepthAssociation
{
public:
  DepthAssociation(double center_ratio, double min_depth_m, double max_depth_m);

  std::optional<HumanMeasurement> associate(
    const Detection2D & detection,
    const cv::Mat & depth_image,
    const CameraIntrinsics & intrinsics,
    const std_msgs::msg::Header & depth_header,
    double depth_scale) const;

private:
  double center_ratio_{0.5};
  double min_depth_m_{0.2};
  double max_depth_m_{6.0};
};

}  

#endif  
