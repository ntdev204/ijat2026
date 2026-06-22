"use client";

import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { useOperationMode } from "@/contexts/OperationModeContext";
import { useMapNavigation } from "@/hooks/useMapNavigation";
import type { MapPayload } from "@/types/robot-runtime";
import { Map as MapIcon, Navigation, Pencil, Radar, RefreshCw, Save, Square, Trash2, X } from "lucide-react";
import { useState } from "react";

export default function MapPage() {
  const { operationMode } = useOperationMode();
  const runtime = useMapNavigation();
  const {
    busy,
    canvasRef,
    currentMap,
    deleteSavedMap,
    loadMaps,
    mapName,
    maps,
    renameSavedMap,
    saveMap,
    selectSavedMap,
    setMapName,
    slamRunning,
    startSlam,
    status,
    stopSlam,
    systemRuntime,
  } = runtime;
  const slamAllowed = (systemRuntime?.allowed_actions.includes("slam") ?? true) && operationMode !== "sim";

  return (
    <div className="space-y-5">
      <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
        <div>
          <h2 className="text-2xl font-bold text-slate-800 tracking-tight">Map</h2>
          <p className="text-sm text-slate-500">
            {currentMap ? `${currentMap.width}x${currentMap.height} @ ${currentMap.resolution}m` : status}
          </p>
          <p className="mt-1 text-xs text-slate-500">
            {operationMode === "sim" ? "SLAM and map saving are disabled in Simulation mode." : "Map controls follow the shared system mode."}
          </p>
          {systemRuntime && (
            <p className="mt-1 text-xs text-slate-500">
              API target: {systemRuntime.device_label} ({systemRuntime.device_role})
            </p>
          )}
        </div>
        <Button variant="outline" className="gap-2 self-start sm:self-auto" disabled={busy} onClick={() => void loadMaps()}>
          <RefreshCw className="size-4" />
          Refresh
        </Button>
      </div>

      <div className="grid grid-cols-1 gap-5 xl:grid-cols-[minmax(0,1fr)_360px]">
        <section className="overflow-hidden rounded-lg border border-slate-200 bg-white shadow-sm">
          <div className="flex items-center gap-2 border-b border-slate-200 px-4 py-3 text-sm text-slate-600">
            <MapIcon className="size-4 text-blue-600" />
            <span className="font-medium text-slate-800">Mapping Workspace</span>
          </div>
          <div className="flex min-h-[320px] items-center justify-center overflow-auto bg-slate-100 p-3 sm:min-h-[520px] sm:p-4">
            {currentMap ? (
              <canvas
                ref={canvasRef}
                style={{ aspectRatio: `${currentMap.width} / ${currentMap.height}` }}
                className="block h-auto w-full max-w-full rounded-md border border-slate-300 bg-white shadow-sm [image-rendering:pixelated]"
              />
            ) : (
              <div className="text-center text-sm text-slate-500">
                Waiting for live /map data while SLAM is running.
              </div>
            )}
          </div>
          <div className="border-t border-slate-200 px-4 py-3 text-sm text-slate-500">
            {status} This page is only for SLAM, scan updates, and saving maps.
          </div>
        </section>

        <aside className="space-y-4">
          <SlamPanel busy={busy} slamAllowed={slamAllowed} slamRunning={slamRunning} startSlam={startSlam} stopSlam={stopSlam} />
          <SaveMapPanel busy={busy} mapName={mapName} status={status} setMapName={setMapName} saveMap={saveMap} />
          <SavedMapsPanel
            busy={busy}
            maps={maps}
            deleteSavedMap={deleteSavedMap}
            renameSavedMap={renameSavedMap}
            selectSavedMap={selectSavedMap}
          />
        </aside>
      </div>
    </div>
  );
}

interface SlamPanelProps {
  busy: boolean;
  slamAllowed: boolean;
  slamRunning: boolean;
  startSlam: () => Promise<void>;
  stopSlam: () => Promise<void>;
}

