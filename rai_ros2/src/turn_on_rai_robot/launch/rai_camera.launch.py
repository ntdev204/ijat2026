import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import AnyLaunchDescriptionSource, PythonLaunchDescriptionSource


def generate_launch_description():
    astra_dir = get_package_share_directory("astra_camera")
    astra_launch_dir = os.path.join(astra_dir, "launch")

    usb_cam_dir = get_package_share_directory("usb_cam")
    usb_cam_launch_dir = os.path.join(usb_cam_dir, "launch")

    video_device_arg = DeclareLaunchArgument(
        "video_device",
        default_value="/dev/video0",
        description="video device serial number.",
    )

    astra_s = IncludeLaunchDescription(
        AnyLaunchDescriptionSource(os.path.join(astra_launch_dir, "astra.launch.xml")),
    )

    astra_pro = IncludeLaunchDescription(
        AnyLaunchDescriptionSource(os.path.join(astra_launch_dir, "astra_pro.launch.xml")),
    )

    dabai = IncludeLaunchDescription(
        AnyLaunchDescriptionSource(os.path.join(astra_launch_dir, "dabai_u3.launch.xml")),
    )

    gemini = IncludeLaunchDescription(
        AnyLaunchDescriptionSource(os.path.join(astra_launch_dir, "gemini.launch.xml")),
    )

    wheeltec_usbcam = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(os.path.join(usb_cam_launch_dir, "demo.launch.py")),
        launch_arguments={"video_device": "/dev/RgbCam"}.items(),
    )

    ld = LaunchDescription()

    # Available options: astra_s, astra_pro, dabai, gemini, wheeltec_usbcam
    ld.add_action(astra_s)
    ld.add_action(video_device_arg)

    return ld
