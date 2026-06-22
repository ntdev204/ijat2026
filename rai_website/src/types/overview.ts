import type { AiUiTelemetry, RobotUiTelemetry } from "@/lib/robot-telemetry";

export interface OverviewTelemetryState {
  robot: RobotUiTelemetry;
  ai: AiUiTelemetry;
  isConnected: boolean;
  robotOnline: boolean;
  robotPose: {
    x: number | null;
    y: number | null;
    yaw: number | null;
  };
}
