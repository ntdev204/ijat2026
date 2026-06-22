"use client";

import { useMemo } from "react";
import { resolveApiEndpoint } from "@/lib/api";
import { formatDateTime, formatNumber, formatPercent } from "@/lib/format";
import type { DatasetPageState, DatasetPageViewModelInput } from "@/types/dataset";

const SPLIT_OPTIONS = [
  { value: "unsplit", label: "unsplit" },
  { value: "train", label: "train" },
  { value: "val", label: "val" },
  { value: "test", label: "test" },
];

const ENVIRONMENT_OPTIONS = [
  { value: "real", label: "real" },
  { value: "sim", label: "sim" },
];

const CSV_ARTIFACTS = [
  "robot.csv",
  "human.csv",
  "context.csv",
  "adaptive_constraints.csv",
  "solver.csv",
  "predicted_human.csv",
  "run_summary.csv",
];

export function useDatasetPageState({
  artifacts,
  scenarios,
  runs,
  scenarioName,
  systemAllowedActions,
}: DatasetPageViewModelInput): DatasetPageState {
  return useMemo(() => {
    const files = artifacts?.files ?? {};
    const artifactStatusItems = Object.entries(files).map(([file, ready]) => ({
      file,
      ready,
    }));
    const artifactReady = artifactStatusItems.filter((item) => item.ready).length;
    const artifactTotal = artifactStatusItems.length;

    const mergedControllers = [
      ...(artifacts?.baseline_controllers ?? []),
      ...(artifacts?.ablation_controllers ?? []),
    ];
    const controllerOptions = mergedControllers
      .filter(
        (item, index, array) =>
          array.findIndex((candidate) => candidate.id === item.id) === index,
      )
      .map((item) => ({
        value: item.id,
        label: item.label || item.id,
      }));

    const selectedScenario =
      scenarios.find((scenario) => scenario.name === scenarioName) ??
      scenarios[0] ??
      null;

    const selectedScenarioSummary = selectedScenario
      ? {
          scientificGoal:
            selectedScenario.scientific_goal ||
            selectedScenario.description ||
            "N/A",
          layout: selectedScenario.layout || "N/A",
          humanCount: selectedScenario.human_count || "N/A",
          humanSpeed: selectedScenario.human_speed || "N/A",
          primaryMetric: selectedScenario.primary_metric || "N/A",
        }
      : null;

    const pagedRunRows = runs.map((run) => ({
      ...run,
      startedAtLabel: formatDateTime(run.start_time),
      durationLabel: `${formatNumber(run.duration, 1)} s`,
      clearanceLabel: `${formatNumber(run.min_human_clearance, 3)} m`,
      timeoutLabel: formatPercent(run.timeout_rate),
      downloadArtifacts: [
        {
          label: "ZIP",
          href: resolveApiEndpoint(`/api/dataset/download/${run.id}`),
        },
        ...CSV_ARTIFACTS.map((file) => ({
          label: file.replace(".csv", ""),
          href: resolveApiEndpoint(
            `/api/dataset/runs/${run.id}/download/derived/${file}`,
          ),
        })),
        {
          label: "metadata",
          href: resolveApiEndpoint(
            `/api/dataset/runs/${run.id}/download/metadata.json`,
          ),
        },
      ],
    }));

    return {
      datasetAllowed: systemAllowedActions?.includes("dataset") ?? true,
      artifactReady,
      artifactTotal,
      controllerOptions,
      environmentOptions: ENVIRONMENT_OPTIONS,
      splitOptions: SPLIT_OPTIONS,
      selectedScenarioSummary,
      artifactStatusItems,
      pagedRunRows,
    };
  }, [artifacts, runs, scenarioName, scenarios, systemAllowedActions]);
}
