from datetime import datetime

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
import launch_ros.actions


def generate_launch_description():
    default_map_name = datetime.now().strftime('RAI_%Y%m%d_%H%M%S_%f')
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
        default_value='/home/rai/rai_ros2/data/map',
        description='Directory for saved map files',
    ))
    ld.add_action(DeclareLaunchArgument(
        'nav2_map_dir',
        default_value='/home/rai/rai_ros2/src/rai_robot_nav2/map',
        description='Backup directory for Nav2 package map files',
    ))
    ld.add_action(DeclareLaunchArgument(
        'map_name',
        default_value=default_map_name,
        description='Map basename without extension. Use map_name:=RAI to intentionally overwrite RAI.',
    ))
    ld.add_action(map_saver)
    ld.add_action(map_backup)
    return ld
 
