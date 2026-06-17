import os
import tempfile

import yaml
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


LOCAL_PLANNER_PRESETS = {
    "CCA_NMPC": {
        "plugin": "cca_nmpc_controller/CCANMPCController",
        "horizon_steps": 20,
        "model_dt": 0.05,
        "max_solver_time_ms": 50.0,
        "max_path_length": 3.0,
        "default_v_ref": 0.5,
        "beta": 3.0,
        "d0": 1.5,
        "d_safe_0": 0.5,
        "d_safe_max": 1.2,
        "v_max_min": 0.1,
        "v_y_max_min": 0.1,
        "omega_max_min": 0.2,
        "q_x": 10.0,
        "q_y": 10.0,
        "q_theta": 5.0,
        "r_vx": 0.1,
        "r_vy": 0.1,
        "r_omega": 0.05,
        "rd_vx": 1.0,
        "rd_vy": 1.0,
        "rd_omega": 0.5,
        "q_active_factor": 0.1,
        "w_slack": 100000.0,
    },
    "MPPI": {
        "plugin": "nav2_mppi_controller::MPPIController",
        "time_steps": 56,
        "model_dt": 0.05,
        "batch_size": 2000,
        "vx_std": 0.2,
        "vy_std": 0.2,
        "wz_std": 0.4,
        "vx_max": 0.2,
        "vx_min": -0.2,
        "vy_max": 0.2,
        "vy_min": -0.2,
        "wz_max": 1.3,
        "iteration_count": 1,
        "prune_distance": 1.7,
        "transform_tolerance": 0.1,
        "temperature": 0.3,
        "gamma": 0.015,
        "motion_model": "Omni",
        "visualize": False,
        "TrajectoryVisualizer.trajectory_step": 5,
        "TrajectoryVisualizer.time_step": 3,
        "critics": [
            "ConstraintCritic",
            "ObstaclesCritic",
            "GoalCritic",
            "GoalAngleCritic",
            "PathAlignCritic",
            "PathFollowCritic",
            "PathAngleCritic",
            "PreferForwardCritic",
        ],
    },
    "DWB": {
        "plugin": "nav2_rotation_shim_controller::RotationShimController",
        "primary_controller": "dwb_core::DWBLocalPlanner",
        "debug_trajectory_details": True,
        "min_vel_x": 0.0,
        "min_vel_y": -0.3,
        "max_vel_x": 0.4,
        "max_vel_y": 0.3,
        "max_vel_theta": 1.0,
        "min_speed_xy": 0.0,
        "max_speed_xy": 0.3,
        "min_speed_theta": 0.0,
        "acc_lim_x": 2.5,
        "acc_lim_y": 2.5,
        "acc_lim_theta": 0.5,
        "decel_lim_x": -0.5,
        "decel_lim_y": -0.5,
        "decel_lim_theta": -0.5,
        "vx_samples": 20,
        "vy_samples": 20,
        "vtheta_samples": 40,
        "sim_time": 1.7,
        "linear_granularity": 0.05,
        "angular_granularity": 0.025,
        "transform_tolerance": 0.2,
        "xy_goal_tolerance": 0.25,
        "trans_stopped_velocity": 0.1,
        "short_circuit_trajectory_evaluation": True,
        "stateful": True,
        "critics": ["RotateToGoal", "Oscillation", "BaseObstacle", "GoalAlign", "PathAlign", "PathDist", "GoalDist"],
        "BaseObstacle.scale": 1.0,
        "PathAlign.scale": 32.0,
        "PathAlign.forward_point_distance": 0.1,
        "GoalAlign.scale": 24.0,
        "GoalAlign.forward_point_distance": 0.1,
        "PathDist.scale": 32.0,
        "GoalDist.scale": 24.0,
        "RotateToGoal.scale": 32.0,
        "RotateToGoal.slowing_factor": 5.0,
        "RotateToGoal.lookahead_time": -1.0,
        "Oscillation.oscillation_reset_dist": 0.05,
        "Oscillation.oscillation_reset_angle": 0.05,
    },
    "DWA": {
        "plugin": "nav2_rotation_shim_controller::RotationShimController",
        "primary_controller": "dwb_core::DWBLocalPlanner",
        "debug_trajectory_details": True,
        "trajectory_generator_name": "dwb_plugins::LimitedAccelGenerator",
        "min_vel_x": 0.0,
        "min_vel_y": -0.3,
        "max_vel_x": 0.4,
        "max_vel_y": 0.3,
        "max_vel_theta": 1.0,
        "min_speed_xy": 0.0,
        "max_speed_xy": 0.3,
        "min_speed_theta": 0.0,
        "acc_lim_x": 2.5,
        "acc_lim_y": 2.5,
        "acc_lim_theta": 0.5,
        "decel_lim_x": -0.5,
        "decel_lim_y": -0.5,
        "decel_lim_theta": -0.5,
        "vx_samples": 20,
        "vy_samples": 20,
        "vtheta_samples": 40,
        "sim_time": 1.2,
        "linear_granularity": 0.05,
        "angular_granularity": 0.025,
        "transform_tolerance": 0.2,
        "xy_goal_tolerance": 0.25,
        "trans_stopped_velocity": 0.1,
        "short_circuit_trajectory_evaluation": True,
        "stateful": True,
        "critics": ["RotateToGoal", "Oscillation", "BaseObstacle", "GoalAlign", "PathAlign", "PathDist", "GoalDist"],
    },
}

