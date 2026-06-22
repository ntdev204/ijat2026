import type { ActiveDatasetPayload } from "@/types/robot-runtime";
import type { DatasetFormState } from "@/types/dataset-runtime";

export const DEFAULT_ACTIVE: ActiveDatasetPayload = {
  active: false,
  run: null,
};

export const PAGE_SIZE = 8;

export const PIPELINE_ACTION_LABELS = {
  validate: "Validated dataset",
  bag_to_csv: "Prepared CSV export",
  metrics: "Computed metrics",
  plots: "Generated figures",
  tables: "Generated tables",
  all: "Ran full pipeline",
} as const;

export function optionalNumber(value: string): number | null {
  if (!value.trim()) {
    return null;
  }
  const numeric = Number(value);
  return Number.isFinite(numeric) ? numeric : null;
}

export function createDefaultDatasetForm(): DatasetFormState {
  return {
    scenarioName: "S1_open_zone",
    controllerId: "CCA_NMPC",
    environment: "real",
    split: "unsplit",
    runIndex: "",
    randomSeed: "",
    robotStartX: "",
    robotStartY: "",
    robotStartTheta: "",
    goalX: "",
    goalY: "",
    goalTheta: "",
    humanBehavior: "unknown",
    intervention: false,
    recordCamera: false,
    notes: "",
    captureTag: "corridor",
    captureClass: "person",
  };
}
