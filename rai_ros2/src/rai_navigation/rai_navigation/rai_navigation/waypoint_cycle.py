#!/usr/bin/env python3
# Copyright 2026 RAI Project
#
# Licensed under the Apache License, Version 2.0

"""
RAI Waypoint Cycle — tuần hoàn qua danh sách waypoint bấm trên RViz.

Không phụ thuộc Nav2 Action interface. Sử dụng:
  - /clicked_point  (PointStamped)  — thêm waypoint từ RViz
  - /goal_pose      (PoseStamped)   — gửi goal tới rai_controller_server
  - /rai_navigation/status (String) — nhận trạng thái SUCCEEDED/FAILED/CANCELED
  - /rai_waypoints  (MarkerArray)   — hiển thị danh sách waypoint trong RViz
"""

import rclpy
from rclpy.node import Node

from geometry_msgs.msg import PoseStamped, PointStamped
from std_msgs.msg import String
from visualization_msgs.msg import Marker, MarkerArray


TERMINAL_STATES = {'SUCCEEDED', 'FAILED', 'CANCELED'}


class RaiWaypointCycle(Node):
    """Cycle through click-added waypoints using rai_navigation status topic."""

    def __init__(self):
        super().__init__('rai_waypoint_cycle')

        self.max_retries_: int = (
            self.declare_parameter('max_retries', 1).get_parameter_value().integer_value
        )

        self._waypoints: list[PoseStamped] = []
        self._index: int = 0
        self._active: bool = False
        self._last_state: str = 'IDLE'
        self._retry_count: int = 0

        # Publishers
        self._goal_pub  = self.create_publisher(PoseStamped, '/goal_pose', 10)
        self._marks_pub = self.create_publisher(MarkerArray, '/rai_waypoints', 100)

        # Subscribers
        self.create_subscription(PointStamped, '/clicked_point', self._on_click, 10)
        self.create_subscription(String, '/rai_navigation/status', self._on_status, 10)

        self.get_logger().info('rai_waypoint_cycle ready — click points in RViz to add waypoints.')

    # ------------------------------------------------------------------
    def _on_click(self, msg: PointStamped) -> None:
        pose = PoseStamped()
        pose.header.frame_id = 'map'
        pose.header.stamp = self.get_clock().now().to_msg()
        pose.pose.position.x = msg.point.x
        pose.pose.position.y = msg.point.y
        pose.pose.position.z = 0.0
        pose.pose.orientation.w = 1.0

        self._waypoints.append(pose)
        self._publish_markers()

        idx = len(self._waypoints) - 1
        self.get_logger().info(
            f'Waypoint {idx} added: ({msg.point.x:.2f}, {msg.point.y:.2f})'
        )

        if not self._active:
            self._index = 0
            self._active = True
            self._retry_count = 0
            self._send_goal(self._index)

    def _on_status(self, msg: String) -> None:
        # Status string may contain detail after colon, e.g. "SUCCEEDED: goal reached"
        state = msg.data.split(':')[0].strip()

        if state == self._last_state:
            return
        self._last_state = state

        if not self._active or state not in TERMINAL_STATES:
            return

        if state == 'SUCCEEDED':
            self.get_logger().info(
                f'Waypoint {self._index} reached. '
                f'Moving to {(self._index + 1) % max(len(self._waypoints), 1)}.'
            )
            self._retry_count = 0
            self._index = (self._index + 1) % len(self._waypoints)
            self._send_goal(self._index)

        else:  # FAILED or CANCELED
            if self._retry_count < self.max_retries_:
                self._retry_count += 1
                self.get_logger().warn(
                    f'Goal {state} at waypoint {self._index}. '
                    f'Retrying ({self._retry_count}/{self.max_retries_})...'
                )
                self._send_goal(self._index)
            else:
                self._retry_count = 0
                next_idx = (self._index + 1) % len(self._waypoints)
                self.get_logger().warn(
                    f'Max retries reached. Skipping waypoint {self._index} → {next_idx}.'
                )
                self._index = next_idx
                self._send_goal(self._index)

    def _send_goal(self, idx: int) -> None:
        if not self._waypoints:
            return
        pose = PoseStamped()
        pose.header.frame_id = self._waypoints[idx].header.frame_id
        pose.header.stamp = self.get_clock().now().to_msg()
        pose.pose = self._waypoints[idx].pose
        self._goal_pub.publish(pose)
        self.get_logger().info(
            f'→ Goal {idx}: '
            f'({pose.pose.position.x:.2f}, {pose.pose.position.y:.2f})'
        )

    def _publish_markers(self) -> None:
        arr = MarkerArray()
        for i, wp in enumerate(self._waypoints):
            # Sphere marker at waypoint position
            sphere = Marker()
            sphere.header.frame_id = 'map'
            sphere.header.stamp = self.get_clock().now().to_msg()
            sphere.ns = 'rai_waypoints_sphere'
            sphere.id = i * 2
            sphere.type = Marker.SPHERE
            sphere.action = Marker.ADD
            sphere.scale.x = sphere.scale.y = sphere.scale.z = 0.2
            sphere.color.a = 1.0
            sphere.color.r = 0.0
            sphere.color.g = 0.8
            sphere.color.b = 1.0
            sphere.pose = wp.pose
            arr.markers.append(sphere)

            # Text label with waypoint index
            label = Marker()
            label.header = sphere.header
            label.ns = 'rai_waypoints_label'
            label.id = i * 2 + 1
            label.type = Marker.TEXT_VIEW_FACING
            label.action = Marker.ADD
            label.scale.z = 0.3
            label.color.a = 1.0
            label.color.r = 1.0
            label.color.g = 1.0
            label.color.b = 1.0
            label.pose.position.x = wp.pose.position.x
            label.pose.position.y = wp.pose.position.y
            label.pose.position.z = 0.3           # float above the sphere
            label.pose.orientation = wp.pose.orientation
            label.text = str(i)
            arr.markers.append(label)

        self._marks_pub.publish(arr)


def main(args=None):
    rclpy.init(args=args)
    node = RaiWaypointCycle()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
