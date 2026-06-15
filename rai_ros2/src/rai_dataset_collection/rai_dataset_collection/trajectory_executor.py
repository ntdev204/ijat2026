#!/usr/bin/env python3
"""
Manual Mecanum command helper for dataset collection.

Map-based scenario execution is intentionally disabled until a real map and
reference trajectory generator are available. This node only publishes a safe
zero command and status, so dataset collection remains manual/Nav2-driven.
"""

import rclpy
from rclpy.node import Node

from geometry_msgs.msg import Twist
from std_msgs.msg import String


class TrajectoryExecutor(Node):
    """Placeholder executor: no map-dependent waypoints."""

    def __init__(self):
        super().__init__('trajectory_executor')
        self.declare_parameter('enabled', False)

        self.enabled = bool(self.get_parameter('enabled').value)
        self.cmd_vel_pub = self.create_publisher(Twist, '/cmd_vel', 10)
        self.status_pub = self.create_publisher(String, '/trajectory_status', 10)

        if self.enabled:
            self.get_logger().warn(
                'trajectory_executor is disabled until map/reference scenarios are integrated. '
                'Publishing stop command only.'
            )
        else:
            self.get_logger().info('trajectory_executor idle: dataset collection is manual/Nav2-driven.')

        self.create_timer(1.0, self.publish_idle_status)

    def publish_idle_status(self):
        msg = String()
        msg.data = 'NO_MAP_SCENARIOS_DISABLED'
        self.status_pub.publish(msg)
        if self.enabled:
            self.cmd_vel_pub.publish(Twist())


def main(args=None):
    rclpy.init(args=args)
    node = TrajectoryExecutor()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info('Keyboard interrupt, stopping')
    finally:
        node.cmd_vel_pub.publish(Twist())
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
