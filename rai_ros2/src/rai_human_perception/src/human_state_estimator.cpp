#include "rai_human_perception/human_state_estimator.hpp"

#include <chrono>

#include "tf2_geometry_msgs/tf2_geometry_msgs.hpp"

namespace rai_human_perception
{

HumanStateEstimator::HumanStateEstimator(std::string target_frame)
: target_frame_(std::move(target_frame))
{
}

std::vector<HumanMeasurement> HumanStateEstimator::transformMeasurements(
  const std::vector<HumanMeasurement> & camera_measurements,
  tf2_ros::Buffer & tf_buffer,
  double timeout_sec) const
{
  std::vector<HumanMeasurement> output;
  output.reserve(camera_measurements.size());
  const auto timeout = tf2::durationFromSec(timeout_sec);

  for (auto measurement : camera_measurements) {
    try {
      measurement.target_point = tf_buffer.transform(
        measurement.camera_point,
        target_frame_,
        timeout);
      output.push_back(measurement);
    } catch (const tf2::TransformException &) {
      continue;
    }
  }
  return output;
}

}  // namespace rai_human_perception
