#!/usr/bin/env python3
"""
Automated Scenario Batch Runner for CCA-NMPC Gazebo Experiments (Phase 9.1).
Automates repeated execution of the configured scenario set for controller benchmarking.
"""

import os
import sys
import time
import json
import random
import math
import shutil
import signal
import subprocess
import threading
from typing import List, Dict, Any, Optional

# ROS 2 imports
import rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry
from geometry_msgs.msg import PoseStamped, Twist
from std_msgs.msg import String

# Default paths
WORKSPACE_DIR = "d:/Research/ijat2026"
URDF_PATH = os.path.join(WORKSPACE_DIR, "rai_ros2/src/rai_robot_urdf/urdf/mini_mec_robot.urdf")
CONTROLLER_PARAMS_PATH = os.path.join(
    WORKSPACE_DIR,
    "rai_ros2/src/rai_navigation/rai_navigation/config/rai_navigation.yaml",
)
OUTPUT_BASE_DIR = os.path.join(WORKSPACE_DIR, "rai_ros2/src/rai_simulation/dataset")
WORLDS_DIR = os.path.join(WORKSPACE_DIR, "rai_ros2/src/rai_simulation/rai_gazebo_worlds/worlds")

SCENARIOS = {
    "S1": {
        "name": "S1_open_zone",
        "world": "s1_open_zone.world",
        "world_name": "s1_open_zone",
        "start": [0.0, 0.0, 0.0],
        "goal": [6.0, 0.0, 0.0],
    },
    "S2": {
        "name": "S2_narrow_corridor",
        "world": "s2_narrow_corridor.world",
        "world_name": "s2_narrow_corridor",
        "start": [0.0, 3.0, 0.0],
        "goal": [6.0, 3.0, 0.0],
    },
    "S3": {
        "name": "S3_human_proximity",
        "world": "s3_human_proximity.world",
        "world_name": "s3_human_proximity",
        "start": [0.0, -3.0, 0.0],
        "goal": [6.0, -3.0, 0.0],
    },
    "S4": {
        "name": "S4_dynamic_crossing",
        "world": "s4_crossing_path.world",
        "world_name": "s4_crossing_path",
        "start": [0.0, 0.0, 0.0],
        "goal": [6.0, 0.0, 0.0],
    },
    "S5": {
        "name": "S5_occlusion",
        "world": "s5_occlusion.world",
        "world_name": "s5_occlusion",
        "start": [0.0, 5.0, 0.0],
        "goal": [6.0, 5.0, 0.0],
    }
}

class TrialMonitorNode(Node):
    """
    ROS 2 Node to monitor the progress of a single simulation trial.
    Sends goal poses, monitors robot state, and triggers actor movement.
    """
    def __init__(self, start_pose: List[float], goal_pose: List[float], scenario_id: str):
        super().__init__('trial_monitor_node')
        self.start_pose = start_pose
        self.goal_pose = goal_pose
        self.scenario_id = scenario_id
        
        self.current_pose = [0.0, 0.0, 0.0]
        self.odom_received = False
        self.goal_reached = False
        self.collision_detected = False
        self.actor_triggered = False
        
        # Publishers and Subscribers
        self.odom_sub = self.create_subscription(
            Odometry,
            '/odom_combined',
            self.odom_callback,
            10
        )
        
        self.goal_pub = self.create_publisher(
            PoseStamped,
            '/goal_pose',
            10
        )
        
        self.human_vel_pub = self.create_publisher(
            Twist,
            '/model/s5_human/cmd_vel',
            10
        )
        
        self.get_logger().info(f"Trial monitor initialized for scenario {scenario_id}")

    def odom_callback(self, msg: Odometry):
        self.odom_received = True
        pos = msg.pose.pose.position
        q = msg.pose.pose.orientation
        siny_cosp = 2.0 * (q.w * q.z + q.x * q.y)
        cosy_cosp = 1.0 - 2.0 * (q.y * q.y + q.z * q.z)
        yaw = math.atan2(siny_cosp, cosy_cosp)
        
        self.current_pose = [pos.x, pos.y, yaw]
        
        # Check if goal reached
        dist_to_goal = math.hypot(pos.x - self.goal_pose[0], pos.y - self.goal_pose[1])
        if dist_to_goal < 0.35:  # tolerance
            self.goal_reached = True
            
        # Trigger S5 human when robot crosses x = 2.0
        if self.scenario_id == "S5" and not self.actor_triggered and pos.x >= 2.0:
            self.trigger_s5_actor()

    def send_goal(self):
        msg = PoseStamped()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = 'map'
        msg.pose.position.x = self.goal_pose[0]
        msg.pose.position.y = self.goal_pose[1]
        msg.pose.position.z = 0.0
        
        yaw = self.goal_pose[2]
        msg.pose.orientation.z = math.sin(yaw / 2.0)
        msg.pose.orientation.w = math.cos(yaw / 2.0)
        
        self.goal_pub.publish(msg)
        self.get_logger().info(f"Published goal pose: {self.goal_pose}")

    def trigger_s5_actor(self):
        self.actor_triggered = True
        self.get_logger().info("Robot crossed x = 2.0! Triggering S5 human actor...")
        
        # Start a thread to move the actor from y = 6.0 to y = 5.0 at 1.2 m/s
        # Distance = 1.0m, Velocity = 1.2 m/s, Duration = 1.0 / 1.2 = 0.833 seconds
        def move_thread():
            rate = self.create_rate(25) # 25 Hz
            start_time = time.time()
            duration = 1.0 / 1.2
            
            while time.time() - start_time < duration:
                twist = Twist()
                twist.linear.y = -1.2  # move along negative Y-axis
                self.human_vel_pub.publish(twist)
                rate.sleep()
                
            # Stop the actor at y = 5.0
            stop_twist = Twist()
            self.human_vel_pub.publish(stop_twist)
            self.get_logger().info("S5 actor reached target position [3.0, 5.0] and stopped.")
            
        t = threading.Thread(target=move_thread)
        t.start()

