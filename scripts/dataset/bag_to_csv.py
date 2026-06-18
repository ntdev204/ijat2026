#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from _common import DERIVED_HEADERS, ensure_layout, parser, print_json


def main() -> int:
    args = parser("Prepare derived CSV files from a rosbag2 run").parse_args()
    dataset = Path(args.dataset)
    ensure_layout(dataset)
    print_json({
        "dataset": str(dataset),
        "bag": args.bag,
        "derived_files": sorted(DERIVED_HEADERS.keys()),
        "message": "CSV schemas are ready. Add rosbag2_py extraction here when bag replay/parsing is available.",
    })
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
