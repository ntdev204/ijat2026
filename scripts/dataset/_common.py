#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path


RUN_INDEX_FIELDS = [
    "run_id", "scenario_id", "controller_id", "environment", "start_time", "end_time",
    "duration_sec", "bag_path", "success", "collision", "timeout", "intervention",
    "random_seed", "robot_start_x", "robot_start_y", "robot_start_theta", "goal_x",
    "goal_y", "goal_theta", "human_behavior", "notes",
]

DERIVED_HEADERS = {
    "aggregates/controller_timeseries.csv": [
        "timestamp", "run_id", "scenario_id", "controller_id", "x_r", "y_r", "theta_r",
        "x_ref", "y_ref", "theta_ref", "tracking_error_xy", "tracking_error_theta",
        "vx_cmd", "vy_cmd", "omega_cmd", "vx_odom", "vy_odom", "omega_odom", "d_h",
        "phi_h", "d_safe", "vx_max_adaptive", "vy_max_adaptive", "omega_max_adaptive",
        "q_x_adaptive", "q_y_adaptive", "q_theta_adaptive", "sample_count",
        "evaluation_time_ms", "controller_status", "timeout_flag", "collision_flag",
        "occlusion_flag",
    ],
    "aggregates/human_states.csv": [
        "timestamp", "run_id", "scenario_id", "human_id", "x_h", "y_h", "vx_h", "vy_h",
        "confidence", "age_sec", "cov_x", "cov_y", "cov_vx", "cov_vy", "x_h_gt",
        "y_h_gt", "vx_h_gt", "vy_h_gt",
    ],
    "aggregates/human_prediction.csv": [
        "timestamp", "run_id", "scenario_id", "human_id", "horizon_i", "x_h_pred",
        "y_h_pred", "x_h_gt", "y_h_gt", "prediction_error",
    ],
    "aggregates/adaptation_timeseries.csv": [
        "timestamp", "run_id", "scenario_id", "controller_id", "d_h", "phi_h", "d_safe",
        "vx_max_adaptive", "vy_max_adaptive", "omega_max_adaptive", "q_x_adaptive",
        "q_y_adaptive", "q_theta_adaptive", "vx_cmd", "vy_cmd", "omega_cmd",
        "occlusion_flag",
    ],
    "aggregates/metrics_per_run.csv": [
        "run_id", "scenario_id", "controller_id", "environment", "success", "collision",
        "timeout", "duration_sec", "rmse_xy", "rmse_theta", "max_lateral_error", "d_min",
        "d_avg", "d_5percentile", "violation_count", "violation_duration",
        "collision_count", "jerk_mean", "jerk_max", "mean_abs_delta_u", "max_abs_delta_u",
        "control_effort", "mean_abs_vx", "mean_abs_vy", "mean_abs_omega",
        "solve_time_mean_ms", "solve_time_median_ms", "solve_time_p95_ms",
        "solve_time_max_ms", "timeout_rate",
    ],
    "aggregates/metrics_summary.csv": [
        "scenario_id", "controller_id", "environment", "n_runs", "success_rate",
        "collision_rate", "timeout_rate_mean", "rmse_xy_mean", "rmse_xy_std",
        "d_min_mean", "d_min_std", "solve_time_p95_mean_ms",
    ],
}


def parser(description: str) -> argparse.ArgumentParser:
    arg_parser = argparse.ArgumentParser(description=description)
    arg_parser.add_argument("--dataset", default="dataset", help="Dataset root directory")
    arg_parser.add_argument("--bag", default="", help="Optional rosbag2 directory")
    return arg_parser


def ensure_layout(dataset: Path) -> None:
    for relative in [
        "raw/sim", "raw/real", "derived", "derived/aggregates", "metadata/calibration", "metadata/splits",
        "figures/system", "figures/trajectories", "figures/timeseries", "figures/boxplots",
        "figures/latency", "figures/ablation", "tables/csv", "tables/latex",
    ]:
        (dataset / relative).mkdir(parents=True, exist_ok=True)
    ensure_csv(dataset / "metadata" / "run_index.csv", RUN_INDEX_FIELDS)
    for filename, fields in DERIVED_HEADERS.items():
        ensure_csv(dataset / "derived" / filename, fields)


def ensure_csv(path: Path, fields: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists() and path.stat().st_size > 0:
        return
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()


def read_csv(path: Path) -> list[dict[str, str]]:
    if not path.exists():
        return []
    with path.open("r", encoding="utf-8", newline="") as handle:
        return list(csv.DictReader(handle))


def write_csv(path: Path, fields: list[str], rows: list[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        for row in rows:
            writer.writerow({field: row.get(field, "") for field in fields})


def metadata_to_run_index_row(metadata_path: Path) -> dict[str, object]:
    metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
    robot_start = metadata.get("robot_start") or {}
    goal = metadata.get("goal") or {}
    return {
        "run_id": metadata.get("run_id", ""),
        "scenario_id": metadata.get("scenario_id", ""),
        "controller_id": metadata.get("controller_id", ""),
        "environment": metadata.get("environment", ""),
        "start_time": metadata.get("start_time", ""),
        "end_time": metadata.get("end_time", ""),
        "duration_sec": metadata.get("duration_sec", ""),
        "bag_path": metadata.get("bag_path", ""),
        "success": metadata.get("success", ""),
        "collision": metadata.get("collision", ""),
        "timeout": metadata.get("timeout", ""),
        "intervention": metadata.get("intervention", ""),
        "random_seed": metadata.get("random_seed", ""),
        "robot_start_x": robot_start.get("x", ""),
        "robot_start_y": robot_start.get("y", ""),
        "robot_start_theta": robot_start.get("theta", ""),
        "goal_x": goal.get("x", ""),
        "goal_y": goal.get("y", ""),
        "goal_theta": goal.get("theta", ""),
        "human_behavior": metadata.get("human_behavior", ""),
        "notes": metadata.get("notes", ""),
    }


def print_json(payload: dict[str, object]) -> None:
    print(json.dumps(payload, indent=2, sort_keys=True))
