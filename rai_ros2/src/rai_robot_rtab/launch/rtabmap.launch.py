
























from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, SetEnvironmentVariable
from launch.substitutions import LaunchConfiguration
from launch.conditions import IfCondition, UnlessCondition
from launch_ros.actions import Node


def generate_launch_description():

    use_sim_time = LaunchConfiguration('use_sim_time')
    qos = LaunchConfiguration('qos')
    localization = LaunchConfiguration('localization')

    icp_parameters={
          'odom_frame_id':'odom_combined',
          'qos':qos
    }

    rtabmap_parameters={
          'subscribe_rgbd':True,
          'subscribe_scan':True,
          'use_action_for_goal':True,
          'odom_sensor_sync': True,
          'qos_scan':qos,
          'qos_image':qos,
          'qos_imu':qos,
          
          'Mem/NotLinkedNodesKept':'false'
    }

    
    shared_parameters={
          'frame_id':'camera_link',
          'use_sim_time':use_sim_time,
          
          'Reg/Strategy':'1',
          'Reg/Force3DoF':'true',
          'Mem/NotLinkedNodesKept':'false',
          'Icp/PointToPlaneMinComplexity':'0.04' 
    }

    remappings=[
          ('odom', '/odom_combined'),
          ('scan', '/scan'),
          ('rgb/image', '/camera/color/image_raw'), 
          ('rgb/camera_info', '/camera/color/camera_info'),
          ('depth/image', '/camera/depth/image_raw')]

    return LaunchDescription([

        
        DeclareLaunchArgument(
            'use_sim_time', default_value='false', choices=['true', 'false'],
            description='Use simulation (Gazebo) clock if true'),
        
        DeclareLaunchArgument(
            'qos', default_value='0',
            description='QoS used for input sensor topics'),
            
        DeclareLaunchArgument(
            'localization', default_value='false', choices=['true', 'false'],
            description='Launch rtabmap in localization mode (a map should have been already created).'),

        
        Node(
            package='rtabmap_sync', executable='rgbd_sync', output='screen',
            parameters=[{'approx_sync':False, 'use_sim_time':use_sim_time, 'qos':qos}],
            remappings=remappings),

        Node(
            package='rtabmap_odom', executable='icp_odometry', output='screen',
            parameters=[icp_parameters, shared_parameters],
            remappings=remappings,
            arguments=["--ros-args", "--log-level", 'icp_odometry:=warn']),

        
        Node(
            condition=UnlessCondition(localization),
            package='rtabmap_slam', executable='rtabmap', output='screen',
            parameters=[rtabmap_parameters, shared_parameters],
            remappings=remappings,
            arguments=['-d']),
            
        
        Node(
            condition=IfCondition(localization),
            package='rtabmap_slam', executable='rtabmap', output='screen',
            parameters=[rtabmap_parameters, shared_parameters,
              {'Mem/IncrementalMemory':'False',
               'Mem/InitWMWithAllNodes':'True'}],
            remappings=remappings),

    ])
