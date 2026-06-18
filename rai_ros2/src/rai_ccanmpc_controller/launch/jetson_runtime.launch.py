import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, SetEnvironmentVariable
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    package_dir = get_package_share_directory("rai_ccanmpc_controller")
    default_params = os.path.join(package_dir, "config", "rai_ccanmpc_controller.yaml")

    params = LaunchConfiguration("params")
    use_sim_time = LaunchConfiguration("use_sim_time")

    return LaunchDescription([
        SetEnvironmentVariable("RAI_DEVICE_ROLE", "jetson"),
        SetEnvironmentVariable("RAI_DEVICE_LABEL", "jetson_orin_nano"),
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
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(package_dir, "launch", "rai_ccanmpc_controller.launch.py")
            ),
            launch_arguments={
                "params": params,
                "use_sim_time": use_sim_time,
            }.items(),
        ),
    ])
