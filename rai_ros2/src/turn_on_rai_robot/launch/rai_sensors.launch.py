import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource


def generate_launch_description():
    bringup_dir = get_package_share_directory('turn_on_rai_robot')
    launch_dir = os.path.join(bringup_dir, 'launch')

    rai_robot = IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(launch_dir, 'turn_on_rai_robot.launch.py')),
    )

    lidar_ros = IncludeLaunchDescription(
            PythonLaunchDescriptionSource(os.path.join(launch_dir, 'rai_lidar.launch.py')),
    )

    return LaunchDescription([
        rai_robot, lidar_ros]
    )
