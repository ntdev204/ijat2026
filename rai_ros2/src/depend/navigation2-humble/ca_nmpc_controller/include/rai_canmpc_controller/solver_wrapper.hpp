#ifndef NAV2_CANMPC_CONTROLLER__SOLVER_WRAPPER_HPP_
#define NAV2_CANMPC_CONTROLLER__SOLVER_WRAPPER_HPP_

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <casadi/casadi.hpp>

namespace rai_canmpc_controller
{

/**
 * @struct SolveInput
 * @brief Inputs for the MPC optimization solver.
 */
struct SolveInput
{
  std::vector<double> x_init;       // Current robot state [x, y, yaw] (size 3)
  std::vector<double> x_ref;        // Reference state trajectory [x0, y0, yaw0, ...] (size 3 * (N + 1))
  std::vector<double> u_ref;        // Reference control trajectory [vx0, vy0, omega0, ...] (size 3 * N)
  std::vector<double> human_data;   // Human states [x, y, vx, vy, ...] (size 4 * N_humans)
  std::vector<double> params;       // Scalar parameters (size 23)
};

/**
 * @struct SolveOutput
 * @brief Outputs from the MPC optimization solver.
 */
struct SolveOutput
{
  bool success{false};
  double solve_time_ms{0.0};
  int iter_count{0};
  std::string status;
  std::vector<double> x_opt;        // Optimal state trajectory (size 3 * (N + 1))
  std::vector<double> u_opt;        // Optimal control inputs (size 3 * N)
  std::vector<double> s_opt;        // Optimal slack variables (size N_humans * N)
};

/**
 * @class SolverWrapper
 * @brief C++ wrapper around the code-generated CasADi solver.
 */
class SolverWrapper
{
public:
  SolverWrapper(int horizon_steps, int num_humans);
  ~SolverWrapper() = default;

  /**
   * @brief Initializes the solver by loading the compiled shared library.
   * @param lib_path Path to the compiled solver .so file.
   * @return True if successful.
   */
  bool init(const std::string & lib_path);

  /**
   * @brief Solves the MPC optimization problem.
   * @param input Solver inputs.
   * @return Solver outputs.
   */
  SolveOutput solve(const SolveInput & input);

  /**
   * @brief Resets the cached trajectories (useful on replanning or activation).
   */
  void resetCache();

private:
  int horizon_steps_;
  int num_humans_;
  int n_vars_;
  int n_params_;
  int n_constraints_;

  casadi::Function solver_;
  bool initialized_{false};
  std::mutex solver_mutex_;

  // Caching for warm starting
  std::vector<double> cached_x_;
  std::vector<double> cached_u_;
  std::vector<double> cached_s_;

  /**
   * @brief Updates the initial guess (warm start) using cached trajectories.
   * @param x0 The vector to be populated with the warm-start guess.
   */
  void getWarmStartGuess(std::vector<double> & x0);

  /**
   * @brief Caches the optimal solution for the next solve iteration.
   * @param x_opt Optimal states.
   * @param u_opt Optimal controls.
   * @param s_opt Optimal slacks.
   */
  void updateCache(
    const std::vector<double> & x_opt,
    const std::vector<double> & u_opt,
    const std::vector<double> & s_opt);
};

}  // namespace rai_canmpc_controller

#endif  // NAV2_CANMPC_CONTROLLER__SOLVER_WRAPPER_HPP_
