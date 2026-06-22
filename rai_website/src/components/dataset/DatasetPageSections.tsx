"use client";

import {
  Activity,
  CheckCircle2,
  Database,
  Download,
  FolderTree,
  Play,
  Square,
  Table2,
  TriangleAlert,
} from "lucide-react";
import { Button } from "@/components/ui/button";
import { Checkbox } from "@/components/ui/checkbox";
import { DropdownField } from "@/components/ui/dropdown-field";
import {
  Pagination,
  PaginationContent,
  PaginationItem,
  PaginationLink,
  PaginationNext,
  PaginationPrevious,
} from "@/components/ui/pagination";
import type { DatasetRuntime } from "@/types/dataset-runtime";
import type {
  DatasetArtifactStatusItem,
  DatasetPageState,
  DatasetPipelineAction,
  DatasetRunRow,
  SelectOption,
} from "@/types/dataset";
import type { DatasetPipelineResult } from "@/types/robot-runtime";

export function DatasetMetadataSection({
  dataset,
  state,
}: {
  dataset: DatasetRuntime;
  state: DatasetPageState;
}) {
  return (
    <section className="grid gap-4 xl:grid-cols-[1.2fr_0.8fr]">
      <article className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
        <div className="mb-4 flex items-center gap-2 text-sm font-semibold text-slate-900">
          <Database className="h-4 w-4 text-blue-600" />
          Run Metadata
        </div>
        <div className="grid gap-3 md:grid-cols-4">
          <SelectField
            label="Scenario"
            value={dataset.form.scenarioName}
            onValueChange={dataset.setScenarioName}
            options={dataset.scenarios.map((scenario) => ({
              value: scenario.name,
              label: scenario.name,
            }))}
          />
          <SelectField
            label="Controller"
            value={dataset.form.controllerId}
            onValueChange={dataset.setControllerId}
            options={state.controllerOptions}
          />
          <SelectField
            label="Environment"
            value={dataset.form.environment}
            onValueChange={(value) => dataset.setEnvironment(value as DatasetRuntime["form"]["environment"])}
            options={state.environmentOptions}
          />
          <SelectField
            label="Split"
            value={dataset.form.split}
            onValueChange={dataset.setSplit}
            options={state.splitOptions}
          />
        </div>

        <div className="mt-3 grid gap-3 md:grid-cols-4">
          <TextInput
            label="Run index"
            value={dataset.form.runIndex}
            onChange={dataset.setRunIndex}
            placeholder="001"
          />
          <TextInput
            label="Random seed"
            value={dataset.form.randomSeed}
            onChange={dataset.setRandomSeed}
          />
          <TextInput
            label="Human behavior"
            value={dataset.form.humanBehavior}
            onChange={dataset.setHumanBehavior}
          />
          <TextInput
            label="Notes"
            value={dataset.form.notes}
            onChange={dataset.setNotes}
          />
        </div>

        <div className="mt-3 grid gap-3 md:grid-cols-6">
          <TextInput label="Start x" value={dataset.form.robotStartX} onChange={dataset.setRobotStartX} />
          <TextInput label="Start y" value={dataset.form.robotStartY} onChange={dataset.setRobotStartY} />
          <TextInput
            label="Start theta"
            value={dataset.form.robotStartTheta}
            onChange={dataset.setRobotStartTheta}
          />
          <TextInput label="Goal x" value={dataset.form.goalX} onChange={dataset.setGoalX} />
          <TextInput label="Goal y" value={dataset.form.goalY} onChange={dataset.setGoalY} />
          <TextInput label="Goal theta" value={dataset.form.goalTheta} onChange={dataset.setGoalTheta} />
        </div>

        <div className="mt-4 flex flex-wrap items-center gap-4">
          <label className="flex items-center gap-2 text-sm text-slate-700">
            <Checkbox
              checked={dataset.form.recordCamera}
              onCheckedChange={(value) => dataset.setRecordCamera(value === true)}
            />
            Record RGB-D topics
          </label>
          <label className="flex items-center gap-2 text-sm text-slate-700">
            <Checkbox
              checked={dataset.form.intervention}
              onCheckedChange={(value) => dataset.setIntervention(value === true)}
            />
            Intervention
          </label>
        </div>
      </article>

      <article className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
        <div className="mb-4 flex items-center gap-2 text-sm font-semibold text-slate-900">
          <Activity className="h-4 w-4 text-emerald-600" />
          Launch Stack
        </div>
        <div className="space-y-3">
          <Metric label="Launch running" value={dataset.launchStatus?.running ? "True" : "False"} />
          <Metric label="Launch pid" value={dataset.launchStatus?.pid ? String(dataset.launchStatus.pid) : "N/A"} />
          <Metric label="Active run id" value={dataset.launchStatus?.active_run_id || "N/A"} />
        </div>
        <div className="mt-4 flex flex-wrap gap-2 max-sm:[&_[data-slot=button]]:flex-1">
          <Button
            type="button"
            onClick={() => void dataset.startLaunchStack()}
            disabled={dataset.busy || !state.datasetAllowed || dataset.launchStatus?.running}
            className="gap-2"
          >
            <Play className="h-4 w-4" />
            Launch dataset stack
          </Button>
          <Button
            type="button"
            variant="outline"
            onClick={() => void dataset.stopLaunchStack()}
            disabled={dataset.busy || !state.datasetAllowed || !dataset.launchStatus?.running}
            className="gap-2"
          >
            <Square className="h-4 w-4" />
            Stop launch stack
          </Button>
        </div>
      </article>
    </section>
  );
}

