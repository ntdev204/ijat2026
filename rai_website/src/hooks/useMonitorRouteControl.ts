"use client";

import { useCallback, useEffect, useState, type MouseEvent, type RefObject } from "react";
import { ApiError, fetchWithAuth } from "@/lib/api";
import { drawOccupancyMap, pixelToWorld, type PoseMarker } from "@/lib/map-canvas";
import type { RobotUiTelemetry } from "@/lib/robot-telemetry";
import type { MapPayload, PathsPayload, Pose2D, RaiNavigationConfig, RobotAnchors } from "@/types/robot-runtime";

type PoseSelectionMode = "route_goal" | "initial_pose" | "home_pose" | null;

export interface MonitorRouteControlRuntime {
  anchors: RobotAnchors;
  busy: boolean;
  routeGoalPose: Pose2D | null;
  poseDraft: Pose2D | null;
  poseDialogOpen: boolean;
  poseDialogTitle: string;
  poseDialogDescription: string;
  selectionMode: PoseSelectionMode;
  statusMessage: string;
  setPoseDraft: (pose: Pose2D) => void;
  loadAnchors: () => Promise<void>;
  handleMapClick: (event: MouseEvent<HTMLCanvasElement>) => void;
  openRouteSelection: () => void;
  openInitialPoseSelection: () => void;
  openHomePoseSelection: () => void;
  closePoseDialog: () => void;
  confirmPoseDraft: () => Promise<void>;
  runSelectedRoute: () => Promise<void>;
  goHome: () => Promise<void>;
  cancelGoal: () => Promise<void>;
  clearRoute: () => void;
}

export function useMonitorRouteControl(
  canvasRef: RefObject<HTMLCanvasElement | null>,
  mapData: MapPayload | null,
  telemetry: RobotUiTelemetry | null,
  paths: PathsPayload,
  raiNavigationConfig: RaiNavigationConfig,
) : MonitorRouteControlRuntime {
  const [busy, setBusy] = useState(false);
  const [routeGoalPose, setRouteGoalPose] = useState<Pose2D | null>(null);
  const [anchors, setAnchors] = useState<RobotAnchors>({ initial_pose: null, home_pose: null });
  const [selectionMode, setSelectionMode] = useState<PoseSelectionMode>(null);
  const [poseDraft, setPoseDraft] = useState<Pose2D | null>(null);
  const [poseDialogOpen, setPoseDialogOpen] = useState(false);
  const [statusMessage, setStatusMessage] = useState("Route control is idle.");

  const loadAnchors = useCallback(async () => {
    try {
      const response = await fetchWithAuth("/api/robot/anchors");
      setAnchors((await response.json()) as RobotAnchors);
    } catch {
      setAnchors({ initial_pose: null, home_pose: null });
    }
  }, []);

  useEffect(() => {
    const timer = window.setTimeout(() => {
      void loadAnchors();
    }, 0);
    return () => window.clearTimeout(timer);
  }, [loadAnchors]);

  useEffect(() => {
    if (!canvasRef.current || !mapData) return;
    drawOccupancyMap(canvasRef.current, mapData, {
      paths,
      telemetry,
      markers: buildMarkers(routeGoalPose, anchors),
    });
  }, [anchors, canvasRef, mapData, paths, routeGoalPose, telemetry]);

  const openRouteSelection = useCallback(() => {
    setSelectionMode("route_goal");
    setStatusMessage("Click goal point on the monitor map.");
  }, []);

  const openInitialPoseSelection = useCallback(() => {
    setSelectionMode("initial_pose");
    setStatusMessage("Click initial pose on the monitor map.");
  }, []);

  const openHomePoseSelection = useCallback(() => {
    setSelectionMode("home_pose");
    setStatusMessage("Click home pose on the monitor map.");
  }, []);

  const handleMapClick = useCallback((event: MouseEvent<HTMLCanvasElement>) => {
    if (!selectionMode || !mapData) {
      setStatusMessage("Use Set Goal, Set Initial Pose, or Set Home before clicking the monitor map.");
      return;
    }
    const canvas = canvasRef.current;
    if (!canvas) return;
    const rect = canvas.getBoundingClientRect();
    const pixel = {
      x: ((event.clientX - rect.left) / rect.width) * canvas.width,
      y: ((event.clientY - rect.top) / rect.height) * canvas.height,
    };
    const world = pixelToWorld(pixel, mapData);
    setPoseDraft({
      x: Number(world.x.toFixed(3)),
      y: Number(world.y.toFixed(3)),
      yaw: routeGoalPose?.yaw ?? 0,
    });
    setPoseDialogOpen(true);
  }, [canvasRef, mapData, routeGoalPose?.yaw, selectionMode]);

  const closePoseDialog = useCallback(() => {
    setPoseDialogOpen(false);
    setPoseDraft(null);
    setSelectionMode(null);
  }, []);

  const confirmPoseDraft = useCallback(async () => {
    if (!poseDraft) return;
    setBusy(true);
    try {
      if (selectionMode === "route_goal") {
        setRouteGoalPose(poseDraft);
        setSelectionMode(null);
        setStatusMessage("Goal point saved. Review and click Send Goal.");
      } else if (selectionMode === "initial_pose") {
        const response = await fetchWithAuth("/api/robot/initial_pose", {
          method: "POST",
          body: JSON.stringify({ ...poseDraft, set_home: anchors.home_pose == null }),
        });
        const payload = await response.json();
        setAnchors({
          initial_pose: payload.initial_pose ?? poseDraft,
          home_pose: payload.home_pose ?? anchors.home_pose,
        });
        setSelectionMode(null);
        setStatusMessage("Initial pose published.");
      } else if (selectionMode === "home_pose") {
        const response = await fetchWithAuth("/api/robot/home", {
          method: "POST",
          body: JSON.stringify(poseDraft),
        });
        const payload = await response.json();
        setAnchors((current) => ({ ...current, home_pose: payload.home_pose ?? poseDraft }));
        setSelectionMode(null);
        setStatusMessage("Home pose updated.");
      }
    } catch (error) {
      setStatusMessage(error instanceof Error ? error.message : "Cannot confirm selected pose.");
      setSelectionMode(null);
    } finally {
      setBusy(false);
      setPoseDialogOpen(false);
      setPoseDraft(null);
    }
  }, [anchors.home_pose, poseDraft, selectionMode]);

  const runSelectedRoute = useCallback(async () => {
    if (!raiNavigationConfig.running) {
      setStatusMessage("RAI Navigation is not running. Start it before sending a goal.");
      return;
    }
    if (!routeGoalPose) {
      setStatusMessage("You need a goal point before sending navigation.");
      return;
    }
    setBusy(true);
    try {
      const response = await fetchWithAuth("/api/robot/nav/goal", {
        method: "POST",
        body: JSON.stringify(routeGoalPose),
      });
      const payload = await response.json();
      setStatusMessage(payload.message ?? "Goal dispatched.");
    } catch (error) {
      setStatusMessage(formatNavigationError(error));
    } finally {
      setBusy(false);
    }
  }, [raiNavigationConfig.running, routeGoalPose]);

  const goHome = useCallback(async () => {
    if (!raiNavigationConfig.running) {
      setStatusMessage("RAI Navigation is not running. Start it before going home.");
      return;
    }
    setBusy(true);
    try {
      const response = await fetchWithAuth("/api/robot/nav/home", { method: "POST" });
      const payload = await response.json();
      setStatusMessage(payload.message ?? "Going home.");
    } catch (error) {
      setStatusMessage(formatNavigationError(error));
    } finally {
      setBusy(false);
    }
  }, [raiNavigationConfig.running]);

  const cancelGoal = useCallback(async () => {
    setBusy(true);
    try {
      await fetchWithAuth("/api/robot/nav/cancel", { method: "POST" });
      setStatusMessage("Navigation goal cancelled.");
    } catch (error) {
      setStatusMessage(error instanceof Error ? error.message : "Cancel failed.");
    } finally {
      setBusy(false);
    }
  }, []);

  const clearRoute = useCallback(() => {
    setRouteGoalPose(null);
    setSelectionMode(null);
    setPoseDialogOpen(false);
    setPoseDraft(null);
    setStatusMessage("Route points cleared.");
  }, []);

  return {
    anchors,
    busy,
    routeGoalPose,
    poseDraft,
    poseDialogOpen,
    poseDialogTitle: getPoseDialogTitle(selectionMode),
    poseDialogDescription: getPoseDialogDescription(selectionMode),
    selectionMode,
    statusMessage,
    setPoseDraft,
    loadAnchors,
    handleMapClick,
    openRouteSelection,
    openInitialPoseSelection,
    openHomePoseSelection,
    closePoseDialog,
    confirmPoseDraft,
    runSelectedRoute,
    goHome,
    cancelGoal,
    clearRoute,
  };
}

