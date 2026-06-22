import type {
  ActiveDatasetPayload,
  DatasetArtifactsPayload,
  DatasetLaunchStatus,
  DatasetPipelineResult,
  DatasetRun,
  DatasetScenario,
  RuntimeEnvironment,
  SystemRuntime,
} from "@/types/robot-runtime";

export interface DatasetFormState {
  scenarioName: string;
  controllerId: string;
  environment: RuntimeEnvironment;
  split: string;
  runIndex: string;
  randomSeed: string;
  robotStartX: string;
  robotStartY: string;
  robotStartTheta: string;
  goalX: string;
  goalY: string;
  goalTheta: string;
  humanBehavior: string;
  intervention: boolean;
  recordCamera: boolean;
  notes: string;
  captureTag: string;
  captureClass: string;
}

export interface DatasetRuntime {
  active: ActiveDatasetPayload;
  systemRuntime: SystemRuntime | null;
  artifacts: DatasetArtifactsPayload | null;
  launchStatus: DatasetLaunchStatus | null;
  pipeline: DatasetPipelineResult | null;
  runs: DatasetRun[];
  scenarios: DatasetScenario[];
  form: DatasetFormState;
  busy: boolean;
  message: string;
  currentPage: number;
  totalPages: number;
  pagedRuns: DatasetRun[];
  setCurrentPage: (value: number) => void;
  setScenarioName: (value: string) => void;
  setControllerId: (value: string) => void;
  setEnvironment: (value: RuntimeEnvironment) => void;
  setSplit: (value: string) => void;
  setRunIndex: (value: string) => void;
  setRandomSeed: (value: string) => void;
  setRobotStartX: (value: string) => void;
  setRobotStartY: (value: string) => void;
  setRobotStartTheta: (value: string) => void;
  setGoalX: (value: string) => void;
  setGoalY: (value: string) => void;
  setGoalTheta: (value: string) => void;
  setHumanBehavior: (value: string) => void;
  setIntervention: (value: boolean) => void;
  setRecordCamera: (value: boolean) => void;
  setNotes: (value: string) => void;
  setCaptureTag: (value: string) => void;
  setCaptureClass: (value: string) => void;
  refresh: () => Promise<void>;
  prepareDataset: () => Promise<void>;
  runPipeline: (
    action: "validate" | "bag_to_csv" | "metrics" | "plots" | "tables" | "all",
    runId?: number,
  ) => Promise<void>;
  startLaunchStack: () => Promise<void>;
  stopLaunchStack: () => Promise<void>;
  startDataset: () => Promise<void>;
  stopDataset: () => Promise<void>;
  captureLabeledSample: () => Promise<void>;
}
