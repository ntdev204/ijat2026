"use client";

import { Button } from "@/components/ui/button";
import { useGazeboRuntime } from "@/hooks/useGazeboRuntime";
import type { MapPayload, PathsPayload } from "@/types/robot-runtime";
import type { RobotUiTelemetry } from "@/lib/robot-telemetry";
import { Bot, Camera, Map as MapIcon, RefreshCw, Users, Waypoints } from "lucide-react";
import type { RefObject } from "react";

export default function GazeboPage() {
  const gazebo = useGazeboRuntime();
  const { currentMap, loadMaps, maps, mountRef, paths, selectSavedMap, setViewMode, status, telemetry, viewMode } = gazebo;

  return (
    <div className="space-y-5">
      <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
        <div>
          <h2 className="text-2xl font-bold text-slate-800 tracking-tight">Gazebo View</h2>
          <p className="text-sm text-slate-500">3D URDF robot over the scanned lidar map for paper-ready screenshots.</p>
        </div>
        <div className="flex gap-2">
          <Button variant={viewMode === "gazebo" ? "default" : "outline"} className="gap-2" onClick={() => setViewMode("gazebo")}>
            <Camera className="size-4" />
            Gazebo
          </Button>
          <Button variant={viewMode === "top" ? "default" : "outline"} className="gap-2" onClick={() => setViewMode("top")}>
            <MapIcon className="size-4" />
            Top
          </Button>
          <Button variant="outline" className="gap-2" onClick={() => void loadMaps()}>
            <RefreshCw className="size-4" />
            Refresh
          </Button>
        </div>
      </div>

      <div className="grid grid-cols-1 gap-5 xl:grid-cols-[minmax(0,1fr)_320px]">
        <GazeboScene mountRef={mountRef} />
        <GazeboSidebar
          currentMap={currentMap}
          maps={maps}
          paths={paths}
          selectSavedMap={selectSavedMap}
          status={status}
          telemetry={telemetry}
        />
      </div>
    </div>
  );
}

interface GazeboSceneProps {
  mountRef: RefObject<HTMLDivElement | null>;
}

function GazeboScene({ mountRef }: GazeboSceneProps) {
  return (
    <section className="overflow-hidden rounded-lg border border-slate-200 bg-white shadow-sm">
      <div className="flex items-center gap-2 border-b border-slate-200 px-4 py-3 text-sm text-slate-600">
        <MapIcon className="size-4 text-blue-600" />
        <span className="font-medium text-slate-800">Gazebo-like Scene</span>
      </div>
      <div ref={mountRef} className="h-[720px] w-full bg-slate-100" />
    </section>
  );
}

interface GazeboSidebarProps {
  currentMap: MapPayload | null;
  maps: MapPayload[];
  paths: PathsPayload;
  selectSavedMap: (map: MapPayload) => Promise<void>;
  status: string;
  telemetry: RobotUiTelemetry | null;
}

function GazeboSidebar({ currentMap, maps, paths, selectSavedMap, status, telemetry }: GazeboSidebarProps) {
  return (
    <aside className="space-y-4">
      <RobotPanel currentMap={currentMap} />
      <RuntimePanel paths={paths} telemetry={telemetry} />
      <SavedMapsPanel maps={maps} selectSavedMap={selectSavedMap} />
      <section className="rounded-lg border border-slate-200 bg-white p-4 text-sm text-slate-600 shadow-sm">{status}</section>
    </aside>
  );
}

interface RobotPanelProps {
  currentMap: MapPayload | null;
}

function RobotPanel({ currentMap }: RobotPanelProps) {
  return (
    <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
      <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">Robot</h3>
      <div className="space-y-2 text-sm text-slate-600">
        <div className="flex items-center gap-2">
          <Bot className="size-4 text-emerald-600" />
          <span>URDF: `mini_mec_robot.urdf`</span>
        </div>
        <div className="flex items-center gap-2">
          <MapIcon className="size-4 text-blue-600" />
          <span>{currentMap ? `${currentMap.width}x${currentMap.height} @ ${currentMap.resolution}m` : "No map loaded"}</span>
        </div>
      </div>
    </section>
  );
}

interface RuntimePanelProps {
  paths: PathsPayload;
  telemetry: RobotUiTelemetry | null;
}

function RuntimePanel({ paths, telemetry }: RuntimePanelProps) {
  return (
    <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
      <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">Runtime</h3>
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

interface SavedMapsPanelProps {
  maps: MapPayload[];
  selectSavedMap: (map: MapPayload) => Promise<void>;
}

function SavedMapsPanel({ maps, selectSavedMap }: SavedMapsPanelProps) {
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
