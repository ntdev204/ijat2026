#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from _common import ensure_layout, parser, print_json


REQUIRED_TOPICS = {
    "/scan", "/scan_filtered", "/odom", "/imu/data_raw", "/tf", "/tf_static", "/cmd_vel",
    "/cmd_vel_web",
    "/cca_nmpc/cmd_vel", "/canmpc/context", "/canmpc/humans", "/canmpc/adaptive_bounds",
    "/canmpc/predicted_trajectory", "/canmpc/local_reference_path", "/canmpc/solver_stats",
    "/local_costmap/costmap", "/local_costmap/published_footprint",
}


def main() -> int:
    args = parser("Validate dataset layout and optional rosbag2 metadata").parse_args()
    dataset = Path(args.dataset)
    ensure_layout(dataset)
    bag = Path(args.bag) if args.bag else None
    warnings: list[str] = []
    topics: set[str] = set()

    if bag:
        if not bag.exists():
            warnings.append(f"bag path does not exist: {bag}")
        else:
            metadata = bag / "metadata.yaml"
            if not metadata.exists():
                warnings.append(f"rosbag metadata.yaml not found: {metadata}")
            else:
                text = metadata.read_text(encoding="utf-8", errors="replace")
                for topic in REQUIRED_TOPICS:
                    if topic in text:
                        topics.add(topic)
                missing = sorted(REQUIRED_TOPICS - topics)
                if missing:
                    warnings.append("missing required topics: " + ", ".join(missing))

    print_json({
        "dataset": str(dataset),
        "bag": str(bag) if bag else "",
        "valid": not warnings,
        "topics_found": sorted(topics),
        "warnings": warnings,
    })
    return 1 if warnings else 0


if __name__ == "__main__":
    raise SystemExit(main())
