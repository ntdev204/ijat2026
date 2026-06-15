#!/bin/bash
# Start one CCA-NMPC dataset collection run.

set -e

SCENARIO="S1_open_zone"
CONTROLLER="CCA_NMPC"
ENVIRONMENT="real"
SPLIT="unsplit"
RUN_ID=""
AUTO_START="true"

while [[ $# -gt 0 ]]; do
    case $1 in
        --scenario)
            SCENARIO="$2"
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
        --split)
            SPLIT="$2"
            shift 2
            ;;
        --run-id)
            RUN_ID="$2"
            shift 2
            ;;
        --no-auto-start)
            AUTO_START="false"
            shift
            ;;
        --help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  --scenario NAME       S1_open_zone | S2_narrow_corridor | S3_human_proximate | S4_dynamic_crossing | S5_occlusion_sudden_appearance"
            echo "  --controller NAME     CCA_NMPC | DISCRETE_CA_NMPC | A_STAR_NMPC | A_STAR_TEB | A_STAR_DWA"
            echo "  --environment NAME    real | sim"
            echo "  --split NAME          train | val | test | unsplit"
            echo "  --run-id NAME         Optional deterministic id, e.g. run_000"
            echo "  --no-auto-start       Launch without starting recorder"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

source /opt/ros/humble/setup.bash
source ~/rai_ros2/install/setup.bash

DATASET_DIR=~/rai_datasets/canmpc
mkdir -p "$DATASET_DIR"

echo ""
echo "Starting CCA-NMPC dataset collection"
echo "Scenario:    $SCENARIO"
echo "Controller:  $CONTROLLER"
echo "Environment: $ENVIRONMENT"
echo "Split:       $SPLIT"
echo "Auto-start:  $AUTO_START"
echo "Output:      $DATASET_DIR/raw/$ENVIRONMENT/$SCENARIO/$CONTROLLER"
echo ""

ros2 launch rai_dataset_collection dataset_collection.launch.py \
    scenario:="$SCENARIO" \
    controller:="$CONTROLLER" \
    environment:="$ENVIRONMENT" \
    split:="$SPLIT" \
    run_id:="$RUN_ID" \
    auto_start:="$AUTO_START"
