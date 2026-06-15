"""ROS2/ZMQ bridge for the website SCADA backend.

Shared protocol:
    Commands:  REQ/REP JSON on port 5555
    Telemetry: PUB/SUB JSON on port 5556
    Camera:    PUB raw JPEG/PNG bytes on port 5557
"""

import math
import os
import re
import signal
import subprocess
import time
import base64
from collections import deque
from datetime import datetime, timezone
from pathlib import Path as FilePath
from threading import RLock, Thread

import cv2
import numpy as np
import rclpy
import tf2_ros
import zmq
from geometry_msgs.msg import PoseStamped, Twist
from nav_msgs.msg import OccupancyGrid, Odometry, Path
from rclpy.node import Node
from rclpy.qos import QoSDurabilityPolicy, QoSHistoryPolicy, QoSProfile, QoSReliabilityPolicy
from sensor_msgs.msg import Image, Imu
from std_msgs.msg import Bool, Float32

MAX_LINEAR_VELOCITY = 1.0
MAX_ANGULAR_VELOCITY = 2.0
MAX_PATROL_WAYPOINT_TOLERANCE = 2.0
MIN_PATROL_WAYPOINT_TOLERANCE = 0.05


class RaiControlNode(Node):
    def __init__(self, zmq_ports, camera_topic='/camera/color/image_raw'):
        super().__init__('scada_control_node')

        self.declare_parameter('zmq_cmd_port', zmq_ports.get('cmd', 5555))
        self.declare_parameter('zmq_telemetry_port', zmq_ports.get('telemetry', 5556))
        self.declare_parameter('zmq_camera_port', zmq_ports.get('camera', 5557))
        self.declare_parameter('camera_topic', camera_topic)
        self.declare_parameter('slam_start_cmd', '')
        self.declare_parameter('nav2_start_cmd', '')
        self.declare_parameter('map_saver_cmd', 'ros2 run nav2_map_server map_saver_cli -f {map_prefix}')
        self.declare_parameter('map_save_dir', '/home/rai/rai_ros2/data/map')

        cmd_port = self.get_parameter('zmq_cmd_port').value
        telemetry_port = self.get_parameter('zmq_telemetry_port').value
        camera_port = self.get_parameter('zmq_camera_port').value
        cam_topic = self.get_parameter('camera_topic').value
        self._slam_start_cmd = str(self.get_parameter('slam_start_cmd').value or '')
        self._nav2_start_cmd = str(self.get_parameter('nav2_start_cmd').value or '')
        self._map_saver_cmd = str(self.get_parameter('map_saver_cmd').value or '')
        self._map_save_dir = FilePath(str(self.get_parameter('map_save_dir').value or '/tmp/rai_maps')).expanduser()

        self.cmd_vel_pub = self.create_publisher(Twist, '/cmd_vel', 10)
        self.goal_pub = self.create_publisher(PoseStamped, '/goal_pose', 10)

        self.create_subscription(Odometry, 'odom', self.odom_cb, 10)
        self.create_subscription(Imu, 'imu/data_raw', self.imu_cb, 10)
        self.create_subscription(Float32, '/PowerVoltage', self.voltage_cb, 10)
        self.create_subscription(Bool, '/robot_charging_flag', self.charging_cb, 10)

        self.tf_buffer = tf2_ros.Buffer()
        self.tf_listener = tf2_ros.TransformListener(self.tf_buffer, self)

        map_qos = QoSProfile(
            reliability=QoSReliabilityPolicy.RELIABLE,
            durability=QoSDurabilityPolicy.TRANSIENT_LOCAL,
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=1
        )
        self.create_subscription(OccupancyGrid, '/map', self.map_cb, map_qos)

        self.create_subscription(Path, '/plan', self.plan_cb, 10)
        self.create_subscription(Path, '/local_plan', self.local_plan_cb, 10)

        cam_qos = QoSProfile(
            reliability=QoSReliabilityPolicy.BEST_EFFORT,
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=1
        )
        self.create_subscription(Image, cam_topic, self.camera_cb, cam_qos)

        self.zmq_context = zmq.Context()

        self.telemetry_pub = self.zmq_context.socket(zmq.PUB)
        self.telemetry_pub.setsockopt(zmq.SNDHWM, 2)
        self.telemetry_pub.setsockopt(zmq.LINGER, 0)
        self.telemetry_pub.bind(f"tcp://0.0.0.0:{telemetry_port}")

        self.camera_pub = self.zmq_context.socket(zmq.PUB)
        self.camera_pub.setsockopt(zmq.SNDHWM, 2)
        self.camera_pub.setsockopt(zmq.LINGER, 0)
        self.camera_pub.bind(f"tcp://0.0.0.0:{camera_port}")

        self.cmd_rep = self.zmq_context.socket(zmq.REP)
        self.cmd_rep.setsockopt(zmq.RCVHWM, 2)
        self.cmd_rep.setsockopt(zmq.SNDHWM, 2)
        self.cmd_rep.setsockopt(zmq.LINGER, 0)
        self.cmd_rep.bind(f"tcp://0.0.0.0:{cmd_port}")

        self.telemetry_data = {
            "odom": {"x": 0.0, "y": 0.0, "z": 0.0, "v_x": 0.0, "v_y": 0.0, "v_z": 0.0, "yaw": 0.0},
            "map_pose": {"x": 0.0, "y": 0.0, "yaw": 0.0},
            "imu": {"ax": 0.0, "ay": 0.0, "az": 0.0},
            "voltage": 0.0,
            "charging": False,
            "plan": [],
            "local_plan": [],
            "navigation_mode": "idle",
            "patrol": {
                "status": "idle",
                "run_id": None,
                "schedule_id": None,
                "route_id": None,
                "current_loop": 0,
                "total_loops": 0,
                "current_waypoint_index": -1,
                "total_waypoints": 0,
                "last_goal": None,
                "message": None,
                "updated_at": None,
            }
        }

        self.map_data = None
        self.map_dirty = False
        self._last_map_msg = None
        self._managed_processes = {}
        self._current_map_path = None

        self._patrol_status = "idle"
        self._patrol_phase = "idle"
        self._patrol_run_id = None
        self._patrol_schedule_id = None
        self._patrol_route_id = None
        self._patrol_waypoints = []
        self._patrol_home = None
        self._patrol_total_loops = 0
        self._patrol_current_loop = 0
        self._patrol_current_waypoint_index = -1
        self._patrol_waypoint_tolerance = 0.25
        self._patrol_goal_sent_at = None
        self._patrol_waypoint_timeout_sec = 180.0
        self._patrol_last_goal = None
        self._patrol_message = None
        self._patrol_lock = RLock()

        self.cmd_thread = Thread(target=self.cmd_loop, daemon=True)
        self.cmd_thread.start()

        self.create_timer(0.1, self.publish_telemetry)
        self.create_timer(2.0, self.publish_map)
        self.create_timer(0.1, self.update_map_pose)
        self.create_timer(0.2, self.patrol_tick)

    def odom_cb(self, msg):
        self.telemetry_data["odom"] = {
            "x": msg.pose.pose.position.x,
            "y": msg.pose.pose.position.y,
            "z": msg.pose.pose.position.z,
            "v_x": msg.twist.twist.linear.x,
            "v_y": msg.twist.twist.linear.y,
            "v_z": msg.twist.twist.angular.z,
            "yaw": self._yaw_from_quat(msg.pose.pose.orientation)
        }

    @staticmethod
    def _yaw_from_quat(q):
        siny_cosp = 2 * (q.w * q.z + q.x * q.y)
        cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z)
        return math.atan2(siny_cosp, cosy_cosp)

    @staticmethod
    def _quat_from_yaw(yaw: float) -> tuple[float, float]:
        return math.sin(yaw / 2.0), math.cos(yaw / 2.0)

    def _now_iso(self) -> str:
        return datetime.now(timezone.utc).isoformat()

    def _list_active_nodes(self):
        try:
            graph_nodes = self.get_node_names_and_namespaces()
        except Exception as exc:
            return {"status": "error", "message": str(exc), "nodes": []}

        nodes = []
        seen = set()
        now = self._now_iso()
        for name, namespace in sorted(graph_nodes, key=lambda item: (item[1], item[0])):
            full_name = self._format_node_name(namespace, name)
            if full_name in seen:
                continue
            seen.add(full_name)
            nodes.append({
                "node_name": full_name,
                "package_name": self._infer_package_name(full_name),
                "status": "active",
                "source": "raspi_ros2",
                "last_changed_at": now,
                "metadata": {
                    "namespace": namespace,
                    "ros_name": name,
                },
            })

        managed_processes = []
        for name, process in sorted(self._managed_processes.items()):
            return_code = process.poll()
            managed_processes.append({
                "name": name,
                "pid": process.pid,
                "status": "running" if return_code is None else "stopped",
                "return_code": return_code,
            })

        return {
            "status": "ok",
            "source": "raspi_ros2",
            "timestamp": now,
            "nodes": nodes,
            "managed_processes": managed_processes,
        }

    @staticmethod
    def _format_node_name(namespace: str, name: str) -> str:
        namespace = (namespace or "/").strip()
        name = (name or "").strip("/")
        if namespace in {"", "/"}:
            return f"/{name}" if name else "/"
        return f"/{namespace.strip('/')}/{name}".replace("//", "/")

    @staticmethod
    def _infer_package_name(node_name: str) -> str | None:
        clean = node_name.strip("/")
        package_hints = (
            ("scada_control_node", "rai_scada_bridge"),
            ("rplidar", "rplidar_ros"),
            ("ldlidar", "rai_lidar_ros2"),
            ("slam_toolbox", "slam_toolbox"),
            ("map_server", "nav2_map_server"),
            ("amcl", "nav2_amcl"),
            ("bt_navigator", "nav2_bt_navigator"),
            ("controller_server", "nav2_controller"),
            ("planner_server", "nav2_planner"),
            ("behavior_server", "nav2_behaviors"),
            ("waypoint_follower", "nav2_waypoint_follower"),
            ("lifecycle_manager", "nav2_lifecycle_manager"),
            ("robot_state_publisher", "robot_state_publisher"),
            ("joint_state_publisher", "joint_state_publisher"),
            ("context_aware", "context-aware"),
        )
        for needle, package in package_hints:
            if needle in clean:
                return package
        return None

    def _collect_log_entries(self, payload: dict):
        limit = int(payload.get("limit") or 200)
        limit = max(1, min(limit, 500))
        per_file = max(20, min(limit, 200))
        files = self._log_file_candidates(payload.get("paths"))

        entries = []
        for path in files:
            for line in self._tail_lines(path, per_file):
                line = line.strip()
                if not line:
                    continue
                timestamp, severity, message = self._parse_log_line(line)
                entries.append({
                    "timestamp": timestamp or self._now_iso(),
                    "severity": severity,
                    "source": f"rai_ros2:{path.name}",
                    "message": message[:2000],
                    "metadata": {"path": str(path)},
                })

        entries.sort(key=lambda item: item.get("timestamp") or "", reverse=True)
        return {
            "status": "ok",
            "source": "rai_ros2",
            "logs": entries[:limit],
            "files": [str(path) for path in files],
            "timestamp": self._now_iso(),
        }

    def _log_file_candidates(self, requested_paths=None):
        roots = []
        if isinstance(requested_paths, list):
            roots.extend(str(item) for item in requested_paths if item)

        env_paths = os.environ.get("RAI_LOG_PATHS", "")
        if env_paths:
            roots.extend(path for path in env_paths.split(os.pathsep) if path)

        roots.extend([
            str(FilePath.home() / ".ros" / "log" / "latest"),
            str(FilePath.home() / ".ros" / "log"),
            "/home/rai/rai_ros2/logs",
            "/tmp/rai_ros2/logs",
        ])

        files = []
        seen = set()
        for root in roots:
            path = FilePath(root).expanduser()
            candidates = []
            if path.is_file():
                candidates = [path]
            elif path.is_dir():
                candidates = [
                    *path.glob("*.log"),
                    *path.glob("*.txt"),
                    *path.glob("**/*.log"),
                ]
            for candidate in candidates:
                try:
                    resolved = candidate.resolve()
                    if resolved in seen or not resolved.is_file():
                        continue
                    seen.add(resolved)
                    files.append(resolved)
                except OSError:
                    continue

        files.sort(key=lambda item: item.stat().st_mtime if item.exists() else 0, reverse=True)
        return files[:20]

    @staticmethod
    def _tail_lines(path: FilePath, max_lines: int):
        try:
            with path.open("r", encoding="utf-8", errors="replace") as handle:
                return list(deque(handle, maxlen=max_lines))
        except OSError:
            return []

    @staticmethod
    def _parse_log_line(line: str):
        timestamp = None
        message = line

        iso_match = re.match(r"^(\d{4}-\d{2}-\d{2}[ T]\d{2}:\d{2}:\d{2}(?:[.,]\d+)?)", line)
        if iso_match:
            timestamp = iso_match.group(1).replace(",", ".")
            message = line[iso_match.end():].strip(" -")

        severity = "INFO"
        sev_match = re.search(r"\b(DEBUG|INFO|WARN|WARNING|ERROR|FATAL|CRITICAL)\b", line, re.IGNORECASE)
        if sev_match:
            severity = sev_match.group(1).upper()
            if severity == "WARN":
                severity = "WARNING"
            if severity == "FATAL":
                severity = "CRITICAL"

        return timestamp, severity, message

    @staticmethod
    def _safe_float(value, field_name: str) -> float:
        numeric = float(value)
        if not math.isfinite(numeric):
            raise ValueError(f"{field_name} must be finite")
        return numeric

    @staticmethod
    def _clamp(value: float, min_value: float, max_value: float) -> float:
        return max(min_value, min(max_value, value))

    @staticmethod
    def _safe_slug(value: str) -> str:
        slug = re.sub(r"[^a-zA-Z0-9_-]+", "_", value.strip()).strip("_")
        return slug or datetime.now(timezone.utc).strftime("map_%Y%m%d_%H%M%S")

    @staticmethod
    def _map_files_exist(map_prefix: FilePath) -> bool:
        return map_prefix.with_suffix(".yaml").exists() or map_prefix.with_suffix(".pgm").exists()

    def _unique_map_prefix(self, name: str) -> FilePath:
        map_prefix = self._map_save_dir / name
        if not self._map_files_exist(map_prefix):
            return map_prefix

        timestamp = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S")
        candidate = self._map_save_dir / f"{name}_{timestamp}"
        index = 2
        while self._map_files_exist(candidate):
            candidate = self._map_save_dir / f"{name}_{timestamp}_{index}"
            index += 1
        return candidate

    def _format_command(self, template: str, **kwargs) -> str:
        try:
            return template.format(**kwargs)
        except Exception:
            return template

    def _start_managed_process(self, name: str, command: str):
        command = str(command or "").strip()
        if not command:
            return {"status": "ok", "message": f"{name} command not configured"}

        existing = self._managed_processes.get(name)
        if existing is not None and existing.poll() is None:
            return {"status": "running", "pid": existing.pid}

        self.get_logger().info(f"Starting {name}: {command}")
        kwargs = {}
        if hasattr(os, "setsid"):
            kwargs["preexec_fn"] = os.setsid
        process = subprocess.Popen(command, shell=True, **kwargs)
        self._managed_processes[name] = process
        return {"status": "started", "pid": process.pid}

    def _stop_managed_process(self, name: str):
        process = self._managed_processes.get(name)
        if process is None or process.poll() is not None:
            self._managed_processes.pop(name, None)
            return {"status": "stopped", "message": f"{name} was not running"}

        self.get_logger().info(f"Stopping {name} pid={process.pid}")
        try:
            if hasattr(os, "killpg"):
                os.killpg(os.getpgid(process.pid), signal.SIGTERM)
            else:
                process.terminate()
            process.wait(timeout=5.0)
        except subprocess.TimeoutExpired:
            if hasattr(os, "killpg"):
                os.killpg(os.getpgid(process.pid), signal.SIGKILL)
            else:
                process.kill()
            process.wait(timeout=2.0)
        finally:
            self._managed_processes.pop(name, None)
        return {"status": "stopped"}

    def _switch_navigation_mode(self, mode: str, map_path: str | None = None):
        mode = mode.lower()
        if mode == "slam":
            nav2_stop = self._stop_managed_process("nav2")
            slam_start = self._start_managed_process("slam", self._slam_start_cmd)
            self.telemetry_data["navigation_mode"] = "slam"
            return {"status": "ok", "mode": "slam", "nav2": nav2_stop, "slam": slam_start}

        if mode == "nav2":
            slam_stop = self._stop_managed_process("slam")
            nav2_stop = self._stop_managed_process("nav2")
            self._current_map_path = map_path or self._current_map_path
            command = self._format_command(self._nav2_start_cmd, map=self._current_map_path or "")
            nav2_start = self._start_managed_process("nav2", command)
            self.telemetry_data["navigation_mode"] = "nav2"
            return {
                "status": "ok",
                "mode": "nav2",
                "map_path": self._current_map_path,
                "slam": slam_stop,
                "nav2_stop": nav2_stop,
                "nav2": nav2_start,
            }

        raise ValueError("mode must be slam or nav2")

    def _control_nav2(self, payload: dict):
        nav2_action = str(payload.get("action", "")).lower()
        if nav2_action == "start":
            map_path = payload.get("map_path") or payload.get("source")
            return self._switch_navigation_mode("nav2", map_path)
        if nav2_action == "stop":
            result = self._stop_managed_process("nav2")
            self.telemetry_data["navigation_mode"] = "idle"
            return {"status": "ok", "mode": "idle", "nav2": result}
        return {"status": "error", "message": "Unknown nav2_control action"}

    def _occupancy_grid_to_png(self, msg):
        w = msg.info.width
        h = msg.info.height
        data = np.asarray(msg.data, dtype=np.int16).reshape((h, w))
        img_array = np.zeros((h, w), dtype=np.uint8)
        img_array[data == -1] = 205
        img_array[data == 0] = 255
        occupied = data > 0
        img_array[occupied] = np.maximum(0, 255 - (data[occupied] * 2.55)).astype(np.uint8)
        img_array = np.flipud(img_array)
        ok, png_bytes = cv2.imencode('.png', img_array)
        if not ok:
            raise RuntimeError("cv2.imencode('.png') failed")
        return png_bytes.tobytes()

    def _save_current_map(self, payload: dict):
        if self._last_map_msg is None:
            return {"status": "error", "message": "No /map data available"}

        raw_name = str(payload.get("name") or "").strip()
        if raw_name:
            name = self._safe_slug(raw_name)
        else:
            name = datetime.now(timezone.utc).strftime("map_%Y%m%d_%H%M%S")
        self._map_save_dir.mkdir(parents=True, exist_ok=True)
        map_prefix = self._unique_map_prefix(name)
        command = self._format_command(
            self._map_saver_cmd,
            map_prefix=str(map_prefix),
            name=map_prefix.name,
        )

        if command:
            self.get_logger().info(f"Saving map with command: {command}")
            completed = subprocess.run(command, shell=True, text=True, capture_output=True, timeout=45.0)
            if completed.returncode != 0:
                message = (completed.stderr or completed.stdout or "map_saver_cli failed").strip()
                return {"status": "error", "message": message}

        yaml_path = map_prefix.with_suffix(".yaml")
        pgm_path = map_prefix.with_suffix(".pgm")
        yaml_text = yaml_path.read_text(encoding="utf-8") if yaml_path.exists() else ""
        pgm_bytes = pgm_path.read_bytes() if pgm_path.exists() else b""
        png_bytes = self._occupancy_grid_to_png(self._last_map_msg)
        self.camera_pub.send(b'MAP:' + png_bytes)
        self._current_map_path = str(yaml_path) if yaml_path.exists() else str(map_prefix)

        return {
            "status": "saved",
            "name": map_prefix.name,
            "source": self._current_map_path,
            "yaml_path": str(yaml_path),
            "pgm_path": str(pgm_path),
            "yaml": yaml_text,
            "pgm_base64": base64.b64encode(pgm_bytes).decode("ascii") if pgm_bytes else None,
            "png_base64": base64.b64encode(png_bytes).decode("ascii"),
            "map_info": self.telemetry_data.get("map_info"),
        }

    def _set_patrol_state(self, status: str, message: str | None = None):
        with self._patrol_lock:
            self._patrol_status = status
            self._patrol_message = message
            self.telemetry_data["patrol"] = {
                "status": self._patrol_status,
                "run_id": self._patrol_run_id,
                "schedule_id": self._patrol_schedule_id,
                "route_id": self._patrol_route_id,
                "current_loop": self._patrol_current_loop,
                "total_loops": self._patrol_total_loops,
                "current_waypoint_index": self._patrol_current_waypoint_index,
                "total_waypoints": len(self._patrol_waypoints),
                "last_goal": self._patrol_last_goal,
                "message": message,
                "updated_at": self._now_iso(),
            }

    def _is_patrol_active(self) -> bool:
        with self._patrol_lock:
            return self._patrol_status in {"starting", "running", "returning_home"}

    def _publish_goal(self, x: float, y: float, yaw: float = 0.0):
        goal = PoseStamped()
        goal.header.frame_id = "map"
        goal.header.stamp = self.get_clock().now().to_msg()
        goal.pose.position.x = float(x)
        goal.pose.position.y = float(y)
        qz, qw = self._quat_from_yaw(float(yaw))
        goal.pose.orientation.z = qz
        goal.pose.orientation.w = qw
        self.goal_pub.publish(goal)
        with self._patrol_lock:
            self._patrol_last_goal = {"x": float(x), "y": float(y), "yaw": float(yaw)}
            self._patrol_goal_sent_at = self.get_clock().now().nanoseconds / 1e9

    def _start_patrol(self, payload: dict):
        waypoints = payload.get("waypoints") or []
        loops = int(payload.get("loops") or 0)
        home = payload.get("home")
        if not isinstance(waypoints, list) or len(waypoints) < 2:
            raise ValueError("Patrol requires at least 2 waypoints")
        if loops <= 0:
            raise ValueError("Patrol loops must be > 0")
        if not isinstance(home, dict):
            raise ValueError("Patrol home is required")

        normalized_waypoints = []
        for index, waypoint in enumerate(waypoints):
            if not isinstance(waypoint, dict):
                raise ValueError(f"Waypoint {index} must be an object")
            normalized_waypoints.append({
                "x": self._safe_float(waypoint.get("x"), f"waypoints[{index}].x"),
                "y": self._safe_float(waypoint.get("y"), f"waypoints[{index}].y"),
                "yaw": self._safe_float(waypoint.get("yaw", 0.0), f"waypoints[{index}].yaw"),
            })

        normalized_home = {
            "x": self._safe_float(home.get("x"), "home.x"),
            "y": self._safe_float(home.get("y"), "home.y"),
            "yaw": self._safe_float(home.get("yaw", 0.0), "home.yaw"),
        }
        waypoint_tolerance = self._clamp(
            self._safe_float(payload.get("waypoint_tolerance", 0.25), "waypoint_tolerance"),
            MIN_PATROL_WAYPOINT_TOLERANCE,
            MAX_PATROL_WAYPOINT_TOLERANCE,
        )

        with self._patrol_lock:
            if self._patrol_status in {"starting", "running", "returning_home"}:
                raise ValueError("Patrol mission already running")
            self._patrol_run_id = payload.get("run_id")
            self._patrol_schedule_id = payload.get("schedule_id")
            self._patrol_route_id = payload.get("route_id")
            self._patrol_waypoints = normalized_waypoints
            self._patrol_home = normalized_home
            self._patrol_total_loops = loops
            self._patrol_current_loop = 0
            self._patrol_current_waypoint_index = -1
            self._patrol_waypoint_tolerance = waypoint_tolerance
            self._patrol_phase = "go_home_start"

        self._set_patrol_state("starting", "Navigating to home before patrol")
        self._publish_goal(normalized_home["x"], normalized_home["y"], normalized_home["yaw"])
        self.get_logger().info(
            f"Started patrol run={self._patrol_run_id} loops={self._patrol_total_loops} waypoints={len(self._patrol_waypoints)}"
        )

    def _stop_patrol(self, status: str, message: str):
        with self._patrol_lock:
            self._patrol_phase = "idle"
            self._patrol_waypoints = []
            self._patrol_total_loops = 0
            self._patrol_current_loop = 0
            self._patrol_current_waypoint_index = -1
            self._patrol_goal_sent_at = None
        self._set_patrol_state(status, message)
        self.get_logger().info(f"Patrol status={status}: {message}")

    def _distance_to_current_goal(self) -> float | None:
        with self._patrol_lock:
            goal = dict(self._patrol_last_goal) if self._patrol_last_goal else None
        pose = self.telemetry_data.get("map_pose")
        if not goal or not pose:
            return None
        dx = float(pose.get("x", 0.0)) - float(goal.get("x", 0.0))
        dy = float(pose.get("y", 0.0)) - float(goal.get("y", 0.0))
        return math.sqrt(dx * dx + dy * dy)

    def _advance_patrol_goal(self):
        with self._patrol_lock:
            phase = self._patrol_phase
            total_loops = self._patrol_total_loops
            waypoints = [dict(item) for item in self._patrol_waypoints]
            current_loop = self._patrol_current_loop
            current_waypoint_index = self._patrol_current_waypoint_index
            home = dict(self._patrol_home) if self._patrol_home else None

        if phase == "go_home_start":
            if not waypoints:
                self._stop_patrol("failed", "No patrol waypoints configured")
                return
            with self._patrol_lock:
                self._patrol_phase = "patrol"
                self._patrol_current_loop = 1
                self._patrol_current_waypoint_index = 0
            waypoint = waypoints[0]
            self._set_patrol_state("running", f"Loop 1/{total_loops}, waypoint 1/{len(waypoints)}")
            self._publish_goal(waypoint["x"], waypoint["y"], waypoint.get("yaw", 0.0))
            return

        if phase == "patrol":
            if current_waypoint_index < len(waypoints) - 1:
                next_index = current_waypoint_index + 1
                with self._patrol_lock:
                    self._patrol_current_waypoint_index = next_index
                waypoint = waypoints[next_index]
                self._set_patrol_state(
                    "running",
                    f"Loop {current_loop}/{total_loops}, waypoint {next_index + 1}/{len(waypoints)}"
                )
                self._publish_goal(waypoint["x"], waypoint["y"], waypoint.get("yaw", 0.0))
                return

            if current_loop < total_loops:
                next_loop = current_loop + 1
                with self._patrol_lock:
                    self._patrol_current_loop = next_loop
                    self._patrol_current_waypoint_index = 0
                waypoint = waypoints[0]
                self._set_patrol_state(
                    "running",
                    f"Loop {next_loop}/{total_loops}, waypoint 1/{len(waypoints)}"
                )
                self._publish_goal(waypoint["x"], waypoint["y"], waypoint.get("yaw", 0.0))
                return

            if home is None:
                self._stop_patrol("failed", "Home position is missing during return phase")
                return
            with self._patrol_lock:
                self._patrol_phase = "return_home_end"
            self._set_patrol_state("returning_home", "All loops complete, returning home")
            self._publish_goal(home["x"], home["y"], home["yaw"])
            return

        if phase == "return_home_end":
            self._stop_patrol("completed", "Patrol completed and returned home")

    def patrol_tick(self):
        if not self._is_patrol_active():
            return

        with self._patrol_lock:
            waypoint_tolerance = self._patrol_waypoint_tolerance
            goal_sent_at = self._patrol_goal_sent_at

        dist = self._distance_to_current_goal()
        if dist is not None and dist <= waypoint_tolerance:
            self._advance_patrol_goal()
            return

        if goal_sent_at is not None:
            now_sec = self.get_clock().now().nanoseconds / 1e9
            if now_sec - goal_sent_at > self._patrol_waypoint_timeout_sec:
                self._stop_patrol("failed", "Timeout while waiting to reach patrol goal")

    def imu_cb(self, msg):
        self.telemetry_data["imu"] = {
            "ax": msg.linear_acceleration.x,
            "ay": msg.linear_acceleration.y,
            "az": msg.linear_acceleration.z
        }

    def voltage_cb(self, msg):
        self.telemetry_data["voltage"] = msg.data

    def charging_cb(self, msg):
        self.telemetry_data["charging"] = msg.data

    def plan_cb(self, msg):
        poses = msg.poses
        step = max(1, len(poses) // 100)
        self.telemetry_data["plan"] = [
            {"x": p.pose.position.x, "y": p.pose.position.y}
            for p in poses[::step]
        ]

    def local_plan_cb(self, msg):
        self.telemetry_data["local_plan"] = [
            {"x": p.pose.position.x, "y": p.pose.position.y}
            for p in msg.poses
        ]

    def map_cb(self, msg):
        self._last_map_msg = msg
        self.get_logger().info(f'Received /map: {msg.info.width}x{msg.info.height}, res={msg.info.resolution}')

        w = msg.info.width
        h = msg.info.height

        self.telemetry_data["map_info"] = {
            "resolution": msg.info.resolution,
            "width": w,
            "height": h,
            "origin": {
                "x": msg.info.origin.position.x,
                "y": msg.info.origin.position.y
            }
        }

        try:
            png_bytes = self._occupancy_grid_to_png(msg)
            self.camera_pub.send(b'MAP:' + png_bytes)
            self.get_logger().info(f'Sent map PNG: {len(png_bytes)} bytes')
        except Exception as e:
            self.get_logger().error(f'Map PNG encode error: {e}')

        self.map_data = None
        self.map_dirty = False

    def update_map_pose(self):
        try:
            t = self.tf_buffer.lookup_transform('map', 'base_link', rclpy.time.Time())
            q = t.transform.rotation
            yaw = self._yaw_from_quat(q)
            self.telemetry_data["map_pose"] = {
                "x": t.transform.translation.x,
                "y": t.transform.translation.y,
                "yaw": yaw
            }
        except (tf2_ros.LookupException, tf2_ros.ConnectivityException, tf2_ros.ExtrapolationException):
            pass

    def camera_cb(self, msg):
        try:
            channels = 3 if '8' in msg.encoding else 1
            if msg.encoding in ['rgb8', 'bgr8', 'mono8']:
                image_np = np.array(msg.data, dtype=np.uint8).reshape((msg.height, msg.width, channels))

                if msg.encoding == 'rgb8':
                    image_np = cv2.cvtColor(image_np, cv2.COLOR_RGB2BGR)

                encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), 65]
                result, encimg = cv2.imencode('.jpg', image_np, encode_param)

                if result:
                    self.camera_pub.send(encimg.tobytes())
        except Exception as e:
            self.get_logger().error(f"Image encode error: {e}")

    def publish_telemetry(self):
        self.telemetry_pub.send_json(self.telemetry_data)

    def publish_map(self):
        if self.map_data and self.map_dirty:
            self.telemetry_pub.send_json({"type": "map", "payload": self.map_data})
            self.map_dirty = False

    def destroy_node(self):
        for name in list(self._managed_processes.keys()):
            try:
                self._stop_managed_process(name)
            except Exception as exc:
                self.get_logger().warning(f"Failed to stop managed process {name}: {exc}")
        return super().destroy_node()

    def cmd_loop(self):
        while rclpy.ok():
            try:
                msg = self.cmd_rep.recv_json()
                action = msg.get("action")
                payload = msg.get("payload", {})

                if action == "cmd_vel":
                    twist = Twist()
                    twist.linear.x = self._clamp(
                        self._safe_float(payload.get("linear_x", 0.0), "linear_x"),
                        -MAX_LINEAR_VELOCITY,
                        MAX_LINEAR_VELOCITY,
                    )
                    twist.linear.y = self._clamp(
                        self._safe_float(payload.get("linear_y", 0.0), "linear_y"),
                        -MAX_LINEAR_VELOCITY,
                        MAX_LINEAR_VELOCITY,
                    )
                    twist.angular.z = self._clamp(
                        self._safe_float(payload.get("angular_z", 0.0), "angular_z"),
                        -MAX_ANGULAR_VELOCITY,
                        MAX_ANGULAR_VELOCITY,
                    )
                    self.cmd_vel_pub.publish(twist)
                    self.cmd_rep.send_json({"status": "ok"})

                elif action == "nav_goal":
                    if self._is_patrol_active():
                        self._stop_patrol("aborted", "Interrupted by manual nav goal")
                    goal = PoseStamped()
                    goal.header.frame_id = "map"
                    goal.header.stamp = self.get_clock().now().to_msg()
                    goal.pose.position.x = self._safe_float(payload.get("x", 0.0), "x")
                    goal.pose.position.y = self._safe_float(payload.get("y", 0.0), "y")
                    yaw = self._safe_float(payload.get("theta", payload.get("yaw", 0.0)), "yaw")
                    qz, qw = self._quat_from_yaw(yaw)
                    goal.pose.orientation.z = qz
                    goal.pose.orientation.w = qw
                    self.goal_pub.publish(goal)
                    self.get_logger().info(f"Published nav goal to x={goal.pose.position.x}, y={goal.pose.position.y}, yaw={yaw}")
                    self.cmd_rep.send_json({"status": "goal_sent"})

                elif action == "patrol_start":
                    self._start_patrol(payload)
                    with self._patrol_lock:
                        run_id = self._patrol_run_id
                    self.cmd_rep.send_json({"status": "started", "run_id": run_id})

                elif action == "patrol_stop":
                    reason = str(payload.get("reason") or "Patrol stopped by command")
                    if self._is_patrol_active():
                        self._stop_patrol("stopped", reason)
                    with self._patrol_lock:
                        run_id = self._patrol_run_id
                    self.cmd_rep.send_json({"status": "stopped", "run_id": run_id})

                elif action == "generate_coverage":
                    from rai_scada_bridge.coverage_planner import CoveragePlanner

                    robot_width = float(payload.get("robot_width", 0.5))
                    overlap = float(payload.get("overlap", 0.1))
                    pattern = str(payload.get("pattern", "boustrophedon"))

                    if self._last_map_msg is None:
                        self.cmd_rep.send_json({"status": "error", "message": "No map available"})
                    else:
                        try:
                            planner = CoveragePlanner(robot_width=robot_width, overlap=overlap)
                            waypoints = planner.generate_coverage_waypoints(self._last_map_msg, pattern=pattern)
                            is_valid, error_msg = planner.validate_waypoints(waypoints, self._last_map_msg)

                            if not is_valid:
                                self.cmd_rep.send_json({"status": "error", "message": error_msg})
                            else:
                                self.cmd_rep.send_json({
                                    "status": "ok",
                                    "waypoints": waypoints,
                                    "count": len(waypoints)
                                })
                        except Exception as e:
                            self.get_logger().error(f"Coverage generation error: {e}")
                            self.cmd_rep.send_json({"status": "error", "message": str(e)})

                elif action == "resend_map":
                    if self._last_map_msg is not None:
                        self.map_cb(self._last_map_msg)
                        self.cmd_rep.send_json({"status": "map_resent"})
                    else:
                        self.cmd_rep.send_json({"status": "no_map_available"})

                elif action == "list_nodes":
                    self.cmd_rep.send_json(self._list_active_nodes())

                elif action == "get_logs":
                    self.cmd_rep.send_json(self._collect_log_entries(payload))

                elif action == "navigation_mode":
                    mode = str(payload.get("mode", "")).lower()
                    map_path = payload.get("map_path") or payload.get("source")
                    self.cmd_rep.send_json(self._switch_navigation_mode(mode, map_path))

                elif action == "slam_control":
                    slam_action = str(payload.get("action", "")).lower()
                    if slam_action == "start":
                        self.cmd_rep.send_json(self._switch_navigation_mode("slam"))
                    elif slam_action == "stop":
                        result = self._stop_managed_process("slam")
                        self.telemetry_data["navigation_mode"] = "idle"
                        self.cmd_rep.send_json({"status": "ok", "slam": result})
                    elif slam_action == "save":
                        self.cmd_rep.send_json(self._save_current_map(payload))
                    elif slam_action == "reset":
                        self._stop_managed_process("slam")
                        time.sleep(0.5)
                        self.cmd_rep.send_json(self._switch_navigation_mode("slam"))
                    else:
                        self.cmd_rep.send_json({"status": "error", "message": "Unknown slam_control action"})

                elif action == "nav2_control":
                    self.cmd_rep.send_json(self._control_nav2(payload))
                else:
                    self.cmd_rep.send_json({"status": "unknown_action"})
            except Exception as e:
                self.get_logger().error(f"ZMQ cmd error: {e}")
                try:
                    self.cmd_rep.send_json({"status": "error", "message": str(e)})
                except Exception:
                    pass
