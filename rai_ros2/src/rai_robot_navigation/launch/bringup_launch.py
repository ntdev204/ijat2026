import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    robot_navigation_dir = get_package_share_directory("rai_robot_navigation")
    navigation_dir = get_package_share_directory("rai_navigation")
    default_params = os.path.join(robot_navigation_dir, "param", "rai_robot_navigation.yaml")

    use_sim_time = LaunchConfiguration("use_sim_time")
    map_yaml = LaunchConfiguration("map")
    params = LaunchConfiguration("params")
    controller_id = LaunchConfiguration("controller_id")
    global_planner_algorithm = LaunchConfiguration("global_planner_algorithm")
    enable_waypoint_cycle = LaunchConfiguration("enable_waypoint_cycle")

    return LaunchDescription([
        DeclareLaunchArgument(
            "use_sim_time",
            default_value="false",
            description="Use simulation clock",
        ),
        DeclareLaunchArgument(
            "map",
            default_value="",
            description="Optional map YAML selected at runtime by the website or launch argument.",
        ),
        DeclareLaunchArgument(
            "params",
            default_value=default_params,
            description="RAI navigation parameter file",
        ),
        DeclareLaunchArgument(
            "controller_id",
            default_value="CCA_NMPC",
            description="Controller algorithm id",
        ),
        DeclareLaunchArgument(
            "global_planner_algorithm",
            default_value="A_STAR",
            description="Global planner: A_STAR | DIJKSTRA | STRAIGHT_LINE",
        ),
        DeclareLaunchArgument(
            "enable_waypoint_cycle",
            default_value="false",
            description="Launch RAI waypoint cycle",
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(navigation_dir, "launch", "rai_navigation.launch.py")
            ),
            launch_arguments={
                "map": map_yaml,
                "params": params,
                "use_sim_time": use_sim_time,
                "controller_id": controller_id,
                "global_planner_algorithm": global_planner_algorithm,
            }.items(),
        ),
        Node(
            condition=IfCondition(enable_waypoint_cycle),
            package="rai_navigation",
            executable="waypoint_cycle.py",
            name="rai_waypoint_cycle",
            output="screen",
            parameters=[{
                "use_sim_time": use_sim_time,
                "max_retries": 1,
            }],
        ),
    ])
