"use client";

import { useCallback, useEffect, useRef, useState, type MutableRefObject, type RefObject } from "react";
import { useWebSocket } from "@/hooks/useWebSocket";
import { fetchWithAuth } from "@/lib/api";
import { createMapTexture } from "@/lib/gazebo-map-texture";
import { normalizeRobotTelemetry, type RobotUiTelemetry } from "@/lib/robot-telemetry";
import { loadRobotModel } from "@/lib/urdf-robot-model";
import type { GazeboViewMode, MapPayload, PathsPayload } from "@/types/robot-runtime";
import * as THREE from "three";

interface RenderState {
  globalKey: string;
  localKey: string;
  humansKey: string;
}

export interface GazeboRuntime {
  mountRef: RefObject<HTMLDivElement | null>;
  maps: MapPayload[];
  currentMap: MapPayload | null;
  telemetry: RobotUiTelemetry | null;
  paths: PathsPayload;
  status: string;
  viewMode: GazeboViewMode;
  setViewMode: (value: GazeboViewMode) => void;
  loadMaps: () => Promise<void>;
  selectSavedMap: (map: MapPayload) => Promise<void>;
}

export function useGazeboRuntime(): GazeboRuntime {
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
  const renderStateRef = useRef<RenderState>({ globalKey: "", localKey: "", humansKey: "" });
  const telemetryRef = useRef<RobotUiTelemetry | null>(null);
  const pathsRef = useRef<PathsPayload>({});
  const currentMapRef = useRef<MapPayload | null>(null);

  const [maps, setMaps] = useState<MapPayload[]>([]);
  const [currentMap, setCurrentMap] = useState<MapPayload | null>(null);
  const [telemetry, setTelemetry] = useState<RobotUiTelemetry | null>(null);
  const [paths, setPaths] = useState<PathsPayload>({});
  const [status, setStatus] = useState("Waiting for realtime map and robot pose.");
  const [viewMode, setViewMode] = useState<GazeboViewMode>("gazebo");

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

  const selectSavedMap = useCallback(async (map: MapPayload) => {
    if (map.id == null) return;
    const response = await fetchWithAuth(`/api/map/${map.id}`);
    setCurrentMap((await response.json()) as MapPayload);
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

    const scene = createScene();
    const camera = new THREE.PerspectiveCamera(36, 1, 0.1, 200);
    const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: false });
    renderer.shadowMap.enabled = true;
    renderer.shadowMap.type = THREE.PCFSoftShadowMap;
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));

    sceneRef.current = scene;
    cameraRef.current = camera;
    rendererRef.current = renderer;
    mount.appendChild(renderer.domElement);

    addSceneActors(scene, humansRef, pathRefs);

    let disposed = false;
    void loadRobotModel()
      .then((robot) => {
        if (disposed) return;
        robotRef.current = robot;
        robot.position.z = 0.035;
        scene.add(robot);
        setStatus("Loaded URDF robot model from local meshes.");
      })
      .catch(() => setStatus("Cannot load URDF robot model."));

    const resize = () => resizeRenderer(mountRef, rendererRef, cameraRef);
    const renderFrame = () => {
      renderGazeboFrame(viewMode, cameraRef, currentMapRef, telemetryRef, pathsRef, robotRef, pathRefs, humansRef, renderStateRef);
      rendererRef.current?.render(sceneRef.current!, cameraRef.current!);
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
      disposeTextureRef(textureRef);
      scene.clear();
    };
  }, [viewMode]);

  useEffect(() => {
    updateMapFloor(sceneRef, floorRef, textureRef, currentMap);
  }, [currentMap]);

  return {
    mountRef,
    maps,
    currentMap,
    telemetry,
    paths,
    status,
    viewMode,
    setViewMode,
    loadMaps,
    selectSavedMap,
  };
}

function createScene() {
  const scene = new THREE.Scene();
  scene.background = new THREE.Color(0xdce6ef);
  scene.fog = new THREE.Fog(0xdce6ef, 6, 40);

  const hemi = new THREE.HemisphereLight(0xf8fbff, 0x7c8796, 1.8);
  scene.add(hemi);

  const sun = new THREE.DirectionalLight(0xffffff, 2.6);
  sun.position.set(-6, -10, 14);
  sun.castShadow = true;
  sun.shadow.mapSize.width = 2048;
  sun.shadow.mapSize.height = 2048;
  scene.add(sun);

  return scene;
}

function addSceneActors(
  scene: THREE.Scene,
  humansRef: MutableRefObject<THREE.Group | null>,
  pathRefs: MutableRefObject<{ global: THREE.Line | null; local: THREE.Line | null }>,
) {
  const humansGroup = new THREE.Group();
  humansRef.current = humansGroup;
  scene.add(humansGroup);

  const globalLine = new THREE.Line(new THREE.BufferGeometry(), new THREE.LineBasicMaterial({ color: 0x2563eb, linewidth: 3 }));
  const localLine = new THREE.Line(new THREE.BufferGeometry(), new THREE.LineBasicMaterial({ color: 0xf97316, linewidth: 2 }));
  pathRefs.current = { global: globalLine, local: localLine };
  scene.add(globalLine);
  scene.add(localLine);
}

