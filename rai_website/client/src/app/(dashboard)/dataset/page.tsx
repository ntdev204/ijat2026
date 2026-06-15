"use client";

import { StatusBadge } from "@/components/ui/StatusBadge";
import { fetchWithAuth, resolveApiEndpoint } from "@/lib/api";
import { Database, Download, Play, RefreshCw, Square } from "lucide-react";
import { useCallback, useEffect, useState } from "react";

interface DatasetRun {
  id: number;
  run_name: string;
  environment: string;
  controller_id: string;
  status: string;
  data_path: string;
  raw_bag_path?: string;
  metadata_path?: string;
  zip_path?: string;
  duration?: number | null;
  samples_count?: number | null;
  start_time?: string | null;
}

interface DatasetScenario {
  id: number;
  name: string;
  context_type?: string;
  difficulty?: string;
  human_mode?: string;
  description?: string;
}

interface ActiveDatasetPayload {
  active: boolean;
  run: DatasetRun | null;
  telemetry?: unknown;
  metadata?: Record<string, unknown> | null;
}

export default function DatasetPage() {
  const [active, setActive] = useState<ActiveDatasetPayload>({ active: false, run: null });
  const [runs, setRuns] = useState<DatasetRun[]>([]);
  const [scenarios, setScenarios] = useState<DatasetScenario[]>([]);
  const [scenarioName, setScenarioName] = useState("S1_open_zone");
  const [controllerId, setControllerId] = useState("CCA_NMPC");
  const [environment, setEnvironment] = useState<"real" | "sim">("real");
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
    setActive((await activeRes.json()) as ActiveDatasetPayload);
    setRuns((await runsRes.json()) as DatasetRun[]);
    const nextScenarios = (await scenariosRes.json()) as DatasetScenario[];
    setScenarios(nextScenarios);
    if (nextScenarios[0] && scenarioName === "S1_open_zone") {
      setScenarioName(nextScenarios[0].name);
    }
  }, [scenarioName]);

  useEffect(() => {
    const timer = window.setTimeout(() => {
      void refresh().catch(() => setMessage("Cannot reach rai_web_api dataset endpoints."));
    }, 0);
    return () => window.clearTimeout(timer);
  }, [refresh]);

  async function startDataset() {
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
        }),
      });
      const payload = await response.json();
      setMessage(`Started ${payload.run_name ?? "dataset run"}.`);
      await refresh();
    } catch (error) {
      setMessage(error instanceof Error ? error.message : "Cannot start dataset run.");
    } finally {
      setBusy(false);
    }
  }

  async function stopDataset() {
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
  }

  async function captureLabeledSample() {
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
  }

  return (
    <div className="space-y-6">
      <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
        <div>
          <h2 className="text-2xl font-bold tracking-tight text-slate-800">Dataset</h2>
          <p className="mt-1 text-sm text-slate-500">Continuous CCA-NMPC dataset collection via rai_web_api.</p>
        </div>
        <StatusBadge status={active.active ? "success" : "default"}>
          {active.active ? "RECORDING" : "IDLE"}
        </StatusBadge>
      </div>

      <section className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
        <div className="mb-4 flex items-center gap-2 text-sm font-semibold text-slate-800">
          <Database className="h-4 w-4 text-blue-600" />
          Run Configuration
        </div>
        <div className="grid gap-3 md:grid-cols-4">
          <label className="block text-sm">
            <span className="text-xs font-medium text-slate-500">Scenario</span>
            <select
              value={scenarioName}
              onChange={(event) => setScenarioName(event.target.value)}
              className="mt-1 w-full rounded-md border border-slate-300 px-3 py-2 text-sm"
            >
              {[scenarioName, ...scenarios.map((item) => item.name)]
                .filter((value, index, values) => value && values.indexOf(value) === index)
                .map((name) => <option key={name} value={name}>{name}</option>)}
            </select>
          </label>
          <TextInput label="Controller" value={controllerId} onChange={setControllerId} />
          <label className="block text-sm">
            <span className="text-xs font-medium text-slate-500">Environment</span>
            <select
              value={environment}
              onChange={(event) => setEnvironment(event.target.value as "real" | "sim")}
              className="mt-1 w-full rounded-md border border-slate-300 px-3 py-2 text-sm"
            >
              <option value="real">real</option>
              <option value="sim">sim</option>
            </select>
          </label>
          <TextInput label="Split" value={split} onChange={setSplit} />
        </div>
        <div className="mt-4 flex flex-wrap gap-2">
          <button
            type="button"
            onClick={startDataset}
            disabled={busy || active.active}
            className="inline-flex items-center gap-2 rounded-md bg-emerald-600 px-3 py-2 text-sm font-semibold text-white disabled:opacity-50"
          >
            <Play className="h-4 w-4" />
            Start
          </button>
          <button
            type="button"
            onClick={stopDataset}
            disabled={busy || !active.active}
            className="inline-flex items-center gap-2 rounded-md bg-slate-800 px-3 py-2 text-sm font-semibold text-white disabled:opacity-50"
          >
            <Square className="h-4 w-4" />
            Stop
          </button>
          <button
            type="button"
            onClick={() => void refresh()}
            disabled={busy}
            className="inline-flex items-center gap-2 rounded-md border border-slate-300 bg-white px-3 py-2 text-sm font-semibold text-slate-800 disabled:opacity-50"
          >
            <RefreshCw className="h-4 w-4" />
            Refresh
          </button>
        </div>
      </section>

      <section className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
        <div className="mb-4 text-sm font-semibold text-slate-800">Labeled Capture</div>
        <div className="grid gap-3 md:grid-cols-3">
          <TextInput label="Tag" value={captureTag} onChange={setCaptureTag} />
          <TextInput label="Class" value={captureClass} onChange={setCaptureClass} />
          <div className="flex items-end">
            <button
              type="button"
              onClick={captureLabeledSample}
              disabled={busy}
              className="inline-flex items-center gap-2 rounded-md border border-slate-300 bg-white px-3 py-2 text-sm font-semibold text-slate-800 disabled:opacity-50"
            >
              <Database className="h-4 w-4" />
              Capture sample
            </button>
          </div>
        </div>
      </section>

      {active.run && (
        <section className="rounded-lg border border-emerald-200 bg-emerald-50 p-5 shadow-sm">
          <div className="mb-3 text-sm font-semibold text-emerald-800">Active Run</div>
          <div className="grid gap-3 md:grid-cols-3">
            <Metric label="Name" value={active.run.run_name} />
            <Metric label="Status" value={active.run.status} />
            <Metric label="Path" value={active.run.data_path} />
          </div>
        </section>
      )}

      {message && <div className="rounded-md border border-slate-200 bg-white px-4 py-3 text-sm text-slate-700">{message}</div>}

      <section className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
        <div className="mb-4 text-sm font-semibold text-slate-800">Recent Runs</div>
        <div className="overflow-x-auto">
          <table className="w-full text-left text-sm text-slate-600">
            <thead className="border-b border-slate-200 text-xs uppercase text-slate-400">
              <tr>
                <th className="py-3 pr-4">Run</th>
                <th className="py-3 pr-4">Controller</th>
                <th className="py-3 pr-4">Status</th>
                <th className="py-3 pr-4">Samples</th>
                <th className="py-3 pr-4">Started</th>
                <th className="py-3 pr-4">Download</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-100">
              {runs.length === 0 ? (
                <tr><td className="py-6 text-slate-400" colSpan={6}>No dataset runs yet.</td></tr>
              ) : (
                runs.map((run) => (
                  <tr key={run.id}>
                    <td className="py-3 pr-4 font-medium text-slate-800">{run.run_name}</td>
                    <td className="py-3 pr-4">{run.controller_id}</td>
                    <td className="py-3 pr-4">{run.status}</td>
                    <td className="py-3 pr-4">{run.samples_count ?? "-"}</td>
                    <td className="py-3 pr-4">{run.start_time ? new Date(run.start_time).toLocaleString() : "-"}</td>
                    <td className="py-3 pr-4">
                      <a
                        href={resolveApiEndpoint(`/api/dataset/download/${run.id}`)}
                        className="inline-flex items-center gap-1 text-blue-700 hover:text-blue-900"
                      >
                        <Download className="h-4 w-4" />
                        zip
                      </a>
                    </td>
                  </tr>
                ))
              )}
            </tbody>
          </table>
        </div>
      </section>
    </div>
  );
}

function TextInput({ label, value, onChange }: { label: string; value: string; onChange: (value: string) => void }) {
  return (
    <label className="block text-sm">
      <span className="text-xs font-medium text-slate-500">{label}</span>
      <input value={value} onChange={(event) => onChange(event.target.value)} className="mt-1 w-full rounded-md border border-slate-300 px-3 py-2 text-sm" />
    </label>
  );
}

function Metric({ label, value }: { label: string; value: string }) {
  return (
    <div className="rounded-lg border border-slate-200 bg-white px-4 py-3 shadow-sm">
      <div className="text-xs text-slate-500">{label}</div>
      <div className="mt-1 truncate text-sm font-semibold text-slate-900">{value}</div>
    </div>
  );
}
