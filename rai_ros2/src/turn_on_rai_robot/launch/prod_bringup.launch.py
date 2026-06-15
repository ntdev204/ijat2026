import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, ExecuteProcess, TimerAction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    rai_scada_bridge_dir = get_package_share_directory('rai_scada_bridge')
    rai_launch_dir = get_package_share_directory('turn_on_rai_robot')
    rai_slam_dir = get_package_share_directory('rai_slam_toolbox')

    adaptive_host_arg = DeclareLaunchArgument(
        'adaptive_host',
        default_value='25.12.4.100',
        description='IP address or hostname of the adaptive runtime on Jetson',
    )

    # Cleanup: Kill all existing ROS2 processes and nodes (no service stop)
    cleanup_all = ExecuteProcess(
        cmd=['bash', '-c',
             'echo "[cleanup] Killing ROS2 processes..."; '
             'pkill -9 -x "rai_robot_node" 2>/dev/null || true; '
             'pkill -9 -x "lslidar_driver_node" 2>/dev/null || true; '
             'pkill -9 -x "scada_bridge" 2>/dev/null || true; '
             'pkill -9 -x "ekf_node" 2>/dev/null || true; '
             'pkill -9 -f "slam_toolbox" 2>/dev/null || true; '
             'pkill -9 -f "nav2" 2>/dev/null || true; '
             'pkill -9 -f "amcl" 2>/dev/null || true; '
             'pkill -9 -f "bt_navigator" 2>/dev/null || true; '
             'pkill -9 -f "controller_server" 2>/dev/null || true; '
             'pkill -9 -f "planner_server" 2>/dev/null || true; '
             'pkill -9 -f "ldlidar" 2>/dev/null || true; '
             'echo "[cleanup] Waiting for processes to terminate..."; '
             'sleep 2; '
             'echo "[cleanup] Cleanup complete."'],
        output='screen',
    )

    return LaunchDescription([
        adaptive_host_arg,

        # 0. Cleanup: Kill all processes and nodes
        cleanup_all,

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
    ])
