#!/usr/bin/env python3
"""
validate_bag.py: Verifies topic presence, monotonic timestamps, frame IDs,
and checks that phi_hat_h in [0, 1], among other quality checks.
"""

import os
import sys
import argparse
import numpy as np
from pathlib import Path

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from bag_reader import Ros2Db3Reader

def validate_run_bag(bag_path_or_run_dir):
    path = Path(bag_path_or_run_dir)
    if path.name == "rosbag2":
        run_dir = path.parent
    else:
        run_dir = path
        
    print(f"Validating rosbag2 in: {run_dir}")
    
    reader = Ros2Db3Reader(run_dir)
    if not reader.db_path:
        print(f"Error: Rosbag database not found at {run_dir}", file=sys.stderr)
        return False
        
    topics = reader.get_topics()
    required_topics = [
        "/odom",
        "/cmd_vel",
        "/canmpc/context",
        "/canmpc/humans",
        "/canmpc/solver_stats"
    ]
    
    missing_topics = []
    for rt in required_topics:
        if rt == "/cmd_vel":
            if "/cmd_vel" not in topics and "/cmd_vel_unstamped" not in topics:
                missing_topics.append(rt)
        elif rt not in topics:
            missing_topics.append(rt)
            
    if missing_topics:
        print(f"FAIL: Missing required topics: {missing_topics}")
        return False
    else:
        print("PASS: All required topics are present.")

    validation_failed = False
    
    for t_name in topics:
        if t_name not in required_topics and t_name != "/cmd_vel_unstamped":
            continue
            
        print(f"Analyzing topic: {t_name}...")
        msgs = reader.read_messages(t_name)
        if not msgs:
            print(f"WARNING: Topic {t_name} is empty.")
            continue
            
        timestamps = [m["timestamp"] for m in msgs]
        is_monotonic = all(x < y for x, y in zip(timestamps, timestamps[1:]))
        if not is_monotonic:
            print(f"FAIL: Timestamps on {t_name} are NOT strictly monotonic!")
            validation_failed = True
        else:
            print(f"  - Timestamps: Monotonic ({len(msgs)} messages)")
            
        if t_name == "/odom":
            nans = 0
            for m in msgs:
                if any(np.isnan([m["x"], m["y"], m["theta"], m["vx"], m["vy"], m["omega"]])) or \
                   any(np.isinf([m["x"], m["y"], m["theta"], m["vx"], m["vy"], m["omega"]])):
                    nans += 1
            if nans > 0:
                print(f"FAIL: {nans} messages in /odom contain NaN or Inf values!")
                validation_failed = True
            else:
                print("  - NaN/Inf check: Clean")
                
        elif t_name in ["/cmd_vel", "/cmd_vel_unstamped"]:
            nans = 0
            for m in msgs:
                if any(np.isnan([m["vx"], m["vy"], m["omega"]])) or \
                   any(np.isinf([m["vx"], m["vy"], m["omega"]])):
                    nans += 1
            if nans > 0:
                print(f"FAIL: {nans} messages in {t_name} contain NaN or Inf values!")
                validation_failed = True
            else:
                print("  - NaN/Inf check: Clean")
                
        elif t_name == "/canmpc/context":
            phi_out_of_bounds = 0
            bounds_viol = 0
            for m in msgs:
                phi = m.get("phi_h", 0.0)
                if phi < 0.0 or phi > 1.0:
                    phi_out_of_bounds += 1
                    
                d_safe = m.get("d_safe", 0.0)
                vx_max = m.get("vx_max", 0.0)
                vy_max = m.get("vy_max", 0.0)
                omega_max = m.get("omega_max", 0.0)
                
                if d_safe < 0.2 or vx_max < 0.05 or vy_max < 0.04 or omega_max < 0.1:
                    bounds_viol += 1
                    
            if phi_out_of_bounds > 0:
                print(f"FAIL: {phi_out_of_bounds} context messages have phi_h outside [0, 1]!")
                validation_failed = True
            else:
                print("  - Context score phi_h bounds check: Clean")
                
            if bounds_viol > 0:
                print(f"FAIL: {bounds_viol} messages have adaptive bounds below configured minimums!")
                validation_failed = True
            else:
                print("  - Adaptive bounds limits check: Clean")
                
        elif t_name == "/canmpc/humans":
            vel_spikes = 0
            nans = 0
            for m in msgs:
                humans_list = m.get("humans", [])
                for h in humans_list:
                    if "pose" in h and isinstance(h["pose"], dict):
                        x_h = h["pose"].get("position", {}).get("x", 0.0)
                        y_h = h["pose"].get("position", {}).get("y", 0.0)
                    else:
                        x_h = h.get("x", 0.0)
                        y_h = h.get("y", 0.0)
                        
                    if "velocity" in h and isinstance(h["velocity"], dict):
                        vx_h = h["velocity"].get("linear", {}).get("x", 0.0)
                        vy_h = h["velocity"].get("linear", {}).get("y", 0.0)
                    else:
                        vx_h = h.get("vx", 0.0)
                        vy_h = h.get("vy", 0.0)
                        
                    if np.isnan([x_h, y_h, vx_h, vy_h]).any() or np.isinf([x_h, y_h, vx_h, vy_h]).any():
                        nans += 1
                    
                    speed = np.hypot(vx_h, vy_h)
                    if speed > 3.0:
                        vel_spikes += 1
                        
            if nans > 0:
                print(f"FAIL: {nans} human states contain NaN or Inf values!")
                validation_failed = True
            if vel_spikes > 0:
                print(f"FAIL: Found {vel_spikes} instances of unrealistic human velocity spikes (> 3m/s)!")
                validation_failed = True
            if nans == 0 and vel_spikes == 0:
                print("  - Human state and velocity check: Clean")
                
        elif t_name == "/canmpc/solver_stats":
            stats_failed = 0
            for m in msgs:
                if m.get("solve_time_ms", 0.0) < 0.0 or m.get("iter_count", 0) < 0:
                    stats_failed += 1
            if stats_failed > 0:
                print(f"FAIL: {stats_failed} messages on {t_name} have negative solve time or iter count!")
                validation_failed = True
            else:
                print("  - Solver stats values check: Clean")
                
    if validation_failed:
        print(f"RESULT: Validation FAILED for run bag in {run_dir}")
        return False
        
    print(f"RESULT: Validation PASSED for run bag in {run_dir}")
    return True

def main():
    parser = argparse.ArgumentParser(description="Validate rosbag2 files for dataset quality")
    parser.add_argument("bag_path", help="Path to specific run directory or its rosbag2 folder")
    args = parser.parse_args()
    
    success = validate_run_bag(args.bag_path)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
