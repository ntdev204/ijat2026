#ifndef RAI_HUMAN_PERCEPTION__HUMAN_STATE_ESTIMATOR_HPP_
#define RAI_HUMAN_PERCEPTION__HUMAN_STATE_ESTIMATOR_HPP_

#include <string>
#include <vector>

#include "geometry_msgs/msg/point_stamped.hpp"
#include "tf2_ros/buffer.h"

#include "rai_human_perception/types.hpp"

namespace rai_human_perception
{

class HumanStateEstimator
{
public:
  explicit HumanStateEstimator(std::string target_frame);

  std::vector<HumanMeasurement> transformMeasurements(
    const std::vector<HumanMeasurement> & camera_measurements,
    tf2_ros::Buffer & tf_buffer,
    double timeout_sec) const;

private:
  std::string target_frame_;
};

}  // namespace rai_human_perception

#endif  // RAI_HUMAN_PERCEPTION__HUMAN_STATE_ESTIMATOR_HPP_
