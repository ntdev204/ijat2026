import json
import math
import threading
import time
import base64
import rclpy
import tf2_ros
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy, DurabilityPolicy

from geometry_msgs.msg import PoseStamped, PoseWithCovarianceStamped, Twist
from sensor_msgs.msg import LaserScan, Image
from nav_msgs.msg import Odometry, OccupancyGrid, Path
from std_msgs.msg import Bool, Empty, Float32, String
from rai_controller.msg import AdaptiveBounds, Context, HumanStates, SolverStats
try:
    from rai_human_perception.msg import ContextInput
    _HAS_PERCEPTION = True
except ImportError:
    ContextInput = None
    _HAS_PERCEPTION = False

class WebBridgeNode(Node):
    """
    ROS2 Node trung gian giao tiếp thread-safe với FastAPI.
    Hỗ trợ cơ chế Lazy Subscription để tiết kiệm CPU cho robot.
    """

    def __init__(self):
        super().__init__('web_bridge_node')
        self.lock = threading.Lock()
        self.cmd_vel_topic = self.declare_parameter("cmd_vel_topic", "/cmd_vel_web").value
        self.map_topic = self.declare_parameter("map_topic", "/map").value
        self.control_frame = self.declare_parameter("control_frame", "base_footprint").value
        self.local_path_topic = self.declare_parameter("local_path_topic", "/canmpc/predicted_trajectory").value
        self.odom_topics = ["/odom_combined", "/odom"]
        self.context_input_topics = ["/human_perception/context_input", "/canmpc/context"]

        self.telemetry = {
            "odom": {
                "x": 0.0,
                "y": 0.0,
                "theta": 0.0,
                "linear_x": 0.0,
                "linear_y": 0.0,
                "angular_z": 0.0,
            },
            "battery": {"voltage": 0.0, "percentage": 0.0},
            "charging": False,
            "context": {
                "legacy_context": "OZ",
                "phi_h": 0.0,
                "d_h": None,
                "d_safe": 0.5,
                "vx_max": 0.45,
                "vy_max": 0.35,
                "omega_max": 1.0,
                "occlusion_flag": False,
                "navigation_mode": "idle",
                "human_count": 0,
                "tracking_quality": 0.0,
            },
            "humans": [],
            "solver": {},
            "lidar_clearance": {"left": 5.0, "right": 5.0},
            "map_pose": {"x": 0.0, "y": 0.0, "yaw": 0.0},
            "map_info": None,
            "last_update": 0.0
        }

        
        self.active_telemetry_clients = 0
        self.active_camera_clients = 0
        self.active_map_clients = 0
        self.active_paths_clients = 0

        
        self.scan_sub = None
        self.odom_subs = []
        self.context_sub = None
        self.humans_sub = None
        self.bounds_sub = None
        self.solver_sub = None
        self.context_input_subs = []
        self.camera_sub = None
        self.map_sub = None
        self.plan_sub = None
        self.local_plan_sub = None

        
        self.latest_map = None
        self.latest_global_plan = []
        self.latest_local_plan = []

        
        self.sensor_qos = QoSProfile(
            reliability=ReliabilityPolicy.BEST_EFFORT,
            history=HistoryPolicy.KEEP_LAST,
            depth=5
        )
        self.reliable_qos = QoSProfile(
            reliability=ReliabilityPolicy.RELIABLE,
            history=HistoryPolicy.KEEP_LAST,
            depth=5
        )
        self.map_qos = QoSProfile(
            reliability=ReliabilityPolicy.RELIABLE,
            durability=DurabilityPolicy.TRANSIENT_LOCAL,
            history=HistoryPolicy.KEEP_LAST,
            depth=1
        )

        
        self.cmd_vel_pub = self.create_publisher(Twist, self.cmd_vel_topic, 10)
        self.cca_goal_pub = self.create_publisher(PoseStamped, '/goal_pose', 10)
        self.rai_cancel_pub = self.create_publisher(Empty, '/rai_navigation/cancel_topic', 10)
        self.initial_pose_pub = self.create_publisher(PoseWithCovarianceStamped, '/initialpose', 10)
        self.voltage_sub = self.create_subscription(
            Float32, '/PowerVoltage', self.voltage_callback, self.reliable_qos
        )
        self.charging_sub = self.create_subscription(
            Bool, '/robot_charging_flag', self.charging_callback, self.reliable_qos
        )

        self.tf_buffer = tf2_ros.Buffer()
        self.tf_listener = tf2_ros.TransformListener(self.tf_buffer, self)

        
        self.latest_camera_frame = None
        self.camera_frame_event = threading.Event()
        self.current_nav_goal_handle = None
        self.pending_nav_goal = None
        self.initial_pose = None
        self.home_pose = None
        self.map_pose_available = False
        self.create_timer(0.1, self.update_map_pose)

        self.get_logger().info("WebBridgeNode initialized.")

    

    def ensure_telemetry_subscriptions(self):
        """Ensure realtime telemetry topics are subscribed for REST polling."""
        with self.lock:
            was_zero = self.active_telemetry_clients == 0
            self.active_telemetry_clients += 1
            if was_zero:
                self._activate_telemetry_subscriptions()

    def register_telemetry_client(self):
        """Kích hoạt subscriptions telemetry khi có client kết nối"""
        with self.lock:
            self.active_telemetry_clients += 1
            if self.active_telemetry_clients == 1:
                self._activate_telemetry_subscriptions()

    def _activate_telemetry_subscriptions(self):
        self.get_logger().info("Activating realtime telemetry subscriptions...")
        if self.scan_sub is None:
            self.scan_sub = self.create_subscription(
                LaserScan, '/scan_filtered', self.scan_callback, self.sensor_qos
            )
        if not self.odom_subs:
            for topic_name in self.odom_topics:
                self.odom_subs.append(
                    self.create_subscription(
                        Odometry, topic_name, self.odom_callback, self.reliable_qos
                    )
                )
        if self.context_sub is None:
            self.context_sub = self.create_subscription(
                Context, '/canmpc/context', self.context_callback, self.reliable_qos
            )
        if self.humans_sub is None:
            self.humans_sub = self.create_subscription(
                HumanStates, '/canmpc/humans', self.humans_callback, self.reliable_qos
            )
        if self.bounds_sub is None:
            self.bounds_sub = self.create_subscription(
                AdaptiveBounds, '/canmpc/adaptive_bounds', self.bounds_callback, self.reliable_qos
            )
        if self.solver_sub is None:
            self.solver_sub = self.create_subscription(
                SolverStats, '/canmpc/solver_stats', self.solver_callback, self.reliable_qos
            )
        if not self.context_input_subs and _HAS_PERCEPTION:
            for topic_name in self.context_input_topics:
                self.context_input_subs.append(
                    self.create_subscription(
                        ContextInput, topic_name, self.context_input_callback, self.reliable_qos
                    )
                )

    def unregister_telemetry_client(self):
        """Hủy subscriptions telemetry khi không còn client nào để tiết kiệm CPU"""
        with self.lock:
            self.active_telemetry_clients = max(0, self.active_telemetry_clients - 1)
            if self.active_telemetry_clients == 0:
                self.get_logger().info("💤 No telemetry clients left. Deactivating subscriptions...")
                if self.scan_sub:
                    self.destroy_subscription(self.scan_sub)
                    self.scan_sub = None
                for sub in self.odom_subs:
                    self.destroy_subscription(sub)
                self.odom_subs = []
                if self.context_sub:
                    self.destroy_subscription(self.context_sub)
                    self.context_sub = None
                if self.humans_sub:
                    self.destroy_subscription(self.humans_sub)
                    self.humans_sub = None
                if self.bounds_sub:
                    self.destroy_subscription(self.bounds_sub)
                    self.bounds_sub = None
                if self.solver_sub:
                    self.destroy_subscription(self.solver_sub)
                    self.solver_sub = None
                for sub in self.context_input_subs:
                    self.destroy_subscription(sub)
                self.context_input_subs = []

    def register_camera_client(self):
        """Kích hoạt subscription camera (topic nặng nhất) khi có client WebRTC xem stream"""
        with self.lock:
            self.active_camera_clients += 1
            if self.active_camera_clients == 1:
                self.get_logger().info("📹 WebRTC camera client connected. Subscribing to /camera/color/image_raw...")
                self.camera_sub = self.create_subscription(
                    Image, '/camera/color/image_raw', self.camera_callback, self.sensor_qos
                )

    def unregister_camera_client(self):
        """Hủy subscribe camera khi không có ai xem WebRTC stream (tiết kiệm CPU cực lớn)"""
        with self.lock:
            self.active_camera_clients = max(0, self.active_camera_clients - 1)
            if self.active_camera_clients == 0:
                self.get_logger().info("💤 No camera clients. Unsubscribing from camera topic...")
                if self.camera_sub:
                    self.destroy_subscription(self.camera_sub)
                    self.camera_sub = None
                self.latest_camera_frame = None

    def register_map_client(self):
        """Kích hoạt subscription map khi có client kết nối websocket"""
        with self.lock:
            self.active_map_clients += 1
            if self.active_map_clients == 1 and self.map_sub is None:
                self._activate_map_subscription()

    def unregister_map_client(self):
        """Hủy subscription map khi không còn client nào"""
        with self.lock:
            self.active_map_clients = max(0, self.active_map_clients - 1)
            if self.active_map_clients == 0:
                self.get_logger().info("💤 No map clients. Unsubscribing from map topic...")
                if self.map_sub:
                    self.destroy_subscription(self.map_sub)
                    self.map_sub = None

    def ensure_map_subscription(self):
        """Đảm bảo topic /map đã được subscribe (dùng khi lưu map qua HTTP endpoint)"""
        with self.lock:
            if self.map_sub is None:
                self._activate_map_subscription()

    def _activate_map_subscription(self):
        self.get_logger().info(f"🗺️ Subscribing to map topic {self.map_topic}...")
        self.map_sub = self.create_subscription(
            OccupancyGrid, self.map_topic, self.map_callback, self.map_qos
        )

    def register_paths_client(self):
        """Kích hoạt subscription global/local paths khi client kết nối websocket"""
        with self.lock:
            self.active_paths_clients += 1
            if self.active_paths_clients == 1:
                self._activate_path_subscriptions()

    def _activate_path_subscriptions(self):
        self.get_logger().info(
            f"🗺️ Subscribing to path topics global={self.global_path_topic}, local={self.local_path_topic}..."
        )
        self.plan_sub = self.create_subscription(
            Path, self.global_path_topic, self.global_plan_callback, self.reliable_qos
        )
        self.local_plan_sub = self.create_subscription(
            Path, self.local_path_topic, self.local_plan_callback, self.reliable_qos
        )

    def unregister_paths_client(self):
        """Hủy subscription paths khi không còn client nào"""
        with self.lock:
            self.active_paths_clients = max(0, self.active_paths_clients - 1)
            if self.active_paths_clients == 0:
                self.get_logger().info("💤 No path clients. Unsubscribing from path topics...")
                if self.plan_sub:
                    self.destroy_subscription(self.plan_sub)
                    self.plan_sub = None
                if self.local_plan_sub:
                    self.destroy_subscription(self.local_plan_sub)
                    self.local_plan_sub = None

    def get_visual_topics(self):
        with self.lock:
            return {
                "map_topic": self.map_topic,
                "global_path_topic": self.global_path_topic,
                "local_path_topic": self.local_path_topic,
            }

    def configure_visual_topics(self, map_topic: str, global_path_topic: str, local_path_topic: str):
        with self.lock:
            next_map_topic = map_topic.strip()
            next_global_topic = global_path_topic.strip()
            next_local_topic = local_path_topic.strip()
            if not next_map_topic or not next_global_topic or not next_local_topic:
                raise ValueError("Visual topics cannot be empty")

            map_changed = next_map_topic != self.map_topic
            paths_changed = next_global_topic != self.global_path_topic or next_local_topic != self.local_path_topic
            self.map_topic = next_map_topic
            self.global_path_topic = next_global_topic
            self.local_path_topic = next_local_topic

            if map_changed and self.map_sub:
                self.destroy_subscription(self.map_sub)
                self.map_sub = None
                self.latest_map = None
                if self.active_map_clients > 0:
                    self._activate_map_subscription()

            if paths_changed:
                if self.plan_sub:
                    self.destroy_subscription(self.plan_sub)
                    self.plan_sub = None
                if self.local_plan_sub:
                    self.destroy_subscription(self.local_plan_sub)
                    self.local_plan_sub = None
                self.latest_global_plan = []
                self.latest_local_plan = []
                if self.active_paths_clients > 0:
                    self._activate_path_subscriptions()

            return {
                "map_topic": self.map_topic,
                "global_path_topic": self.global_path_topic,
                "local_path_topic": self.local_path_topic,
            }

    

    def map_callback(self, msg: OccupancyGrid):
        """Callback nhận dữ liệu map (OccupancyGrid) và mã hóa base64 dữ liệu grid để lưu trữ/gửi đi"""
        try:
            data_bytes = bytes([x & 0xFF for x in msg.data])
            grid_base64 = base64.b64encode(data_bytes).decode('utf-8')
            
            with self.lock:
                self.latest_map = {
                    "width": msg.info.width,
                    "height": msg.info.height,
                    "resolution": msg.info.resolution,
                    "origin_x": msg.info.origin.position.x,
                    "origin_y": msg.info.origin.position.y,
                    "grid_data": grid_base64,
                    "timestamp": time.time()
                }
                self.telemetry["map_info"] = {
                    "resolution": msg.info.resolution,
                    "width": msg.info.width,
                    "height": msg.info.height,
                    "origin": {
                        "x": msg.info.origin.position.x,
                        "y": msg.info.origin.position.y,
                    },
                }
        except Exception as e:
            self.get_logger().error(f"Error in map_callback: {e}")

    def global_plan_callback(self, msg: Path):
        """Callback nhận đường đi toàn cục (global plan)"""
        try:
            poses = [{"x": p.pose.position.x, "y": p.pose.position.y} for p in msg.poses]
            with self.lock:
                self.latest_global_plan = poses
        except Exception as e:
            self.get_logger().error(f"Error in global_plan_callback: {e}")

    def local_plan_callback(self, msg: Path):
        """Callback nhận đường đi cục bộ (local plan)"""
        try:
            poses = [{"x": p.pose.position.x, "y": p.pose.position.y} for p in msg.poses]
            with self.lock:
                self.latest_local_plan = poses
        except Exception as e:
            self.get_logger().error(f"Error in local_plan_callback: {e}")

    def voltage_callback(self, msg: Float32):
        """Callback đọc pin và tính % pin trực tiếp"""
        with self.lock:
            v = msg.data
            self.telemetry["battery"]["voltage"] = round(v, 2)
            
            if v <= 20.0:
                pct = 0.0
            elif v >= 25.2:
                pct = 100.0
            else:
                pct = (v - 20.0) / 5.2 * 100.0
            self.telemetry["battery"]["percentage"] = round(pct, 1)

    def charging_callback(self, msg: Bool):
        with self.lock:
            self.telemetry["charging"] = bool(msg.data)

    def odom_callback(self, msg: Odometry):
        """Callback nhận odom và tính toán toạ độ Pose"""
        with self.lock:
            
            self.telemetry["odom"]["x"] = round(msg.pose.pose.position.x, 3)
            self.telemetry["odom"]["y"] = round(msg.pose.pose.position.y, 3)
            
            q = msg.pose.pose.orientation
            siny_cosp = 2 * (q.w * q.z + q.x * q.y)
            cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z)
            self.telemetry["odom"]["theta"] = round(math.atan2(siny_cosp, cosy_cosp), 3)

            
            self.telemetry["odom"]["linear_x"] = round(msg.twist.twist.linear.x, 3)
            self.telemetry["odom"]["linear_y"] = round(msg.twist.twist.linear.y, 3)
            self.telemetry["odom"]["angular_z"] = round(msg.twist.twist.angular.z, 3)
            self.telemetry["last_update"] = time.time()

    def scan_callback(self, msg: LaserScan):
        """Tính khoảng cách hông trái/phải từ LiDAR scan_filtered"""
        ranges = msg.ranges
        num_rays = len(ranges)
        if num_rays == 0:
            return

        
        idx_left = int((1.57 - msg.angle_min) / msg.angle_increment)
        idx_right = int((-1.57 - msg.angle_min) / msg.angle_increment)

        with self.lock:
            if 0 <= idx_left < num_rays and msg.range_min < ranges[idx_left] < msg.range_max:
                self.telemetry["lidar_clearance"]["left"] = round(ranges[idx_left], 2)
            if 0 <= idx_right < num_rays and msg.range_min < ranges[idx_right] < msg.range_max:
                self.telemetry["lidar_clearance"]["right"] = round(ranges[idx_right], 2)

    def context_callback(self, msg: Context):
        with self.lock:
            self.telemetry["context"].update({
                "phi_h": round(float(msg.phi_h), 4),
                "d_h": round(float(msg.nearest_human_dist), 3),
                "d_safe": round(float(msg.d_safe), 3),
                "vx_max": round(float(msg.vx_max), 3),
                "vy_max": round(float(msg.vy_max), 3),
                "omega_max": round(float(msg.omega_max), 3),
                "occlusion_flag": bool(msg.occlusion_flag),
                "legacy_context": self._legacy_context_label(float(msg.phi_h)),
            })

    def humans_callback(self, msg: HumanStates):
        with self.lock:
            self.telemetry["humans"] = [
                {
                    "id": int(human.id),
                    "x": round(float(human.x), 3),
                    "y": round(float(human.y), 3),
                    "vx": round(float(human.vx), 3),
                    "vy": round(float(human.vy), 3),
                    "confidence": round(float(human.confidence), 3),
                    "age_sec": round(float(human.age_sec), 3),
                }
                for human in msg.humans
            ]

    @staticmethod
    def _legacy_context_label(phi_h: float) -> str:
        if phi_h > 0.65:
            return "HPZ"
        return "OZ"

    def bounds_callback(self, msg: AdaptiveBounds):
        with self.lock:
            self.telemetry["context"].update({
                "d_safe": round(float(msg.d_safe), 3),
                "vx_max": round(float(msg.vx_max), 3),
                "vy_max": round(float(msg.vy_max), 3),
                "omega_max": round(float(msg.omega_max), 3),
                "q_scale": round(float(msg.q_scale), 3),
            })

    def solver_callback(self, msg: SolverStats):
        with self.lock:
            self.telemetry["solver"] = {
                "solve_time_ms": round(float(msg.solve_time_ms), 3),
                "iter_count": int(msg.iter_count),
                "status": str(msg.status),
                "timeout_flag": bool(msg.timeout_flag),
                "collision_flag": bool(msg.collision_flag),
            }

    def context_input_callback(self, msg: ContextInput):
        with self.lock:
            self.telemetry["context"]["human_count"] = int(msg.human_count)
            self.telemetry["context"]["tracking_quality"] = round(float(msg.tracking_quality), 3)

    def camera_callback(self, msg: Image):
        """Lưu frame ảnh thô mới nhất làm bộ đệm cho WebRTC"""
        with self.lock:
            self.latest_camera_frame = msg
            self.camera_frame_event.set()
        

    def update_map_pose(self):
        try:
            transform = self.tf_buffer.lookup_transform('map', self.control_frame, rclpy.time.Time())
        except (tf2_ros.LookupException, tf2_ros.ConnectivityException, tf2_ros.ExtrapolationException) as exc:
            self.get_logger().debug(f"Map pose TF unavailable: {exc}")
            self._check_pending_goal_timeout()
            return

        q = transform.transform.rotation
        siny_cosp = 2 * (q.w * q.z + q.x * q.y)
        cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z)
        yaw = round(math.atan2(siny_cosp, cosy_cosp), 3)
        self._check_pending_goal_timeout()
        with self.lock:
            self.map_pose_available = True
            self.telemetry["map_pose"] = {
                "x": round(transform.transform.translation.x, 3),
                "y": round(transform.transform.translation.y, 3),
                "yaw": yaw,
            }
            self._maybe_dispatch_pending_rai_goal_locked()

    def _maybe_dispatch_pending_rai_goal_locked(self):
        pending = dict(self.pending_nav_goal) if self.pending_nav_goal else None
        current_pose = dict(self.telemetry.get("map_pose", {}))
        if pending is None or pending.get("mode") != "rai_navigation":
            return
        distance = math.hypot(
            float(pending.get("start_x", 0.0)) - float(current_pose.get("x", 0.0) or 0.0),
            float(pending.get("start_y", 0.0)) - float(current_pose.get("y", 0.0) or 0.0),
        )
        if distance > float(pending.get("start_tolerance", 0.25)):
            return
        self.pending_nav_goal = None
        goal_x = pending["x"]
        goal_y = pending["y"]
        goal_yaw = pending.get("yaw", 0.0)
        self.send_cca_nmpc_goal(goal_x, goal_y, goal_yaw)

    def _check_pending_goal_timeout(self):
        """Clear pending goal if it has been waiting too long (TF unavailable)."""
        with self.lock:
            pending = self.pending_nav_goal
            if pending is not None:
                queued_at = pending.get("queued_at", 0.0)
                if queued_at > 0.0 and (time.time() - queued_at) > 30.0:
                    self.get_logger().warning(
                        f"Pending nav goal timed out after 30s (TF unavailable). Clearing."
                    )
                    self.pending_nav_goal = None

    def _maybe_dispatch_pending_rai_goal(self):
        with self.lock:
            self._maybe_dispatch_pending_rai_goal_locked()

    def publish_cmd_vel(self, vx: float, vy: float, wz: float):
        """Gửi lệnh điều khiển xe (Twist) với dead zone filtering."""
        magnitude = math.sqrt(vx * vx + vy * vy + wz * wz)
        if magnitude < 0.05:
            self.get_logger().debug(f"Dropping cmd_vel below dead zone threshold: ({vx:.3f}, {vy:.3f}, {wz:.3f})")
            return
        msg = Twist()
        msg.linear.x = vx
        msg.linear.y = vy
        msg.angular.z = wz
        self.cmd_vel_pub.publish(msg)

    def capture_current_pose_as_anchor(self, prefer_map: bool = True, set_home: bool = True):
        with self.lock:
            pose = None
            if prefer_map and self.map_pose_available:
                pose = dict(self.telemetry["map_pose"])
            elif self.telemetry["last_update"] > 0.0:
                pose = {
                    "x": self.telemetry["odom"]["x"],
                    "y": self.telemetry["odom"]["y"],
                    "yaw": self.telemetry["odom"]["theta"],
                }

            if pose is None:
                return None

            anchor = {
                "x": round(float(pose["x"]), 3),
                "y": round(float(pose["y"]), 3),
                "yaw": round(float(pose["yaw"]), 3),
            }
            self.initial_pose = dict(anchor)
            if set_home or self.home_pose is None:
                self.home_pose = dict(anchor)
            return anchor

    def get_anchor_state(self):
        with self.lock:
            return {
                "initial_pose": dict(self.initial_pose) if self.initial_pose else None,
                "home_pose": dict(self.home_pose) if self.home_pose else None,
            }

    def send_nav_goal(self, x: float, y: float, yaw: float):
        """Send a goal to RAI Navigation."""
        return self.send_cca_nmpc_goal(x, y, yaw)

    def send_cca_nmpc_goal(self, x: float, y: float, yaw: float):
        """Publish goal for the standalone RAI navigation controller."""
        goal_msg = PoseStamped()
        goal_msg.header.frame_id = 'map'
        goal_msg.header.stamp = self.get_clock().now().to_msg()
        goal_msg.pose.position.x = x
        goal_msg.pose.position.y = y
        qz, qw = self._yaw_to_quaternion(yaw)
        goal_msg.pose.orientation.z = qz
        goal_msg.pose.orientation.w = qw
        self.cca_goal_pub.publish(goal_msg)
        self.get_logger().info(f"Published CCA-NMPC goal: x={x:.2f}, y={y:.2f}, yaw={yaw:.2f}")
        return True

    def send_cca_nmpc_route(self, start_pose: dict, goal_pose: dict, start_tolerance: float = 0.25):
        """Go to start first if needed, then send the final RAI navigation goal."""
        with self.lock:
            current_pose = dict(self.telemetry.get("map_pose", {}))

        current_x = float(current_pose.get("x", 0.0) or 0.0)
        current_y = float(current_pose.get("y", 0.0) or 0.0)
        start_x = float(start_pose["x"])
        start_y = float(start_pose["y"])
        distance_to_start = math.hypot(start_x - current_x, start_y - current_y)

        with self.lock:
            self.pending_nav_goal = None

        if distance_to_start <= start_tolerance:
            return self.send_cca_nmpc_goal(
                float(goal_pose["x"]),
                float(goal_pose["y"]),
                float(goal_pose.get("yaw", 0.0)),
            )

        pending = {
            "x": float(goal_pose["x"]),
            "y": float(goal_pose["y"]),
            "yaw": float(goal_pose.get("yaw", 0.0)),
            "mode": "rai_navigation",
            "start_x": start_x,
            "start_y": start_y,
            "start_tolerance": start_tolerance,
            "queued_at": time.time(),
        }
        with self.lock:
            self.pending_nav_goal = pending
        return self.send_cca_nmpc_goal(start_x, start_y, float(start_pose.get("yaw", 0.0)))

    def send_nav_route(self, start_pose: dict, goal_pose: dict, start_tolerance: float = 0.25):
        """Compatibility wrapper for RAI Navigation route dispatch."""
        return self.send_cca_nmpc_route(start_pose, goal_pose, start_tolerance)

    def send_home_goal(self):
        with self.lock:
            home_pose = dict(self.home_pose) if self.home_pose else None
        if home_pose is None:
            self.get_logger().error("Home pose is not set.")
            return False
        return self.send_cca_nmpc_goal(home_pose["x"], home_pose["y"], home_pose.get("yaw", 0.0))

    def cancel_nav_goal(self):
        """Cancel pending RAI navigation route chaining and the active controller goal."""
        with self.lock:
            self.pending_nav_goal = None
            self.current_nav_goal_handle = None
        self.rai_cancel_pub.publish(Empty())

    def _yaw_to_quaternion(self, yaw: float):
        cy = math.cos(yaw * 0.5)
        sy = math.sin(yaw * 0.5)
        return sy, cy
