from datetime import datetime
from pathlib import Path
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
import launch_ros.actions


def _default_dirs():
    package_share = Path(get_package_share_directory('rai_nav2')).resolve()
    workspace_dir = package_share.parents[3] if len(package_share.parents) > 3 else package_share
    source_map_dir = workspace_dir / 'src' / 'rai_robot_nav2' / 'map'

    return (
        str(workspace_dir / 'data' / 'map'),
        str(source_map_dir if source_map_dir.exists() else package_share / 'map'),
    )


def _ensure_map_dirs(context, *args, **kwargs):
    for config_name in ('map_dir', 'nav2_map_dir'):
        path = context.perform_substitution(LaunchConfiguration(config_name))
        os.makedirs(path, exist_ok=True)
    return []


def generate_launch_description():
    default_map_name = datetime.now().strftime('RAI_%Y%m%d_%H%M%S_%f')
    default_map_dir, default_nav2_map_dir = _default_dirs()
    map_dir = LaunchConfiguration('map_dir')
    nav2_map_dir = LaunchConfiguration('nav2_map_dir')
    map_name = LaunchConfiguration('map_name')

    map_saver = launch_ros.actions.Node(
        package='nav2_map_server',
        executable='map_saver_cli',
        output='screen',
        arguments=['-f', PathJoinSubstitution([map_dir, map_name])],
        
        parameters=[{'save_map_timeout': 20000.0},
                    {'free_thresh_default': 0.196}]

        )
    map_backup = launch_ros.actions.Node(
        package='nav2_map_server',
        executable='map_saver_cli',
        name='map_backup',
        output='screen',
        arguments=['-f', PathJoinSubstitution([nav2_map_dir, map_name])],
        
        parameters=[{'save_map_timeout': 20000.0},
                    {'free_thresh_default': 0.196}]

    )
    ld = LaunchDescription()

    ld.add_action(DeclareLaunchArgument(
        'map_dir',
        default_value=default_map_dir,
        description='Directory for saved map files',
    ))
    ld.add_action(DeclareLaunchArgument(
        'nav2_map_dir',
        default_value=default_nav2_map_dir,
        description='Backup directory for Nav2 package map files',
    ))
    ld.add_action(DeclareLaunchArgument(
        'map_name',
        default_value=default_map_name,
        description='Map basename without extension. Use map_name:=RAI to intentionally overwrite RAI.',
    ))
    ld.add_action(OpaqueFunction(function=_ensure_map_dirs))
    ld.add_action(map_saver)
    ld.add_action(map_backup)
    return ld
 
