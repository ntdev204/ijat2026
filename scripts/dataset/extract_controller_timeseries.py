#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from _common import ensure_layout, parser, print_json


def main() -> int:
    args = parser("Extract controller time series from rosbag2 into derived/controller_timeseries.csv").parse_args()
    dataset = Path(args.dataset)
    ensure_layout(dataset)
    print_json({
        "output": str(dataset / "derived" / "controller_timeseries.csv"),
        "bag": args.bag,
        "message": "Schema exists; implement rosbag2_py extraction for production batch export.",
    })
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
