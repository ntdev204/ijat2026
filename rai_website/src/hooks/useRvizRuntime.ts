"use client";

import { useCallback, useEffect, useRef, useState, type MutableRefObject, type RefObject } from "react";
import { OrbitControls } from "three/examples/jsm/controls/OrbitControls.js";
import { useWebSocket } from "@/hooks/useWebSocket";
import { fetchWithAuth } from "@/lib/api";
import { useOperationMode } from "@/contexts/OperationModeContext";
import { createRvizMapTexture } from "@/lib/rviz-map-texture";
import { normalizeRobotTelemetry, type RobotUiTelemetry } from "@/lib/robot-telemetry";
import { loadRobotModel } from "@/lib/urdf-robot-model";
import type { MapPayload, PathsPayload, RobotModelOption, RvizTopics, RvizViewMode } from "@/types/robot-runtime";
import * as THREE from "three";

interface RenderState {
  globalKey: string;
  localKey: string;
  humansKey: string;
  perspectiveKey: string;
}

interface SceneRefs {
  floorRef: MutableRefObject<THREE.Mesh | null>;
  textureRef: MutableRefObject<THREE.Texture | null>;
  pathRefs: MutableRefObject<{ global: THREE.Line | null; local: THREE.Line | null }>;
  humansRef: MutableRefObject<THREE.Group | null>;
  robotAxisRef: MutableRefObject<THREE.AxesHelper | null>;
  footprintRef: MutableRefObject<THREE.LineLoop | null>;
}

export interface RvizRuntime {
  mountRef: RefObject<HTMLDivElement | null>;
  maps: MapPayload[];
  currentMap: MapPayload | null;
  telemetry: RobotUiTelemetry | null;
  paths: PathsPayload;
  robotModels: RobotModelOption[];
  selectedRobotModelId: string;
  topics: RvizTopics;
  topicDraft: RvizTopics;
  status: string;
  viewMode: RvizViewMode;
  setViewMode: (value: RvizViewMode) => void;
  selectRobotModel: (modelId: string) => void;
  setTopicDraft: (value: RvizTopics) => void;
  applyTopics: () => Promise<void>;
  loadMaps: () => Promise<void>;
  selectSavedMap: (map: MapPayload) => Promise<void>;
}

