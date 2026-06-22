"use client";

import { useCallback, useEffect, useState } from "react";
import { fetchWithAuth } from "@/lib/api";
import { useOperationMode } from "@/contexts/OperationModeContext";
import { readSelectedMapId, writeSelectedMapId } from "@/lib/map-selection";
import type { RaiNavigationConfig, RaiNavigationOption, SystemRuntime } from "@/types/robot-runtime";

export interface RaiNavigationControlRuntime {
  busy: boolean;
  message: string;
  systemRuntime: SystemRuntime | null;
  raiNavigationConfig: RaiNavigationConfig;
  raiNavigationMaps: Array<{ id?: number; name?: string; yaml_path?: string | null }>;
  selectedMapId: string;
  raiNavigationGlobalOptions: RaiNavigationOption[];
  raiNavigationLocalOptions: RaiNavigationOption[];
  loadRaiNavigationState: () => Promise<void>;
  updateRaiNavigationConfig: (localPlanner: string, globalPlanner: string, mapId?: number) => Promise<void>;
  selectRaiNavigationMap: (mapId: string) => Promise<void>;
  startRaiNavigation: () => Promise<void>;
  stopRaiNavigation: () => Promise<void>;
}

interface RaiNavigationOptionsResponse {
  local_planners: RaiNavigationOption[];
  global_planners: RaiNavigationOption[];
}

const DEFAULT_RAI_NAVIGATION_CONFIG: RaiNavigationConfig = {
  local_planner: "CCA_NMPC",
  global_planner: "A_STAR",
  map_path: "",
  running: false,
};

