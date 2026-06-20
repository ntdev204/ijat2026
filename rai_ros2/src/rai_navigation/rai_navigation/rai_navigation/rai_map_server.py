#!/usr/bin/env python3
"""RAI static map server without Nav2 dependencies."""

import math
import os
from pathlib import Path

import rclpy
import yaml
from nav_msgs.msg import OccupancyGrid
from PIL import Image
from rclpy.node import Node
from rclpy.qos import DurabilityPolicy, QoSProfile, ReliabilityPolicy


class RaiMapServer(Node):
    """Load a ROS map YAML and publish it as a latched OccupancyGrid."""

    def __init__(self):
        super().__init__("rai_map_server")
        yaml_path = self.declare_parameter("yaml_filename", "").value
        frame_id = self.declare_parameter("frame_id", "map").value
        topic_name = self.declare_parameter("topic_name", "/map").value

        qos = QoSProfile(
            depth=1,
            reliability=ReliabilityPolicy.RELIABLE,
            durability=DurabilityPolicy.TRANSIENT_LOCAL,
        )
        self.map_pub = self.create_publisher(OccupancyGrid, topic_name, qos)

        if not yaml_path:
            self.get_logger().warn("yaml_filename is empty; static /map will not be published.")
            return

        try:
            msg = self._load_map(Path(yaml_path).expanduser(), frame_id)
        except Exception as exc:
            self.get_logger().error(f"failed to load map '{yaml_path}': {exc}")
            return

        self.map_pub.publish(msg)
        self.get_logger().info(
            f"published {topic_name}: {msg.info.width}x{msg.info.height}, "
            f"resolution={msg.info.resolution:.4f}, frame={frame_id}"
        )

    def _load_map(self, yaml_path: Path, frame_id: str) -> OccupancyGrid:
        if not yaml_path.is_file():
            raise FileNotFoundError(yaml_path)

        with yaml_path.open("r", encoding="utf-8") as handle:
            meta = yaml.safe_load(handle) or {}

        image_path = Path(meta["image"])
        if not image_path.is_absolute():
            image_path = yaml_path.parent / image_path

        resolution = float(meta["resolution"])
        origin = meta.get("origin", [0.0, 0.0, 0.0])
        negate = int(meta.get("negate", 0))
        occupied_thresh = float(meta.get("occupied_thresh", 0.65))
        free_thresh = float(meta.get("free_thresh", 0.196))

        image = Image.open(os.fspath(image_path)).convert("L")
        width, height = image.size
        pixels = list(image.getdata())

        values: list[int] = []
        for pixel in pixels:
            if negate:
                pixel = 255 - pixel
            occupancy = 1.0 - float(pixel) / 255.0
            if occupancy > occupied_thresh:
                values.append(100)
            elif occupancy < free_thresh:
                values.append(0)
            else:
                values.append(-1)

        rows = [values[i * width:(i + 1) * width] for i in range(height)]
        rows.reverse()

        msg = OccupancyGrid()
        msg.header.frame_id = frame_id
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.info.resolution = resolution
        msg.info.width = width
        msg.info.height = height
        msg.info.origin.position.x = float(origin[0])
        msg.info.origin.position.y = float(origin[1])
        yaw = float(origin[2])
        msg.info.origin.orientation.z = math.sin(yaw * 0.5)
        msg.info.origin.orientation.w = math.cos(yaw * 0.5)
        msg.data = [cell for row in rows for cell in row]
        return msg


def main(args=None):
    rclpy.init(args=args)
    node = RaiMapServer()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
