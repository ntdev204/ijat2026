"use client";

import { StatusBadge } from "@/components/ui/StatusBadge";
import { Button } from "@/components/ui/button";
import { DropdownField } from "@/components/ui/dropdown-field";
import { useDatasetRuntime } from "@/hooks/useDatasetRuntime";
import { resolveApiEndpoint } from "@/lib/api";
import type { RuntimeEnvironment } from "@/types/robot-runtime";
import { Database, Download, Play, RefreshCw, Square } from "lucide-react";

export default function DatasetPage() {
  const dataset = useDatasetRuntime();
  const { active, busy, form, message, runs, scenarios } = dataset;

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
            <DropdownField
              value={form.scenarioName}
              onValueChange={dataset.setScenarioName}
              options={[form.scenarioName, ...scenarios.map((item) => item.name)]
                .filter((value, index, values) => value && values.indexOf(value) === index)
                .map((name) => ({ value: name, label: name }))}
            />
          </label>
          <TextInput label="Controller" value={form.controllerId} onChange={dataset.setControllerId} />
          <label className="block text-sm">
            <span className="text-xs font-medium text-slate-500">Environment</span>
            <DropdownField
              value={form.environment}
              onValueChange={(value) => dataset.setEnvironment(value as RuntimeEnvironment)}
              options={[
                { value: "real", label: "real" },
                { value: "sim", label: "sim" },
              ]}
            />
          </label>
          <TextInput label="Split" value={form.split} onChange={dataset.setSplit} />
        </div>
        <div className="mt-4 flex flex-wrap gap-2">
          <Button type="button" onClick={() => void dataset.startDataset()} disabled={busy || active.active} className="gap-2">
            <Play className="h-4 w-4" />
            Start
          </Button>
          <Button type="button" variant="outline" onClick={() => void dataset.stopDataset()} disabled={busy || !active.active} className="gap-2">
            <Square className="h-4 w-4" />
            Stop
          </Button>
          <Button type="button" variant="outline" onClick={() => void dataset.refresh()} disabled={busy} className="gap-2">
            <RefreshCw className="h-4 w-4" />
            Refresh
          </Button>
        </div>
      </section>

      <section className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
        <div className="mb-4 text-sm font-semibold text-slate-800">Labeled Capture</div>
        <div className="grid gap-3 md:grid-cols-3">
          <TextInput label="Tag" value={form.captureTag} onChange={dataset.setCaptureTag} />
          <TextInput label="Class" value={form.captureClass} onChange={dataset.setCaptureClass} />
          <div className="flex items-end">
            <Button type="button" variant="outline" onClick={() => void dataset.captureLabeledSample()} disabled={busy} className="gap-2">
              <Database className="h-4 w-4" />
              Capture sample
            </Button>
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

interface TextInputProps {
  label: string;
  value: string;
  onChange: (value: string) => void;
}

function TextInput({ label, value, onChange }: TextInputProps) {
  return (
    <label className="block text-sm">
      <span className="text-xs font-medium text-slate-500">{label}</span>
      <input value={value} onChange={(event) => onChange(event.target.value)} className="mt-1 w-full rounded-md border border-slate-300 px-3 py-2 text-sm" />
    </label>
  );
}

interface MetricProps {
  label: string;
  value: string;
}

function Metric({ label, value }: MetricProps) {
  return (
    <div className="rounded-lg border border-slate-200 bg-white px-4 py-3 shadow-sm">
      <div className="text-xs text-slate-500">{label}</div>
      <div className="mt-1 truncate text-sm font-semibold text-slate-900">{value}</div>
    </div>
  );
}
