"use client";

import { Button } from "@/components/ui/button";
import { Input } from "@/components/ui/input";
import { useWebSocket } from "@/hooks/useWebSocket";
import { fetchWithAuth } from "@/lib/api";
import { normalizeRobotTelemetry, type RobotUiTelemetry } from "@/lib/robot-telemetry";
import { LocateFixed, Map as MapIcon, Navigation, Play, RefreshCw, Save, Square, Users, Waypoints } from "lucide-react";
import { useCallback, useEffect, useRef, useState } from "react";

interface MapPayload {
  id?: number;
  name?: string;
  width: number;
  height: number;
  resolution: number;
  origin_x: number;
  origin_y: number;
  grid_data: string;
  created_at?: string | null;
}

interface PathsPayload {
  global_plan?: Array<{ x: number; y: number }>;
  local_plan?: Array<{ x: number; y: number }>;
}

interface Nav2Option {
  id: string;
  label: string;
}

interface Nav2Config {
  local_planner: string;
  global_planner: string;
  map_path: string;
  running: boolean;
}

interface Point2D {
  x: number;
  y: number;
}

function worldToPixel(point: Point2D, map: MapPayload): Point2D {
  return {
    x: (point.x - map.origin_x) / map.resolution,
    y: map.height - (point.y - map.origin_y) / map.resolution,
  };
}

function pixelToWorld(point: Point2D, map: MapPayload): Point2D {
  return {
    x: map.origin_x + point.x * map.resolution,
    y: map.origin_y + (map.height - point.y) * map.resolution,
  };
}

function decodeGridData(gridData: string) {
  const binary = window.atob(gridData);
  const values = new Uint8Array(binary.length);
  for (let index = 0; index < binary.length; index += 1) {
    values[index] = binary.charCodeAt(index);
  }
  return values;
}

function occupancyColor(value: number) {
  if (value === 255) return 205;
  if (value >= 100) return 24;
  if (value === 0) return 245;
  return Math.max(40, 245 - Math.round(value * 2));
}