GLOBAL_PLANNER_PRESETS = {
    "A_STAR": {
        "plugin": "nav2_navfn_planner/NavfnPlanner",
        "tolerance": 0.5,
        "use_astar": True,
        "allow_unknown": True,
    },
    "DIJKSTRA": {
        "plugin": "nav2_navfn_planner/NavfnPlanner",
        "tolerance": 0.5,
        "use_astar": False,
        "allow_unknown": True,
    },
    "HYBRID_ASTAR": {
        "plugin": "nav2_smac_planner/SmacPlannerHybrid",
        "tolerance": 0.5,
        "allow_unknown": True,
        "downsample_costmap": False,
        "minimum_turning_radius": 0.2,
        "motion_model_for_search": "DUBIN",
    },
}

BT_PLUGIN_LIB_NAMES = [
    "nav2_compute_path_to_pose_action_bt_node",
    "nav2_compute_path_through_poses_action_bt_node",
    "nav2_controller_cancel_bt_node",
    "nav2_wait_cancel_bt_node",
    "nav2_spin_cancel_bt_node",
    "nav2_back_up_cancel_bt_node",
    "nav2_assisted_teleop_cancel_bt_node",
    "nav2_drive_on_heading_cancel_bt_node",
    "nav2_smooth_path_action_bt_node",
    "nav2_follow_path_action_bt_node",
    "nav2_back_up_action_bt_node",
    "nav2_drive_on_heading_bt_node",
    "nav2_spin_action_bt_node",
    "nav2_wait_action_bt_node",
    "nav2_assisted_teleop_action_bt_node",
    "nav2_clear_costmap_service_bt_node",
    "nav2_is_stuck_condition_bt_node",
    "nav2_transform_available_condition_bt_node",
    "nav2_goal_reached_condition_bt_node",
    "nav2_globally_updated_goal_condition_bt_node",
    "nav2_goal_updated_condition_bt_node",
    "nav2_is_path_valid_condition_bt_node",
    "nav2_time_expired_condition_bt_node",
    "nav2_path_expiring_timer_condition",
    "nav2_distance_traveled_condition_bt_node",
    "nav2_initial_pose_received_condition_bt_node",
    "nav2_is_battery_low_condition_bt_node",
    "nav2_reinitialize_global_localization_service_bt_node",
    "nav2_rate_controller_bt_node",
    "nav2_distance_controller_bt_node",
    "nav2_speed_controller_bt_node",
    "nav2_truncate_path_action_bt_node",
    "nav2_truncate_path_local_action_bt_node",
    "nav2_goal_updater_node_bt_node",
    "nav2_path_longer_on_approach_bt_node",
    "nav2_recovery_node_bt_node",
    "nav2_navigate_to_pose_action_bt_node",
    "nav2_navigate_through_poses_action_bt_node",
    "nav2_remove_passed_goals_action_bt_node",
    "nav2_pipeline_sequence_bt_node",
    "nav2_round_robin_node_bt_node",
    "nav2_single_trigger_bt_node",
    "nav2_planner_selector_bt_node",
    "nav2_controller_selector_bt_node",
    "nav2_smoother_selector_bt_node",
    "nav2_goal_checker_selector_bt_node",
    "nav2_goal_updated_controller_bt_node",
]