export function DatasetOperationsSection({
  dataset,
  state,
}: {
  dataset: DatasetRuntime;
  state: DatasetPageState;
}) {
  return (
    <section className="grid gap-4 xl:grid-cols-[1.1fr_0.9fr]">
      <article className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
        <div className="mb-4 flex items-center gap-2 text-sm font-semibold text-slate-900">
          <FolderTree className="h-4 w-4 text-violet-600" />
          Recording Control
        </div>
        <div className="flex flex-wrap gap-2 max-sm:[&_[data-slot=button]]:flex-1">
          <Button
            type="button"
            variant="outline"
            onClick={() => void dataset.prepareDataset()}
            disabled={dataset.busy || !state.datasetAllowed || dataset.active.active}
            className="gap-2"
          >
            <FolderTree className="h-4 w-4" />
            Prepare folders
          </Button>
          <Button
            type="button"
            onClick={() => void dataset.startDataset()}
            disabled={dataset.busy || !state.datasetAllowed || dataset.active.active}
            className="gap-2"
          >
            <Play className="h-4 w-4" />
            Start run
          </Button>
          <Button
            type="button"
            variant="outline"
            onClick={() => void dataset.stopDataset()}
            disabled={dataset.busy || !state.datasetAllowed || !dataset.active.active}
            className="gap-2"
          >
            <Square className="h-4 w-4" />
            Stop run
          </Button>
        </div>
        {dataset.active.run ? (
          <div className="mt-4 grid gap-3 md:grid-cols-3">
            <Metric label="Run" value={dataset.active.run.run_name} />
            <Metric label="Status" value={dataset.active.run.status} />
            <Metric label="Path" value={dataset.active.run.data_path} />
          </div>
        ) : (
          <EmptyState message="No dataset run is recording right now." />
        )}
        {!state.datasetAllowed && dataset.systemRuntime && (
          <p className="mt-4 text-sm text-amber-700">
            Dataset control is disabled for {dataset.systemRuntime.device_label} ({dataset.systemRuntime.device_role}).
          </p>
        )}
      </article>

      <DatasetArtifactCard
        artifactReady={state.artifactReady}
        artifactTotal={state.artifactTotal}
        items={state.artifactStatusItems}
      />
    </section>
  );
}

