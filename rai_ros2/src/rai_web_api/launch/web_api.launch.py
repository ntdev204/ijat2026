import os
import sys
from pathlib import Path

from ament_index_python.packages import get_package_prefix
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def _prepend_env(name, value):
    current = os.environ.get(name, "")
    return value if not current else f"{value}{os.pathsep}{current}"


def generate_launch_description():
    install_root = Path(get_package_prefix('rai_web_api')).resolve().parent
    controller_prefix = install_root / 'rai_controller'
    controller_python_path = controller_prefix / 'local' / 'lib' / f'python{sys.version_info.major}.{sys.version_info.minor}' / 'dist-packages'
    controller_library_path = controller_prefix / 'lib'
    workspace_root = Path(os.getenv('RAI_WORKSPACE_ROOT', str(Path.home() / 'ijat2026')))
    rai_ros2_data_dir = workspace_root / 'rai_ros2' / 'data'

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
            'RAI_NAVIGATION_MAP': str(rai_ros2_data_dir / 'map' / 'RAI.yaml'),
            'RAI_MAP_STORAGE_DIR': str(rai_ros2_data_dir / 'map'),
            'RAI_WORLD_STORAGE_DIR': str(rai_ros2_data_dir / 'worlds'),
            'PYTHONPATH': _prepend_env('PYTHONPATH', str(controller_python_path)),
            'LD_LIBRARY_PATH': _prepend_env('LD_LIBRARY_PATH', str(controller_library_path)),
        },
    )

    return LaunchDescription([
        host_arg,
        lan_host_arg,
        port_arg,
        web_api_node,
    ])
