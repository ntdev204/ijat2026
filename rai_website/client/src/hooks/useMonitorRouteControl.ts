"use client";

import { useCallback, useEffect, useState, type MouseEvent, type RefObject } from "react";
import { ApiError, fetchWithAuth } from "@/lib/api";
import { drawOccupancyMap, pixelToWorld, type PoseMarker } from "@/lib/map-canvas";
import type { RobotUiTelemetry } from "@/lib/robot-telemetry";
import type { MapPayload, Nav2Config, Pose2D, RobotAnchors } from "@/types/robot-runtime";

type PoseSelectionMode = "route_start" | "route_goal" | "initial_pose" | "home_pose" | null;

export interface MonitorRouteControlRuntime {
  anchors: RobotAnchors;
  busy: boolean;
  routeStartPose: Pose2D | null;
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
  nav2Config: Nav2Config,
) : MonitorRouteControlRuntime {
  const [busy, setBusy] = useState(false);
  const [routeStartPose, setRouteStartPose] = useState<Pose2D | null>(null);
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
      telemetry,
      markers: buildMarkers(routeStartPose, routeGoalPose, anchors),
    });
  }, [anchors, canvasRef, mapData, routeGoalPose, routeStartPose, telemetry]);

  function openRouteSelection() {
    setSelectionMode("route_start");
    setStatusMessage("Click start point on the monitor map.");
  }

  function openInitialPoseSelection() {
    setSelectionMode("initial_pose");
    setStatusMessage("Click initial pose on the monitor map.");
  }

  function openHomePoseSelection() {
    setSelectionMode("home_pose");
    setStatusMessage("Click home pose on the monitor map.");
  }

  function handleMapClick(event: MouseEvent<HTMLCanvasElement>) {
    if (!selectionMode || !mapData) {
      setStatusMessage("Use Route 2 Points, Set Initial Pose, or Set Home before clicking the monitor map.");
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
      yaw: selectionMode === "route_start" && routeStartPose ? routeStartPose.yaw : 0,
    });
    setPoseDialogOpen(true);
  }

  function closePoseDialog() {
    setPoseDialogOpen(false);
    setPoseDraft(null);
    if (selectionMode !== "route_goal") {
      setSelectionMode(null);
    }
  }

  async function confirmPoseDraft() {
    if (!poseDraft) return;
    setBusy(true);
    try {
      if (selectionMode === "route_start") {
        setRouteStartPose(poseDraft);
        setSelectionMode("route_goal");
        setStatusMessage("Start point saved. Click goal point on the monitor map.");
      } else if (selectionMode === "route_goal") {
        setRouteGoalPose(poseDraft);
        setSelectionMode(null);
        setStatusMessage("Goal point saved. Review the route and click Run Route.");
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
        setStatusMessage("Initial pose published to Nav2.");
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
  }

  async function runSelectedRoute() {
    if (!nav2Config.running) {
      setStatusMessage("Nav2 is not running. Start Nav2 before running a route.");
      return;
    }
    if (!routeStartPose || !routeGoalPose) {
      setStatusMessage("You need both start and goal points before running a route.");
      return;
    }
    setBusy(true);
    try {
      const response = await fetchWithAuth("/api/robot/nav/route", {
        method: "POST",
        body: JSON.stringify({ start: routeStartPose, goal: routeGoalPose, start_tolerance: 0.25 }),
      });
      const payload = await response.json();
      setStatusMessage(payload.message ?? "Route dispatched.");
    } catch (error) {
      setStatusMessage(formatNavigationError(error));
    } finally {
      setBusy(false);
    }
  }

  async function goHome() {
    if (!nav2Config.running) {
      setStatusMessage("Nav2 is not running. Start Nav2 before going home.");
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
  }

  async function cancelGoal() {
    setBusy(true);
    try {
      await fetchWithAuth("/api/robot/nav/cancel", { method: "POST" });
      setStatusMessage("Navigation goal cancelled.");
    } catch (error) {
      setStatusMessage(error instanceof Error ? error.message : "Cancel failed.");
    } finally {
      setBusy(false);
    }
  }

  function clearRoute() {
    setRouteStartPose(null);
    setRouteGoalPose(null);
    setSelectionMode(null);
    setPoseDialogOpen(false);
    setPoseDraft(null);
    setStatusMessage("Route points cleared.");
  }

  return {
    anchors,
    busy,
    routeStartPose,
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

function buildMarkers(routeStartPose: Pose2D | null, routeGoalPose: Pose2D | null, anchors: RobotAnchors): PoseMarker[] {
  return [
    routeStartPose ? { label: "START", color: "#16a34a", pose: routeStartPose } : null,
    routeGoalPose ? { label: "GOAL", color: "#dc2626", pose: routeGoalPose } : null,
    anchors.initial_pose ? { label: "INIT", color: "#7c3aed", pose: anchors.initial_pose } : null,
    anchors.home_pose ? { label: "HOME", color: "#0f766e", pose: anchors.home_pose } : null,
  ].filter((marker): marker is PoseMarker => marker !== null);
}

function getPoseDialogTitle(selectionMode: PoseSelectionMode) {
  if (selectionMode === "route_start") return "Start Point";
  if (selectionMode === "route_goal") return "Goal Point";
  if (selectionMode === "initial_pose") return "Initial Pose";
  if (selectionMode === "home_pose") return "Home Pose";
  return "Pose";
}

function getPoseDialogDescription(selectionMode: PoseSelectionMode) {
  if (selectionMode === "route_start") return "Confirm the start point that Nav2 should reach before the final goal.";
  if (selectionMode === "route_goal") return "Confirm the final goal point and heading.";
  if (selectionMode === "initial_pose") return "Set the initial pose to align the robot with the map frame.";
  if (selectionMode === "home_pose") return "Set the home pose used by the Go Home command.";
  return "Confirm the selected pose.";
}

function formatNavigationError(error: unknown) {
  if (error instanceof ApiError) {
    if (error.status === 503 && error.message.includes("Nav2 action server is not available")) {
      return "Nav2 is enabled but the navigate_to_pose action server is not ready yet.";
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
