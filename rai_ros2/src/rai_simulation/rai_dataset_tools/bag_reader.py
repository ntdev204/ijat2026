import os
import sqlite3
import struct
import math
import json
from pathlib import Path

# Helper function to read a string from CDR binary representation
def read_string(blob, offset):
    # Align to 4-byte boundary
    offset = (offset + 3) & ~3
    if offset + 4 > len(blob):
        return "", offset
    length = struct.unpack('<I', blob[offset:offset+4])[0]
    if offset + 4 + length > len(blob):
        return "", offset + 4
    # CDR string has null terminator at the end
    data = blob[offset+4:offset+4+length-1].decode('utf-8', errors='ignore')
    return data, offset + 4 + length

# Parse geometry_msgs/msg/Twist
def parse_twist(blob):
    if len(blob) < 52: # 4 bytes header + 48 bytes doubles
        return 0.0, 0.0, 0.0
    vx, vy, vz, wx, wy, wz = struct.unpack('<dddddd', blob[4:52])
    return vx, vy, wz

# Parse geometry_msgs/msg/TwistStamped
def parse_twist_stamped(blob):
    if len(blob) < 64:
        return 0, 0.0, 0.0, 0.0
    sec, nanosec = struct.unpack('<iI', blob[4:12])
    frame_id, offset = read_string(blob, 12)
    offset = (offset + 7) & ~7 # Align to double
    if offset + 48 <= len(blob):
        vx, vy, vz, wx, wy, wz = struct.unpack('<dddddd', blob[offset:offset+48])
    else:
        vx, vy, wz = 0.0, 0.0, 0.0
    stamp = sec + nanosec * 1e-9
    return stamp, vx, vy, wz

# Parse std_msgs/msg/String (context and humans are published as JSON strings)
def parse_string(blob):
    data, _ = read_string(blob, 4)
    return data

# Parse std_msgs/msg/Float32MultiArray
def parse_float_array(blob):
    if len(blob) < 16:
        return []
    dim_len = struct.unpack('<I', blob[4:8])[0]
    offset = 8
    # Skip dimensions
    for _ in range(dim_len):
        _, offset = read_string(blob, offset)
        offset = (offset + 3) & ~3
        offset += 8
    offset = (offset + 3) & ~3
    offset += 4
    offset = (offset + 3) & ~3
    if offset + 4 > len(blob):
        return []
    data_len = struct.unpack('<I', blob[offset:offset+4])[0]
    offset += 4
    floats = []
    for _ in range(data_len):
        if offset + 4 <= len(blob):
            floats.append(struct.unpack('<f', blob[offset:offset+4])[0])
            offset += 4
    return floats

# Parse nav_msgs/msg/Odometry
def parse_odom(blob):
    if len(blob) < 400:
        return 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
    
    sec, nanosec = struct.unpack('<iI', blob[4:12])
    stamp = sec + nanosec * 1e-9
    
    frame_id, offset = read_string(blob, 12)
    child_frame_id, offset = read_string(blob, offset)
    
    offset = (offset + 7) & ~7
    x, y, z, qx, qy, qz, qw = struct.unpack('<ddddddd', blob[offset:offset+56])
    
    siny_cosp = 2.0 * (qw * qz + qx * qy)
    cosy_cosp = 1.0 - 2.0 * (qy * qy + qz * qz)
    theta = math.atan2(siny_cosp, cosy_cosp)
    
    offset += 56 + 288
    
    offset = (offset + 7) & ~7
    vx, vy, vz, wx, wy, wz = struct.unpack('<dddddd', blob[offset:offset+48])
    
    return stamp, x, y, theta, vx, vy, wz

# Parse canmpc_msgs/msg/SolverStats
def parse_solver_stats(blob):
    if len(blob) < 30:
        return 0, 0.0, 0, "unknown", False, False
    sec, nanosec = struct.unpack('<iI', blob[4:12])
    stamp = sec + nanosec * 1e-9
    frame_id, offset = read_string(blob, 12)
    
    offset = (offset + 7) & ~7
    solve_time_ms = struct.unpack('<d', blob[offset:offset+8])[0]
    offset += 8
    
    offset = (offset + 3) & ~3
    iter_count = struct.unpack('<i', blob[offset:offset+4])[0]
    offset += 4
    
    status, offset = read_string(blob, offset)
    
    timeout_flag = bool(blob[offset]) if offset < len(blob) else False
    collision_flag = bool(blob[offset+1]) if offset+1 < len(blob) else False
    
    return stamp, solve_time_ms, iter_count, status, timeout_flag, collision_flag

# Parse nav_msgs/msg/Path
def parse_path(blob):
    if len(blob) < 20:
        return []
    sec, nanosec = struct.unpack('<iI', blob[4:12])
    frame_id, offset = read_string(blob, 12)
    
    offset = (offset + 3) & ~3
    poses_len = struct.unpack('<I', blob[offset:offset+4])[0]
    offset += 4
    
    waypoints = []
    for _ in range(poses_len):
        offset = (offset + 3) & ~3
        p_sec, p_nanosec = struct.unpack('<iI', blob[offset:offset+8])
        p_frame_id, offset = read_string(blob, offset+8)
        
        offset = (offset + 7) & ~7
        px, py, pz, pqx, pqy, pqz, pqw = struct.unpack('<ddddddd', blob[offset:offset+56])
        offset += 56
        
        siny_cosp = 2.0 * (pqw * pqz + pqx * pqy)
        cosy_cosp = 1.0 - 2.0 * (pqy * pqy + pqz * pqz)
        pyaw = math.atan2(siny_cosp, cosy_cosp)
        
        waypoints.append((px, py, pyaw))
        
    return waypoints

