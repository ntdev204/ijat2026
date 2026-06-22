import type { RefObject } from "react";
import type { RobotUiTelemetry } from "@/lib/robot-telemetry";
import type { MapPayload, PathsPayload, StreamState } from "@/types/robot-runtime";

export interface MonitorRuntime {
  videoRef: RefObject<HTMLVideoElement | null>;
  mapCanvasRef: RefObject<HTMLCanvasElement | null>;
  state: StreamState;
  message: string;
  telemetry: RobotUiTelemetry | null;
  mapData: MapPayload | null;
  paths: PathsPayload;
  live: boolean;
  showSavedMap: (mapId: number) => Promise<void>;
}
