#include <gtest/gtest.h>
#include "cca_nmpc_controller/mecanum_model.hpp"
#include <cmath>
#include <limits>

using cca_nmpc_controller::MecanumModel;

// Helper function to calculate exact analytical state integration
MecanumModel::State analyticalSolution(
  const MecanumModel::State & state,
  const MecanumModel::Control & control,
  double dt)
{
  double x0 = state[0];
  double y0 = state[1];
  double theta0 = state[2];
  double vx = control[0];
  double vy = control[1];
  double omega = control[2];

  if (std::abs(omega) < 1e-9) {
    double dx = vx * std::cos(theta0) - vy * std::sin(theta0);
    double dy = vx * std::sin(theta0) + vy * std::cos(theta0);
    return {x0 + dx * dt, y0 + dy * dt, MecanumModel::normalizeAngle(theta0)};
  } else {
    double theta_next = theta0 + omega * dt;
    double x_next = x0 + (vx / omega) * (std::sin(theta_next) - std::sin(theta0)) +
      (vy / omega) * (std::cos(theta_next) - std::cos(theta0));
    double y_next = y0 - (vx / omega) * (std::cos(theta_next) - std::cos(theta0)) +
      (vy / omega) * (std::sin(theta_next) - std::sin(theta0));
    return {x_next, y_next, MecanumModel::normalizeAngle(theta_next)};
  }
}

TEST(MecanumModelTest, NormalizeAngle)
{
  // Test already normalized angles
  EXPECT_NEAR(MecanumModel::normalizeAngle(0.0), 0.0, 1e-6);
  EXPECT_NEAR(MecanumModel::normalizeAngle(1.5), 1.5, 1e-6);
  EXPECT_NEAR(MecanumModel::normalizeAngle(-2.0), -2.0, 1e-6);

  // Test wrapping of positive angles
  EXPECT_NEAR(MecanumModel::normalizeAngle(M_PI + 0.1), -M_PI + 0.1, 1e-6);
  EXPECT_NEAR(MecanumModel::normalizeAngle(3.0 * M_PI), -M_PI, 1e-6);
  EXPECT_NEAR(MecanumModel::normalizeAngle(100.0 * M_PI + 0.5), 0.5, 1e-6);

  // Test wrapping of negative angles
  EXPECT_NEAR(MecanumModel::normalizeAngle(-M_PI - 0.1), M_PI - 0.1, 1e-6);
  EXPECT_NEAR(MecanumModel::normalizeAngle(-3.0 * M_PI), M_PI, 1e-6);
  EXPECT_NEAR(MecanumModel::normalizeAngle(-100.0 * M_PI - 0.5), -0.5, 1e-6);

  // Test boundaries
  EXPECT_NEAR(MecanumModel::normalizeAngle(M_PI), M_PI, 1e-6);
  EXPECT_NEAR(MecanumModel::normalizeAngle(-M_PI), -M_PI, 1e-6);
}

TEST(MecanumModelTest, Kinematics)
{
  // Test pure forward motion (yaw = 0)
  MecanumModel::State s1 = {0.0, 0.0, 0.0};
  MecanumModel::Control u1 = {1.0, 0.0, 0.0};
  MecanumModel::State ds1 = MecanumModel::kinematics(s1, u1);
  EXPECT_NEAR(ds1[0], 1.0, 1e-6);
  EXPECT_NEAR(ds1[1], 0.0, 1e-6);
  EXPECT_NEAR(ds1[2], 0.0, 1e-6);

  // Test pure lateral motion (yaw = pi/2)
  MecanumModel::State s2 = {0.0, 0.0, M_PI_2};
  MecanumModel::Control u2 = {1.0, 0.0, 0.0};
  MecanumModel::State ds2 = MecanumModel::kinematics(s2, u2);
  EXPECT_NEAR(ds2[0], 0.0, 1e-6);
  EXPECT_NEAR(ds2[1], 1.0, 1e-6);
  EXPECT_NEAR(ds2[2], 0.0, 1e-6);

  // Test pure rotation (yaw = 0, omega = 0.5)
  MecanumModel::State s3 = {1.0, -1.0, 0.0};
  MecanumModel::Control u3 = {0.0, 0.0, 0.5};
  MecanumModel::State ds3 = MecanumModel::kinematics(s3, u3);
  EXPECT_NEAR(ds3[0], 0.0, 1e-6);
  EXPECT_NEAR(ds3[1], 0.0, 1e-6);
  EXPECT_NEAR(ds3[2], 0.5, 1e-6);

  // Test combined kinematics inputs
  MecanumModel::State s4 = {2.0, 3.0, M_PI_4};
  MecanumModel::Control u4 = {1.5, -0.5, 0.2};
  MecanumModel::State ds4 = MecanumModel::kinematics(s4, u4);
  // dx = vx*cos(yaw) - vy*sin(yaw) = 1.5*cos(pi/4) - (-0.5)*sin(pi/4) = 2.0*sin(pi/4) = sqrt(2)
  // dy = vx*sin(yaw) + vy*cos(yaw) = 1.5*sin(pi/4) + (-0.5)*cos(pi/4) = 1.0*cos(pi/4) = sqrt(2)/2
  EXPECT_NEAR(ds4[0], std::sqrt(2.0), 1e-6);
  EXPECT_NEAR(ds4[1], std::sqrt(2.0) / 2.0, 1e-6);
  EXPECT_NEAR(ds4[2], 0.2, 1e-6);
}

