import type { MapPayload, PathsPayload, Point2D, Pose2D } from "@/types/robot-runtime";
import type { RobotUiTelemetry } from "@/lib/robot-telemetry";

export interface MapCanvasOverlay {
  paths?: PathsPayload;
  telemetry?: RobotUiTelemetry | null;
  goal?: Point2D | null;
  markers?: PoseMarker[];
  showHumanLabels?: boolean;
  showRobot?: boolean;
}

export interface PoseMarker {
  color: string;
  label: string;
  pose: Pose2D;
}

export function worldToPixel(point: Point2D, map: MapPayload): Point2D {
  return {
    x: (point.x - map.origin_x) / map.resolution,
    y: map.height - (point.y - map.origin_y) / map.resolution,
  };
}

export function pixelToWorld(point: Point2D, map: MapPayload): Point2D {
  return {
    x: map.origin_x + point.x * map.resolution,
    y: map.origin_y + (map.height - point.y) * map.resolution,
  };
}

export function decodeGridData(gridData: string) {
  const binary = window.atob(gridData);
  const values = new Uint8Array(binary.length);
  for (let index = 0; index < binary.length; index += 1) {
    values[index] = binary.charCodeAt(index);
  }
  return values;
}

export function occupancyColor(value: number) {
  if (value === 255) return 205;
  if (value >= 100) return 24;
  if (value === 0) return 245;
  return Math.max(40, 245 - Math.round(value * 2));
}

function clamp(value: number, min: number, max: number) {
  return Math.min(max, Math.max(min, value));
}

function scaledOverlaySize(map: MapPayload, ratio: number, min: number, max: number) {
  return clamp(Math.min(map.width, map.height) * ratio, min, max);
}

export function drawOccupancyMap(canvas: HTMLCanvasElement, map: MapPayload, overlay: MapCanvasOverlay = {}) {
  const ctx = canvas.getContext("2d");
  if (!ctx) return;

  canvas.width = map.width;
  canvas.height = map.height;
  const image = ctx.createImageData(map.width, map.height);
  const grid = decodeGridData(map.grid_data);

  for (let y = 0; y < map.height; y += 1) {
    for (let x = 0; x < map.width; x += 1) {
      const src = (map.height - 1 - y) * map.width + x;
      const dst = (y * map.width + x) * 4;
      const color = occupancyColor(grid[src] ?? 255);
      image.data[dst] = color;
      image.data[dst + 1] = color;
      image.data[dst + 2] = color;
      image.data[dst + 3] = 255;
    }
  }

  ctx.putImageData(image, 0, 0);
  drawPath(ctx, map, overlay.paths?.global_plan, "#2563eb", 3);
  drawPath(ctx, map, overlay.paths?.local_plan, "#f97316", 2);
  drawHumans(ctx, map, overlay.telemetry, Boolean(overlay.showHumanLabels));
  if (overlay.goal) drawGoal(ctx, map, overlay.goal);
  drawMarkers(ctx, map, overlay.markers ?? []);
  if (overlay.showRobot !== false) drawRobot(ctx, map, overlay.telemetry);
}

function drawPath(
  ctx: CanvasRenderingContext2D,
  map: MapPayload,
  points: Point2D[] | undefined,
  color: string,
  width: number,
) {
  if (!points || points.length < 2) return;
  ctx.save();
  ctx.strokeStyle = color;
  ctx.lineWidth = Math.min(width, scaledOverlaySize(map, 0.012, 1, width));
  ctx.beginPath();
  points.forEach((point, index) => {
    const pixel = worldToPixel(point, map);
    if (index === 0) ctx.moveTo(pixel.x, pixel.y);
    else ctx.lineTo(pixel.x, pixel.y);
  });
  ctx.stroke();
  ctx.restore();
}

