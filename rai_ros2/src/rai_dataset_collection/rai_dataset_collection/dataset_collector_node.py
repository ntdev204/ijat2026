#!/usr/bin/env python3
"""
Dataset collector for Continuous Context-Adaptive CA-NMPC.

The collector stores raw ROS 2 bags plus run metadata for the new CCA-NMPC
flow: Mecanum commands, continuous human-proximity context, adaptive bounds,
human state estimates, local reference, predicted trajectory, and solver stats.
"""

import json
import os
from datetime import datetime
from pathlib import Path
from typing import Any

import rclpy
from rclpy.node import Node
from rclpy.qos import HistoryPolicy, QoSProfile, ReliabilityPolicy
from rclpy.serialization import serialize_message

from geometry_msgs.msg import PolygonStamped, Twist
from nav_msgs.msg import OccupancyGrid, Odometry, Path as NavPath
from sensor_msgs.msg import CameraInfo, Image, JointState, LaserScan, Imu
from std_msgs.msg import Float32, Float32MultiArray, String
from tf2_msgs.msg import TFMessage
from ccanmpc_msgs.msg import Context, HumanStates

import rosbag2_py
from rosbag2_py import ConverterOptions, StorageOptions, TopicMetadata


class DatasetCollectorNode(Node):
    """Collect CCA-NMPC raw bags and per-run metadata."""

    def __init__(self):
        super().__init__('dataset_collector_node')

        self.declare_parameter('base_path', os.path.expanduser('~/rai_datasets/canmpc'))
        self.declare_parameter('environment', 'sim')
        self.declare_parameter('scenario_id', 'S1_open_zone')
        self.declare_parameter('scenario_name', '')
        self.declare_parameter('controller_id', 'CCA_NMPC')
        self.declare_parameter('run_id', '')
        self.declare_parameter('split', 'unsplit')
        self.declare_parameter('auto_start', False)

        self.base_path = Path(self.get_parameter('base_path').value).expanduser()
        self.environment = self.get_parameter('environment').value
        self.scenario_id = self.get_parameter('scenario_id').value
        legacy_scenario_name = self.get_parameter('scenario_name').value
        if legacy_scenario_name and self.scenario_id == 'S1_open_zone':
            self.scenario_id = legacy_scenario_name
        self.controller_id = self.get_parameter('controller_id').value
        self.run_id = self.get_parameter('run_id').value
        self.split = self.get_parameter('split').value
        self.auto_start = self.get_parameter('auto_start').value

        self.is_recording = False
        self.bag_writer = None
        self.current_session_path: Path | None = None
        self.start_time = None

        self.current_voltage = 0.0
        self.current_battery_pct = 0.0
        self.last_battery_warn_time = None
        self.battery_warn_interval = 30.0
        self.voltage_curve = [
            (25.2, 100.0), (24.6, 90.0), (24.0, 80.0), (23.6, 70.0),
            (23.2, 60.0), (22.8, 50.0), (22.4, 40.0), (22.0, 30.0),
            (21.6, 20.0), (21.0, 10.0), (20.0, 0.0),
        ]

        self.metadata: dict[str, Any] = self._new_metadata()

        sensor_qos = QoSProfile(
            reliability=ReliabilityPolicy.BEST_EFFORT,
            history=HistoryPolicy.KEEP_LAST,
            depth=10,
        )
        reliable_qos = QoSProfile(
            reliability=ReliabilityPolicy.RELIABLE,
            history=HistoryPolicy.KEEP_LAST,
            depth=20,
        )

        self.topic_specs = [
            ('/scan', LaserScan, 'sensor_msgs/msg/LaserScan', sensor_qos),
            ('/scan_filtered', LaserScan, 'sensor_msgs/msg/LaserScan', sensor_qos),
            ('/camera/color/image_raw', Image, 'sensor_msgs/msg/Image', sensor_qos),
            ('/camera/depth/image_rect_raw', Image, 'sensor_msgs/msg/Image', sensor_qos),
            ('/camera/aligned_depth_to_color/image_raw', Image, 'sensor_msgs/msg/Image', sensor_qos),
            ('/camera/camera_info', CameraInfo, 'sensor_msgs/msg/CameraInfo', reliable_qos),
            ('/camera/color/camera_info', CameraInfo, 'sensor_msgs/msg/CameraInfo', reliable_qos),
            ('/tf', TFMessage, 'tf2_msgs/msg/TFMessage', reliable_qos),
            ('/tf_static', TFMessage, 'tf2_msgs/msg/TFMessage', reliable_qos),
            ('/odom', Odometry, 'nav_msgs/msg/Odometry', reliable_qos),
            ('/imu/data_raw', Imu, 'sensor_msgs/msg/Imu', reliable_qos),
            ('/cmd_vel', Twist, 'geometry_msgs/msg/Twist', reliable_qos),
            ('/joint_states', JointState, 'sensor_msgs/msg/JointState', reliable_qos),
            ('/wheel_encoders', JointState, 'sensor_msgs/msg/JointState', reliable_qos),
            ('/voltage', Float32, 'std_msgs/msg/Float32', reliable_qos),
            ('/context', String, 'std_msgs/msg/String', reliable_qos),
            ('/canmpc/context', Context, 'ccanmpc_msgs/msg/Context', reliable_qos),
            ('/canmpc/humans', HumanStates, 'ccanmpc_msgs/msg/HumanStates', reliable_qos),
            ('/canmpc/context_json', String, 'std_msgs/msg/String', reliable_qos),
            ('/canmpc/humans_json', String, 'std_msgs/msg/String', reliable_qos),
            ('/canmpc/adaptive_bounds', Float32MultiArray, 'std_msgs/msg/Float32MultiArray', reliable_qos),
            ('/canmpc/local_reference_path', NavPath, 'nav_msgs/msg/Path', reliable_qos),
            ('/canmpc/predicted_trajectory', NavPath, 'nav_msgs/msg/Path', reliable_qos),
            ('/canmpc/solver_stats', String, 'std_msgs/msg/String', reliable_qos),
            ('/local_costmap/costmap', OccupancyGrid, 'nav_msgs/msg/OccupancyGrid', reliable_qos),
            ('/local_costmap/published_footprint', PolygonStamped, 'geometry_msgs/msg/PolygonStamped', reliable_qos),
        ]

        for topic_name, msg_type, _, qos in self.topic_specs:
            self.create_subscription(
                msg_type,
                topic_name,
                lambda msg, name=topic_name: self._topic_callback(name, msg),
                qos,
            )

        self.create_timer(0.1, self.timer_callback)

        self.get_logger().info(f'CCA-NMPC dataset collector initialized: {self.base_path}')
        self.get_logger().info(
            f'environment={self.environment}, scenario={self.scenario_id}, '
            f'controller={self.controller_id}, split={self.split}'
        )

        if self.auto_start:
            self.start_recording()

    def _new_metadata(self) -> dict[str, Any]:
        return {
            'schema_version': '2.0.0',
            'flow': 'continuous_context_adaptive_ca_nmpc',
            'environment': self.environment,
            'scenario_id': self.scenario_id,
            'controller_id': self.controller_id,
            'run_id': self.run_id,
            'split': self.split,
            'samples': 0,
            'samples_by_topic': {},
            'duration': 0.0,
            'continuous_context': {
                'phi_h_samples': [],
                'd_h_samples': [],
                'd_safe_samples': [],
                'vx_max_samples': [],
                'vy_max_samples': [],
                'omega_max_samples': [],
                'occlusion_events': 0,
            },
            'human_state': {
                'samples': 0,
                'last': None,
            },
            'velocity_profile': [],
            'cmd_profile': [],
            'solver': {
                'samples': 0,
                'timeouts': 0,
                'solve_time_ms_samples': [],
                'last': None,
            },
            'metrics': {
                'tracking': {
                    'rmse_xy': None,
                    'rmse_theta': None,
                    'max_lateral_error': None,
                },
                'safety': {
                    'd_min': None,
                    'd_avg': None,
                    'd_5percentile': None,
                    'violation_count': 0,
                    'violation_duration': 0.0,
                    'collision_count': 0,
                },
                'smoothness': {
                    'jerk_mean': None,
                    'jerk_max': None,
                    'mean_abs_delta_u': None,
                    'max_abs_delta_u': None,
                },
                'control': {
                    'control_effort': None,
                    'mean_abs_vx': None,
                    'mean_abs_vy': None,
                    'mean_abs_omega': None,
                },
                'realtime': {
                    'solve_time_mean_ms': None,
                    'solve_time_median_ms': None,
                    'solve_time_p95_ms': None,
                    'solve_time_max_ms': None,
                    'timeout_rate': None,
                },
            },
            'battery': {
                'voltage_samples': [],
                'percentage_samples': [],
                'start_voltage': None,
                'end_voltage': None,
                'min_voltage': None,
                'min_percentage': None,
            },
        }

    def start_recording(self):
        """Start recording one raw bag and metadata file."""
        if self.is_recording:
            self.get_logger().warn('Already recording')
            return

        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        if not self.run_id:
            self.run_id = f'run_{timestamp}'

        self.current_session_path = (
            self.base_path / 'raw' / self.environment /
            self.scenario_id / self.controller_id / self.run_id
        )
        self.current_session_path.mkdir(parents=True, exist_ok=True)

        bag_path = str(self.current_session_path / 'rosbag2')
        storage_options = StorageOptions(uri=bag_path, storage_id='mcap')
        converter_options = ConverterOptions(
            input_serialization_format='cdr',
            output_serialization_format='cdr',
        )

        self.bag_writer = rosbag2_py.SequentialWriter()
        self.bag_writer.open(storage_options, converter_options)
        self._create_bag_topics()

        self.metadata = self._new_metadata()
        self.metadata['run_id'] = self.run_id
        self.metadata['start_time'] = timestamp
        self.metadata['bag_path'] = bag_path
        self.metadata['battery']['start_voltage'] = self.current_voltage or None

        self.is_recording = True
        self.start_time = self.get_clock().now()
        self._write_metadata()

        self.get_logger().info(f'Started recording: {bag_path}')

    def stop_recording(self):
        """Stop recording and finalize metadata."""
        if not self.is_recording:
            self.get_logger().warn('Not recording')
            return

        self.is_recording = False

        if self.bag_writer:
            del self.bag_writer
            self.bag_writer = None

        end_time = self.get_clock().now()
        duration = (end_time - self.start_time).nanoseconds / 1e9 if self.start_time else 0.0
        self.metadata['duration'] = duration
        self.metadata['end_time'] = datetime.now().strftime('%Y%m%d_%H%M%S')
        self._compute_run_metrics()
        self._finalize_battery_stats()
        self._trim_metadata_samples()
        self._write_metadata()

        self.get_logger().info(f'Stopped recording. Duration: {duration:.2f}s')
        self.get_logger().info(f'Metadata saved: {self.current_session_path / "metadata.json"}')

    def _create_bag_topics(self):
        for topic_name, _, topic_type, _ in self.topic_specs:
            topic_metadata = TopicMetadata(
                name=topic_name,
                type=topic_type,
                serialization_format='cdr',
            )
            self.bag_writer.create_topic(topic_metadata)

    def _topic_callback(self, topic_name: str, msg):
        self._write_message(topic_name, msg)
        if not self.is_recording:
            if topic_name == '/voltage':
                self._handle_voltage(msg)
            return

        self.metadata['samples'] += 1
        samples_by_topic = self.metadata['samples_by_topic']
        samples_by_topic[topic_name] = samples_by_topic.get(topic_name, 0) + 1

        if topic_name == '/odom':
            self._handle_odom(msg)
        elif topic_name == '/cmd_vel':
            self._handle_cmd_vel(msg)
        elif topic_name == '/voltage':
            self._handle_voltage(msg)
        elif topic_name == '/canmpc/context':
            self._handle_context_msg(msg)
        elif topic_name == '/canmpc/humans':
            self._handle_humans_msg(msg)
        elif topic_name == '/canmpc/context_json':
            self._handle_context_json(msg.data)
        elif topic_name == '/canmpc/humans_json':
            self._handle_humans_json(msg.data)
        elif topic_name == '/canmpc/adaptive_bounds':
            self._handle_adaptive_bounds(msg)
        elif topic_name == '/canmpc/solver_stats':
            self._handle_solver_stats(msg.data)

    def _write_message(self, topic_name: str, msg):
        if self.is_recording and self.bag_writer:
            self.bag_writer.write(topic_name, serialize_message(msg), self.get_clock().now().nanoseconds)

    def _elapsed(self) -> float:
        if not self.start_time:
            return 0.0
        return (self.get_clock().now() - self.start_time).nanoseconds / 1e9

    def _handle_odom(self, msg: Odometry):
        self.metadata['velocity_profile'].append({
            't': self._elapsed(),
            'vx': msg.twist.twist.linear.x,
            'vy': msg.twist.twist.linear.y,
            'omega': msg.twist.twist.angular.z,
        })

    def _handle_cmd_vel(self, msg: Twist):
        sample = {
            't': self._elapsed(),
            'vx': msg.linear.x,
            'vy': msg.linear.y,
            'omega': msg.angular.z,
        }
        self.metadata['last_cmd_vel'] = sample
        self.metadata['cmd_profile'].append(sample)

    def _handle_context_json(self, data: str):
        payload = self._loads_json(data)
        if not payload:
            return
        context = self.metadata['continuous_context']
        self._append_if_number(context['phi_h_samples'], payload.get('phi_h'))
        self._append_if_number(context['d_h_samples'], payload.get('d_h'))
        self._append_if_number(context['d_safe_samples'], payload.get('d_safe'))
        self._append_if_number(context['vx_max_samples'], payload.get('vx_max'))
        self._append_if_number(context['vy_max_samples'], payload.get('vy_max'))
        self._append_if_number(context['omega_max_samples'], payload.get('omega_max'))
        if payload.get('occlusion_flag'):
            context['occlusion_events'] += 1
        self.metadata['last_context'] = payload

    def _handle_context_msg(self, msg: Context):
        payload = {
            'stamp': {'sec': msg.header.stamp.sec, 'nanosec': msg.header.stamp.nanosec},
            'phi_h': float(msg.phi_h),
            'd_h': float(msg.nearest_human_dist),
            'd_safe': float(msg.d_safe),
            'vx_max': float(msg.vx_max),
            'vy_max': float(msg.vy_max),
            'omega_max': float(msg.omega_max),
            'occlusion_flag': bool(msg.occlusion_flag),
        }
        context = self.metadata['continuous_context']
        self._append_if_number(context['phi_h_samples'], payload['phi_h'])
        self._append_if_number(context['d_h_samples'], payload['d_h'])
        self._append_if_number(context['d_safe_samples'], payload['d_safe'])
        self._append_if_number(context['vx_max_samples'], payload['vx_max'])
        self._append_if_number(context['vy_max_samples'], payload['vy_max'])
        self._append_if_number(context['omega_max_samples'], payload['omega_max'])
        if payload['occlusion_flag']:
            context['occlusion_events'] += 1
        self.metadata['last_context'] = payload

    def _handle_humans_json(self, data: str):
        payload = self._loads_json(data)
        if not payload:
            return
        self.metadata['human_state']['samples'] += 1
        self.metadata['human_state']['last'] = payload

    def _handle_humans_msg(self, msg: HumanStates):
        humans = []
        for human in msg.humans:
            humans.append({
                'id': int(human.id),
                'x': float(human.pose.position.x),
                'y': float(human.pose.position.y),
                'vx': float(human.velocity.linear.x),
                'vy': float(human.velocity.linear.y),
                'confidence': float(human.confidence),
            })
        self.metadata['human_state']['samples'] += 1
        self.metadata['human_state']['last'] = {
            'stamp': {'sec': msg.header.stamp.sec, 'nanosec': msg.header.stamp.nanosec},
            'humans': humans,
        }

    def _handle_adaptive_bounds(self, msg: Float32MultiArray):
        data = list(msg.data)
        if len(data) >= 5:
            context = self.metadata['continuous_context']
            self._append_if_number(context['d_safe_samples'], data[1])
            self._append_if_number(context['vx_max_samples'], data[2])
            self._append_if_number(context['vy_max_samples'], data[3])
            self._append_if_number(context['omega_max_samples'], data[4])

    def _handle_solver_stats(self, data: str):
        payload = self._loads_json(data)
        solver = self.metadata['solver']
        solver['samples'] += 1
        solver['last'] = payload if payload else {'raw': data}
        if payload and payload.get('timeout'):
            solver['timeouts'] += 1
        solve_time = self._extract_solve_time_ms(payload)
        if solve_time is not None:
            solver['solve_time_ms_samples'].append(solve_time)

    @staticmethod
    def _extract_solve_time_ms(payload: dict[str, Any] | None) -> float | None:
        if not payload:
            return None
        for key in ('solve_time_ms', 'solver_time_ms', 'elapsed_ms', 'duration_ms'):
            value = payload.get(key)
            if isinstance(value, (int, float)):
                return float(value)
        for key in ('solve_time', 'solver_time', 'elapsed', 'duration'):
            value = payload.get(key)
            if isinstance(value, (int, float)):
                return float(value) * 1000.0
        return None

    def _compute_run_metrics(self):
        metrics = self.metadata['metrics']
        context = self.metadata['continuous_context']
        d_h = [float(v) for v in context.get('d_h_samples', []) if isinstance(v, (int, float))]
        d_safe = [float(v) for v in context.get('d_safe_samples', []) if isinstance(v, (int, float))]

        if d_h:
            sorted_dist = sorted(d_h)
            p5_idx = max(0, min(len(sorted_dist) - 1, int(0.05 * (len(sorted_dist) - 1))))
            metrics['safety']['d_min'] = min(d_h)
            metrics['safety']['d_avg'] = sum(d_h) / len(d_h)
            metrics['safety']['d_5percentile'] = sorted_dist[p5_idx]

        if d_h and d_safe:
            pair_count = min(len(d_h), len(d_safe))
            violations = sum(1 for i in range(pair_count) if d_h[i] < d_safe[i])
            metrics['safety']['violation_count'] = violations
            metrics['safety']['violation_duration'] = violations * 0.1

        cmd_profile = self.metadata.get('cmd_profile', [])
        if cmd_profile:
            vx = [abs(float(s['vx'])) for s in cmd_profile]
            vy = [abs(float(s['vy'])) for s in cmd_profile]
            omega = [abs(float(s['omega'])) for s in cmd_profile]
            metrics['control']['mean_abs_vx'] = sum(vx) / len(vx)
            metrics['control']['mean_abs_vy'] = sum(vy) / len(vy)
            metrics['control']['mean_abs_omega'] = sum(omega) / len(omega)

            effort = 0.0
            deltas = []
            jerks = []
            for i, sample in enumerate(cmd_profile):
                dt = 0.1
                if i > 0:
                    dt = max(1e-3, float(sample['t']) - float(cmd_profile[i - 1]['t']))
                effort += (
                    float(sample['vx']) ** 2 +
                    float(sample['vy']) ** 2 +
                    float(sample['omega']) ** 2
                ) * dt
                if i > 0:
                    prev = cmd_profile[i - 1]
                    du = (
                        (float(sample['vx']) - float(prev['vx'])) ** 2 +
                        (float(sample['vy']) - float(prev['vy'])) ** 2 +
                        (float(sample['omega']) - float(prev['omega'])) ** 2
                    ) ** 0.5
                    deltas.append(du)
                    jerks.append(du / dt)
            metrics['control']['control_effort'] = effort
            if deltas:
                metrics['smoothness']['mean_abs_delta_u'] = sum(deltas) / len(deltas)
                metrics['smoothness']['max_abs_delta_u'] = max(deltas)
            if jerks:
                metrics['smoothness']['jerk_mean'] = sum(jerks) / len(jerks)
                metrics['smoothness']['jerk_max'] = max(jerks)

        solver = self.metadata['solver']
        solve_times = [
            float(v) for v in solver.get('solve_time_ms_samples', [])
            if isinstance(v, (int, float))
        ]
        if solve_times:
            sorted_times = sorted(solve_times)
            mid = len(sorted_times) // 2
            p95_idx = max(0, min(len(sorted_times) - 1, int(0.95 * (len(sorted_times) - 1))))
            metrics['realtime']['solve_time_mean_ms'] = sum(solve_times) / len(solve_times)
            metrics['realtime']['solve_time_median_ms'] = (
                sorted_times[mid] if len(sorted_times) % 2 == 1
                else 0.5 * (sorted_times[mid - 1] + sorted_times[mid])
            )
            metrics['realtime']['solve_time_p95_ms'] = sorted_times[p95_idx]
            metrics['realtime']['solve_time_max_ms'] = max(solve_times)
        if solver.get('samples', 0):
            metrics['realtime']['timeout_rate'] = solver.get('timeouts', 0) / solver['samples']

    def _handle_voltage(self, msg: Float32):
        self.current_voltage = msg.data
        self.current_battery_pct = self._calculate_battery_percentage(self.current_voltage)

        if self.is_recording:
            self.metadata['battery']['voltage_samples'].append(self.current_voltage)
            self.metadata['battery']['percentage_samples'].append(self.current_battery_pct)

        self._check_battery_warning()

    @staticmethod
    def _loads_json(data: str) -> dict[str, Any] | None:
        try:
            payload = json.loads(data)
            return payload if isinstance(payload, dict) else None
        except Exception:
            return None

    @staticmethod
    def _append_if_number(values: list[float], value: Any):
        if isinstance(value, (int, float)):
            values.append(float(value))

    def _finalize_battery_stats(self):
        battery = self.metadata['battery']
        battery['end_voltage'] = self.current_voltage or None
        voltage_samples = battery['voltage_samples']
        pct_samples = battery['percentage_samples']
        if voltage_samples:
            battery['min_voltage'] = min(voltage_samples)
            battery['avg_voltage'] = sum(voltage_samples) / len(voltage_samples)
        if pct_samples:
            battery['min_percentage'] = min(pct_samples)
            battery['avg_percentage'] = sum(pct_samples) / len(pct_samples)

    def _trim_metadata_samples(self):
        self.metadata['velocity_profile'] = self.metadata['velocity_profile'][-500:]
        self.metadata['cmd_profile'] = self.metadata['cmd_profile'][-500:]
        for values in self.metadata['continuous_context'].values():
            if isinstance(values, list):
                del values[:-500]
        self.metadata['solver']['solve_time_ms_samples'] = self.metadata['solver']['solve_time_ms_samples'][-500:]
        self.metadata['battery']['voltage_samples'] = self.metadata['battery']['voltage_samples'][-100:]
        self.metadata['battery']['percentage_samples'] = self.metadata['battery']['percentage_samples'][-100:]

    def _write_metadata(self):
        if not self.current_session_path:
            return
        metadata_path = self.current_session_path / 'metadata.json'
        with open(metadata_path, 'w', encoding='utf-8') as f:
            json.dump(self.metadata, f, indent=2)

    def _calculate_battery_percentage(self, voltage: float) -> float:
        if voltage <= 20.0:
            return 0.0
        if voltage >= 25.2:
            return 100.0

        for i in range(len(self.voltage_curve) - 1):
            v_high, p_high = self.voltage_curve[i]
            v_low, p_low = self.voltage_curve[i + 1]
            if v_low <= voltage <= v_high:
                ratio = (voltage - v_low) / (v_high - v_low)
                return round(p_low + ratio * (p_high - p_low), 1)

        return round(max(0.0, min(100.0, (voltage - 20.0) / 5.2 * 100.0)), 1)

    def _check_battery_warning(self):
        now = self.get_clock().now()
        if self.last_battery_warn_time is not None:
            elapsed = (now - self.last_battery_warn_time).nanoseconds / 1e9
            if elapsed < self.battery_warn_interval:
                return

        if self.current_battery_pct <= 10.0:
            self.get_logger().error(
                f'Critical battery: {self.current_voltage:.2f}V ({self.current_battery_pct:.1f}%)'
            )
            self.last_battery_warn_time = now
        elif self.current_battery_pct <= 20.0:
            self.get_logger().warn(
                f'Low battery: {self.current_voltage:.2f}V ({self.current_battery_pct:.1f}%)'
            )
            self.last_battery_warn_time = now

    def timer_callback(self):
        if self.is_recording and self.start_time:
            duration = self._elapsed()
            if int(duration) % 10 == 0 and duration > 0:
                self._write_metadata()
                self.get_logger().info(
                    f'Recording {self.scenario_id}/{self.controller_id}/{self.run_id}: '
                    f'{duration:.1f}s, samples={self.metadata["samples"]}'
                )


def main(args=None):
    rclpy.init(args=args)
    node = DatasetCollectorNode()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info('Keyboard interrupt, stopping recording')
        node.stop_recording()
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
