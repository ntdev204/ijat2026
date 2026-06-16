"use client";

import { Button } from "@/components/ui/button";
import { useWebSocket } from "@/hooks/useWebSocket";
import { fetchWithAuth } from "@/lib/api";
import { normalizeRobotTelemetry, type RobotUiTelemetry } from "@/lib/robot-telemetry";
import { loadRobotModel } from "@/lib/urdf-robot-model";
import * as THREE from "three";
import { Bot, Camera, Map as MapIcon, RefreshCw, Users, Waypoints } from "lucide-react";
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
  yaml_path?: string | null;
}

interface PathsPayload {
  global_plan?: Array<{ x: number; y: number }>;
  local_plan?: Array<{ x: number; y: number }>;
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
  if (value === 255) return 210;
  if (value >= 100) return 36;
  if (value === 0) return 245;
  return Math.max(48, 245 - Math.round(value * 2));
}

function createMapTexture(map: MapPayload) {
  const canvas = document.createElement("canvas");
  canvas.width = map.width;
  canvas.height = map.height;
  const context = canvas.getContext("2d");
  if (!context) throw new Error("Cannot create map canvas");

  const image = context.createImageData(map.width, map.height);
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
  context.putImageData(image, 0, 0);

  const texture = new THREE.CanvasTexture(canvas);
  texture.colorSpace = THREE.SRGBColorSpace;
  texture.anisotropy = 8;
  return texture;
}

