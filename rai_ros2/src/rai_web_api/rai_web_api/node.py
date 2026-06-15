import json
import math
import threading
import time
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy
from rclpy.action import ActionClient

from geometry_msgs.msg import Twist, PoseStamped
from sensor_msgs.msg import LaserScan, Image
from nav_msgs.msg import Odometry
from std_msgs.msg import Float32, Float32MultiArray, String

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
            "context": {
                "legacy_context": "OZ",
                "phi_h": 0.0,
                "d_h": None,
                "d_safe": 0.5,
                "vx_max": 0.8,
                "vy_max": 0.5,
                "omega_max": 1.0,
                "occlusion_flag": False,
            },
            "humans": [],
            "solver": {},
            "lidar_clearance": {"left": 5.0, "right": 5.0},
            "last_update": 0.0
        }

        # Lazy Subscription counters
        self.active_telemetry_clients = 0
        self.active_camera_clients = 0

        # Subscriptions placeholders
        self.scan_sub = None
        self.odom_sub = None
        self.context_sub = None
        self.humans_sub = None
        self.bounds_sub = None
        self.solver_sub = None
        self.camera_sub = None

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

        # Permanent Publishers/Clients
        self.cmd_vel_pub = self.create_publisher(Twist, '/cmd_vel', 10)
        self.voltage_sub = self.create_subscription(
            Float32, '/voltage', self.voltage_callback, self.reliable_qos
        )

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
                Odometry, '/odom', self.odom_callback, self.reliable_qos
            )
        if self.context_sub is None:
            self.context_sub = self.create_subscription(
                String, '/canmpc/context', self.context_callback, self.reliable_qos
            )
        if self.humans_sub is None:
            self.humans_sub = self.create_subscription(
                String, '/canmpc/humans', self.humans_callback, self.reliable_qos
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

    # --- CALLBACKS ---

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

    def context_callback(self, msg: String):
        with self.lock:
            try:
                payload = json.loads(msg.data)
                if isinstance(payload, dict):
                    self.telemetry["context"].update(payload)
                else:
                    self.telemetry["context"]["legacy_context"] = str(payload)
            except Exception:
                self.telemetry["context"]["legacy_context"] = msg.data

    def humans_callback(self, msg: String):
        with self.lock:
            try:
                payload = json.loads(msg.data)
                humans = payload.get("humans", []) if isinstance(payload, dict) else []
                self.telemetry["humans"] = humans if isinstance(humans, list) else []
            except Exception:
                self.telemetry["humans"] = []

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


    # --- ACTIONS & COMMANDS ---

    def publish_cmd_vel(self, vx: float, vy: float, wz: float):
        """Gửi lệnh điều khiển xe (Twist)"""
        msg = Twist()
        msg.linear.x = vx
        msg.linear.y = vy
        msg.angular.z = wz
        self.cmd_vel_pub.publish(msg)

    def send_nav_goal(self, x: float, y: float, yaw: float):
        """Gửi mục tiêu di chuyển Nav2 (NavigateToPose)"""
        if not self.nav_to_pose_client:
            self.get_logger().error("Nav2 action client is not initialized!")
            return False

        if not self.nav_to_pose_client.wait_for_action_server(timeout_sec=2.0):
            self.get_logger().error("Nav2 action server not available!")
            return False

        goal_msg = NavigateToPose.Goal()
        goal_msg.pose.header.frame_id = 'map'
        goal_msg.pose.header.stamp = self.get_clock().now().to_msg()
        goal_msg.pose.pose.position.x = x
        goal_msg.pose.pose.position.y = y

        # Yaw to quaternion
        import math
        cy = math.cos(yaw * 0.5)
        sy = math.sin(yaw * 0.5)
        goal_msg.pose.pose.orientation.w = cy
        goal_msg.pose.pose.orientation.z = sy

        self.get_logger().info(f"Sending Nav2 goal: x={x:.2f}, y={y:.2f}, yaw={yaw:.2f}")
        self.nav_to_pose_client.send_goal_async(goal_msg)
        return True

    def cancel_nav_goal(self):
        """Hủy hành trình di chuyển Nav2 đang chạy"""
        # (Nếu có action handle, thực hiện cancel)
        self.get_logger().info("Cancelling current Nav2 goal...")
        # Ở cấp độ đơn giản, ta có thể publish một twist trống hoặc reset action client
