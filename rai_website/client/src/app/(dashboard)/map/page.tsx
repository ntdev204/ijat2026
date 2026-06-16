"use client";

import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { useMapNavigation } from "@/hooks/useMapNavigation";
import type { MapPayload } from "@/types/robot-runtime";
import { Map as MapIcon, Navigation, Radar, RefreshCw, Save, Square } from "lucide-react";

export default function MapPage() {
  const runtime = useMapNavigation();
  const { busy, currentMap, loadMaps, mapName, maps, saveMap, selectSavedMap, setMapName, startSlam, status, stopSlam } = runtime;

  return (
    <div className="space-y-5">
      <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
        <div>
          <h2 className="text-2xl font-bold text-slate-800 tracking-tight">Map</h2>
          <p className="text-sm text-slate-500">
            {currentMap ? `${currentMap.width}x${currentMap.height} @ ${currentMap.resolution}m` : status}
          </p>
        </div>
        <Button variant="outline" className="gap-2" disabled={busy} onClick={() => void loadMaps()}>
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
          <div className="flex min-h-[520px] items-center justify-center bg-slate-100 p-6 text-center text-sm text-slate-500">
            Use this page only for SLAM, scan updates, and saving maps. Route planning and anchor control now live in Monitor.
          </div>
          <div className="border-t border-slate-200 px-4 py-3 text-sm text-slate-500">{status}</div>
        </section>

        <aside className="space-y-4">
          <SlamPanel busy={busy} startSlam={startSlam} stopSlam={stopSlam} />
          <SaveMapPanel busy={busy} mapName={mapName} status={status} setMapName={setMapName} saveMap={saveMap} />
          <SavedMapsPanel maps={maps} selectSavedMap={selectSavedMap} />
        </aside>
      </div>
    </div>
  );
}

interface SlamPanelProps {
  busy: boolean;
  startSlam: () => Promise<void>;
  stopSlam: () => Promise<void>;
}

function SlamPanel({ busy, startSlam, stopSlam }: SlamPanelProps) {
  return (
    <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
      <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">SLAM / Scan</h3>
      <div className="flex gap-2">
        <Button type="button" className="gap-2" disabled={busy} onClick={() => void startSlam()}>
          <Radar className="size-4" />
          Start SLAM
        </Button>
        <Button type="button" variant="outline" className="gap-2" disabled={busy} onClick={() => void stopSlam()}>
          <Square className="size-4" />
          Stop
        </Button>
      </div>
      <p className="mt-3 text-sm text-slate-500">Use this mode to scan and update the live map before saving it for Nav2 reuse.</p>
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
  maps: MapPayload[];
  selectSavedMap: (map: MapPayload) => Promise<void>;
}

function SavedMapsPanel({ maps, selectSavedMap }: SavedMapsPanelProps) {
  return (
    <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
      <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">Saved Maps</h3>
      <div className="space-y-2">
        {maps.length === 0 ? (
          <p className="text-sm text-slate-500">No saved maps.</p>
        ) : (
          maps.map((item) => (
            <button
              key={item.id ?? item.name}
              type="button"
              className="w-full rounded-md border border-slate-200 bg-white p-3 text-left hover:bg-slate-50"
              onClick={() => void selectSavedMap(item)}
            >
              <div className="flex items-center gap-2 font-semibold text-slate-800">
                <Navigation className="size-4 text-blue-600" />
                <span className="truncate">{item.name ?? `Map #${item.id}`}</span>
              </div>
              <p className="mt-1 text-xs text-slate-400">
                {item.width}x{item.height}
              </p>
              <p className="mt-1 truncate text-xs text-slate-400">{item.yaml_path ?? "No YAML exported"}</p>
            </button>
          ))
        )}
      </div>
    </section>
  );
}
