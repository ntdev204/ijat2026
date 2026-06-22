#include "rai_human_perception/depth_association.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace rai_human_perception
{

DepthAssociation::DepthAssociation(double center_ratio, double min_depth_m, double max_depth_m)
: center_ratio_(std::clamp(center_ratio, 0.1, 1.0)),
  min_depth_m_(min_depth_m),
  max_depth_m_(max_depth_m)
{
}

std::optional<HumanMeasurement> DepthAssociation::associate(
  const Detection2D & detection,
  const cv::Mat & depth_image,
  const CameraIntrinsics & intrinsics,
  const std_msgs::msg::Header & depth_header,
  double depth_scale) const
{
  if (depth_image.empty() || !intrinsics.valid || depth_scale <= 0.0) {
    return std::nullopt;
  }

  const cv::Rect image_rect(0, 0, depth_image.cols, depth_image.rows);
  const cv::Rect bbox = detection.bbox & image_rect;
  if (bbox.area() <= 0) {
    return std::nullopt;
  }

  const int roi_w = std::max(1, static_cast<int>(bbox.width * center_ratio_));
  const int roi_h = std::max(1, static_cast<int>(bbox.height * center_ratio_));
  const int roi_x = bbox.x + (bbox.width - roi_w) / 2;
  const int roi_y = bbox.y + (bbox.height - roi_h) / 2;
  const cv::Rect roi(roi_x, roi_y, roi_w, roi_h);

  std::vector<double> depths;
  depths.reserve(static_cast<size_t>(roi.area()));
  for (int y = roi.y; y < roi.y + roi.height; ++y) {
    for (int x = roi.x; x < roi.x + roi.width; ++x) {
      double z = 0.0;
      if (depth_image.type() == CV_16UC1) {
        z = static_cast<double>(depth_image.at<uint16_t>(y, x)) * depth_scale;
      } else if (depth_image.type() == CV_32FC1) {
        z = static_cast<double>(depth_image.at<float>(y, x)) * depth_scale;
      } else {
        continue;
      }
      if (std::isfinite(z) && z >= min_depth_m_ && z <= max_depth_m_) {
        depths.push_back(z);
      }
    }
  }
  if (depths.empty()) {
    return std::nullopt;
  }

  const auto median_it = depths.begin() + static_cast<long>(depths.size() / 2);
  std::nth_element(depths.begin(), median_it, depths.end());
  const double z_h = *median_it;
  const double u = static_cast<double>(bbox.x + bbox.width / 2);
  const double v = static_cast<double>(bbox.y + bbox.height / 2);

  HumanMeasurement measurement;
  measurement.id = detection.track_id;
  measurement.confidence = detection.confidence;
  measurement.depth_m = z_h;
  measurement.camera_point.header = depth_header;
  measurement.camera_point.point.x = (u - intrinsics.cx) * z_h / intrinsics.fx;
  measurement.camera_point.point.y = (v - intrinsics.cy) * z_h / intrinsics.fy;
  measurement.camera_point.point.z = z_h;
  return measurement;
}

}  // namespace rai_human_perception
