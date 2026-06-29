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

export interface RobotModelOption {
  id: string;
  label: string;
  urdf_path: string;
}

export interface SimulationWorldOption {
  id: string;
  label: string;
  source: "package" | "generated" | string;
  path: string;
  created_at?: string | null;
}

export interface RvizTopics {
  map_topic: string;
  global_path_topic: string;
  local_path_topic: string;
}

export interface RaiNavigationOption {
  id: string;
  label: string;
}

export interface RaiNavigationConfig {
  local_planner: string;
  global_planner: string;
  map_path: string;
  selected_map_id?: number | null;
  running: boolean;
  controller_server_enabled?: boolean;
  cca_nmpc_running?: boolean;
}

export interface SystemRuntime {
  device_role: string;
  device_label: string;
  allowed_actions: string[];
  api_host: string;
  api_port: number;
  operation_mode?: "real" | "sim" | "hybrid";
  operation_mode_options?: Array<{
    id: "real" | "sim" | "hybrid";
    label: string;
    description: string;
  }>;
  peer_devices?: {
    pi?: string | null;
    jetson?: string | null;
  };
}

export interface SystemComponent {
  id: "robot_base" | "lidar" | "camera" | "slam" | "navigation" | "simulation" | "dataset";
  label: string;
  host_device: string;
  action: string;
  allowed_here: boolean;
  running: boolean;
  pid?: number | null;
  launch_file: string;
  description: string;
  capabilities?: {
    enable_depth_toggle?: boolean;
    enable_depth?: boolean;
  };
  proxy_error?: string;
}

export interface SystemComponentsPayload extends SystemRuntime {
  components: SystemComponent[];
}

export interface DatasetRun {
  id: number;
  run_name: string;
  scenario_name?: string;
  environment: string;
  controller_id: string;
  run_index?: number | null;
  split?: string;
  status: string;
  validation_status?: string | null;
  success?: boolean | null;
  data_path: string;
  raw_bag_path?: string;
  metadata_path?: string;
  zip_path?: string;
  duration?: number | null;
  samples_count?: number | null;
  phi_h_max?: number | null;
  min_human_clearance?: number | null;
  timeout_rate?: number | null;
  metrics?: Record<string, unknown>;
  start_voltage?: number | null;
  end_voltage?: number | null;
  avg_percentage?: number | null;
  file_size_bytes?: number | null;
  start_time?: string | null;
}

export interface DatasetScenario {
  id: number;
  name: string;
  context_type?: string;
  difficulty?: string;
  human_mode?: string;
  expected_runs?: number | null;
  description?: string;
  scientific_goal?: string;
  layout?: string;
  human_count?: string;
  human_speed?: string;
  human_trajectory?: string;
  primary_metric?: string;
}

export interface ActiveDatasetPayload {
  active: boolean;
  run: DatasetRun | null;
  telemetry?: unknown;
  metadata?: Record<string, unknown> | null;
}

export interface DatasetArtifactsPayload {
  base_path: string;
  required_topics: string[];
  optional_camera_topics: string[];
  files: Record<string, boolean>;
  baseline_controllers?: Array<{ id: string; label?: string; description?: string }>;
  ablation_controllers?: Array<{ id: string; label?: string; description?: string }>;
  dataset_groups?: Array<{ id: string; label?: string; objective?: string }>;
  required_figures?: string[];
  statistical_validation?: {
    minimum_runs_per_condition?: number;
    recommended_runs_per_core_scenario?: number;
    recommended_runs_for_s6?: number;
    reporting?: string;
    significance_tests?: string[];
    effect_size?: string[];
  };
}

export interface DatasetLaunchStatus {
  running: boolean;
  pid?: number | null;
  active_run_id?: string | null;
}

export interface DatasetPipelineResult {
  success: boolean;
  action: string;
  results: Array<{
    command: string[];
    returncode: number;
    stdout: string;
    stderr: string;
    duration_sec: number;
    started_at?: string;
    finished_at?: string;
  }>;
  artifacts: DatasetArtifactsPayload;
}

export type RuntimeEnvironment = "real" | "sim";
export type StreamState = "idle" | "connecting" | "live" | "error";
export type RvizViewMode = "perspective" | "top" | "follow";

export interface RuntimeSnapshot {
  map: MapPayload | null;
  paths: PathsPayload;
  telemetry: RobotUiTelemetry | null;
}

export interface RobotAnchors {
  initial_pose: Pose2D | null;
  home_pose: Pose2D | null;
}
