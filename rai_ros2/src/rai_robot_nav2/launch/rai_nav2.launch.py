import os
from ament_index_python.packages import get_package_share_directory
from launch_ros.actions import Node
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration

def generate_launch_description():
    use_sim_time = LaunchConfiguration('use_sim_time', default='false')
    slam = LaunchConfiguration('slam', default='False')

    rai_robot_dir = get_package_share_directory('turn_on_rai_robot')
    rai_launch_dir = os.path.join(rai_robot_dir, 'launch')
        
    rai_nav_dir = get_package_share_directory('rai_nav2')
    rai_nav_launchr = os.path.join(rai_nav_dir, 'launch')

    map_default = '/home/rai/rai_ros2/data/map/RAI.yaml'
    if not os.path.exists(map_default):
        map_default = os.path.join(rai_nav_dir, 'map', 'RAI.yaml')
    map_file = LaunchConfiguration('map', default=map_default)

    #Modify the model parameter file, the options are:
    #param_mini_akm.yaml/param_mini_4wd.yaml/param_mini_diff.yaml/
    #param_mini_mec.yaml/param_mini_omni.yaml/param_mini_tank.yaml/
    #param_senior_akm.yaml/param_senior_diff.yaml/param_senior_mec_bs.yaml
    #param_senior_mec_dl.yaml/param_top_4wd_bs.yaml/param_top_4wd_dl.yaml
    #param_top_akm_dl.yaml/param_four_wheel_diff_dl.yaml/param_four_wheel_diff_bs.yaml

    param_dir = os.path.join(rai_nav_dir, 'param','rai_params')
    param_file = LaunchConfiguration('params', default=os.path.join(
        param_dir, 'param_mini_mec.yaml'))


    return LaunchDescription([
        DeclareLaunchArgument(
            'map',
            default_value=map_file,
            description='Full path to map file to load'),

        DeclareLaunchArgument(
            'params',
            default_value=param_file,
            description='Full path to param file to load'),

        DeclareLaunchArgument(
            'slam',
            default_value='False',
            description='Run SLAM if true, localization with map if false'),
        Node(
            name='waypoint_cycle',
            package='nav2_waypoint_cycle',
            executable='nav2_waypoint_cycle',
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                [rai_nav_launchr, '/bringup_launch.py']),
            launch_arguments={
                'map': map_file,
                'slam': slam,
                'use_sim_time': use_sim_time,
                'params_file': param_file}.items(),
        ),

    ])
