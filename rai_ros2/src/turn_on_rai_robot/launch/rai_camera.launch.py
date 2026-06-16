import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.launch_description_sources import AnyLaunchDescriptionSource
from launch.actions import IncludeLaunchDescription
from launch.substitutions import LaunchConfiguration


def _camera_actions(context, *_args, **_kwargs):
    camera_type = context.perform_substitution(LaunchConfiguration("camera_type")).strip().lower()
    astra_dir = get_package_share_directory("astra_camera")
    astra_launch_dir = os.path.join(astra_dir, "launch")

    if camera_type != "astra_s":
        raise RuntimeError(f"Unsupported camera_type: {camera_type}")

    return [
        IncludeLaunchDescription(
            AnyLaunchDescriptionSource(os.path.join(astra_launch_dir, "astra.launch.xml")),
            launch_arguments={
                "camera_name": "camera",
                "enable_color": "true",
                "color_width": "640",
                "color_height": "480",
                "color_fps": "30",
                "enable_depth": "false",
                "depth_width": "640",
                "depth_height": "480",
                "depth_fps": "15",
                "enable_ir": "false",
                "enable_point_cloud": "false",
                "enable_colored_point_cloud": "false",
                "depth_registration": "false",
                "color_depth_synchronization": "false",
                "enable_d2c_viewer": "false",
                "enable_publish_extrinsic": "false",
                "publish_tf": "true",
                "tf_publish_rate": "10.0",
                "oni_log_level": "error",
                "oni_log_to_console": "false",
                "oni_log_to_file": "false",
            }.items(),
        )
    ]


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            "camera_type",
            default_value="astra_s",
            description="astra_s",
        ),
        DeclareLaunchArgument(
            "enable_depth",
            default_value="false",
            description="Enable Astra depth stream when explicitly needed.",
        ),
        OpaqueFunction(function=_camera_actions),
    ])
