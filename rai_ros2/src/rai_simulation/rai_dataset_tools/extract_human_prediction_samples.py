#!/usr/bin/env python3
"""
extract_human_prediction_samples.py: Evaluates human constant-velocity prediction models
against ground truth future paths in a run, writing human_prediction_samples.parquet.
"""

import os
import sys
import argparse
import json
import numpy as np
import pandas as pd
from pathlib import Path

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from bag_to_parquet import extract_run_to_parquet

def extract_human_predictions(run_dir, horizon_n=20, ts=0.1):
    run_dir = Path(run_dir)
    print(f"Extracting human prediction samples for: {run_dir}")
    
    humans_file = run_dir / "humans.parquet"
    metadata_file = run_dir / "metadata.json"
    
    if not humans_file.exists():
        print("humans.parquet not found. Attempting raw extraction...")
        extract_run_to_parquet(run_dir)
        if not humans_file.exists():
            print(f"Error: Could not extract humans topic for {run_dir}", file=sys.stderr)
            return False
            
    scenario_id = "unknown"
    run_id = run_dir.name
    
    if metadata_file.exists():
        try:
            with open(metadata_file, "r") as f:
                meta = json.load(f)
                scenario_id = meta.get("scenario_id", scenario_id)
                run_id = meta.get("run_id", run_id)
        except Exception:
            pass
            
    if scenario_id == "unknown":
        parts = run_dir.resolve().parts
        if len(parts) >= 3:
            scenario_id = parts[-3]
            
    df_hum = pd.read_parquet(humans_file)
    if df_hum.empty or "human_id" not in df_hum.columns:
        print("No human observations found or empty dataframe.")
        return False
        
    df_hum = df_hum[df_hum["human_id"] != -1].copy()
    if df_hum.empty:
        print("No active human records to compute predictions.")
        return False
        
    df_hum = df_hum.sort_values("timestamp")
    
    prediction_rows = []
    grouped = df_hum.groupby("human_id")
    
    for h_id, group in grouped:
        group = group.sort_values("timestamp").reset_index(drop=True)
        timestamps = group["timestamp"].values
        xs = group["x_h"].values
        ys = group["y_h"].values
        vxs = group["vx_h"].values
        vys = group["vy_h"].values
        
        n_obs = len(group)
        for idx in range(n_obs):
            t_curr = timestamps[idx]
            x_curr = xs[idx]
            y_curr = ys[idx]
            vx_curr = vxs[idx]
            vy_curr = vys[idx]
            
            for i in range(horizon_n + 1):
                t_pred = t_curr + i * ts
                x_pred = x_curr + i * ts * vx_curr
                y_pred = y_curr + i * ts * vys_curr
                
                diffs = np.abs(timestamps - t_pred)
                best_idx = np.argmin(diffs)
                
                x_gt = np.nan
                y_gt = np.nan
                pred_error = np.nan
                
                if diffs[best_idx] <= (ts / 2.0):
                    x_gt = xs[best_idx]
                    y_gt = ys[best_idx]
                    pred_error = np.sqrt((x_pred - x_gt)**2 + (y_pred - y_gt)**2)
                    
                prediction_rows.append({
                    "timestamp": t_curr,
                    "scenario_id": scenario_id,
                    "run_id": run_id,
                    "human_id": int(h_id),
                    "horizon_i": i,
                    "x_h_pred": x_pred,
                    "y_h_pred": y_pred,
                    "x_h_gt": x_gt,
                    "y_h_gt": y_gt,
                    "prediction_error": pred_error
                })
                
    if not prediction_rows:
        print("No prediction rows generated.")
        return False
        
    df_pred = pd.DataFrame(prediction_rows)
    output_file = run_dir / "human_prediction_samples.parquet"
    df_pred.to_parquet(output_file, index=False)
    print(f"Saved {len(df_pred)} prediction rows to {output_file}")
    return True

def main():
    parser = argparse.ArgumentParser(description="Extract human prediction samples from run telemetry")
    parser.add_argument("--run", required=True, help="Path to specific run directory")
    parser.add_argument("--horizon", type=int, default=20, help="NMPC horizon steps N")
    parser.add_argument("--ts", type=type(0.1), default=0.1, help="NMPC timestep Ts")
    args = parser.parse_args()
    
    success = extract_human_predictions(args.run, args.horizon, args.ts)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
