"use client";

import { MapPoseDialog } from "@/components/map/MapPoseDialog";
import { Nav2PlannerPanel } from "@/components/nav2/Nav2PlannerPanel";
import { Button } from "@/components/ui/button";
import { StatusBadge } from "@/components/ui/StatusBadge";
import { useMonitorRouteControl } from "@/hooks/useMonitorRouteControl";
import { useNav2Control } from "@/hooks/useNav2Control";
import { useMonitorRuntime } from "@/hooks/useMonitorRuntime";
import type { RobotUiTelemetry } from "@/lib/robot-telemetry";
import type { MapPayload, PathsPayload, Pose2D, RobotAnchors, StreamState } from "@/types/robot-runtime";
import { Home, LocateFixed, MapPinned, Map as MapIcon, Navigation, Square, Users, Video, VideoOff, Waypoints } from "lucide-react";
import type { MouseEvent, RefObject } from "react";

export default function MonitorPage() {
  const nav2 = useNav2Control();
  const monitor = useMonitorRuntime();
  const route = useMonitorRouteControl(monitor.mapCanvasRef, monitor.mapData, monitor.telemetry, monitor.paths, nav2.nav2Config);
  const { live, mapCanvasRef, mapData, message, paths, state, telemetry, videoRef } = monitor;

  return (
    <div className="space-y-5">
      <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
        <div>
          <h2 className="text-2xl font-bold tracking-tight text-slate-800">Monitor</h2>
          <p className="mt-1 text-sm text-slate-500">Video stream, realtime map, planner traces, and route control.</p>
        </div>
        <StatusBadge status={live ? "success" : state === "error" ? "error" : "warning"}>
          {live ? "LIVE" : state.toUpperCase()}
        </StatusBadge>
      </div>

      <div className="grid grid-cols-1 gap-5 xl:grid-cols-[minmax(0,1.2fr)_minmax(0,1fr)]">
        <VideoPanel live={live} message={message} state={state} videoRef={videoRef} />
        <RealtimeMapPanel
          mapCanvasRef={mapCanvasRef}
          mapData={mapData}
          nav2Running={nav2.nav2Config.running}
          paths={paths}
          selectionMode={route.selectionMode}
          statusMessage={route.statusMessage}
          telemetry={telemetry}
          onMapClick={route.handleMapClick}
        />
      </div>

      <div className="grid grid-cols-1 gap-5 xl:grid-cols-[minmax(0,1fr)_minmax(0,1fr)]">
        <RoutePanel
          busy={route.busy}
          nav2Running={nav2.nav2Config.running}
          routeGoalPose={route.routeGoalPose}
          routeStartPose={route.routeStartPose}
          runSelectedRoute={route.runSelectedRoute}
          clearRoute={route.clearRoute}
          cancelGoal={route.cancelGoal}
          openRouteSelection={route.openRouteSelection}
        />
        <AnchorPanel
          anchors={route.anchors}
          busy={route.busy}
          goHome={route.goHome}
          openHomePoseSelection={route.openHomePoseSelection}
          openInitialPoseSelection={route.openInitialPoseSelection}
        />
      </div>

      <MonitorMessage message={message} telemetry={telemetry} />
      <div className="rounded-md border border-slate-200 bg-white px-4 py-3 text-sm text-slate-700">{route.statusMessage}</div>
      <Nav2PlannerPanel
        busy={nav2.busy}
        message={nav2.message}
        nav2Config={nav2.nav2Config}
        nav2GlobalOptions={nav2.nav2GlobalOptions}
        nav2LocalOptions={nav2.nav2LocalOptions}
        loadNav2State={nav2.loadNav2State}
        updateNav2Config={nav2.updateNav2Config}
        startNav2={nav2.startNav2}
        stopNav2={nav2.stopNav2}
      />

      <MapPoseDialog
        open={route.poseDialogOpen}
        title={route.poseDialogTitle}
        description={route.poseDialogDescription}
        pose={route.poseDraft}
        busy={route.busy}
        onClose={route.closePoseDialog}
        onChange={route.setPoseDraft}
        onConfirm={() => void route.confirmPoseDraft()}
      />
    </div>
  );
}

