#!/usr/bin/env python3
"""
rplidar_slam_toolbox.launch.py - RPLidar SLAM (async mode)

File nay chi chay SLAM Toolbox (async).
Robot base + lidar + scan filter da duoc khoi dong boi prod_bringup.launch.py.

Su dung doc lap:
  ros2 launch rai_slam_toolbox rplidar_slam_toolbox.launch.py

Hoac tich hop vao bringup tong (sau rai_sensors):
  them vao prod_bringup.launch.py
"""
import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    slam_dir = get_package_share_directory('rai_slam_toolbox')

    return LaunchDescription([
        Node(
            package='slam_toolbox',
            executable='async_slam_toolbox_node',
            name='slam_toolbox',
            output='screen',
            parameters=[
                os.path.join(slam_dir, 'config', 'mapper_params_rplidar_a1.yaml')
            ],
            remappings=[('odom', 'odom_combined')],
        ),
    ])
