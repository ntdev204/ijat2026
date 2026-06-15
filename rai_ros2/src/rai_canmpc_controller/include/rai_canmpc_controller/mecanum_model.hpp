#ifndef NAV2_CANMPC_CONTROLLER__MECANUM_MODEL_HPP_
#define NAV2_CANMPC_CONTROLLER__MECANUM_MODEL_HPP_

#include <array>
#include <cmath>

namespace rai_canmpc_controller
{

/**
 * @class MecanumModel
 * @brief Represents the kinematics of a 3-DOF Mecanum mobile robot, using Runge-Kutta 4th Order (RK4) integration.
 */
class MecanumModel
{
public:
  using State = std::array<double, 3>;    // [x, y, yaw]
  using Control = std::array<double, 3>;  // [vx, vy, omega]

  MecanumModel() = default;
  ~MecanumModel() = default;

  /**
   * @brief Integrates the system state forward by dt using Runge-Kutta 4 (RK4)
   * @param state Current state [x, y, yaw]
   * @param control Applied control input [vx, vy, omega]
   * @param dt Time step (seconds)
   * @return Next integrated state [x, y, yaw]
   */
  static State predictRK4(const State & state, const Control & control, double dt);

  /**
   * @brief Computes the continuous-time derivative f(x, u)
   * @param state Current state [x, y, yaw]
   * @param control Applied control input [vx, vy, omega]
   * @return State derivative [dx, dy, dyaw]
   */
  static State kinematics(const State & state, const Control & control);

  /**
   * @brief Utility function to normalize angle in [-pi, pi]
   * @param angle Input angle in radians
   * @return Normalized angle
   */
  static double normalizeAngle(double angle);
};

}  // namespace rai_canmpc_controller

#endif  // NAV2_CANMPC_CONTROLLER__MECANUM_MODEL_HPP_