TEST(MecanumModelTest, PredictRK4_PureForward)
{
  double dt = 0.1;
  MecanumModel::Control control = {1.0, 0.0, 0.0};

  // Case 1: Start at origin with heading 0
  MecanumModel::State s1 = {0.0, 0.0, 0.0};
  MecanumModel::State next_s1 = MecanumModel::predictRK4(s1, control, dt);
  MecanumModel::State analytical_s1 = analyticalSolution(s1, control, dt);

  EXPECT_NEAR(next_s1[0], analytical_s1[0], 1e-9);
  EXPECT_NEAR(next_s1[1], analytical_s1[1], 1e-9);
  EXPECT_NEAR(next_s1[2], analytical_s1[2], 1e-9);
  EXPECT_NEAR(next_s1[0], 0.1, 1e-9);

  // Case 2: Start at non-zero state with heading pi/6
  MecanumModel::State s2 = {1.0, 2.0, M_PI / 6.0};
  MecanumModel::State next_s2 = MecanumModel::predictRK4(s2, control, dt);
  MecanumModel::State analytical_s2 = analyticalSolution(s2, control, dt);

  EXPECT_NEAR(next_s2[0], analytical_s2[0], 1e-9);
  EXPECT_NEAR(next_s2[1], analytical_s2[1], 1e-9);
  EXPECT_NEAR(next_s2[2], analytical_s2[2], 1e-9);
}

TEST(MecanumModelTest, PredictRK4_PureLateral)
{
  double dt = 0.1;
  MecanumModel::Control control = {0.0, 1.0, 0.0};

  // Case 1: Start at origin with heading 0
  MecanumModel::State s1 = {0.0, 0.0, 0.0};
  MecanumModel::State next_s1 = MecanumModel::predictRK4(s1, control, dt);
  MecanumModel::State analytical_s1 = analyticalSolution(s1, control, dt);

  EXPECT_NEAR(next_s1[0], analytical_s1[0], 1e-9);
  EXPECT_NEAR(next_s1[1], analytical_s1[1], 1e-9);
  EXPECT_NEAR(next_s1[2], analytical_s1[2], 1e-9);
  EXPECT_NEAR(next_s1[1], 0.1, 1e-9);

  // Case 2: Start at non-zero state with heading pi/6
  MecanumModel::State s2 = {1.0, 2.0, M_PI / 6.0};
  MecanumModel::State next_s2 = MecanumModel::predictRK4(s2, control, dt);
  MecanumModel::State analytical_s2 = analyticalSolution(s2, control, dt);

  EXPECT_NEAR(next_s2[0], analytical_s2[0], 1e-9);
  EXPECT_NEAR(next_s2[1], analytical_s2[1], 1e-9);
  EXPECT_NEAR(next_s2[2], analytical_s2[2], 1e-9);
}

