"use client";

import { useCallback, useEffect, useState } from "react";
import { fetchWithAuth } from "@/lib/api";
import type { MapPayload } from "@/types/robot-runtime";

export interface MapNavigationRuntime {
  maps: MapPayload[];
  currentMap: MapPayload | null;
  mapName: string;
  busy: boolean;
  status: string;
  setMapName: (value: string) => void;
  loadMaps: () => Promise<void>;
  saveMap: () => Promise<void>;
  selectSavedMap: (map: MapPayload) => Promise<void>;
  startSlam: () => Promise<void>;
  stopSlam: () => Promise<void>;
}

export function useMapNavigation(): MapNavigationRuntime {
  const [maps, setMaps] = useState<MapPayload[]>([]);
  const [currentMap, setCurrentMap] = useState<MapPayload | null>(null);
  const [mapName, setMapName] = useState("");
  const [busy, setBusy] = useState(false);
  const [status, setStatus] = useState("Waiting for map data.");

  const loadMaps = useCallback(async () => {
    try {
      const response = await fetchWithAuth("/api/map/list");
      const savedMaps = (await response.json()) as MapPayload[];
      setMaps(savedMaps);
      if (currentMap == null && savedMaps[0]?.id != null) {
        const mapResponse = await fetchWithAuth(`/api/map/${savedMaps[0].id}`);
        setCurrentMap((await mapResponse.json()) as MapPayload);
      }
    } catch {
      setMaps([]);
    }
  }, [currentMap]);

  useEffect(() => {
    const timer = window.setTimeout(() => {
      void loadMaps();
    }, 0);
    return () => window.clearTimeout(timer);
  }, [loadMaps]);

  const startSlam = useCallback(async () => {
    setBusy(true);
    try {
      const response = await fetchWithAuth("/api/robot/slam/start", { method: "POST" });
      const payload = await response.json();
      setStatus(payload.message ?? "SLAM started.");
    } catch (error) {
      setStatus(error instanceof Error ? error.message : "SLAM start failed.");
    } finally {
      setBusy(false);
    }
  }, []);

  const stopSlam = useCallback(async () => {
    setBusy(true);
    try {
      const response = await fetchWithAuth("/api/robot/slam/stop", { method: "POST" });
      const payload = await response.json();
      setStatus(payload.message ?? "SLAM stopped.");
    } catch (error) {
      setStatus(error instanceof Error ? error.message : "SLAM stop failed.");
    } finally {
      setBusy(false);
    }
  }, []);

  const saveMap = useCallback(async () => {
    const name = mapName.trim();
    if (!name) {
      setStatus("Map name is required.");
      return;
    }
    setBusy(true);
    try {
      await fetchWithAuth("/api/map/save", { method: "POST", body: JSON.stringify({ name }) });
      setMapName("");
      setStatus(`Saved map ${name}.`);
      await loadMaps();
    } catch (error) {
      setStatus(error instanceof Error ? error.message : "Save map failed.");
    } finally {
      setBusy(false);
    }
  }, [loadMaps, mapName]);

  const selectSavedMap = useCallback(async (map: MapPayload) => {
    if (map.id == null) return;
    const response = await fetchWithAuth(`/api/map/${map.id}`);
    const nextMap = (await response.json()) as MapPayload;
    setCurrentMap(nextMap);
    setStatus(`Loaded map ${nextMap.name ?? `#${map.id}`}.`);
  }, []);

  return {
    maps,
    currentMap,
    mapName,
    busy,
    status,
    setMapName,
    loadMaps,
    saveMap,
    selectSavedMap,
    startSlam,
    stopSlam,
  };
}
