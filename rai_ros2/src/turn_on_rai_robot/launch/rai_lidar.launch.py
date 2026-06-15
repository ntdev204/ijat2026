#!/usr/bin/env python3
"""Launch RPLidar A1M8 for the Rai robot."""

from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():

    return LaunchDescription([
        # RPLidar A1M8 node
        Node(
            package='rplidar_ros',
            executable='rplidar_node',
            name='rplidar_node',
            parameters=[{
                'serial_port':      '/dev/rai_lidar',
                'serial_baudrate':  115200,
                'frame_id':         'laser',
                'angle_compensate': True,
            }],
            output='screen',
        ),
    ])
