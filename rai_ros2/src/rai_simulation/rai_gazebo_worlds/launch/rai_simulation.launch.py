import os
import xml.etree.ElementTree as ET

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, IncludeLaunchDescription, OpaqueFunction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def _as_bool(value: str) -> bool:
    return value.strip().lower() in {"1", "true", "yes", "on"}


def _resolve_world_path(world_value: str) -> str:
    worlds_dir = os.path.join(get_package_share_directory("rai_gazebo_worlds"), "worlds")
    if os.path.isabs(world_value) and os.path.exists(world_value):
        return world_value
    candidate = os.path.join(worlds_dir, world_value)
    if os.path.exists(candidate):
        return candidate
    if not world_value.endswith(".world"):
        candidate = os.path.join(worlds_dir, f"{world_value}.world")
        if os.path.exists(candidate):
            return candidate
    raise RuntimeError(f"Simulation world not found: {world_value}")


def _world_name(world_path: str) -> str:
    try:
        root = ET.parse(world_path).getroot()
        world = root.find("world")
        if world is not None:
            return world.attrib.get("name", "default")
    except Exception:
        pass
    return os.path.splitext(os.path.basename(world_path))[0]


def _resolve_robot_urdf(robot_model: str) -> str:
    urdf_path = os.path.join(
        get_package_share_directory("rai_robot_urdf"),
        "urdf",
        f"{robot_model}.urdf",
    )
    if not os.path.exists(urdf_path):
        raise RuntimeError(f"Robot URDF not found: {urdf_path}")
    return urdf_path


def _launch_simulation(context):
    world_path = _resolve_world_path(context.perform_substitution(LaunchConfiguration("world")))
    robot_model = context.perform_substitution(LaunchConfiguration("robot_model"))
    robot_name = context.perform_substitution(LaunchConfiguration("robot_name"))
    use_sim_time = context.perform_substitution(LaunchConfiguration("use_sim_time"))
    headless = _as_bool(context.perform_substitution(LaunchConfiguration("headless")))
    start_slam = _as_bool(context.perform_substitution(LaunchConfiguration("start_slam")))
    start_navigation = _as_bool(context.perform_substitution(LaunchConfiguration("start_navigation")))
    params = context.perform_substitution(LaunchConfiguration("params"))
    map_yaml = context.perform_substitution(LaunchConfiguration("map"))
    world_name = _world_name(world_path)
    urdf_path = _resolve_robot_urdf(robot_model)

    with open(urdf_path, "r", encoding="utf-8") as handle:
        robot_description = handle.read()

    gazebo_cmd = ["ign", "gazebo", world_path, "-r"]
    if headless:
        gazebo_cmd.append("-s")

    actions = [
        ExecuteProcess(cmd=gazebo_cmd, output="screen"),
        Node(
            package="robot_state_publisher",
            executable="robot_state_publisher",
            name="robot_state_publisher",
            output="screen",
            parameters=[{"robot_description": robot_description, "use_sim_time": use_sim_time}],
        ),
        Node(
            package="ros_gz_sim",
            executable="create",
            name="rai_sim_spawn_robot",
            output="screen",
            arguments=[
                "-world",
                world_name,
                "-file",
                urdf_path,
                "-name",
                robot_name,
                "-x",
                context.perform_substitution(LaunchConfiguration("x")),
                "-y",
                context.perform_substitution(LaunchConfiguration("y")),
                "-z",
                context.perform_substitution(LaunchConfiguration("z")),
                "-Y",
                context.perform_substitution(LaunchConfiguration("yaw")),
            ],
        ),
        Node(
            package="ros_gz_bridge",
            executable="parameter_bridge",
            name="rai_sim_gz_bridge",
            output="screen",
            arguments=[
                "/clock@rosgraph_msgs/msg/Clock[ignition.msgs.Clock",
                "/scan@sensor_msgs/msg/LaserScan[ignition.msgs.LaserScan",
                f"/model/{robot_name}/odometry@nav_msgs/msg/Odometry[ignition.msgs.Odometry",
                "/cmd_vel@geometry_msgs/msg/Twist]ignition.msgs.Twist",
                "/tf@tf2_msgs/msg/TFMessage[ignition.msgs.Pose_V",
                "/tf_static@tf2_msgs/msg/TFMessage[ignition.msgs.Pose_V",
            ],
            remappings=[(f"/model/{robot_name}/odometry", "/odom_combined")],
        ),
    ]

    if start_slam:
        actions.append(
            Node(
                package="slam_toolbox",
                executable="async_slam_toolbox_node",
                name="slam_toolbox",
                output="screen",
                parameters=[
                    os.path.join(
                        get_package_share_directory("rai_slam_toolbox"),
                        "config",
                        "mapper_params_online_async.yaml",
                    ),
                    {"use_sim_time": use_sim_time},
                ],
                remappings=[("odom", "odom_combined")],
            )
        )

    if start_navigation:
        actions.append(
            IncludeLaunchDescription(
                PythonLaunchDescriptionSource(
                    os.path.join(
                        get_package_share_directory("rai_navigation"),
                        "launch",
                        "rai_navigation.launch.py",
                    )
                ),
                launch_arguments={
                    "params": params,
                    "map": map_yaml,
                    "use_sim_time": use_sim_time,
                    "cmd_vel_topic": "/cmd_vel",
                    "map_topic": "/map",
                    "laser_scan_topic": "/scan",
                    "imu_topic": "/imu/data",
                }.items(),
            )
        )

    return actions


def generate_launch_description():
    default_world = os.path.join(get_package_share_directory("rai_gazebo_worlds"), "worlds", "s1_open_zone.world")
    default_params = os.path.join(get_package_share_directory("rai_navigation"), "config", "rai_navigation.yaml")

    return LaunchDescription([
        DeclareLaunchArgument("world", default_value=default_world, description="World id/name or absolute .world path"),
        DeclareLaunchArgument("robot_model", default_value="mini_mec_robot", description="URDF model id from rai_robot_urdf/urdf"),
        DeclareLaunchArgument("robot_name", default_value="rai_sim_robot", description="Gazebo model name"),
        DeclareLaunchArgument("x", default_value="0.0"),
        DeclareLaunchArgument("y", default_value="0.0"),
        DeclareLaunchArgument("z", default_value="0.05"),
        DeclareLaunchArgument("yaw", default_value="0.0"),
        DeclareLaunchArgument("use_sim_time", default_value="true"),
        DeclareLaunchArgument("headless", default_value="false"),
        DeclareLaunchArgument("start_slam", default_value="true"),
        DeclareLaunchArgument("start_navigation", default_value="false"),
        DeclareLaunchArgument("params", default_value=default_params),
        DeclareLaunchArgument("map", default_value=""),
        OpaqueFunction(function=_launch_simulation),
    ])
