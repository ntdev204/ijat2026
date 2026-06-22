# RAI Controller CCA-NMPC

`rai_controller_cca_nmpc` owns the CCA-NMPC controller algorithm. `rai_controller` only hosts the server, topic/service contract, and algorithm registry.

## Mathematical Model Implemented In The Core

Robot state and Mecanum command:

```text
X_{r,k} = [x_{r,k}, y_{r,k}, theta_{r,k}]^T
u_k = [v_{x,k}, v_{y,k}, omega_k]^T
```

The rollout uses the 3-DOF Mecanum kinematic model:

```text
x_{r,k+1} = x_{r,k} + (v_{x,k} cos(theta_{r,k}) - v_{y,k} sin(theta_{r,k})) T_s
y_{r,k+1} = y_{r,k} + (v_{x,k} sin(theta_{r,k}) + v_{y,k} cos(theta_{r,k})) T_s
theta_{r,k+1} = wrap(theta_{r,k} + omega_k T_s)
```

For numerical rollout the implementation uses a fourth-order Runge-Kutta
integration step over the same Mecanum model.

For each human track:

```text
S_{h,j,k} = [p_{h,j,k}^T, v_{h,j,k}^T, c_{h,j,k}, P_{h,j,k}]^T
p_hat_{h,j,k+i|k} = p_{h,j,k} + i T_s v_{h,j,k}
c_hat_{h,j,k+i|k} = c_{det,j,k} exp(-trace(P_{h,j,k}))
```

At every predicted step, each predicted human produces:

```text
d_hat_{h,j,k+i|k} = ||p_hat_{r,k+i|k} - p_hat_{h,j,k+i|k}||
v_hat_{h,j,k+i|k} = v_hat_{h,j,k+i|k}
```

The robot velocity used in the relative-motion term is the world-frame
projection of the Mecanum command:

```text
v_hat_{r,k+i|k} =
  [
    v_{x,k+i|k} cos(theta_{r,k+i|k}) - v_{y,k+i|k} sin(theta_{r,k+i|k})
    v_{x,k+i|k} sin(theta_{r,k+i|k}) + v_{y,k+i|k} cos(theta_{r,k+i|k})
  ]^T

v_hat_{rel,k+i|k} = v_hat_{h,k+i|k} - v_hat_{r,k+i|k}
```

The relative direction term is then:

```text
e_hat_{rh,k+i|k} =
  (p_hat_{r,k+i|k} - p_hat_{h,j,k+i|k}) /
  max(d_hat_{h,j,k+i|k}, epsilon)

cos(Delta theta_hat_{j,k+i|k}) =
  (v_hat_{rel,k+i|k} dot e_hat_{rh,k+i|k}) /
  max(||v_hat_{rel,k+i|k}||, epsilon)
```

The implemented continuous context score is:

```text
phi_hat_{j,k+i|k} = sigma(z_{j,k+i|k})

z_{j,k+i|k} =
  context_distance_weight (d_0 - d_hat_{h,j,k+i|k}) / d_0
  + context_velocity_weight ||v_hat_{h,j,k+i|k}|| / human_velocity_max
  + context_direction_weight cos(Delta theta_hat_{j,k+i|k})
  + context_confidence_weight c_hat_{h,j,k+i|k}
  + context_bias
```

where `sigma(z) = 1 / (1 + exp(-z))`. This replaces the older
distance-only form `phi_h = f(d_h)`.

Multi-human aggregation is implemented with a conservative max operator:

```text
phi_hat_{h,k+i|k} = max_j phi_hat_{j,k+i|k}
d_hat_{h,k+i|k} = min_j d_hat_{h,j,k+i|k}
```

The NMPC horizon updates context-dependent parameters at every step:

```text
p_hat_h -> d_hat_h, v_hat_h, Delta theta_hat, c_hat_h
  -> phi_hat_h
  -> Q(phi_hat_h), d_safe(phi_hat_h), v_max(phi_hat_h), omega_max(phi_hat_h)
```

The adaptive safety distance is explicitly:

```text
d_safe(phi_hat_h) = d_safe,0 + k_d phi_hat_h
```

with the implementation equivalent:

```text
k_d = d_safe,max - d_safe,0
```

For example:

```text
d_safe(phi_hat_h) = 0.5 + 0.3 phi_hat_h
```

## Adaptive Velocity Constraints

The adaptive velocity limits are explicitly:

```text
v_x,max(phi_hat_h) = v_x,max,0 - k_vx phi_hat_h
v_y,max(phi_hat_h) = v_y,max,0 - k_vy phi_hat_h
omega_max(phi_hat_h) = omega_max,0 - k_omega phi_hat_h
```

with the implementation written in bounded form as:

```text
k_vx = v_x,max,0 - v_x,min
k_vy = v_y,max,0 - v_y,min
k_omega = omega_max,0 - omega_min
```

These limits enter the NMPC problem as explicit constraints:

```text
|v_{x,k+i|k}| <= v_x,max(phi_hat_{h,k+i|k})
|v_{y,k+i|k}| <= v_y,max(phi_hat_{h,k+i|k})
|omega_{k+i|k}| <= omega_max(phi_hat_{h,k+i|k})
```

The tracking weights are:

```text
q_x(phi_hat_h) = q_x + phi_hat_h q_human_x
q_y(phi_hat_h) = q_y + phi_hat_h q_human_y
q_theta(phi_hat_h) = q_theta + phi_hat_h q_human_theta
```

The human interaction cost is:

```text
J_h(k+i|k) =
  human_cost_weight phi_hat_{h,k+i|k}
  max(0, d_0 - d_hat_{h,k+i|k})^2
```

Hard safety constraints are checked with:

```text
d_hat_{h,k+i|k} >= d_safe(phi_hat_{h,k+i|k})
```
