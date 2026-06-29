#!/usr/bin/env python3
from __future__ import annotations

import math
from pathlib import Path

from _common import DERIVED_HEADERS, ensure_layout, parser, print_json, read_csv, write_csv


def _float(value: str) -> float:
    try:
        return float(value)
    except (TypeError, ValueError):
        return math.nan


def main() -> int:
    args = parser("Compute metrics_per_run.csv and metrics_summary.csv from derived/aggregates time series").parse_args()
    dataset = Path(args.dataset)
    ensure_layout(dataset)
    run_index = read_csv(dataset / "metadata" / "run_index.csv")
    controller_rows = read_csv(dataset / "derived" / "aggregates" / "controller_timeseries.csv")
    by_run: dict[str, list[dict[str, str]]] = {}
    for row in controller_rows:
        by_run.setdefault(row.get("run_id", ""), []).append(row)

    metrics = []
    for run in run_index:
        rows = by_run.get(run.get("run_id", ""), [])
        d_values = [_float(row.get("d_h", "")) for row in rows]
        d_values = [value for value in d_values if math.isfinite(value)]
        solve_values = [_float(row.get("evaluation_time_ms", "")) for row in rows]
        solve_values = sorted(value for value in solve_values if math.isfinite(value))
        timeout_flags = [row.get("timeout_flag") in {"1", "true", "True"} for row in rows]
        metrics.append({
            **{key: run.get(key, "") for key in ["run_id", "scenario_id", "controller_id", "environment", "success", "collision", "timeout", "duration_sec"]},
            "d_min": min(d_values) if d_values else "",
            "d_avg": sum(d_values) / len(d_values) if d_values else "",
            "solve_time_mean_ms": sum(solve_values) / len(solve_values) if solve_values else "",
            "solve_time_p95_ms": solve_values[int(0.95 * (len(solve_values) - 1))] if solve_values else "",
            "timeout_rate": sum(timeout_flags) / len(timeout_flags) if timeout_flags else "",
        })

    write_csv(dataset / "derived" / "aggregates" / "metrics_per_run.csv", DERIVED_HEADERS["aggregates/metrics_per_run.csv"], metrics)
    write_csv(dataset / "derived" / "aggregates" / "metrics_summary.csv", DERIVED_HEADERS["aggregates/metrics_summary.csv"], [])
    print_json({"metrics_per_run": len(metrics), "note": "Empty fields mean no extracted time-series rows were available."})
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