function buildMarkers(routeGoalPose: Pose2D | null, anchors: RobotAnchors): PoseMarker[] {
  return [
    routeGoalPose ? { label: "GOAL", color: "#dc2626", pose: routeGoalPose } : null,
    anchors.initial_pose ? { label: "INIT", color: "#7c3aed", pose: anchors.initial_pose } : null,
    anchors.home_pose ? { label: "HOME", color: "#0f766e", pose: anchors.home_pose } : null,
  ].filter((marker): marker is PoseMarker => marker !== null);
}

function getPoseDialogTitle(selectionMode: PoseSelectionMode) {
  if (selectionMode === "route_goal") return "Goal Point";
  if (selectionMode === "initial_pose") return "Initial Pose";
  if (selectionMode === "home_pose") return "Home Pose";
  return "Pose";
}

function getPoseDialogDescription(selectionMode: PoseSelectionMode) {
  if (selectionMode === "route_goal") return "Confirm the final goal point and heading.";
  if (selectionMode === "initial_pose") return "Set the initial pose to align the robot with the map frame.";
  if (selectionMode === "home_pose") return "Set the home pose used by the Go Home command.";
  return "Confirm the selected pose.";
}

function formatNavigationError(error: unknown) {
  if (error instanceof ApiError) {
    if (error.status === 503 && error.message.includes("RAI navigation goal service is not available")) {
      return "RAI Navigation is enabled but the goal interface is not ready yet.";
    }
    if (error.status === 503 && error.message.includes("Home pose is not set")) {
      return "Home pose is not set. Use Set Home on the monitor map first.";
    }
    if (error.status === 503 && error.message.includes("ROS2 bridge is not ready")) {
      return "ROS2 bridge is not ready. Check rai_web_api and robot runtime.";
    }
  }
  return error instanceof Error ? error.message : "Navigation command failed.";
}
