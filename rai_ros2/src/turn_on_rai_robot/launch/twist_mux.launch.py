import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    package_dir = get_package_share_directory("turn_on_rai_robot")
    default_params = os.path.join(package_dir, "config", "twist_mux.yaml")

    params = LaunchConfiguration("params")

    return LaunchDescription([
        DeclareLaunchArgument(
            "params",
            default_value=default_params,
            description="Path to twist_mux parameter file",
        ),
        Node(
            package="twist_mux",
            executable="twist_mux",
            name="twist_mux",
            output="screen",
            parameters=[params],
            remappings=[
                ("cmd_vel_out", "/cmd_vel"),
            ],
        ),
    ])
