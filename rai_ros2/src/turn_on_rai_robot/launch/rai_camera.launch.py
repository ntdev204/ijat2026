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
        )
    ]


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            "camera_type",
            default_value="astra_s",
            description="astra_s",
        ),
        OpaqueFunction(function=_camera_actions),
    ])
