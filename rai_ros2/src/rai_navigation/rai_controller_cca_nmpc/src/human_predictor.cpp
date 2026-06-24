#include "rai_controller_cca_nmpc/human_predictor.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace rai_controller_cca_nmpc
{

HumanPredictor::HumanPredictor(const CcanmpcParameters & parameters)
: params_(parameters)
{
}

double HumanPredictor::computeCovarianceConfidence(const HumanState & human) const
{
  const double covariance_trace =
    std::max(0.0, human.covariance_x) +
    std::max(0.0, human.covariance_y) +
    std::max(0.0, human.covariance_vx) +
    std::max(0.0, human.covariance_vy);
  return std::clamp(std::exp(-covariance_trace), 0.0, 1.0);
}

double HumanPredictor::computeEffectiveConfidence(const HumanState & human) const
{
  const double detector_confidence = std::clamp(human.confidence, 0.0, 1.0);
  return std::clamp(detector_confidence * computeCovarianceConfidence(human), 0.0, 1.0);
}

double HumanPredictor::computePhi(const SingleHumanPrediction & prediction) const
{
  if (!prediction.valid || !std::isfinite(prediction.distance)) {
    return 0.0;
  }

  const double distance_term =
    (params_.d0 - prediction.distance) / std::max(params_.d0, 1e-6);
  const double velocity_term =
    prediction.speed / std::max(params_.human_velocity_max, 1e-6);
  const double z =
    params_.context_distance_weight * distance_term +
    params_.context_velocity_weight * velocity_term +
    params_.context_direction_weight * prediction.direction_cos +
    params_.context_confidence_weight * prediction.confidence +
    params_.context_bias;
  return std::clamp(1.0 / (1.0 + std::exp(-z)), 0.0, 1.0);
}

PredictedHumanContext HumanPredictor::predictHumanContext(
  const std::vector<HumanState> & humans,
  double x,
  double y,
  double robot_vx_world,
  double robot_vy_world,
  double time_from_now) const
{
  PredictedHumanContext aggregated;
  double max_phi = 0.0;
  double min_distance = std::numeric_limits<double>::infinity();
  bool have_valid_human = false;

  for (const auto & human : humans) {
    if (human.age_sec > params_.max_human_age_sec) {
      continue;
    }

    const double effective_confidence = computeEffectiveConfidence(human);
    if (effective_confidence <= 0.0) {
      continue;
    }

    const double prediction_time = std::max(0.0, time_from_now - std::max(0.0, human.age_sec));
    const double hx = human.x + human.vx * prediction_time;
    const double hy = human.y + human.vy * prediction_time;
    if (!std::isfinite(hx) || !std::isfinite(hy) ||
      !std::isfinite(human.vx) || !std::isfinite(human.vy))
    {
      continue;
    }

    SingleHumanPrediction prediction;
    prediction.distance = std::hypot(x - hx, y - hy);
    prediction.speed = std::hypot(human.vx, human.vy);
    prediction.confidence = effective_confidence;
    prediction.valid = true;

    const double ex = (x - hx) / std::max(prediction.distance, 1e-6);
    const double ey = (y - hy) / std::max(prediction.distance, 1e-6);
    const double rel_vx = human.vx - robot_vx_world;
    const double rel_vy = human.vy - robot_vy_world;
    const double rel_speed = std::hypot(rel_vx, rel_vy);
    prediction.direction_cos = rel_speed > 1e-6 ?
      std::clamp((rel_vx * ex + rel_vy * ey) / rel_speed, -1.0, 1.0) :
      0.0;
    prediction.phi_h = computePhi(prediction);

    have_valid_human = true;
    min_distance = std::min(min_distance, prediction.distance);

    if (!aggregated.valid || prediction.phi_h > max_phi) {
      aggregated.speed = prediction.speed;
      aggregated.direction_cos = prediction.direction_cos;
      aggregated.confidence = prediction.confidence;
      max_phi = prediction.phi_h;
      aggregated.valid = true;
    }
  }

  if (!have_valid_human) {
    return aggregated;
  }

  aggregated.distance = min_distance;
  aggregated.phi_h = max_phi;
  return aggregated;
}

}  