function drawHumans(ctx: CanvasRenderingContext2D, map: MapPayload, telemetry?: RobotUiTelemetry | null, showLabels = false) {
  const radius = scaledOverlaySize(map, 0.025, 2.5, 7);
  for (const human of telemetry?.humans ?? []) {
    if (typeof human.x !== "number" || typeof human.y !== "number") continue;
    const pixel = worldToPixel({ x: human.x, y: human.y }, map);
    ctx.save();
    ctx.fillStyle = "rgba(220, 38, 38, 0.85)";
    ctx.beginPath();
    ctx.arc(pixel.x, pixel.y, radius, 0, Math.PI * 2);
    ctx.fill();
    if (showLabels) {
      ctx.fillStyle = "#ffffff";
      ctx.font = "12px sans-serif";
      ctx.fillText(`H${human.id ?? "?"}`, pixel.x + 10, pixel.y - 8);
    }
    ctx.restore();
  }
}

function drawGoal(ctx: CanvasRenderingContext2D, map: MapPayload, goal: Point2D) {
  const pixel = worldToPixel(goal, map);
  const radius = scaledOverlaySize(map, 0.035, 4, 8);
  ctx.save();
  ctx.strokeStyle = "#dc2626";
  ctx.lineWidth = scaledOverlaySize(map, 0.01, 1, 3);
  ctx.beginPath();
  ctx.arc(pixel.x, pixel.y, radius, 0, Math.PI * 2);
  ctx.moveTo(pixel.x - radius * 1.5, pixel.y);
  ctx.lineTo(pixel.x + radius * 1.5, pixel.y);
  ctx.moveTo(pixel.x, pixel.y - radius * 1.5);
  ctx.lineTo(pixel.x, pixel.y + radius * 1.5);
  ctx.stroke();
  ctx.restore();
}

function drawRobot(ctx: CanvasRenderingContext2D, map: MapPayload, telemetry?: RobotUiTelemetry | null) {
  const pose = telemetry?.map_pose;
  if (pose?.x == null || pose.y == null) return;

  const robot = worldToPixel({ x: pose.x, y: pose.y }, map);
  const body = scaledOverlaySize(map, 0.035, 3, 7);
  const nose = body * 1.7;
  const tail = body * 1.1;
  ctx.save();
  ctx.translate(robot.x, robot.y);
  ctx.rotate(-(pose.yaw ?? 0));
  ctx.fillStyle = "#16a34a";
  ctx.strokeStyle = "#052e16";
  ctx.lineWidth = scaledOverlaySize(map, 0.008, 1, 2);
  ctx.beginPath();
  ctx.moveTo(nose, 0);
  ctx.lineTo(-tail, body);
  ctx.lineTo(-tail, -body);
  ctx.closePath();
  ctx.fill();
  ctx.stroke();
  ctx.restore();
}

function drawMarkers(ctx: CanvasRenderingContext2D, map: MapPayload, markers: PoseMarker[]) {
  const radius = scaledOverlaySize(map, 0.04, 4, 10);
  const arrow = scaledOverlaySize(map, 0.045, 5, 11);
  for (const marker of markers) {
    const pixel = worldToPixel(marker.pose, map);
    ctx.save();
    ctx.translate(pixel.x, pixel.y);
    ctx.rotate(-(marker.pose.yaw ?? 0));
    ctx.strokeStyle = marker.color;
    ctx.fillStyle = "#ffffff";
    ctx.lineWidth = scaledOverlaySize(map, 0.008, 1, 2);
    ctx.beginPath();
    ctx.arc(0, 0, radius, 0, Math.PI * 2);
    ctx.fill();
    ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(arrow, 0);
    ctx.lineTo(-arrow * 0.5, arrow * 0.45);
    ctx.lineTo(-arrow * 0.5, -arrow * 0.45);
    ctx.closePath();
    ctx.fillStyle = marker.color;
    ctx.fill();
    ctx.stroke();
    ctx.restore();

    ctx.save();
    ctx.fillStyle = marker.color;
    ctx.font = "bold 12px sans-serif";
    ctx.fillText(marker.label, pixel.x + 12, pixel.y - 12);
    ctx.restore();
  }
}
