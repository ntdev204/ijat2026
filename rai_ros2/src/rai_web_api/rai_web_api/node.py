import json
import math
import threading
import time
import base64
import rclpy
import tf2_ros
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy, DurabilityPolicy
from rclpy.action import ActionClient

from geometry_msgs.msg import PoseStamped, PoseWithCovarianceStamped, Twist
from sensor_msgs.msg import LaserScan, Image
from nav_msgs.msg import Odometry, OccupancyGrid, Path
from std_msgs.msg import Bool, Float32, Float32MultiArray, String
from action_msgs.msg import GoalStatus
from ccanmpc_msgs.msg import Context, HumanStates

try:
    from nav2_msgs.action import NavigateToPose
    NAV2_AVAILABLE = True
except ImportError:
    NAV2_AVAILABLE = False

class WebBridgeNode(Node):
    """
    ROS2 Node trung gian giao tiếp thread-safe với FastAPI.
    Hỗ trợ cơ chế Lazy Subscription để tiết kiệm CPU cho robot.
    """

    def __init__(self):
        super().__init__('web_bridge_node')
        self.lock = threading.Lock()

        # Telemetry State (Thread-safe)
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
                "vx_max": 0.8,
                "vy_max": 0.5,
                "omega_max": 1.0,
                "occlusion_flag": False,
                "navigation_mode": "idle",
            },
            "humans": [],
            "solver": {},
            "lidar_clearance": {"left": 5.0, "right": 5.0},
            "map_pose": {"x": 0.0, "y": 0.0, "yaw": 0.0},
            "map_info": None,
            "last_update": 0.0
        }

        # Lazy Subscription counters
        self.active_telemetry_clients = 0
        self.active_camera_clients = 0
        self.active_map_clients = 0
        self.active_paths_clients = 0

        # Subscriptions placeholders
        self.scan_sub = None
        self.odom_sub = None
        self.context_sub = None
        self.humans_sub = None
        self.bounds_sub = None
        self.solver_sub = None
        self.camera_sub = None
        self.map_sub = None
        self.plan_sub = None
        self.local_plan_sub = None

        # Thread-safe caches for map and paths
        self.latest_map = None
        self.latest_global_plan = []
        self.latest_local_plan = []

        # QoS Profiles
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

        # Permanent Publishers/Clients
        self.cmd_vel_pub = self.create_publisher(Twist, '/cmd_vel', 10)
        self.initial_pose_pub = self.create_publisher(PoseWithCovarianceStamped, '/initialpose', 10)
        self.voltage_sub = self.create_subscription(
            Float32, '/PowerVoltage', self.voltage_callback, self.reliable_qos
        )
        self.charging_sub = self.create_subscription(
            Bool, '/robot_charging_flag', self.charging_callback, self.reliable_qos
        )

        self.tf_buffer = tf2_ros.Buffer()
        self.tf_listener = tf2_ros.TransformListener(self.tf_buffer, self)

        # Nav2 Action Client
        if NAV2_AVAILABLE:
            self.nav_to_pose_client = ActionClient(self, NavigateToPose, 'navigate_to_pose')
            self.get_logger().info("Nav2 Action Client initialized successfully.")
        else:
            self.nav_to_pose_client = None
            self.get_logger().warn("Nav2 Action Client is NOT available (nav2_msgs missing).")

        # Camera frame cache (cho WebRTC)
        self.latest_camera_frame = None
        self.camera_frame_event = threading.Event()
        self.current_nav_goal_handle = None
        self.pending_nav_goal = None
        self.initial_pose = None
        self.home_pose = None
        self.map_pose_available = False
        self.create_timer(0.1, self.update_map_pose)

        self.get_logger().info("WebBridgeNode initialized.")

    # --- LAZY SUBSCRIPTION MANAGEMENT ---

    def ensure_telemetry_subscriptions(self):
        """Ensure realtime telemetry topics are subscribed for REST polling."""
        with self.lock:
            if self.active_telemetry_clients == 0:
                self.active_telemetry_clients = 1
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
        if self.odom_sub is None:
            self.odom_sub = self.create_subscription(
                Odometry, '/odom_combined', self.odom_callback, self.reliable_qos
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
                Float32MultiArray, '/canmpc/adaptive_bounds', self.bounds_callback, self.reliable_qos
            )
        if self.solver_sub is None:
            self.solver_sub = self.create_subscription(
                String, '/canmpc/solver_stats', self.solver_callback, self.reliable_qos
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
                if self.odom_sub:
                    self.destroy_subscription(self.odom_sub)
                    self.odom_sub = None
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
            if self.active_map_clients == 1:
                self.get_logger().info("🗺️ Map client connected. Subscribing to /map...")
                self.map_sub = self.create_subscription(
                    OccupancyGrid, '/map', self.map_callback, self.map_qos
                )

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
                self.get_logger().info("🗺️ Ensuring subscription to /map for saving...")
                self.map_sub = self.create_subscription(
                    OccupancyGrid, '/map', self.map_callback, self.map_qos
                )

    def register_paths_client(self):
        """Kích hoạt subscription global/local paths khi client kết nối websocket"""
        with self.lock:
            self.active_paths_clients += 1
            if self.active_paths_clients == 1:
                self.get_logger().info("🗺️ Path client connected. Subscribing to /plan and /local_plan...")
                self.plan_sub = self.create_subscription(
                    Path, '/plan', self.global_plan_callback, self.reliable_qos
                )
                self.local_plan_sub = self.create_subscription(
                    Path, '/local_plan', self.local_plan_callback, self.reliable_qos
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

    # --- CALLBACKS ---

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
            # 24V Battery / 6S LiPo calculation (20.0V -> 25.2V)
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
            # Extract position
            self.telemetry["odom"]["x"] = round(msg.pose.pose.position.x, 3)
            self.telemetry["odom"]["y"] = round(msg.pose.pose.position.y, 3)
            # Euler yaw from quaternion
            q = msg.pose.pose.orientation
            siny_cosp = 2 * (q.w * q.z + q.x * q.y)
            cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z)
            self.telemetry["odom"]["theta"] = round(math.atan2(siny_cosp, cosy_cosp), 3)

            # Velocities
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

        # Lấy tia ở góc 90 độ (trái) và -90 độ (phải)
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
                    "x": round(float(human.pose.position.x), 3),
                    "y": round(float(human.pose.position.y), 3),
                    "vx": round(float(human.velocity.linear.x), 3),
                    "vy": round(float(human.velocity.linear.y), 3),
                    "confidence": round(float(human.confidence), 3),
                }
                for human in msg.humans
            ]

    @staticmethod
    def _legacy_context_label(phi_h: float) -> str:
        if phi_h > 0.65:
            return "HPZ"
        return "OZ"

    def bounds_callback(self, msg: Float32MultiArray):
        data = list(msg.data)
        if len(data) < 5:
            return
        with self.lock:
            self.telemetry["context"].update({
                "phi_h": round(float(data[0]), 4),
                "d_safe": round(float(data[1]), 3),
                "vx_max": round(float(data[2]), 3),
                "vy_max": round(float(data[3]), 3),
                "omega_max": round(float(data[4]), 3),
            })

    def solver_callback(self, msg: String):
        with self.lock:
            try:
                payload = json.loads(msg.data)
                self.telemetry["solver"] = payload if isinstance(payload, dict) else {"raw": msg.data}
            except Exception:
                self.telemetry["solver"] = {"raw": msg.data}

    def camera_callback(self, msg: Image):
        """Lưu frame ảnh thô mới nhất làm bộ đệm cho WebRTC"""
        with self.lock:
            self.latest_camera_frame = msg
            self.camera_frame_event.set()
        # No lock needed for event set – it is thread‑safe.

    def update_map_pose(self):
        try:
            transform = self.tf_buffer.lookup_transform('map', 'base_footprint', rclpy.time.Time())
        except (tf2_ros.LookupException, tf2_ros.ConnectivityException, tf2_ros.ExtrapolationException) as exc:
            self.get_logger().debug(f"Map pose TF unavailable: {exc}")
            return

        q = transform.transform.rotation
        siny_cosp = 2 * (q.w * q.z + q.x * q.y)
        cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z)
        yaw = round(math.atan2(siny_cosp, cosy_cosp), 3)
        with self.lock:
            self.map_pose_available = True
            self.telemetry["map_pose"] = {
                "x": round(transform.transform.translation.x, 3),
                "y": round(transform.transform.translation.y, 3),
                "yaw": yaw,
            }


    # --- ACTIONS & COMMANDS ---

    def publish_cmd_vel(self, vx: float, vy: float, wz: float):
        """Gửi lệnh điều khiển xe (Twist)"""
        msg = Twist()
        msg.linear.x = vx
        msg.linear.y = vy
        msg.angular.z = wz
        self.cmd_vel_pub.publish(msg)

    def publish_initial_pose(self, x: float, y: float, yaw: float, set_home: bool = False):
        """Đặt vị trí khởi tạo cho AMCL / Nav2."""
        msg = PoseWithCovarianceStamped()
        msg.header.frame_id = 'map'
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.pose.pose.position.x = x
        msg.pose.pose.position.y = y
        qz, qw = self._yaw_to_quaternion(yaw)
        msg.pose.pose.orientation.z = qz
        msg.pose.pose.orientation.w = qw
        msg.pose.covariance[0] = 0.25
        msg.pose.covariance[7] = 0.25
        msg.pose.covariance[35] = 0.068
        self.initial_pose_pub.publish(msg)
        pose = {"x": round(x, 3), "y": round(y, 3), "yaw": round(yaw, 3)}
        with self.lock:
            self.initial_pose = pose
            if set_home or self.home_pose is None:
                self.home_pose = dict(pose)
        self.get_logger().info(f"Published initial pose: x={x:.2f}, y={y:.2f}, yaw={yaw:.2f}")
        return pose

    def set_home_pose(self, x: float, y: float, yaw: float):
        pose = {"x": round(x, 3), "y": round(y, 3), "yaw": round(yaw, 3)}
        with self.lock:
            self.home_pose = pose
        self.get_logger().info(f"Updated home pose: x={x:.2f}, y={y:.2f}, yaw={yaw:.2f}")
        return pose

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
        """Gửi mục tiêu di chuyển Nav2 (NavigateToPose)."""
        return self._dispatch_nav_goal(x, y, yaw)

    def send_nav_route(self, start_pose: dict, goal_pose: dict, start_tolerance: float = 0.25):
        """Đi đến start trước nếu robot chưa ở start, sau đó mới đi goal."""
        with self.lock:
            current_pose = dict(self.telemetry.get("map_pose", {}))

        current_x = float(current_pose.get("x", 0.0) or 0.0)
        current_y = float(current_pose.get("y", 0.0) or 0.0)
        start_x = float(start_pose["x"])
        start_y = float(start_pose["y"])
        distance_to_start = math.hypot(start_x - current_x, start_y - current_y)
        self.pending_nav_goal = None

        if distance_to_start <= start_tolerance:
            self.get_logger().info("Robot is already near start point. Sending goal directly.")
            return self._dispatch_nav_goal(float(goal_pose["x"]), float(goal_pose["y"]), float(goal_pose.get("yaw", 0.0)))

        self.pending_nav_goal = {
            "x": float(goal_pose["x"]),
            "y": float(goal_pose["y"]),
            "yaw": float(goal_pose.get("yaw", 0.0)),
        }
        self.get_logger().info(
            f"Robot is {distance_to_start:.2f}m from start. Going to start first, then goal."
        )
        return self._dispatch_nav_goal(start_x, start_y, float(start_pose.get("yaw", 0.0)))

    def send_home_goal(self):
        with self.lock:
            home_pose = dict(self.home_pose) if self.home_pose else None
        if home_pose is None:
            self.get_logger().error("Home pose is not set.")
            return False
        return self._dispatch_nav_goal(home_pose["x"], home_pose["y"], home_pose.get("yaw", 0.0))

    def _dispatch_nav_goal(self, x: float, y: float, yaw: float):
        """Gửi goal tới action server và giữ callback để chain goal tiếp theo."""
        if not self.nav_to_pose_client:
            self.get_logger().error("Nav2 action client is not initialized!")
            return False

        if not self._wait_for_nav_server(timeout_sec=2.0):
            self.get_logger().error("Nav2 action server not available!")
            return False

        goal_msg = NavigateToPose.Goal()
        goal_msg.pose.header.frame_id = 'map'
        goal_msg.pose.header.stamp = self.get_clock().now().to_msg()
        goal_msg.pose.pose.position.x = x
        goal_msg.pose.pose.position.y = y

        qz, qw = self._yaw_to_quaternion(yaw)
        goal_msg.pose.pose.orientation.z = qz
        goal_msg.pose.pose.orientation.w = qw

        self.get_logger().info(f"Sending Nav2 goal: x={x:.2f}, y={y:.2f}, yaw={yaw:.2f}")
        send_future = self.nav_to_pose_client.send_goal_async(goal_msg)
        send_future.add_done_callback(self._handle_nav_goal_response)
        return True

    def _wait_for_nav_server(self, timeout_sec: float = 2.0):
        """Tương thích nhiều bản rclpy ActionClient."""
        if self.nav_to_pose_client is None:
            return False

        wait_for_server = getattr(self.nav_to_pose_client, "wait_for_server", None)
        if callable(wait_for_server):
            return bool(wait_for_server(timeout_sec=timeout_sec))

        wait_for_action_server = getattr(self.nav_to_pose_client, "wait_for_action_server", None)
        if callable(wait_for_action_server):
            return bool(wait_for_action_server(timeout_sec=timeout_sec))

        server_is_ready = getattr(self.nav_to_pose_client, "server_is_ready", None)
        if callable(server_is_ready):
            deadline = time.time() + timeout_sec
            while time.time() < deadline:
                if bool(server_is_ready()):
                    return True
                time.sleep(0.1)
        return False

    def cancel_nav_goal(self):
        """Hủy hành trình di chuyển Nav2 đang chạy."""
        self.pending_nav_goal = None
        self.get_logger().info("Cancelling current Nav2 goal...")
        if self.current_nav_goal_handle is not None:
            self.current_nav_goal_handle.cancel_goal_async()
            self.current_nav_goal_handle = None

    def _handle_nav_goal_response(self, future):
        try:
            goal_handle = future.result()
        except Exception as error:
            self.get_logger().error(f"Failed to send Nav2 goal: {error}")
            self.pending_nav_goal = None
            return

        if not goal_handle.accepted:
            self.get_logger().error("Nav2 goal was rejected by action server.")
            self.pending_nav_goal = None
            return

        self.current_nav_goal_handle = goal_handle
        result_future = goal_handle.get_result_async()
        result_future.add_done_callback(self._handle_nav_goal_result)

    def _handle_nav_goal_result(self, future):
        self.current_nav_goal_handle = None
        try:
            result = future.result()
        except Exception as error:
            self.get_logger().error(f"Nav2 goal result failed: {error}")
            self.pending_nav_goal = None
            return

        if result.status == GoalStatus.STATUS_SUCCEEDED:
            self.get_logger().info("Nav2 goal completed successfully.")
            if self.pending_nav_goal is not None:
                next_goal = dict(self.pending_nav_goal)
                self.pending_nav_goal = None
                self.get_logger().info("Dispatching pending goal after reaching start point.")
                self._dispatch_nav_goal(next_goal["x"], next_goal["y"], next_goal.get("yaw", 0.0))
            return

        self.get_logger().warn(f"Nav2 goal finished with status {result.status}.")
        self.pending_nav_goal = None

    def _yaw_to_quaternion(self, yaw: float):
        cy = math.cos(yaw * 0.5)
        sy = math.sin(yaw * 0.5)
        return sy, cy