function DatasetArtifactCard({
  artifactReady,
  artifactTotal,
  items,
}: {
  artifactReady: number;
  artifactTotal: number;
  items: DatasetArtifactStatusItem[];
}) {
  return (
    <article className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
      <div className="mb-4 flex items-center gap-2 text-sm font-semibold text-slate-900">
        {artifactReady === artifactTotal && artifactTotal > 0 ? (
          <CheckCircle2 className="h-4 w-4 text-emerald-600" />
        ) : (
          <TriangleAlert className="h-4 w-4 text-amber-600" />
        )}
        Artifact Readiness
      </div>
      <div className="text-3xl font-semibold text-slate-900">
        {artifactReady}/{artifactTotal}
      </div>
      <p className="mt-2 text-sm text-slate-500">
        Required schema files, derived CSV outputs, and benchmark metadata status.
      </p>
      <div className="mt-4 max-h-40 overflow-auto rounded-md border border-slate-200 bg-slate-50 p-3 text-xs text-slate-700">
        {items.length > 0 ? (
          items.map((item) => (
            <div
              key={item.file}
              className="flex items-center justify-between gap-3 py-1"
            >
              <span className="truncate">{item.file}</span>
              <span className={item.ready ? "text-emerald-700" : "text-amber-700"}>
                {item.ready ? "ready" : "missing"}
              </span>
            </div>
          ))
        ) : (
          <EmptyState message="No artifact status yet." compact />
        )}
      </div>
    </article>
  );
}

export function DatasetAnalysisSection({
  dataset,
  state,
}: {
  dataset: DatasetRuntime;
  state: DatasetPageState;
}) {
  return (
    <section className="grid gap-4 xl:grid-cols-3">
      <article className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
        <div className="mb-4 text-sm font-semibold text-slate-900">
          Scenario Specification
        </div>
        {state.selectedScenarioSummary ? (
          <div className="space-y-2">
            <Metric label="Scientific goal" value={state.selectedScenarioSummary.scientificGoal} />
            <Metric label="Layout" value={state.selectedScenarioSummary.layout} />
            <Metric label="Humans" value={state.selectedScenarioSummary.humanCount} />
            <Metric label="Human speed" value={state.selectedScenarioSummary.humanSpeed} />
            <Metric label="Primary metric" value={state.selectedScenarioSummary.primaryMetric} />
          </div>
        ) : (
          <EmptyState message="No scenario metadata loaded." />
        )}
      </article>

      <article className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
        <div className="mb-4 text-sm font-semibold text-slate-900">
          Benchmark Framework
        </div>
        <div className="space-y-2">
          <Metric
            label="Baselines"
            value={(dataset.artifacts?.baseline_controllers || []).map((item) => item.id).join(", ") || "N/A"}
          />
          <Metric
            label="Ablations"
            value={(dataset.artifacts?.ablation_controllers || []).map((item) => item.id).join(", ") || "N/A"}
          />
          <Metric
            label="Dataset groups"
            value={(dataset.artifacts?.dataset_groups || []).map((item) => item.id).join(", ") || "N/A"}
          />
          <Metric
            label="Required figures"
            value={String((dataset.artifacts?.required_figures || []).length)}
          />
        </div>
      </article>

      <DatasetPipelineCard
        busy={dataset.busy}
        datasetAllowed={state.datasetAllowed}
        pipeline={dataset.pipeline}
        onRunPipeline={dataset.runPipeline}
      />
    </section>
  );
}

