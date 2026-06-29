#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys

sys.path.append(str(Path(__file__).resolve().parents[1] / "dataset"))
from _common import ensure_layout, parser, print_json  # noqa: E402


def main() -> int:
    args = parser("Create controller time-series figures").parse_args()
    dataset = Path(args.dataset)
    ensure_layout(dataset)
    output = dataset / "figures" / "timeseries" / "README.txt"
    output.write_text("Run bag_to_csv/extract_metrics after collecting bags; plotting hooks are ready.\n", encoding="utf-8")
    print_json({"output": str(output)})
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
