from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    host_arg = DeclareLaunchArgument(
        'host',
        default_value='0.0.0.0',
        description='Host address for Rai FastAPI server'
    )
    port_arg = DeclareLaunchArgument(
        'port',
        default_value='8080',
        description='Port for Rai FastAPI server'
    )

    web_api_node = Node(
        package='rai_web_api',
        executable='web_api',
        name='rai_web_api',
        output='screen',
        additional_env={
            'RAI_API_HOST': LaunchConfiguration('host'),
            'RAI_API_PORT': LaunchConfiguration('port'),
        },
    )

    return LaunchDescription([
        host_arg,
        port_arg,
        web_api_node,
    ])
