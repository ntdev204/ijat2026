#include "cca_nmpc_controller/mecanum_model.hpp"

namespace cca_nmpc_controller
{

double MecanumModel::normalizeAngle(double angle)
{
  while (angle > M_PI) {
    angle -= 2.0 * M_PI;
  }
  while (angle < -M_PI) {
    angle += 2.0 * M_PI;
  }
  return angle;
}

MecanumModel::State MecanumModel::kinematics(const State & state, const Control & control)
{
  double yaw = state[2];
  double vx = control[0];
  double vy = control[1];
  double omega = control[2];

  double dx = vx * std::cos(yaw) - vy * std::sin(yaw);
  double dy = vx * std::sin(yaw) + vy * std::cos(yaw);
  double dyaw = omega;

  return {dx, dy, dyaw};
}

MecanumModel::State MecanumModel::predictRK4(const State & state, const Control & control, double dt)
{
  // RK4 Integration:
  // k1 = f(x, u)
  // k2 = f(x + dt/2 * k1, u)
  // k3 = f(x + dt/2 * k2, u)
  // k4 = f(x + dt * k3, u)
  // x_next = x + dt/6 * (k1 + 2*k2 + 2*k3 + k4)

  State k1 = kinematics(state, control);

  State state_k2;
  state_k2[0] = state[0] + 0.5 * dt * k1[0];
  state_k2[1] = state[1] + 0.5 * dt * k1[1];
  state_k2[2] = state[2] + 0.5 * dt * k1[2];
  State k2 = kinematics(state_k2, control);

  State state_k3;
  state_k3[0] = state[0] + 0.5 * dt * k2[0];
  state_k3[1] = state[1] + 0.5 * dt * k2[1];
  state_k3[2] = state[2] + 0.5 * dt * k2[2];
  State k3 = kinematics(state_k3, control);

  State state_k4;
  state_k4[0] = state[0] + dt * k3[0];
  state_k4[1] = state[1] + dt * k3[1];
  state_k4[2] = state[2] + dt * k3[2];
  State k4 = kinematics(state_k4, control);

  State next_state;
  next_state[0] = state[0] + (dt / 6.0) * (k1[0] + 2.0 * k2[0] + 2.0 * k3[0] + k4[0]);
  next_state[1] = state[1] + (dt / 6.0) * (k1[1] + 2.0 * k2[1] + 2.0 * k3[1] + k4[1]);
  next_state[2] = normalizeAngle(state[2] + (dt / 6.0) * (k1[2] + 2.0 * k2[2] + 2.0 * k3[2] + k4[2]));

  return next_state;
}

}  // namespace cca_nmpc_controller
