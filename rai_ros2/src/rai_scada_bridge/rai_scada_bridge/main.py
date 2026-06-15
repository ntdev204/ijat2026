import rclpy
from rai_scada_bridge.node import RaiControlNode


def main(args=None):
    rclpy.init(args=args)

    zmq_ports = {
        "cmd": 5555,
        "telemetry": 5556,
        "camera": 5557
    }

    node = RaiControlNode(zmq_ports)
    node.get_logger().info('SCADA ZMQ Bridge started (standalone ROS2 package)')

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