export function useRvizRuntime(): RvizRuntime {
  const { operationMode } = useOperationMode();
  const mountRef = useRef<HTMLDivElement | null>(null);
  const sceneRef = useRef<THREE.Scene | null>(null);
  const cameraRef = useRef<THREE.PerspectiveCamera | null>(null);
  const rendererRef = useRef<THREE.WebGLRenderer | null>(null);
  const controlsRef = useRef<OrbitControls | null>(null);
  const animationRef = useRef<number | null>(null);
  const robotRef = useRef<THREE.Group | null>(null);
  const floorRef = useRef<THREE.Mesh | null>(null);
  const textureRef = useRef<THREE.Texture | null>(null);
  const pathRefs = useRef<{ global: THREE.Line | null; local: THREE.Line | null }>({ global: null, local: null });
  const humansRef = useRef<THREE.Group | null>(null);
  const robotAxisRef = useRef<THREE.AxesHelper | null>(null);
  const footprintRef = useRef<THREE.LineLoop | null>(null);
  const renderStateRef = useRef<RenderState>({ globalKey: "", localKey: "", humansKey: "", perspectiveKey: "" });
  const telemetryRef = useRef<RobotUiTelemetry | null>(null);
  const pathsRef = useRef<PathsPayload>({});
  const currentMapRef = useRef<MapPayload | null>(null);
  const viewModeRef = useRef<RvizViewMode>("perspective");

  const [maps, setMaps] = useState<MapPayload[]>([]);
  const [currentMap, setCurrentMap] = useState<MapPayload | null>(null);
  const [telemetry, setTelemetry] = useState<RobotUiTelemetry | null>(null);
  const [paths, setPaths] = useState<PathsPayload>({});
  const [robotModels, setRobotModels] = useState<RobotModelOption[]>([]);
  const [selectedRobotModelId, setSelectedRobotModelId] = useState("mini_mec_robot");
  const [topics, setTopics] = useState<RvizTopics>({
    map_topic: "/map",
    global_path_topic: "/rai_navigation/global_path",
    local_path_topic: "/canmpc/predicted_trajectory",
  });
  const [topicDraft, setTopicDraft] = useState<RvizTopics>({
    map_topic: "/map",
    global_path_topic: "/rai_navigation/global_path",
    local_path_topic: "/canmpc/predicted_trajectory",
  });
  const [status, setStatus] = useState("Waiting for map, TF, and live robot pose.");
  const [viewMode, setViewMode] = useState<RvizViewMode>("perspective");

  useEffect(() => {
    telemetryRef.current = telemetry;
  }, [telemetry]);

  useEffect(() => {
    pathsRef.current = paths;
  }, [paths]);

  useEffect(() => {
    currentMapRef.current = currentMap;
  }, [currentMap]);

  useEffect(() => {
    viewModeRef.current = viewMode;
  }, [viewMode]);

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

  const loadRobotModels = useCallback(async () => {
    try {
      const response = await fetchWithAuth("/api/robot-models");
      const models = (await response.json()) as RobotModelOption[];
      setRobotModels(models);
      if (models.length > 0 && !models.some((model) => model.id === selectedRobotModelId)) {
        setSelectedRobotModelId(models[0].id);
      }
    } catch {
      setRobotModels([]);
    }
  }, [selectedRobotModelId]);

  const loadTopics = useCallback(async () => {
    try {
      const response = await fetchWithAuth("/api/rviz/topics");
      const payload = (await response.json()) as RvizTopics;
      setTopics(payload);
      setTopicDraft(payload);
    } catch {
      setTopics({
        map_topic: "/map",
        global_path_topic: "/rai_navigation/global_path",
        local_path_topic: "/canmpc/predicted_trajectory",
      });
    }
  }, []);

  const applyTopics = useCallback(async () => {
    if (operationMode === "sim") {
      setStatus("RViz topic changes are disabled in Simulation mode.");
      return;
    }
    try {
      const response = await fetchWithAuth("/api/rviz/topics", {
        method: "POST",
        body: JSON.stringify(topicDraft),
      });
      const payload = (await response.json()) as RvizTopics;
      setTopics(payload);
      setTopicDraft(payload);
      setStatus("RViz topic subscriptions updated.");
    } catch (error) {
      setStatus(error instanceof Error ? error.message : "Failed to update RViz topics.");
    }
  }, [operationMode, topicDraft]);

  useEffect(() => {
    const timer = window.setTimeout(() => {
      void loadMaps();
      void loadRobotModels();
      void loadTopics();
    }, 0);
    return () => window.clearTimeout(timer);
  }, [loadMaps, loadRobotModels, loadTopics]);

  useEffect(() => {
    const mount = mountRef.current;
    if (!mount) return;

    const scene = createScene();
    const camera = new THREE.PerspectiveCamera(42, 1, 0.05, 500);
    const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: false });
    renderer.shadowMap.enabled = true;
    renderer.shadowMap.type = THREE.PCFSoftShadowMap;
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));

    const controls = new OrbitControls(camera, renderer.domElement);
    controls.enableDamping = true;
    controls.dampingFactor = 0.08;
    controls.minPolarAngle = 0;
    controls.maxPolarAngle = Math.PI;
    controls.minDistance = 0.25;
    controls.maxDistance = 80;
    controls.screenSpacePanning = true;

    sceneRef.current = scene;
    cameraRef.current = camera;
    rendererRef.current = renderer;
    controlsRef.current = controls;
    mount.appendChild(renderer.domElement);

    addSceneActors(scene, { floorRef, textureRef, pathRefs, humansRef, robotAxisRef, footprintRef });

    const resize = () => resizeRenderer(mountRef, rendererRef, cameraRef);
    const renderFrame = () => {
      renderRvizFrame({
        viewMode: viewModeRef.current,
        camera,
        controls,
        map: currentMapRef.current,
        telemetry: telemetryRef.current,
        paths: pathsRef.current,
        robot: robotRef.current,
        pathLines: pathRefs.current,
        humansGroup: humansRef.current,
        robotAxis: robotAxisRef.current,
        footprint: footprintRef.current,
        renderState: renderStateRef.current,
      });
      controls.update();
      renderer.render(scene, camera);
      animationRef.current = requestAnimationFrame(renderFrame);
    };

    resize();
    window.addEventListener("resize", resize);
    renderFrame();

    return () => {
      window.removeEventListener("resize", resize);
      if (animationRef.current != null) cancelAnimationFrame(animationRef.current);
      controls.dispose();
      renderer.dispose();
      mount.removeChild(renderer.domElement);
      disposeRobot(robotRef.current);
      disposeTextureRef(textureRef);
      scene.clear();
    };
  }, []);

  useEffect(() => {
    const scene = sceneRef.current;
    if (!scene) return;

    let disposed = false;
    setStatus(`Loading ${selectedRobotModelId}.urdf...`);
    void loadRobotModel(selectedRobotModelId)
      .then((robot) => {
        if (disposed) {
          disposeRobot(robot);
          return;
        }
        if (robotRef.current) {
          scene.remove(robotRef.current);
          disposeRobot(robotRef.current);
        }
        robotRef.current = robot;
        scene.add(robot);
        updateFootprintFromRobot(footprintRef.current, robot);
        setStatus(`RViz2 viewer ready. ${selectedRobotModelId}.urdf loaded with base_link origin preserved.`);
      })
      .catch(() => setStatus(`Cannot load ${selectedRobotModelId}.urdf.`));

    return () => {
      disposed = true;
    };
  }, [selectedRobotModelId]);

  useEffect(() => {
    updateMapFloor(sceneRef, floorRef, textureRef, currentMap);
  }, [currentMap]);

  return {
    mountRef,
    maps,
    currentMap,
    telemetry,
    paths,
    robotModels,
    selectedRobotModelId,
    topics,
    topicDraft,
    status,
    viewMode,
    setViewMode,
    selectRobotModel: setSelectedRobotModelId,
    setTopicDraft,
    applyTopics,
    loadMaps,
    selectSavedMap,
  };
}

