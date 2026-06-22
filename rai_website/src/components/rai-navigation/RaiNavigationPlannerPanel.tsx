"use client";

import { Button } from "@/components/ui/button";
import { DropdownField } from "@/components/ui/dropdown-field";
import { Input } from "@/components/ui/input";
import { useOperationMode } from "@/contexts/OperationModeContext";
import type { RaiNavigationConfig, RaiNavigationOption, SystemRuntime } from "@/types/robot-runtime";
import { Play, RefreshCw, Square } from "lucide-react";

export interface RaiNavigationPlannerPanelProps {
  busy: boolean;
  message?: string;
  systemRuntime?: SystemRuntime | null;
  raiNavigationConfig: RaiNavigationConfig;
  raiNavigationMaps: Array<{ id?: number; name?: string; yaml_path?: string | null }>;
  selectedMapId: string;
  raiNavigationGlobalOptions: RaiNavigationOption[];
  raiNavigationLocalOptions: RaiNavigationOption[];
  loadRaiNavigationState?: () => Promise<void>;
  updateRaiNavigationConfig: (localPlanner: string, globalPlanner: string, mapId?: number) => Promise<void>;
  selectRaiNavigationMap: (mapId: string) => Promise<void>;
  startRaiNavigation: () => Promise<void>;
  stopRaiNavigation: () => Promise<void>;
}

export function RaiNavigationPlannerPanel({
  busy,
  message,
  systemRuntime,
  raiNavigationConfig,
  raiNavigationMaps,
  selectedMapId,
  raiNavigationGlobalOptions,
  raiNavigationLocalOptions,
  loadRaiNavigationState,
  updateRaiNavigationConfig,
  selectRaiNavigationMap,
  startRaiNavigation,
  stopRaiNavigation,
}: RaiNavigationPlannerPanelProps) {
  const { operationMode } = useOperationMode();
  const hasSelectedMap = selectedMapId.length > 0;
  const raiNavigationAllowed = (systemRuntime?.allowed_actions.includes("navigation") || !systemRuntime) && operationMode !== "sim";

  return (
    <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
      <div className="mb-3 flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
        <h3 className="text-sm font-bold uppercase tracking-wider text-slate-400">RAI Navigation</h3>
        {loadRaiNavigationState && (
          <Button type="button" variant="outline" size="sm" className="gap-2" disabled={busy} onClick={() => void loadRaiNavigationState()}>
            <RefreshCw className="size-4" />
            Refresh
          </Button>
        )}
      </div>

      <div className="grid gap-3">
        <label className="block text-sm">
          <span className="text-xs font-medium text-slate-500">Navigation map</span>
          <DropdownField
            value={selectedMapId}
            onValueChange={(value) => void selectRaiNavigationMap(value)}
            options={raiNavigationMaps
              .filter((map) => map.id != null)
              .map((map) => ({
                value: String(map.id),
                label: map.name ?? `Map #${map.id}`,
              }))}
            placeholder={raiNavigationAllowed ? "Select saved map" : "RAI Navigation not available on this device"}
          />
        </label>
        <label className="block text-sm">
          <span className="text-xs font-medium text-slate-500">Local planner</span>
          <DropdownField
            value={raiNavigationConfig.local_planner}
            onValueChange={(value) => void updateRaiNavigationConfig(value, raiNavigationConfig.global_planner)}
            options={raiNavigationLocalOptions}
          />
        </label>
        <label className="block text-sm">
          <span className="text-xs font-medium text-slate-500">Global planner</span>
          <DropdownField
            value={raiNavigationConfig.global_planner}
            onValueChange={(value) => void updateRaiNavigationConfig(raiNavigationConfig.local_planner, value)}
            options={raiNavigationGlobalOptions}
          />
        </label>
        <label className="block text-sm">
          <span className="text-xs font-medium text-slate-500">Map YAML path</span>
          <Input value={raiNavigationConfig.map_path} readOnly />
        </label>
      </div>

      <div className="mt-4 flex flex-wrap gap-2 max-sm:[&_[data-slot=button]]:flex-1">
        <Button type="button" className="gap-2" disabled={busy || !raiNavigationAllowed || raiNavigationConfig.running || !hasSelectedMap} onClick={() => void startRaiNavigation()}>
          <Play className="size-4" />
          Start RAI Navigation
        </Button>
        <Button type="button" variant="outline" className="gap-2" disabled={busy || !raiNavigationAllowed || !raiNavigationConfig.running} onClick={() => void stopRaiNavigation()}>
          <Square className="size-4" />
          Stop
        </Button>
      </div>

      <p className="mt-3 break-words text-sm text-slate-500">
        Running: {raiNavigationConfig.running ? "yes" : "no"} | {raiNavigationConfig.local_planner} + {raiNavigationConfig.global_planner}
      </p>
      {!raiNavigationAllowed && systemRuntime && (
        <p className="mt-2 text-sm text-amber-600">
          {operationMode === "sim"
            ? "RAI Navigation is disabled in Simulation mode."
            : `This API is connected to ${systemRuntime.device_label} (${systemRuntime.device_role}). RAI Navigation controls are Pi-only.`}
        </p>
      )}
      {!hasSelectedMap && <p className="mt-2 text-sm text-amber-600">Select a saved map before starting RAI Navigation.</p>}
      {message && <p className="mt-2 text-sm text-slate-500">{message}</p>}
    </section>
  );
}
