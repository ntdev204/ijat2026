import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, SetEnvironmentVariable
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    turn_on_dir = get_package_share_directory("turn_on_rai_robot")
    nav2_dir = get_package_share_directory("rai_nav2")

    with_nav2 = LaunchConfiguration("with_nav2")
    map_path = LaunchConfiguration("map")
    params_path = LaunchConfiguration("params")
    local_planner = LaunchConfiguration("local_planner")
    global_planner = LaunchConfiguration("global_planner")

    return LaunchDescription([
        SetEnvironmentVariable("RAI_DEVICE_ROLE", "pi"),
        SetEnvironmentVariable("RAI_DEVICE_LABEL", "raspberry_pi_4"),
        DeclareLaunchArgument(
            "with_nav2",
            default_value="false",
            description="Launch Nav2 map/localization/planner/costmap stack on Pi",
        ),
        DeclareLaunchArgument(
            "map",
            default_value="/home/rai/rai_ros2/data/map/RAI.yaml",
            description="Nav2 map YAML path",
        ),
        DeclareLaunchArgument(
            "params",
            default_value=os.path.join(nav2_dir, "param", "rai_params", "canmpc_mec_nav2.yaml"),
            description="Nav2 params path",
        ),
        DeclareLaunchArgument(
            "local_planner",
            default_value="MPPI",
            description="Nav2 local planner preset for Pi-side Nav2 stack",
        ),
        DeclareLaunchArgument(
            "global_planner",
            default_value="A_STAR",
            description="Nav2 global planner preset",
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(turn_on_dir, "launch", "prod_bringup.launch.py")
            ),
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(nav2_dir, "launch", "rai_nav2.launch.py")
            ),
            condition=IfCondition(with_nav2),
            launch_arguments={
                "map": map_path,
                "params": params_path,
                "local_planner": local_planner,
                "global_planner": global_planner,
                "use_composition": "False",
            }.items(),
        ),
    ])
