import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, ExecuteProcess, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    rai_scada_bridge_dir = get_package_share_directory('rai_scada_bridge')
    rai_web_api_dir = get_package_share_directory('rai_web_api')
    rai_launch_dir = get_package_share_directory('turn_on_rai_robot')
    rai_slam_dir = get_package_share_directory('rai_slam_toolbox')

    adaptive_host_arg = DeclareLaunchArgument(
        'adaptive_host',
        default_value='25.12.4.100',
        description='IP address or hostname of the adaptive runtime on Jetson',
    )

    return LaunchDescription([
        adaptive_host_arg,

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

        # 3. SCADA ZMQ Bridge (ROS2 <-> Web Server)
        # NOTE: Nav2 đã được tắt — chế độ bám người KHÔNG cần map/global_costmap
        TimerAction(
            period=7.0,
            actions=[
                IncludeLaunchDescription(
                    PythonLaunchDescriptionSource(
                        os.path.join(rai_scada_bridge_dir, 'launch', 'scada_bridge.launch.py')
                    )
                ),
            ]
        ),

        # 4. FastAPI bridge used by rai_website
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
