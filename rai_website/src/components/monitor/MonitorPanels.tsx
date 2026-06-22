"use client";

import type { ReactNode } from "react";
import {
  Home,
  LocateFixed,
  Map as MapIcon,
  MapPinned,
  Navigation,
  Square,
  Users,
  Video,
  VideoOff,
  Waypoints,
} from "lucide-react";
import type { MouseEvent, RefObject } from "react";
import { Button } from "@/components/ui/button";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import type { RobotUiTelemetry } from "@/lib/robot-telemetry";
import { formatNumber } from "@/lib/format";
import type {
  MapPayload,
  PathsPayload,
  Pose2D,
  RobotAnchors,
  StreamState,
} from "@/types/robot-runtime";

export interface VideoPanelProps {
  live: boolean;
  message: string;
  state: StreamState;
  humanCount: number;
  videoRef: RefObject<HTMLVideoElement | null>;
}

export function VideoPanel({
  live,
  message,
  state,
  humanCount,
  videoRef,
}: VideoPanelProps) {
  const badgeClassName =
    state === "live"
      ? "bg-emerald-100 text-emerald-700"
      : state === "error"
        ? "bg-slate-200 text-slate-600"
        : "bg-amber-100 text-amber-700";
  const badgeLabel =
    state === "live"
      ? "Camera live"
      : state === "error"
        ? "Camera unavailable"
        : "Camera not running";

  return (
    <Card className="overflow-hidden">
      <CardHeader className="border-b border-slate-200 pb-3">
        <div className="flex flex-wrap items-center gap-2 text-sm text-slate-600">
          <Video className="size-4 text-blue-600" />
          <CardTitle className="text-base font-medium text-slate-800">
            Realtime Camera
          </CardTitle>
          <span
            className={`ml-auto rounded-full px-2 py-1 text-xs font-medium max-sm:ml-0 ${badgeClassName}`}
          >
            {badgeLabel}
          </span>
        </div>
      </CardHeader>
      <CardContent className="p-0">
        <div
          className={
            live
              ? "relative flex min-h-[220px] items-center justify-center overflow-hidden bg-black sm:min-h-[360px]"
              : "relative flex min-h-[220px] items-center justify-center bg-slate-50 sm:min-h-[360px]"
          }
        >
          <video
            ref={videoRef}
            autoPlay
            playsInline
            muted
            className={live ? "h-full max-h-[360px] w-full object-contain" : "hidden"}
          />
          {!live && (
            <div className="absolute inset-0 flex flex-col items-center justify-center gap-3 px-6 text-center text-slate-500">
              {state === "error" ? (
                <VideoOff className="h-12 w-12 text-slate-400" />
              ) : (
                <Video className="h-12 w-12 text-slate-400" />
              )}
              <span className="max-w-md text-sm text-slate-500">{message}</span>
            </div>
          )}
        </div>
        <div className="border-t border-slate-200 px-4 py-2 text-sm text-slate-500">
          {message}
        </div>
        <div className="grid gap-3 border-t border-slate-200 px-4 py-3 text-sm text-slate-600 md:grid-cols-2">
          <InlineMetric label="Stream" value={live ? "Live" : "Offline"} icon={<Video className="size-4 text-blue-600" />} />
          <InlineMetric label="Humans" value={String(humanCount)} icon={<Users className="size-4 text-rose-600" />} />
        </div>
      </CardContent>
    </Card>
  );
}

export interface RealtimeMapPanelProps {
  mapCanvasRef: RefObject<HTMLCanvasElement | null>;
  mapData: MapPayload | null;
  raiNavigationRunning: boolean;
  paths: PathsPayload;
  selectionMode: string | null;
  statusMessage: string;
  onMapClick: (event: MouseEvent<HTMLCanvasElement>) => void;
}

export function RealtimeMapPanel({
  mapCanvasRef,
  mapData,
  raiNavigationRunning,
  paths,
  selectionMode,
  statusMessage,
  onMapClick,
}: RealtimeMapPanelProps) {
  return (
    <Card className="overflow-hidden">
      <CardHeader className="border-b border-slate-200 pb-3">
        <div className="flex flex-wrap items-center gap-2 text-sm text-slate-600">
          <MapIcon className="size-4 text-blue-600" />
          <CardTitle className="text-base font-medium text-slate-800">
            Realtime Map
          </CardTitle>
          <span
            className={`ml-auto rounded-full px-2 py-1 text-xs font-medium max-sm:ml-0 ${raiNavigationRunning ? "bg-emerald-100 text-emerald-700" : "bg-amber-100 text-amber-700"}`}
          >
            {raiNavigationRunning
              ? "RAI Navigation ready"
              : "RAI Navigation not running"}
          </span>
        </div>
      </CardHeader>
      <CardContent className="p-0">
        <div className="flex min-h-[240px] items-center justify-center overflow-auto bg-slate-100 p-3 sm:min-h-90 sm:p-4">
          {mapData ? (
            <canvas
              ref={mapCanvasRef}
              onClick={onMapClick}
              style={{ aspectRatio: `${mapData.width} / ${mapData.height}` }}
              className={`block h-auto w-full max-w-full rounded-md border border-slate-300 bg-white shadow-sm [image-rendering:pixelated] ${selectionMode ? "cursor-crosshair" : "cursor-default"}`}
            />
          ) : (
            <div className="text-sm text-slate-400">Waiting for /api/ws/map.</div>
          )}
        </div>
        <div className="border-t border-slate-200 px-4 py-2 text-sm text-slate-500">
          {statusMessage}
        </div>
        <div className="grid gap-3 border-t border-slate-200 px-4 py-3 text-sm text-slate-600 md:grid-cols-2">
          <InlineMetric label="Global" value={String(paths.global_plan?.length ?? 0)} icon={<Waypoints className="size-4 text-blue-600" />} />
          <InlineMetric label="Local" value={String(paths.local_plan?.length ?? 0)} icon={<Waypoints className="size-4 text-amber-600" />} />
        </div>
      </CardContent>
    </Card>
  );
}