function DatasetPipelineCard({
  busy,
  datasetAllowed,
  pipeline,
  onRunPipeline,
}: {
  busy: boolean;
  datasetAllowed: boolean;
  pipeline: DatasetPipelineResult | null;
  onRunPipeline: (action: DatasetPipelineAction) => Promise<void>;
}) {
  return (
    <article className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
      <div className="mb-4 flex items-center gap-2 text-sm font-semibold text-slate-900">
        <Table2 className="h-4 w-4 text-indigo-600" />
        Processing Pipeline
      </div>
        <div className="flex flex-wrap gap-2 max-sm:[&_[data-slot=button]]:flex-1">
        <PipelineButton label="Validate" onClick={() => void onRunPipeline("validate")} disabled={busy || !datasetAllowed} />
        <PipelineButton label="Bag to CSV" onClick={() => void onRunPipeline("bag_to_csv")} disabled={busy || !datasetAllowed} />
        <PipelineButton label="Metrics" onClick={() => void onRunPipeline("metrics")} disabled={busy || !datasetAllowed} />
        <PipelineButton label="Figures" onClick={() => void onRunPipeline("plots")} disabled={busy || !datasetAllowed} />
        <PipelineButton label="Tables" onClick={() => void onRunPipeline("tables")} disabled={busy || !datasetAllowed} />
        <PipelineButton label="Run all" onClick={() => void onRunPipeline("all")} disabled={busy || !datasetAllowed} primary />
      </div>
      {pipeline && (
        <div className="mt-4 max-h-44 overflow-auto rounded-md border border-slate-200 bg-slate-50 p-3 text-xs text-slate-700">
          {pipeline.results.map((item, index) => (
            <div key={`${item.command.join(" ")}-${index}`} className="mb-3 last:mb-0">
              <div className={item.returncode === 0 ? "font-medium text-emerald-700" : "font-medium text-red-700"}>
                {item.returncode === 0 ? "OK" : `ERR ${item.returncode}`} · {item.command.join(" ")}
              </div>
              {item.stderr && <pre className="mt-1 whitespace-pre-wrap text-red-700">{item.stderr}</pre>}
              {item.stdout && <pre className="mt-1 whitespace-pre-wrap text-slate-600">{item.stdout}</pre>}
            </div>
          ))}
        </div>
      )}
    </article>
  );
}

export function DatasetCaptureSection({ dataset }: { dataset: DatasetRuntime }) {
  return (
    <section className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
      <div className="mb-4 text-sm font-semibold text-slate-900">Labeled Capture</div>
      <div className="grid gap-3 md:grid-cols-3">
        <TextInput label="Tag" value={dataset.form.captureTag} onChange={dataset.setCaptureTag} />
        <TextInput label="Class" value={dataset.form.captureClass} onChange={dataset.setCaptureClass} />
        <div className="flex items-end">
          <Button
            type="button"
            variant="outline"
            onClick={() => void dataset.captureLabeledSample()}
            disabled={dataset.busy}
            className="w-full gap-2 md:w-auto"
          >
            <Database className="h-4 w-4" />
            Capture sample
          </Button>
        </div>
      </div>
    </section>
  );
}

export function DatasetRunsSection({
  currentPage,
  runs,
  rows,
  totalPages,
  onPageChange,
}: {
  currentPage: number;
  runs: DatasetRuntime["runs"];
  rows: DatasetRunRow[];
  totalPages: number;
  onPageChange: (page: number) => void;
}) {
  const startIndex = (currentPage - 1) * 8;
  const pageRows = rows.slice(startIndex, startIndex + 8);

  return (
    <section className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
      <div className="mb-4 flex flex-col gap-3 lg:flex-row lg:items-center lg:justify-between">
        <div className="text-sm font-semibold text-slate-900">Dataset Runs</div>
        <span className="text-xs text-slate-500">
          Page {currentPage} / {totalPages}
        </span>
      </div>

      <div className="-mx-2 overflow-x-auto px-2">
        <table className="w-full min-w-[1180px] text-left text-sm text-slate-700">
          <thead className="border-b border-slate-200 text-xs uppercase text-slate-400">
            <tr>
              <th className="py-3 pr-4">Run</th>
              <th className="py-3 pr-4">Status</th>
              <th className="py-3 pr-4">Scenario</th>
              <th className="py-3 pr-4">Samples</th>
              <th className="py-3 pr-4">Duration</th>
              <th className="py-3 pr-4">Min distance</th>
              <th className="py-3 pr-4">Timeout</th>
              <th className="py-3 pr-4">Downloads</th>
            </tr>
          </thead>
          <tbody className="divide-y divide-slate-100">
            {runs.length === 0 ? (
              <tr>
                <td className="py-6 text-slate-400" colSpan={8}>
                  No dataset runs yet.
                </td>
              </tr>
            ) : (
              pageRows.map((run) => (
                <tr key={run.id} className="align-top">
                  <td className="py-3 pr-4">
                    <div className="font-medium text-slate-900">{run.run_name}</div>
                    <div className="mt-1 text-xs text-slate-500">{run.startedAtLabel}</div>
                  </td>
                  <td className="py-3 pr-4">{run.status}</td>
                  <td className="py-3 pr-4">
                    <div>{run.controller_id}</div>
                    <div className="mt-1 text-xs text-slate-500">
                      {run.environment} / {run.split || "unsplit"} / {run.scenario_name || "-"}
                    </div>
                  </td>
                  <td className="py-3 pr-4">{run.samples_count ?? 0}</td>
                  <td className="py-3 pr-4">{run.durationLabel}</td>
                  <td className="py-3 pr-4">{run.clearanceLabel}</td>
                  <td className="py-3 pr-4">{run.timeoutLabel}</td>
                  <td className="py-3 pr-4">
                    <div className="flex max-w-[360px] flex-wrap gap-2">
                      {run.downloadArtifacts.map((artifact) => (
                        <DownloadLink
                          key={`${run.id}-${artifact.label}`}
                          href={artifact.href}
                          label={artifact.label}
                        />
                      ))}
                    </div>
                  </td>
                </tr>
              ))
            )}
          </tbody>
        </table>
      </div>

      {runs.length > 0 && (
        <Pagination className="mt-4 justify-end">
          <PaginationContent>
            <PaginationItem>
              <PaginationPrevious
                href="#dataset-runs"
                text="Prev"
                aria-disabled={currentPage <= 1}
                className={currentPage <= 1 ? "pointer-events-none opacity-50" : ""}
                onClick={(event) => {
                  event.preventDefault();
                  if (currentPage > 1) {
                    onPageChange(currentPage - 1);
                  }
                }}
              />
            </PaginationItem>
            <PaginationItem>
              <PaginationLink href="#dataset-runs" isActive size="default" onClick={(event) => event.preventDefault()}>
                {currentPage}
              </PaginationLink>
            </PaginationItem>
            <PaginationItem>
              <PaginationNext
                href="#dataset-runs"
                text="Next"
                aria-disabled={currentPage >= totalPages}
                className={currentPage >= totalPages ? "pointer-events-none opacity-50" : ""}
                onClick={(event) => {
                  event.preventDefault();
                  if (currentPage < totalPages) {
                    onPageChange(currentPage + 1);
                  }
                }}
              />
            </PaginationItem>
          </PaginationContent>
        </Pagination>
      )}
    </section>
  );
}

