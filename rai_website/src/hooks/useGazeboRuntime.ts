"use client";

import { useCallback, useEffect, useState } from "react";
import { fetchWithAuth } from "@/lib/api";
import { useOperationMode } from "@/contexts/OperationModeContext";
import type { RobotModelOption, SimulationWorldOption, SystemRuntime } from "@/types/robot-runtime";

interface GazeboStatusPayload {
  running?: boolean;
  world_id?: string;
  world_path?: string;
  robot_model?: string;
  start_slam?: boolean;
  start_navigation?: boolean;
}

export interface GazeboRuntime {
  systemRuntime: SystemRuntime | null;
  worlds: SimulationWorldOption[];
  robotModels: RobotModelOption[];
  selectedWorldId: string;
  selectedRobotModelId: string;
  worldName: string;
  worldWidth: number;
  worldHeight: number;
  obstacleCount: number;
  corridor: boolean;
  running: boolean;
  busy: boolean;
  status: string;
  setSelectedWorldId: (value: string) => void;
  setSelectedRobotModelId: (value: string) => void;
  setWorldName: (value: string) => void;
  setWorldWidth: (value: number) => void;
  setWorldHeight: (value: number) => void;
  setObstacleCount: (value: number) => void;
  setCorridor: (value: boolean) => void;
  refresh: () => Promise<void>;
  createWorld: () => Promise<void>;
  startSimulation: () => Promise<void>;
  stopSimulation: () => Promise<void>;
}

export function useGazeboRuntime(): GazeboRuntime {
  const { runtime } = useOperationMode();
  const [systemRuntime, setSystemRuntime] = useState<SystemRuntime | null>(null);
  const [worlds, setWorlds] = useState<SimulationWorldOption[]>([]);
  const [robotModels, setRobotModels] = useState<RobotModelOption[]>([]);
  const [selectedWorldId, setSelectedWorldId] = useState("s1_open_zone");
  const [selectedRobotModelId, setSelectedRobotModelId] = useState("mini_mec_robot");
  const [worldName, setWorldName] = useState("virtual_lab");
  const [worldWidth, setWorldWidth] = useState(8);
  const [worldHeight, setWorldHeight] = useState(6);
  const [obstacleCount, setObstacleCount] = useState(3);
  const [corridor, setCorridor] = useState(false);
  const [running, setRunning] = useState(false);
  const [busy, setBusy] = useState(false);
  const [status, setStatus] = useState("Simulation stack is idle.");

  const refresh = useCallback(async () => {
    try {
      const [worldsResponse, modelsResponse, simStatusResponse] = await Promise.all([
        fetchWithAuth("/api/simulation/worlds"),
        fetchWithAuth("/api/robot-models"),
        fetchWithAuth("/api/simulation/status"),
      ]);
      const nextWorlds = (await worldsResponse.json()) as SimulationWorldOption[];
      const nextModels = (await modelsResponse.json()) as RobotModelOption[];
      const simStatus = (await simStatusResponse.json()) as GazeboStatusPayload;

      setSystemRuntime(runtime);
      setWorlds(nextWorlds);
      setRobotModels(nextModels);
      setRunning(Boolean(simStatus.running));

      if (simStatus.world_id) {
        setSelectedWorldId(simStatus.world_id);
      } else if (nextWorlds.length > 0 && !nextWorlds.some((world) => world.id === selectedWorldId)) {
        setSelectedWorldId(nextWorlds[0].id);
      }

      if (simStatus.robot_model) {
        setSelectedRobotModelId(simStatus.robot_model);
      } else if (nextModels.length > 0 && !nextModels.some((model) => model.id === selectedRobotModelId)) {
        setSelectedRobotModelId(nextModels[0].id);
      }

      if (simStatus.running) {
        const worldLabel = nextWorlds.find((world) => world.id === (simStatus.world_id ?? ""))?.label ?? simStatus.world_id ?? "current world";
        setStatus(`Simulation is running on ${worldLabel} with ${simStatus.robot_model ?? selectedRobotModelId}.`);
      }
    } catch {
      setWorlds([]);
      setRobotModels([]);
      setStatus("Cannot load simulation runtime.");
    }
  }, [runtime, selectedRobotModelId, selectedWorldId]);

  useEffect(() => {
    const timer = window.setTimeout(() => {
      void refresh();
    }, 0);
    return () => window.clearTimeout(timer);
  }, [refresh]);

  const createWorld = useCallback(async () => {
    const name = worldName.trim();
    if (!name) {
      setStatus("World name is required.");
      return;
    }
    setBusy(true);
    try {
      const response = await fetchWithAuth("/api/simulation/worlds", {
        method: "POST",
        body: JSON.stringify({
          name,
          width: worldWidth,
          height: worldHeight,
          obstacle_count: obstacleCount,
          corridor,
        }),
      });
      const payload = (await response.json()) as { message?: string; world?: SimulationWorldOption };
      setStatus(payload.message ?? "Virtual world created.");
      await refresh();
      if (payload.world?.id) {
        setSelectedWorldId(payload.world.id);
      }
    } catch (error) {
      setStatus(error instanceof Error ? error.message : "Create world failed.");
    } finally {
      setBusy(false);
    }
  }, [corridor, obstacleCount, refresh, worldHeight, worldName, worldWidth]);

  const startSimulation = useCallback(async () => {
    setBusy(true);
    try {
      const response = await fetchWithAuth("/api/simulation/start", {
        method: "POST",
        body: JSON.stringify({
          world_id: selectedWorldId,
          robot_model: selectedRobotModelId,
          start_slam: true,
          start_navigation: false,
          headless: false,
        }),
      });
      const payload = (await response.json()) as { message?: string };
      setRunning(true);
      setStatus(payload.message ?? "Simulation started.");
      await refresh();
    } catch (error) {
      setStatus(error instanceof Error ? error.message : "Simulation start failed.");
    } finally {
      setBusy(false);
    }
  }, [refresh, selectedRobotModelId, selectedWorldId]);

  const stopSimulation = useCallback(async () => {
    setBusy(true);
    try {
      const response = await fetchWithAuth("/api/simulation/stop", { method: "POST" });
      const payload = (await response.json()) as { message?: string };
      setRunning(false);
      setStatus(payload.message ?? "Simulation stopped.");
      await refresh();
    } catch (error) {
      setStatus(error instanceof Error ? error.message : "Simulation stop failed.");
    } finally {
      setBusy(false);
    }
  }, [refresh]);

  return {
    systemRuntime,
    worlds,
    robotModels,
    selectedWorldId,
    selectedRobotModelId,
    worldName,
    worldWidth,
    worldHeight,
    obstacleCount,
    corridor,
    running,
    busy,
    status,
    setSelectedWorldId,
    setSelectedRobotModelId,
    setWorldName,
    setWorldWidth,
    setWorldHeight,
    setObstacleCount,
    setCorridor,
    refresh,
    createWorld,
    startSimulation,
    stopSimulation,
  };
}
