from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    perception_config = LaunchConfiguration("perception_config")
    kalman_config = LaunchConfiguration("kalman_config")
    use_sim_time = LaunchConfiguration("use_sim_time")

    default_perception_config = PathJoinSubstitution([
        FindPackageShare("rai_human_perception"),
        "config",
        "perception.yaml",
    ])
    default_kalman_config = PathJoinSubstitution([
        FindPackageShare("rai_human_perception"),
        "config",
        "kalman.yaml",
    ])

    return LaunchDescription([
        DeclareLaunchArgument("perception_config", default_value=default_perception_config),
        DeclareLaunchArgument("kalman_config", default_value=default_kalman_config),
        DeclareLaunchArgument("use_sim_time", default_value="false"),
        Node(
            package="rai_human_perception",
            executable="perception_node",
            name="rai_human_perception",
            output="screen",
            parameters=[
                perception_config,
                kalman_config,
                {"use_sim_time": use_sim_time},
            ],
        ),
    ])
