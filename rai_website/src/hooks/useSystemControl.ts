"use client";

import { useCallback, useEffect, useMemo, useState } from "react";
import { toast } from "sonner";
import { fetchWithAuth } from "@/lib/api";
import { useOperationMode } from "@/contexts/OperationModeContext";
import type { SystemComponent, SystemComponentsPayload } from "@/types/robot-runtime";

type ComponentId =
  | "robot_base"
  | "lidar"
  | "camera"
  | "slam"
  | "navigation"
  | "simulation"
  | "dataset";

const FALLBACK_COMPONENTS: SystemComponent[] = [
  {
    id: "robot_base",
    label: "Robot Base",
    host_device: "pi",
    action: "hardware",
    allowed_here: true,
    running: false,
    pid: null,
    launch_file: "turn_on_rai_robot.launch.py",
    description: "",
    capabilities: {},
  },
  {
    id: "lidar",
    label: "LiDAR",
    host_device: "pi",
    action: "lidar",
    allowed_here: true,
    running: false,
    pid: null,
    launch_file: "rai_lidar.launch.py",
    description: "",
    capabilities: {},
  },
  {
    id: "camera",
    label: "Camera",
    host_device: "jetson",
    action: "camera",
    allowed_here: true,
    running: false,
    pid: null,
    launch_file: "rai_camera.launch.py",
    description: "",
    capabilities: { enable_depth_toggle: true },
  },
  {
    id: "slam",
    label: "SLAM",
    host_device: "pi",
    action: "slam",
    allowed_here: true,
    running: false,
    pid: null,
    launch_file: "online_async_launch.py",
    description: "",
    capabilities: {},
  },
  {
    id: "navigation",
    label: "Navigation",
    host_device: "pi",
    action: "navigation",
    allowed_here: true,
    running: false,
    pid: null,
    launch_file: "rai_navigation.launch.py",
    description: "",
    capabilities: {},
  },
  {
    id: "simulation",
    label: "Simulation",
    host_device: "pi",
    action: "simulation",
    allowed_here: true,
    running: false,
    pid: null,
    launch_file: "rai_simulation.launch.py",
    description: "",
    capabilities: {},
  },
  {
    id: "dataset",
    label: "Dataset",
    host_device: "pi",
    action: "dataset",
    allowed_here: true,
    running: false,
    pid: null,
    launch_file: "dataset_collection.launch.py",
    description: "",
    capabilities: {},
  },
];

export function useSystemControl() {
  const { refresh: refreshRuntime } = useOperationMode();
  const [payload, setPayload] = useState<SystemComponentsPayload | null>(null);
  const [busyId, setBusyId] = useState<string | null>(null);
  const [cameraDepthEnabled, setCameraDepthEnabled] = useState(false);

  const refresh = useCallback(async () => {
    await refreshRuntime();
    const response = await fetchWithAuth("/api/system/components");
    const nextPayload = (await response.json()) as SystemComponentsPayload;
    setPayload(nextPayload);
    const camera = nextPayload.components.find((item) => item.id === "camera");
    if (camera?.capabilities?.enable_depth !== undefined) {
      setCameraDepthEnabled(Boolean(camera.capabilities.enable_depth));
    }
  }, [refreshRuntime]);

  useEffect(() => {
    let active = true;
    const initialTimer = window.setTimeout(() => {
      void refresh().catch((error) => {
        if (!active) return;
        toast.error(error instanceof Error ? error.message : "Cannot load system controls.");
      });
    }, 0);
    const interval = window.setInterval(() => {
      void refresh().catch(() => undefined);
    }, 3000);
    return () => {
      active = false;
      window.clearTimeout(initialTimer);
      window.clearInterval(interval);
    };
  }, [refresh]);

  const command = useCallback(async (componentId: ComponentId, action: "start" | "stop") => {
    setBusyId(`${componentId}:${action}`);
    try {
      const body =
        componentId === "camera" && action === "start"
          ? JSON.stringify({ enable_depth: cameraDepthEnabled })
          : undefined;
      const response = await fetchWithAuth(`/api/system/components/${componentId === "robot_base" ? "robot" : componentId}/${action}`, {
        method: "POST",
        body,
      });
      const result = await response.json();
      toast.success(result.message || `${componentId} ${action}ed.`);
      await refresh();
    } catch (error) {
      toast.error(error instanceof Error ? error.message : `Cannot ${action} ${componentId}.`);
    } finally {
      setBusyId(null);
    }
  }, [cameraDepthEnabled, refresh]);

  const components = useMemo(() => {
    if (!payload?.components?.length) {
      return FALLBACK_COMPONENTS;
    }
    return FALLBACK_COMPONENTS.map((fallback) => {
      const live = payload.components.find((item) => item.id === fallback.id);
      return live ? { ...fallback, ...live } : fallback;
    });
  }, [payload]);
  return {
    payload,
    components,
    busyId,
    cameraDepthEnabled,
    setCameraDepthEnabled,
    refresh,
    startComponent: (componentId: ComponentId) => command(componentId, "start"),
    stopComponent: (componentId: ComponentId) => command(componentId, "stop"),
  };
}

export type { SystemComponent };
