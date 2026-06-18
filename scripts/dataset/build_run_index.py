#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from _common import RUN_INDEX_FIELDS, ensure_layout, metadata_to_run_index_row, parser, print_json, write_csv


def main() -> int:
    args = parser("Build dataset/metadata/run_index.csv from run metadata.json files").parse_args()
    dataset = Path(args.dataset)
    ensure_layout(dataset)
    rows = []
    for metadata_path in sorted((dataset / "raw").glob("**/metadata.json")):
        try:
            rows.append(metadata_to_run_index_row(metadata_path))
        except Exception as exc:
            rows.append({"run_id": metadata_path.parent.name, "notes": f"metadata parse error: {exc}"})
    output = dataset / "metadata" / "run_index.csv"
    write_csv(output, RUN_INDEX_FIELDS, rows)
    print_json({"output": str(output), "rows": len(rows)})
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
