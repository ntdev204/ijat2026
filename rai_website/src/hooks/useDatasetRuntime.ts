"use client";

import { useCallback, useEffect, useMemo, useState } from "react";
import { useOperationMode } from "@/contexts/OperationModeContext";
import { fetchWithAuth } from "@/lib/api";
import {
  createDefaultDatasetForm,
  DEFAULT_ACTIVE,
  optionalNumber,
  PAGE_SIZE,
  PIPELINE_ACTION_LABELS,
} from "@/lib/dataset-runtime";
import type { DatasetFormState, DatasetRuntime } from "@/types/dataset-runtime";
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

interface DatasetRuntimeState {
  active: ActiveDatasetPayload;
  systemRuntime: SystemRuntime | null;
  artifacts: DatasetArtifactsPayload | null;
  launchStatus: DatasetLaunchStatus | null;
  pipeline: DatasetPipelineResult | null;
  runs: DatasetRun[];
  scenarios: DatasetScenario[];
  currentPage: number;
  busy: boolean;
  message: string;
}

export function useDatasetRuntime(): DatasetRuntime {
  const { operationMode, runtime } = useOperationMode();
  const [state, setState] = useState<DatasetRuntimeState>({
    active: DEFAULT_ACTIVE,
    systemRuntime: null,
    artifacts: null,
    launchStatus: null,
    pipeline: null,
    runs: [],
    scenarios: [],
    currentPage: 1,
    busy: false,
    message: "",
  });
  const [form, setForm] = useState<DatasetFormState>(createDefaultDatasetForm);

  const patchState = useCallback((patch: Partial<DatasetRuntimeState>) => {
    setState((current) => ({ ...current, ...patch }));
  }, []);

  const setBusy = useCallback((busy: boolean) => {
    patchState({ busy });
  }, [patchState]);

  const setMessage = useCallback((message: string) => {
    patchState({ message });
  }, [patchState]);

  const patchForm = useCallback((patch: Partial<DatasetFormState>) => {
    setForm((current) => ({ ...current, ...patch }));
  }, []);

  const refresh = useCallback(async () => {
    patchState({ systemRuntime: runtime });

    if (!runtime || !runtime.allowed_actions.includes("dataset") || operationMode === "sim") {
      patchState({
        active: DEFAULT_ACTIVE,
        runs: [],
        scenarios: [],
        artifacts: null,
        launchStatus: null,
        message: !runtime
          ? "Waiting for system runtime."
          : operationMode === "sim"
            ? "Dataset controls are disabled in Simulation mode."
            : `Dataset controls are disabled on ${runtime.device_label} (${runtime.device_role}).`,
      });
      return;
    }

    const [activeRes, runsRes, scenariosRes, artifactsRes, launchRes] = await Promise.all([
      fetchWithAuth("/api/dataset/active"),
      fetchWithAuth("/api/dataset/runs"),
      fetchWithAuth("/api/dataset/scenarios"),
      fetchWithAuth("/api/dataset/artifacts"),
      fetchWithAuth("/api/dataset/launch/status"),
    ]);

    const nextScenarios = (await scenariosRes.json()) as DatasetScenario[];
    const nextRuns = (await runsRes.json()) as DatasetRun[];

    patchState({
      active: (await activeRes.json()) as ActiveDatasetPayload,
      runs: nextRuns,
      artifacts: (await artifactsRes.json()) as DatasetArtifactsPayload,
      launchStatus: (await launchRes.json()) as DatasetLaunchStatus,
      scenarios: nextScenarios,
      currentPage: Math.min(
        Math.max(1, state.currentPage),
        Math.max(1, Math.ceil(nextRuns.length / PAGE_SIZE)),
      ),
    });

    setForm((current) =>
      current.scenarioName === "S1_open_zone" && nextScenarios[0]
        ? { ...current, scenarioName: nextScenarios[0].name }
        : current,
    );
  }, [operationMode, patchState, runtime, state.currentPage]);

  useEffect(() => {
    const timer = window.setTimeout(() => {
      void refresh().catch(() => setMessage("Cannot reach rai_web_api dataset endpoints."));
    }, 0);
    return () => window.clearTimeout(timer);
  }, [refresh, setMessage]);

  const runBusyAction = useCallback(
    async (action: () => Promise<void>, fallbackMessage: string) => {
      setBusy(true);
      setMessage("");
      try {
        await action();
      } catch (error) {
        setMessage(error instanceof Error ? error.message : fallbackMessage);
      } finally {
        setBusy(false);
      }
    },
    [setBusy, setMessage],
  );

  const prepareDataset = useCallback(
    async () =>
      runBusyAction(async () => {
        const response = await fetchWithAuth("/api/dataset/prepare", { method: "POST" });
        patchState({ artifacts: (await response.json()) as DatasetArtifactsPayload });
        setMessage("Dataset folders and schema files are ready.");
        await refresh();
      }, "Cannot prepare dataset folders."),
    [patchState, refresh, runBusyAction, setMessage],
  );

  const runPipeline = useCallback(
    async (
      action: "validate" | "bag_to_csv" | "metrics" | "plots" | "tables" | "all",
      runId?: number,
    ) =>
      runBusyAction(async () => {
        const response = await fetchWithAuth("/api/dataset/pipeline", {
          method: "POST",
          body: JSON.stringify({ action, run_id: runId ?? state.active.run?.id ?? null }),
        });
        const payload = (await response.json()) as DatasetPipelineResult;
        const failed = payload.results.find((item) => item.returncode !== 0);
        patchState({ pipeline: payload, artifacts: payload.artifacts });
        setMessage(
          failed
            ? `Pipeline failed: ${failed.command.join(" ")}`
            : `${PIPELINE_ACTION_LABELS[action]}.`,
        );
        await refresh();
      }, "Cannot run dataset pipeline."),
    [patchState, refresh, runBusyAction, setMessage, state.active.run?.id],
  );

  const startLaunchStack = useCallback(
    async () =>
      runBusyAction(async () => {
        const response = await fetchWithAuth("/api/dataset/launch/start", {
          method: "POST",
          body: JSON.stringify({
            scenario_name: form.scenarioName,
            controller_id: form.controllerId,
            environment: form.environment,
            split: form.split,
            run_id: form.runIndex.trim() ? `run_${form.runIndex.trim()}` : "",
            auto_start: true,
          }),
        });
        const payload = await response.json();
        setMessage(`Launch stack started (pid ${payload.pid ?? "N/A"}).`);
        await refresh();
      }, "Cannot start dataset launch stack."),
    [form.controllerId, form.environment, form.runIndex, form.scenarioName, form.split, refresh, runBusyAction, setMessage],
  );

  const stopLaunchStack = useCallback(
    async () =>
      runBusyAction(async () => {
        await fetchWithAuth("/api/dataset/launch/stop", { method: "POST" });
        setMessage("Stopped dataset launch stack.");
        await refresh();
      }, "Cannot stop dataset launch stack."),
    [refresh, runBusyAction, setMessage],
  );

  const startDataset = useCallback(
    async () =>
      runBusyAction(async () => {
        const response = await fetchWithAuth("/api/dataset/start", {
          method: "POST",
          body: JSON.stringify({
            scenario_name: form.scenarioName,
            controller_id: form.controllerId,
            environment: form.environment,
            split: form.split,
            run_index: optionalNumber(form.runIndex),
            random_seed: optionalNumber(form.randomSeed),
            robot_start_x: optionalNumber(form.robotStartX),
            robot_start_y: optionalNumber(form.robotStartY),
            robot_start_theta: optionalNumber(form.robotStartTheta),
            goal_x: optionalNumber(form.goalX),
            goal_y: optionalNumber(form.goalY),
            goal_theta: optionalNumber(form.goalTheta),
            human_behavior: form.humanBehavior,
            intervention: form.intervention,
            record_camera: form.recordCamera,
            notes: form.notes,
          }),
        });
        const payload = await response.json();
        setMessage(
          `Started ${payload.run_name ?? "dataset run"} with ${payload.recorded_topics?.length ?? 0} topics.`,
        );
        await refresh();
      }, "Cannot start dataset run."),
    [form, refresh, runBusyAction, setMessage],
  );

  const stopDataset = useCallback(
    async () =>
      runBusyAction(async () => {
        const response = await fetchWithAuth("/api/dataset/stop", { method: "POST" });
        const payload = await response.json();
        setMessage(`Stopped run #${payload.run_id}. Compression continues in background.`);
        await refresh();
      }, "Cannot stop dataset run."),
    [refresh, runBusyAction, setMessage],
  );

  const captureLabeledSample = useCallback(
    async () =>
      runBusyAction(async () => {
        const response = await fetchWithAuth("/api/dataset/capture", {
          method: "POST",
          body: JSON.stringify({ tag: form.captureTag, class_name: form.captureClass }),
        });
        const payload = await response.json();
        setMessage(
          `Captured ${payload.filename} with tag=${payload.tag} class=${payload.class_name}.`,
        );
      }, "Cannot capture labeled sample."),
    [form.captureClass, form.captureTag, runBusyAction, setMessage],
  );

  const totalPages = useMemo(
    () => Math.max(1, Math.ceil(state.runs.length / PAGE_SIZE)),
    [state.runs.length],
  );
  const pagedRuns = useMemo(
    () => state.runs.slice((state.currentPage - 1) * PAGE_SIZE, state.currentPage * PAGE_SIZE),
    [state.currentPage, state.runs],
  );

  return {
    active: state.active,
    systemRuntime: state.systemRuntime,
    artifacts: state.artifacts,
    launchStatus: state.launchStatus,
    pipeline: state.pipeline,
    runs: state.runs,
    scenarios: state.scenarios,
    form,
    busy: state.busy,
    message: state.message,
    currentPage: state.currentPage,
    totalPages,
    pagedRuns,
    setCurrentPage: (value) => patchState({ currentPage: value }),
    setScenarioName: (value) => patchForm({ scenarioName: value }),
    setControllerId: (value) => patchForm({ controllerId: value }),
    setEnvironment: (value: RuntimeEnvironment) => patchForm({ environment: value }),
    setSplit: (value) => patchForm({ split: value }),
    setRunIndex: (value) => patchForm({ runIndex: value }),
    setRandomSeed: (value) => patchForm({ randomSeed: value }),
    setRobotStartX: (value) => patchForm({ robotStartX: value }),
    setRobotStartY: (value) => patchForm({ robotStartY: value }),
    setRobotStartTheta: (value) => patchForm({ robotStartTheta: value }),
    setGoalX: (value) => patchForm({ goalX: value }),
    setGoalY: (value) => patchForm({ goalY: value }),
    setGoalTheta: (value) => patchForm({ goalTheta: value }),
    setHumanBehavior: (value) => patchForm({ humanBehavior: value }),
    setIntervention: (value) => patchForm({ intervention: value }),
    setRecordCamera: (value) => patchForm({ recordCamera: value }),
    setNotes: (value) => patchForm({ notes: value }),
    setCaptureTag: (value) => patchForm({ captureTag: value }),
    setCaptureClass: (value) => patchForm({ captureClass: value }),
    refresh,
    prepareDataset,
    runPipeline,
    startLaunchStack,
    stopLaunchStack,
    startDataset,
    stopDataset,
    captureLabeledSample,
  };
}

export type { DatasetFormState, DatasetRuntime };
