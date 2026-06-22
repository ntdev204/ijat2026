"use client";

import { RefreshCw } from "lucide-react";
import {
  DatasetAnalysisSection,
  DatasetCaptureSection,
  DatasetMetadataSection,
  DatasetOperationsSection,
  DatasetRunsSection,
} from "@/components/dataset/DatasetPageSections";
import { StatusBadge } from "@/components/ui/StatusBadge";
import { Button } from "@/components/ui/button";
import { useDatasetPageState } from "@/hooks/useDatasetPageState";
import { useDatasetRuntime } from "@/hooks/useDatasetRuntime";

export default function DatasetPage() {
  const dataset = useDatasetRuntime();
  const state = useDatasetPageState({
    artifacts: dataset.artifacts,
    scenarios: dataset.scenarios,
    runs: dataset.runs,
    currentPage: dataset.currentPage,
    scenarioName: dataset.form.scenarioName,
    environment: dataset.form.environment,
    systemAllowedActions: dataset.systemRuntime?.allowed_actions,
  });

  return (
    <div className="space-y-5 sm:space-y-6">
      <div className="flex flex-col gap-3 lg:flex-row lg:items-center lg:justify-between">
        <div>
          <h2 className="text-2xl font-bold tracking-tight text-slate-900">
            Dataset Operations
          </h2>
          <p className="mt-1 text-sm text-slate-500">
            Collect, launch, export, and benchmark the full CCA-NMPC dataset
            pipeline from one dashboard.
          </p>
          {dataset.systemRuntime && (
            <p className="mt-1 text-xs text-slate-500">
              API target: {dataset.systemRuntime.device_label} (
              {dataset.systemRuntime.device_role})
            </p>
          )}
        </div>
        <div className="flex flex-wrap items-center gap-2 max-sm:[&_[data-slot=button]]:flex-1">
          <StatusBadge
            status={
              dataset.active.active
                ? "success"
                : dataset.launchStatus?.running
                  ? "warning"
                  : "default"
            }
          >
            {dataset.active.active
              ? "RECORDING"
              : dataset.launchStatus?.running
                ? "STACK LIVE"
                : "IDLE"}
          </StatusBadge>
          <Button
            type="button"
            variant="outline"
            onClick={() => void dataset.refresh()}
            disabled={dataset.busy}
            className="gap-2"
          >
            <RefreshCw className="h-4 w-4" />
            Refresh
          </Button>
        </div>
      </div>

      <DatasetMetadataSection dataset={dataset} state={state} />
      <DatasetOperationsSection dataset={dataset} state={state} />
      <DatasetAnalysisSection dataset={dataset} state={state} />
      <DatasetCaptureSection dataset={dataset} />

      {dataset.message && (
        <div className="rounded-md border border-slate-200 bg-white px-4 py-3 text-sm text-slate-700">
          {dataset.message}
        </div>
      )}

      <DatasetRunsSection
        currentPage={dataset.currentPage}
        runs={dataset.runs}
        rows={state.pagedRunRows}
        totalPages={dataset.totalPages}
        onPageChange={dataset.setCurrentPage}
      />
    </div>
  );
}
