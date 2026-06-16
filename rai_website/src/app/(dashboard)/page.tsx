"use client";

import { MetricCard } from "@/components/ui/MetricCard";
import { StatusBadge } from "@/components/ui/StatusBadge";
import { useWebSocket } from "@/hooks/useWebSocket";
import { normalizeRobotTelemetry } from "@/lib/robot-telemetry";
import { Activity, Battery, Video, Users } from "lucide-react";
import { useState } from "react";

interface RobotTelemetry {
  connected?: boolean;
  battery_percent?: number | null;
  pos_x?: number | null;
  pos_y?: number | null;
  yaw?: number | null;
  map_pose?: {
    x?: number | null;
    y?: number | null;
    yaw?: number | null;
  } | null;
}

interface AiMetrics {
  state?: string;
  ready?: boolean;
  result_connected?: boolean;
  fps?: number;
  mode?: string;
  persons?: number;
  obstacles?: number;
}

interface TelemetryPayload {
  robot?: RobotTelemetry;
  ai?: AiMetrics;
}

const formatNumber = (value: number | null | undefined, digits = 1) =>
  typeof value === "number" && Number.isFinite(value) ? value.toFixed(digits) : "-";

export default function Dashboard() {
  const [telemetry, setTelemetry] = useState<TelemetryPayload>({});

  const { isConnected } = useWebSocket("/ws/telemetry", {
    onMessage: (msg) => {
      try {
        setTelemetry(normalizeRobotTelemetry(JSON.parse(msg.data)));
      } catch {
        setTelemetry({});
      }
    },
  });

  const robot = telemetry.robot ?? {};
  const ai = telemetry.ai ?? {};
  const robotOnline = isConnected && robot.connected !== false;
  const robotPose = {
    x: robot.map_pose?.x ?? robot.pos_x ?? null,
    y: robot.map_pose?.y ?? robot.pos_y ?? null,
    yaw: robot.map_pose?.yaw ?? robot.yaw ?? null,
  };

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <h2 className="text-2xl font-bold text-slate-800 tracking-tight">Overview</h2>
        <div className="flex items-center gap-3">
          <span className="text-sm font-medium text-slate-500">Robot Status:</span>
          <StatusBadge status={robotOnline ? "success" : "error"}>
            {robotOnline ? "ONLINE" : "OFFLINE"}
          </StatusBadge>
        </div>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-6">
        <MetricCard
          title="Adaptive State"
          value={ai.state ?? ai.mode ?? "-"}
          icon={<Activity className="w-5 h-5 text-blue-500" />}
        />
        <MetricCard
          title="Battery"
          value={
            typeof robot.battery_percent === "number"
              ? `${robot.battery_percent.toFixed(1)}%`
              : "-"
          }
          icon={<Battery className="w-5 h-5 text-emerald-500" />}
        />
        <MetricCard
          title="Adaptive FPS"
          value={formatNumber(ai.fps)}
          icon={<Video className="w-5 h-5 text-purple-500" />}
        />
        <MetricCard
          title="Tracked Entities"
          value={typeof ai.persons === "number" ? ai.persons : "-"}
          icon={<Users className="w-5 h-5 text-amber-500" />}
        />
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        <div className="lg:col-span-2 bg-white rounded-xl border border-slate-200 p-6 shadow-sm min-h-[400px] flex flex-col">
          <h3 className="text-lg font-semibold text-slate-800 mb-4">Live Position</h3>
          <div className="flex-1 bg-slate-100 rounded-lg border border-slate-200 flex items-center justify-center">
            {robotOnline ? (
              <div className="grid grid-cols-3 gap-6 text-center">
                <div>
                  <p className="text-xs uppercase text-slate-400 font-semibold">X</p>
                  <p className="text-2xl font-bold text-slate-800">{formatNumber(robotPose.x, 3)}</p>
                </div>
                <div>
                  <p className="text-xs uppercase text-slate-400 font-semibold">Y</p>
                  <p className="text-2xl font-bold text-slate-800">{formatNumber(robotPose.y, 3)}</p>
                </div>
                <div>
                  <p className="text-xs uppercase text-slate-400 font-semibold">Yaw</p>
                  <p className="text-2xl font-bold text-slate-800">{formatNumber(robotPose.yaw, 3)}</p>
                </div>
              </div>
            ) : (
              <span className="text-slate-400 font-medium">No robot telemetry received.</span>
            )}
          </div>
        </div>

        <div className="bg-white rounded-xl border border-slate-200 p-6 shadow-sm min-h-[400px] flex flex-col">
          <h3 className="text-lg font-semibold text-slate-800 mb-4">Runtime Sources</h3>
          <div className="flex-1 overflow-auto space-y-4">
            <RuntimeRow label="FastAPI" value={isConnected ? "telemetry websocket connected" : "waiting for /api/ws/telemetry"} />
            <RuntimeRow label="ROS2 odom" value={robotOnline ? "live /odom_combined updates" : "no fresh odom update"} />
            <RuntimeRow label="CCA-NMPC" value={ai.state ? `context ${ai.state}` : "waiting for /canmpc/context"} />
            <RuntimeRow label="Battery" value={typeof robot.battery_percent === "number" ? `${robot.battery_percent.toFixed(1)}%` : "waiting for /voltage"} />
          </div>
        </div>
      </div>
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
