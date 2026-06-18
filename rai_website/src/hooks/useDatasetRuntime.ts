"use client";

import { useCallback, useEffect, useState } from "react";
import { fetchWithAuth } from "@/lib/api";
import type {
  ActiveDatasetPayload,
  DatasetArtifactsPayload,
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
  pipeline: DatasetPipelineResult | null;
  runs: DatasetRun[];
  scenarios: DatasetScenario[];
  form: DatasetFormState;
  busy: boolean;
  message: string;
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
  runPipeline: (action: "validate" | "bag_to_csv" | "metrics" | "plots" | "tables" | "all", runId?: number) => Promise<void>;
  startDataset: () => Promise<void>;
  stopDataset: () => Promise<void>;
  captureLabeledSample: () => Promise<void>;
}

const DEFAULT_ACTIVE: ActiveDatasetPayload = { active: false, run: null };
const PIPELINE_ACTION_LABELS = {
  validate: "Validated dataset",
  bag_to_csv: "Prepared CSV export",
  metrics: "Computed metrics",
  plots: "Generated figures",
  tables: "Generated tables",
  all: "Ran full pipeline",
} as const;

function optionalNumber(value: string): number | null {
  if (!value.trim()) {
    return null;
  }
  const numeric = Number(value);
  return Number.isFinite(numeric) ? numeric : null;
}