export function useRaiNavigationControl(): RaiNavigationControlRuntime {
  const { runtime } = useOperationMode();
  const [busy, setBusy] = useState(false);
  const [message, setMessage] = useState("Waiting for RAI Navigation state.");
  const [systemRuntime, setSystemRuntime] = useState<SystemRuntime | null>(null);
  const [raiNavigationConfig, setRaiNavigationConfig] = useState<RaiNavigationConfig>(DEFAULT_RAI_NAVIGATION_CONFIG);
  const [raiNavigationMaps, setRaiNavigationMaps] = useState<Array<{ id?: number; name?: string; yaml_path?: string | null }>>([]);
  const [selectedMapId, setSelectedMapId] = useState("");
  const [raiNavigationLocalOptions, setRaiNavigationLocalOptions] = useState<RaiNavigationOption[]>([]);
  const [raiNavigationGlobalOptions, setRaiNavigationGlobalOptions] = useState<RaiNavigationOption[]>([]);

  const loadRaiNavigationState = useCallback(async () => {
    try {
      setSystemRuntime(runtime);
      if (!runtime || !runtime.allowed_actions.includes("navigation")) {
        setRaiNavigationMaps([]);
        setSelectedMapId("");
        setRaiNavigationLocalOptions([]);
        setRaiNavigationGlobalOptions([]);
        setRaiNavigationConfig(DEFAULT_RAI_NAVIGATION_CONFIG);
        setMessage(runtime ? `RAI Navigation control is disabled on ${runtime.device_label} (${runtime.device_role}).` : "Waiting for system runtime.");
        return;
      }
      const [optionsResponse, configResponse, mapsResponse] = await Promise.all([
        fetchWithAuth("/api/rai-navigation/options"),
        fetchWithAuth("/api/rai-navigation/config"),
        fetchWithAuth("/api/map/list"),
      ]);
      const options = (await optionsResponse.json()) as RaiNavigationOptionsResponse;
      setRaiNavigationLocalOptions(options.local_planners ?? []);
      setRaiNavigationGlobalOptions(options.global_planners ?? []);
      const config = (await configResponse.json()) as RaiNavigationConfig;
      const maps = (await mapsResponse.json()) as Array<{ id?: number; name?: string; yaml_path?: string | null }>;
      const storedMapId = readSelectedMapId();
      const matchedMapId = maps.find((map) => map.yaml_path === config.map_path)?.id ?? null;
      const resolvedMapId =
        storedMapId != null && maps.some((map) => map.id === storedMapId)
          ? storedMapId
          : (config.selected_map_id ?? matchedMapId);

      setRaiNavigationMaps(maps);
      setSelectedMapId(resolvedMapId != null ? String(resolvedMapId) : "");
      setRaiNavigationConfig(config);
      setMessage(`RAI Navigation ${config.running ? "running" : "idle"} on ${config.local_planner} + ${config.global_planner}.`);
    } catch (error) {
      setRaiNavigationMaps([]);
      setSelectedMapId("");
      setRaiNavigationLocalOptions([]);
      setRaiNavigationGlobalOptions([]);
      setMessage(error instanceof Error ? error.message : "Cannot load RAI Navigation state.");
    }
  }, [runtime]);

  useEffect(() => {
    const timer = window.setTimeout(() => {
      void loadRaiNavigationState();
    }, 0);
    return () => window.clearTimeout(timer);
  }, [loadRaiNavigationState]);

  const updateRaiNavigationConfig = useCallback(
    async (nextLocalPlanner: string, nextGlobalPlanner: string, mapId?: number) => {
      setBusy(true);
      try {
        const effectiveMapId =
          mapId ?? (selectedMapId ? Number.parseInt(selectedMapId, 10) : undefined);
        const payload: { local_planner: string; global_planner: string; map_path?: string; map_id?: number } = {
          local_planner: nextLocalPlanner,
          global_planner: nextGlobalPlanner,
        };
        if (effectiveMapId != null && Number.isFinite(effectiveMapId)) payload.map_id = effectiveMapId;
        else payload.map_path = raiNavigationConfig.map_path;

        const response = await fetchWithAuth("/api/rai-navigation/config", {
          method: "POST",
          body: JSON.stringify(payload),
        });
        const config = (await response.json()) as RaiNavigationConfig;
        setRaiNavigationConfig(config);
        if (effectiveMapId != null && Number.isFinite(effectiveMapId)) {
          setSelectedMapId(String(effectiveMapId));
          writeSelectedMapId(effectiveMapId);
        }
        setMessage(`RAI Navigation config updated: ${config.local_planner} + ${config.global_planner}.`);
      } catch (error) {
        setMessage(error instanceof Error ? error.message : "Cannot update RAI Navigation config.");
      } finally {
        setBusy(false);
      }
    },
    [raiNavigationConfig.map_path, selectedMapId],
  );

  const selectRaiNavigationMap = useCallback(
    async (mapId: string) => {
      setSelectedMapId(mapId);
      const parsedMapId = Number.parseInt(mapId, 10);
      if (!Number.isFinite(parsedMapId)) return;
      writeSelectedMapId(parsedMapId);
      await updateRaiNavigationConfig(raiNavigationConfig.local_planner, raiNavigationConfig.global_planner, parsedMapId);
    },
    [raiNavigationConfig.global_planner, raiNavigationConfig.local_planner, updateRaiNavigationConfig],
  );

  const startRaiNavigation = useCallback(async () => {
    setBusy(true);
    try {
      const parsedMapId = selectedMapId ? Number.parseInt(selectedMapId, 10) : NaN;
      if (!Number.isFinite(parsedMapId)) {
        throw new Error("No RAI Navigation map selected. Choose a saved map before starting RAI Navigation.");
      }
      await fetchWithAuth("/api/rai-navigation/config", {
        method: "POST",
        body: JSON.stringify({
          local_planner: raiNavigationConfig.local_planner,
          global_planner: raiNavigationConfig.global_planner,
          map_id: parsedMapId,
        }),
      });
      const response = await fetchWithAuth("/api/rai-navigation/start", { method: "POST" });
      const config = (await response.json()) as RaiNavigationConfig;
      setRaiNavigationConfig(config);
      setMessage(`RAI Navigation started with ${config.local_planner} / ${config.global_planner}.`);
    } catch (error) {
      setMessage(error instanceof Error ? error.message : "RAI Navigation start failed.");
    } finally {
      setBusy(false);
    }
  }, [raiNavigationConfig.global_planner, raiNavigationConfig.local_planner, selectedMapId]);

  const stopRaiNavigation = useCallback(async () => {
    setBusy(true);
    try {
      const response = await fetchWithAuth("/api/rai-navigation/stop", { method: "POST" });
      const config = (await response.json()) as RaiNavigationConfig;
      setRaiNavigationConfig(config);
      setMessage("RAI Navigation stopped.");
    } catch (error) {
      setMessage(error instanceof Error ? error.message : "RAI Navigation stop failed.");
    } finally {
      setBusy(false);
    }
  }, []);

  return {
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
  };
}
