import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, SetEnvironmentVariable
from launch.launch_description_sources import PythonLaunchDescriptionSource


def generate_launch_description():
    turn_on_dir = get_package_share_directory("turn_on_rai_robot")

    return LaunchDescription([
        SetEnvironmentVariable("RAI_DEVICE_ROLE", "pi"),
        SetEnvironmentVariable("RAI_DEVICE_LABEL", "raspberry_pi_4"),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(turn_on_dir, "launch", "prod_bringup.launch.py")
            ),
        ),
    ])