export interface RoutePanelProps {
  busy: boolean;
  raiNavigationRunning: boolean;
  routeGoalPose: Pose2D | null;
  openRouteSelection: () => void;
  runSelectedRoute: () => Promise<void>;
  clearRoute: () => void;
  cancelGoal: () => Promise<void>;
}

export function RoutePanel({
  busy,
  raiNavigationRunning,
  routeGoalPose,
  openRouteSelection,
  runSelectedRoute,
  clearRoute,
  cancelGoal,
}: RoutePanelProps) {
  return (
    <Card>
      <CardHeader className="pb-3">
        <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
          <CardTitle className="text-sm font-bold uppercase tracking-wider text-slate-400">
            Route
          </CardTitle>
          <Button
            type="button"
            variant="outline"
            size="sm"
            className="gap-2"
            disabled={busy}
            onClick={openRouteSelection}
          >
            <MapPinned className="size-4" />
            Set Goal
          </Button>
        </div>
      </CardHeader>
      <CardContent>
        <div className="grid gap-3">
          <PoseSummary label="Goal" pose={routeGoalPose} />
        </div>
        <div className="mt-4 flex flex-wrap gap-2 max-sm:[&_[data-slot=button]]:flex-1">
          <Button
            type="button"
            className="gap-2"
            disabled={busy || !raiNavigationRunning || !routeGoalPose}
            onClick={() => void runSelectedRoute()}
          >
            <Navigation className="size-4" />
            Send Goal
          </Button>
          <Button type="button" variant="outline" disabled={busy || !routeGoalPose} onClick={clearRoute}>
            Clear
          </Button>
          <Button type="button" variant="outline" className="gap-2" disabled={busy} onClick={() => void cancelGoal()}>
            <Square className="size-4" />
            Cancel Nav
          </Button>
        </div>
      </CardContent>
    </Card>
  );
}

export interface AnchorPanelProps {
  anchors: RobotAnchors;
  busy: boolean;
  goHome: () => Promise<void>;
  openHomePoseSelection: () => void;
  openInitialPoseSelection: () => void;
}

export function AnchorPanel({
  anchors,
  busy,
  goHome,
  openHomePoseSelection,
  openInitialPoseSelection,
}: AnchorPanelProps) {
  return (
    <Card>
      <CardHeader className="pb-3">
        <CardTitle className="text-sm font-bold uppercase tracking-wider text-slate-400">
          Anchors
        </CardTitle>
      </CardHeader>
      <CardContent>
        <div className="grid gap-3">
          <PoseSummary label="Initial Pose" pose={anchors.initial_pose} />
          <PoseSummary label="Home Pose" pose={anchors.home_pose} />
        </div>
        <div className="mt-4 flex flex-wrap gap-2 max-sm:[&_[data-slot=button]]:flex-1">
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
      </CardContent>
    </Card>
  );
}

export function MonitorMessage({
  message,
  telemetry,
}: {
  message: string;
  telemetry: RobotUiTelemetry | null;
}) {
  return (
    <Card>
      <CardContent className="px-4 py-3 text-sm text-slate-700">
        <div>{message}</div>
        {(telemetry?.humans.length ?? 0) > 0 && (
          <div className="mt-3 grid gap-2 md:grid-cols-2">
            {telemetry?.humans.map((human, index) => (
              <div
                key={`human-${human.id ?? index}`}
                className="rounded-md border border-slate-200 bg-slate-50 px-3 py-2"
              >
                H{human.id ?? "?"}: ({formatNumber(human.x, 2)}, {formatNumber(human.y, 2)}) conf{" "}
                {formatNumber(human.confidence, 2)}
              </div>
            ))}
          </div>
        )}
      </CardContent>
    </Card>
  );
}

function PoseSummary({ label, pose }: { label: string; pose: Pose2D | null }) {
  return (
    <div className="rounded-md border border-slate-200 bg-slate-50 px-3 py-2 text-sm text-slate-700">
      <div className="text-xs font-medium uppercase tracking-wider text-slate-400">
        {label}
      </div>
      <div className="mt-1 break-words">
        {pose
          ? `x=${pose.x.toFixed(2)}, y=${pose.y.toFixed(2)}, theta=${pose.yaw.toFixed(2)}`
          : "Not set"}
      </div>
    </div>
  );
}

function InlineMetric({
  label,
  value,
  icon,
}: {
  label: string;
  value: string;
  icon: ReactNode;
}) {
  return (
    <div className="flex items-center gap-2">
      {icon}
      <span>
        {label} {value}
      </span>
    </div>
  );
}
