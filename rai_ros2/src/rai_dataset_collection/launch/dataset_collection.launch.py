"""
Main Dataset Collection Launch File
Khởi động toàn bộ hệ thống thu thập dataset CA-NMPC
"""

import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    IncludeLaunchDescription,
    TimerAction,
    LogInfo
)
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    """
    Launch toàn bộ dataset collection system:
    1. Hardware layer (từ turn_on_rai_robot)
    2. Continuous context estimator (phi_hat_h + adaptive bounds)
    3. Dataset Collector (raw ROS 2 bags + metadata)

    Note: Manual control mode - user drives robot following MANUAL_SCENARIOS.md
    """

    # Package directories
    rai_robot_dir = get_package_share_directory('turn_on_rai_robot')

    # Launch arguments
    scenario_arg = DeclareLaunchArgument(
        'scenario',
        default_value='S1_open_zone',
        description='Manual protocol scenario label: S1_open_zone ... S5_occlusion_sudden_appearance'
    )

    controller_arg = DeclareLaunchArgument(
        'controller',
        default_value='CCA_NMPC',
        description='Controller id for dataset organization'
    )

    environment_arg = DeclareLaunchArgument(
        'environment',
        default_value='real',
        description='Dataset environment: sim or real'
    )

    split_arg = DeclareLaunchArgument(
        'split',
        default_value='unsplit',
        description='Dataset split tag: train, val, test, or unsplit'
    )

    run_id_arg = DeclareLaunchArgument(
        'run_id',
        default_value='',
        description='Optional deterministic run id such as run_000'
    )

    auto_start_arg = DeclareLaunchArgument(
        'auto_start',
        default_value='true',
        description='Auto-start recording'
    )

    # Get launch configurations
    scenario = LaunchConfiguration('scenario')
    controller = LaunchConfiguration('controller')
    environment = LaunchConfiguration('environment')
    split = LaunchConfiguration('split')
    run_id = LaunchConfiguration('run_id')
    auto_start = LaunchConfiguration('auto_start')

    # Log banner
    log_banner = LogInfo(
        msg='\n'
            '╔════════════════════════════════════════════════════════════╗\n'
            '║   CCA-NMPC Dataset Collection System                       ║\n'
            '║   Manual S1-S5 protocol: continuous phi_h + Mecanum flow   ║\n'
            '╚════════════════════════════════════════════════════════════╝\n'
    )

    # 1. Hardware Layer (from turn_on_rai_robot)
    # Khởi động: Chassis, LiDAR, Camera, IMU, Encoders
    hardware_launch = TimerAction(
        period=2.0,
        actions=[
            LogInfo(msg='[MAIN] Launching hardware layer...'),
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    os.path.join(rai_robot_dir, 'launch', 'dataset_collection.launch.py')
                )
            ),
        ]
    )

    # 2. Continuous context estimator
    context_monitor = TimerAction(
        period=12.0,  # Wait for hardware to stabilize
        actions=[
            LogInfo(msg='[MAIN] Launching continuous context estimator...'),
            Node(
                package='rai_dataset_collection',
                executable='context_monitor',
                name='context_monitor',
                output='screen',
                parameters=[{
                    'use_sim_time': False,
                    'publish_legacy_context': True,
                }]
            ),
        ]
    )

    # 3. Dataset Collector
    # Record sensor data to ROS2 bag files
    dataset_collector = TimerAction(
        period=14.0,
        actions=[
            LogInfo(msg='[MAIN] Launching Dataset Collector...'),
            Node(
                package='rai_dataset_collection',
                executable='dataset_collector',
                name='dataset_collector',
                output='screen',
                parameters=[{
                    'scenario_id': scenario,
                    'controller_id': controller,
                    'environment': environment,
                    'split': split,
                    'run_id': run_id,
                    'auto_start': auto_start,
                    'base_path': os.path.expanduser('~/rai_datasets/canmpc'),
                }]
            ),
        ]
    )

    # Ready message
    ready_log = TimerAction(
        period=18.0,
        actions=[
            LogInfo(
                msg='\n'
                    '✅ Dataset Collection System READY!\n'
                    '📹 Recording to: ~/rai_datasets/canmpc/\n'
                    '📊 Recording /canmpc/context, /canmpc/humans, adaptive bounds\n'
                    '🎮 Execute S1-S5 manually. Map-based automation is disabled.\n'
                    '\n'
                    '🚀 Starting data collection...\n'
            ),
        ]
    )

    return LaunchDescription([
        # Arguments
        scenario_arg,
        controller_arg,
        environment_arg,
        split_arg,
        run_id_arg,
        auto_start_arg,

        # Banner
        log_banner,

        # 1. Hardware layer (sensors, motors)
        hardware_launch,

        # 2. Context detection
        context_monitor,

        # 3. Data recording
        dataset_collector,

        # Ready message
        ready_log,
    ])
