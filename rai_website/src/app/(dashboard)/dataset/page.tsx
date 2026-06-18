"use client";

import type React from "react";
import { StatusBadge } from "@/components/ui/StatusBadge";
import { Button } from "@/components/ui/button";
import { Checkbox } from "@/components/ui/checkbox";
import { DropdownField } from "@/components/ui/dropdown-field";
import { useDatasetRuntime } from "@/hooks/useDatasetRuntime";
import { resolveApiEndpoint } from "@/lib/api";
import type { RuntimeEnvironment } from "@/types/robot-runtime";
import {
  Activity,
  BarChart3,
  CheckCircle2,
  Database,
  Download,
  FileText,
  FolderTree,
  LineChart,
  Play,
  RefreshCw,
  Square,
  Table2,
  TriangleAlert,
} from "lucide-react";

const DEFAULT_SCENARIOS = [
  "S1_open_zone",
  "S2_crossing_human",
  "S3_corridor",
  "S4_occlusion",
  "S5_dense_dynamic",
];

const CONTROLLERS = [
  { value: "CCA_NMPC", label: "CCA-NMPC" },
  { value: "DWB", label: "DWB" },
  { value: "MPPI", label: "MPPI" },
  { value: "TEB", label: "TEB" },
  { value: "MANUAL", label: "Manual" },
];

