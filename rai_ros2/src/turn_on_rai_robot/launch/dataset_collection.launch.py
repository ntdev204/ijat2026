"""
Dataset Collection Hardware Bringup Launch File
Khởi động hardware (sensors, motors) để thu thập dataset CA-NMPC
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
    2. Start chassis + motors (rai_sensors)
    3. Start LiDAR scan filter
    4. Start RGB-D camera
    5. NO Navigation/SLAM (not needed for data collection)
    """

    # Package directories
    rai_launch_dir = get_package_share_directory('turn_on_rai_robot')
    rai_slam_dir = get_package_share_directory('rai_slam_toolbox')

    # Launch arguments
    dataset_mode_arg = DeclareLaunchArgument(
        'dataset_mode',
        default_value='collection',
        description='Dataset collection mode: collection | validation'
    )

    # Cleanup: Kill existing ROS2 processes (same as prod_bringup)
    cleanup_processes = ExecuteProcess(
        cmd=['bash', '-c',
             'echo "[dataset_collection] Cleaning up existing processes..."; '
             'pkill -9 -x "rai_robot_node" 2>/dev/null || true; '
             'pkill -9 -x "lslidar_driver_node" 2>/dev/null || true; '
             'pkill -9 -x "ekf_node" 2>/dev/null || true; '
             'pkill -9 -f "slam_toolbox" 2>/dev/null || true; '
             'pkill -9 -f "nav2" 2>/dev/null || true; '
             'pkill -9 -f "ldlidar" 2>/dev/null || true; '
             'pkill -9 -f "astra" 2>/dev/null || true; '
             'echo "[dataset_collection] Waiting 2 seconds..."; '
             'sleep 2; '
             'echo "[dataset_collection] Cleanup complete."'],
        output='screen',
    )

    # Log start message
    log_start = LogInfo(
        msg='[dataset_collection] Starting hardware layer for CA-NMPC dataset collection...'
    )

    # 1. Base Hardware Layer (Chassis, Lidar, IMU, Encoders, TF)
    # Same as prod_bringup rai_sensors
    hardware_layer = TimerAction(
        period=3.0,
        actions=[
            LogInfo(msg='[dataset_collection] Launching base hardware (chassis, sensors)...'),
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    os.path.join(rai_launch_dir, 'launch', 'rai_sensors.launch.py')
                ),
                launch_arguments={}.items()
            ),
        ]
    )

    # 2. LiDAR Scan Filter
    # Filters raw scans to remove robot body reflections
    # Output: /scan_filtered (used by context detector)
    lidar_filter = TimerAction(
        period=5.0,
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

    # 3. RGB-D Camera (Astra Camera)
    # Provides RGB + Depth for YOLO v8 human detection
    # Topics: /camera/color/image_raw, /camera/depth/image_raw
    camera_node = TimerAction(
        period=6.0,
        actions=[
            LogInfo(msg='[dataset_collection] Starting Astra RGB-D camera...'),
            Node(
                package='astra_camera',
                executable='astra_camera_node',
                name='astra_camera',
                output='screen',
                parameters=[{
                    'color_width': 640,
                    'color_height': 480,
                    'color_fps': 30,
                    'depth_width': 640,
                    'depth_height': 480,
                    'depth_fps': 30,
                    'enable_color': True,
                    'enable_depth': True,
                    'enable_infra': False,
                    'depth_registration': True,  # Align depth to color frame
                }],
                remappings=[
                    ('/camera/color/image_raw', '/camera/color/image_raw'),
                    ('/camera/depth/image_raw', '/camera/depth/image_raw'),
                ]
            ),
        ]
    )

    # 4. Static TF: Camera → Base Link
    # Camera mounted at (0.15, 0.0, 0.25) with 6° depression angle
    camera_tf = TimerAction(
        period=7.0,
        actions=[
            LogInfo(msg='[dataset_collection] Publishing camera TF...'),
            Node(
                package='tf2_ros',
                executable='static_transform_publisher',
                name='camera_base_tf',
                arguments=[
                    '0.15', '0.0', '0.25',  # x, y, z (meters)
                    '0.0', '0.105', '0.0',   # roll, pitch(6°), yaw (radians)
                    'base_link',
                    'camera_link'
                ],
                output='screen'
            ),
        ]
    )

    # 5. Completion Log
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
        # Arguments
        dataset_mode_arg,

        # Log
        log_start,

        # 0. Cleanup
        cleanup_processes,

        # 1. Hardware layer (chassis, motors, lidar, imu, encoders)
        hardware_layer,

        # 2. LiDAR filter
        lidar_filter,

        # 3. RGB-D camera
        camera_node,

        # 4. Camera TF
        camera_tf,

        # 5. Completion message
        complete_log,
    ])
