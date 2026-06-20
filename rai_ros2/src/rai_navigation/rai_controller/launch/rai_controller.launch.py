import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    package_dir = get_package_share_directory("rai_controller")
    default_params = os.path.join(package_dir, "config", "rai_controller.yaml")

    params = LaunchConfiguration("params")
    use_sim_time = LaunchConfiguration("use_sim_time")
    cmd_vel_topic = LaunchConfiguration("cmd_vel_topic")
    goal_topic = LaunchConfiguration("goal_topic")
    cancel_topic = LaunchConfiguration("cancel_topic")
    clear_costmap_topic = LaunchConfiguration("clear_costmap_topic")
    replan_topic = LaunchConfiguration("replan_topic")
    status_topic = LaunchConfiguration("status_topic")
    navigate_to_pose_service = LaunchConfiguration("navigate_to_pose_service")
    cancel_service = LaunchConfiguration("cancel_service")
    clear_costmap_service = LaunchConfiguration("clear_costmap_service")
    replan_service = LaunchConfiguration("replan_service")
    get_status_service = LaunchConfiguration("get_status_service")
    list_algorithms_service = LaunchConfiguration("list_algorithms_service")
    set_controller_service = LaunchConfiguration("set_controller_service")
    set_planner_service = LaunchConfiguration("set_planner_service")
    controller_id = LaunchConfiguration("controller_id")
    laser_scan_topic = LaunchConfiguration("laser_scan_topic")
    imu_topic = LaunchConfiguration("imu_topic")
    map_topic = LaunchConfiguration("map_topic")
    global_planner_algorithm = LaunchConfiguration("global_planner_algorithm")

    return LaunchDescription([
        DeclareLaunchArgument(
            "params",
            default_value=default_params,
            description="Path to rai_controller parameter file",
        ),
        DeclareLaunchArgument(
            "use_sim_time",
            default_value="false",
            description="Use simulation clock",
        ),
        DeclareLaunchArgument(
            "cmd_vel_topic",
            default_value="/rai_navigation/cmd_vel",
            description="Velocity command output topic",
        ),
        DeclareLaunchArgument(
            "goal_topic",
            default_value="/goal_pose",
            description="PoseStamped goal input topic",
        ),
        DeclareLaunchArgument(
            "cancel_topic",
            default_value="/rai_navigation/cancel_topic",
            description="Empty message topic used to cancel the active navigation task",
        ),
        DeclareLaunchArgument(
            "clear_costmap_topic",
            default_value="/rai_navigation/clear_costmap_topic",
            description="Empty message topic used to clear local obstacle grids",
        ),
        DeclareLaunchArgument(
            "replan_topic",
            default_value="/rai_navigation/replan_topic",
            description="Empty message topic used to replan to the active goal",
        ),
        DeclareLaunchArgument(
            "status_topic",
            default_value="/rai_navigation/status",
            description="String navigation status output topic",
        ),
        DeclareLaunchArgument(
            "navigate_to_pose_service",
            default_value="/rai_navigation/navigate_to_pose",
            description="Service used to submit a goal with planner/controller ids",
        ),
        DeclareLaunchArgument(
            "cancel_service",
            default_value="/rai_navigation/cancel",
            description="Service used to cancel the active navigation task",
        ),
        DeclareLaunchArgument(
            "clear_costmap_service",
            default_value="/rai_navigation/clear_costmap",
            description="Service used to clear local obstacle grids",
        ),
        DeclareLaunchArgument(
            "replan_service",
            default_value="/rai_navigation/replan",
            description="Service used to replan to the active goal",
        ),
        DeclareLaunchArgument(
            "get_status_service",
            default_value="/rai_navigation/get_status",
            description="Service used to query navigation status",
        ),
        DeclareLaunchArgument(
            "list_algorithms_service",
            default_value="/rai_navigation/list_algorithms",
            description="Service used to list available planners and controllers",
        ),
        DeclareLaunchArgument(
            "set_controller_service",
            default_value="/rai_navigation/set_controller",
            description="Service used to select active controller",
        ),
        DeclareLaunchArgument(
            "set_planner_service",
            default_value="/rai_navigation/set_planner",
            description="Service used to select active global planner",
        ),
        DeclareLaunchArgument(
            "controller_id",
            default_value="CCA_NMPC",
            description="Active controller algorithm id",
        ),
        DeclareLaunchArgument(
            "laser_scan_topic",
            default_value="/scan_filtered",
            description="LaserScan input topic used to build the standalone obstacle grid",
        ),
        DeclareLaunchArgument(
            "imu_topic",
            default_value="/imu/data",
            description="IMU input topic used for angular velocity fallback",
        ),
        DeclareLaunchArgument(
            "map_topic",
            default_value="/map",
            description="OccupancyGrid map input topic used for scanned-map navigation",
        ),
        DeclareLaunchArgument(
            "global_planner_algorithm",
            default_value="A_STAR",
            description="Global planner: A_STAR | DIJKSTRA | STRAIGHT_LINE",
        ),
        Node(
            package="rai_controller",
            executable="rai_controller_server",
            name="rai_controller",
            output="screen",
            parameters=[
                params,
                {"use_sim_time": use_sim_time},
                {"cmd_vel_topic": cmd_vel_topic},
                {"goal_topic": goal_topic},
                {"cancel_topic": cancel_topic},
                {"clear_costmap_topic": clear_costmap_topic},
                {"replan_topic": replan_topic},
                {"status_topic": status_topic},
                {"navigate_to_pose_service": navigate_to_pose_service},
                {"cancel_service": cancel_service},
                {"clear_costmap_service": clear_costmap_service},
                {"replan_service": replan_service},
                {"get_status_service": get_status_service},
                {"list_algorithms_service": list_algorithms_service},
                {"set_controller_service": set_controller_service},
                {"set_planner_service": set_planner_service},
                {"controller_id": controller_id},
                {"laser_scan_topic": laser_scan_topic},
                {"imu_topic": imu_topic},
                {"map_topic": map_topic},
                {"global_planner_algorithm": global_planner_algorithm},
            ],
        ),
    ])
