import os

from ament_index_python.packages import PackageNotFoundError, get_package_prefix, get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, LogInfo, OpaqueFunction
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def _launch_twist_mux(context, *_args, **_kwargs):
    try:
        get_package_prefix("twist_mux")
    except PackageNotFoundError:
        return [
            LogInfo(
                msg=(
                    "Package 'twist_mux' is not installed. Skipping velocity mux. "
                    "Install it with: sudo apt install ros-humble-twist-mux"
                )
            )
        ]

    params = context.perform_substitution(LaunchConfiguration("params"))
    return [
        Node(
            package="twist_mux",
            executable="twist_mux",
            name="twist_mux",
            output="screen",
            parameters=[params],
            remappings=[
                ("cmd_vel_out", "/cmd_vel"),
            ],
        )
    ]


def generate_launch_description():
    package_dir = get_package_share_directory("turn_on_rai_robot")
    default_params = os.path.join(package_dir, "config", "twist_mux.yaml")

    return LaunchDescription([
        DeclareLaunchArgument(
            "params",
            default_value=default_params,
            description="Path to twist_mux parameter file",
        ),
        OpaqueFunction(function=_launch_twist_mux),
    ])
