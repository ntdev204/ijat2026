#include <gtest/gtest.h>
#include "rai_canmpc_controller/solver_wrapper.hpp"
#include <ament_index_cpp/get_package_prefix.hpp>
#include <cmath>
#include <vector>
#include <string>
#include <filesystem>
#include <iostream>

using rai_canmpc_controller::SolverWrapper;
using rai_canmpc_controller::SolveInput;
using rai_canmpc_controller::SolveOutput;

// Helper function to locate the compiled canmpc_solver shared library cross-platform
std::string locateSolverLibrary()
{
  std::string package_prefix;
  try {
    package_prefix = ament_index_cpp::get_package_prefix("ca_nmpc_controller");
  } catch (const std::exception & e) {
    std::cout << "[SolverWrapperTest] Could not get package prefix: " << e.what() << std::endl;
    return "";
  }

  std::vector<std::string> search_paths = {
    package_prefix + "/lib/libcanmpc_solver.so",
    package_prefix + "/bin/canmpc_solver.dll",
    package_prefix + "/lib/canmpc_solver.dll",
    package_prefix + "/bin/libcanmpc_solver.dll",
    package_prefix + "/lib/libcanmpc_solver.dll",
    package_prefix + "/lib/libcanmpc_solver.dylib"
  };

  for (const auto & path : search_paths) {
    if (std::filesystem::exists(path)) {
      std::cout << "[SolverWrapperTest] Found solver library at: " << path << std::endl;
      return path;
    }
  }

  // Fallback to default linux path
  return package_prefix + "/lib/libcanmpc_solver.so";
}

TEST(SolverWrapperTest, InitializationAndReset)
{
  int N = 20;
  int num_humans = 3;
  SolverWrapper wrapper(N, num_humans);

  // Wrapper shouldn't be initialized before loading the library
  SolveInput input;
  input.x_init = {0.0, 0.0, 0.0};
  input.x_ref.assign(3 * (N + 1), 0.0);
  input.u_ref.assign(3 * N, 0.0);
  input.human_data.assign(4 * num_humans, 999.0);
  input.params.assign(23, 0.0);

  SolveOutput output = wrapper.solve(input);
  EXPECT_FALSE(output.success);
  EXPECT_EQ(output.status, "NOT_INITIALIZED");
}

TEST(SolverWrapperTest, InvalidLibraryPath)
{
  SolverWrapper wrapper(20, 3);
  bool success = wrapper.init("/invalid/path/to/libcanmpc_solver.so");
  EXPECT_FALSE(success);
}

TEST(SolverWrapperTest, DummySolveRun)
{
  int N = 20;
  int num_humans = 3;
  SolverWrapper wrapper(N, num_humans);

  std::string lib_path = locateSolverLibrary();
  if (lib_path.empty() || !std::filesystem::exists(lib_path)) {
    std::cout << "[SolverWrapperTest] [WARNING] Compiled solver library not found. Skipping dummy run execution test." << std::endl;
    return;
  }

  bool success = wrapper.init(lib_path);
  ASSERT_TRUE(success);

  // Set up inputs for a dummy solver execution run
  SolveInput input;
  input.x_init = {0.0, 0.0, 0.0}; // [x, y, yaw]
  
  // Set reference state trajectory (N+1 states, initialized to 0.0)
  input.x_ref.assign(3 * (N + 1), 0.0);
  
  // Set reference controls (N control inputs, set v_ref = 0.5)
  input.u_ref.assign(3 * N, 0.0);
  for (int i = 0; i < N; ++i) {
    input.u_ref[3 * i] = 0.5;
  }

  // Populate human data (3 humans * 4 variables = 12 elements, set far away to prevent collisions)
  input.human_data.assign(4 * num_humans, 999.0);

  // Populate 23 scalar hyperparameters matching the controller configuration
  input.params = {
    0.05,       // model_dt
    3.0,        // beta
    1.5,        // d0
    0.5,        // d_safe_0
    1.2,        // d_safe_max
    0.5,        // v_ref
    1.0,        // max_vx
    0.1,        // v_max_min
    1.0,        // max_vy
    0.1,        // v_y_max_min
    1.0,        // max_omega
    0.2,        // omega_max_min
    10.0,       // q_x
    10.0,       // q_y
    5.0,        // q_theta
    0.1,        // r_vx
    0.1,        // r_vy
    0.05,       // r_omega
    1.0,        // rd_vx
    1.0,        // rd_vy
    0.5,        // rd_omega
    0.1,        // q_active_factor
    100000.0    // w_slack
  };

  // Run optimization solve
  SolveOutput output = wrapper.solve(input);

  // Verify result correctness
  EXPECT_TRUE(output.success);
  EXPECT_EQ(output.status, "SUCCESS");
  EXPECT_GT(output.solve_time_ms, 0.0);
  EXPECT_EQ(output.x_opt.size(), 3 * (N + 1));
  EXPECT_EQ(output.u_opt.size(), 3 * N);
  EXPECT_EQ(output.s_opt.size(), num_humans * N);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