function createScene() {
  const scene = new THREE.Scene();
  scene.background = new THREE.Color(0xe8edf3);
  scene.fog = new THREE.Fog(0xe8edf3, 10, 65);

  const hemi = new THREE.HemisphereLight(0xfafcff, 0x7b8794, 1.7);
  scene.add(hemi);

  const sun = new THREE.DirectionalLight(0xffffff, 2.1);
  sun.position.set(-6, -8, 16);
  sun.castShadow = true;
  sun.shadow.mapSize.width = 2048;
  sun.shadow.mapSize.height = 2048;
  scene.add(sun);

  scene.add(new THREE.AxesHelper(0.6));
  return scene;
}

function addSceneActors(scene: THREE.Scene, refs: SceneRefs) {
  const grid = new THREE.GridHelper(30, 60, 0x64748b, 0xcbd5e1);
  grid.rotation.x = Math.PI / 2;
  grid.position.z = -0.001;
  scene.add(grid);

  const humansGroup = new THREE.Group();
  refs.humansRef.current = humansGroup;
  scene.add(humansGroup);

  const globalLine = new THREE.Line(new THREE.BufferGeometry(), new THREE.LineBasicMaterial({ color: 0x2563eb }));
  const localLine = new THREE.Line(new THREE.BufferGeometry(), new THREE.LineBasicMaterial({ color: 0xf97316 }));
  refs.pathRefs.current = { global: globalLine, local: localLine };
  scene.add(globalLine);
  scene.add(localLine);

  const robotAxis = new THREE.AxesHelper(0.22);
  refs.robotAxisRef.current = robotAxis;
  scene.add(robotAxis);

  const footprint = new THREE.LineLoop(
    new THREE.BufferGeometry(),
    new THREE.LineBasicMaterial({ color: 0x0f766e }),
  );
  refs.footprintRef.current = footprint;
  scene.add(footprint);
}