export default function MapPage() {
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const [maps, setMaps] = useState<MapPayload[]>([]);
  const [currentMap, setCurrentMap] = useState<MapPayload | null>(null);
  const [telemetry, setTelemetry] = useState<RobotUiTelemetry | null>(null);
  const [paths, setPaths] = useState<PathsPayload>({});
  const [mapName, setMapName] = useState("");
  const [lastGoal, setLastGoal] = useState<Point2D | null>(null);
  const [busy, setBusy] = useState(false);
  const [status, setStatus] = useState("Waiting for map data.");
  const [nav2LocalOptions, setNav2LocalOptions] = useState<Nav2Option[]>([]);
  const [nav2GlobalOptions, setNav2GlobalOptions] = useState<Nav2Option[]>([]);
  const [nav2Config, setNav2Config] = useState<Nav2Config>({
    local_planner: "CA_NMPC",
    global_planner: "A_STAR",
    map_path: "/home/rai/rai_ros2/data/map/RAI.yaml",
    running: false,
  });

  useWebSocket("/ws/telemetry", {
    onMessage: (event) => {
      try {
        setTelemetry(normalizeRobotTelemetry(JSON.parse(String(event.data))).robot);
      } catch {
        setTelemetry(null);
      }
    },
  });

  useWebSocket("/ws/map", {
    onMessage: (event) => {
      try {
        const payload = JSON.parse(String(event.data)) as MapPayload;
        setCurrentMap(payload);
        setStatus("Live map updated from /api/ws/map.");
      } catch {
        setStatus("Invalid map payload.");
      }
    },
  });

  useWebSocket("/ws/paths", {
    onMessage: (event) => {
      try {
        setPaths(JSON.parse(String(event.data)) as PathsPayload);
      } catch {
        setPaths({});
      }
    },
  });

  const loadMaps = useCallback(async () => {
    try {
      const response = await fetchWithAuth("/api/map/list");
      const savedMaps = (await response.json()) as MapPayload[];
      setMaps(savedMaps);
      if (!currentMap && savedMaps[0]?.id != null) {
        const mapResponse = await fetchWithAuth(`/api/map/${savedMaps[0].id}`);
        setCurrentMap((await mapResponse.json()) as MapPayload);
      }
    } catch {
      setMaps([]);
    }
  }, [currentMap]);

  const loadNav2State = useCallback(async () => {
    try {
      const [optionsResponse, configResponse] = await Promise.all([
        fetchWithAuth("/api/nav2/options"),
        fetchWithAuth("/api/nav2/config"),
      ]);
      const options = await optionsResponse.json();
      const config = (await configResponse.json()) as Nav2Config;
      setNav2LocalOptions(options.local_planners ?? []);
      setNav2GlobalOptions(options.global_planners ?? []);
      setNav2Config(config);
    } catch {
      setNav2LocalOptions([]);
      setNav2GlobalOptions([]);
    }
  }, []);

  useEffect(() => {
    const timer = window.setTimeout(() => {
      void Promise.all([loadMaps(), loadNav2State()]);
    }, 0);
    return () => window.clearTimeout(timer);
  }, [loadMaps, loadNav2State]);

  useEffect(() => {
    const canvas = canvasRef.current;
    const map = currentMap;
    if (!canvas || !map) return;
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

    const drawPath = (points: Array<{ x: number; y: number }> | undefined, color: string, width: number) => {
      if (!points || points.length < 2) return;
      ctx.save();
      ctx.strokeStyle = color;
      ctx.lineWidth = width;
      ctx.beginPath();
      points.forEach((point, index) => {
        const pixel = worldToPixel(point, map);
        if (index === 0) ctx.moveTo(pixel.x, pixel.y);
        else ctx.lineTo(pixel.x, pixel.y);
      });
      ctx.stroke();
      ctx.restore();
    };

    drawPath(paths.global_plan, "#2563eb", 3);
    drawPath(paths.local_plan, "#f97316", 2);

    for (const human of telemetry?.humans ?? []) {
      if (typeof human.x !== "number" || typeof human.y !== "number") continue;
      const pixel = worldToPixel({ x: human.x, y: human.y }, map);
      ctx.save();
      ctx.fillStyle = "rgba(220, 38, 38, 0.85)";
      ctx.beginPath();
      ctx.arc(pixel.x, pixel.y, 7, 0, Math.PI * 2);
      ctx.fill();
      ctx.fillStyle = "#ffffff";
      ctx.font = "12px sans-serif";
      ctx.fillText(`H${human.id ?? "?"}`, pixel.x + 10, pixel.y - 8);
      ctx.restore();
    }

    if (lastGoal) {
      const goal = worldToPixel(lastGoal, map);
      ctx.save();
      ctx.strokeStyle = "#dc2626";
      ctx.lineWidth = 3;
      ctx.beginPath();
      ctx.arc(goal.x, goal.y, 8, 0, Math.PI * 2);
      ctx.moveTo(goal.x - 12, goal.y);
      ctx.lineTo(goal.x + 12, goal.y);
      ctx.moveTo(goal.x, goal.y - 12);
      ctx.lineTo(goal.x, goal.y + 12);
      ctx.stroke();
      ctx.restore();
    }

    const pose = telemetry?.map_pose;
    if (pose?.x != null && pose.y != null) {
      const robot = worldToPixel({ x: pose.x, y: pose.y }, map);
      ctx.save();
      ctx.translate(robot.x, robot.y);
      ctx.rotate(-(pose.yaw ?? 0));
      ctx.fillStyle = "#16a34a";
      ctx.strokeStyle = "#052e16";
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.moveTo(14, 0);
      ctx.lineTo(-10, 8);
      ctx.lineTo(-10, -8);
      ctx.closePath();
      ctx.fill();
      ctx.stroke();
      ctx.restore();
    }
  }, [currentMap, lastGoal, paths.global_plan, paths.local_plan, telemetry]);

  async function updateNav2Config(nextLocalPlanner: string, nextGlobalPlanner: string) {
    const response = await fetchWithAuth("/api/nav2/config", {
      method: "POST",
      body: JSON.stringify({
        local_planner: nextLocalPlanner,
        global_planner: nextGlobalPlanner,
        map_path: nav2Config.map_path,
      }),
    });
    setNav2Config((await response.json()) as Nav2Config);
  }

  async function startNav2() {
    setBusy(true);
    try {
      const response = await fetchWithAuth("/api/nav2/start", { method: "POST" });
      setNav2Config((await response.json()) as Nav2Config);
      setStatus(`Nav2 started with ${nav2Config.local_planner} / ${nav2Config.global_planner}.`);
    } catch (error) {
      setStatus(error instanceof Error ? error.message : "Nav2 start failed.");
    } finally {
      setBusy(false);
    }
  }

  async function stopNav2() {
    setBusy(true);
    try {
      const response = await fetchWithAuth("/api/nav2/stop", { method: "POST" });
      setNav2Config((await response.json()) as Nav2Config);
      setStatus("Nav2 stopped.");
    } catch (error) {
      setStatus(error instanceof Error ? error.message : "Nav2 stop failed.");
    } finally {
      setBusy(false);
    }
  }

  async function saveMap() {
    const name = mapName.trim();
    if (!name) {
      setStatus("Map name is required.");
      return;
    }
    setBusy(true);
    try {
      await fetchWithAuth("/api/map/save", { method: "POST", body: JSON.stringify({ name }) });
      setMapName("");
      setStatus(`Saved map ${name}.`);
      await loadMaps();
    } catch (error) {
      setStatus(error instanceof Error ? error.message : "Save map failed.");
    } finally {
      setBusy(false);
    }
  }

  async function sendGoal(event: React.MouseEvent<HTMLCanvasElement>) {
    const canvas = canvasRef.current;
    const map = currentMap;
    if (!canvas || !map) return;
    const rect = canvas.getBoundingClientRect();
    const pixel = {
      x: ((event.clientX - rect.left) / rect.width) * canvas.width,
      y: ((event.clientY - rect.top) / rect.height) * canvas.height,
    };
    const goal = pixelToWorld(pixel, map);
    setLastGoal(goal);
    setBusy(true);
    try {
      await fetchWithAuth("/api/robot/nav/goal", {
        method: "POST",
        body: JSON.stringify({ x: goal.x, y: goal.y, yaw: 0 }),
      });
      setStatus(`Goal sent: ${goal.x.toFixed(2)}, ${goal.y.toFixed(2)}.`);
    } catch (error) {
      setStatus(error instanceof Error ? error.message : "Goal send failed.");
    } finally {
      setBusy(false);
    }
  }

  async function cancelGoal() {
    setBusy(true);
    try {
      await fetchWithAuth("/api/robot/nav/cancel", { method: "POST" });
      setLastGoal(null);
      setStatus("Navigation goal cancelled.");
    } catch (error) {
      setStatus(error instanceof Error ? error.message : "Cancel failed.");
    } finally {
      setBusy(false);
    }
  }

  return (
    <div className="space-y-5">
      <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
        <div>
          <h2 className="text-2xl font-bold text-slate-800 tracking-tight">Map & Navigation</h2>
          <p className="text-sm text-slate-500">
            {currentMap ? `${currentMap.width}x${currentMap.height} @ ${currentMap.resolution}m` : status}
          </p>
        </div>
        <div className="flex gap-2">
          <Button variant="outline" className="gap-2" disabled={busy} onClick={() => void Promise.all([loadMaps(), loadNav2State()])}>
            <RefreshCw className="size-4" />
            Refresh
          </Button>
          <Button variant="outline" className="gap-2" disabled={busy || !lastGoal} onClick={() => void cancelGoal()}>
            <Square className="size-4" />
            Cancel Goal
          </Button>
        </div>
      </div>

      <div className="grid grid-cols-1 gap-5 xl:grid-cols-[minmax(0,1fr)_360px]">
        <section className="overflow-hidden rounded-lg border border-slate-200 bg-white shadow-sm">
          <div className="flex items-center gap-2 border-b border-slate-200 px-4 py-3 text-sm text-slate-600">
            <MapIcon className="size-4 text-blue-600" />
            <span className="font-medium text-slate-800">Realtime Occupancy Grid</span>
          </div>
          <div className="flex min-h-[520px] items-center justify-center overflow-auto bg-slate-100 p-4">
            {currentMap ? (
              <canvas
                ref={canvasRef}
                onClick={(event) => void sendGoal(event)}
                className="max-h-[calc(100vh-280px)] max-w-full cursor-crosshair rounded-md border border-slate-300 bg-white shadow-sm"
              />
            ) : (
              <div className="text-sm text-slate-400">Waiting for /map via /api/ws/map.</div>
            )}
          </div>
          <div className="grid gap-3 border-t border-slate-200 px-4 py-3 text-sm text-slate-600 md:grid-cols-4">
            <div className="flex items-center gap-2">
              <LocateFixed className="size-4 text-emerald-600" />
              <span>
                Robot {telemetry?.map_pose.x != null && telemetry.map_pose.y != null ? `${telemetry.map_pose.x.toFixed(2)}, ${telemetry.map_pose.y.toFixed(2)}` : "unknown"}
              </span>
            </div>
            <div className="flex items-center gap-2">
              <Waypoints className="size-4 text-blue-600" />
              <span>Global {paths.global_plan?.length ?? 0}</span>
            </div>
            <div className="flex items-center gap-2">
              <Navigation className="size-4 text-amber-600" />
              <span>Local {paths.local_plan?.length ?? 0}</span>
            </div>
            <div className="flex items-center gap-2">
              <Users className="size-4 text-rose-600" />
              <span>Humans {telemetry?.humans.length ?? 0}</span>
            </div>
          </div>
        </section>

        <aside className="space-y-4">
          <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
            <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">Nav2 Planner Selection</h3>
            <div className="grid gap-3">
              <label className="block text-sm">
                <span className="text-xs font-medium text-slate-500">Local planner</span>
                <select
                  value={nav2Config.local_planner}
                  onChange={(event) => void updateNav2Config(event.target.value, nav2Config.global_planner)}
                  className="mt-1 w-full rounded-md border border-slate-300 px-3 py-2 text-sm"
                >
                  {nav2LocalOptions.map((option) => <option key={option.id} value={option.id}>{option.label}</option>)}
                </select>
              </label>
              <label className="block text-sm">
                <span className="text-xs font-medium text-slate-500">Global planner</span>
                <select
                  value={nav2Config.global_planner}
                  onChange={(event) => void updateNav2Config(nav2Config.local_planner, event.target.value)}
                  className="mt-1 w-full rounded-md border border-slate-300 px-3 py-2 text-sm"
                >
                  {nav2GlobalOptions.map((option) => <option key={option.id} value={option.id}>{option.label}</option>)}
                </select>
              </label>
              <label className="block text-sm">
                <span className="text-xs font-medium text-slate-500">Map YAML path</span>
                <Input
                  value={nav2Config.map_path}
                  onChange={(event) => setNav2Config((current) => ({ ...current, map_path: event.target.value }))}
                  onBlur={() => void updateNav2Config(nav2Config.local_planner, nav2Config.global_planner)}
                />
              </label>
            </div>
            <div className="mt-4 flex gap-2">
              <Button type="button" className="gap-2" disabled={busy || nav2Config.running} onClick={() => void startNav2()}>
                <Play className="size-4" />
                Start Nav2
              </Button>
              <Button type="button" variant="outline" className="gap-2" disabled={busy || !nav2Config.running} onClick={() => void stopNav2()}>
                <Square className="size-4" />
                Stop
              </Button>
            </div>
            <p className="mt-3 text-sm text-slate-500">
              Running: {nav2Config.running ? "yes" : "no"} | {nav2Config.local_planner} + {nav2Config.global_planner}
            </p>
          </section>

          <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
            <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">Save Live Map</h3>
            <div className="flex gap-2">
              <Input value={mapName} onChange={(event) => setMapName(event.target.value)} placeholder="Map name" disabled={busy} />
              <Button type="button" size="icon" disabled={busy} onClick={() => void saveMap()} title="Save map">
                <Save className="size-4" />
              </Button>
            </div>
            <p className="mt-3 text-sm text-slate-500">{status}</p>
          </section>

          <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
            <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">Saved Maps</h3>
            <div className="space-y-2">
              {maps.length === 0 ? (
                <p className="text-sm text-slate-500">No saved maps.</p>
              ) : (
                maps.map((item) => (
                  <button
                    key={item.id ?? item.name}
                    type="button"
                    className="w-full rounded-md border border-slate-200 bg-white p-3 text-left hover:bg-slate-50"
                    onClick={async () => {
                      if (item.id == null) return;
                      const response = await fetchWithAuth(`/api/map/${item.id}`);
                      setCurrentMap((await response.json()) as MapPayload);
                    }}
                  >
                    <div className="flex items-center gap-2 font-semibold text-slate-800">
                      <Navigation className="size-4 text-blue-600" />
                      <span className="truncate">{item.name ?? `Map #${item.id}`}</span>
                    </div>
                    <p className="mt-1 text-xs text-slate-400">{item.width}x{item.height}</p>
                  </button>
                ))
              )}
            </div>
          </section>
        </aside>
      </div>
    </div>
  );
}
