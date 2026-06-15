#!/usr/bin/env python3
"""
extract_controller_samples.py: Extract and compile controller telemetry from rosbag2 data
into the tabular schema format required for optimization and controller verification.
"""

import os
import sys
import argparse
from pathlib import Path

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from sync_topics import sync_run_topics
from bag_to_parquet import extract_run_to_parquet

def main():
    parser = argparse.ArgumentParser(description="Extract controller samples schema from rosbag")
    parser.add_argument("--run", required=True, help="Path to specific run directory to extract")
    args = parser.parse_args()
    
    run_dir = Path(args.run)
    if not run_dir.exists():
        print(f"Error: Run directory {run_dir} does not exist.", file=sys.stderr)
        sys.exit(1)
        
    print("Step 1: Extracting raw topics to Parquet...")
    raw_ok = extract_run_to_parquet(run_dir)
    if not raw_ok:
        print("Error: Raw topic extraction failed.", file=sys.stderr)
        sys.exit(1)
        
    print("Step 2: Synchronizing and computing controller samples...")
    sync_ok = sync_run_topics(run_dir)
    if not sync_ok:
        print("Error: Topic synchronization failed.", file=sys.stderr)
        sys.exit(1)
        
    print(f"Success: Controller samples extracted and saved for {run_dir.name}")
    sys.exit(0)

if __name__ == "__main__":
    main()
