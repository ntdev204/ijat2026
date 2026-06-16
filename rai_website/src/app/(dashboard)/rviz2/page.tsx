"use client";

import { Button } from "@/components/ui/button";
import { useRvizRuntime } from "@/hooks/useRvizRuntime";
import type { MapPayload, PathsPayload } from "@/types/robot-runtime";
import type { RobotUiTelemetry } from "@/lib/robot-telemetry";
import { Bot, Compass, Map as MapIcon, RefreshCw, ScanSearch, Users, Waypoints } from "lucide-react";
import type { RefObject } from "react";

export default function Rviz2Page() {
  const rviz = useRvizRuntime();
  const { currentMap, loadMaps, maps, mountRef, paths, selectSavedMap, setViewMode, status, telemetry, viewMode } = rviz;

  return (
    <div className="space-y-5">
      <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
        <div>
          <h2 className="text-2xl font-bold tracking-tight text-slate-800">RViz2 View</h2>
          <p className="text-sm text-slate-500">Map, TF-style robot model, planners, and live scene overlays in an RViz-oriented viewer.</p>
        </div>
        <div className="flex flex-wrap gap-2">
          <Button variant={viewMode === "perspective" ? "default" : "outline"} className="gap-2" onClick={() => setViewMode("perspective")}>
            <Compass className="size-4" />
            Perspective
          </Button>
          <Button variant={viewMode === "top" ? "default" : "outline"} className="gap-2" onClick={() => setViewMode("top")}>
            <MapIcon className="size-4" />
            Top
          </Button>
          <Button variant={viewMode === "follow" ? "default" : "outline"} className="gap-2" onClick={() => setViewMode("follow")}>
            <ScanSearch className="size-4" />
            Follow
          </Button>
          <Button variant="outline" className="gap-2" onClick={() => void loadMaps()}>
            <RefreshCw className="size-4" />
            Refresh
          </Button>
        </div>
      </div>

      <div className="grid grid-cols-1 gap-5 xl:grid-cols-[minmax(0,1fr)_340px]">
        <RvizScene mountRef={mountRef} />
        <RvizSidebar currentMap={currentMap} maps={maps} paths={paths} selectSavedMap={selectSavedMap} status={status} telemetry={telemetry} />
      </div>
    </div>
  );
}

interface RvizSceneProps {
  mountRef: RefObject<HTMLDivElement | null>;
}

function RvizScene({ mountRef }: RvizSceneProps) {
  return (
    <section className="overflow-hidden rounded-lg border border-slate-200 bg-white shadow-sm">
      <div className="flex items-center gap-2 border-b border-slate-200 px-4 py-3 text-sm text-slate-600">
        <MapIcon className="size-4 text-blue-600" />
        <span className="font-medium text-slate-800">RViz2 Scene</span>
        <span className="ml-auto text-xs text-slate-400">Grid + Map + TF + Path + Humans</span>
      </div>
      <div ref={mountRef} className="h-[760px] w-full bg-slate-100" />
    </section>
  );
}

interface RvizSidebarProps {
  currentMap: MapPayload | null;
  maps: MapPayload[];
  paths: PathsPayload;
  selectSavedMap: (map: MapPayload) => Promise<void>;
  status: string;
  telemetry: RobotUiTelemetry | null;
}

function RvizSidebar({ currentMap, maps, paths, selectSavedMap, status, telemetry }: RvizSidebarProps) {
  return (
    <aside className="space-y-4">
      <RobotPanel currentMap={currentMap} telemetry={telemetry} />
      <RuntimePanel paths={paths} telemetry={telemetry} />
      <SavedMapsPanel maps={maps} selectSavedMap={selectSavedMap} />
      <section className="rounded-lg border border-slate-200 bg-white p-4 text-sm text-slate-600 shadow-sm">{status}</section>
    </aside>
  );
}

function RobotPanel({ currentMap, telemetry }: { currentMap: MapPayload | null; telemetry: RobotUiTelemetry | null }) {
  return (
    <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
      <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">Robot Model</h3>
      <div className="space-y-2 text-sm text-slate-600">
        <div className="flex items-center gap-2">
          <Bot className="size-4 text-emerald-600" />
          <span>URDF: `mini_mec_robot.urdf` with preserved `base_link` origin</span>
        </div>
        <div className="flex items-center gap-2">
          <MapIcon className="size-4 text-blue-600" />
          <span>{currentMap ? `${currentMap.width}x${currentMap.height} @ ${currentMap.resolution}m` : "No map loaded"}</span>
        </div>
        <div className="rounded-md border border-slate-200 bg-slate-50 px-3 py-2 text-xs text-slate-500">
          Pose: x={formatPose(telemetry?.map_pose.x)}, y={formatPose(telemetry?.map_pose.y)}, yaw={formatPose(telemetry?.map_pose.yaw)}
        </div>
      </div>
    </section>
  );
}

function RuntimePanel({ paths, telemetry }: { paths: PathsPayload; telemetry: RobotUiTelemetry | null }) {
  return (
    <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
      <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">Overlays</h3>
      <div className="space-y-2 text-sm text-slate-600">
        <div className="flex items-center gap-2">
          <Waypoints className="size-4 text-blue-600" />
          <span>Global plan: {paths.global_plan?.length ?? 0}</span>
        </div>
        <div className="flex items-center gap-2">
          <Waypoints className="size-4 text-amber-600" />
          <span>Local plan: {paths.local_plan?.length ?? 0}</span>
        </div>
        <div className="flex items-center gap-2">
          <Users className="size-4 text-rose-600" />
          <span>Humans: {telemetry?.humans.length ?? 0}</span>
        </div>
      </div>
    </section>
  );
}

function SavedMapsPanel({ maps, selectSavedMap }: { maps: MapPayload[]; selectSavedMap: (map: MapPayload) => Promise<void> }) {
  return (
    <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
      <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">Saved Maps</h3>
      <div className="space-y-2">
        {maps.map((item) => (
          <button
            key={item.id ?? item.name}
            type="button"
            className="w-full rounded-md border border-slate-200 bg-white p-3 text-left hover:bg-slate-50"
            onClick={() => void selectSavedMap(item)}
          >
            <div className="font-semibold text-slate-800">{item.name ?? `Map #${item.id}`}</div>
            <div className="mt-1 text-xs text-slate-400">
              {item.width}x{item.height}
            </div>
            <div className="mt-1 truncate text-xs text-slate-400">{item.yaml_path ?? "-"}</div>
          </button>
        ))}
      </div>
    </section>
  );
}

function formatPose(value: number | null | undefined) {
  return typeof value === "number" && Number.isFinite(value) ? value.toFixed(3) : "-";
}