def _write_runtime_bt_tree(source_path, destination_path):
    with open(source_path, "r", encoding="utf-8") as handle:
        tree_xml = handle.read()

    tree_xml = tree_xml.replace(
        '<FollowPath path="{path}" controller_id="FollowPath"/>',
        '<FollowPath path="{path}" controller_id="FollowPath" goal_checker_id="general_goal_checker"/>',
    )

    with open(destination_path, "w", encoding="utf-8") as handle:
        handle.write(tree_xml)


def _write_runtime_params(context, *_args, **_kwargs):
    params_path = context.perform_substitution(LaunchConfiguration("params"))
    map_path = context.perform_substitution(LaunchConfiguration("map"))
    slam = context.perform_substitution(LaunchConfiguration("slam")).lower() in ("true", "1", "yes")
    local_planner = context.perform_substitution(LaunchConfiguration("local_planner")).upper()
    global_planner = context.perform_substitution(LaunchConfiguration("global_planner")).upper()

    if not os.path.exists(params_path):
        raise FileNotFoundError(f"Nav2 params file not found: {params_path}")
    if not slam and not os.path.exists(map_path):
        raise FileNotFoundError(f"Nav2 map file not found: {map_path}")

    with open(params_path, "r", encoding="utf-8") as handle:
        config = yaml.safe_load(handle) or {}

    if local_planner not in LOCAL_PLANNER_PRESETS:
        local_planner = "CCA_NMPC"
    local_preset = LOCAL_PLANNER_PRESETS[local_planner]
    global_preset = GLOBAL_PLANNER_PRESETS.get(global_planner, GLOBAL_PLANNER_PRESETS["A_STAR"])

    controller_params = config.setdefault("controller_server", {}).setdefault("ros__parameters", {})
    planner_params = config.setdefault("planner_server", {}).setdefault("ros__parameters", {})
    bt_params = config.setdefault("bt_navigator", {}).setdefault("ros__parameters", {})
    amcl_params = config.setdefault("amcl", {}).setdefault("ros__parameters", {})
    local_costmap_params = (
        config.setdefault("local_costmap", {})
        .setdefault("local_costmap", {})
        .setdefault("ros__parameters", {})
    )
    global_costmap_params = (
        config.setdefault("global_costmap", {})
        .setdefault("global_costmap", {})
        .setdefault("ros__parameters", {})
    )

    nav2_bt_dir = get_package_share_directory("nav2_bt_navigator")
    bt_tree_dir = os.path.join(nav2_bt_dir, "behavior_trees")
    temp_dir = tempfile.mkdtemp(prefix="rai_nav2_")
    runtime_nav_to_pose_bt = os.path.join(
        temp_dir,
        "navigate_to_pose_w_replanning_and_recovery.xml",
    )
    runtime_nav_through_poses_bt = os.path.join(
        temp_dir,
        "navigate_through_poses_w_replanning_and_recovery.xml",
    )
    _write_runtime_bt_tree(
        os.path.join(bt_tree_dir, "navigate_to_pose_w_replanning_and_recovery.xml"),
        runtime_nav_to_pose_bt,
    )
    _write_runtime_bt_tree(
        os.path.join(bt_tree_dir, "navigate_through_poses_w_replanning_and_recovery.xml"),
        runtime_nav_through_poses_bt,
    )

    controller_params["controller_plugins"] = ["FollowPath"]
    controller_params["goal_checker_plugins"] = ["general_goal_checker"]
    controller_params["current_goal_checker"] = "general_goal_checker"
    controller_params["FollowPath"] = local_preset
    controller_params["selected_local_planner"] = local_planner
    if local_planner == "MPPI":
        model_dt = float(local_preset.get("model_dt", 0.05))
        controller_params["controller_frequency"] = 1.0 / model_dt

    planner_params["planner_plugins"] = ["GridBased"]
    planner_params["GridBased"] = global_preset
    planner_params["selected_global_planner"] = global_planner
    amcl_params.setdefault("set_initial_pose", True)
    amcl_params["transform_tolerance"] = 0.3
    local_costmap_params["transform_tolerance"] = 0.3
    global_costmap_params["transform_tolerance"] = 0.3
    bt_params["plugin_lib_names"] = BT_PLUGIN_LIB_NAMES
    bt_params["default_nav_to_pose_bt_xml"] = runtime_nav_to_pose_bt
    bt_params["default_nav_through_poses_bt_xml"] = runtime_nav_through_poses_bt

    runtime_params_path = os.path.join(temp_dir, "nav2_runtime.yaml")
    with open(runtime_params_path, "w", encoding="utf-8") as handle:
        yaml.safe_dump(config, handle, sort_keys=False)

    context.launch_configurations["runtime_params"] = runtime_params_path
    return []