export default function GazeboPage() {
  const mountRef = useRef<HTMLDivElement | null>(null);
  const sceneRef = useRef<THREE.Scene | null>(null);
  const cameraRef = useRef<THREE.PerspectiveCamera | null>(null);
  const rendererRef = useRef<THREE.WebGLRenderer | null>(null);
  const animationRef = useRef<number | null>(null);
  const robotRef = useRef<THREE.Group | null>(null);
  const floorRef = useRef<THREE.Mesh | null>(null);
  const textureRef = useRef<THREE.Texture | null>(null);
  const pathRefs = useRef<{ global: THREE.Line | null; local: THREE.Line | null }>({ global: null, local: null });
  const humansRef = useRef<THREE.Group | null>(null);
  const renderStateRef = useRef<{ globalKey: string; localKey: string; humansKey: string }>({
    globalKey: "",
    localKey: "",
    humansKey: "",
  });
  const telemetryRef = useRef<RobotUiTelemetry | null>(null);
  const pathsRef = useRef<PathsPayload>({});
  const currentMapRef = useRef<MapPayload | null>(null);

  const [maps, setMaps] = useState<MapPayload[]>([]);
  const [currentMap, setCurrentMap] = useState<MapPayload | null>(null);
  const [telemetry, setTelemetry] = useState<RobotUiTelemetry | null>(null);
  const [paths, setPaths] = useState<PathsPayload>({});
  const [status, setStatus] = useState("Waiting for realtime map and robot pose.");
  const [viewMode, setViewMode] = useState<"gazebo" | "top">("gazebo");

  useEffect(() => {
    telemetryRef.current = telemetry;
  }, [telemetry]);

  useEffect(() => {
    pathsRef.current = paths;
  }, [paths]);

  useEffect(() => {
    currentMapRef.current = currentMap;
  }, [currentMap]);

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
        setCurrentMap(JSON.parse(String(event.data)) as MapPayload);
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
      if (!currentMapRef.current && savedMaps[0]?.id != null) {
        const mapResponse = await fetchWithAuth(`/api/map/${savedMaps[0].id}`);
        setCurrentMap((await mapResponse.json()) as MapPayload);
      }
    } catch {
      setMaps([]);
    }
  }, []);

  useEffect(() => {
    const timer = window.setTimeout(() => {
      void loadMaps();
    }, 0);
    return () => window.clearTimeout(timer);
  }, [loadMaps]);

  useEffect(() => {
    const mount = mountRef.current;
    if (!mount) return;

    const scene = new THREE.Scene();
    scene.background = new THREE.Color(0xdce6ef);
    scene.fog = new THREE.Fog(0xdce6ef, 6, 40);
    sceneRef.current = scene;

    const camera = new THREE.PerspectiveCamera(36, 1, 0.1, 200);
    cameraRef.current = camera;

    const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: false });
    renderer.shadowMap.enabled = true;
    renderer.shadowMap.type = THREE.PCFSoftShadowMap;
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    rendererRef.current = renderer;
    mount.appendChild(renderer.domElement);

    const hemi = new THREE.HemisphereLight(0xf8fbff, 0x7c8796, 1.8);
    scene.add(hemi);

    const sun = new THREE.DirectionalLight(0xffffff, 2.6);
    sun.position.set(-6, -10, 14);
    sun.castShadow = true;
    sun.shadow.mapSize.width = 2048;
    sun.shadow.mapSize.height = 2048;
    scene.add(sun);

    const humansGroup = new THREE.Group();
    humansRef.current = humansGroup;
    scene.add(humansGroup);

    const globalLine = new THREE.Line(
      new THREE.BufferGeometry(),
      new THREE.LineBasicMaterial({ color: 0x2563eb, linewidth: 3 }),
    );
    const localLine = new THREE.Line(
      new THREE.BufferGeometry(),
      new THREE.LineBasicMaterial({ color: 0xf97316, linewidth: 2 }),
    );
    pathRefs.current = { global: globalLine, local: localLine };
    scene.add(globalLine);
    scene.add(localLine);

    let disposed = false;
    void loadRobotModel()
      .then((robot) => {
        if (disposed) return;
        robotRef.current = robot;
        robot.position.z = 0.035;
        scene.add(robot);
        setStatus("Loaded URDF robot model from local meshes.");
      })
      .catch(() => {
        setStatus("Cannot load URDF robot model.");
      });

    const resize = () => {
      if (!mountRef.current || !rendererRef.current || !cameraRef.current) return;
      const width = mountRef.current.clientWidth;
      const height = Math.max(480, mountRef.current.clientHeight);
      rendererRef.current.setSize(width, height, false);
      cameraRef.current.aspect = width / height;
      cameraRef.current.updateProjectionMatrix();
    };

    const renderFrame = () => {
      const map = currentMapRef.current;
      const robot = robotRef.current;
      const cameraNode = cameraRef.current;
      const rendererNode = rendererRef.current;
      const sceneNode = sceneRef.current;
      if (map && cameraNode && rendererNode && sceneNode) {
        const centerX = map.origin_x + (map.width * map.resolution) / 2;
        const centerY = map.origin_y + (map.height * map.resolution) / 2;
        const span = Math.max(map.width * map.resolution, map.height * map.resolution);

        if (viewMode === "gazebo") {
          cameraNode.position.set(centerX - span * 0.42, centerY - span * 0.62, Math.max(2.4, span * 0.92));
          cameraNode.lookAt(centerX, centerY, 0);
        } else {
          cameraNode.position.set(centerX, centerY, Math.max(3.5, span * 1.35));
          cameraNode.up.set(0, 1, 0);
          cameraNode.lookAt(centerX, centerY, 0);
        }

        const telemetryValue = telemetryRef.current;
        if (robot && telemetryValue?.map_pose.x != null && telemetryValue.map_pose.y != null) {
          robot.position.set(telemetryValue.map_pose.x, telemetryValue.map_pose.y, 0.035);
          robot.rotation.set(0, 0, telemetryValue.map_pose.yaw ?? 0);
        }

        const globalPoints = pathsRef.current.global_plan ?? [];
        const localPoints = pathsRef.current.local_plan ?? [];
        const globalKey = globalPoints.map((point) => `${point.x.toFixed(3)},${point.y.toFixed(3)}`).join("|");
        const localKey = localPoints.map((point) => `${point.x.toFixed(3)},${point.y.toFixed(3)}`).join("|");

        if (globalKey !== renderStateRef.current.globalKey) {
          const line = pathRefs.current.global;
          const points = globalPoints.map((point) => new THREE.Vector3(point.x, point.y, 0.03));
          line?.geometry.dispose();
          line!.geometry = new THREE.BufferGeometry().setFromPoints(points);
          renderStateRef.current.globalKey = globalKey;
        }
        if (localKey !== renderStateRef.current.localKey) {
          const line = pathRefs.current.local;
          const points = localPoints.map((point) => new THREE.Vector3(point.x, point.y, 0.05));
          line?.geometry.dispose();
          line!.geometry = new THREE.BufferGeometry().setFromPoints(points);
          renderStateRef.current.localKey = localKey;
        }

        const humans = telemetryValue?.humans ?? [];
        const humansKey = humans
          .map((human) => `${human.id ?? "x"}:${human.x ?? 0}:${human.y ?? 0}:${human.confidence ?? 0}`)
          .join("|");
        if (humansKey !== renderStateRef.current.humansKey) {
          const humansGroup = humansRef.current;
          while (humansGroup && humansGroup.children.length > 0) {
            const child = humansGroup.children[0] as THREE.Mesh;
            humansGroup.remove(child);
            child.geometry.dispose();
            if (Array.isArray(child.material)) child.material.forEach((item) => item.dispose());
            else child.material.dispose();
          }
          for (const human of humans) {
            if (typeof human.x !== "number" || typeof human.y !== "number") continue;
            const marker = new THREE.Mesh(
              new THREE.CapsuleGeometry(0.06, 0.24, 6, 10),
              new THREE.MeshStandardMaterial({ color: 0xdc2626, roughness: 0.55 }),
            );
            marker.castShadow = true;
            marker.position.set(human.x, human.y, 0.18);
            humansGroup?.add(marker);
          }
          renderStateRef.current.humansKey = humansKey;
        }
      }

      rendererNode?.render(sceneNode!, cameraNode!);
      animationRef.current = requestAnimationFrame(renderFrame);
    };

    resize();
    window.addEventListener("resize", resize);
    renderFrame();

    return () => {
      disposed = true;
      window.removeEventListener("resize", resize);
      if (animationRef.current != null) cancelAnimationFrame(animationRef.current);
      renderer.dispose();
      mount.removeChild(renderer.domElement);
      textureRef.current?.dispose();
      scene.clear();
    };
  }, [viewMode]);

  useEffect(() => {
    const scene = sceneRef.current;
    const map = currentMap;
    if (!scene || !map) return;

    textureRef.current?.dispose();
    if (floorRef.current) {
      scene.remove(floorRef.current);
      floorRef.current.geometry.dispose();
      if (Array.isArray(floorRef.current.material)) floorRef.current.material.forEach((item) => item.dispose());
      else floorRef.current.material.dispose();
    }

    const widthMeters = map.width * map.resolution;
    const heightMeters = map.height * map.resolution;
    const centerX = map.origin_x + widthMeters / 2;
    const centerY = map.origin_y + heightMeters / 2;

    const mapTexture = createMapTexture(map);
    mapTexture.flipY = false;
    textureRef.current = mapTexture;

    const floor = new THREE.Mesh(
      new THREE.PlaneGeometry(widthMeters, heightMeters),
      new THREE.MeshStandardMaterial({ map: mapTexture, roughness: 0.95, metalness: 0.02 }),
    );
    floor.position.set(centerX, centerY, 0);
    floor.receiveShadow = true;
    scene.add(floor);
    floorRef.current = floor;
  }, [currentMap]);

  return (
    <div className="space-y-5">
      <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
        <div>
          <h2 className="text-2xl font-bold text-slate-800 tracking-tight">Gazebo View</h2>
          <p className="text-sm text-slate-500">3D URDF robot over the scanned lidar map for paper-ready screenshots.</p>
        </div>
        <div className="flex gap-2">
          <Button variant={viewMode === "gazebo" ? "default" : "outline"} className="gap-2" onClick={() => setViewMode("gazebo")}>
            <Camera className="size-4" />
            Gazebo
          </Button>
          <Button variant={viewMode === "top" ? "default" : "outline"} className="gap-2" onClick={() => setViewMode("top")}>
            <MapIcon className="size-4" />
            Top
          </Button>
          <Button variant="outline" className="gap-2" onClick={() => void loadMaps()}>
            <RefreshCw className="size-4" />
            Refresh
          </Button>
        </div>
      </div>

      <div className="grid grid-cols-1 gap-5 xl:grid-cols-[minmax(0,1fr)_320px]">
        <section className="overflow-hidden rounded-lg border border-slate-200 bg-white shadow-sm">
          <div className="flex items-center gap-2 border-b border-slate-200 px-4 py-3 text-sm text-slate-600">
            <MapIcon className="size-4 text-blue-600" />
            <span className="font-medium text-slate-800">Gazebo-like Scene</span>
          </div>
          <div ref={mountRef} className="h-[720px] w-full bg-slate-100" />
        </section>

        <aside className="space-y-4">
          <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
            <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">Robot</h3>
            <div className="space-y-2 text-sm text-slate-600">
              <div className="flex items-center gap-2">
                <Bot className="size-4 text-emerald-600" />
                <span>URDF: `mini_mec_robot.urdf`</span>
              </div>
              <div className="flex items-center gap-2">
                <MapIcon className="size-4 text-blue-600" />
                <span>{currentMap ? `${currentMap.width}x${currentMap.height} @ ${currentMap.resolution}m` : "No map loaded"}</span>
              </div>
            </div>
          </section>

          <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
            <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">Runtime</h3>
            <div className="space-y-2 text-sm text-slate-600">
              <div className="flex items-center gap-2">
                <Waypoints className="size-4 text-blue-600" />
                <span>Global plan: {paths.global_plan?.length ?? 0}</span>
              </div>
              <div className="flex items-center gap-2">
                <Waypoints className="size-4 text-amber-600" />
                <span>Local plan: {paths.local_plan?.length ?? 0}</span>
              </div>
              <div className="flex items-center gap-2">
                <Users className="size-4 text-rose-600" />
                <span>Humans: {telemetry?.humans.length ?? 0}</span>
              </div>
            </div>
          </section>

          <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
            <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">Saved Maps</h3>
            <div className="space-y-2">
              {maps.map((item) => (
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
                  <div className="font-semibold text-slate-800">{item.name ?? `Map #${item.id}`}</div>
                  <div className="mt-1 text-xs text-slate-400">{item.width}x{item.height}</div>
                  <div className="mt-1 truncate text-xs text-slate-400">{item.yaml_path ?? "-"}</div>
                </button>
              ))}
            </div>
          </section>

          <section className="rounded-lg border border-slate-200 bg-white p-4 text-sm text-slate-600 shadow-sm">
            {status}
          </section>
        </aside>
      </div>
    </div>
  );
}
