"use client";

import { useCallback, useEffect, useState } from "react";
import { fetchWithAuth } from "@/lib/api";
import type {
  ActiveDatasetPayload,
  DatasetRun,
  DatasetScenario,
  RuntimeEnvironment,
} from "@/types/robot-runtime";

export interface DatasetFormState {
  scenarioName: string;
  controllerId: string;
  environment: RuntimeEnvironment;
  split: string;
  captureTag: string;
  captureClass: string;
}

export interface DatasetRuntime {
  active: ActiveDatasetPayload;
  runs: DatasetRun[];
  scenarios: DatasetScenario[];
  form: DatasetFormState;
  busy: boolean;
  message: string;
  setScenarioName: (value: string) => void;
  setControllerId: (value: string) => void;
  setEnvironment: (value: RuntimeEnvironment) => void;
  setSplit: (value: string) => void;
  setCaptureTag: (value: string) => void;
  setCaptureClass: (value: string) => void;
  refresh: () => Promise<void>;
  startDataset: () => Promise<void>;
  stopDataset: () => Promise<void>;
  captureLabeledSample: () => Promise<void>;
}

const DEFAULT_ACTIVE: ActiveDatasetPayload = { active: false, run: null };

export function useDatasetRuntime(): DatasetRuntime {
  const [active, setActive] = useState<ActiveDatasetPayload>(DEFAULT_ACTIVE);
  const [runs, setRuns] = useState<DatasetRun[]>([]);
  const [scenarios, setScenarios] = useState<DatasetScenario[]>([]);
  const [scenarioName, setScenarioName] = useState("S1_open_zone");
  const [controllerId, setControllerId] = useState("CCA_NMPC");
  const [environment, setEnvironment] = useState<RuntimeEnvironment>("real");
  const [split, setSplit] = useState("unsplit");
  const [captureTag, setCaptureTag] = useState("corridor");
  const [captureClass, setCaptureClass] = useState("person");
  const [busy, setBusy] = useState(false);
  const [message, setMessage] = useState("");

  const refresh = useCallback(async () => {
    const [activeRes, runsRes, scenariosRes] = await Promise.all([
      fetchWithAuth("/api/dataset/active"),
      fetchWithAuth("/api/dataset/runs"),
      fetchWithAuth("/api/dataset/scenarios"),
    ]);
    const nextScenarios = (await scenariosRes.json()) as DatasetScenario[];
    setActive((await activeRes.json()) as ActiveDatasetPayload);
    setRuns((await runsRes.json()) as DatasetRun[]);
    setScenarios(nextScenarios);
    setScenarioName((current) => (current === "S1_open_zone" && nextScenarios[0] ? nextScenarios[0].name : current));
  }, []);

  useEffect(() => {
    const timer = window.setTimeout(() => {
      void refresh().catch(() => setMessage("Cannot reach rai_web_api dataset endpoints."));
    }, 0);
    return () => window.clearTimeout(timer);
  }, [refresh]);

  const startDataset = useCallback(async () => {
    setBusy(true);
    setMessage("");
    try {
      const response = await fetchWithAuth("/api/dataset/start", {
        method: "POST",
        body: JSON.stringify({ scenario_name: scenarioName, controller_id: controllerId, environment, split }),
      });
      const payload = await response.json();
      setMessage(`Started ${payload.run_name ?? "dataset run"}.`);
      await refresh();
    } catch (error) {
      setMessage(error instanceof Error ? error.message : "Cannot start dataset run.");
    } finally {
      setBusy(false);
    }
  }, [controllerId, environment, refresh, scenarioName, split]);

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
    runs,
    scenarios,
    form: { scenarioName, controllerId, environment, split, captureTag, captureClass },
    busy,
    message,
    setScenarioName,
    setControllerId,
    setEnvironment,
    setSplit,
    setCaptureTag,
    setCaptureClass,
    refresh,
    startDataset,
    stopDataset,
    captureLabeledSample,
  };
}
