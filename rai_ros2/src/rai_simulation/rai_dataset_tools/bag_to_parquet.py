#!/usr/bin/env python3
"""
bag_to_parquet.py: Extracts topic telemetry from rosbag2 databases in a run directory
and writes them to Apache Parquet format.
"""

import os
import sys
import argparse
from pathlib import Path
import pandas as pd

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from bag_reader import Ros2Db3Reader

def extract_run_to_parquet(run_dir):
    run_dir = Path(run_dir)
    print(f"Extracting rosbag to parquet for: {run_dir}")
    
    reader = Ros2Db3Reader(run_dir)
    if not reader.db_path:
        print(f"Error: No rosbag database found in {run_dir}", file=sys.stderr)
        return False
        
    topics = reader.get_topics()
    print(f"Found topics: {topics}")
    
    extracted_any = False
    
    
    odom_topic = "/odom"
    if odom_topic in topics:
        print(f"Extracting {odom_topic}...")
        msgs = reader.read_messages(odom_topic)
        if msgs:
            df = pd.DataFrame(msgs)
            output_file = run_dir / "odom.parquet"
            df.to_parquet(output_file, index=False)
            print(f"Saved {len(df)} rows to {output_file}")
            extracted_any = True
            
    
    cmd_topic = None
    if "/cmd_vel" in topics:
        cmd_topic = "/cmd_vel"
    elif "/cmd_vel_unstamped" in topics:
        cmd_topic = "/cmd_vel_unstamped"
        
    if cmd_topic:
        print(f"Extracting {cmd_topic}...")
        msgs = reader.read_messages(cmd_topic)
        if msgs:
            df = pd.DataFrame(msgs)
            output_file = run_dir / "cmd_vel.parquet"
            df.to_parquet(output_file, index=False)
            print(f"Saved {len(df)} rows to {output_file}")
            extracted_any = True
            
    
    context_topic = "/canmpc/context"
    if context_topic in topics:
        print(f"Extracting {context_topic}...")
        msgs = reader.read_messages(context_topic)
        if msgs:
            df = pd.DataFrame(msgs)
            if "stamp" in df.columns:
                df["stamp_sec"] = df["stamp"].apply(lambda s: s.get("sec", 0) if isinstance(s, dict) else 0)
                df["stamp_nanosec"] = df["stamp"].apply(lambda s: s.get("nanosec", 0) if isinstance(s, dict) else 0)
                df = df.drop(columns=["stamp"])
            output_file = run_dir / "context.parquet"
            df.to_parquet(output_file, index=False)
            print(f"Saved {len(df)} rows to {output_file}")
            extracted_any = True
            
    
    humans_topic = "/canmpc/humans"
    if humans_topic in topics:
        print(f"Extracting {humans_topic}...")
        msgs = reader.read_messages(humans_topic)
        if msgs:
            flattened = []
            for msg in msgs:
                stamp = msg.get("timestamp", 0.0)
                humans_list = msg.get("humans", [])
                if not humans_list:
                    flattened.append({
                        "timestamp": stamp,
                        "human_id": -1,
                        "x_h": None,
                        "y_h": None,
                        "vx_h": None,
                        "vy_h": None,
                        "confidence": 0.0
                    })
                else:
                    for h in humans_list:
                        h_id = h.get("id", 0)
                        conf = h.get("confidence", 0.5)
                        
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
                            
                        flattened.append({
                            "timestamp": stamp,
                            "human_id": h_id,
                            "x_h": x_h,
                            "y_h": y_h,
                            "vx_h": vx_h,
                            "vy_h": vy_h,
                            "confidence": conf
                        })
            df = pd.DataFrame(flattened)
            output_file = run_dir / "humans.parquet"
            df.to_parquet(output_file, index=False)
            print(f"Saved {len(df)} rows to {output_file}")
            extracted_any = True

    
    solver_topic = "/canmpc/solver_stats"
    if solver_topic in topics:
        print(f"Extracting {solver_topic}...")
        msgs = reader.read_messages(solver_topic)
        if msgs:
            df = pd.DataFrame(msgs)
            output_file = run_dir / "solver_stats.parquet"
            df.to_parquet(output_file, index=False)
            print(f"Saved {len(df)} rows to {output_file}")
            extracted_any = True
            
    
    bounds_topic = "/canmpc/adaptive_bounds"
    if bounds_topic in topics:
        print(f"Extracting {bounds_topic}...")
        msgs = reader.read_messages(bounds_topic)
        if msgs:
            flat_bounds = []
            for msg in msgs:
                data = msg.get("data", [])
                if len(data) >= 5:
                    flat_bounds.append({
                        "timestamp": msg.get("timestamp", 0.0),
                        "phi_h": data[0],
                        "d_safe": data[1],
                        "vx_max": data[2],
                        "vy_max": data[3],
                        "omega_max": data[4]
                    })
            if flat_bounds:
                df = pd.DataFrame(flat_bounds)
                output_file = run_dir / "adaptive_bounds.parquet"
                df.to_parquet(output_file, index=False)
                print(f"Saved {len(df)} rows to {output_file}")
                extracted_any = True

    
    path_topic = "/canmpc/local_reference_path"
    if path_topic in topics:
        print(f"Extracting {path_topic}...")
        msgs = reader.read_messages(path_topic)
        if msgs:
            flat_path = []
            for msg in msgs:
                timestamp = msg.get("timestamp", 0.0)
                waypoints = msg.get("waypoints", [])
                for idx, wp in enumerate(waypoints):
                    flat_path.append({
                        "timestamp": timestamp,
                        "waypoint_index": idx,
                        "x_ref": wp[0],
                        "y_ref": wp[1],
                        "theta_ref": wp[2]
                    })
            if flat_path:
                df = pd.DataFrame(flat_path)
                output_file = run_dir / "local_reference_path.parquet"
                df.to_parquet(output_file, index=False)
                print(f"Saved {len(df)} rows to {output_file}")
                extracted_any = True

    
    scan_topic = "/scan"
    if scan_topic in topics:
        import math
        print(f"Extracting {scan_topic}...")
        msgs = reader.read_messages(scan_topic)
        if msgs:
            clearances = []
            for msg in msgs:
                ranges = msg.get("ranges", [])
                valid_ranges = [r for r in ranges if r > 0.05 and not math.isnan(r) and not math.isinf(r)]
                min_r = min(valid_ranges) if valid_ranges else 10.0
                clearances.append({
                    "timestamp": msg.get("timestamp", 0.0),
                    "min_scan_clearance": min_r
                })
            if clearances:
                df = pd.DataFrame(clearances)
                output_file = run_dir / "scan.parquet"
                df.to_parquet(output_file, index=False)
                print(f"Saved {len(df)} rows to {output_file}")
                extracted_any = True

    return extracted_any

def main():
    parser = argparse.ArgumentParser(description="Convert rosbag2 topic data to Parquet tables")
    parser.add_argument("--run", help="Path to specific run directory containing a rosbag2 folder")
    parser.add_argument("--dir", help="Path to directory containing multiple runs to process recursively")
    args = parser.parse_args()
    
    if args.run:
        success = extract_run_to_parquet(args.run)
        sys.exit(0 if success else 1)
    elif args.dir:
        run_dirs = []
        for root, dirs, files in os.walk(args.dir):
            if "rosbag2" in dirs:
                run_dirs.append(root)
        print(f"Found {len(run_dirs)} runs to process in {args.dir}")
        success_count = 0
        for r_dir in run_dirs:
            if extract_run_to_parquet(r_dir):
                success_count += 1
        print(f"Processed {success_count}/{len(run_dirs)} runs successfully")
        sys.exit(0)
    else:
        parser.print_help()
        sys.exit(1)

if __name__ == "__main__":
    main()
