import type {
  DatasetArtifactsPayload,
  DatasetRun,
  DatasetScenario,
  RuntimeEnvironment,
} from "@/types/robot-runtime";

export interface SelectOption {
  value: string;
  label: string;
}

export interface DatasetDownloadArtifact {
  label: string;
  href: string;
}

export interface DatasetRunRow extends DatasetRun {
  startedAtLabel: string;
  durationLabel: string;
  clearanceLabel: string;
  timeoutLabel: string;
  downloadArtifacts: DatasetDownloadArtifact[];
}

export interface DatasetScenarioSummary {
  scientificGoal: string;
  layout: string;
  humanCount: string;
  humanSpeed: string;
  primaryMetric: string;
}

export interface DatasetArtifactStatusItem {
  file: string;
  ready: boolean;
}

export interface DatasetPageState {
  datasetAllowed: boolean;
  artifactReady: number;
  artifactTotal: number;
  controllerOptions: SelectOption[];
  environmentOptions: SelectOption[];
  splitOptions: SelectOption[];
  selectedScenarioSummary: DatasetScenarioSummary | null;
  artifactStatusItems: DatasetArtifactStatusItem[];
  pagedRunRows: DatasetRunRow[];
}

export type DatasetPipelineAction =
  | "validate"
  | "bag_to_csv"
  | "metrics"
  | "plots"
  | "tables"
  | "all";

export interface DatasetPageViewModelInput {
  artifacts: DatasetArtifactsPayload | null;
  scenarios: DatasetScenario[];
  runs: DatasetRun[];
  currentPage: number;
  scenarioName: string;
  environment: RuntimeEnvironment;
  systemAllowedActions?: string[];
}
