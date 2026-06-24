"""
Dataset Collection Hardware Bringup Launch File
Khởi động hardware (sensors, motors) để thu thập dataset CCA-NMPC
Được gọi từ rai_dataset_collection package
"""

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    IncludeLaunchDescription,
    ExecuteProcess,
    TimerAction,
    LogInfo
)
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    """
    Launch hardware layer for dataset collection:
    1. Cleanup existing processes
    2. Start chassis + motors
    3. Start LiDAR
    4. Start RGB-D camera
    5. Start LiDAR scan filter
    5. NO Navigation/SLAM (not needed for data collection)
    """

    
    rai_launch_dir = get_package_share_directory('turn_on_rai_robot')
    rai_slam_dir = get_package_share_directory('rai_slam_toolbox')

    
    dataset_mode_arg = DeclareLaunchArgument(
        'dataset_mode',
        default_value='collection',
        description='Dataset collection mode: collection | validation'
    )

    
    cleanup_processes = ExecuteProcess(
        cmd=['bash', '-c',
             'echo "[dataset_collection] Cleaning up existing processes..."; '
             'pkill -9 -x "rai_robot_node" 2>/dev/null || true; '
             'pkill -9 -x "lslidar_driver_node" 2>/dev/null || true; '
             'pkill -9 -x "ekf_node" 2>/dev/null || true; '
             'pkill -9 -f "slam_toolbox" 2>/dev/null || true; '
             'pkill -9 -f "rai_controller" 2>/dev/null || true; '
             'pkill -9 -f "rai_navigation" 2>/dev/null || true; '
             'pkill -9 -f "astra" 2>/dev/null || true; '
             'echo "[dataset_collection] Waiting 2 seconds..."; '
             'sleep 2; '
             'echo "[dataset_collection] Cleanup complete."'],
        output='screen',
    )

    
    log_start = LogInfo(
        msg='[dataset_collection] Starting hardware layer for CCA-NMPC dataset collection...'
    )

    
    hardware_layer = TimerAction(
        period=3.0,
        actions=[
            LogInfo(msg='[dataset_collection] Launching base hardware (chassis, sensors)...'),
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    os.path.join(rai_launch_dir, 'launch', 'turn_on_rai_robot.launch.py')
                ),
                launch_arguments={}.items()
            ),
        ]
    )

    
    lidar_node = TimerAction(
        period=5.0,
        actions=[
            LogInfo(msg='[dataset_collection] Launching LiDAR...'),
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    os.path.join(rai_launch_dir, 'launch', 'rai_lidar.launch.py')
                ),
                launch_arguments={}.items()
            ),
        ]
    )

    
    lidar_filter = TimerAction(
        period=7.0,
        actions=[
            LogInfo(msg='[dataset_collection] Starting LiDAR scan filter...'),
            Node(
                package='rai_slam_toolbox',
                executable='scan_box_filter_node',
                name='scan_box_filter_node',
                output='screen',
                parameters=[
                    os.path.join(rai_slam_dir, 'config', 'laser_filter.yaml')
                ],
                remappings=[
                    ('/scan', '/scan'),
                    ('/scan_filtered', '/scan_filtered')
                ]
            ),
        ]
    )

    
    
    
    camera_node = TimerAction(
        period=6.0,
        actions=[
            LogInfo(msg='[dataset_collection] Starting Astra RGB-D camera...'),
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    os.path.join(rai_launch_dir, 'launch', 'rai_camera.launch.py')
                ),
                launch_arguments={
                    'enable_depth': 'true',
                }.items()
            ),
        ]
    )

    
    
    camera_tf = TimerAction(
        period=7.0,
        actions=[
            LogInfo(msg='[dataset_collection] Publishing camera TF...'),
            Node(
                package='tf2_ros',
                executable='static_transform_publisher',
                name='camera_base_tf',
                arguments=[
                    '0.15', '0.0', '0.25',  
                    '0.0', '0.105', '0.0',   
                    'base_link',
                    'camera_link'
                ],
                output='screen'
            ),
        ]
    )

    
    complete_log = TimerAction(
        period=10.0,
        actions=[
            LogInfo(
                msg='[dataset_collection] ✅ Hardware layer ready! '
                    'Sensors: LiDAR, Camera, Encoders, IMU, TF. '
                    'Waiting for dataset collector nodes...'
            ),
        ]
    )

    return LaunchDescription([
        
        dataset_mode_arg,

        
        log_start,

        
        cleanup_processes,

        
        hardware_layer,

        
        lidar_node,

        
        lidar_filter,

        
        camera_node,

        
        camera_tf,

        
        complete_log,
    ])