export default function DatasetPage() {
  const dataset = useDatasetRuntime();
  const { active, artifacts, busy, form, message, pipeline, runs, scenarios } = dataset;
  const artifactReady = artifacts ? Object.values(artifacts.files).filter(Boolean).length : 0;
  const artifactTotal = artifacts ? Object.keys(artifacts.files).length : 0;

  return (
    <div className="space-y-6">
      <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
        <div>
          <h2 className="text-2xl font-bold tracking-tight text-slate-800">Dataset Collection</h2>
          <p className="mt-1 text-sm text-slate-500">Record rosbag2 runs and build CCA predictive-control paper artifacts.</p>
        </div>
        <div className="flex items-center gap-2">
          <StatusBadge status={active.active ? "success" : "default"}>
            {active.active ? "RECORDING" : "IDLE"}
          </StatusBadge>
          <Button type="button" variant="outline" onClick={() => void dataset.refresh()} disabled={busy} className="gap-2">
            <RefreshCw className="h-4 w-4" />
            Refresh
          </Button>
        </div>
      </div>

      <section className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
        <div className="mb-4 flex items-center justify-between gap-3">
          <div className="flex items-center gap-2 text-sm font-semibold text-slate-800">
            <Database className="h-4 w-4 text-blue-600" />
            Run Metadata
          </div>
          <span className="text-xs text-slate-500">{artifacts?.base_path ?? "dataset/"}</span>
        </div>

        <div className="grid gap-3 md:grid-cols-4">
          <label className="block text-sm">
            <span className="text-xs font-medium text-slate-500">Scenario</span>
            <DropdownField
              value={form.scenarioName}
              onValueChange={dataset.setScenarioName}
              options={[...DEFAULT_SCENARIOS, ...scenarios.map((item) => item.name)]
                .filter((value, index, values) => value && values.indexOf(value) === index)
                .map((name) => ({ value: name, label: name }))}
            />
          </label>
          <label className="block text-sm">
            <span className="text-xs font-medium text-slate-500">Controller</span>
            <DropdownField value={form.controllerId} onValueChange={dataset.setControllerId} options={CONTROLLERS} />
          </label>
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

        <div className="mt-3 grid gap-3 md:grid-cols-4">
          <TextInput label="Run index" value={form.runIndex} onChange={dataset.setRunIndex} placeholder="001" />
          <TextInput label="Random seed" value={form.randomSeed} onChange={dataset.setRandomSeed} />
          <TextInput label="Human behavior" value={form.humanBehavior} onChange={dataset.setHumanBehavior} />
          <TextInput label="Notes" value={form.notes} onChange={dataset.setNotes} />
        </div>

        <div className="mt-3 grid gap-3 md:grid-cols-6">
          <TextInput label="Start x" value={form.robotStartX} onChange={dataset.setRobotStartX} />
          <TextInput label="Start y" value={form.robotStartY} onChange={dataset.setRobotStartY} />
          <TextInput label="Start theta" value={form.robotStartTheta} onChange={dataset.setRobotStartTheta} />
          <TextInput label="Goal x" value={form.goalX} onChange={dataset.setGoalX} />
          <TextInput label="Goal y" value={form.goalY} onChange={dataset.setGoalY} />
          <TextInput label="Goal theta" value={form.goalTheta} onChange={dataset.setGoalTheta} />
        </div>

        <div className="mt-4 flex flex-wrap items-center gap-4">
          <label className="flex items-center gap-2 text-sm text-slate-700">
            <Checkbox checked={form.recordCamera} onCheckedChange={(value) => dataset.setRecordCamera(value === true)} />
            Record RGB-D topics
          </label>
          <label className="flex items-center gap-2 text-sm text-slate-700">
            <Checkbox checked={form.intervention} onCheckedChange={(value) => dataset.setIntervention(value === true)} />
            Intervention
          </label>
        </div>
      </section>

      <section className="grid gap-4 lg:grid-cols-[1.15fr_0.85fr]">
        <div className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
          <div className="mb-4 flex items-center gap-2 text-sm font-semibold text-slate-800">
            <Activity className="h-4 w-4 text-emerald-600" />
            Recording Control
          </div>
          <div className="flex flex-wrap gap-2">
            <Button type="button" variant="outline" onClick={() => void dataset.prepareDataset()} disabled={busy || active.active} className="gap-2">
              <FolderTree className="h-4 w-4" />
              Prepare folders
            </Button>
            <Button type="button" onClick={() => void dataset.startDataset()} disabled={busy || active.active} className="gap-2">
              <Play className="h-4 w-4" />
              Start bag
            </Button>
            <Button type="button" variant="outline" onClick={() => void dataset.stopDataset()} disabled={busy || !active.active} className="gap-2">
              <Square className="h-4 w-4" />
              Stop
            </Button>
          </div>

          {active.run && (
            <div className="mt-4 grid gap-3 md:grid-cols-3">
              <Metric label="Run" value={active.run.run_name} />
              <Metric label="Status" value={active.run.status} />
              <Metric label="Bag" value={active.run.raw_bag_path ?? "-"} />
            </div>
          )}
        </div>

        <div className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
          <div className="mb-4 flex items-center gap-2 text-sm font-semibold text-slate-800">
            {artifactReady === artifactTotal && artifactTotal > 0 ? (
              <CheckCircle2 className="h-4 w-4 text-emerald-600" />
            ) : (
              <TriangleAlert className="h-4 w-4 text-amber-600" />
            )}
            Artifact Readiness
          </div>
          <div className="text-3xl font-semibold text-slate-900">{artifactReady}/{artifactTotal}</div>
          <div className="mt-2 text-sm text-slate-500">Required metadata and derived CSV schemas present.</div>
          <div className="mt-4 max-h-32 overflow-auto rounded-md border border-slate-100 bg-slate-50 p-3 text-xs text-slate-600">
            {artifacts ? Object.entries(artifacts.files).map(([file, ready]) => (
              <div key={file} className="flex justify-between gap-3">
                <span className="truncate">{file}</span>
                <span className={ready ? "text-emerald-700" : "text-amber-700"}>{ready ? "ok" : "missing"}</span>
              </div>
            )) : "No artifact status yet."}
          </div>
        </div>
      </section>

      <section className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
        <div className="mb-4 flex items-center gap-2 text-sm font-semibold text-slate-800">
          <LineChart className="h-4 w-4 text-indigo-600" />
          Processing Pipeline
        </div>
        <div className="flex flex-wrap gap-2">
          <PipelineButton icon={<CheckCircle2 className="h-4 w-4" />} label="Validate" busy={busy} onClick={() => void dataset.runPipeline("validate")} />
          <PipelineButton icon={<FileText className="h-4 w-4" />} label="Bag to CSV" busy={busy} onClick={() => void dataset.runPipeline("bag_to_csv")} />
          <PipelineButton icon={<BarChart3 className="h-4 w-4" />} label="Metrics" busy={busy} onClick={() => void dataset.runPipeline("metrics")} />
          <PipelineButton icon={<LineChart className="h-4 w-4" />} label="Figures" busy={busy} onClick={() => void dataset.runPipeline("plots")} />
          <PipelineButton icon={<Table2 className="h-4 w-4" />} label="Tables" busy={busy} onClick={() => void dataset.runPipeline("tables")} />
          <PipelineButton icon={<Database className="h-4 w-4" />} label="Run all" busy={busy} primary onClick={() => void dataset.runPipeline("all")} />
        </div>
        {pipeline && (
          <div className="mt-4 overflow-auto rounded-md border border-slate-100 bg-slate-50 p-3 text-xs text-slate-700">
            {pipeline.results.map((item, index) => (
              <div key={`${item.command.join(" ")}-${index}`} className="mb-2 last:mb-0">
                <div className={item.returncode === 0 ? "font-medium text-emerald-700" : "font-medium text-red-700"}>
                  {item.returncode === 0 ? "OK" : `ERR ${item.returncode}`} · {item.command.join(" ")}
                </div>
                {item.stderr && <pre className="mt-1 whitespace-pre-wrap text-red-700">{item.stderr}</pre>}
                {item.stdout && <pre className="mt-1 whitespace-pre-wrap text-slate-600">{item.stdout}</pre>}
              </div>
            ))}
          </div>
        )}
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

      {message && <div className="rounded-md border border-slate-200 bg-white px-4 py-3 text-sm text-slate-700">{message}</div>}

      <section className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
        <div className="mb-4 text-sm font-semibold text-slate-800">Recent Runs</div>
        <div className="overflow-x-auto">
          <table className="w-full text-left text-sm text-slate-600">
            <thead className="border-b border-slate-200 text-xs uppercase text-slate-400">
              <tr>
                <th className="py-3 pr-4">Run</th>
                <th className="py-3 pr-4">Scenario</th>
                <th className="py-3 pr-4">Controller</th>
                <th className="py-3 pr-4">Status</th>
                <th className="py-3 pr-4">Duration</th>
                <th className="py-3 pr-4">Min d_h</th>
                <th className="py-3 pr-4">Started</th>
                <th className="py-3 pr-4">Download</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-100">
              {runs.length === 0 ? (
                <tr><td className="py-6 text-slate-400" colSpan={8}>No dataset runs yet.</td></tr>
              ) : (
                runs.map((run) => (
                  <tr key={run.id}>
                    <td className="py-3 pr-4 font-medium text-slate-800">{run.run_name}</td>
                    <td className="py-3 pr-4">{run.scenario_name ?? "-"}</td>
                    <td className="py-3 pr-4">{run.controller_id}</td>
                    <td className="py-3 pr-4">{run.status}</td>
                    <td className="py-3 pr-4">{formatNumber(run.duration)}</td>
                    <td className="py-3 pr-4">{formatNumber(run.min_human_clearance)}</td>
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
  placeholder?: string;
}

function TextInput({ label, value, onChange, placeholder }: TextInputProps) {
  return (
    <label className="block text-sm">
      <span className="text-xs font-medium text-slate-500">{label}</span>
      <input
        value={value}
        placeholder={placeholder}
        onChange={(event) => onChange(event.target.value)}
        className="mt-1 h-9 w-full rounded-md border border-slate-300 px-3 py-2 text-sm outline-none focus:border-blue-500 focus:ring-2 focus:ring-blue-100"
      />
    </label>
  );
}

interface PipelineButtonProps {
  icon: React.ReactNode;
  label: string;
  busy: boolean;
  primary?: boolean;
  onClick: () => void;
}

function PipelineButton({ icon, label, busy, primary = false, onClick }: PipelineButtonProps) {
  return (
    <Button type="button" variant={primary ? "default" : "outline"} onClick={onClick} disabled={busy} className="gap-2">
      {icon}
      {label}
    </Button>
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

function formatNumber(value: number | null | undefined) {
  if (value === null || value === undefined || Number.isNaN(value)) {
    return "-";
  }
  return value.toFixed(2);
}
