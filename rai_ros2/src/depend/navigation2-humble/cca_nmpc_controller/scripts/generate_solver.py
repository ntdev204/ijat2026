#!/usr/bin/env python3
import os
import sys
import casadi as ca

def main():
    # Horizon and dimensions
    N = 20          # prediction steps
    N_humans = 3    # support up to 3 humans

    print(f"Generating NMPC solver C code with N={N}, N_humans={N_humans}...")

    # Decision variables
    # 1. States: X = [x_0, y_0, theta_0, ..., x_N, y_N, theta_N] (size 3 * (N + 1))
    X = ca.MX.sym('X', 3 * (N + 1))
    # 2. Controls: U = [vx_0, vy_0, omega_0, ..., vx_{N-1}, vy_{N-1}, omega_{N-1}] (size 3 * N)
    U = ca.MX.sym('U', 3 * N)
    # 3. Slacks: S = [s_{j, i}] for j=0..N_humans-1, i=1..N (size N_humans * N)
    S = ca.MX.sym('S', N_humans * N)

    # Flatten decision variables vector
    vars_list = [X, U, S]
    vars_all = ca.vertcat(*vars_list)

    # Parameter vector structure:
    # - X_init (3): current robot pose [x_init, y_init, theta_init]
    # - X_ref (3 * (N + 1)): reference pose trajectory
    # - U_ref (3 * N): reference control trajectory
    # - Human pos/vel (4 * N_humans): [x_h, y_h, vx_h, vy_h] for each human
    # - Scalar parameters (23):
    #   0: dt
    #   1: beta
    #   2: d0
    #   3: d_safe_0
    #   4: d_safe_max
    #   5: v_ref
    #   6: v_max_0
    #   7: v_max_min
    #   8: v_y_max_0
    #   9: v_y_max_min
    #   10: omega_max_0
    #   11: omega_max_min
    #   12: q_x
    #   13: q_y
    #   14: q_theta
    #   15: r_vx
    #   16: r_vy
    #   17: r_omega
    #   18: rd_vx
    #   19: rd_vy
    #   20: rd_omega
    #   21: q_active_factor
    #   22: w_slack

    X_init = ca.MX.sym('X_init', 3)
    X_ref = ca.MX.sym('X_ref', 3 * (N + 1))
    U_ref = ca.MX.sym('U_ref', 3 * N)
    H_data = ca.MX.sym('H_data', 4 * N_humans)
    Params = ca.MX.sym('Params', 23)

    p_all = ca.vertcat(X_init, X_ref, U_ref, H_data, Params)

    # Extract scalar parameters
    dt = Params[0]
    beta = Params[1]
    d0 = Params[2]
    d_safe_0 = Params[3]
    d_safe_max = Params[4]
    v_ref = Params[5]
    v_max_0 = Params[6]
    v_max_min = Params[7]
    v_y_max_0 = Params[8]
    v_y_max_min = Params[9]
    omega_max_0 = Params[10]
    omega_max_min = Params[11]
    q_x = Params[12]
    q_y = Params[13]
    q_theta = Params[14]
    r_vx = Params[15]
    r_vy = Params[16]
    r_omega = Params[17]
    rd_vx = Params[18]
    rd_vy = Params[19]
    rd_omega = Params[20]
    q_active_factor = Params[21]
    w_slack = Params[22]

    # Kinematic integration helper
    def mecanum_kinematics(state, control):
        theta = state[2]
        vx = control[0]
        vy = control[1]
        omega = control[2]
        dpx = vx * ca.cos(theta) - vy * ca.sin(theta)
        dpy = vx * ca.sin(theta) + vy * ca.cos(theta)
        dtheta = omega
        return ca.vertcat(dpx, dpy, dtheta)

    def predict_rk4(state, control, h):
        k1 = mecanum_kinematics(state, control)
        k2 = mecanum_kinematics(state + h / 2.0 * k1, control)
        k3 = mecanum_kinematics(state + h / 2.0 * k2, control)
        k4 = mecanum_kinematics(state + h * k3, control)
        return state + h / 6.0 * (k1 + 2.0 * k2 + 2.0 * k3 + k4)

    # Build cost function and constraints
    obj = 0.0
    g_eq = []
    g_ineq = []

    # Initial state constraint (equality)
    x_0 = X[0:3]
    g_eq.append(x_0 - X_init)

    # Dynamics, tracking cost, safety, and adaptive limits constraints
    for i in range(N):
        # State and control extraction
        x_curr = X[3*i : 3*i+3]
        x_next = X[3*i+3 : 3*i+6]
        u_curr = U[3*i : 3*i+3]

        # References
        xr = X_ref[3*i : 3*i+3]
        ur = U_ref[3*i : 3*i+3]

        # Dynamic human prediction
        # For each human, predict position along the horizon (constant velocity model)
        phi_j_list = []
        d_j_list = []
        for j in range(N_humans):
            xh0 = H_data[4*j]
            yh0 = H_data[4*j + 1]
            vxh = H_data[4*j + 2]
            vyh = H_data[4*j + 3]

            # Predicted position at step i
            xh_pred = xh0 + i * dt * vxh
            yh_pred = yh0 + i * dt * vyh

            # Distance to human (add epsilon to avoid division by zero)
            dist = ca.sqrt((x_curr[0] - xh_pred)**2 + (x_curr[1] - yh_pred)**2 + 1e-6)
            d_j_list.append(dist)

            # Context score for this human
            phi_j = 1.0 / (1.0 + ca.exp(beta * (dist - d0)))
            phi_j_list.append(phi_j)

        # Combined context score using smooth logical OR: 1 - prod(1 - phi_j)
        prod_term = 1.0
        for phi_j in phi_j_list:
            prod_term *= (1.0 - phi_j)
        phi_k = 1.0 - prod_term

        # Continuous adaptive parameters
        Q_scale = (1.0 - q_active_factor) * (1.0 - phi_k) + q_active_factor
        Q_diag = ca.vertcat(q_x * Q_scale, q_y * Q_scale, q_theta * Q_scale)

        d_safe_k = d_safe_0 * (1.0 - phi_k) + d_safe_max * phi_k

        v_max_k = v_max_0 * (1.0 - phi_k) + v_max_min * phi_k
        v_y_max_k = v_y_max_0 * (1.0 - phi_k) + v_y_max_min * phi_k
        omega_max_k = omega_max_0 * (1.0 - phi_k) + omega_max_min * phi_k

        # 1. State tracking cost
        err = x_curr - xr
        obj += ca.mtimes([err.T, ca.diag(Q_diag), err])

        # 2. Control cost
        obj += ca.mtimes([u_curr.T, ca.diag(ca.vertcat(r_vx, r_vy, r_omega)), u_curr])

        # 3. Control rate cost (delta u)
        if i > 0:
            u_prev = U[3*(i-1) : 3*i]
            du = u_curr - u_prev
            obj += ca.mtimes([du.T, ca.diag(ca.vertcat(rd_vx, rd_vy, rd_omega)), du])

        # 4. Kinematics constraints (RK4 equality constraints)
        x_next_pred = predict_rk4(x_curr, u_curr, dt)
        g_eq.append(x_next - x_next_pred)

        # 5. Safety distance constraints (with slacks)
        if i > 0:
            for j in range(N_humans):
                slack_idx = j * N + (i - 1)
                slack_val = S[slack_idx]
                
                # dist_j >= d_safe - slack
                # i.e., dist_j - d_safe + slack >= 0
                g_ineq.append(d_j_list[j] - d_safe_k + slack_val)
                
                # Penalty on slack
                obj += w_slack * slack_val**2

        # 6. Adaptive velocity limits constraints
        g_ineq.append(u_curr[0] + v_max_k)
        g_ineq.append(v_max_k - u_curr[0])

        g_ineq.append(u_curr[1] + v_y_max_k)
        g_ineq.append(v_y_max_k - u_curr[1])

        g_ineq.append(u_curr[2] + omega_max_k)
        g_ineq.append(omega_max_k - u_curr[2])

    # Terminal state tracking cost
    x_N = X[3*N : 3*N+3]
    xr_N = X_ref[3*N : 3*N+3]
    err_N = x_N - xr_N
    
    phi_j_N_list = []
    for j in range(N_humans):
        xh0 = H_data[4*j]
        yh0 = H_data[4*j + 1]
        vxh = H_data[4*j + 2]
        vyh = H_data[4*j + 3]
        xh_pred_N = xh0 + N * dt * vxh
        yh_pred_N = yh0 + N * dt * vyh
        dist_N = ca.sqrt((x_N[0] - xh_pred_N)**2 + (x_N[1] - yh_pred_N)**2 + 1e-6)
        phi_j_N = 1.0 / (1.0 + ca.exp(beta * (dist_N - d0)))
        phi_j_N_list.append(phi_j_N)

    prod_term_N = 1.0
    for phi_j_N in phi_j_N_list:
        prod_term_N *= (1.0 - phi_j_N)
    phi_N = 1.0 - prod_term_N

    Q_scale_N = (1.0 - q_active_factor) * (1.0 - phi_N) + q_active_factor
    Q_diag_N = ca.vertcat(q_x * Q_scale_N, q_y * Q_scale_N, q_theta * Q_scale_N)
    obj += ca.mtimes([err_N.T, ca.diag(Q_diag_N * 5.0), err_N])

    # Concatenate all constraints
    g_all = ca.vertcat(*g_eq, *g_ineq)

    # Setup NLP dictionary
    nlp = {'x': vars_all, 'f': obj, 'g': g_all, 'p': p_all}

    # Solver options for sqpmethod & qrqp
    opts = {
        'qpsol': 'qrqp',
        'print_time': False,
        'print_header': False,
        'print_iteration': False,
        'max_iter': 15,
        'tol_pr': 1e-3,
        'tol_du': 1e-3
    }

    # Create solver
    solver = ca.nlpsol('canmpc_solver', 'sqpmethod', nlp, opts)

    # Output directory relative to scripts directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    gen_dir = os.path.join(script_dir, '../generated')
    os.makedirs(gen_dir, exist_ok=True)
    
    # Change working directory to gen_dir so CodeGenerator writes files there
    os.chdir(gen_dir)
    
    # Generate C code using CodeGenerator
    cg = ca.CodeGenerator('canmpc_solver')
    cg.add(solver)
    cg.generate()
    print(f"Successfully generated solver C code in: {gen_dir}")

if __name__ == '__main__':
    main()
