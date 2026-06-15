#!/usr/bin/env python3
"""
sync_topics.py: Synchronizes high-frequency signals like /odom, /cmd_vel,
/canmpc/context, and /canmpc/humans into a unified timestamped pandas DataFrame.
"""

import os
import sys
import argparse
import json
import numpy as np
import pandas as pd
from pathlib import Path

def sync_run_topics(run_dir):
    run_dir = Path(run_dir)
    print(f"Synchronizing topics in run directory: {run_dir}")
    
    odom_file = run_dir / "odom.parquet"
    cmd_file = run_dir / "cmd_vel.parquet"
    context_file = run_dir / "context.parquet"
    humans_file = run_dir / "humans.parquet"
    stats_file = run_dir / "solver_stats.parquet"
    path_file = run_dir / "local_reference_path.parquet"
    metadata_file = run_dir / "metadata.json"
    
    if not odom_file.exists():
        print(f"Error: {odom_file} does not exist. Cannot synchronize.", file=sys.stderr)
        return False
        
    df_odom = pd.read_parquet(odom_file).sort_values("timestamp")
    
    scenario_id = "unknown"
    run_id = run_dir.name
    controller_id = "unknown"
    
    if metadata_file.exists():
        try:
            with open(metadata_file, "r") as f:
                meta = json.load(f)
                scenario_id = meta.get("scenario_id", scenario_id)
                run_id = meta.get("run_id", run_id)
                controller_id = meta.get("controller_id", controller_id)
        except Exception as e:
            print(f"Warning: Could not parse metadata.json: {e}")
            
    if scenario_id == "unknown" or controller_id == "unknown":
        parts = run_dir.resolve().parts
        if len(parts) >= 3:
            if controller_id == "unknown":
                controller_id = parts[-2]
            if scenario_id == "unknown":
                scenario_id = parts[-3]

    print(f"  Run Metadata: Scenario={scenario_id}, Controller={controller_id}, Run={run_id}")

    df_sync = df_odom.copy()
    df_sync = df_sync.rename(columns={
        "x": "x_r",
        "y": "y_r",
        "theta": "theta_r",
        "vx": "vx_odom",
        "vy": "vy_odom",
        "omega": "omega_odom"
    })
    
    if cmd_file.exists():
        df_cmd = pd.read_parquet(cmd_file).sort_values("timestamp")
        df_cmd = df_cmd.rename(columns={
            "vx": "vx_cmd",
            "vy": "vy_cmd",
            "omega": "omega_cmd"
        })
        df_sync = pd.merge_asof(
            df_sync, df_cmd,
            on="timestamp",
            direction="nearest",
            tolerance=0.1
        )
    else:
        df_sync["vx_cmd"] = 0.0
        df_sync["vy_cmd"] = 0.0
        df_sync["omega_cmd"] = 0.0
        
    if context_file.exists():
        df_ctx = pd.read_parquet(context_file).sort_values("timestamp")
        df_ctx = df_ctx.rename(columns={
            "phi_h": "phi_hat_h",
            "vx_max": "vx_max_adaptive",
            "vy_max": "vy_max_adaptive",
            "omega_max": "omega_max_adaptive"
        })
        cols_to_keep = ["timestamp", "phi_hat_h", "d_h", "d_safe", "vx_max_adaptive", "vy_max_adaptive", "omega_max_adaptive", "occlusion_flag"]
        df_ctx = df_ctx[[c for c in cols_to_keep if c in df_ctx.columns]]
        df_sync = pd.merge_asof(
            df_sync, df_ctx,
            on="timestamp",
            direction="nearest",
            tolerance=0.2
        )
    else:
        df_sync["phi_hat_h"] = 0.0
        df_sync["d_h"] = 10.0
        df_sync["d_safe"] = 0.5
        df_sync["vx_max_adaptive"] = 0.8
        df_sync["vy_max_adaptive"] = 0.5
        df_sync["omega_max_adaptive"] = 1.0
        df_sync["occlusion_flag"] = False

    if humans_file.exists():
        df_hum = pd.read_parquet(humans_file).sort_values("timestamp")
        df_hum_active = df_hum[df_hum["human_id"] != -1].copy()
        if not df_hum_active.empty:
            df_sync = pd.merge_asof(
                df_sync, df_hum_active[["timestamp", "x_h", "y_h", "vx_h", "vy_h"]],
                on="timestamp",
                direction="nearest",
                tolerance=0.2
            )
        else:
            df_sync["x_h"] = np.nan
            df_sync["y_h"] = np.nan
            df_sync["vx_h"] = np.nan
            df_sync["vy_h"] = np.nan
    else:
        df_sync["x_h"] = np.nan
        df_sync["y_h"] = np.nan
        df_sync["vx_h"] = np.nan
        df_sync["vy_h"] = np.nan

    if stats_file.exists():
        df_stats = pd.read_parquet(stats_file).sort_values("timestamp")
        df_sync = pd.merge_asof(
            df_sync, df_stats[["timestamp", "solve_time_ms", "status", "timeout_flag", "collision_flag"]],
            on="timestamp",
            direction="nearest",
            tolerance=0.2
        )
        df_sync = df_sync.rename(columns={
            "status": "solver_status"
        })
    else:
        df_sync["solve_time_ms"] = np.nan
        df_sync["solver_status"] = "unknown"
        df_sync["timeout_flag"] = False
        df_sync["collision_flag"] = False

    df_sync["x_ref"] = np.nan
    df_sync["y_ref"] = np.nan
    df_sync["theta_ref"] = np.nan
    df_sync["tracking_error_xy"] = np.nan
    df_sync["tracking_error_theta"] = np.nan

    if path_file.exists():
        try:
            df_path = pd.read_parquet(path_file)
            if not df_path.empty:
                grouped_path = df_path.groupby("timestamp")
                path_times = sorted(grouped_path.groups.keys())
                
                if path_times:
                    for i, row in df_sync.iterrows():
                        rx, ry, rtheta = row["x_r"], row["y_r"], row["theta_r"]
                        idx = np.searchsorted(path_times, row["timestamp"])
                        idx = min(len(path_times) - 1, max(0, idx))
                        best_path_t = path_times[idx]
                        
                        path_wps = grouped_path.get_group(best_path_t)
                        dists = np.sqrt((path_wps["x_ref"] - rx)**2 + (path_wps["y_ref"] - ry)**2)
                        min_idx = dists.idxmin()
                        wp = path_wps.loc[min_idx]
                        
                        df_sync.at[i, "x_ref"] = wp["x_ref"]
                        df_sync.at[i, "y_ref"] = wp["y_ref"]
                        df_sync.at[i, "theta_ref"] = wp["theta_ref"]
                        df_sync.at[i, "tracking_error_xy"] = dists.min()
                        
                        h_err = wp["theta_ref"] - rtheta
                        h_err = (h_err + np.pi) % (2 * np.pi) - np.pi
                        df_sync.at[i, "tracking_error_theta"] = h_err
        except Exception as e:
            print(f"Warning: Failed to compute reference tracking error: {e}")

    df_sync["scenario_id"] = scenario_id
    df_sync["run_id"] = run_id
    df_sync["controller_id"] = controller_id
    
    # Merge Laser Scan Clearance if exists
    scan_file = run_dir / "scan.parquet"
    if scan_file.exists():
        df_scan = pd.read_parquet(scan_file).sort_values("timestamp")
        df_sync = pd.merge_asof(
            df_sync, df_scan,
            on="timestamp",
            direction="nearest",
            tolerance=0.2
        )
        df_sync["min_costmap_clearance"] = df_sync["min_scan_clearance"]
    else:
        df_sync["min_costmap_clearance"] = np.nan

    # Calculate command jerk (derivative of acceleration)
    dt = df_sync["timestamp"].diff()
    dt_clamped = dt.where(dt > 1e-5, np.nan)
    
    ax_cmd = df_sync["vx_cmd"].diff() / dt_clamped
    ay_cmd = df_sync["vy_cmd"].diff() / dt_clamped
    alpha_cmd = df_sync["omega_cmd"].diff() / dt_clamped
    
    jx_cmd = ax_cmd.diff() / dt_clamped
    jy_cmd = ay_cmd.diff() / dt_clamped
    jtheta_cmd = alpha_cmd.diff() / dt_clamped
    
    df_sync["jerk_x"] = jx_cmd.fillna(0.0)
    df_sync["jerk_y"] = jy_cmd.fillna(0.0)
    df_sync["jerk_theta"] = jtheta_cmd.fillna(0.0)
    df_sync["jerk_magnitude"] = np.sqrt(df_sync["jerk_x"]**2 + df_sync["jerk_y"]**2)

    schema_cols = [
        "timestamp", "scenario_id", "run_id", "controller_id",
        "x_r", "y_r", "theta_r",
        "vx_cmd", "vy_cmd", "omega_cmd",
        "x_ref", "y_ref", "theta_ref",
        "tracking_error_xy", "tracking_error_theta",
        "x_h", "y_h", "vx_h", "vy_h",
        "d_h", "phi_hat_h", "d_safe",
        "vx_max_adaptive", "vy_max_adaptive", "omega_max_adaptive",
        "min_costmap_clearance", "solve_time_ms", "solver_status",
        "timeout_flag", "collision_flag", "occlusion_flag",
        "jerk_x", "jerk_y", "jerk_theta", "jerk_magnitude"
    ]
    
    final_cols = []
    for col in schema_cols:
        if col not in df_sync.columns:
            df_sync[col] = np.nan
        final_cols.append(col)
        
    df_final = df_sync[final_cols]
    
    output_file = run_dir / "controller_samples.parquet"
    df_final.to_parquet(output_file, index=False)
    print(f"Successfully synced run! Saved {len(df_final)} rows to {output_file}")
    return True

def main():
    parser = argparse.ArgumentParser(description="Synchronize multiple topics into a unified parquet dataframe")
    parser.add_argument("--run", help="Path to specific run directory to synchronize")
    parser.add_argument("--dir", help="Path to directory containing multiple runs to process recursively")
    args = parser.parse_args()
    
    if args.run:
        success = sync_run_topics(args.run)
        sys.exit(0 if success else 1)
    elif args.dir:
        run_dirs = []
        for root, dirs, files in os.walk(args.dir):
            if "odom.parquet" in files:
                run_dirs.append(root)
        print(f"Found {len(run_dirs)} runs with odom.parquet to synchronize in {args.dir}")
        success_count = 0
        for r_dir in run_dirs:
            if sync_run_topics(r_dir):
                success_count += 1
        print(f"Synchronized {success_count}/{len(run_dirs)} runs successfully")
        sys.exit(0)
    else:
        parser.print_help()
        sys.exit(1)

if __name__ == "__main__":
    main()
