from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    weights = LaunchConfiguration("weights")
    output = LaunchConfiguration("output")
    imgsz = LaunchConfiguration("imgsz")
    fp16 = LaunchConfiguration("fp16")

    script = PathJoinSubstitution([
        FindPackageShare("rai_human_perception"),
        "scripts",
        "export_engine.py",
    ])

    return LaunchDescription([
        DeclareLaunchArgument("weights", default_value="best.pt"),
        DeclareLaunchArgument("output", default_value="model.engine"),
        DeclareLaunchArgument("imgsz", default_value="640"),
        DeclareLaunchArgument("fp16", default_value="true"),
        ExecuteProcess(
            cmd=[
                "python3",
                script,
                "--weights",
                weights,
                "--imgsz",
                imgsz,
                "--output",
                output,
                "--fp16",
            ],
            output="screen",
        ),
    ])