def clean_leftover_processes():
    """Clean up any leftover Gazebo and ROS 2 processes to ensure clean trial environment."""
    print("Cleaning up old simulation and ROS 2 nodes...")
    if sys.platform == "win32":
        cmds = [
            'taskkill /F /IM ruby.exe /T',
            'taskkill /F /IM ign.exe /T',
            'taskkill /F /IM gazebo.exe /T',
            'taskkill /F /IM rviz2.exe /T',
            'taskkill /F /IM ros2.exe /T',
            'taskkill /F /IM _ros2_daemon.exe /T',
            'taskkill /F /IM static_transform_publisher.exe /T'
        ]
        for cmd in cmds:
            subprocess.run(cmd, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    else:
        # Linux / WSL
        subprocess.run('pkill -9 -f "ign gazebo"', shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        subprocess.run('pkill -9 -f "ros2"', shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        subprocess.run('pkill -9 -f "controller_server"', shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        subprocess.run('pkill -9 -f "ros_gz_bridge"', shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(2.0)

def generate_jittered_pose(base_pose: List[float], pos_std: float = 0.05, yaw_std: float = 0.03) -> List[float]:
    """Generate a randomized starting/goal pose using Gaussian jitter."""
    x = base_pose[0] + random.normalvariate(0, pos_std)
    y = base_pose[1] + random.normalvariate(0, pos_std)
    yaw = base_pose[2] + random.normalvariate(0, yaw_std)
    return [x, y, yaw]

def run_trial(scenario_id: str, run_idx: int, controller_id: str, global_planner: str) -> Dict[str, Any]:
    """Execute a single scenario simulation run."""
    print(f"\n==================== Starting Scenario {scenario_id} - Run {run_idx:03d} ====================")
    
    # 1. Prepare Paths
    scen_info = SCENARIOS[scenario_id]
    world_path = os.path.join(WORLDS_DIR, scen_info["world"])
    run_name = f"run_{run_idx:03d}"
    run_dir = os.path.join(OUTPUT_BASE_DIR, scen_info["name"], controller_id, run_name)
    os.makedirs(run_dir, exist_ok=True)
    
    # Generate jittered poses
    start_pose = generate_jittered_pose(scen_info["start"])
    goal_pose = generate_jittered_pose(scen_info["goal"])
    
    # 2. Cleanup existing nodes
    clean_leftover_processes()
    
    # 3. Launch Gazebo Fortress (headless mode suggested to save resources)
    print(f"Launching Gazebo with world: {scen_info['world']}")
    gz_cmd = f"ign gazebo {world_path} -r -s"  # -s runs headless
    gz_proc = subprocess.Popen(gz_cmd, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(4.0)  # wait for Gazebo to start
    
    # 4. Spawn the Mecanum robot
    print(f"Spawning mini_mec_robot at start pose: {start_pose}")
    spawn_cmd = (
        f"ros2 run ros_gz_sim create -world {scen_info['world_name']} "
        f"-file {URDF_PATH} -name mini_mec_robot "
        f"-x {start_pose[0]} -y {start_pose[1]} -z 0.1 -Y {start_pose[2]}"
    )
    subprocess.run(spawn_cmd, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(2.0)
    
    # 5. Launch ros_gz_bridge
    print("Launching ROS 2 Gazebo bridges...")
    bridge_topics = [
        "/clock@rosgraph_msgs/msg/Clock[ignition.msgs.Clock",
        "/scan@sensor_msgs/msg/LaserScan[ignition.msgs.LaserScan",
        "/model/mini_mec_robot/odometry@nav_msgs/msg/Odometry[ignition.msgs.Odometry",
        "/cmd_vel@geometry_msgs/msg/Twist]ignition.msgs.Twist",
        "/tf@tf2_msgs/msg/TFMessage[ignition.msgs.Pose_V",
        "/tf_static@tf2_msgs/msg/TFMessage[ignition.msgs.Pose_V",
    ]
    if scenario_id == "S5":
        bridge_topics.append("/model/s5_human/cmd_vel@geometry_msgs/msg/Twist]ignition.msgs.Twist")
        
    bridge_cmd = f"ros2 run ros_gz_bridge parameter_bridge {' '.join(bridge_topics)} --ros-args -r /model/mini_mec_robot/odometry:=/odom_combined"
    bridge_proc = subprocess.Popen(bridge_cmd, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(2.0)
    
    # 6. Launch context monitor (from rai_dataset_collection)
    print("Launching context monitor node...")
    context_cmd = "ros2 run rai_dataset_collection context_monitor --ros-args -p use_sim_time:=True"
    context_proc = subprocess.Popen(context_cmd, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(2.0)
    
    # 7. Launch standalone RAI navigation stack.
    print("Launching standalone RAI navigation stack...")
    controller_cmd = (
        "ros2 launch rai_navigation rai_navigation.launch.py "
        f"params:={CONTROLLER_PARAMS_PATH} use_sim_time:=True "
        "cmd_vel_topic:=/cmd_vel laser_scan_topic:=/scan imu_topic:= map_topic:= "
        f"controller_id:={controller_id} global_planner_algorithm:={global_planner}"
    )
    controller_proc = subprocess.Popen(controller_cmd, shell=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(4.0)
    
    # 8. Start ROS bag recording
    bag_out_dir = os.path.join(run_dir, "rosbag2")
    bag_topics = [
        "/scan", "/tf", "/tf_static", "/odom_combined", "/cmd_vel",
        "/canmpc/context", "/canmpc/humans", "/rai_navigation/global_path",
        "/canmpc/solver_stats", "/canmpc/adaptive_bounds",
        "/rai_navigation/local_costmap", "/rai_navigation/status", "/map"
    ]
    print(f"Starting ros2 bag record to {bag_out_dir}...")
    bag_cmd = f"ros2 bag record -o {bag_out_dir} {' '.join(bag_topics)}"
    
    # Run bag recording in a separate process group if on Windows to allow sending CTRL_C
    if sys.platform == "win32":
        bag_proc = subprocess.Popen(bag_cmd, shell=True, creationflags=subprocess.CREATE_NEW_PROCESS_GROUP)
    else:
        bag_proc = subprocess.Popen(bag_cmd, shell=True, preexec_fn=os.setsid)
        
    time.sleep(2.0)
    
    # 9. ROS 2 node to monitor the trial progress
    rclpy.init()
    monitor = TrialMonitorNode(start_pose, goal_pose, scenario_id)
    
    # Send start goal command
    monitor.send_goal()
    
    # Run the trial loop
    max_duration = 35.0  # seconds timeout
    start_time = time.time()
    success = False
    reason = "timeout"
    
    rate = monitor.create_rate(10) # 10 Hz
    while time.time() - start_time < max_duration:
        rclpy.spin_once(monitor, timeout_sec=0.1)
        
        if monitor.goal_reached:
            print("Target reached successfully!")
            success = True
            reason = "reached_goal"
            break
            
        rate.sleep()
        
    duration = time.time() - start_time
    monitor.destroy_node()
    rclpy.shutdown()
    
    # 10. Clean stop bag recording
    print("Stopping bag recording...")
    if sys.platform == "win32":
        subprocess.run(f"taskkill /F /PID {bag_proc.pid} /T", shell=True)
    else:
        try:
            os.killpg(os.getpgid(bag_proc.pid), signal.SIGINT)
        except ProcessLookupError:
            pass
    time.sleep(2.0)
    
    # 11. Clean up processes
    clean_leftover_processes()
    
    # 12. Save metadata JSON
    metadata = {
        "schema_version": "2.0.0",
        "flow": "continuous_context_adaptive_ca_nmpc",
        "scenario_id": scenario_id,
        "scenario_name": scen_info["name"],
        "run_id": run_name,
        "controller_id": controller_id,
        "global_planner": global_planner,
        "start_pose": start_pose,
        "goal_pose": goal_pose,
        "success": success,
        "duration": duration,
        "reason": reason,
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%S"),
        "random_seed": random.getstate()[1][0]
    }
    
    metadata_path = os.path.join(run_dir, "metadata.json")
    with open(metadata_path, 'w') as f:
        json.dump(metadata, f, indent=4)
        
    print(f"Run completed. Success: {success}, Duration: {duration:.2f}s, Reason: {reason}")
    print(f"Metadata written to: {metadata_path}")
    
    return metadata

def main():
    import argparse
    parser = argparse.ArgumentParser(description="Automated Gazebo scenario batch runner for CCA-NMPC benchmark experiments.")
    parser.add_argument("--scenarios", type=str, nargs="+", default=["S1", "S2", "S3", "S4", "S5"], help="Scenarios to run (e.g. S1 S2 S3 S4 S5)")
    parser.add_argument("--controller", type=str, default="CCA_NMPC", help="Controller id (CCA_NMPC, NMPC, MPPI, DWA, DWB, TEB)")
    parser.add_argument("--global-planner", type=str, default="A_STAR", help="Global planner: A_STAR, DIJKSTRA, STRAIGHT_LINE")
    parser.add_argument("--runs", type=int, default=30, help="Number of runs per scenario (default: 30)")
    parser.add_argument("--seed", type=int, default=42, help="Seed value for reproducibility")
    args = parser.parse_args()
    
    random.seed(args.seed)
    
    print("Starting CCA-NMPC Scenario Runner Batch Process...")
    print(f"Target Scenarios: {args.scenarios}")
    print(f"Controller: {args.controller}")
    print(f"Global planner: {args.global_planner}")
    print(f"Runs per scenario: {args.runs}")
    print(f"Seed: {args.seed}")
    
    summary = []
    
    for scen in args.scenarios:
        if scen not in SCENARIOS:
            print(f"Warning: Scenario {scen} not found. Skipping.")
            continue
            
        scen_success_count = 0
        for run_idx in range(args.runs):
            try:
                res = run_trial(scen, run_idx, args.controller, args.global_planner)
                summary.append(res)
                if res["success"]:
                    scen_success_count += 1
            except Exception as e:
                print(f"Error executing scenario {scen} run {run_idx}: {e}")
                clean_leftover_processes()
                
        print(f"\nScenario {scen} Finished. Success rate: {scen_success_count}/{args.runs} ({scen_success_count/args.runs * 100:.1f}%)")
        
    # Write batch summary run index CSV
    run_index_path = os.path.join(OUTPUT_BASE_DIR, "run_index.csv")
    print(f"\nWriting batch run index to: {run_index_path}")
    
    header = "scenario_id,run_id,controller_id,success,duration,reason,timestamp\n"
    write_header = not os.path.exists(run_index_path)
    
    with open(run_index_path, 'a' if not write_header else 'w') as f:
        if write_header:
            f.write(header)
        for s in summary:
            line = f"{s['scenario_id']},{s['run_id']},{s['controller_id']},{s['success']},{s['duration']:.3f},{s['reason']},{s['timestamp']}\n"
            f.write(line)
            
    print("Batch run index updated successfully.")
    print("All tasks completed.")

if __name__ == "__main__":
    main()