function updateFootprintFromRobot(footprint: THREE.LineLoop | null, robot: THREE.Group) {
  if (!footprint) return;
  const bounds = new THREE.Box3().setFromObject(robot);
  if (!Number.isFinite(bounds.min.x) || !Number.isFinite(bounds.max.x)) return;
  const points = [
    new THREE.Vector3(bounds.max.x, bounds.max.y, 0.01),
    new THREE.Vector3(bounds.max.x, bounds.min.y, 0.01),
    new THREE.Vector3(bounds.min.x, bounds.min.y, 0.01),
    new THREE.Vector3(bounds.min.x, bounds.max.y, 0.01),
  ];
  footprint.geometry.dispose();
  footprint.geometry = new THREE.BufferGeometry().setFromPoints(points);
}

function resizeRenderer(
  mountRef: RefObject<HTMLDivElement | null>,
  rendererRef: RefObject<THREE.WebGLRenderer | null>,
  cameraRef: RefObject<THREE.PerspectiveCamera | null>,
) {
  if (!mountRef.current || !rendererRef.current || !cameraRef.current) return;
  const width = mountRef.current.clientWidth;
  const height = Math.max(540, mountRef.current.clientHeight);
  rendererRef.current.setSize(width, height, false);
  cameraRef.current.aspect = width / height;
  cameraRef.current.updateProjectionMatrix();
}

function renderRvizFrame({
  viewMode,
  camera,
  controls,
  map,
  telemetry,
  paths,
  robot,
  pathLines,
  humansGroup,
  robotAxis,
  footprint,
  renderState,
}: {
  viewMode: RvizViewMode;
  camera: THREE.PerspectiveCamera;
  controls: OrbitControls;
  map: MapPayload | null;
  telemetry: RobotUiTelemetry | null;
  paths: PathsPayload;
  robot: THREE.Group | null;
  pathLines: { global: THREE.Line | null; local: THREE.Line | null };
  humansGroup: THREE.Group | null;
  robotAxis: THREE.AxesHelper | null;
  footprint: THREE.LineLoop | null;
  renderState: RenderState;
}) {
  if (!map) return;

  updateCamera(viewMode, camera, controls, map, telemetry, renderState);
  updateRobot(robot, robotAxis, footprint, telemetry);
  updatePathLine(pathLines.global, paths.global_plan ?? [], "globalKey", renderState, 0.03);
  updatePathLine(pathLines.local, paths.local_plan ?? [], "localKey", renderState, 0.05);
  updateHumans(humansGroup, telemetry, renderState);
}

