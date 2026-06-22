#!/usr/bin/env python3
"""
Continuous context estimator for CCA-NMPC dataset collection.

This node replaces the old OZ/NC/HPZ classifier as the primary runtime signal.
It publishes typed CCA-NMPC messages for the controller and legacy JSON mirrors
for web and dataset tooling that still consumes compact dictionaries.
"""

import json
import math
import time
from typing import Optional

import numpy as np

import rclpy
from rclpy.node import Node
from rclpy.qos import HistoryPolicy, QoSProfile, ReliabilityPolicy

from nav_msgs.msg import Odometry
from sensor_msgs.msg import Image, LaserScan
from std_msgs.msg import Float32MultiArray, String
from rai_controller.msg import AdaptiveBounds, Context, HumanState, HumanStates


class ContextMonitor(Node):
    """Estimate phi_h, human state, and adaptive bounds for CCA-NMPC."""

    def __init__(self):
        super().__init__('context_monitor')

        self.declare_parameter('d0', 2.5)
        self.declare_parameter('beta', 3.0)
        self.declare_parameter('context_distance_weight', 6.0)
        self.declare_parameter('context_velocity_weight', 1.0)
        self.declare_parameter('context_direction_weight', 1.0)
        self.declare_parameter('context_confidence_weight', 0.5)
        self.declare_parameter('context_bias', 0.0)
        self.declare_parameter('human_velocity_max', 1.5)
        self.declare_parameter('d_min', 0.5)
        self.declare_parameter('k_d', 0.3)
        self.declare_parameter('vx_max_0', 0.45)
        self.declare_parameter('vy_max_0', 0.35)
        self.declare_parameter('omega_max_0', 1.0)
        self.declare_parameter('k_vx', 0.37)
        self.declare_parameter('k_vy', 0.29)
        self.declare_parameter('k_omega', 0.85)
        self.declare_parameter('vx_min_bound', 0.08)
        self.declare_parameter('vy_min_bound', 0.06)
        self.declare_parameter('omega_min_bound', 0.15)
        self.declare_parameter('publish_legacy_context', True)
        self.declare_parameter('publish_pseudo_humans', False)
        self.declare_parameter('human_timeout_sec', 0.7)

        self.d0 = float(self.get_parameter('d0').value)
        self.beta = float(self.get_parameter('beta').value)
        self.context_distance_weight = float(self.get_parameter('context_distance_weight').value)
        self.context_velocity_weight = float(self.get_parameter('context_velocity_weight').value)
        self.context_direction_weight = float(self.get_parameter('context_direction_weight').value)
        self.context_confidence_weight = float(self.get_parameter('context_confidence_weight').value)
        self.context_bias = float(self.get_parameter('context_bias').value)
        self.human_velocity_max = float(self.get_parameter('human_velocity_max').value)
        self.d_min = float(self.get_parameter('d_min').value)
        self.k_d = float(self.get_parameter('k_d').value)
        self.vx_max_0 = float(self.get_parameter('vx_max_0').value)
        self.vy_max_0 = float(self.get_parameter('vy_max_0').value)
        self.omega_max_0 = float(self.get_parameter('omega_max_0').value)
        self.k_vx = float(self.get_parameter('k_vx').value)
        self.k_vy = float(self.get_parameter('k_vy').value)
        self.k_omega = float(self.get_parameter('k_omega').value)
        self.vx_min_bound = float(self.get_parameter('vx_min_bound').value)
        self.vy_min_bound = float(self.get_parameter('vy_min_bound').value)
        self.omega_min_bound = float(self.get_parameter('omega_min_bound').value)
        self.publish_legacy_context = bool(self.get_parameter('publish_legacy_context').value)
        self.publish_pseudo_humans = bool(self.get_parameter('publish_pseudo_humans').value)
        self.human_timeout_sec = float(self.get_parameter('human_timeout_sec').value)

        self.robot_pose = {'x': 0.0, 'y': 0.0, 'theta': 0.0}
        self.robot_velocity = {'vx': 0.0, 'vy': 0.0}
        self.phi_w = 5.0
        self.nearest_human_dist: Optional[float] = None
        self.last_human_time: Optional[float] = None
        self.last_human_observation: Optional[dict] = None
        self.human_state = {
            'id': 0,
            'x': 0.0,
            'y': 0.0,
            'vx': 0.0,
            'vy': 0.0,
            'confidence': 0.0,
            'age_sec': 0.0,
        }
        self.occlusion_flag = False

        self.context_pub = self.create_publisher(Context, '/canmpc/context', 10)
        self.humans_pub = self.create_publisher(HumanStates, '/canmpc/humans', 10)
        self.context_json_pub = self.create_publisher(String, '/canmpc/context_json', 10)
        self.humans_json_pub = self.create_publisher(String, '/canmpc/humans_json', 10)
        self.bounds_pub = self.create_publisher(Float32MultiArray, '/canmpc/adaptive_bounds_array', 10)
        self.typed_bounds_pub = self.create_publisher(AdaptiveBounds, '/canmpc/adaptive_bounds', 10)
        self.legacy_context_pub = self.create_publisher(String, '/context', 10)

        sensor_qos = QoSProfile(
            reliability=ReliabilityPolicy.BEST_EFFORT,
            history=HistoryPolicy.KEEP_LAST,
            depth=10,
        )
        reliable_qos = QoSProfile(
            reliability=ReliabilityPolicy.RELIABLE,
            history=HistoryPolicy.KEEP_LAST,
            depth=10,
        )

        self.create_subscription(LaserScan, '/scan_filtered', self.scan_callback, sensor_qos)
        self.create_subscription(Image, '/camera/aligned_depth_to_color/image_raw', self.depth_callback, sensor_qos)
        self.create_subscription(Image, '/camera/depth/image_rect_raw', self.depth_callback, sensor_qos)
        self.create_subscription(Odometry, '/odom', self.odom_callback, reliable_qos)

        self.create_timer(0.1, self.publish_context)

        self.get_logger().info('Continuous CCA-NMPC context estimator initialized')
        self.get_logger().info(
            'Publishing typed /canmpc/context and /canmpc/humans; '
            'legacy JSON mirrors on /canmpc/context_json and /canmpc/humans_json')

    def odom_callback(self, msg: Odometry):
        q = msg.pose.pose.orientation
        siny_cosp = 2.0 * (q.w * q.z + q.x * q.y)
        cosy_cosp = 1.0 - 2.0 * (q.y * q.y + q.z * q.z)
        self.robot_pose = {
            'x': msg.pose.pose.position.x,
            'y': msg.pose.pose.position.y,
            'theta': math.atan2(siny_cosp, cosy_cosp),
        }
        theta = self.robot_pose['theta']
        vx_body = msg.twist.twist.linear.x
        vy_body = msg.twist.twist.linear.y
        self.robot_velocity = {
            'vx': vx_body * math.cos(theta) - vy_body * math.sin(theta),
            'vy': vx_body * math.sin(theta) + vy_body * math.cos(theta),
        }

    def scan_callback(self, msg: LaserScan):
        ranges = np.asarray(msg.ranges, dtype=np.float32)
        if ranges.size == 0 or msg.angle_increment == 0.0:
            return

        angles = msg.angle_min + np.arange(ranges.size) * msg.angle_increment
        valid = np.isfinite(ranges) & (ranges > msg.range_min) & (ranges < msg.range_max)

        left = valid & (angles > math.radians(35.0)) & (angles < math.radians(100.0))
        right = valid & (angles < math.radians(-35.0)) & (angles > math.radians(-100.0))

        left_clearance = float(np.nanmedian(ranges[left])) if np.any(left) else 2.5
        right_clearance = float(np.nanmedian(ranges[right])) if np.any(right) else 2.5
        self.phi_w = left_clearance + right_clearance

    def depth_callback(self, msg: Image):
        distance = self._estimate_center_depth(msg)
        if distance is None:
            return

        now = time.time()
        previous = self.last_human_observation
        x_h = self.robot_pose['x'] + distance * math.cos(self.robot_pose['theta'])
        y_h = self.robot_pose['y'] + distance * math.sin(self.robot_pose['theta'])

        vx_h = 0.0
        vy_h = 0.0
        if previous is not None:
            dt = max(1e-3, now - previous['time'])
            vx_h = (x_h - previous['x']) / dt
            vy_h = (y_h - previous['y']) / dt

        # Clamp because this heuristic uses depth-only pseudo detections.
        speed = math.hypot(vx_h, vy_h)
        if speed > 1.8:
            scale = 1.8 / speed
            vx_h *= scale
            vy_h *= scale

        self.nearest_human_dist = distance
        self.last_human_time = now
        self.occlusion_flag = previous is None and distance < self.d0
        self.human_state = {
            'id': 0,
            'x': x_h,
            'y': y_h,
            'vx': vx_h,
            'vy': vy_h,
            'confidence': 0.5,
            'age_sec': 0.0,
        }
        self.last_human_observation = {'time': now, 'x': x_h, 'y': y_h}

    def _estimate_center_depth(self, msg: Image) -> Optional[float]:
        if msg.height == 0 or msg.width == 0 or not msg.data:
            return None

        if msg.encoding in ('16UC1', 'mono16'):
            dtype = np.uint16
            scale = 0.001
        elif msg.encoding in ('32FC1',):
            dtype = np.float32
            scale = 1.0
        else:
            return None

        try:
            image = np.frombuffer(msg.data, dtype=dtype).reshape(msg.height, -1)
        except ValueError:
            return None

        h0 = max(0, int(msg.height * 0.35))
        h1 = min(msg.height, int(msg.height * 0.70))
        w0 = max(0, int(msg.width * 0.35))
        w1 = min(msg.width, int(msg.width * 0.65))
        roi = image[h0:h1, w0:w1].astype(np.float32) * scale
        valid = roi[np.isfinite(roi) & (roi > 0.2) & (roi < 5.0)]
        if valid.size == 0:
            return None

        # Lower percentile approximates nearest person/obstacle in the central ROI.
        return float(np.percentile(valid, 10))

    def _current_human_distance(self) -> float:
        if self.last_human_time is None:
            return 10.0
        if time.time() - self.last_human_time > self.human_timeout_sec:
            self.nearest_human_dist = None
            self.occlusion_flag = False
            self.human_state['confidence'] = 0.0
            self.human_state['age_sec'] = time.time() - self.last_human_time
            return 10.0
        self.human_state['age_sec'] = time.time() - self.last_human_time
        return float(self.nearest_human_dist or 10.0)

    def _phi_h(self, distance: float) -> float:
        confidence = float(self.human_state.get('confidence', 0.0))
        if distance >= 10.0 or confidence <= 0.0:
            return 0.0

        hx = float(self.human_state.get('x', self.robot_pose['x']))
        hy = float(self.human_state.get('y', self.robot_pose['y']))
        hvx = float(self.human_state.get('vx', 0.0))
        hvy = float(self.human_state.get('vy', 0.0))
        speed = math.hypot(hvx, hvy)

        ex = (self.robot_pose['x'] - hx) / max(distance, 1e-6)
        ey = (self.robot_pose['y'] - hy) / max(distance, 1e-6)
        rel_vx = hvx - self.robot_velocity['vx']
        rel_vy = hvy - self.robot_velocity['vy']
        rel_speed = math.hypot(rel_vx, rel_vy)
        direction_cos = 0.0
        if rel_speed > 1e-6:
            direction_cos = max(-1.0, min(1.0, (rel_vx * ex + rel_vy * ey) / rel_speed))

        z = (
            self.context_distance_weight * ((self.d0 - distance) / max(self.d0, 1e-6))
            + self.context_velocity_weight * (speed / max(self.human_velocity_max, 1e-6))
            + self.context_direction_weight * direction_cos
            + self.context_confidence_weight * confidence
            + self.context_bias
        )
        return max(0.0, min(1.0, 1.0 / (1.0 + math.exp(-z))))

    def _adaptive_bounds(self, phi_h: float) -> dict:
        return {
            'd_safe': min(self.d_min + self.k_d * phi_h, self.d_min + self.k_d),
            'vx_max': max(self.vx_min_bound, self.vx_max_0 - self.k_vx * phi_h),
            'vy_max': max(self.vy_min_bound, self.vy_max_0 - self.k_vy * phi_h),
            'omega_max': max(self.omega_min_bound, self.omega_max_0 - self.k_omega * phi_h),
        }

    def _legacy_label(self, phi_h: float) -> str:
        if phi_h > 0.65:
            return 'HPZ'
        if self.phi_w <= 2.0:
            return 'NC'
        return 'OZ'

    def publish_context(self):
        d_h = self._current_human_distance()
        phi_h = self._phi_h(d_h)
        bounds = self._adaptive_bounds(phi_h)
        now_msg = self.get_clock().now().to_msg()

        context_payload = {
            'stamp': {'sec': now_msg.sec, 'nanosec': now_msg.nanosec},
            'phi_h': phi_h,
            'd_h': d_h,
            'phi_w': self.phi_w,
            'd_safe': bounds['d_safe'],
            'vx_max': bounds['vx_max'],
            'vy_max': bounds['vy_max'],
            'omega_max': bounds['omega_max'],
            'occlusion_flag': self.occlusion_flag,
            'legacy_context': self._legacy_label(phi_h),
        }

        context_msg = Context()
        context_msg.header.stamp = now_msg
        context_msg.header.frame_id = 'map'
        context_msg.phi_h = float(phi_h)
        context_msg.nearest_human_dist = float(d_h)
        context_msg.d_safe = float(bounds['d_safe'])
        context_msg.vx_max = float(bounds['vx_max'])
        context_msg.vy_max = float(bounds['vy_max'])
        context_msg.omega_max = float(bounds['omega_max'])
        context_msg.occlusion_flag = bool(self.occlusion_flag)
        self.context_pub.publish(context_msg)

        if self.publish_pseudo_humans:
            humans_msg = HumanStates()
            humans_msg.header.stamp = now_msg
            humans_msg.header.frame_id = 'map'
            if self.human_state.get('confidence', 0.0) > 0.0:
                human_msg = HumanState()
                human_msg.id = int(self.human_state['id'])
                human_msg.x = float(self.human_state['x'])
                human_msg.y = float(self.human_state['y'])
                human_msg.vx = float(self.human_state['vx'])
                human_msg.vy = float(self.human_state['vy'])
                human_msg.confidence = float(self.human_state['confidence'])
                human_msg.age_sec = float(self.human_state.get('age_sec', 0.0))
                humans_msg.humans.append(human_msg)
            self.humans_pub.publish(humans_msg)

        context_json_msg = String()
        context_json_msg.data = json.dumps(context_payload, separators=(',', ':'))
        self.context_json_pub.publish(context_json_msg)

        if self.publish_pseudo_humans:
            humans_json_msg = String()
            humans_json_msg.data = json.dumps({
                'stamp': context_payload['stamp'],
                'humans': [self.human_state] if self.human_state.get('confidence', 0.0) > 0.0 else [],
            }, separators=(',', ':'))
            self.humans_json_pub.publish(humans_json_msg)

        bounds_msg = Float32MultiArray()
        bounds_msg.data = [
            float(phi_h),
            float(bounds['d_safe']),
            float(bounds['vx_max']),
            float(bounds['vy_max']),
            float(bounds['omega_max']),
        ]
        self.bounds_pub.publish(bounds_msg)

        typed_bounds_msg = AdaptiveBounds()
        typed_bounds_msg.header.stamp = now_msg
        typed_bounds_msg.header.frame_id = 'map'
        typed_bounds_msg.vx_max = float(bounds['vx_max'])
        typed_bounds_msg.vy_max = float(bounds['vy_max'])
        typed_bounds_msg.omega_max = float(bounds['omega_max'])
        typed_bounds_msg.d_safe = float(bounds['d_safe'])
        typed_bounds_msg.q_scale = float(1.0 + phi_h)
        self.typed_bounds_pub.publish(typed_bounds_msg)

        if self.publish_legacy_context:
            legacy_msg = String()
            legacy_msg.data = context_payload['legacy_context']
            self.legacy_context_pub.publish(legacy_msg)


def main(args=None):
    rclpy.init(args=args)
    node = ContextMonitor()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info('Keyboard interrupt, shutting down')
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
