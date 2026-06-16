import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch_ros.actions import Node


def generate_launch_description():
    rai_web_api_dir = get_package_share_directory('rai_web_api')
    rai_launch_dir = get_package_share_directory('turn_on_rai_robot')
    rai_slam_dir = get_package_share_directory('rai_slam_toolbox')

    return LaunchDescription([
        # 1. Base Hardware Layer (Chassis, Lidar, IMU, EKF, TF)
        TimerAction(
            period=5.0,
            actions=[
                IncludeLaunchDescription(
                    PythonLaunchDescriptionSource(
                        os.path.join(rai_launch_dir, 'launch', 'rai_sensors.launch.py')
                    )
                ),
            ]
        ),

        # 1.5. Filter raw lidar scans once for all runtime modes.
        # SLAM, AMCL and Nav2 costmaps should consume /scan_filtered.
        TimerAction(
            period=6.0,
            actions=[
                Node(
                    package='rai_slam_toolbox',
                    executable='scan_box_filter_node',
                    name='scan_box_filter_node',
                    output='screen',
                    parameters=[
                        os.path.join(rai_slam_dir, 'config', 'laser_filter.yaml')
                    ],
                ),
            ]
        ),

        # 3. FastAPI bridge used by rai_website
        TimerAction(
            period=8.0,
            actions=[
                IncludeLaunchDescription(
                    PythonLaunchDescriptionSource(
                        os.path.join(rai_web_api_dir, 'launch', 'web_api.launch.py')
                    ),
                    launch_arguments={
                        'host': '0.0.0.0',
                        'port': '8080',
                    }.items(),
                ),
            ]
        ),
    ])
