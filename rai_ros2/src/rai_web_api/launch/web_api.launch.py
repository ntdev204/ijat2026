import os
from pathlib import Path

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    workspace_root = Path(os.getenv('RAI_WORKSPACE_ROOT', str(Path.home() / 'ijat2026')))
    rai_ros2_data_dir = workspace_root / 'rai_ros2' / 'data'
    dataset_dir = Path(os.getenv('RAI_DATASET_PATH', str(workspace_root / 'dataset')))

    host_arg = DeclareLaunchArgument(
        'host',
        default_value=os.getenv('RAI_API_HOST', '0.0.0.0'),
        description='Host address for Rai FastAPI server'
    )
    lan_host_arg = DeclareLaunchArgument(
        'lan_host',
        default_value=os.getenv('RAI_LAN_HOST', os.getenv('RAI_SERVER_HOST', 'localhost')),
        description='Browser-facing host for Rai FastAPI server'
    )
    port_arg = DeclareLaunchArgument(
        'port',
        default_value='8080',
        description='Port for Rai FastAPI server'
    )

    web_api_node = Node(
        package='rai_web_api',
        executable='web_api',
        name='rai_web_api',
        output='screen',
        parameters=[
            {'cmd_vel_topic': '/cmd_vel_web'},
        ],
        additional_env={
            'RAI_API_HOST': LaunchConfiguration('host'),
            'RAI_API_PORT': LaunchConfiguration('port'),
            'RAI_LAN_HOST': LaunchConfiguration('lan_host'),
            'RAI_WORKSPACE_ROOT': str(workspace_root),
            'RAI_DATASET_PATH': str(dataset_dir),
            'RAI_NAVIGATION_MAP': str(rai_ros2_data_dir / 'map' / 'RAI.yaml'),
            'RAI_MAP_STORAGE_DIR': str(rai_ros2_data_dir / 'map'),
            'RAI_WORLD_STORAGE_DIR': str(rai_ros2_data_dir / 'worlds'),
        },
    )

    return LaunchDescription([
        host_arg,
        lan_host_arg,
        port_arg,
        web_api_node,
    ])