def generate_launch_description():
    use_sim_time = LaunchConfiguration("use_sim_time", default="false")
    slam = LaunchConfiguration("slam", default="False")
    use_composition = LaunchConfiguration("use_composition", default="False")
    runtime_params = LaunchConfiguration("runtime_params")

    rai_nav_dir = get_package_share_directory("rai_nav2")
    rai_nav_launch_dir = os.path.join(rai_nav_dir, "launch")

    map_default = "/home/rai/rai_ros2/data/map/RAI.yaml"
    map_file = LaunchConfiguration("map")

    param_dir = os.path.join(rai_nav_dir, "param", "rai_params")
    default_params_path = os.path.join(param_dir, "canmpc_mec_nav2.yaml")
    param_file = LaunchConfiguration("params")
    local_planner = LaunchConfiguration("local_planner")
    global_planner = LaunchConfiguration("global_planner")

    return LaunchDescription([
        DeclareLaunchArgument(
            "map",
            default_value=map_default,
            description="Full path to map file to load",
        ),
        DeclareLaunchArgument(
            "params",
            default_value=default_params_path,
            description="Full path to base param file to load",
        ),
        DeclareLaunchArgument(
            "local_planner",
            default_value="CCA_NMPC",
            description="CCA_NMPC | MPPI | DWB | DWA",
        ),
        DeclareLaunchArgument(
            "global_planner",
            default_value="A_STAR",
            description="A_STAR | DIJKSTRA | HYBRID_ASTAR",
        ),
        DeclareLaunchArgument(
            "slam",
            default_value="False",
            description="Run SLAM if true, localization with map if false",
        ),
        DeclareLaunchArgument(
            "use_composition",
            default_value="False",
            description="Use Nav2 component composition if true",
        ),
        OpaqueFunction(function=_write_runtime_params),
        Node(
            name="waypoint_cycle",
            package="nav2_waypoint_cycle",
            executable="nav2_waypoint_cycle",
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource([rai_nav_launch_dir, "/bringup_launch.py"]),
            launch_arguments={
                "map": map_file,
                "slam": slam,
                "use_sim_time": use_sim_time,
                "use_composition": use_composition,
                "params_file": runtime_params,
            }.items(),
        ),
    ])