function resizeRenderer(
  mountRef: RefObject<HTMLDivElement | null>,
  rendererRef: RefObject<THREE.WebGLRenderer | null>,
  cameraRef: RefObject<THREE.PerspectiveCamera | null>,
) {
  if (!mountRef.current || !rendererRef.current || !cameraRef.current) return;
  const width = mountRef.current.clientWidth;
  const height = Math.max(480, mountRef.current.clientHeight);
  rendererRef.current.setSize(width, height, false);
  cameraRef.current.aspect = width / height;
  cameraRef.current.updateProjectionMatrix();
}

function renderGazeboFrame(
  viewMode: GazeboViewMode,
  cameraRef: RefObject<THREE.PerspectiveCamera | null>,
  currentMapRef: RefObject<MapPayload | null>,
  telemetryRef: RefObject<RobotUiTelemetry | null>,
  pathsRef: RefObject<PathsPayload>,
  robotRef: RefObject<THREE.Group | null>,
  pathRefs: RefObject<{ global: THREE.Line | null; local: THREE.Line | null }>,
  humansRef: RefObject<THREE.Group | null>,
  renderStateRef: RefObject<RenderState>,
) {
  const map = currentMapRef.current;
  const camera = cameraRef.current;
  if (!map || !camera) return;

  updateCamera(viewMode, camera, map);
  updateRobot(robotRef.current, telemetryRef.current);
  updatePathLine(pathRefs.current?.global, pathsRef.current.global_plan ?? [], "globalKey", renderStateRef.current);
  updatePathLine(pathRefs.current?.local, pathsRef.current.local_plan ?? [], "localKey", renderStateRef.current);
  updateHumans(humansRef.current, telemetryRef.current, renderStateRef.current);

}

function updateCamera(viewMode: GazeboViewMode, camera: THREE.PerspectiveCamera, map: MapPayload) {
  const centerX = map.origin_x + (map.width * map.resolution) / 2;
  const centerY = map.origin_y + (map.height * map.resolution) / 2;
  const span = Math.max(map.width * map.resolution, map.height * map.resolution);

  if (viewMode === "gazebo") {
    camera.position.set(centerX - span * 0.42, centerY - span * 0.62, Math.max(2.4, span * 0.92));
    camera.lookAt(centerX, centerY, 0);
    return;
  }

  camera.position.set(centerX, centerY, Math.max(3.5, span * 1.35));
  camera.up.set(0, 1, 0);
  camera.lookAt(centerX, centerY, 0);
}

function updateRobot(robot: THREE.Group | null, telemetry: RobotUiTelemetry | null) {
  if (!robot || telemetry?.map_pose.x == null || telemetry.map_pose.y == null) return;
  robot.position.set(telemetry.map_pose.x, telemetry.map_pose.y, 0.035);
  robot.rotation.set(0, 0, telemetry.map_pose.yaw ?? 0);
}

function updatePathLine(
  line: THREE.Line | null | undefined,
  points2d: Array<{ x: number; y: number }>,
  stateKey: "globalKey" | "localKey",
  renderState: RenderState | null,
) {
  if (!line || !renderState) return;
  const key = points2d.map((point) => `${point.x.toFixed(3)},${point.y.toFixed(3)}`).join("|");
  if (key === renderState[stateKey]) return;
  line.geometry.dispose();
  line.geometry = new THREE.BufferGeometry().setFromPoints(points2d.map((point) => new THREE.Vector3(point.x, point.y, stateKey === "globalKey" ? 0.03 : 0.05)));
  renderState[stateKey] = key;
}

function updateHumans(group: THREE.Group | null, telemetry: RobotUiTelemetry | null, renderState: RenderState | null) {
  const humans = telemetry?.humans ?? [];
  const key = humans.map((human) => `${human.id ?? "x"}:${human.x ?? 0}:${human.y ?? 0}:${human.confidence ?? 0}`).join("|");
  if (!group || !renderState || key === renderState.humansKey) return;

  while (group.children.length > 0) {
    const child = group.children[0] as THREE.Mesh;
    group.remove(child);
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
    group.add(marker);
  }
  renderState.humansKey = key;
}

function updateMapFloor(
  sceneRef: RefObject<THREE.Scene | null>,
  floorRef: RefObject<THREE.Mesh | null>,
  textureRef: RefObject<THREE.Texture | null>,
  map: MapPayload | null,
) {
  const scene = sceneRef.current;
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
}

function disposeTextureRef(textureRef: RefObject<THREE.Texture | null>) {
  textureRef.current?.dispose();
  textureRef.current = null;
}
