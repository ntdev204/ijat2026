#!/usr/bin/env python3
"""
Dataset collector for Continuous Context-Aware NMPC.

The collector stores raw ROS 2 bags plus run metadata for the new CCA-NMPC
flow: Mecanum commands, continuous human-proximity context, adaptive bounds,
human state estimates, local reference, predicted trajectory, and solver stats.
"""

import csv
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
from rai_controller.msg import AdaptiveBounds, Context, HumanStates, SolverStats
from rai_human_perception.msg import ContextInput, HumanArray
from rai_dataset_collection.research_dataset_spec import scenario_by_id

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
        self.csv_writers: dict[str, tuple[Any, csv.DictWriter]] = {}
        self.latest_odom_sample: dict[str, Any] | None = None
        self.latest_cmd_sample: dict[str, Any] | None = None
        self.latest_context_row: dict[str, Any] | None = None
        self.latest_context_input_row: dict[str, Any] | None = None
        self.latest_humans_row: list[dict[str, Any]] = []
        self.latest_solver_row: dict[str, Any] | None = None
        self.last_odom_stamp_key: tuple[int, int] | None = None
        self.last_context_input_stamp_key: tuple[int, int] | None = None

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
            ('/odom_combined', Odometry, 'nav_msgs/msg/Odometry', reliable_qos),
            ('/imu/data_raw', Imu, 'sensor_msgs/msg/Imu', reliable_qos),
            ('/cmd_vel', Twist, 'geometry_msgs/msg/Twist', reliable_qos),
            ('/joint_states', JointState, 'sensor_msgs/msg/JointState', reliable_qos),
            ('/wheel_encoders', JointState, 'sensor_msgs/msg/JointState', reliable_qos),
            ('/voltage', Float32, 'std_msgs/msg/Float32', reliable_qos),
            ('/context', String, 'std_msgs/msg/String', reliable_qos),
            ('/canmpc/context', Context, 'rai_controller/msg/Context', reliable_qos),
            ('/canmpc/humans', HumanStates, 'rai_controller/msg/HumanStates', reliable_qos),
            ('/cca_nmpc/context_input', ContextInput, 'rai_human_perception/msg/ContextInput', reliable_qos),
            ('/human_perception/context_input', ContextInput, 'rai_human_perception/msg/ContextInput', reliable_qos),
            ('/cca_nmpc/humans', HumanArray, 'rai_human_perception/msg/HumanArray', reliable_qos),
            ('/canmpc/context_json', String, 'std_msgs/msg/String', reliable_qos),
            ('/canmpc/humans_json', String, 'std_msgs/msg/String', reliable_qos),
            ('/canmpc/adaptive_bounds', AdaptiveBounds, 'rai_controller/msg/AdaptiveBounds', reliable_qos),
            ('/canmpc/adaptive_bounds_array', Float32MultiArray, 'std_msgs/msg/Float32MultiArray', reliable_qos),
            ('/canmpc/local_reference_path', NavPath, 'nav_msgs/msg/Path', reliable_qos),
            ('/canmpc/predicted_trajectory', NavPath, 'nav_msgs/msg/Path', reliable_qos),
            ('/canmpc/solver_stats', SolverStats, 'rai_controller/msg/SolverStats', reliable_qos),
            ('/perception/debug/image', Image, 'sensor_msgs/msg/Image', sensor_qos),
            ('/perception/debug/depth', Image, 'sensor_msgs/msg/Image', sensor_qos),
            ('/perception/debug/tracks', String, 'std_msgs/msg/String', reliable_qos),
            ('/perception/debug/latency', Float32MultiArray, 'std_msgs/msg/Float32MultiArray', reliable_qos),
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
            'research_schema_version': '3.0.0',
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
            'scenario_spec': scenario_by_id(self.scenario_id) or {},
            'dataset_groups': [
                'human_motion_dataset',
                'context_dataset',
                'navigation_dataset',
                'baseline_comparison_dataset',
            ],
            'perception_context_input': {
                'samples': 0,
                'last': None,
            },
            'perception_humans': {
                'samples': 0,
                'last': None,
            },
            'perception_debug': {
                'tracks_samples': 0,
                'latency_samples': [],
                'last_tracks': None,
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
                'context': {
                    'mean_phi_h': None,
                    'peak_phi_h': None,
                    'context_activation_duration': 0.0,
                    'adaptive_distance_gain': None,
                    'adaptive_velocity_reduction': None,
                    'context_stability': None,
                    'context_response_time': None,
                },
                'tracking': {
                    'rmse_xy': None,
                    'rmse_theta': None,
                    'max_lateral_error': None,
                    'path_length': None,
                    'travel_time': None,
                    'goal_reaching_rate': None,
                    'tracking_error': None,
                },
                'safety': {
                    'd_min': None,
                    'd_avg': None,
                    'd_5percentile': None,
                    'violation_count': 0,
                    'violation_duration': 0.0,
                    'collision_count': 0,
                    'near_miss_count': 0,
                    'time_to_collision': None,
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
                    'control_energy': None,
                },
                'realtime': {
                    'solve_time_mean_ms': None,
                    'solve_time_median_ms': None,
                    'solve_time_p95_ms': None,
                    'solve_time_max_ms': None,
                    'solve_time_std_ms': None,
                    'timeout_rate': None,
                    'sample_count_mean': None,
                    'real_time_factor': None,
                    'cpu_usage': None,
                    'memory_usage': None,
                },
                'prediction': {
                    'prediction_rmse': None,
                    'prediction_mae': None,
                    'prediction_horizon_error': None,
                    'velocity_prediction_error': None,
                },
                'perception': {
                    'detection_precision': None,
                    'detection_recall': None,
                    'tracking_rate': None,
                    'id_switches': None,
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
            'video_metadata': {},
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
        self._open_csv_logs()

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
        self._close_csv_logs()

        end_time = self.get_clock().now()
        duration = (end_time - self.start_time).nanoseconds / 1e9 if self.start_time else 0.0
        self.metadata['duration'] = duration
        self.metadata['end_time'] = datetime.now().strftime('%Y%m%d_%H%M%S')
        self._compute_run_metrics()
        self._write_run_summary()
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

    def _open_csv_logs(self):
        if not self.current_session_path:
            return
        derived_path = self.current_session_path / 'derived'
        derived_path.mkdir(parents=True, exist_ok=True)
        schemas = {
            'robot.csv': [
                'time', 'x_r', 'y_r', 'theta_r', 'vx_odom', 'vy_odom', 'omega_odom',
                'vx_cmd', 'vy_cmd', 'omega_cmd', 'reference_x', 'reference_y', 'reference_theta',
                'path_length', 'travel_time', 'tracking_error_xy', 'tracking_error_theta',
            ],
            'human.csv': [
                'time', 'human_id', 'x_h', 'y_h', 'vx_h', 'vy_h', 'tracking_confidence',
                'age_sec', 'covariance_x', 'covariance_y', 'covariance_vx', 'covariance_vy',
                'covariance_trace', 'source',
            ],
            'context.csv': [
                'time', 'phi_h', 'nearest_human_id', 'nearest_human_distance', 'relative_speed',
                'relative_heading', 'human_confidence', 'human_count', 'tracking_quality',
                'occlusion_flag', 'context_source',
            ],
            'adaptive_constraints.csv': [
                'time', 'phi_h', 'd_safe', 'vx_max', 'vy_max', 'omega_max', 'q_scale',
            ],
            'solver.csv': [
                'time', 'solve_time_ms', 'sample_count', 'best_cost', 'timeout', 'collision', 'status',
            ],
            'predicted_human.csv': [
                'time', 'human_id', 'x_pred', 'y_pred', 'vx_pred', 'vy_pred', 'horizon_step', 'prediction_source',
            ],
            'run_summary.csv': [
                'run_id', 'scenario_id', 'controller_id', 'environment', 'duration_sec',
                'mean_phi_h', 'peak_phi_h', 'context_activation_duration', 'adaptive_distance_gain',
                'adaptive_velocity_reduction', 'rmse_xy', 'rmse_theta', 'path_length', 'travel_time',
                'goal_reaching_rate', 'minimum_human_distance', 'safety_violation_count',
                'safety_violation_duration', 'near_miss_count', 'time_to_collision', 'jerk_mean',
                'jerk_max', 'mean_delta_u', 'control_energy', 'solve_time_mean_ms', 'solve_time_max_ms',
                'solve_time_std_ms', 'timeout_rate', 'prediction_rmse', 'prediction_mae',
                'detection_precision', 'detection_recall', 'tracking_rate', 'id_switches',
            ],
            'video_metadata.csv': [
                'scenario', 'controller', 'video', 'humans', 'environment', 'operator', 'run_id', 'notes',
            ],
        }
        for filename, fields in schemas.items():
            handle = open(derived_path / filename, 'w', encoding='utf-8', newline='')
            writer = csv.DictWriter(handle, fieldnames=fields)
            writer.writeheader()
            self.csv_writers[filename] = (handle, writer)

    def _close_csv_logs(self):
        for handle, _ in self.csv_writers.values():
            handle.close()
        self.csv_writers.clear()

    def _write_csv_row(self, filename: str, row: dict[str, Any]):
        target = self.csv_writers.get(filename)
        if target is None:
            return
        _, writer = target
        writer.writerow(row)

    def _topic_callback(self, topic_name: str, msg):
        self._write_message(topic_name, msg)
        if not self.is_recording:
            if topic_name == '/voltage':
                self._handle_voltage(msg)
            return

        self.metadata['samples'] += 1
        samples_by_topic = self.metadata['samples_by_topic']
        samples_by_topic[topic_name] = samples_by_topic.get(topic_name, 0) + 1

        if topic_name in {'/odom', '/odom_combined'}:
            self._handle_odom(msg)
        elif topic_name == '/cmd_vel':
            self._handle_cmd_vel(msg)
        elif topic_name == '/voltage':
            self._handle_voltage(msg)
        elif topic_name == '/canmpc/context':
            self._handle_context_msg(msg)
        elif topic_name == '/canmpc/humans':
            self._handle_humans_msg(msg)
        elif topic_name in {'/cca_nmpc/context_input', '/human_perception/context_input'}:
            self._handle_context_input(msg)
        elif topic_name == '/cca_nmpc/humans':
            self._handle_perception_humans(msg)
        elif topic_name == '/canmpc/context_json':
            self._handle_context_json(msg.data)
        elif topic_name == '/canmpc/humans_json':
            self._handle_humans_json(msg.data)
        elif topic_name == '/canmpc/adaptive_bounds':
            self._handle_adaptive_bounds(msg)
        elif topic_name == '/canmpc/adaptive_bounds_array':
            self._handle_adaptive_bounds_array(msg)
        elif topic_name == '/canmpc/solver_stats':
            self._handle_solver_stats(msg)
        elif topic_name == '/perception/debug/tracks':
            self._handle_perception_tracks(msg.data)
        elif topic_name == '/perception/debug/latency':
            self._handle_perception_latency(msg)

    def _write_message(self, topic_name: str, msg):
        if self.is_recording and self.bag_writer:
            self.bag_writer.write(topic_name, serialize_message(msg), self.get_clock().now().nanoseconds)

    def _elapsed(self) -> float:
        if not self.start_time:
            return 0.0
        return (self.get_clock().now() - self.start_time).nanoseconds / 1e9

    def _handle_odom(self, msg: Odometry):
        stamp_key = (int(msg.header.stamp.sec), int(msg.header.stamp.nanosec))
        if self.last_odom_stamp_key == stamp_key:
            return
        self.last_odom_stamp_key = stamp_key
        sample = {
            't': self._elapsed(),
            'vx': msg.twist.twist.linear.x,
            'vy': msg.twist.twist.linear.y,
            'omega': msg.twist.twist.angular.z,
        }
        self.metadata['velocity_profile'].append(sample)
        self.latest_odom_sample = {
            'time': sample['t'],
            'x_r': float(msg.pose.pose.position.x),
            'y_r': float(msg.pose.pose.position.y),
            'theta_r': 0.0,
            'vx_odom': float(msg.twist.twist.linear.x),
            'vy_odom': float(msg.twist.twist.linear.y),
            'omega_odom': float(msg.twist.twist.angular.z),
        }
        self._write_robot_row()

    def _handle_cmd_vel(self, msg: Twist):
        sample = {
            't': self._elapsed(),
            'vx': msg.linear.x,
            'vy': msg.linear.y,
            'omega': msg.angular.z,
        }
        self.metadata['last_cmd_vel'] = sample
        self.metadata['cmd_profile'].append(sample)
        self.latest_cmd_sample = {
            'time': sample['t'],
            'vx_cmd': float(msg.linear.x),
            'vy_cmd': float(msg.linear.y),
            'omega_cmd': float(msg.angular.z),
        }
        self._write_robot_row()

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
        self.latest_context_row = {
            'time': self._elapsed(),
            'phi_h': payload['phi_h'],
            'nearest_human_distance': payload['d_h'],
            'occlusion_flag': payload['occlusion_flag'],
            'context_source': 'controller_context',
        }
        self._write_context_row()

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
                'x': float(human.x),
                'y': float(human.y),
                'vx': float(human.vx),
                'vy': float(human.vy),
                'confidence': float(human.confidence),
                'age_sec': float(human.age_sec),
                'covariance_x': float(human.covariance_x),
                'covariance_y': float(human.covariance_y),
                'covariance_vx': float(human.covariance_vx),
                'covariance_vy': float(human.covariance_vy),
                'covariance_trace': float(
                    human.covariance_x + human.covariance_y + human.covariance_vx + human.covariance_vy
                ),
            })
        self.metadata['human_state']['samples'] += 1
        self.metadata['human_state']['last'] = {
            'stamp': {'sec': msg.header.stamp.sec, 'nanosec': msg.header.stamp.nanosec},
            'humans': humans,
        }
        self.latest_humans_row = humans
        for human in humans:
            self._write_csv_row('human.csv', {
                'time': self._elapsed(),
                'human_id': human['id'],
                'x_h': human['x'],
                'y_h': human['y'],
                'vx_h': human['vx'],
                'vy_h': human['vy'],
                'tracking_confidence': human['confidence'],
                'age_sec': human['age_sec'],
                'covariance_x': human['covariance_x'],
                'covariance_y': human['covariance_y'],
                'covariance_vx': human['covariance_vx'],
                'covariance_vy': human['covariance_vy'],
                'covariance_trace': human['covariance_trace'],
                'source': 'controller_humans',
            })
        self._write_context_row()
        self._write_prediction_rows()

    def _handle_context_input(self, msg: ContextInput):
        stamp_key = (int(msg.header.stamp.sec), int(msg.header.stamp.nanosec))
        if self.last_context_input_stamp_key == stamp_key:
            return
        self.last_context_input_stamp_key = stamp_key
        self.metadata['perception_context_input']['samples'] += 1
        self.metadata['perception_context_input']['last'] = {
            'stamp': {'sec': msg.header.stamp.sec, 'nanosec': msg.header.stamp.nanosec},
            'nearest_human_distance': float(msg.nearest_human_distance),
            'relative_speed': float(msg.relative_speed),
            'human_count': int(msg.human_count),
            'tracking_quality': float(msg.tracking_quality),
        }
        self.latest_context_input_row = {
            'time': self._elapsed(),
            'relative_speed': float(msg.relative_speed),
            'human_count': int(msg.human_count),
            'tracking_quality': float(msg.tracking_quality),
        }
        self._write_context_row()

    def _handle_perception_humans(self, msg: HumanArray):
        humans = []
        for human in msg.humans:
            humans.append({
                'id': int(human.id),
                'x': float(human.x),
                'y': float(human.y),
                'vx': float(human.vx),
                'vy': float(human.vy),
                'confidence': float(human.confidence),
                'covariance_x': float(human.covariance_x),
                'covariance_y': float(human.covariance_y),
                'covariance_vx': float(human.covariance_vx),
                'covariance_vy': float(human.covariance_vy),
                'covariance_trace': float(
                    human.covariance_x +
                    human.covariance_y +
                    human.covariance_vx +
                    human.covariance_vy
                ),
            })
        self.metadata['perception_humans']['samples'] += 1
        self.metadata['perception_humans']['last'] = {
            'stamp': {'sec': msg.header.stamp.sec, 'nanosec': msg.header.stamp.nanosec},
            'humans': humans,
        }
        for human in humans:
            self._write_csv_row('human.csv', {
                'time': self._elapsed(),
                'human_id': human['id'],
                'x_h': human['x'],
                'y_h': human['y'],
                'vx_h': human['vx'],
                'vy_h': human['vy'],
                'tracking_confidence': human['confidence'],
                'age_sec': '',
                'covariance_x': human['covariance_x'],
                'covariance_y': human['covariance_y'],
                'covariance_vx': human['covariance_vx'],
                'covariance_vy': human['covariance_vy'],
                'covariance_trace': human['covariance_trace'],
                'source': 'perception_humans',
            })

    def _handle_perception_tracks(self, data: str):
        self.metadata['perception_debug']['tracks_samples'] += 1
        self.metadata['perception_debug']['last_tracks'] = {
            't': self._elapsed(),
            'raw': data,
        }

    def _handle_perception_latency(self, msg: Float32MultiArray):
        self.metadata['perception_debug']['latency_samples'].append({
            't': self._elapsed(),
            'values': [float(value) for value in msg.data],
        })

    def _handle_adaptive_bounds(self, msg: AdaptiveBounds):
        context = self.metadata['continuous_context']
        self._append_if_number(context['d_safe_samples'], msg.d_safe)
        self._append_if_number(context['vx_max_samples'], msg.vx_max)
        self._append_if_number(context['vy_max_samples'], msg.vy_max)
        self._append_if_number(context['omega_max_samples'], msg.omega_max)
        self._write_csv_row('adaptive_constraints.csv', {
            'time': self._elapsed(),
            'phi_h': self.latest_context_row['phi_h'] if self.latest_context_row else '',
            'd_safe': float(msg.d_safe),
            'vx_max': float(msg.vx_max),
            'vy_max': float(msg.vy_max),
            'omega_max': float(msg.omega_max),
            'q_scale': float(msg.q_scale),
        })

    def _handle_adaptive_bounds_array(self, msg: Float32MultiArray):
        data = list(msg.data)
        if len(data) >= 5:
            context = self.metadata['continuous_context']
            self._append_if_number(context['d_safe_samples'], data[1])
            self._append_if_number(context['vx_max_samples'], data[2])
            self._append_if_number(context['vy_max_samples'], data[3])
            self._append_if_number(context['omega_max_samples'], data[4])

    def _handle_solver_stats(self, msg: SolverStats):
        payload = {
            'solve_time_ms': float(msg.solve_time_ms),
            'iter_count': int(msg.iter_count),
            'status': str(msg.status),
            'timeout_flag': bool(msg.timeout_flag),
            'collision_flag': bool(msg.collision_flag),
        }
        solver = self.metadata['solver']
        solver['samples'] += 1
        solver['last'] = payload
        if payload.get('timeout_flag'):
            solver['timeouts'] += 1
        solver['solve_time_ms_samples'].append(payload['solve_time_ms'])
        self._write_csv_row('solver.csv', {
            'time': self._elapsed(),
            'solve_time_ms': float(msg.solve_time_ms),
            'sample_count': int(msg.iter_count),
            'best_cost': '',
            'timeout': bool(msg.timeout_flag),
            'collision': bool(msg.collision_flag),
            'status': str(msg.status),
        })

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

    def _write_robot_row(self):
        if self.latest_odom_sample is None:
            return
        self._write_csv_row('robot.csv', {
            'time': self.latest_odom_sample.get('time', self._elapsed()),
            'x_r': self.latest_odom_sample.get('x_r', ''),
            'y_r': self.latest_odom_sample.get('y_r', ''),
            'theta_r': self.latest_odom_sample.get('theta_r', ''),
            'vx_odom': self.latest_odom_sample.get('vx_odom', ''),
            'vy_odom': self.latest_odom_sample.get('vy_odom', ''),
            'omega_odom': self.latest_odom_sample.get('omega_odom', ''),
            'vx_cmd': self.latest_cmd_sample.get('vx_cmd', '') if self.latest_cmd_sample else '',
            'vy_cmd': self.latest_cmd_sample.get('vy_cmd', '') if self.latest_cmd_sample else '',
            'omega_cmd': self.latest_cmd_sample.get('omega_cmd', '') if self.latest_cmd_sample else '',
            'reference_x': '',
            'reference_y': '',
            'reference_theta': '',
            'path_length': '',
            'travel_time': self._elapsed(),
            'tracking_error_xy': '',
            'tracking_error_theta': '',
        })

    def _write_context_row(self):
        if self.latest_context_row is None:
            return
        nearest_human_id = ''
        human_confidence = ''
        if self.latest_humans_row:
            nearest = min(
                self.latest_humans_row,
                key=lambda item: item.get('x', 0.0) ** 2 + item.get('y', 0.0) ** 2,
            )
            nearest_human_id = nearest.get('id', '')
            human_confidence = nearest.get('confidence', '')
        self._write_csv_row('context.csv', {
            'time': self.latest_context_row.get('time', self._elapsed()),
            'phi_h': self.latest_context_row.get('phi_h', ''),
            'nearest_human_id': nearest_human_id,
            'nearest_human_distance': self.latest_context_row.get('nearest_human_distance', ''),
            'relative_speed': self.latest_context_input_row.get('relative_speed', '') if self.latest_context_input_row else '',
            'relative_heading': '',
            'human_confidence': human_confidence,
            'human_count': self.latest_context_input_row.get('human_count', '') if self.latest_context_input_row else len(self.latest_humans_row),
            'tracking_quality': self.latest_context_input_row.get('tracking_quality', '') if self.latest_context_input_row else '',
            'occlusion_flag': self.latest_context_row.get('occlusion_flag', ''),
            'context_source': self.latest_context_row.get('context_source', ''),
        })

    def _write_prediction_rows(self):
        if not self.latest_humans_row:
            return
        for human in self.latest_humans_row:
            for horizon_step in range(1, 6):
                dt = 0.1 * horizon_step
                self._write_csv_row('predicted_human.csv', {
                    'time': self._elapsed(),
                    'human_id': human['id'],
                    'x_pred': human['x'] + human['vx'] * dt,
                    'y_pred': human['y'] + human['vy'] * dt,
                    'vx_pred': human['vx'],
                    'vy_pred': human['vy'],
                    'horizon_step': horizon_step,
                    'prediction_source': 'constant_velocity',
                })

    def _compute_run_metrics(self):
        metrics = self.metadata['metrics']
        context = self.metadata['continuous_context']
        phi_h = [float(v) for v in context.get('phi_h_samples', []) if isinstance(v, (int, float))]
        d_h = [float(v) for v in context.get('d_h_samples', []) if isinstance(v, (int, float))]
        d_safe = [float(v) for v in context.get('d_safe_samples', []) if isinstance(v, (int, float))]
        vx_max = [float(v) for v in context.get('vx_max_samples', []) if isinstance(v, (int, float))]
        vy_max = [float(v) for v in context.get('vy_max_samples', []) if isinstance(v, (int, float))]
        omega_max = [float(v) for v in context.get('omega_max_samples', []) if isinstance(v, (int, float))]

        if phi_h:
            metrics['context']['mean_phi_h'] = sum(phi_h) / len(phi_h)
            metrics['context']['peak_phi_h'] = max(phi_h)
            metrics['context']['context_activation_duration'] = sum(0.1 for value in phi_h if value > 0.7)
            metrics['context']['context_stability'] = (
                sum(abs(phi_h[i] - phi_h[i - 1]) for i in range(1, len(phi_h))) / max(1, len(phi_h) - 1)
            )
            first_active_idx = next((i for i, value in enumerate(phi_h) if value > 0.7), None)
            metrics['context']['context_response_time'] = first_active_idx * 0.1 if first_active_idx is not None else None
        if d_safe:
            metrics['context']['adaptive_distance_gain'] = sum(d_safe) / len(d_safe) - 0.5
        if vx_max or vy_max or omega_max:
            reductions = []
            if vx_max:
                reductions.append(0.45 - (sum(vx_max) / len(vx_max)))
            if vy_max:
                reductions.append(0.35 - (sum(vy_max) / len(vy_max)))
            if omega_max:
                reductions.append(1.0 - (sum(omega_max) / len(omega_max)))
            metrics['context']['adaptive_velocity_reduction'] = sum(reductions) / len(reductions) if reductions else None

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
            metrics['tracking']['travel_time'] = self.metadata.get('duration', 0.0)

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
            metrics['control']['control_energy'] = effort
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
            mean = metrics['realtime']['solve_time_mean_ms']
            metrics['realtime']['solve_time_std_ms'] = (
                sum((value - mean) ** 2 for value in solve_times) / len(solve_times)
            ) ** 0.5
        if solver.get('samples', 0):
            metrics['realtime']['timeout_rate'] = solver.get('timeouts', 0) / solver['samples']
            metrics['realtime']['sample_count_mean'] = solver.get('samples', 0)
            metrics['realtime']['real_time_factor'] = (
                (self.metadata.get('duration', 0.0) / max(1e-6, sum(solve_times) / 1000.0))
                if solve_times else None
            )

        self.metadata['video_metadata'] = {
            'scenario': self.scenario_id,
            'controller': self.controller_id,
            'video': f'{self.run_id}.mp4',
            'humans': len(self.latest_humans_row),
            'environment': self.environment,
            'operator': 'user',
            'run_id': self.run_id,
            'notes': '',
        }

    def _write_run_summary(self):
        metrics = self.metadata['metrics']
        self._write_csv_row('run_summary.csv', {
            'run_id': self.run_id,
            'scenario_id': self.scenario_id,
            'controller_id': self.controller_id,
            'environment': self.environment,
            'duration_sec': self.metadata.get('duration'),
            'mean_phi_h': metrics['context'].get('mean_phi_h'),
            'peak_phi_h': metrics['context'].get('peak_phi_h'),
            'context_activation_duration': metrics['context'].get('context_activation_duration'),
            'adaptive_distance_gain': metrics['context'].get('adaptive_distance_gain'),
            'adaptive_velocity_reduction': metrics['context'].get('adaptive_velocity_reduction'),
            'rmse_xy': metrics['tracking'].get('rmse_xy'),
            'rmse_theta': metrics['tracking'].get('rmse_theta'),
            'path_length': metrics['tracking'].get('path_length'),
            'travel_time': metrics['tracking'].get('travel_time'),
            'goal_reaching_rate': metrics['tracking'].get('goal_reaching_rate'),
            'minimum_human_distance': metrics['safety'].get('d_min'),
            'safety_violation_count': metrics['safety'].get('violation_count'),
            'safety_violation_duration': metrics['safety'].get('violation_duration'),
            'near_miss_count': metrics['safety'].get('near_miss_count'),
            'time_to_collision': metrics['safety'].get('time_to_collision'),
            'jerk_mean': metrics['smoothness'].get('jerk_mean'),
            'jerk_max': metrics['smoothness'].get('jerk_max'),
            'mean_delta_u': metrics['smoothness'].get('mean_abs_delta_u'),
            'control_energy': metrics['control'].get('control_energy'),
            'solve_time_mean_ms': metrics['realtime'].get('solve_time_mean_ms'),
            'solve_time_max_ms': metrics['realtime'].get('solve_time_max_ms'),
            'solve_time_std_ms': metrics['realtime'].get('solve_time_std_ms'),
            'timeout_rate': metrics['realtime'].get('timeout_rate'),
            'prediction_rmse': metrics['prediction'].get('prediction_rmse'),
            'prediction_mae': metrics['prediction'].get('prediction_mae'),
            'detection_precision': metrics['perception'].get('detection_precision'),
            'detection_recall': metrics['perception'].get('detection_recall'),
            'tracking_rate': metrics['perception'].get('tracking_rate'),
            'id_switches': metrics['perception'].get('id_switches'),
        })
        self._write_csv_row('video_metadata.csv', self.metadata.get('video_metadata', {}))

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
