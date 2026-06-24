#!/usr/bin/env python3
"""
process_dataset.py: Master execution script for the dataset processing pipeline.
Automates extraction, synchronization, metric evaluation, and dataset splitting.
"""

import os
import sys
import argparse
from pathlib import Path


SCRIPT_DIR = Path(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(str(SCRIPT_DIR))

from bag_to_parquet import extract_run_to_parquet
from sync_topics import sync_run_topics
from build_run_index import build_run_index
from dataset_split import split_dataset

def main():
    parser = argparse.ArgumentParser(description="Master dataset pipeline processor for CA-NMPC experiments")
    parser.add_argument("--path", default="d:/Research/ijat2026/rai_ros2/src/rai_simulation/dataset", 
                        help="Dataset base directory containing raw run folders")
    parser.add_argument("--seed", type=int, default=42, help="Seed for Train/Val/Test dataset splitting")
    args = parser.parse_args()

    base_path = Path(args.path).expanduser()
    if not base_path.exists():
        print(f"Error: Base directory {base_path} does not exist.", file=sys.stderr)
        sys.exit(1)

    print("\n" + "="*60)
    print("STARTING DATASET TELEMETRY EXTRACTION AND PROCESSING PIPELINE")
    print("="*60)
    print(f"Dataset directory: {base_path.resolve()}")
    print(f"Split seed: {args.seed}")

    
    print("\n[Step 1/5] Scanning for run directories containing rosbag2...")
    run_dirs = []
    for root, dirs, files in os.walk(base_path):
        if "rosbag2" in dirs:
            run_dirs.append(Path(root))
    
    total_runs = len(run_dirs)
    print(f"Found {total_runs} runs with rosbag2 databases.")
    if total_runs == 0:
        print("Error: No rosbag2 runs found. Cannot proceed.", file=sys.stderr)
        sys.exit(1)

    
    print("\n[Step 2/5] Extracting rosbag2 databases to raw Parquet files...")
    raw_success = 0
    for idx, r_dir in enumerate(run_dirs, start=1):
        print(f"({idx}/{total_runs}) Extracting run: {r_dir.name}...")
        try:
            if extract_run_to_parquet(r_dir):
                raw_success += 1
        except Exception as e:
            print(f"Error during raw extraction for {r_dir.name}: {e}", file=sys.stderr)
            
    print(f"Raw extraction completed: {raw_success}/{total_runs} runs processed successfully.")

    
    print("\n[Step 3/5] Synchronizing high-frequency telemetry & computing jerks/clearance...")
    sync_success = 0
    for idx, r_dir in enumerate(run_dirs, start=1):
        print(f"({idx}/{total_runs}) Synchronizing run: {r_dir.name}...")
        try:
            if sync_run_topics(r_dir):
                sync_success += 1
        except Exception as e:
            print(f"Error during topic synchronization for {r_dir.name}: {e}", file=sys.stderr)
            
    print(f"Synchronization completed: {sync_success}/{total_runs} runs processed successfully.")

    
    print("\n[Step 4/5] Building centralized run index and computing evaluation metrics (RMSE, Jerk, Clearance)...")
    index_ok = False
    try:
        index_ok = build_run_index(base_path)
    except Exception as e:
        print(f"Error during run index building: {e}", file=sys.stderr)

    if not index_ok:
        print("Error: Centralized run index generation failed. Aborting split compile.", file=sys.stderr)
        sys.exit(1)

    
    print("\n[Step 5/5] Splitting dataset (70/15/15) and compiling consolidated split Parquet files...")
    split_ok = False
    try:
        split_ok = split_dataset(base_path, args.seed)
    except Exception as e:
        print(f"Error during dataset splitting: {e}", file=sys.stderr)

    if not split_ok:
        print("Error: Dataset splitting or parquet consolidation failed.", file=sys.stderr)
        sys.exit(1)

    print("\n" + "="*60)
    print("DATASET PIPELINE PROCESSING COMPLETED SUCCESSFULLY!")
    print("="*60 + "\n")

if __name__ == "__main__":
    main()
