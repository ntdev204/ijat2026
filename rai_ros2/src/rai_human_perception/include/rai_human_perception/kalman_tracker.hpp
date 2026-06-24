#ifndef RAI_HUMAN_PERCEPTION__KALMAN_TRACKER_HPP_
#define RAI_HUMAN_PERCEPTION__KALMAN_TRACKER_HPP_

#include <map>
#include <vector>

#include <opencv2/video/tracking.hpp>

#include "rclcpp/time.hpp"

#include "rai_human_perception/types.hpp"

namespace rai_human_perception
{

struct KalmanOptions
{
  double process_noise_position{0.08};
  double process_noise_velocity{0.8};
  double measurement_noise_position{0.05};
  double association_gate_m{0.8};
  double max_track_age_sec{0.7};
};

class KalmanTracker
{
public:
  explicit KalmanTracker(KalmanOptions options);

  std::vector<HumanTrack> update(
    const std::vector<HumanMeasurement> & measurements,
    const rclcpp::Time & stamp);

private:
  struct TrackState
  {
    int id{-1};
    cv::KalmanFilter filter;
    rclcpp::Time last_update;
    rclcpp::Time first_seen;
    double confidence{0.0};
    double depth_m{0.0};
  };

  cv::KalmanFilter makeFilter(const HumanMeasurement & measurement) const;
  void predict(TrackState & track, double dt) const;
  void correct(TrackState & track, const HumanMeasurement & measurement) const;
  int associateMeasurement(const HumanMeasurement & measurement, std::vector<int> & claimed_ids) const;
  HumanTrack toHumanTrack(const TrackState & track, const rclcpp::Time & stamp) const;

  KalmanOptions options_;
  std::map<int, TrackState> tracks_;
  int next_track_id_{1};
};

}  

#endif  
