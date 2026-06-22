export interface RaiTelemetry {
  odom?: {
    x?: number;
    y?: number;
    theta?: number;
    linear_x?: number;
    linear_y?: number;
    angular_z?: number;
  };
  battery?: {
    voltage?: number;
    percentage?: number;
  };
  charging?: boolean;
  context?: Record<string, unknown>;
  humans?: unknown[];
  solver?: Record<string, unknown>;
  lidar_clearance?: {
    left?: number;
    right?: number;
  };
  map_info?: RobotUiTelemetry["map_info"];
  map_pose?: {
    x?: number;
    y?: number;
    yaw?: number;
  };
  last_update?: number;
}

export interface RobotUiTelemetry {
  connected: boolean;
  battery_percent: number | null;
  voltage: number | null;
  charging: boolean;
  pos_x: number | null;
  pos_y: number | null;
  yaw: number | null;
  speed: number | null;
  map_pose: {
    x: number | null;
    y: number | null;
    yaw: number | null;
  };
  navigation_mode: string | null;
  human_count: number;
  tracking_quality: number | null;
  plan: Array<{ x: number; y: number }>;
  local_plan: Array<{ x: number; y: number }>;
  humans: Array<{
    id?: number;
    x?: number;
    y?: number;
    vx?: number;
    vy?: number;
    confidence?: number;
  }>;
  map_info: {
    resolution: number;
    width: number;
    height: number;
    origin?: { x?: number; y?: number };
  } | null;
}

export interface AiUiTelemetry {
  state?: string;
  ready: boolean;
  fps?: number;
  persons: number;
  obstacles: number;
}

function numberOrNull(value: unknown) {
  return typeof value === "number" && Number.isFinite(value) ? value : null;
}

export function normalizeRobotTelemetry(payload: RaiTelemetry | null | undefined): {
  robot: RobotUiTelemetry;
  ai: AiUiTelemetry;
} {
  const odom = payload?.odom ?? {};
  const context = payload?.context ?? {};
  const humans = Array.isArray(payload?.humans) ? payload.humans : [];
  const solver = payload?.solver ?? {};
  const lastUpdate = numberOrNull(payload?.last_update);
  const connected = lastUpdate === null || Date.now() / 1000 - lastUpdate < 5;
  const x = numberOrNull(odom.x);
  const y = numberOrNull(odom.y);
  const yaw = numberOrNull(odom.theta);
  const mapPose = payload?.map_pose ?? {};
  const mapX = numberOrNull(mapPose.x);
  const mapY = numberOrNull(mapPose.y);
  const mapYaw = numberOrNull(mapPose.yaw);
  const vx = numberOrNull(odom.linear_x) ?? 0;
  const vy = numberOrNull(odom.linear_y) ?? 0;

  return {
    robot: {
      connected,
      battery_percent: numberOrNull(payload?.battery?.percentage),
      voltage: numberOrNull(payload?.battery?.voltage),
      charging: Boolean(payload && "charging" in payload ? (payload as RaiTelemetry & { charging?: boolean }).charging : false),
      pos_x: x,
      pos_y: y,
      yaw,
      speed: Math.sqrt(vx * vx + vy * vy),
      map_pose: { x: mapX ?? x, y: mapY ?? y, yaw: mapYaw ?? yaw },
      navigation_mode: typeof context.navigation_mode === "string" ? context.navigation_mode : null,
      human_count: typeof context.human_count === "number" ? context.human_count : humans.length,
      tracking_quality: numberOrNull(context.tracking_quality),
      plan: [],
      local_plan: [],
      humans: humans as RobotUiTelemetry["humans"],
      map_info:
        payload && typeof (payload as RaiTelemetry & { map_info?: unknown }).map_info === "object"
          ? ((payload as RaiTelemetry & { map_info?: RobotUiTelemetry["map_info"] }).map_info ?? null)
          : null,
    },
    ai: {
      state: typeof context.legacy_context === "string" ? context.legacy_context : undefined,
      ready: true,
      fps: numberOrNull(solver.fps) ?? undefined,
      persons: humans.length,
      obstacles: 0,
    },
  };
}
