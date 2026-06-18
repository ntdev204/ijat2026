#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys

sys.path.append(str(Path(__file__).resolve().parents[1] / "dataset"))
from _common import ensure_layout, parser, print_json, read_csv  # noqa: E402


def main() -> int:
    args = parser("Create CSV and LaTeX metric tables").parse_args()
    dataset = Path(args.dataset)
    ensure_layout(dataset)
    rows = read_csv(dataset / "derived" / "metrics_per_run.csv")
    csv_output = dataset / "tables" / "csv" / "metrics_per_run.csv"
    tex_output = dataset / "tables" / "latex" / "metrics_summary.tex"
    csv_output.write_text((dataset / "derived" / "metrics_per_run.csv").read_text(encoding="utf-8"), encoding="utf-8")
    tex_output.write_text(
        "% Auto-generated table placeholder. Fill after metrics_per_run.csv has numeric rows.\n"
        "\\begin{tabular}{lll}\nScenario & Controller & Runs \\\\\n"
        f"all & all & {max(0, len(rows))} \\\\\n"
        "\\end{tabular}\n",
        encoding="utf-8",
    )
    print_json({"csv": str(csv_output), "latex": str(tex_output), "rows": len(rows)})
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