interface VideoPanelProps {
  live: boolean;
  message: string;
  state: StreamState;
  videoRef: RefObject<HTMLVideoElement | null>;
}

function VideoPanel({ live, message, state, videoRef }: VideoPanelProps) {
  return (
    <section className="overflow-hidden rounded-lg border border-slate-800 bg-black shadow-lg">
      <div className="relative aspect-[4/3] bg-black">
        <video ref={videoRef} autoPlay playsInline muted className="h-full w-full object-contain" />
        {!live && (
          <div className="absolute inset-0 flex flex-col items-center justify-center gap-2 text-slate-500">
            {state === "error" ? <VideoOff className="h-12 w-12" /> : <Video className="h-12 w-12" />}
            <span>{message}</span>
          </div>
        )}
      </div>
      <div className="border-t border-slate-800 px-4 py-3 text-sm text-slate-300">
        2D bbox overlay is unavailable until the perception pipeline publishes image-space detections. Current `/canmpc/humans` only exposes tracked world poses.
      </div>
    </section>
  );
}

interface RealtimeMapPanelProps {
  mapCanvasRef: RefObject<HTMLCanvasElement | null>;
  mapData: MapPayload | null;
  nav2Running: boolean;
  paths: PathsPayload;
  selectionMode: string | null;
  statusMessage: string;
  telemetry: RobotUiTelemetry | null;
  onMapClick: (event: MouseEvent<HTMLCanvasElement>) => void;
}

function RealtimeMapPanel({ mapCanvasRef, mapData, nav2Running, paths, selectionMode, statusMessage, telemetry, onMapClick }: RealtimeMapPanelProps) {
  return (
    <section className="overflow-hidden rounded-lg border border-slate-200 bg-white shadow-sm">
      <div className="flex items-center gap-2 border-b border-slate-200 px-4 py-3 text-sm text-slate-600">
        <MapIcon className="size-4 text-blue-600" />
        <span className="font-medium text-slate-800">Realtime Map</span>
        <span className={`ml-auto rounded-full px-2 py-1 text-xs font-medium ${nav2Running ? "bg-emerald-100 text-emerald-700" : "bg-amber-100 text-amber-700"}`}>
          {nav2Running ? "Nav2 ready" : "Nav2 not running"}
        </span>
      </div>
      <div className="flex min-h-[360px] items-center justify-center overflow-auto bg-slate-100 p-4">
        {mapData ? (
          <canvas
            ref={mapCanvasRef}
            onClick={onMapClick}
            className={`h-[min(48vh,500px)] w-auto max-w-full rounded-md border border-slate-300 bg-white shadow-sm [image-rendering:pixelated] ${selectionMode ? "cursor-crosshair" : "cursor-default"}`}
          />
        ) : (
          <div className="text-sm text-slate-400">Waiting for /api/ws/map.</div>
        )}
      </div>
      <div className="border-t border-slate-200 px-4 py-2 text-sm text-slate-500">{statusMessage}</div>
      <div className="grid gap-3 border-t border-slate-200 px-4 py-3 text-sm text-slate-600 md:grid-cols-3">
        <div className="flex items-center gap-2">
          <Waypoints className="size-4 text-blue-600" />
          <span>Global {paths.global_plan?.length ?? 0}</span>
        </div>
        <div className="flex items-center gap-2">
          <Waypoints className="size-4 text-amber-600" />
          <span>Local {paths.local_plan?.length ?? 0}</span>
        </div>
        <div className="flex items-center gap-2">
          <Users className="size-4 text-rose-600" />
          <span>Humans {telemetry?.humans.length ?? 0}</span>
        </div>
      </div>
    </section>
  );
}

interface RoutePanelProps {
  busy: boolean;
  nav2Running: boolean;
  routeStartPose: Pose2D | null;
  routeGoalPose: Pose2D | null;
  openRouteSelection: () => void;
  runSelectedRoute: () => Promise<void>;
  clearRoute: () => void;
  cancelGoal: () => Promise<void>;
}

