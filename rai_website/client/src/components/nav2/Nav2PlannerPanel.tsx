"use client";

import { Button } from "@/components/ui/button";
import { DropdownField } from "@/components/ui/dropdown-field";
import { Input } from "@/components/ui/input";
import type { Nav2Config, Nav2Option } from "@/types/robot-runtime";
import { Play, RefreshCw, Square } from "lucide-react";

export interface Nav2PlannerPanelProps {
  busy: boolean;
  message?: string;
  nav2Config: Nav2Config;
  nav2Maps: Array<{ id?: number; name?: string; yaml_path?: string | null }>;
  selectedMapId: string;
  nav2GlobalOptions: Nav2Option[];
  nav2LocalOptions: Nav2Option[];
  loadNav2State?: () => Promise<void>;
  updateNav2Config: (localPlanner: string, globalPlanner: string, mapId?: number) => Promise<void>;
  selectNav2Map: (mapId: string) => Promise<void>;
  startNav2: () => Promise<void>;
  stopNav2: () => Promise<void>;
}

export function Nav2PlannerPanel({
  busy,
  message,
  nav2Config,
  nav2Maps,
  selectedMapId,
  nav2GlobalOptions,
  nav2LocalOptions,
  loadNav2State,
  updateNav2Config,
  selectNav2Map,
  startNav2,
  stopNav2,
}: Nav2PlannerPanelProps) {
  return (
    <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
      <div className="mb-3 flex items-center justify-between gap-3">
        <h3 className="text-sm font-bold uppercase tracking-wider text-slate-400">Nav2 Planner Selection</h3>
        {loadNav2State && (
          <Button type="button" variant="outline" size="sm" className="gap-2" disabled={busy} onClick={() => void loadNav2State()}>
            <RefreshCw className="size-4" />
            Refresh
          </Button>
        )}
      </div>

      <div className="grid gap-3">
        <label className="block text-sm">
          <span className="text-xs font-medium text-slate-500">Nav2 map</span>
          <DropdownField
            value={selectedMapId}
            onValueChange={(value) => void selectNav2Map(value)}
            options={nav2Maps
              .filter((map) => map.id != null)
              .map((map) => ({
                value: String(map.id),
                label: map.name ?? `Map #${map.id}`,
              }))}
            placeholder="Select saved map"
          />
        </label>
        <label className="block text-sm">
          <span className="text-xs font-medium text-slate-500">Local planner</span>
          <DropdownField
            value={nav2Config.local_planner}
            onValueChange={(value) => void updateNav2Config(value, nav2Config.global_planner)}
            options={nav2LocalOptions}
          />
        </label>
        <label className="block text-sm">
          <span className="text-xs font-medium text-slate-500">Global planner</span>
          <DropdownField
            value={nav2Config.global_planner}
            onValueChange={(value) => void updateNav2Config(nav2Config.local_planner, value)}
            options={nav2GlobalOptions}
          />
        </label>
        <label className="block text-sm">
          <span className="text-xs font-medium text-slate-500">Map YAML path</span>
          <Input value={nav2Config.map_path} readOnly />
        </label>
      </div>

      <div className="mt-4 flex gap-2">
        <Button type="button" className="gap-2" disabled={busy || nav2Config.running} onClick={() => void startNav2()}>
          <Play className="size-4" />
          Start Nav2
        </Button>
        <Button type="button" variant="outline" className="gap-2" disabled={busy || !nav2Config.running} onClick={() => void stopNav2()}>
          <Square className="size-4" />
          Stop
        </Button>
      </div>

      <p className="mt-3 text-sm text-slate-500">
        Running: {nav2Config.running ? "yes" : "no"} | {nav2Config.local_planner} + {nav2Config.global_planner}
      </p>
      {message && <p className="mt-2 text-sm text-slate-500">{message}</p>}
    </section>
  );
}