function updateCamera(
  viewMode: RvizViewMode,
  camera: THREE.PerspectiveCamera,
  controls: OrbitControls,
  map: MapPayload,
  telemetry: RobotUiTelemetry | null,
  renderState: RenderState,
) {
  const centerX = map.origin_x + (map.width * map.resolution) / 2;
  const centerY = map.origin_y + (map.height * map.resolution) / 2;
  const span = Math.max(map.width * map.resolution, map.height * map.resolution);
  const targetX = telemetry?.map_pose.x ?? centerX;
  const targetY = telemetry?.map_pose.y ?? centerY;

  if (viewMode === "top") {
    controls.target.set(targetX, targetY, 0.05);
    camera.position.set(targetX, targetY, Math.max(3.5, span * 1.2));
    camera.up.set(0, 1, 0);
    camera.lookAt(targetX, targetY, 0);
    return;
  }

  if (viewMode === "follow") {
    controls.target.set(targetX, targetY, 0.05);
    const yaw = telemetry?.map_pose.yaw ?? 0;
    const followDistance = Math.max(1.0, span * 0.18);
    camera.position.set(
      targetX - Math.cos(yaw) * followDistance,
      targetY - Math.sin(yaw) * followDistance,
      Math.max(0.8, followDistance * 0.8),
    );
    camera.lookAt(targetX, targetY, 0.08);
    return;
  }

  const perspectiveKey = `${map.origin_x}:${map.origin_y}:${map.width}:${map.height}:${map.resolution}`;
  if (renderState.perspectiveKey !== perspectiveKey) {
    controls.target.set(targetX, targetY, 0.05);
    camera.up.set(0, 0, 1);
    camera.position.set(centerX - span * 0.42, centerY - span * 0.58, Math.max(2.2, span * 0.84));
    camera.lookAt(targetX, targetY, 0.04);
    renderState.perspectiveKey = perspectiveKey;
  }
}

function updateRobot(
  robot: THREE.Group | null,
  robotAxis: THREE.AxesHelper | null,
  footprint: THREE.LineLoop | null,
  telemetry: RobotUiTelemetry | null,
) {
  if (telemetry?.map_pose.x == null || telemetry.map_pose.y == null) return;

  const x = telemetry.map_pose.x;
  const y = telemetry.map_pose.y;
  const yaw = telemetry.map_pose.yaw ?? 0;

  if (robot) {
    robot.position.x = x;
    robot.position.y = y;
    robot.rotation.set(0, 0, yaw);
  }

  if (robotAxis) {
    robotAxis.position.set(x, y, 0.01);
    robotAxis.rotation.set(0, 0, yaw);
  }

  if (footprint) {
    footprint.position.set(x, y, 0);
    footprint.rotation.set(0, 0, yaw);
  }
}

function updatePathLine(
  line: THREE.Line | null,
  points2d: Array<{ x: number; y: number }>,
  stateKey: "globalKey" | "localKey",
  renderState: RenderState,
  z: number,
) {
  if (!line) return;
  const key = points2d.map((point) => `${point.x.toFixed(3)},${point.y.toFixed(3)}`).join("|");
  if (key === renderState[stateKey]) return;
  line.geometry.dispose();
  line.geometry = new THREE.BufferGeometry().setFromPoints(points2d.map((point) => new THREE.Vector3(point.x, point.y, z)));
  renderState[stateKey] = key;
}

function updateHumans(group: THREE.Group | null, telemetry: RobotUiTelemetry | null, renderState: RenderState) {
  const humans = telemetry?.humans ?? [];
  const key = humans.map((human) => `${human.id ?? "x"}:${human.x ?? 0}:${human.y ?? 0}:${human.confidence ?? 0}`).join("|");
  if (!group || key === renderState.humansKey) return;

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
      new THREE.CylinderGeometry(0.08, 0.1, 0.26, 18),
      new THREE.MeshStandardMaterial({ color: 0xdc2626, roughness: 0.45 }),
    );
    marker.castShadow = true;
    marker.position.set(human.x, human.y, 0.13);
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
  const mapTexture = createRvizMapTexture(map);
  mapTexture.flipY = false;
  textureRef.current = mapTexture;

  const floor = new THREE.Mesh(
    new THREE.PlaneGeometry(widthMeters, heightMeters),
    new THREE.MeshStandardMaterial({ map: mapTexture, roughness: 0.98, metalness: 0.01 }),
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

function disposeRobot(robot: THREE.Group | null) {
  if (!robot) return;
  robot.traverse((child) => {
    if (!(child instanceof THREE.Mesh)) return;
    child.geometry.dispose();
    if (Array.isArray(child.material)) child.material.forEach((item) => item.dispose());
    else child.material.dispose();
  });
}
