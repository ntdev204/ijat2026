#include "rai_human_perception/kalman_tracker.hpp"

#include <algorithm>
#include <cmath>

namespace rai_human_perception
{

KalmanTracker::KalmanTracker(KalmanOptions options)
: options_(options)
{
}

std::vector<HumanTrack> KalmanTracker::update(
  const std::vector<HumanMeasurement> & measurements,
  const rclcpp::Time & stamp)
{
  std::vector<int> claimed_ids;
  for (const auto & measurement : measurements) {
    int track_id = associateMeasurement(measurement, claimed_ids);
    if (track_id < 0) {
      track_id = next_track_id_++;
    }
    claimed_ids.push_back(track_id);

    auto it = tracks_.find(track_id);
    if (it == tracks_.end()) {
      TrackState track;
      track.id = track_id;
      track.filter = makeFilter(measurement);
      track.first_seen = stamp;
      track.last_update = stamp;
      track.confidence = measurement.confidence;
      track.depth_m = measurement.depth_m;
      tracks_[track_id] = track;
      continue;
    }

    const double dt = std::max(1e-3, (stamp - it->second.last_update).seconds());
    predict(it->second, dt);
    correct(it->second, measurement);
    it->second.last_update = stamp;
    it->second.confidence = measurement.confidence;
    it->second.depth_m = measurement.depth_m;
  }

  for (auto it = tracks_.begin(); it != tracks_.end();) {
    if ((stamp - it->second.last_update).seconds() > options_.max_track_age_sec) {
      it = tracks_.erase(it);
    } else {
      ++it;
    }
  }

  std::vector<HumanTrack> output;
  output.reserve(tracks_.size());
  for (auto & [id, track] : tracks_) {
    const double dt = std::max(0.0, (stamp - track.last_update).seconds());
    if (dt > 1e-6) {
      predict(track, dt);
      track.last_update = stamp;
    }
    output.push_back(toHumanTrack(track, stamp));
  }
  return output;
}

cv::KalmanFilter KalmanTracker::makeFilter(const HumanMeasurement & measurement) const
{
  cv::KalmanFilter filter(4, 2, 0, CV_64F);
  filter.transitionMatrix = (cv::Mat_<double>(4, 4) <<
    1, 0, 1, 0,
    0, 1, 0, 1,
    0, 0, 1, 0,
    0, 0, 0, 1);
  filter.measurementMatrix = (cv::Mat_<double>(2, 4) <<
    1, 0, 0, 0,
    0, 1, 0, 0);
  cv::setIdentity(filter.processNoiseCov, cv::Scalar::all(options_.process_noise_position));
  filter.processNoiseCov.at<double>(2, 2) = options_.process_noise_velocity;
  filter.processNoiseCov.at<double>(3, 3) = options_.process_noise_velocity;
  cv::setIdentity(filter.measurementNoiseCov, cv::Scalar::all(options_.measurement_noise_position));
  cv::setIdentity(filter.errorCovPost, cv::Scalar::all(0.1));
  filter.statePost.at<double>(0) = measurement.target_point.point.x;
  filter.statePost.at<double>(1) = measurement.target_point.point.y;
  filter.statePost.at<double>(2) = 0.0;
  filter.statePost.at<double>(3) = 0.0;
  return filter;
}

void KalmanTracker::predict(TrackState & track, double dt) const
{
  track.filter.transitionMatrix.at<double>(0, 2) = dt;
  track.filter.transitionMatrix.at<double>(1, 3) = dt;
  track.filter.predict();
}

void KalmanTracker::correct(TrackState & track, const HumanMeasurement & measurement) const
{
  cv::Mat z(2, 1, CV_64F);
  z.at<double>(0) = measurement.target_point.point.x;
  z.at<double>(1) = measurement.target_point.point.y;
  track.filter.correct(z);
}

int KalmanTracker::associateMeasurement(
  const HumanMeasurement & measurement,
  std::vector<int> & claimed_ids) const
{
  int best_id = -1;
  double best_distance = options_.association_gate_m;
  for (const auto & [id, track] : tracks_) {
    if (std::find(claimed_ids.begin(), claimed_ids.end(), id) != claimed_ids.end()) {
      continue;
    }
    const double dx = track.filter.statePost.at<double>(0) - measurement.target_point.point.x;
    const double dy = track.filter.statePost.at<double>(1) - measurement.target_point.point.y;
    const double distance = std::hypot(dx, dy);
    if (distance < best_distance) {
      best_distance = distance;
      best_id = id;
    }
  }
  return best_id;
}

HumanTrack KalmanTracker::toHumanTrack(const TrackState & track, const rclcpp::Time & stamp) const
{
  HumanTrack human;
  human.id = track.id;
  human.x = track.filter.statePost.at<double>(0);
  human.y = track.filter.statePost.at<double>(1);
  human.vx = track.filter.statePost.at<double>(2);
  human.vy = track.filter.statePost.at<double>(3);
  human.confidence = track.confidence;
  human.age_sec = (stamp - track.first_seen).seconds();
  human.covariance_x = track.filter.errorCovPost.at<double>(0, 0);
  human.covariance_y = track.filter.errorCovPost.at<double>(1, 1);
  human.covariance_vx = track.filter.errorCovPost.at<double>(2, 2);
  human.covariance_vy = track.filter.errorCovPost.at<double>(3, 3);
  human.depth_m = track.depth_m;
  return human;
}

}  