function SlamPanel({ busy, slamAllowed, slamRunning, startSlam, stopSlam }: SlamPanelProps) {
  return (
    <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
      <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">SLAM / Scan</h3>
      <div className="flex flex-wrap gap-2 max-sm:[&_[data-slot=button]]:flex-1">
        <Button type="button" className="gap-2" disabled={busy || !slamAllowed || slamRunning} onClick={() => void startSlam()}>
          <Radar className="size-4" />
          Start SLAM
        </Button>
        <Button type="button" variant="outline" className="gap-2" disabled={busy || !slamAllowed || !slamRunning} onClick={() => void stopSlam()}>
          <Square className="size-4" />
          Stop
        </Button>
      </div>
      <p className="mt-3 text-sm text-slate-500">
        {!slamAllowed
          ? "SLAM controls are only available when the web UI targets the Pi API."
          : slamRunning
            ? "SLAM is running. Watch the live map update before saving."
            : "Start SLAM to scan and update the live map."}
      </p>
    </section>
  );
}

interface SaveMapPanelProps {
  busy: boolean;
  mapName: string;
  status: string;
  setMapName: (value: string) => void;
  saveMap: () => Promise<void>;
}

function SaveMapPanel({ busy, mapName, status, setMapName, saveMap }: SaveMapPanelProps) {
  return (
    <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
      <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">Save Live Map</h3>
      <div className="flex gap-2">
        <Input value={mapName} onChange={(event) => setMapName(event.target.value)} placeholder="Map name" disabled={busy} />
        <Button type="button" size="icon" disabled={busy} onClick={() => void saveMap()} title="Save map">
          <Save className="size-4" />
        </Button>
      </div>
      <p className="mt-3 text-sm text-slate-500">{status}</p>
    </section>
  );
}

interface SavedMapsPanelProps {
  busy: boolean;
  maps: MapPayload[];
  deleteSavedMap: (map: MapPayload) => Promise<void>;
  renameSavedMap: (map: MapPayload, name: string) => Promise<void>;
  selectSavedMap: (map: MapPayload) => Promise<void>;
}

function SavedMapsPanel({ busy, maps, deleteSavedMap, renameSavedMap, selectSavedMap }: SavedMapsPanelProps) {
  const [editingId, setEditingId] = useState<number | null>(null);
  const [draftName, setDraftName] = useState("");

  function startEditing(map: MapPayload) {
    setEditingId(map.id ?? null);
    setDraftName(map.name ?? "");
  }

  function cancelEditing() {
    setEditingId(null);
    setDraftName("");
  }

  async function submitRename(map: MapPayload) {
    await renameSavedMap(map, draftName);
    cancelEditing();
  }

  return (
    <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
      <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">Saved Maps</h3>
      <div className="space-y-2">
        {maps.length === 0 ? (
          <p className="text-sm text-slate-500">No saved maps.</p>
        ) : (
          maps.map((item) => (
            <div key={item.id ?? item.name} className="rounded-md border border-slate-200 bg-white p-3">
              <button type="button" className="w-full text-left" onClick={() => void selectSavedMap(item)}>
                <div className="flex items-center gap-2 font-semibold text-slate-800">
                  <Navigation className="size-4 text-blue-600" />
                  <span className="truncate">{item.name ?? `Map #${item.id}`}</span>
                </div>
                <p className="mt-1 text-xs text-slate-400">
                  {item.width}x{item.height}
                </p>
                <p className="mt-1 truncate text-xs text-slate-400">{item.yaml_path ?? "No YAML exported"}</p>
              </button>

              {editingId === item.id ? (
                <div className="mt-3 flex gap-2">
                  <Input value={draftName} onChange={(event) => setDraftName(event.target.value)} disabled={busy} />
                  <Button type="button" size="icon" disabled={busy || !draftName.trim()} onClick={() => void submitRename(item)} title="Save name">
                    <Save className="size-4" />
                  </Button>
                  <Button type="button" variant="outline" size="icon" disabled={busy} onClick={cancelEditing} title="Cancel edit">
                    <X className="size-4" />
                  </Button>
                </div>
              ) : (
                <div className="mt-3 flex flex-wrap gap-2 max-sm:[&_[data-slot=button]]:flex-1">
                  <Button type="button" variant="outline" size="sm" className="gap-2" disabled={busy || item.id == null} onClick={() => startEditing(item)}>
                    <Pencil className="size-4" />
                    Rename
                  </Button>
                  <Button type="button" variant="outline" size="sm" className="gap-2" disabled={busy || item.id == null} onClick={() => void deleteSavedMap(item)}>
                    <Trash2 className="size-4" />
                    Delete
                  </Button>
                </div>
              )}
            </div>
          ))
        )}
      </div>
    </section>
  );
}
