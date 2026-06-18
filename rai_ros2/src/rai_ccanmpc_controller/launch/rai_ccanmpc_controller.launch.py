import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    package_dir = get_package_share_directory("rai_ccanmpc_controller")
    default_params = os.path.join(package_dir, "config", "rai_ccanmpc_controller.yaml")

    params = LaunchConfiguration("params")
    use_sim_time = LaunchConfiguration("use_sim_time")

    return LaunchDescription([
        DeclareLaunchArgument(
            "params",
            default_value=default_params,
            description="Path to rai_ccanmpc_controller parameter file",
        ),
        DeclareLaunchArgument(
            "use_sim_time",
            default_value="false",
            description="Use simulation clock",
        ),
        Node(
            package="rai_ccanmpc_controller",
            executable="rai_ccanmpc_controller_node",
            name="rai_ccanmpc_controller",
            output="screen",
            parameters=[
                params,
                {"use_sim_time": use_sim_time},
            ],
        ),
    ])
