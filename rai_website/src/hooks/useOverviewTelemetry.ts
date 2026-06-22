"use client";

import { useMemo, useState } from "react";
import { useWebSocket } from "@/hooks/useWebSocket";
import { normalizeRobotTelemetry } from "@/lib/robot-telemetry";
import type { OverviewTelemetryState } from "@/types/overview";

const EMPTY_TELEMETRY = normalizeRobotTelemetry(null);

export function useOverviewTelemetry(): OverviewTelemetryState {
  const [telemetry, setTelemetry] = useState(EMPTY_TELEMETRY);

  const { isConnected } = useWebSocket("/ws/telemetry", {
    onMessage: (msg) => {
      try {
        setTelemetry(normalizeRobotTelemetry(JSON.parse(msg.data)));
      } catch {
        setTelemetry(EMPTY_TELEMETRY);
      }
    },
  });

  return useMemo(() => {
    const robot = telemetry.robot;
    const ai = telemetry.ai;
    const robotOnline = isConnected && robot.connected !== false;

    return {
      robot,
      ai,
      isConnected,
      robotOnline,
      robotPose: {
        x: robot.map_pose.x ?? robot.pos_x ?? null,
        y: robot.map_pose.y ?? robot.pos_y ?? null,
        yaw: robot.map_pose.yaw ?? robot.yaw ?? null,
      },
    };
  }, [isConnected, telemetry.ai, telemetry.robot]);
}