function RoutePanel({ busy, nav2Running, routeStartPose, routeGoalPose, openRouteSelection, runSelectedRoute, clearRoute, cancelGoal }: RoutePanelProps) {
  return (
    <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
      <div className="mb-3 flex items-center justify-between gap-3">
        <h3 className="text-sm font-bold uppercase tracking-wider text-slate-400">Route</h3>
        <Button type="button" variant="outline" size="sm" className="gap-2" disabled={busy} onClick={openRouteSelection}>
          <MapPinned className="size-4" />
          Route 2 Points
        </Button>
      </div>
      <div className="grid gap-3">
        <PoseSummary label="Start" pose={routeStartPose} />
        <PoseSummary label="Goal" pose={routeGoalPose} />
      </div>
      <div className="mt-4 flex flex-wrap gap-2">
        <Button type="button" className="gap-2" disabled={busy || !nav2Running || !routeStartPose || !routeGoalPose} onClick={() => void runSelectedRoute()}>
          <Navigation className="size-4" />
          Run Route
        </Button>
        <Button type="button" variant="outline" disabled={busy || (!routeStartPose && !routeGoalPose)} onClick={clearRoute}>
          Clear
        </Button>
        <Button type="button" variant="outline" className="gap-2" disabled={busy} onClick={() => void cancelGoal()}>
          <Square className="size-4" />
          Cancel Nav
        </Button>
      </div>
    </section>
  );
}

interface AnchorPanelProps {
  anchors: RobotAnchors;
  busy: boolean;
  goHome: () => Promise<void>;
  openHomePoseSelection: () => void;
  openInitialPoseSelection: () => void;
}

function AnchorPanel({ anchors, busy, goHome, openHomePoseSelection, openInitialPoseSelection }: AnchorPanelProps) {
  return (
    <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
      <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">Anchors</h3>
      <div className="grid gap-3">
        <PoseSummary label="Initial Pose" pose={anchors.initial_pose} />
        <PoseSummary label="Home Pose" pose={anchors.home_pose} />
      </div>
      <div className="mt-4 flex flex-wrap gap-2">
        <Button type="button" variant="outline" className="gap-2" disabled={busy} onClick={openInitialPoseSelection}>
          <LocateFixed className="size-4" />
          Set Initial Pose
        </Button>
        <Button type="button" variant="outline" className="gap-2" disabled={busy} onClick={openHomePoseSelection}>
          <Home className="size-4" />
          Set Home
        </Button>
        <Button type="button" className="gap-2" disabled={busy || !anchors.home_pose} onClick={() => void goHome()}>
          <Home className="size-4" />
          Go Home
        </Button>
      </div>
    </section>
  );
}

interface PoseSummaryProps {
  label: string;
  pose: Pose2D | null;
}

function PoseSummary({ label, pose }: PoseSummaryProps) {
  return (
    <div className="rounded-md border border-slate-200 bg-slate-50 px-3 py-2 text-sm text-slate-700">
      <div className="text-xs font-medium uppercase tracking-wider text-slate-400">{label}</div>
      <div className="mt-1">{pose ? `x=${pose.x.toFixed(2)}, y=${pose.y.toFixed(2)}, theta=${pose.yaw.toFixed(2)}` : "Not set"}</div>
    </div>
  );
}

interface MonitorMessageProps {
  message: string;
  telemetry: RobotUiTelemetry | null;
}

function MonitorMessage({ message, telemetry }: MonitorMessageProps) {
  return (
    <div className="rounded-md border border-slate-200 bg-white px-4 py-3 text-sm text-slate-700">
      {message}
      {(telemetry?.humans.length ?? 0) > 0 && (
        <div className="mt-3 grid gap-2 md:grid-cols-2">
          {telemetry?.humans.map((human, index) => (
            <div key={`human-${human.id ?? index}`} className="rounded-md border border-slate-200 bg-slate-50 px-3 py-2">
              H{human.id ?? "?"}: ({human.x?.toFixed?.(2) ?? "-"}, {human.y?.toFixed?.(2) ?? "-"}) conf {human.confidence?.toFixed?.(2) ?? "-"}
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
