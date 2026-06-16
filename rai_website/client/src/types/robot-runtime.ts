import type { RobotUiTelemetry } from "@/lib/robot-telemetry";

export interface Point2D {
  x: number;
  y: number;
}

export interface Pose2D extends Point2D {
  yaw: number;
}

export interface MapPayload {
  id?: number;
  name?: string;
  width: number;
  height: number;
  resolution: number;
  origin_x: number;
  origin_y: number;
  grid_data: string;
  yaml_path?: string | null;
  pgm_path?: string | null;
  created_at?: string | null;
}

export interface PathsPayload {
  global_plan?: Point2D[];
  local_plan?: Point2D[];
}

export interface Nav2Option {
  id: string;
  label: string;
}

export interface Nav2Config {
  local_planner: string;
  global_planner: string;
  map_path: string;
  running: boolean;
}

export interface DatasetRun {
  id: number;
  run_name: string;
  environment: string;
  controller_id: string;
  status: string;
  data_path: string;
  raw_bag_path?: string;
  metadata_path?: string;
  zip_path?: string;
  duration?: number | null;
  samples_count?: number | null;
  start_time?: string | null;
}

export interface DatasetScenario {
  id: number;
  name: string;
  context_type?: string;
  difficulty?: string;
  human_mode?: string;
  description?: string;
}

export interface ActiveDatasetPayload {
  active: boolean;
  run: DatasetRun | null;
  telemetry?: unknown;
  metadata?: Record<string, unknown> | null;
}

export type RuntimeEnvironment = "real" | "sim";
export type StreamState = "idle" | "connecting" | "live" | "error";
export type GazeboViewMode = "gazebo" | "top";

export interface RuntimeSnapshot {
  map: MapPayload | null;
  paths: PathsPayload;
  telemetry: RobotUiTelemetry | null;
}

export interface RobotAnchors {
  initial_pose: Pose2D | null;
  home_pose: Pose2D | null;
}