TEST(MecanumModelTest, PredictRK4_PureRotation)
{
  double dt = 0.2;
  MecanumModel::Control control = {0.0, 0.0, 0.5};

  // Case 1: Start at origin with heading 0
  MecanumModel::State s1 = {0.0, 0.0, 0.0};
  MecanumModel::State next_s1 = MecanumModel::predictRK4(s1, control, dt);
  MecanumModel::State analytical_s1 = analyticalSolution(s1, control, dt);

  EXPECT_NEAR(next_s1[0], analytical_s1[0], 1e-9);
  EXPECT_NEAR(next_s1[1], analytical_s1[1], 1e-9);
  EXPECT_NEAR(next_s1[2], analytical_s1[2], 1e-9);
  EXPECT_NEAR(next_s1[2], 0.1, 1e-9);

  // Case 2: Start at non-zero state with heading pi/4
  MecanumModel::State s2 = {1.0, 2.0, M_PI_4};
  MecanumModel::State next_s2 = MecanumModel::predictRK4(s2, control, dt);
  MecanumModel::State analytical_s2 = analyticalSolution(s2, control, dt);

  EXPECT_NEAR(next_s2[0], analytical_s2[0], 1e-9);
  EXPECT_NEAR(next_s2[1], analytical_s2[1], 1e-9);
  EXPECT_NEAR(next_s2[2], analytical_s2[2], 1e-9);
}

TEST(MecanumModelTest, PredictRK4_CombinedInputs)
{
  // Test multiple combined inputs with rotation to verify RK4 matches analytical solution
  std::vector<MecanumModel::State> states = {
    {0.0, 0.0, 0.0},
    {1.0, -2.0, M_PI_4},
    {-5.0, 10.0, -M_PI_2}
  };

  std::vector<MecanumModel::Control> controls = {
    {1.0, 0.5, 0.1},
    {2.0, -1.0, -0.3},
    {-0.5, 1.2, 0.5}
  };

  std::vector<double> dts = {0.05, 0.1, 0.2};

  for (const auto & s : states) {
    for (const auto & u : controls) {
      for (double dt : dts) {
        MecanumModel::State next_rk4 = MecanumModel::predictRK4(s, u, dt);
        MecanumModel::State next_analytical = analyticalSolution(s, u, dt);

        // RK4 error is O(dt^5), so it should be extremely close to the analytical solution
        EXPECT_NEAR(next_rk4[0], next_analytical[0], 1e-6);
        EXPECT_NEAR(next_rk4[1], next_analytical[1], 1e-6);
        EXPECT_NEAR(next_rk4[2], next_analytical[2], 1e-6);
      }
    }
  }
}

TEST(MecanumModelTest, PredictRK4_EdgeCases)
{
  // Zero time step
  MecanumModel::State s = {1.0, 2.0, 0.5};
  MecanumModel::Control u = {1.0, 2.0, 3.0};
  MecanumModel::State next_s_zero = MecanumModel::predictRK4(s, u, 0.0);
  EXPECT_DOUBLE_EQ(next_s_zero[0], s[0]);
  EXPECT_DOUBLE_EQ(next_s_zero[1], s[1]);
  EXPECT_DOUBLE_EQ(next_s_zero[2], s[2]);

  // Negative time step (integrating backward)
  double dt_neg = -0.1;
  MecanumModel::State next_s_neg = MecanumModel::predictRK4(s, u, dt_neg);
  MecanumModel::State analytical_s_neg = analyticalSolution(s, u, dt_neg);
  EXPECT_NEAR(next_s_neg[0], analytical_s_neg[0], 1e-6);
  EXPECT_NEAR(next_s_neg[1], analytical_s_neg[1], 1e-6);
  EXPECT_NEAR(next_s_neg[2], analytical_s_neg[2], 1e-6);

  // NaN input handling: should propagate NaN and not crash or loop
  double nan_val = std::numeric_limits<double>::quiet_NaN();
  
  MecanumModel::State s_nan = {nan_val, 0.0, 0.0};
  MecanumModel::State next_s_nan1 = MecanumModel::predictRK4(s_nan, u, 0.1);
  EXPECT_TRUE(std::isnan(next_s_nan1[0]));

  MecanumModel::Control u_nan = {1.0, nan_val, 0.0};
  MecanumModel::State next_s_nan2 = MecanumModel::predictRK4(s, u_nan, 0.1);
  EXPECT_TRUE(std::isnan(next_s_nan2[1]));

  MecanumModel::State next_s_nan3 = MecanumModel::predictRK4(s, u, nan_val);
  EXPECT_TRUE(std::isnan(next_s_nan3[0]));
  EXPECT_TRUE(std::isnan(next_s_nan3[1]));
  EXPECT_TRUE(std::isnan(next_s_nan3[2]));
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