function DownloadLink({ href, label }: { href: string; label: string }) {
  return (
    <a
      href={href}
      target="_blank"
      rel="noreferrer"
      className="inline-flex items-center gap-1 rounded-md border border-slate-200 px-2 py-1 text-xs font-medium text-slate-700 transition hover:border-slate-300 hover:bg-slate-50 hover:text-slate-900"
    >
      <Download className="h-3.5 w-3.5" />
      {label}
    </a>
  );
}

function SelectField({
  label,
  value,
  onValueChange,
  options,
}: {
  label: string;
  value: string;
  onValueChange: (value: string) => void;
  options: SelectOption[];
}) {
  return (
    <label className="block text-sm">
      <span className="text-xs font-medium text-slate-500">{label}</span>
      <DropdownField value={value} onValueChange={onValueChange} options={options} />
    </label>
  );
}

function TextInput({
  label,
  value,
  onChange,
  placeholder,
}: {
  label: string;
  value: string;
  onChange: (value: string) => void;
  placeholder?: string;
}) {
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

function PipelineButton({
  label,
  disabled,
  primary = false,
  onClick,
}: {
  label: string;
  disabled: boolean;
  primary?: boolean;
  onClick: () => void;
}) {
  return (
    <Button type="button" variant={primary ? "default" : "outline"} onClick={onClick} disabled={disabled}>
      {label}
    </Button>
  );
}

function Metric({ label, value }: { label: string; value: string }) {
  return (
    <div className="rounded-md border border-slate-200 bg-slate-50 px-3 py-2">
      <div className="text-xs text-slate-500">{label}</div>
      <div className="mt-1 break-words text-sm font-medium text-slate-900">{value}</div>
    </div>
  );
}

function EmptyState({ message, compact = false }: { message: string; compact?: boolean }) {
  return (
    <div
      className={
        compact
          ? "text-xs text-slate-500"
          : "rounded-md border border-dashed border-slate-200 px-4 py-5 text-sm text-slate-500"
      }
    >
      {message}
    </div>
  );
}
