import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def _maybe_start_map_server(context, map_yaml, map_topic, use_sim_time):
    yaml_filename = context.perform_substitution(map_yaml).strip()
    if not yaml_filename:
        return []

    return [
        Node(
            package="rai_navigation",
            executable="rai_map_server.py",
            name="rai_map_server",
            output="screen",
            parameters=[{
                "yaml_filename": yaml_filename,
                "frame_id": "map",
                "topic_name": context.perform_substitution(map_topic),
                "use_sim_time": context.perform_substitution(use_sim_time),
            }],
        )
    ]


def generate_launch_description():
    navigation_dir = get_package_share_directory("rai_navigation")
    controller_dir = get_package_share_directory("rai_controller")
    default_params = os.path.join(navigation_dir, "config", "rai_navigation.yaml")

    params = LaunchConfiguration("params")
    map_yaml = LaunchConfiguration("map")
    use_sim_time = LaunchConfiguration("use_sim_time")
    controller_id = LaunchConfiguration("controller_id")
    global_planner_algorithm = LaunchConfiguration("global_planner_algorithm")
    cmd_vel_topic = LaunchConfiguration("cmd_vel_topic")
    goal_topic = LaunchConfiguration("goal_topic")
    map_topic = LaunchConfiguration("map_topic")
    laser_scan_topic = LaunchConfiguration("laser_scan_topic")
    imu_topic = LaunchConfiguration("imu_topic")
    status_topic = LaunchConfiguration("status_topic")
    navigate_to_pose_service = LaunchConfiguration("navigate_to_pose_service")
    cancel_service = LaunchConfiguration("cancel_service")
    clear_costmap_service = LaunchConfiguration("clear_costmap_service")
    replan_service = LaunchConfiguration("replan_service")
    get_status_service = LaunchConfiguration("get_status_service")
    list_algorithms_service = LaunchConfiguration("list_algorithms_service")
    set_controller_service = LaunchConfiguration("set_controller_service")
    set_planner_service = LaunchConfiguration("set_planner_service")

    return LaunchDescription([
        DeclareLaunchArgument(
            "params",
            default_value=default_params,
            description="Path to RAI navigation parameter file",
        ),
        DeclareLaunchArgument(
            "map",
            default_value="",
            description="Optional static map YAML. If empty, /map must come from SLAM or another map server.",
        ),
        DeclareLaunchArgument(
            "use_sim_time",
            default_value="false",
            description="Use simulation clock",
        ),
        DeclareLaunchArgument(
            "controller_id",
            default_value="CCA_NMPC",
            description="Controller algorithm id: CCA_NMPC",
        ),
        DeclareLaunchArgument(
            "global_planner_algorithm",
            default_value="A_STAR",
            description="Global planner: A_STAR | DIJKSTRA | STRAIGHT_LINE",
        ),
        DeclareLaunchArgument(
            "cmd_vel_topic",
            default_value="/rai_navigation/cmd_vel",
            description="Velocity command output topic",
        ),
        DeclareLaunchArgument(
            "goal_topic",
            default_value="/goal_pose",
            description="PoseStamped goal input topic",
        ),
        DeclareLaunchArgument(
            "map_topic",
            default_value="/map",
            description="OccupancyGrid map input topic",
        ),
        DeclareLaunchArgument(
            "laser_scan_topic",
            default_value="/scan_filtered",
            description="LaserScan input topic",
        ),
        DeclareLaunchArgument(
            "imu_topic",
            default_value="/imu/data",
            description="IMU input topic",
        ),
        DeclareLaunchArgument(
            "status_topic",
            default_value="/rai_navigation/status",
            description="Navigation status output topic",
        ),
        DeclareLaunchArgument(
            "navigate_to_pose_service",
            default_value="/rai_navigation/navigate_to_pose",
            description="Service used to submit navigation goals",
        ),
        DeclareLaunchArgument(
            "cancel_service",
            default_value="/rai_navigation/cancel",
            description="Service used to cancel navigation",
        ),
        DeclareLaunchArgument(
            "clear_costmap_service",
            default_value="/rai_navigation/clear_costmap",
            description="Service used to clear local obstacle grids",
        ),
        DeclareLaunchArgument(
            "replan_service",
            default_value="/rai_navigation/replan",
            description="Service used to replan to the active goal",
        ),
        DeclareLaunchArgument(
            "get_status_service",
            default_value="/rai_navigation/get_status",
            description="Service used to query navigation status",
        ),
        DeclareLaunchArgument(
            "list_algorithms_service",
            default_value="/rai_navigation/list_algorithms",
            description="Service used to list controller and planner algorithms",
        ),
        DeclareLaunchArgument(
            "set_controller_service",
            default_value="/rai_navigation/set_controller",
            description="Service used to select controller algorithm",
        ),
        DeclareLaunchArgument(
            "set_planner_service",
            default_value="/rai_navigation/set_planner",
            description="Service used to select global planner algorithm",
        ),
        OpaqueFunction(
            function=_maybe_start_map_server,
            args=[map_yaml, map_topic, use_sim_time],
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(controller_dir, "launch", "rai_controller.launch.py")
            ),
            launch_arguments={
                "params": params,
                "use_sim_time": use_sim_time,
                "controller_id": controller_id,
                "global_planner_algorithm": global_planner_algorithm,
                "cmd_vel_topic": cmd_vel_topic,
                "goal_topic": goal_topic,
                "map_topic": map_topic,
                "laser_scan_topic": laser_scan_topic,
                "imu_topic": imu_topic,
                "status_topic": status_topic,
                "navigate_to_pose_service": navigate_to_pose_service,
                "cancel_service": cancel_service,
                "clear_costmap_service": clear_costmap_service,
                "replan_service": replan_service,
                "get_status_service": get_status_service,
                "list_algorithms_service": list_algorithms_service,
                "set_controller_service": set_controller_service,
                "set_planner_service": set_planner_service,
            }.items(),
        ),
    ])