export function useDatasetRuntime(): DatasetRuntime {
  const [active, setActive] = useState<ActiveDatasetPayload>(DEFAULT_ACTIVE);
  const [systemRuntime, setSystemRuntime] = useState<SystemRuntime | null>(null);
  const [artifacts, setArtifacts] = useState<DatasetArtifactsPayload | null>(null);
  const [pipeline, setPipeline] = useState<DatasetPipelineResult | null>(null);
  const [runs, setRuns] = useState<DatasetRun[]>([]);
  const [scenarios, setScenarios] = useState<DatasetScenario[]>([]);
  const [scenarioName, setScenarioName] = useState("S1_open_zone");
  const [controllerId, setControllerId] = useState("CCA_NMPC");
  const [environment, setEnvironment] = useState<RuntimeEnvironment>("real");
  const [split, setSplit] = useState("unsplit");
  const [runIndex, setRunIndex] = useState("");
  const [randomSeed, setRandomSeed] = useState("");
  const [robotStartX, setRobotStartX] = useState("");
  const [robotStartY, setRobotStartY] = useState("");
  const [robotStartTheta, setRobotStartTheta] = useState("");
  const [goalX, setGoalX] = useState("");
  const [goalY, setGoalY] = useState("");
  const [goalTheta, setGoalTheta] = useState("");
  const [humanBehavior, setHumanBehavior] = useState("unknown");
  const [intervention, setIntervention] = useState(false);
  const [recordCamera, setRecordCamera] = useState(false);
  const [notes, setNotes] = useState("");
  const [captureTag, setCaptureTag] = useState("corridor");
  const [captureClass, setCaptureClass] = useState("person");
  const [busy, setBusy] = useState(false);
  const [message, setMessage] = useState("");

  const refresh = useCallback(async () => {
    const runtimeRes = await fetchWithAuth("/api/system/runtime");
    const runtime = (await runtimeRes.json()) as SystemRuntime;
    setSystemRuntime(runtime);
    if (!runtime.allowed_actions.includes("dataset")) {
      setActive(DEFAULT_ACTIVE);
      setRuns([]);
      setScenarios([]);
      setArtifacts(null);
      setMessage(`Dataset controls are disabled on ${runtime.device_label} (${runtime.device_role}).`);
      return;
    }

    const [activeRes, runsRes, scenariosRes, artifactsRes] = await Promise.all([
      fetchWithAuth("/api/dataset/active"),
      fetchWithAuth("/api/dataset/runs"),
      fetchWithAuth("/api/dataset/scenarios"),
      fetchWithAuth("/api/dataset/artifacts"),
    ]);
    const nextScenarios = (await scenariosRes.json()) as DatasetScenario[];
    setActive((await activeRes.json()) as ActiveDatasetPayload);
    setRuns((await runsRes.json()) as DatasetRun[]);
    setArtifacts((await artifactsRes.json()) as DatasetArtifactsPayload);
    setScenarios(nextScenarios);
    setScenarioName((current) => (current === "S1_open_zone" && nextScenarios[0] ? nextScenarios[0].name : current));
  }, []);

  useEffect(() => {
    const timer = window.setTimeout(() => {
      void refresh().catch(() => setMessage("Cannot reach rai_web_api dataset endpoints."));
    }, 0);
    return () => window.clearTimeout(timer);
  }, [refresh]);

  const prepareDataset = useCallback(async () => {
    setBusy(true);
    setMessage("");
    try {
      const response = await fetchWithAuth("/api/dataset/prepare", { method: "POST" });
      setArtifacts((await response.json()) as DatasetArtifactsPayload);
      setMessage("Dataset folders and schema files are ready.");
      await refresh();
    } catch (error) {
      setMessage(error instanceof Error ? error.message : "Cannot prepare dataset folders.");
    } finally {
      setBusy(false);
    }
  }, [refresh]);

  const runPipeline = useCallback(async (
    action: "validate" | "bag_to_csv" | "metrics" | "plots" | "tables" | "all",
    runId?: number,
  ) => {
    setBusy(true);
    setMessage("");
    try {
      const response = await fetchWithAuth("/api/dataset/pipeline", {
        method: "POST",
        body: JSON.stringify({ action, run_id: runId ?? active.run?.id ?? null }),
      });
      const payload = (await response.json()) as DatasetPipelineResult;
      setPipeline(payload);
      setArtifacts(payload.artifacts);
      const failed = payload.results.find((item) => item.returncode !== 0);
      setMessage(failed ? `Pipeline failed: ${failed.command.join(" ")}` : `${PIPELINE_ACTION_LABELS[action]}.`);
      await refresh();
    } catch (error) {
      setMessage(error instanceof Error ? error.message : "Cannot run dataset pipeline.");
    } finally {
      setBusy(false);
    }
  }, [active.run?.id, refresh]);

  const startDataset = useCallback(async () => {
    setBusy(true);
    setMessage("");
    try {
      const response = await fetchWithAuth("/api/dataset/start", {
        method: "POST",
        body: JSON.stringify({
          scenario_name: scenarioName,
          controller_id: controllerId,
          environment,
          split,
          run_index: optionalNumber(runIndex),
          random_seed: optionalNumber(randomSeed),
          robot_start_x: optionalNumber(robotStartX),
          robot_start_y: optionalNumber(robotStartY),
          robot_start_theta: optionalNumber(robotStartTheta),
          goal_x: optionalNumber(goalX),
          goal_y: optionalNumber(goalY),
          goal_theta: optionalNumber(goalTheta),
          human_behavior: humanBehavior,
          intervention,
          record_camera: recordCamera,
          notes,
        }),
      });
      const payload = await response.json();
      setMessage(`Started ${payload.run_name ?? "dataset run"} with ${payload.recorded_topics?.length ?? 0} topics.`);
      await refresh();
    } catch (error) {
      setMessage(error instanceof Error ? error.message : "Cannot start dataset run.");
    } finally {
      setBusy(false);
    }
  }, [
    controllerId,
    environment,
    goalTheta,
    goalX,
    goalY,
    humanBehavior,
    intervention,
    notes,
    randomSeed,
    recordCamera,
    refresh,
    robotStartTheta,
    robotStartX,
    robotStartY,
    runIndex,
    scenarioName,
    split,
  ]);

  const stopDataset = useCallback(async () => {
    setBusy(true);
    setMessage("");
    try {
      const response = await fetchWithAuth("/api/dataset/stop", { method: "POST" });
      const payload = await response.json();
      setMessage(`Stopped run #${payload.run_id}. Compression continues in background.`);
      await refresh();
    } catch (error) {
      setMessage(error instanceof Error ? error.message : "Cannot stop dataset run.");
    } finally {
      setBusy(false);
    }
  }, [refresh]);

  const captureLabeledSample = useCallback(async () => {
    setBusy(true);
    setMessage("");
    try {
      const response = await fetchWithAuth("/api/dataset/capture", {
        method: "POST",
        body: JSON.stringify({ tag: captureTag, class_name: captureClass }),
      });
      const payload = await response.json();
      setMessage(`Captured ${payload.filename} with tag=${payload.tag} class=${payload.class_name}.`);
    } catch (error) {
      setMessage(error instanceof Error ? error.message : "Cannot capture labeled sample.");
    } finally {
      setBusy(false);
    }
  }, [captureClass, captureTag]);

  return {
    active,
    systemRuntime,
    artifacts,
    pipeline,
    runs,
    scenarios,
    form: {
      scenarioName,
      controllerId,
      environment,
      split,
      runIndex,
      randomSeed,
      robotStartX,
      robotStartY,
      robotStartTheta,
      goalX,
      goalY,
      goalTheta,
      humanBehavior,
      intervention,
      recordCamera,
      notes,
      captureTag,
      captureClass,
    },
    busy,
    message,
    setScenarioName,
    setControllerId,
    setEnvironment,
    setSplit,
    setRunIndex,
    setRandomSeed,
    setRobotStartX,
    setRobotStartY,
    setRobotStartTheta,
    setGoalX,
    setGoalY,
    setGoalTheta,
    setHumanBehavior,
    setIntervention,
    setRecordCamera,
    setNotes,
    setCaptureTag,
    setCaptureClass,
    refresh,
    prepareDataset,
    runPipeline,
    startDataset,
    stopDataset,
    captureLabeledSample,
  };
}
