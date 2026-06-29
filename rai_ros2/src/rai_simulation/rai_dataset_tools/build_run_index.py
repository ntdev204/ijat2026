#!/usr/bin/env python3
"""
build_run_index.py: Scans runs, verifies metadata correctness, and builds
a centralized run_index.csv under dataset/metadata/.
"""

import os
import sys
import json
import argparse
from pathlib import Path
import numpy as np
import pandas as pd

def build_run_index(base_path):
    base_path = Path(base_path).expanduser()
    print(f"Scanning base path for runs: {base_path}")
    
    metadata_dir = base_path / "metadata"
    metadata_dir.mkdir(parents=True, exist_ok=True)
    
    raw_dir = base_path / "raw"
    if not raw_dir.exists():
        raw_dir = base_path
        
    print(f"Searching under raw directory: {raw_dir}")
    
    run_records = []
    meta_files = list(raw_dir.glob("**/metadata.json"))
    print(f"Found {len(meta_files)} metadata.json files.")
    
    for meta_file in meta_files:
        run_dir = meta_file.parent
        parts = run_dir.resolve().relative_to(base_path.resolve()).parts
        
        environment = "sim"
        for part in parts:
            if part in ["sim", "real"]:
                environment = part
                break
                
        try:
            with open(meta_file, "r") as f:
                meta = json.load(f)
        except Exception as e:
            print(f"Warning: Could not read {meta_file}: {e}")
            continue
            
        schema_ver = meta.get("schema_version", "")
        flow = meta.get("flow", "")
        if schema_ver != "2.0.0" or flow != "continuous_context_adaptive_ca_nmpc":
            print(f"Warning: Run at {run_dir} has invalid metadata schema. Expected schema_version 2.0.0 and continuous_context_adaptive_ca_nmpc.")
            
        run_id = meta.get("run_id", run_dir.name)
        scenario_id = meta.get("scenario_id", "unknown")
        controller_id = meta.get("controller_id", "unknown")
        duration = meta.get("duration", 0.0)
        success = meta.get("success", True)
        
        parquet_file = run_dir / "controller_samples.parquet"
        rmse_xy = -1.0
        rmse_theta = -1.0
        mean_jerk = -1.0
        max_jerk = -1.0
        min_clearance = -1.0

        if parquet_file.exists():
            try:
                df = pd.read_parquet(parquet_file)
                row_count = len(df)
                if "solve_time_ms" in df.columns:
                    avg_solve_time = df["solve_time_ms"].mean()
                if "collision_flag" in df.columns:
                    collision_count = int(df["collision_flag"].any())
                if "timeout_flag" in df.columns:
                    timeout_rate = df["timeout_flag"].mean()
                if "tracking_error_xy" in df.columns:
                    rmse_xy = np.sqrt((df["tracking_error_xy"]**2).mean())
                if "tracking_error_theta" in df.columns:
                    rmse_theta = np.sqrt((df["tracking_error_theta"]**2).mean())
                if "jerk_magnitude" in df.columns:
                    mean_jerk = df["jerk_magnitude"].mean()
                    max_jerk = df["jerk_magnitude"].max()
                if "min_costmap_clearance" in df.columns:
                    valid_clearances = df["min_costmap_clearance"].dropna()
                    if not valid_clearances.empty:
                        min_clearance = valid_clearances.min()
            except Exception as e:
                print(f"Warning: Could not extract stats from {parquet_file}: {e}")
                
        record = {
            "run_id": run_id,
            "scenario_id": scenario_id,
            "controller_id": controller_id,
            "environment": environment,
            "duration_sec": duration,
            "success": success,
            "row_count": row_count,
            "avg_solve_time_ms": avg_solve_time if avg_solve_time is not None else -1.0,
            "collision_occurred": collision_count > 0,
            "timeout_rate": timeout_rate,
            "rmse_xy": rmse_xy,
            "rmse_theta": rmse_theta,
            "mean_jerk": mean_jerk,
            "max_jerk": max_jerk,
            "min_clearance": min_clearance,
            "path": str(run_dir.resolve())
        }
        
        run_records.append(record)
        
    if not run_records:
        print("No runs found. run_index.csv was not created.")
        return False
        
    df_index = pd.DataFrame(run_records)
    output_file = metadata_dir / "run_index.csv"
    df_index.to_csv(output_file, index=False)
    
    print("\n" + "="*50)
    print("RUN INDEX BUILD COMPLETE")
    print("="*50)
    print(f"Saved run index to: {output_file}")
    print(f"Total runs indexed: {len(df_index)}")
    print(f"  - Sim runs:  {len(df_index[df_index['environment'] == 'sim'])}")
    print(f"  - Real runs: {len(df_index[df_index['environment'] == 'real'])}")
    
    print("\nRuns by scenario:")
    for sc, count in df_index["scenario_id"].value_counts().items():
        print(f"  - {sc}: {count} runs")
        
    print("\nRuns by controller:")
    for ctrl, count in df_index["controller_id"].value_counts().items():
        print(f"  - {ctrl}: {count} runs")
    print("="*50 + "\n")
    
    return True

def main():
    parser = argparse.ArgumentParser(description="Scan run directories and build run_index.csv")
    parser.add_argument("--path", default="d:/Research/ijat2026/rai_ros2/src/rai_simulation/dataset", help="Dataset base path")
    args = parser.parse_args()
    
    success = build_run_index(args.path)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
