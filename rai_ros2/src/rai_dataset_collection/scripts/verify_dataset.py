#!/usr/bin/env python3
"""
Verify Continuous Context-Aware NMPC dataset layout and metadata.
"""

import json
import sys
from datetime import datetime
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
PACKAGE_DIR = SCRIPT_DIR.parent / "rai_dataset_collection"
if str(PACKAGE_DIR.parent) not in sys.path:
    sys.path.append(str(PACKAGE_DIR.parent))

from rai_dataset_collection.research_dataset_spec import (  # noqa: E402
    ABLATION_CONTROLLERS,
    BASELINE_CONTROLLERS,
    SCENARIO_SPECS,
)


class DatasetVerifier:
    """Verify CCA-NMPC dataset quality."""

    def __init__(self, base_path: str, environment: str = "sim", min_runs: int | None = None):
        self.base_path = Path(base_path).expanduser()
        self.raw_path = self.base_path / "raw" / environment
        self.environment = environment
        self.min_runs_override = min_runs
        self.scenario_specs = {item["id"]: item for item in SCENARIO_SPECS}
        self.scenarios = list(self.scenario_specs.keys())
        self.controllers = list(dict.fromkeys(
            [item["id"] for item in BASELINE_CONTROLLERS] +
            [item["id"] for item in ABLATION_CONTROLLERS]
        ))
        self.required_topics = [
            "/odom",
            "/cmd_vel",
            "/canmpc/context",
            "/canmpc/humans",
            "/canmpc/adaptive_bounds",
        ]
        self.required_csv = [
            "derived/robot.csv",
            "derived/human.csv",
            "derived/context.csv",
            "derived/adaptive_constraints.csv",
            "derived/solver.csv",
            "derived/predicted_human.csv",
            "derived/run_summary.csv",
        ]
        self.results = {
            "total_runs": 0,
            "valid_runs": 0,
            "scenarios": {},
            "issues": [],
            "warnings": [],
        }

    def verify_all(self) -> bool:
        print("=" * 72)
        print("CCA-NMPC Dataset Verification")
        print("=" * 72)
        print(f"Dataset path: {self.base_path}")
        print(f"Raw path:     {self.raw_path}")
        print(f"Environment:  {self.environment}")

        if not self.raw_path.exists():
            print(f"Dataset raw path does not exist: {self.raw_path}")
            return False

        for scenario in self.scenarios:
            self._verify_scenario(scenario)

        self._print_summary()
        return self.results["valid_runs"] >= len(self.scenarios) * self.min_runs_per_scenario

    def _verify_scenario(self, scenario: str):
        scenario_path = self.raw_path / scenario
        required_runs = self.min_runs_override or int(self.scenario_specs.get(scenario, {}).get("minimum_runs", 0))
        print(f"\nVerifying {scenario}...")

        if not scenario_path.exists():
            self._add_scenario_result(scenario, 0, 0, "MISSING")
            self.results["issues"].append(f"{scenario}: missing scenario directory")
            return

        run_dirs = []
        for controller in self.controllers:
            controller_dir = scenario_path / controller
            if not controller_dir.exists():
                self.results["warnings"].append(f"{scenario}: controller folder missing {controller}")
                continue
            run_dirs.extend([p for p in controller_dir.iterdir() if p.is_dir()])

        valid_count = 0
        for run_dir in sorted(run_dirs):
            self.results["total_runs"] += 1
            if self._verify_run(run_dir, scenario):
                valid_count += 1
                self.results["valid_runs"] += 1

        status = "COMPLETE" if valid_count >= required_runs else "INCOMPLETE"
        self._add_scenario_result(scenario, len(run_dirs), valid_count, status)

        print(f"  Total runs: {len(run_dirs)}")
        print(f"  Valid runs: {valid_count}/{required_runs}")
        print(f"  Status: {status}")

        if valid_count < required_runs:
            missing = required_runs - valid_count
            self.results["issues"].append(f"{scenario}: need {missing} more valid runs")

    def _add_scenario_result(self, scenario: str, total: int, valid: int, status: str):
        self.results["scenarios"][scenario] = {
            "total": total,
            "valid": valid,
            "status": status,
        }

    def _verify_run(self, run_dir: Path, scenario: str) -> bool:
        metadata_path = run_dir / "metadata.json"
        bag_path = run_dir / "rosbag2"
        if not metadata_path.exists() or not bag_path.exists():
            return False
        for relative in self.required_csv:
            if not (run_dir / relative).exists():
                self.results["issues"].append(f"{run_dir}: missing {relative}")
                return False

        try:
            metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
        except Exception:
            return False

        if metadata.get("schema_version") != "2.0.0":
            return False
        if metadata.get("research_schema_version") != "3.0.0":
            self.results["issues"].append(f"{run_dir}: research schema version missing")
            return False
        if metadata.get("flow") != "continuous_context_adaptive_ca_nmpc":
            return False
        if metadata.get("scenario_id") != scenario:
            return False
        if metadata.get("duration", 0.0) <= 0.0:
            return False

        samples_by_topic = metadata.get("samples_by_topic", {})
        missing_topics = [topic for topic in self.required_topics if samples_by_topic.get(topic, 0) <= 0]
        if missing_topics:
            self.results["issues"].append(f"{run_dir}: missing samples for {', '.join(missing_topics)}")
            return False

        context = metadata.get("continuous_context", {})
        phi_samples = context.get("phi_h_samples", [])
        if not phi_samples:
            return False
        if any(phi < 0.0 or phi > 1.0 for phi in phi_samples):
            self.results["issues"].append(f"{run_dir}: phi_h outside [0, 1]")
            return False

        bag_files = list(bag_path.glob("*.mcap")) + list(bag_path.glob("*.db3"))
        if not bag_files:
            return False
        if sum(path.stat().st_size for path in bag_files) < 100_000:
            return False

        metrics = metadata.get("metrics", {})
        if "context" not in metrics or "realtime" not in metrics:
            self.results["issues"].append(f"{run_dir}: missing context/realtime metrics")
            return False

        return True

    def _print_summary(self):
        print("\n" + "=" * 72)
        print("VERIFICATION SUMMARY")
        print("=" * 72)
        print(f"Total runs collected: {self.results['total_runs']}")
        print(f"Valid runs:           {self.results['valid_runs']}")

        print("\nPer-scenario status:")
        print(f"{'Scenario':<36} {'Collected':<10} {'Valid':<10} {'Status':<12}")
        print("-" * 72)
        for scenario in self.scenarios:
            data = self.results["scenarios"].get(scenario, {"total": 0, "valid": 0, "status": "MISSING"})
            print(f"{scenario:<36} {data['total']:<10} {data['valid']:<10} {data['status']:<12}")

        if self.results["issues"]:
            print("\nIssues:")
            for issue in self.results["issues"]:
                print(f"  - {issue}")

        if self.results["warnings"]:
            print("\nWarnings:")
            for warning in self.results["warnings"]:
                print(f"  - {warning}")

        total_required = sum(self.min_runs_override or int(self.scenario_specs.get(scenario, {}).get("minimum_runs", 0)) for scenario in self.scenarios)
        percentage = (self.results["valid_runs"] / total_required * 100.0) if total_required else 0.0
        print("\n" + "=" * 72)
        if self.results["valid_runs"] >= total_required:
            print(f"Dataset COMPLETE: {self.results['valid_runs']}/{total_required} valid runs ({percentage:.1f}%)")
        else:
            print(f"Dataset INCOMPLETE: {self.results['valid_runs']}/{total_required} valid runs ({percentage:.1f}%)")
        print("=" * 72)

    def export_report(self, output_file: str | None = None):
        if output_file is None:
            output_file = self.base_path / f"verification_report_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"

        report = {
            "verification_date": datetime.now().isoformat(),
            "dataset_path": str(self.base_path),
            "environment": self.environment,
            **self.results,
            "complete": self.results["valid_runs"] >= len(self.scenarios) * self.min_runs_per_scenario,
        }

        Path(output_file).write_text(json.dumps(report, indent=2), encoding="utf-8")
        print(f"\nReport saved: {output_file}")


def main():
    import argparse

    parser = argparse.ArgumentParser(description="Verify CCA-NMPC dataset")
    parser.add_argument("--path", default="~/rai_datasets/canmpc", help="Dataset base path")
    parser.add_argument("--environment", default="sim", choices=["sim", "real"], help="Dataset environment")
    parser.add_argument("--min-runs", type=int, default=None, help="Optional override for minimum valid runs per scenario")
    parser.add_argument("--export", action="store_true", help="Export verification report to JSON")
    args = parser.parse_args()

    verifier = DatasetVerifier(args.path, args.environment, args.min_runs)
    is_complete = verifier.verify_all()
    if args.export:
        verifier.export_report()
    sys.exit(0 if is_complete else 1)


if __name__ == "__main__":
    main()
