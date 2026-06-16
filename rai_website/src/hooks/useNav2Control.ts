"use client";

import { useCallback, useEffect, useState } from "react";
import { fetchWithAuth } from "@/lib/api";
import { readSelectedMapId, writeSelectedMapId } from "@/lib/map-selection";
import type { Nav2Config, Nav2Option } from "@/types/robot-runtime";

export interface Nav2ControlRuntime {
  busy: boolean;
  message: string;
  nav2Config: Nav2Config;
  nav2Maps: Array<{ id?: number; name?: string; yaml_path?: string | null }>;
  selectedMapId: string;
  nav2GlobalOptions: Nav2Option[];
  nav2LocalOptions: Nav2Option[];
  loadNav2State: () => Promise<void>;
  updateNav2Config: (localPlanner: string, globalPlanner: string, mapId?: number) => Promise<void>;
  selectNav2Map: (mapId: string) => Promise<void>;
  startNav2: () => Promise<void>;
  stopNav2: () => Promise<void>;
}

const DEFAULT_NAV2_CONFIG: Nav2Config = {
  local_planner: "CA_NMPC",
  global_planner: "A_STAR",
  map_path: "",
  running: false,
};

export function useNav2Control(): Nav2ControlRuntime {
  const [busy, setBusy] = useState(false);
  const [message, setMessage] = useState("Waiting for Nav2 state.");
  const [nav2Config, setNav2Config] = useState<Nav2Config>(DEFAULT_NAV2_CONFIG);
  const [nav2Maps, setNav2Maps] = useState<Array<{ id?: number; name?: string; yaml_path?: string | null }>>([]);
  const [selectedMapId, setSelectedMapId] = useState("");
  const [nav2LocalOptions, setNav2LocalOptions] = useState<Nav2Option[]>([]);
  const [nav2GlobalOptions, setNav2GlobalOptions] = useState<Nav2Option[]>([]);

  const loadNav2State = useCallback(async () => {
    try {
      const [optionsResponse, configResponse, mapsResponse] = await Promise.all([
        fetchWithAuth("/api/nav2/options"),
        fetchWithAuth("/api/nav2/config"),
        fetchWithAuth("/api/map/list"),
      ]);
      const options = await optionsResponse.json();
      setNav2LocalOptions(options.local_planners ?? []);
      setNav2GlobalOptions(options.global_planners ?? []);
      const config = (await configResponse.json()) as Nav2Config;
      const maps = (await mapsResponse.json()) as Array<{ id?: number; name?: string; yaml_path?: string | null }>;
      const storedMapId = readSelectedMapId();
      const matchedMapId = maps.find((map) => map.yaml_path === config.map_path)?.id ?? null;
      const resolvedMapId =
        storedMapId != null && maps.some((map) => map.id === storedMapId)
          ? storedMapId
          : (config.selected_map_id ?? matchedMapId);

      setNav2Maps(maps);
      setSelectedMapId(resolvedMapId != null ? String(resolvedMapId) : "");
      setNav2Config(config);
      setMessage(`Nav2 ${config.running ? "running" : "idle"} on ${config.local_planner} + ${config.global_planner}.`);
    } catch (error) {
      setNav2Maps([]);
      setSelectedMapId("");
      setNav2LocalOptions([]);
      setNav2GlobalOptions([]);
      setMessage(error instanceof Error ? error.message : "Cannot load Nav2 state.");
    }
  }, []);

  useEffect(() => {
    const timer = window.setTimeout(() => {
      void loadNav2State();
    }, 0);
    return () => window.clearTimeout(timer);
  }, [loadNav2State]);

  const updateNav2Config = useCallback(
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
        else payload.map_path = nav2Config.map_path;

        const response = await fetchWithAuth("/api/nav2/config", {
          method: "POST",
          body: JSON.stringify(payload),
        });
        const config = (await response.json()) as Nav2Config;
        setNav2Config(config);
        if (effectiveMapId != null && Number.isFinite(effectiveMapId)) {
          setSelectedMapId(String(effectiveMapId));
          writeSelectedMapId(effectiveMapId);
        }
        setMessage(`Nav2 config updated: ${config.local_planner} + ${config.global_planner}.`);
      } catch (error) {
        setMessage(error instanceof Error ? error.message : "Cannot update Nav2 config.");
      } finally {
        setBusy(false);
      }
    },
    [nav2Config.map_path, selectedMapId],
  );

  const selectNav2Map = useCallback(
    async (mapId: string) => {
      setSelectedMapId(mapId);
      const parsedMapId = Number.parseInt(mapId, 10);
      if (!Number.isFinite(parsedMapId)) return;
      writeSelectedMapId(parsedMapId);
      await updateNav2Config(nav2Config.local_planner, nav2Config.global_planner, parsedMapId);
    },
    [nav2Config.global_planner, nav2Config.local_planner, updateNav2Config],
  );

  const startNav2 = useCallback(async () => {
    setBusy(true);
    try {
      const parsedMapId = selectedMapId ? Number.parseInt(selectedMapId, 10) : NaN;
      if (!Number.isFinite(parsedMapId)) {
        throw new Error("No Nav2 map selected. Choose a saved map before starting Nav2.");
      }
      if (Number.isFinite(parsedMapId)) {
        await fetchWithAuth("/api/nav2/config", {
          method: "POST",
          body: JSON.stringify({
            local_planner: nav2Config.local_planner,
            global_planner: nav2Config.global_planner,
            map_id: parsedMapId,
          }),
        });
      }
      const response = await fetchWithAuth("/api/nav2/start", { method: "POST" });
      const config = (await response.json()) as Nav2Config;
      setNav2Config(config);
      setMessage(`Nav2 started with ${config.local_planner} / ${config.global_planner}.`);
    } catch (error) {
      setMessage(error instanceof Error ? error.message : "Nav2 start failed.");
    } finally {
      setBusy(false);
    }
  }, [nav2Config.global_planner, nav2Config.local_planner, selectedMapId]);

  const stopNav2 = useCallback(async () => {
    setBusy(true);
    try {
      const response = await fetchWithAuth("/api/nav2/stop", { method: "POST" });
      const config = (await response.json()) as Nav2Config;
      setNav2Config(config);
      setMessage("Nav2 stopped.");
    } catch (error) {
      setMessage(error instanceof Error ? error.message : "Nav2 stop failed.");
    } finally {
      setBusy(false);
    }
  }, []);

  return {
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
  };
}
