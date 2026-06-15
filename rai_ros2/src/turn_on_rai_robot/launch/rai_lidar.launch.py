#!/usr/bin/env python3
"""Launch the Rai robot LSLiDAR N10P."""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource


def generate_launch_description():
    lslidar_dir = get_package_share_directory('lslidar_driver')
    lslidar_launch_dir = os.path.join(lslidar_dir, 'launch')

    lsn10p = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(lslidar_launch_dir, 'lsn10p_launch.py')),
    )

    ld = LaunchDescription()
    ld.add_action(lsn10p)
    return ld
