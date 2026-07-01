#include "rai_controller_cca_nmpc/ccanmpc_core.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>

namespace rai_controller_cca_nmpc
{
namespace
{

double finiteOr(double value, double fallback)
{
  return std::isfinite(value) ? value : fallback;
}

bool poseIsFinite(const geometry_msgs::msg::PoseStamped & pose)
{
  return std::isfinite(pose.pose.position.x) &&
         std::isfinite(pose.pose.position.y) &&
         std::isfinite(pose.pose.orientation.x) &&
         std::isfinite(pose.pose.orientation.y) &&
         std::isfinite(pose.pose.orientation.z) &&
         std::isfinite(pose.pose.orientation.w);
}

double distance2d(const geometry_msgs::msg::PoseStamped & a, const geometry_msgs::msg::PoseStamped & b)
{
  return std::hypot(a.pose.position.x - b.pose.position.x, a.pose.position.y - b.pose.position.y);
}

std::vector<double> linspace(double min_value, double max_value, int samples)
{
  if (samples <= 1) {
    return {(min_value + max_value) * 0.5};
  }

  std::vector<double> values;
  values.reserve(static_cast<size_t>(samples));
  const double step = (max_value - min_value) / static_cast<double>(samples - 1);
  for (int i = 0; i < samples; ++i) {
    values.push_back(min_value + step * static_cast<double>(i));
  }
  return values;
}

double clampDelta(double target, double current, double max_delta)
{
  return current + std::clamp(target - current, -max_delta, max_delta);
}

struct ControlSample
{
  double vx{0.0};
  double vy{0.0};
  double omega{0.0};
};

bool nearlySame(double a, double b)
{
  return std::abs(a - b) < 1e-6;
}

void addUniqueSample(std::vector<ControlSample> & samples, double vx, double vy, double omega)
{
  for (const auto & sample : samples) {
    if (nearlySame(sample.vx, vx) && nearlySame(sample.vy, vy) && nearlySame(sample.omega, omega)) {
      return;
    }
  }
  samples.push_back({vx, vy, omega});
}

std::vector<ControlSample> makeGridSamples(
  const std::vector<double> & vx_values,
  const std::vector<double> & vy_values,
  const std::vector<double> & omega_values,
  size_t max_samples)
{
  std::vector<ControlSample> samples;
  samples.reserve(std::min(
    max_samples,
    vx_values.size() * vy_values.size() * omega_values.size()));

  for (double vx : vx_values) {
    for (double vy : vy_values) {
      for (double omega : omega_values) {
        addUniqueSample(samples, vx, vy, omega);
        if (samples.size() >= max_samples) {
          return samples;
        }
      }
    }
  }
  return samples;
}

}  

double normalizeAngle(double angle)
{
  while (angle > M_PI) {
    angle -= 2.0 * M_PI;
  }
  while (angle < -M_PI) {
    angle += 2.0 * M_PI;
  }
  return angle;
}

double yawFromPose(const geometry_msgs::msg::PoseStamped & pose)
{
  const auto & q = pose.pose.orientation;
  return std::atan2(
    2.0 * (q.w * q.z + q.x * q.y),
    1.0 - 2.0 * (q.y * q.y + q.z * q.z));
}

geometry_msgs::msg::PoseStamped makePose(
  const std::string & frame_id,
  const rclcpp::Time & stamp,
  double x,
  double y,
  double yaw)
{
  geometry_msgs::msg::PoseStamped pose;
  pose.header.frame_id = frame_id;
  pose.header.stamp = stamp;
  pose.pose.position.x = x;
  pose.pose.position.y = y;
  pose.pose.orientation.z = std::sin(yaw * 0.5);
  pose.pose.orientation.w = std::cos(yaw * 0.5);
  return pose;
}

CcanmpcCore::CcanmpcCore(CcanmpcParameters parameters)
: params_(parameters), human_predictor_(params_)
{
  params_.horizon_steps = std::max(4, params_.horizon_steps);
  params_.sample_time = std::clamp(params_.sample_time, 0.02, 0.25);
}

void CcanmpcCore::reset()
{
  last_command_ = geometry_msgs::msg::Twist();
}

nav_msgs::msg::Path CcanmpcCore::pruneAndCropReference(
  const geometry_msgs::msg::PoseStamped & robot_pose,
  const nav_msgs::msg::Path & reference_path) const
{
  nav_msgs::msg::Path local_path;
  local_path.header = reference_path.header;
  local_path.header.stamp = robot_pose.header.stamp;

  if (reference_path.poses.empty()) {
    return local_path;
  }

  size_t closest_idx = 0;
  double closest_dist = std::numeric_limits<double>::infinity();
  for (size_t i = 0; i < reference_path.poses.size(); ++i) {
    const double dist = distance2d(robot_pose, reference_path.poses[i]);
    if (dist < closest_dist) {
      closest_idx = i;
      closest_dist = dist;
    }
  }

  double length = 0.0;
  geometry_msgs::msg::PoseStamped previous;
  bool have_previous = false;
  for (size_t i = closest_idx; i < reference_path.poses.size(); ++i) {
    auto pose = reference_path.poses[i];
    if (pose.header.frame_id.empty()) {
      pose.header.frame_id = reference_path.header.frame_id;
    }
    if (!poseIsFinite(pose)) {
      continue;
    }

    if (have_previous) {
      length += distance2d(previous, pose);
      if (length > params_.max_reference_length) {
        break;
      }
    }

    local_path.poses.push_back(pose);
    previous = pose;
    have_previous = true;
  }

  return local_path;
}

std::vector<geometry_msgs::msg::PoseStamped> CcanmpcCore::resampleReference(
  const geometry_msgs::msg::PoseStamped & robot_pose,
  const nav_msgs::msg::Path & local_path,
  double v_ref) const
{
  std::vector<geometry_msgs::msg::PoseStamped> reference;
  reference.reserve(static_cast<size_t>(params_.horizon_steps + 1));

  if (local_path.poses.empty()) {
    for (int i = 0; i <= params_.horizon_steps; ++i) {
      reference.push_back(robot_pose);
    }
    return reference;
  }

  std::vector<double> arclength;
  arclength.reserve(local_path.poses.size());
  arclength.push_back(0.0);
  for (size_t i = 1; i < local_path.poses.size(); ++i) {
    arclength.push_back(arclength.back() + distance2d(local_path.poses[i - 1], local_path.poses[i]));
  }

  if (arclength.empty() || arclength.back() < 1e-6) {
    std::cerr << "[ccanmpc] Warning: reference path is degenerate (zero or near-zero length); skipping reference generation." << std::endl;
    return reference;
  }

  const double ds = std::max(0.01, v_ref * params_.sample_time);
  for (int step = 0; step <= params_.horizon_steps; ++step) {
    const double target_s = ds * static_cast<double>(step);
    const auto it = std::lower_bound(arclength.begin(), arclength.end(), target_s);
    const size_t idx = static_cast<size_t>(std::distance(arclength.begin(), it));

    if (idx == 0) {
      reference.push_back(local_path.poses.front());
      continue;
    }
    if (idx >= local_path.poses.size()) {
      reference.push_back(local_path.poses.back());
      continue;
    }

    const double s0 = arclength[idx - 1];
    const double s1 = arclength[idx];
    const double ratio = (s1 > s0) ? (target_s - s0) / (s1 - s0) : 0.0;
    const auto & p0 = local_path.poses[idx - 1];
    const auto & p1 = local_path.poses[idx];
    const double yaw = std::atan2(
      p1.pose.position.y - p0.pose.position.y,
      p1.pose.position.x - p0.pose.position.x);

    reference.push_back(makePose(
      local_path.header.frame_id,
      robot_pose.header.stamp,
      p0.pose.position.x + ratio * (p1.pose.position.x - p0.pose.position.x),
      p0.pose.position.y + ratio * (p1.pose.position.y - p0.pose.position.y),
      yaw));
  }

  return reference;
}

double CcanmpcCore::occupancyCost(const nav_msgs::msg::OccupancyGrid * costmap, double x, double y) const
{
  if (!costmap || costmap->data.empty() || costmap->info.resolution <= 0.0) {
    return 0.0;
  }

  const double origin_x = costmap->info.origin.position.x;
  const double origin_y = costmap->info.origin.position.y;
  const int mx = static_cast<int>(std::floor((x - origin_x) / costmap->info.resolution));
  const int my = static_cast<int>(std::floor((y - origin_y) / costmap->info.resolution));
  if (mx < 0 || my < 0 ||
    mx >= static_cast<int>(costmap->info.width) ||
    my >= static_cast<int>(costmap->info.height))
  {
    return 100.0;
  }

  const size_t idx = static_cast<size_t>(my) * costmap->info.width + static_cast<size_t>(mx);
  const int raw = static_cast<int>(costmap->data[idx]);
  if (raw < 0) {
    return 100.0;
  }
  return static_cast<double>(raw);
}

ContextState CcanmpcCore::computeAdaptiveParameters(double phi_h) const
{
  ContextState adaptive;
  adaptive.phi_h = std::clamp(phi_h, 0.0, 1.0);
  const double d_safe_upper = std::min(params_.d_safe_max, params_.d_safe_0 + params_.k_d);
  const double vx_lower = std::max(params_.vx_max_min, params_.vx_max_0 - params_.k_vx);
  const double vy_lower = std::max(params_.vy_max_min, params_.vy_max_0 - params_.k_vy);
  const double omega_lower = std::max(params_.omega_max_min, params_.omega_max_0 - params_.k_omega);
  adaptive.d_safe = std::clamp(
    params_.d_safe_0 + params_.k_d * adaptive.phi_h,
    params_.d_safe_0,
    d_safe_upper);
  adaptive.vx_max = std::clamp(
    params_.vx_max_0 - params_.k_vx * adaptive.phi_h,
    vx_lower,
    params_.vx_max_0);
  adaptive.vy_max = std::clamp(
    params_.vy_max_0 - params_.k_vy * adaptive.phi_h,
    vy_lower,
    params_.vy_max_0);
  adaptive.omega_max = std::clamp(
    params_.omega_max_0 - params_.k_omega * adaptive.phi_h,
    omega_lower,
    params_.omega_max_0);
  return adaptive;
}

ContextState CcanmpcCore::mergeExternalLimits(
  const ContextState & adaptive,
  const ContextState & external_context) const
{
  ContextState merged = adaptive;
  if (external_context.d_safe > 0.0) {
    merged.d_safe = std::max(
      merged.d_safe,
      std::clamp(external_context.d_safe, params_.d_safe_0, params_.d_safe_max));
  }
  if (external_context.vx_max > 0.0) {
    merged.vx_max = std::max(params_.vx_max_min, std::min(merged.vx_max, external_context.vx_max));
  }
  if (external_context.vy_max > 0.0) {
    merged.vy_max = std::max(params_.vy_max_min, std::min(merged.vy_max, external_context.vy_max));
  }
  if (external_context.omega_max > 0.0) {
    merged.omega_max = std::max(
      params_.omega_max_min,
      std::min(merged.omega_max, external_context.omega_max));
  }
  return merged;
}

double CcanmpcCore::adaptiveVelocityConstraintPenalty(
  double vx,
  double vy,
  double omega,
  const ContextState & adaptive,
  bool & violated) const
{
  const double vx_violation = std::max(0.0, std::abs(vx) - adaptive.vx_max);
  const double vy_violation = std::max(0.0, std::abs(vy) - adaptive.vy_max);
  const double omega_violation = std::max(0.0, std::abs(omega) - adaptive.omega_max);
  violated = vx_violation > 0.0 || vy_violation > 0.0 || omega_violation > 0.0;
  if (!violated) {
    return 0.0;
  }

  
  
  return params_.w_slack * (
    vx_violation * vx_violation +
    vy_violation * vy_violation +
    omega_violation * omega_violation);
}

void CcanmpcCore::rolloutMecanumStep(
  double & x,
  double & y,
  double & yaw,
  double vx,
  double vy,
  double omega,
  double dt) const
{
  const auto derivative = [vx, vy, omega](double heading) {
    return std::array<double, 3>{
      vx * std::cos(heading) - vy * std::sin(heading),
      vx * std::sin(heading) + vy * std::cos(heading),
      omega};
  };

  const auto k1 = derivative(yaw);
  const auto k2 = derivative(yaw + 0.5 * dt * k1[2]);
  const auto k3 = derivative(yaw + 0.5 * dt * k2[2]);
  const auto k4 = derivative(yaw + dt * k3[2]);

  x += (dt / 6.0) * (k1[0] + 2.0 * k2[0] + 2.0 * k3[0] + k4[0]);
  y += (dt / 6.0) * (k1[1] + 2.0 * k2[1] + 2.0 * k3[1] + k4[1]);
  yaw = normalizeAngle(yaw + (dt / 6.0) * (k1[2] + 2.0 * k2[2] + 2.0 * k3[2] + k4[2]));
}

double CcanmpcCore::scoreTrackingError(
  double x,
  double y,
  double yaw,
  const geometry_msgs::msg::PoseStamped & reference,
  double phi_h) const
{
  const double ref_yaw = finiteOr(yawFromPose(reference), yaw);
  const double qx = params_.q_x + phi_h * params_.q_human_x;
  const double qy = params_.q_y + phi_h * params_.q_human_y;
  const double qtheta = params_.q_theta + phi_h * params_.q_human_theta;
  return
    qx * std::pow(x - reference.pose.position.x, 2.0) +
    qy * std::pow(y - reference.pose.position.y, 2.0) +
    qtheta * std::pow(normalizeAngle(yaw - ref_yaw), 2.0);
}

SolveResult CcanmpcCore::solve(
  const geometry_msgs::msg::PoseStamped & robot_pose,
  const geometry_msgs::msg::Twist & robot_velocity,
  const nav_msgs::msg::Path & reference_path,
  const std::vector<HumanState> & humans,
  const ContextState & context,
  const nav_msgs::msg::OccupancyGrid * costmap)
{
  const auto start = std::chrono::steady_clock::now();
  SolveResult result;
  result.local_reference_path.header.frame_id = robot_pose.header.frame_id;
  result.local_reference_path.header.stamp = robot_pose.header.stamp;
  result.predicted_trajectory.header = result.local_reference_path.header;

  if (!poseIsFinite(robot_pose)) {
    result.status = "INVALID_ROBOT_POSE";
    return result;
  }

  const nav_msgs::msg::Path local_path = pruneAndCropReference(robot_pose, reference_path);
  if (local_path.poses.empty()) {
    result.status = "NO_REFERENCE_PATH";
    return result;
  }

  const double yaw0 = finiteOr(yawFromPose(robot_pose), 0.0);
  const double robot_vx_world =
    robot_velocity.linear.x * std::cos(yaw0) - robot_velocity.linear.y * std::sin(yaw0);
  const double robot_vy_world =
    robot_velocity.linear.x * std::sin(yaw0) + robot_velocity.linear.y * std::cos(yaw0);
  const auto measured_context = human_predictor_.predictHumanContext(
    humans,
    robot_pose.pose.position.x,
    robot_pose.pose.position.y,
    robot_vx_world,
    robot_vy_world,
    0.0);
  const double measured_phi = measured_context.phi_h;
  result.phi_h = std::max(std::clamp(context.phi_h, 0.0, 1.0), measured_phi);
  const ContextState adaptive = mergeExternalLimits(computeAdaptiveParameters(result.phi_h), context);
  result.q_scale = 1.0 + result.phi_h;
  result.d_safe = adaptive.d_safe;
  result.vx_max = adaptive.vx_max;
  result.vy_max = adaptive.vy_max;
  result.omega_max = adaptive.omega_max;

  const double v_ref = std::clamp(params_.default_v_ref, params_.vx_max_min, result.vx_max);
  const auto reference = resampleReference(robot_pose, local_path, v_ref);
  result.local_reference_path.header = local_path.header;
  result.local_reference_path.header.stamp = robot_pose.header.stamp;
  result.local_reference_path.poses = reference;

  const double x0 = robot_pose.pose.position.x;
  const double y0 = robot_pose.pose.position.y;
  const auto & lookahead = reference[std::min<size_t>(1, reference.size() - 1)];
  const double path_heading = std::atan2(
    lookahead.pose.position.y - y0,
    lookahead.pose.position.x - x0);
  const double heading_error = normalizeAngle(path_heading - yaw0);
  const double nominal_vx = std::clamp(v_ref * std::cos(heading_error), -0.30 * result.vx_max, result.vx_max);
  const double nominal_vy = std::clamp(v_ref * std::sin(heading_error), -result.vy_max, result.vy_max);
  const double nominal_omega = std::clamp(
    heading_error / std::max(params_.sample_time * static_cast<double>(params_.horizon_steps), 1e-3),
    -result.omega_max,
    result.omega_max);

  const int max_samples = std::max(12, params_.max_control_samples);
  const int coarse_v = params_.adaptive_sampling ? std::min(3, std::max(2, params_.vx_samples)) : params_.vx_samples;
  const int coarse_w = params_.adaptive_sampling ? std::min(5, std::max(3, params_.omega_samples)) : params_.omega_samples;
  const auto coarse_vx = linspace(-0.30 * result.vx_max, result.vx_max, coarse_v);
  const auto coarse_vy = linspace(-result.vy_max, result.vy_max, coarse_v);
  const auto coarse_omega = linspace(-result.omega_max, result.omega_max, coarse_w);

  std::vector<ControlSample> candidates;
  candidates.reserve(static_cast<size_t>(max_samples));
  addUniqueSample(candidates, 0.0, 0.0, 0.0);
  addUniqueSample(candidates, last_command_.linear.x, last_command_.linear.y, last_command_.angular.z);
  addUniqueSample(candidates, nominal_vx, nominal_vy, nominal_omega);

  auto coarse_samples = makeGridSamples(coarse_vx, coarse_vy, coarse_omega, static_cast<size_t>(max_samples));
  std::sort(
    coarse_samples.begin(), coarse_samples.end(),
    [&](const auto & a, const auto & b) {
      const auto score = [&](const auto & s) {
        return std::pow((s.vx - nominal_vx) / std::max(result.vx_max, 1e-3), 2.0) +
               std::pow((s.vy - nominal_vy) / std::max(result.vy_max, 1e-3), 2.0) +
               std::pow((s.omega - nominal_omega) / std::max(result.omega_max, 1e-3), 2.0);
      };
      return score(a) < score(b);
    });
  for (const auto & sample : coarse_samples) {
    addUniqueSample(candidates, sample.vx, sample.vy, sample.omega);
    if (static_cast<int>(candidates.size()) >= max_samples) {
      break;
    }
  }

  if (params_.adaptive_sampling && static_cast<int>(candidates.size()) < max_samples) {
    const int refine_samples = std::clamp(params_.refinement_samples, 2, 7);
    const double vx_radius = result.vx_max * std::clamp(params_.refinement_radius_fraction, 0.05, 0.8);
    const double vy_radius = result.vy_max * std::clamp(params_.refinement_radius_fraction, 0.05, 0.8);
    const double omega_radius = result.omega_max * std::clamp(params_.refinement_radius_fraction, 0.05, 0.8);
    const std::vector<ControlSample> centers = {
      {last_command_.linear.x, last_command_.linear.y, last_command_.angular.z},
      {nominal_vx, nominal_vy, nominal_omega},
    };
    for (const auto & center : centers) {
      const auto vx_values = linspace(
        std::clamp(center.vx - vx_radius, -0.30 * result.vx_max, result.vx_max),
        std::clamp(center.vx + vx_radius, -0.30 * result.vx_max, result.vx_max),
        refine_samples);
      const auto vy_values = linspace(
        std::clamp(center.vy - vy_radius, -result.vy_max, result.vy_max),
        std::clamp(center.vy + vy_radius, -result.vy_max, result.vy_max),
        refine_samples);
      const auto omega_values = linspace(
        std::clamp(center.omega - omega_radius, -result.omega_max, result.omega_max),
        std::clamp(center.omega + omega_radius, -result.omega_max, result.omega_max),
        refine_samples);
      auto local_samples = makeGridSamples(vx_values, vy_values, omega_values, static_cast<size_t>(max_samples));
      for (const auto & sample : local_samples) {
        addUniqueSample(candidates, sample.vx, sample.vy, sample.omega);
        if (static_cast<int>(candidates.size()) >= max_samples) {
          break;
        }
      }
      if (static_cast<int>(candidates.size()) >= max_samples) {
        break;
      }
    }
  }

  if (!params_.adaptive_sampling) {
    candidates = makeGridSamples(
      linspace(-0.30 * result.vx_max, result.vx_max, params_.vx_samples),
      linspace(-result.vy_max, result.vy_max, params_.vy_samples),
      linspace(-result.omega_max, result.omega_max, params_.omega_samples),
      static_cast<size_t>(std::max(1, params_.vx_samples * params_.vy_samples * params_.omega_samples)));
  }

  const double max_dvx = params_.acc_lim_x * params_.sample_time;
  const double max_dvy = params_.acc_lim_y * params_.sample_time;
  const double max_domega = params_.acc_lim_theta * params_.sample_time;

  double best_score = std::numeric_limits<double>::infinity();
  geometry_msgs::msg::Twist best_command;
  nav_msgs::msg::Path best_path;
  best_path.header = result.predicted_trajectory.header;

  for (const auto & candidate : candidates) {
        const double elapsed_ms =
          std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        if (elapsed_ms > std::max(1.0, params_.max_compute_time_ms - params_.timeout_margin_ms)) {
          result.timeout = true;
          break;
        }

        ++result.sample_count;
        const double vx = std::clamp(
          clampDelta(candidate.vx, robot_velocity.linear.x, max_dvx), -result.vx_max, result.vx_max);
        const double vy = std::clamp(
          clampDelta(candidate.vy, robot_velocity.linear.y, max_dvy), -result.vy_max, result.vy_max);
        const double omega = std::clamp(
          clampDelta(candidate.omega, robot_velocity.angular.z, max_domega),
          -result.omega_max,
          result.omega_max);

        if (std::hypot(vx, vy) < params_.min_speed_xy && std::abs(omega) < 0.03) {
          continue;
        }

        double x = x0;
        double y = y0;
        double yaw = yaw0;
        double score =
          params_.rd_vx * std::pow(vx - last_command_.linear.x, 2.0) +
          params_.rd_vy * std::pow(vy - last_command_.linear.y, 2.0) +
          params_.rd_omega * std::pow(omega - last_command_.angular.z, 2.0);
        bool collision = false;
        nav_msgs::msg::Path rollout;
        rollout.header = result.predicted_trajectory.header;
        rollout.poses.push_back(makePose(rollout.header.frame_id, robot_pose.header.stamp, x, y, yaw));

        for (int i = 1; i <= params_.horizon_steps; ++i) {
          rolloutMecanumStep(x, y, yaw, vx, vy, omega, params_.sample_time);

          const auto & ref = reference[std::min(static_cast<size_t>(i), reference.size() - 1)];
          const double predicted_robot_vx_world = vx * std::cos(yaw) - vy * std::sin(yaw);
          const double predicted_robot_vy_world = vx * std::sin(yaw) + vy * std::cos(yaw);
          const auto step_context = human_predictor_.predictHumanContext(
            humans,
            x,
            y,
            predicted_robot_vx_world,
            predicted_robot_vy_world,
            i * params_.sample_time);
          const double step_phi = step_context.phi_h;
          const ContextState step_adaptive =
            mergeExternalLimits(computeAdaptiveParameters(step_phi), context);
          score += scoreTrackingError(x, y, yaw, ref, step_phi);
          score += params_.r_vx * vx * vx + params_.r_vy * vy * vy + params_.r_omega * omega * omega;
          bool velocity_constraint_violated = false;
          const double velocity_constraint_penalty = adaptiveVelocityConstraintPenalty(
            vx, vy, omega, step_adaptive, velocity_constraint_violated);
          if (velocity_constraint_violated) {
            collision = true;
            result.collision = true;
            score += velocity_constraint_penalty;
            break;
          }

          const double map_cost = occupancyCost(costmap, x, y);
          if (map_cost >= params_.costmap_collision_threshold) {
            collision = true;
            result.collision = true;
            score += params_.w_slack;
            break;
          }
          score += params_.costmap_cost_weight * map_cost;

          if (step_context.valid && std::isfinite(step_context.distance)) {
            if (step_context.distance < step_adaptive.d_safe) {
              collision = true;
              result.collision = true;
              score += params_.w_slack *
                std::pow(step_adaptive.d_safe - step_context.distance, 2.0);
              break;
            }
            score += params_.human_cost_weight * step_phi *
              std::pow(std::max(0.0, params_.d0 - step_context.distance), 2.0);
          }

          rollout.poses.push_back(makePose(rollout.header.frame_id, robot_pose.header.stamp, x, y, yaw));
        }

        const auto & terminal = reference.back();
        score += params_.terminal_cost_weight *
          std::hypot(x - terminal.pose.position.x, y - terminal.pose.position.y);

        if (!collision && std::isfinite(score) && score < best_score) {
          best_score = score;
          best_command.linear.x = vx;
          best_command.linear.y = vy;
          best_command.angular.z = omega;
          best_path = rollout;
        }
  }

  if (std::isfinite(best_score)) {
    result.command = best_command;
    result.predicted_trajectory = best_path;
    result.status = result.timeout ?
      "SAMPLING_BASED_CCA_PREDICTIVE_CONTROL_TIMEOUT_OK" :
      "SAMPLING_BASED_CCA_PREDICTIVE_CONTROL_OK";
    last_command_ = best_command;
  } else {
    result.status = result.timeout ? "TIMEOUT_NO_VALID_TRAJECTORY" : "NO_VALID_TRAJECTORY";
    last_command_ = geometry_msgs::msg::Twist();
  }

  result.solve_time_ms =
    std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
  return result;
}

}  
