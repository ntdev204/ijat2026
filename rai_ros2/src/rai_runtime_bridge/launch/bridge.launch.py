import os
from pathlib import Path

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    workspace_root = Path(os.getenv("RAI_WORKSPACE_ROOT", str(Path.home() / "ijat2026")))
    dataset_dir = Path(os.getenv("RAI_DATASET_PATH", str(workspace_root / "dataset")))

    role_arg = DeclareLaunchArgument(
        "role",
        default_value=os.getenv("RAI_DEVICE_ROLE", "pi"),
        description="Runtime bridge device role: pi, jetson, laptop, or sim",
    )
    host_arg = DeclareLaunchArgument(
        "host",
        default_value=os.getenv("RAI_BRIDGE_HOST", "0.0.0.0"),
        description="Host address for the runtime bridge server",
    )
    port_arg = DeclareLaunchArgument(
        "port",
        default_value=os.getenv("RAI_BRIDGE_PORT", "8090"),
        description="Port for the runtime bridge server",
    )

    bridge_node = Node(
        package="rai_runtime_bridge",
        executable="runtime_bridge",
        name="rai_runtime_bridge",
        output="screen",
        additional_env={
            "RAI_DEVICE_ROLE": LaunchConfiguration("role"),
            "RAI_BRIDGE_HOST": LaunchConfiguration("host"),
            "RAI_BRIDGE_PORT": LaunchConfiguration("port"),
            "RAI_WORKSPACE_ROOT": str(workspace_root),
            "RAI_DATASET_PATH": str(dataset_dir),
        },
    )

    return LaunchDescription([role_arg, host_arg, port_arg, bridge_node])
