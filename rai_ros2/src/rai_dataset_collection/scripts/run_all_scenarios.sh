#!/bin/bash
# Guided manual S1-S6 collection helper.
# It does not execute map-based trajectories. It launches the recorder for each
# required scenario label and waits for the operator to perform the scenario.

set -e

SCENARIOS=(
    "S1_open_zone"
    "S2_narrow_corridor"
    "S3_human_proximate"
    "S4_dynamic_crossing"
    "S5_occlusion"
    "S6_human_approaching"
)

RUNS_PER_SCENARIO=""
CONTROLLER="CCA_NMPC"
ENVIRONMENT="real"

declare -A SCENARIO_RUN_TARGETS=(
    ["S1_open_zone"]=30
    ["S2_narrow_corridor"]=30
    ["S3_human_proximate"]=40
    ["S4_dynamic_crossing"]=50
    ["S5_occlusion"]=50
    ["S6_human_approaching"]=60
)

while [[ $# -gt 0 ]]; do
    case $1 in
        --runs)
            RUNS_PER_SCENARIO="$2"
            shift 2
            ;;
        --controller)
            CONTROLLER="$2"
            shift 2
            ;;
        --environment)
            ENVIRONMENT="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo "  --runs N            Override runs per scenario for every scenario"
            echo "  --controller NAME   Controller id (default: CCA_NMPC)"
            echo "  --environment NAME  real | sim (default: real)"
            echo ""
            echo "This is a guided manual helper. It does not execute trajectories."
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

source /opt/ros/humble/setup.bash
source ~/rai_ros2/install/setup.bash

echo "Guided manual S1-S6 collection"
echo "Controller: $CONTROLLER"
echo "Environment: $ENVIRONMENT"
if [[ -n "$RUNS_PER_SCENARIO" ]]; then
    echo "Runs/scenario override: $RUNS_PER_SCENARIO"
else
    echo "Runs/scenario: from research spec (30/30/40/50/50/60)"
fi
echo ""

for scenario in "${SCENARIOS[@]}"; do
    echo "========================================"
    echo "Scenario: $scenario"
    echo "========================================"
    target_runs="${SCENARIO_RUN_TARGETS[$scenario]}"
    if [[ -n "$RUNS_PER_SCENARIO" ]]; then
        target_runs="$RUNS_PER_SCENARIO"
    fi
    echo "Target runs: $target_runs"
    for ((run=0; run<target_runs; run++)); do
        run_id=$(printf "run_%03d" "$run")
        echo ""
        echo "Prepare manual scenario $scenario, $run_id."
        read -r -p "Press ENTER to start recording, Ctrl+C to abort..."

        ros2 launch rai_dataset_collection dataset_collection.launch.py \
            scenario:="$scenario" \
            controller:="$CONTROLLER" \
            environment:="$ENVIRONMENT" \
            run_id:="$run_id" \
            auto_start:=true &

        launch_pid=$!
        echo "Recording. Perform $scenario now."
        read -r -p "Press ENTER to stop this run..."

        kill -SIGINT "$launch_pid" 2>/dev/null || true
        wait "$launch_pid" 2>/dev/null || true
        echo "Saved $scenario/$CONTROLLER/$run_id"
    done
done
