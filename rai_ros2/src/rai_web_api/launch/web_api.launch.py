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
    cca_prefix = install_root / 'rai_ccanmpc_controller'
    cca_python_path = cca_prefix / 'local' / 'lib' / f'python{sys.version_info.major}.{sys.version_info.minor}' / 'dist-packages'
    cca_library_path = cca_prefix / 'lib'

    host_arg = DeclareLaunchArgument(
        'host',
        default_value='0.0.0.0',
        description='Host address for Rai FastAPI server'
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
            'PYTHONPATH': _prepend_env('PYTHONPATH', str(cca_python_path)),
            'LD_LIBRARY_PATH': _prepend_env('LD_LIBRARY_PATH', str(cca_library_path)),
        },
    )

    return LaunchDescription([
        host_arg,
        port_arg,
        web_api_node,
    ])
