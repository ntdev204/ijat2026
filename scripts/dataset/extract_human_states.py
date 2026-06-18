#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from _common import ensure_layout, parser, print_json


def main() -> int:
    args = parser("Extract human state and prediction CSVs from rosbag2").parse_args()
    dataset = Path(args.dataset)
    ensure_layout(dataset)
    print_json({
        "outputs": [
            str(dataset / "derived" / "human_states.csv"),
            str(dataset / "derived" / "human_prediction.csv"),
        ],
        "bag": args.bag,
        "message": "Schemas exist; implement detector/GT alignment when ground truth topics are finalized.",
    })
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
