"use client";

import { Activity, Battery, Video, Users } from "lucide-react";
import { MetricCard } from "@/components/ui/MetricCard";
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card";
import { formatNumber } from "@/lib/format";
import type { OverviewTelemetryState } from "@/types/overview";

export function OverviewMetrics({
  ai,
  batteryPercent,
}: {
  ai: OverviewTelemetryState["ai"];
  batteryPercent: number | null;
}) {
  return (
    <div className="grid grid-cols-1 gap-4 md:grid-cols-2 lg:grid-cols-4 xl:gap-6">
      <MetricCard
        title="Adaptive State"
        value={ai.state ?? "-"}
        icon={<Activity className="h-5 w-5 text-blue-500" />}
      />
      <MetricCard
        title="Battery"
        value={typeof batteryPercent === "number" ? `${batteryPercent.toFixed(1)}%` : "-"}
        icon={<Battery className="h-5 w-5 text-emerald-500" />}
      />
      <MetricCard
        title="Adaptive FPS"
        value={formatNumber(ai.fps, 1)}
        icon={<Video className="h-5 w-5 text-cyan-500" />}
      />
      <MetricCard
        title="Tracked Entities"
        value={typeof ai.persons === "number" ? String(ai.persons) : "-"}
        icon={<Users className="h-5 w-5 text-amber-500" />}
      />
    </div>
  );
}

export function OverviewPositionSection({
  robotOnline,
  robotPose,
}: Pick<OverviewTelemetryState, "robotOnline" | "robotPose">) {
  return (
    <Card className="min-h-[320px] lg:col-span-2 lg:min-h-[400px]">
      <CardHeader>
        <CardTitle className="text-lg font-semibold text-slate-800">
          Live Position
        </CardTitle>
      </CardHeader>
      <CardContent>
        <div className="flex min-h-[220px] items-center justify-center rounded-lg border border-slate-200 bg-slate-100 sm:h-[300px]">
          {robotOnline ? (
            <div className="grid w-full grid-cols-1 gap-4 text-center sm:w-auto sm:grid-cols-3 sm:gap-6">
              <PositionMetric label="X" value={robotPose.x} />
              <PositionMetric label="Y" value={robotPose.y} />
              <PositionMetric label="Yaw" value={robotPose.yaw} />
            </div>
          ) : (
            <span className="font-medium text-slate-400">
              No robot telemetry received.
            </span>
          )}
        </div>
      </CardContent>
    </Card>
  );
}

export function OverviewRuntimeSources({
  isConnected,
  robotOnline,
  adaptiveState,
  batteryPercent,
}: {
  isConnected: boolean;
  robotOnline: boolean;
  adaptiveState: string | undefined;
  batteryPercent: number | null;
}) {
  return (
    <Card className="min-h-[320px] lg:min-h-[400px]">
      <CardHeader>
        <CardTitle className="text-lg font-semibold text-slate-800">
          Runtime Sources
        </CardTitle>
      </CardHeader>
      <CardContent>
        <div className="space-y-4">
          <RuntimeRow
            label="FastAPI"
            value={isConnected ? "Telemetry websocket connected" : "Waiting for /api/ws/telemetry"}
          />
          <RuntimeRow
            label="ROS2 odom"
            value={robotOnline ? "Live /odom_combined updates" : "No fresh odom update"}
          />
          <RuntimeRow
            label="CCA-NMPC"
            value={adaptiveState ? `Context ${adaptiveState}` : "Waiting for /canmpc/context"}
          />
          <RuntimeRow
            label="Battery"
            value={typeof batteryPercent === "number" ? `${batteryPercent.toFixed(1)}%` : "Waiting for /voltage"}
          />
        </div>
      </CardContent>
    </Card>
  );
}

function PositionMetric({
  label,
  value,
}: {
  label: string;
  value: number | null;
}) {
  return (
    <div>
      <p className="text-xs font-semibold uppercase text-slate-400">{label}</p>
      <p className="text-xl font-bold text-slate-800 sm:text-2xl">{formatNumber(value, 3)}</p>
    </div>
  );
}

function RuntimeRow({ label, value }: { label: string; value: string }) {
  return (
    <div className="rounded-lg border border-slate-100 bg-slate-50 px-4 py-3 text-sm">
      <div className="font-semibold text-slate-800">{label}</div>
      <div className="mt-1 text-slate-500">{value}</div>
    </div>
  );
}
