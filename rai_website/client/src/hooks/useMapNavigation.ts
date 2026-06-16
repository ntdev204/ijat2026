"use client";

import { useCallback, useEffect, useRef, useState, type RefObject } from "react";
import { useWebSocket } from "@/hooks/useWebSocket";
import { fetchWithAuth } from "@/lib/api";
import { drawOccupancyMap } from "@/lib/map-canvas";
import type { MapPayload } from "@/types/robot-runtime";

export interface MapNavigationRuntime {
  canvasRef: RefObject<HTMLCanvasElement | null>;
  maps: MapPayload[];
  currentMap: MapPayload | null;
  mapName: string;
  busy: boolean;
  slamRunning: boolean;
  status: string;
  setMapName: (value: string) => void;
  loadMaps: () => Promise<void>;
  saveMap: () => Promise<void>;
  selectSavedMap: (map: MapPayload) => Promise<void>;
  renameSavedMap: (map: MapPayload, name: string) => Promise<void>;
  deleteSavedMap: (map: MapPayload) => Promise<void>;
  startSlam: () => Promise<void>;
  stopSlam: () => Promise<void>;
}

export function useMapNavigation(): MapNavigationRuntime {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const [maps, setMaps] = useState<MapPayload[]>([]);
  const [currentMap, setCurrentMap] = useState<MapPayload | null>(null);
  const [mapName, setMapName] = useState("");
  const [busy, setBusy] = useState(false);
  const [slamRunning, setSlamRunning] = useState(false);
  const [status, setStatus] = useState("Waiting for map data.");

  useWebSocket("/ws/map", {
    onMessage: (event) => {
      try {
        setCurrentMap(JSON.parse(String(event.data)) as MapPayload);
        setStatus("Live SLAM map updated.");
      } catch {
        setStatus("Invalid live map payload.");
      }
    },
  });

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

  useEffect(() => {
    if (!canvasRef.current || !currentMap) return;
    drawOccupancyMap(canvasRef.current, currentMap, { showRobot: false });
  }, [currentMap]);

  const startSlam = useCallback(async () => {
    setBusy(true);
    try {
      const response = await fetchWithAuth("/api/robot/slam/start", { method: "POST" });
      const payload = await response.json();
      setStatus(payload.message ?? "SLAM started.");
      setSlamRunning(true);
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
      setSlamRunning(false);
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

  const renameSavedMap = useCallback(
    async (map: MapPayload, name: string) => {
      const nextName = name.trim();
      if (map.id == null || !nextName) return;
      setBusy(true);
      try {
        await fetchWithAuth(`/api/map/${map.id}`, {
          method: "PATCH",
          body: JSON.stringify({ name: nextName }),
        });
        setStatus(`Renamed map to ${nextName}.`);
        await loadMaps();
      } catch (error) {
        setStatus(error instanceof Error ? error.message : "Rename map failed.");
      } finally {
        setBusy(false);
      }
    },
    [loadMaps],
  );

  const deleteSavedMap = useCallback(
    async (map: MapPayload) => {
      if (map.id == null) return;
      setBusy(true);
      try {
        await fetchWithAuth(`/api/map/${map.id}/delete`, { method: "POST" });
        setMaps((items) => items.filter((item) => item.id !== map.id));
        setCurrentMap((current) => (current?.id === map.id ? null : current));
        setStatus(`Deleted map ${map.name ?? `#${map.id}`}.`);
      } catch (error) {
        setStatus(error instanceof Error ? error.message : "Delete map failed.");
      } finally {
        setBusy(false);
      }
    },
    [],
  );

  return {
    canvasRef,
    maps,
    currentMap,
    mapName,
    busy,
    slamRunning,
    status,
    setMapName,
    loadMaps,
    saveMap,
    selectSavedMap,
    renameSavedMap,
    deleteSavedMap,
    startSlam,
    stopSlam,
  };
}