# Parse sensor_msgs/msg/LaserScan
def parse_laser_scan(blob):
    if len(blob) < 40:
        return 0.0, []
    sec, nanosec = struct.unpack('<iI', blob[4:12])
    stamp = sec + nanosec * 1e-9
    frame_id, offset = read_string(blob, 12)
    
    offset = (offset + 3) & ~3
    if offset + 28 > len(blob):
        return stamp, []
        
    angle_min, angle_max, angle_incr, time_incr, scan_time, r_min, r_max = struct.unpack(
        '<fffffff', blob[offset:offset+28]
    )
    offset += 28
    
    offset = (offset + 3) & ~3
    if offset + 4 > len(blob):
        return stamp, []
    ranges_len = struct.unpack('<I', blob[offset:offset+4])[0]
    offset += 4
    
    if offset + ranges_len * 4 > len(blob):
        return stamp, []
        
    ranges = list(struct.unpack(f'<{ranges_len}f', blob[offset:offset+ranges_len*4]))
    return stamp, ranges

class Ros2Db3Reader:
    """Reads messages from a ROS 2 db3 SQLite bag file."""
    
    def __init__(self, run_dir):
        self.run_dir = Path(run_dir)
        self.bag_dir = self.run_dir / "rosbag2"
        self.db_path = self._find_db_file()
        self.topic_map = {}
        if self.db_path:
            self._load_topics()
            
    def _find_db_file(self):
        if not self.bag_dir.exists():
            dbs = list(self.run_dir.glob("**/*.db3"))
            if dbs:
                return dbs[0]
            return None
        dbs = list(self.bag_dir.glob("*.db3"))
        if dbs:
            return dbs[0]
        return None
        
    def _load_topics(self):
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        cursor.execute("SELECT id, name, type FROM topics")
        for topic_id, name, topic_type in cursor.fetchall():
            self.topic_map[name] = {"id": topic_id, "type": topic_type}
        conn.close()
        
    def get_topics(self):
        return list(self.topic_map.keys())
        
    def read_messages(self, topic_name):
        if topic_name not in self.topic_map:
            return []
        
        topic_info = self.topic_map[topic_name]
        topic_id = topic_info["id"]
        topic_type = topic_info["type"]
        
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        cursor.execute(
            "SELECT timestamp, data FROM messages WHERE topic_id = ? ORDER BY timestamp",
            (topic_id,)
        )
        
        parsed_messages = []
        for stamp_ns, data in cursor.fetchall():
            stamp_sec = stamp_ns * 1e-9
            
            try:
                if topic_type == "nav_msgs/msg/Odometry":
                    m_stamp, x, y, theta, vx, vy, wz = parse_odom(data)
                    parsed_messages.append({
                        "timestamp": m_stamp if m_stamp > 0 else stamp_sec,
                        "x": x, "y": y, "theta": theta,
                        "vx": vx, "vy": vy, "omega": wz
                    })
                elif topic_type == "geometry_msgs/msg/Twist":
                    vx, vy, wz = parse_twist(data)
                    parsed_messages.append({
                        "timestamp": stamp_sec,
                        "vx": vx, "vy": vy, "omega": wz
                    })
                elif topic_type == "geometry_msgs/msg/TwistStamped":
                    m_stamp, vx, vy, wz = parse_twist_stamped(data)
                    parsed_messages.append({
                        "timestamp": m_stamp if m_stamp > 0 else stamp_sec,
                        "vx": vx, "vy": vy, "omega": wz
                    })
                elif topic_type == "std_msgs/msg/String":
                    json_str = parse_string(data)
                    try:
                        payload = json.loads(json_str)
                        if "stamp" in payload:
                            m_stamp = payload["stamp"]["sec"] + payload["stamp"]["nanosec"] * 1e-9
                        else:
                            m_stamp = stamp_sec
                        payload["timestamp"] = m_stamp
                        parsed_messages.append(payload)
                    except Exception:
                        parsed_messages.append({
                            "timestamp": stamp_sec,
                            "data": json_str
                        })
                elif topic_type == "std_msgs/msg/Float32MultiArray":
                    floats = parse_float_array(data)
                    parsed_messages.append({
                        "timestamp": stamp_sec,
                        "data": floats
                    })
                elif topic_type == "canmpc_msgs/msg/SolverStats":
                    m_stamp, solve_time_ms, iter_count, status, t_flag, c_flag = parse_solver_stats(data)
                    parsed_messages.append({
                        "timestamp": m_stamp if m_stamp > 0 else stamp_sec,
                        "solve_time_ms": solve_time_ms,
                        "iter_count": iter_count,
                        "status": status,
                        "timeout_flag": t_flag,
                        "collision_flag": c_flag
                    })
                elif topic_type == "nav_msgs/msg/Path":
                    waypoints = parse_path(data)
                    parsed_messages.append({
                        "timestamp": stamp_sec,
                        "waypoints": waypoints
                    })
                elif topic_type == "sensor_msgs/msg/LaserScan":
                    m_stamp, ranges = parse_laser_scan(data)
                    parsed_messages.append({
                        "timestamp": m_stamp if m_stamp > 0 else stamp_sec,
                        "ranges": ranges
                    })

            except Exception as e:
                print(f"Error parsing message on {topic_name}: {e}")
                
        conn.close()
        return parsed_messages
