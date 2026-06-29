from __future__ import annotations

from dataclasses import dataclass


BASELINE_CONTROLLERS = [
    {
        "id": "CCA_NMPC",
        "label": "CCA-NMPC",
        "group": "proposed",
        "description": "Continuous Context-Aware NMPC with predictive human context",
    },
]


ABLATION_CONTROLLERS = [
    {
        "id": "NMPC",
        "label": "NMPC",
        "description": "Nominal Mecanum NMPC without human context adaptation",
    },
]


DATASET_GROUPS = [
    {
        "id": "human_motion_dataset",
        "label": "Dataset 1: Human Motion Dataset",
        "objective": "Evaluate Kalman tracking and human motion prediction.",
        "required_files": ["human.csv", "predicted_human.csv"],
    },
    {
        "id": "context_dataset",
        "label": "Dataset 2: Context Dataset",
        "objective": "Validate continuous context score phi_h and predictive context response.",
        "required_files": ["context.csv", "adaptive_constraints.csv"],
    },
    {
        "id": "navigation_dataset",
        "label": "Dataset 3: Navigation Dataset",
        "objective": "Evaluate robot navigation quality, safety, and command behavior.",
        "required_files": ["robot.csv", "context.csv", "human.csv", "solver.csv"],
    },
    {
        "id": "baseline_comparison_dataset",
        "label": "Dataset 4: Baseline Comparison Dataset",
        "objective": "Benchmark CCA-NMPC against the native nominal NMPC variant under identical scenarios without Nav2 dependencies.",
        "required_files": ["run_summary.csv"],
    },
]


SCENARIO_SPECS = [
    {
        "id": "S1_open_zone",
        "title": "Open Zone",
        "scientific_goal": "Validate nominal tracking and low-context operation when no human risk is present.",
        "layout": "Open area with wide clearance and no close pedestrian interaction.",
        "human_count": "0-1 distant humans",
        "human_speed": "0.0-0.3 m/s",
        "human_trajectory": "None or far away from robot corridor",
        "minimum_runs": 30,
        "primary_metric": "tracking rmse_xy",
        "difficulty": "easy",
    },
    {
        "id": "S2_narrow_corridor",
        "title": "Narrow Corridor",
        "scientific_goal": "Evaluate lateral Mecanum behavior and adaptive limits in constrained clearance.",
        "layout": "Corridor or aisle with limited lateral clearance.",
        "human_count": "0-1",
        "human_speed": "0.0-0.5 m/s",
        "human_trajectory": "Static side occupancy or slow parallel motion",
        "minimum_runs": 30,
        "primary_metric": "minimum_human_distance",
        "difficulty": "medium",
    },
    {
        "id": "S3_human_proximate",
        "title": "Human Proximate",
        "scientific_goal": "Show continuous increase of phi_h and adaptive safety distance near nearby humans.",
        "layout": "Open or semi-structured path with a nearby standing or slow human.",
        "human_count": "1-2",
        "human_speed": "0.0-0.4 m/s",
        "human_trajectory": "Static or slow drift near robot path",
        "minimum_runs": 40,
        "primary_metric": "mean_context_phi_h",
        "difficulty": "hard",
    },
    {
        "id": "S4_dynamic_crossing",
        "title": "Dynamic Crossing",
        "scientific_goal": "Validate predictive human context and slowdown response during crossing conflicts.",
        "layout": "Robot path intersected by lateral pedestrian crossing.",
        "human_count": "1-2",
        "human_speed": "0.4-1.2 m/s",
        "human_trajectory": "Crossing from left-to-right or right-to-left",
        "minimum_runs": 50,
        "primary_metric": "prediction_rmse",
        "difficulty": "hard",
    },
    {
        "id": "S5_occlusion",
        "title": "Occlusion",
        "scientific_goal": "Stress perception confidence, covariance-aware context, and fallback safety behavior.",
        "layout": "Corner, wall, shelf, or static occluder that hides pedestrian until late reveal.",
        "human_count": "1-2",
        "human_speed": "0.3-1.0 m/s",
        "human_trajectory": "Sudden appearance from hidden area",
        "minimum_runs": 50,
        "primary_metric": "context_response_time",
        "difficulty": "stress",
    },
    {
        "id": "S6_human_approaching",
        "title": "Human Approaching",
        "scientific_goal": "Demonstrate the effect of cos(delta_theta) by contrasting approaching and receding motion at similar distance.",
        "layout": "Frontal interaction corridor or open lane with repeated head-on and away-from-robot motion.",
        "human_count": "1",
        "human_speed": "0.5-1.2 m/s",
        "human_trajectory": "Directly approaching robot, then directly moving away under similar initial distance",
        "minimum_runs": 60,
        "primary_metric": "relative_heading_vs_phi_h",
        "difficulty": "critical",
    },
]


VIDEO_METADATA_FIELDS = [
    "scenario",
    "controller",
    "video",
    "humans",
    "environment",
    "operator",
    "run_id",
    "notes",
]


REQUIRED_FIGURES = [
    "trajectory_comparison",
    "phi_h_timeseries",
    "d_safe_timeseries",
    "v_max_timeseries",
    "human_distance_vs_phi_h",
    "prediction_error",
    "solver_distribution",
    "safety_comparison",
    "rmse_comparison",
    "adaptive_constraint_behaviour",
    "ablation_results",
]


STATISTICAL_PLAN = {
    "minimum_runs_per_condition": 30,
    "recommended_runs_per_core_scenario": 50,
    "recommended_runs_for_s6": 60,
    "reporting": "mean +- std and 95% confidence interval",
    "significance_tests": ["paired t-test when normal", "Wilcoxon signed-rank when non-normal"],
    "effect_size": ["Cohen d", "Cliff delta"],
}


def scenario_by_id(scenario_id: str) -> dict | None:
    for item in SCENARIO_SPECS:
        if item["id"] == scenario_id:
            return item
    return None
