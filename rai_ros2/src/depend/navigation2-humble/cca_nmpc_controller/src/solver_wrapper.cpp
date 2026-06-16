#include "cca_nmpc_controller/solver_wrapper.hpp"
#include <chrono>
#include <iostream>

namespace cca_nmpc_controller
{

SolverWrapper::SolverWrapper(int horizon_steps, int num_humans)
: horizon_steps_(horizon_steps), num_humans_(num_humans)
{
  // Total states in horizon: N + 1
  // Total controls in horizon: N
  // Total slacks in horizon: num_humans * N
  int n_states = 3 * (horizon_steps_ + 1);
  int n_controls = 3 * horizon_steps_;
  int n_slacks = num_humans_ * horizon_steps_;

  n_vars_ = n_states + n_controls + n_slacks;

  // Parameters:
  // - X_init: 3
  // - X_ref: 3 * (N + 1)
  // - U_ref: 3 * N
  // - H_data: 4 * num_humans
  // - Params: 23
  n_params_ = 3 + n_states + n_controls + (4 * num_humans_) + 23;

  // Constraints:
  // - Initial state: 3
  // - Dynamics: 3 * N
  // - Safety: num_humans * (N - 1)
  // - Velocity limits: 6 * N
  n_constraints_ = 3 + (3 * horizon_steps_) + (num_humans_ * (horizon_steps_ - 1)) + (6 * horizon_steps_);

  resetCache();
}

bool SolverWrapper::init(const std::string & lib_path)
{
  std::lock_guard<std::mutex> lock(solver_mutex_);
  try {
    std::cout << "[SolverWrapper] Loading external solver library: " << lib_path << std::endl;
    // Load the code-generated solver function from the shared library
    solver_ = casadi::external("canmpc_solver", lib_path);
    initialized_ = true;
    return true;
  } catch (const std::exception & e) {
    std::cerr << "[SolverWrapper] Failed to load solver: " << e.what() << std::endl;
    initialized_ = false;
    return false;
  }
}

void SolverWrapper::resetCache()
{
  cached_x_.assign(3 * (horizon_steps_ + 1), 0.0);
  cached_u_.assign(3 * horizon_steps_, 0.0);
  cached_s_.assign(num_humans_ * horizon_steps_, 0.0);
}

void SolverWrapper::getWarmStartGuess(std::vector<double> & x0)
{
  x0.clear();
  x0.reserve(n_vars_);

  // Shift state cache forward, duplicating the last state
  std::vector<double> warm_x(3 * (horizon_steps_ + 1));
  for (int i = 0; i < horizon_steps_; ++i) {
    warm_x[3 * i] = cached_x_[3 * (i + 1)];
    warm_x[3 * i + 1] = cached_x_[3 * (i + 1) + 1];
    warm_x[3 * i + 2] = cached_x_[3 * (i + 1) + 2];
  }
  warm_x[3 * horizon_steps_] = cached_x_[3 * horizon_steps_];
  warm_x[3 * horizon_steps_ + 1] = cached_x_[3 * horizon_steps_ + 1];
  warm_x[3 * horizon_steps_ + 2] = cached_x_[3 * horizon_steps_ + 2];

  // Shift control cache forward, duplicating the last control
  std::vector<double> warm_u(3 * horizon_steps_);
  for (int i = 0; i < horizon_steps_ - 1; ++i) {
    warm_u[3 * i] = cached_u_[3 * (i + 1)];
    warm_u[3 * i + 1] = cached_u_[3 * (i + 1) + 1];
    warm_u[3 * i + 2] = cached_u_[3 * (i + 1) + 2];
  }
  warm_u[3 * (horizon_steps_ - 1)] = cached_u_[3 * (horizon_steps_ - 1)];
  warm_u[3 * (horizon_steps_ - 1) + 1] = cached_u_[3 * (horizon_steps_ - 1) + 1];
  warm_u[3 * (horizon_steps_ - 1) + 2] = cached_u_[3 * (horizon_steps_ - 1) + 2];

  // Shift slack cache
  std::vector<double> warm_s(num_humans_ * horizon_steps_);
  for (int j = 0; j < num_humans_; ++j) {
    for (int i = 0; i < horizon_steps_ - 1; ++i) {
      warm_s[j * horizon_steps_ + i] = cached_s_[j * horizon_steps_ + i + 1];
    }
    warm_s[j * horizon_steps_ + horizon_steps_ - 1] = cached_s_[j * horizon_steps_ + horizon_steps_ - 1];
  }

  // Concatenate to build the initial guess x0
  x0.insert(x0.end(), warm_x.begin(), warm_x.end());
  x0.insert(x0.end(), warm_u.begin(), warm_u.end());
  x0.insert(x0.end(), warm_s.begin(), warm_s.end());
}

void SolverWrapper::updateCache(
  const std::vector<double> & x_opt,
  const std::vector<double> & u_opt,
  const std::vector<double> & s_opt)
{
  cached_x_ = x_opt;
  cached_u_ = u_opt;
  cached_s_ = s_opt;
}

SolveOutput SolverWrapper::solve(const SolveInput & input)
{
  std::lock_guard<std::mutex> lock(solver_mutex_);
  SolveOutput output;

  if (!initialized_) {
    output.success = false;
    output.status = "NOT_INITIALIZED";
    return output;
  }

  auto start_time = std::chrono::high_resolution_clock::now();

  // 1. Prepare initial guess x0 (warm start)
  std::vector<double> x0;
  getWarmStartGuess(x0);

  // 2. Prepare parameter vector p
  std::vector<double> p;
  p.reserve(n_params_);
  p.insert(p.end(), input.x_init.begin(), input.x_init.end());
  p.insert(p.end(), input.x_ref.begin(), input.x_ref.end());
  p.insert(p.end(), input.u_ref.begin(), input.u_ref.end());
  p.insert(p.end(), input.human_data.begin(), input.human_data.end());
  p.insert(p.end(), input.params.begin(), input.params.end());

  // 3. Prepare decision variable bounds lbx and ubx
  std::vector<double> lbx(n_vars_, -1e19);
  std::vector<double> ubx(n_vars_, 1e19);

  // Enforce S >= 0 on slack variables (which occupy the last part of decision variables)
  int state_size = 3 * (horizon_steps_ + 1);
  int control_size = 3 * horizon_steps_;
  for (int i = 0; i < num_humans_ * horizon_steps_; ++i) {
    lbx[state_size + control_size + i] = 0.0;
  }

  // 4. Prepare constraint bounds lbg and ubg
  std::vector<double> lbg(n_constraints_, 0.0);
  std::vector<double> ubg(n_constraints_, 0.0);

  // - Initial state constraint (first 3): lbg = 0, ubg = 0
  // - Dynamic integration constraints (next 3 * N = 60): lbg = 0, ubg = 0
  // - Safety distance constraints (next num_humans * (N - 1) = 57): lbg = 0, ubg = 1e19
  // - Velocity limits constraints (next 6 * N = 120): lbg = 0, ubg = 1e19
  int eq_constraints = 3 + (3 * horizon_steps_);
  for (int i = eq_constraints; i < n_constraints_; ++i) {
    lbg[i] = 0.0;
    ubg[i] = 1e19;
  }

  // 5. Invoke solver
  try {
    std::map<std::string, casadi::DM> arg;
    arg["x0"] = x0;
    arg["p"] = p;
    arg["lbx"] = lbx;
    arg["ubx"] = ubx;
    arg["lbg"] = lbg;
    arg["ubg"] = ubg;

    std::map<std::string, casadi::DM> res = solver_(arg);

    // Extract optimization results
    std::vector<double> x_all = std::vector<double>(res.at("x"));
    
    // Split x_all back into states, controls, and slacks
    output.x_opt.assign(x_all.begin(), x_all.begin() + state_size);
    output.u_opt.assign(x_all.begin() + state_size, x_all.begin() + state_size + control_size);
    output.s_opt.assign(x_all.begin() + state_size + control_size, x_all.end());

    // Update caches for warm-starting
    updateCache(output.x_opt, output.u_opt, output.s_opt);

    // Set output stats
    output.success = true;
    output.status = "SUCCESS";
    output.iter_count = 1; // solver doesn't expose internal SQP iterations cleanly without verbosity
  } catch (const std::exception & e) {
    output.success = false;
    output.status = std::string("SOLVER_ERROR: ") + e.what();
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  output.solve_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

  return output;
}

}  // namespace cca_nmpc_controller
