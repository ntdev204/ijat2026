import asyncio
import base64
import csv
import http.client
import json
import logging
import os
import signal
import shlex
import shutil
import socket
import subprocess
import time
import urllib.parse
from datetime import datetime
from pathlib import Path
from typing import Optional
from xml.sax.saxutils import escape

import uvicorn
import yaml
from fastapi import BackgroundTasks, Depends, FastAPI, HTTPException, Request, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse
from pydantic import BaseModel, Field
from sqlalchemy import desc, select
from sqlalchemy.ext.asyncio import AsyncSession

from rai_web_api.database import DatasetRun, DatasetScenario, RuntimeSetting, SavedMap, create_session, get_db, init_db
from rai_web_api.research_dataset_spec import (
    ABLATION_CONTROLLERS,
    BASELINE_CONTROLLERS,
    DATASET_GROUPS,
    REQUIRED_FIGURES,
    SCENARIO_SPECS,
    STATISTICAL_PLAN,
)

logger = logging.getLogger("rai_web_api")
logging.basicConfig(level=os.getenv("RAI_API_LOG_LEVEL", "INFO"))

HOME_DIR = Path.home()
DEFAULT_WORKSPACE_DIR = Path(os.getenv("RAI_WORKSPACE_ROOT", str(HOME_DIR / "ijat2026"))).expanduser()
DEFAULT_RAI_ROS2_DATA_DIR = DEFAULT_WORKSPACE_DIR / "rai_ros2" / "data"

DATASET_BASE_PATH = Path(
    os.getenv("RAI_DATASET_PATH", str(DEFAULT_WORKSPACE_DIR / "dataset"))
).expanduser()
DEFAULT_HOST = os.getenv("RAI_API_HOST", "0.0.0.0")
DEFAULT_PORT = int(os.getenv("RAI_API_PORT", "8080"))
WEB_API_ROLES = {"hub", "laptop"}
DEVICE_ROLE = os.getenv("RAI_DEVICE_ROLE", "hub").strip().lower()
DEVICE_LABEL = os.getenv("RAI_DEVICE_LABEL", DEVICE_ROLE or "unknown")
DEFAULT_LAN_HOST = os.getenv("RAI_LAN_HOST", os.getenv("RAI_SERVER_HOST", "localhost")).strip()

def _cors_origins() -> list[str]:
    configured = os.getenv("RAI_API_CORS", "").strip()
    if configured:
        origins = []
        for origin in configured.split(","):
            cleaned = origin.strip()
            if not cleaned:
                continue
            try:
                parsed = urllib.parse.urlparse(cleaned)
            except Exception:
                continue
            if parsed.scheme not in {"http", "https"} or not parsed.netloc:
                continue
            origins.append(f"{parsed.scheme}://{parsed.netloc}")
        return origins
    default_origins = [
        f"http://{DEFAULT_LAN_HOST}:3000",
        f"http://{DEFAULT_LAN_HOST}",
        "http://localhost:3000",
    ]
    return default_origins

app = FastAPI(title="Rai Robot Web API", version="1.0.0")
app.add_middleware(
    CORSMiddleware,
    allow_origins=_cors_origins(),
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

active_dataset_run_id: Optional[int] = None
simulation_process: Optional[subprocess.Popen] = None

DATASET_REQUIRED_TOPICS = [
    "/scan",
    "/scan_filtered",
    "/odom",
    "/imu/data_raw",
    "/tf",
    "/tf_static",
    "/cmd_vel",
    "/cmd_vel_web",
    "/canmpc/context",
    "/canmpc/humans",
    "/canmpc/adaptive_bounds",
    "/canmpc/predicted_trajectory",
    "/canmpc/local_reference_path",
    "/canmpc/solver_stats",
    "/rai_navigation/status",
    "/rai_navigation/global_path",
    "/rai_navigation/local_costmap",
]
DATASET_CAMERA_TOPICS = [
    "/camera/color/image_raw",
    "/camera/depth/image_rect_raw",
    "/camera/aligned_depth_to_color/image_raw",
    "/camera/camera_info",
]
RUN_INDEX_FIELDS = [
    "run_id",
    "scenario_id",
    "controller_id",
    "environment",
    "start_time",
    "end_time",
    "duration_sec",
    "bag_path",
    "success",
    "collision",
    "timeout",
    "intervention",
    "random_seed",
    "robot_start_x",
    "robot_start_y",
    "robot_start_theta",
    "goal_x",
    "goal_y",
    "goal_theta",
    "human_behavior",
    "notes",
]

SIMULATION_KILL_PATTERNS = (
    "ros2 launch rai_gazebo_worlds rai_simulation.launch.py",
    "rai_sim_gz_bridge",
    "rai_sim_spawn_robot",
    "ign gazebo",
    "gz sim",
)
simulation_runtime_config = {
    "running": False,
    "world_id": "",
    "world_path": "",
    "robot_model": "mini_mec_robot",
    "start_slam": True,
    "start_navigation": False,
    "last_command": None,
}
system_operation_mode = os.getenv("RAI_OPERATION_MODE", "real").strip().lower()
if system_operation_mode not in {"real", "sim", "hybrid"}:
    system_operation_mode = "real"
camera_depth_enabled = os.getenv("RAI_CAMERA_ENABLE_DEPTH", "false").strip().lower() == "true"

MAP_STORAGE_DIR = Path(
    os.getenv("RAI_MAP_STORAGE_DIR", str(DEFAULT_RAI_ROS2_DATA_DIR / "map"))
).expanduser()
WORLD_STORAGE_DIR = Path(
    os.getenv("RAI_WORLD_STORAGE_DIR", str(DEFAULT_RAI_ROS2_DATA_DIR / "worlds"))
).expanduser()
RUNTIME_SETTING_OPERATION_MODE = "system.operation_mode"
RUNTIME_SETTING_CAMERA_DEPTH = "system.camera.enable_depth"

ROLE_ALLOWED_ACTIONS = {
    "hub": {"teleop", "navigation", "slam", "dataset", "maps", "simulation", "system", "hardware", "lidar", "camera"},
    "laptop": {"teleop", "navigation", "slam", "dataset", "maps", "simulation", "system", "hardware", "lidar", "camera"},
    "pi": {"hardware", "lidar", "slam", "navigation", "dataset", "system"},
    "jetson": {"camera", "perception", "system"},
}
ALLOWED_ACTIONS = ROLE_ALLOWED_ACTIONS.get(DEVICE_ROLE, set())
PI_BRIDGE_URL = os.getenv("RAI_PI_BRIDGE_URL", "").strip().rstrip("/")
JETSON_BRIDGE_URL = os.getenv("RAI_JETSON_BRIDGE_URL", "").strip().rstrip("/")
BRIDGE_RESPONSE_LIMIT_BYTES = 1024 * 1024


class WebRtcOffer(BaseModel):
    sdp: str
    type: str


class VelocityCommand(BaseModel):
    linear_x: float = Field(default=0.0, ge=-1.5, le=1.5)
    linear_y: float = Field(default=0.0, ge=-1.5, le=1.5)
    angular_z: float = Field(default=0.0, ge=-3.0, le=3.0)


class NavGoalRequest(BaseModel):
    x: float
    y: float
    yaw: float = 0.0


class PoseRequest(BaseModel):
    x: float
    y: float
    yaw: float = 0.0


class InitialPoseRequest(PoseRequest):
    set_home: bool = True


class RoutePlanRequest(BaseModel):
    start: PoseRequest
    goal: PoseRequest
    start_tolerance: float = Field(default=0.25, ge=0.0, le=2.0)


class DatasetStartRequest(BaseModel):
    scenario_name: str = Field(default="S1_open_zone")
    controller_id: str = Field(default="CCA_NMPC")
    environment: str = Field(default="real", pattern="^(sim|real)$")
    run_index: Optional[int] = Field(default=None, ge=0)
    split: str = Field(default="unsplit")
    random_seed: Optional[int] = None
    robot_start_x: Optional[float] = None
    robot_start_y: Optional[float] = None
    robot_start_theta: Optional[float] = None
    goal_x: Optional[float] = None
    goal_y: Optional[float] = None
    goal_theta: Optional[float] = None
    human_behavior: str = ""
    intervention: bool = False
    record_camera: bool = False
    notes: str = ""


class DatasetCaptureRequest(BaseModel):
    tag: str = Field(default="corridor")
    class_name: str = Field(default="person")


class DatasetPipelineRequest(BaseModel):
    action: str = Field(default="all", pattern="^(validate|bag_to_csv|metrics|plots|tables|all)$")
    run_id: Optional[int] = Field(default=None, ge=1)
    bag_path: Optional[str] = None


class DatasetLaunchRequest(BaseModel):
    scenario_name: str = Field(default="S1_open_zone")
    controller_id: str = Field(default="CCA_NMPC")
    environment: str = Field(default="real", pattern="^(sim|real)$")
    split: str = Field(default="unsplit")
    run_id: str = Field(default="")
    auto_start: bool = True


class SaveMapRequest(BaseModel):
    name: str = Field(..., min_length=1)


class UpdateMapRequest(BaseModel):
    name: str = Field(..., min_length=1)


class RaiNavigationConfigRequest(BaseModel):
    local_planner: str = Field(default="CCA_NMPC")
    global_planner: str = Field(default="A_STAR")
    map_path: Optional[str] = None
    map_id: Optional[int] = None
    params_path: Optional[str] = None


class SimulationWorldCreateRequest(BaseModel):
    name: str = Field(default="virtual_lab", min_length=1, max_length=64)
    width: float = Field(default=8.0, ge=2.0, le=50.0)
    height: float = Field(default=6.0, ge=2.0, le=50.0)
    obstacle_count: int = Field(default=3, ge=0, le=24)
    obstacle_size: float = Field(default=0.6, ge=0.15, le=3.0)
    corridor: bool = False


class SimulationStartRequest(BaseModel):
    world_id: str = Field(default="s1_open_zone")
    robot_model: str = Field(default="mini_mec_robot")
    start_slam: bool = True
    start_navigation: bool = False
    headless: bool = False
    map_path: Optional[str] = None


class RvizTopicsRequest(BaseModel):
    map_topic: str = Field(default="/map", min_length=1)
    global_path_topic: str = Field(default="/rai_navigation/global_path", min_length=1)
    local_path_topic: str = Field(default="/canmpc/predicted_trajectory", min_length=1)


class SystemComponentCommandRequest(BaseModel):
    enable_depth: bool = False


class SystemOperationModeRequest(BaseModel):
    mode: str = Field(default="real", pattern="^(real|sim|hybrid)$")


def _rai_robot_urdf_share() -> Path:
    configured = os.getenv("RAI_ROBOT_URDF_DIR", "").strip()
    candidates = [
        Path(configured).expanduser() if configured else None,
        DEFAULT_WORKSPACE_DIR / "rai_ros2" / "src" / "rai_robot_urdf" / "rai_robot_urdf",
        DEFAULT_WORKSPACE_DIR / "rai_ros2" / "install" / "rai_robot_urdf" / "share" / "rai_robot_urdf",
        Path(__file__).resolve().parents[2].parent / "rai_robot_urdf" / "rai_robot_urdf",
    ]
    for candidate in candidates:
        if candidate and candidate.exists():
            return candidate.resolve()
    return candidates[-1].resolve()


def _safe_robot_model_path(relative_path: str) -> Path:
    root = _rai_robot_urdf_share()
    path = (root / relative_path).resolve()
    if root != path and root not in path.parents:
        raise HTTPException(status_code=400, detail="Invalid robot model path")
    if not path.exists() or not path.is_file():
        raise HTTPException(status_code=404, detail="Robot model asset not found")
    return path


def _robot_model_label(model_id: str) -> str:
    return model_id.replace("_", " ").replace("-", " ").title()


def _rai_gazebo_worlds_share() -> Path:
    configured = os.getenv("RAI_GAZEBO_WORLDS_DIR", "").strip()
    candidates = [
        Path(configured).expanduser() if configured else None,
        DEFAULT_WORKSPACE_DIR / "rai_ros2" / "src" / "rai_simulation" / "rai_gazebo_worlds",
        DEFAULT_WORKSPACE_DIR / "rai_ros2" / "install" / "rai_gazebo_worlds" / "share" / "rai_gazebo_worlds",
        Path(__file__).resolve().parents[2].parent / "rai_simulation" / "rai_gazebo_worlds",
    ]
    for candidate in candidates:
        if candidate and candidate.exists():
            return candidate.resolve()
    return candidates[-1].resolve()


def _world_id_from_path(path: Path) -> str:
    return path.stem


def _world_label(world_id: str) -> str:
    return world_id.replace("_", " ").replace("-", " ").title()


def _list_world_files() -> list[tuple[str, Path, str]]:
    worlds: list[tuple[str, Path, str]] = []
    package_worlds = _rai_gazebo_worlds_share() / "worlds"
    if package_worlds.exists():
        for path in sorted(package_worlds.glob("*.world")):
            worlds.append((_world_id_from_path(path), path.resolve(), "package"))
    if WORLD_STORAGE_DIR.exists():
        for path in sorted(WORLD_STORAGE_DIR.glob("*.world")):
            worlds.append((f"generated:{_world_id_from_path(path)}", path.resolve(), "generated"))
    return worlds


def _world_summary(world_id: str, path: Path, source: str) -> dict:
    return {
        "id": world_id,
        "label": _world_label(world_id.split(":", 1)[-1]),
        "source": source,
        "path": str(path),
        "created_at": datetime.fromtimestamp(path.stat().st_mtime).isoformat() if path.exists() else None,
    }


def _safe_world_path(world_id: str) -> Path:
    for candidate_id, path, _source in _list_world_files():
        if candidate_id == world_id or candidate_id.split(":", 1)[-1] == world_id:
            return path

    path = Path(world_id).expanduser().resolve()
    roots = [
        (_rai_gazebo_worlds_share() / "worlds").resolve(),
        WORLD_STORAGE_DIR.resolve(),
    ]
    if not any(root == path or root in path.parents for root in roots):
        raise HTTPException(status_code=400, detail="World path must be inside RAI world directories")
    if not path.exists() or not path.is_file() or path.suffix != ".world":
        raise HTTPException(status_code=404, detail="Simulation world not found")
    return path


def _validate_robot_model_id(model_id: str) -> str:
    if "/" in model_id or "\\" in model_id or not model_id.strip():
        raise HTTPException(status_code=400, detail="Invalid robot model id")
    path = _rai_robot_urdf_share() / "urdf" / f"{model_id}.urdf"
    if not path.exists():
        raise HTTPException(status_code=404, detail=f"Robot model not found: {model_id}")
    return model_id


def _model_xml(name: str, x: float, y: float, sx: float, sy: float, sz: float = 0.5) -> str:
    return f"""
    <model name="{escape(name)}">
      <static>true</static>
      <pose>{x:.3f} {y:.3f} {sz / 2.0:.3f} 0 0 0</pose>
      <link name="link">
        <collision name="collision">
          <geometry><box><size>{sx:.3f} {sy:.3f} {sz:.3f}</size></box></geometry>
        </collision>
        <visual name="visual">
          <geometry><box><size>{sx:.3f} {sy:.3f} {sz:.3f}</size></box></geometry>
          <material><ambient>0.55 0.60 0.65 1</ambient><diffuse>0.55 0.60 0.65 1</diffuse></material>
        </visual>
      </link>
    </model>"""


def _write_virtual_world(request: SimulationWorldCreateRequest) -> Path:
    WORLD_STORAGE_DIR.mkdir(parents=True, exist_ok=True)
    base_name = _safe_map_name(request.name)
    world_path = WORLD_STORAGE_DIR / f"{base_name}.world"
    suffix = 2
    while world_path.exists():
        world_path = WORLD_STORAGE_DIR / f"{base_name}_{suffix}.world"
        suffix += 1

    width = float(request.width)
    height = float(request.height)
    wall = 0.12
    obstacles = [
        _model_xml("wall_north", 0.0, height / 2.0, width, wall, 0.7),
        _model_xml("wall_south", 0.0, -height / 2.0, width, wall, 0.7),
        _model_xml("wall_east", width / 2.0, 0.0, wall, height, 0.7),
        _model_xml("wall_west", -width / 2.0, 0.0, wall, height, 0.7),
    ]

    if request.corridor:
        corridor_gap = max(1.2, min(width, height) * 0.25)
        obstacles.append(_model_xml("corridor_left", -corridor_gap, 0.0, 0.18, height * 0.72, 0.55))
        obstacles.append(_model_xml("corridor_right", corridor_gap, 0.0, 0.18, height * 0.72, 0.55))

    count = int(request.obstacle_count)
    if count > 0:
        cols = max(1, min(count, 4))
        rows = int((count + cols - 1) / cols)
        spacing_x = width / (cols + 1)
        spacing_y = height / (rows + 1)
        for index in range(count):
            col = index % cols
            row = index // cols
            x = -width / 2.0 + spacing_x * (col + 1)
            y = -height / 2.0 + spacing_y * (row + 1)
            if abs(x) < 0.6 and abs(y) < 0.6:
                y += min(1.0, spacing_y * 0.5)
            obstacles.append(_model_xml(f"box_{index + 1}", x, y, request.obstacle_size, request.obstacle_size, 0.55))

    world_xml = f"""<?xml version="1.0" ?>
<sdf version="1.7">
  <world name="{escape(base_name)}">
    <physics name="default_physics" type="ode">
      <max_step_size>0.004</max_step_size>
      <real_time_factor>1.0</real_time_factor>
    </physics>
    <plugin filename="ignition-gazebo-physics-system" name="ignition::gazebo::systems::Physics"/>
    <plugin filename="ignition-gazebo-scene-broadcaster-system" name="ignition::gazebo::systems::SceneBroadcaster"/>
    <plugin filename="ignition-gazebo-sensors-system" name="ignition::gazebo::systems::Sensors">
      <render_engine>ogre2</render_engine>
    </plugin>
    <light name="sun" type="directional">
      <pose>0 0 8 0 0 0</pose>
      <diffuse>0.9 0.9 0.9 1</diffuse>
      <specular>0.1 0.1 0.1 1</specular>
      <direction>-0.5 0.2 -1</direction>
    </light>
    <model name="ground_plane">
      <static>true</static>
      <link name="link">
        <collision name="collision">
          <geometry><box><size>{width:.3f} {height:.3f} 0.02</size></box></geometry>
        </collision>
        <visual name="visual">
          <geometry><box><size>{width:.3f} {height:.3f} 0.02</size></box></geometry>
          <material><ambient>0.82 0.84 0.86 1</ambient><diffuse>0.82 0.84 0.86 1</diffuse></material>
        </visual>
      </link>
    </model>
    {"".join(obstacles)}
  </world>
</sdf>
"""
    world_path.write_text(world_xml, encoding="utf-8")
    return world_path


def _simulation_launch_command(request: SimulationStartRequest, world_path: Path) -> str:
    robot_model = _validate_robot_model_id(request.robot_model)
    args = [
        "ros2",
        "launch",
        "rai_gazebo_worlds",
        "rai_simulation.launch.py",
        f"world:={world_path}",
        f"robot_model:={robot_model}",
        "use_sim_time:=true",
        f"start_slam:={str(request.start_slam).lower()}",
        f"start_navigation:={str(request.start_navigation).lower()}",
        f"headless:={str(request.headless).lower()}",
    ]
    if request.map_path:
        args.append(f"map:={Path(request.map_path).expanduser()}")
    return " ".join(shlex.quote(str(item)) for item in args)


def _ros_runtime_env() -> dict:
    env = os.environ.copy()
    cyclone_library = Path("/opt/ros/humble/lib/librmw_cyclonedds_cpp.so")
    if cyclone_library.exists():
        env["RMW_IMPLEMENTATION"] = "rmw_cyclonedds_cpp"
        env.pop("FASTDDS_BUILTIN_TRANSPORTS", None)
    else:
        env["RMW_IMPLEMENTATION"] = "rmw_fastrtps_cpp"
        env["FASTDDS_BUILTIN_TRANSPORTS"] = "UDPv4"
    env.setdefault("RCUTILS_LOGGING_BUFFERED_STREAM", "1")
    return env


def _system_runtime_payload(pi_online: bool = False, jetson_online: bool = False) -> dict:
    return {
        "device_role": DEVICE_ROLE,
        "device_label": DEVICE_LABEL,
        "allowed_actions": sorted(ALLOWED_ACTIONS),
        "api_host": DEFAULT_HOST,
        "api_port": DEFAULT_PORT,
        "operation_mode": system_operation_mode,
        "operation_mode_options": [
            {"id": "real", "label": "Thuc te", "description": "Robot that, sensor that, no simulation runtime."},
            {"id": "sim", "label": "Mo phong", "description": "Robot mo phong trong Gazebo/RViz, khong dung phan cung that."},
            {"id": "hybrid", "label": "Hybrid", "description": "Robot that ket hop visualization/simulation workspace tren Gazebo hoac RViz."},
        ],
        "runtime_bridges": {
            "pi": {
                "url": PI_BRIDGE_URL or None,
                "online": pi_online,
            },
            "jetson": {
                "url": JETSON_BRIDGE_URL or None,
                "online": jetson_online,
            },
        },
    }


async def _get_runtime_setting(key: str, default: str) -> str:
    session = create_session()
    try:
        setting = await session.get(RuntimeSetting, key)
        return setting.value if setting is not None else default
    finally:
        await session.close()


async def _set_runtime_setting(key: str, value: str) -> None:
    session = create_session()
    try:
        setting = await session.get(RuntimeSetting, key)
        if setting is None:
            setting = RuntimeSetting(key=key, value=value)
            session.add(setting)
        else:
            setting.value = value
        await session.commit()
    finally:
        await session.close()


async def _load_runtime_settings() -> None:
    global system_operation_mode, camera_depth_enabled
    persisted_mode = await _get_runtime_setting(RUNTIME_SETTING_OPERATION_MODE, system_operation_mode)
    if persisted_mode in {"real", "sim", "hybrid"}:
        system_operation_mode = persisted_mode
    persisted_camera_depth = await _get_runtime_setting(
        RUNTIME_SETTING_CAMERA_DEPTH,
        "true" if camera_depth_enabled else "false",
    )
    camera_depth_enabled = persisted_camera_depth.strip().lower() == "true"


def _require_action(action: str) -> None:
    if action in ALLOWED_ACTIONS:
        return
    raise HTTPException(
        status_code=403,
        detail=(
            f"Action '{action}' is not allowed on device role '{DEVICE_ROLE}'. "
            f"Allowed actions here: {', '.join(sorted(ALLOWED_ACTIONS)) or 'none'}."
        ),
    )


def _start_process(command: str, env: Optional[dict] = None) -> subprocess.Popen:
    kwargs = {}
    if hasattr(os, "setsid"):
        kwargs["preexec_fn"] = os.setsid
    kwargs["env"] = env or os.environ.copy()
    return subprocess.Popen(command, shell=True, **kwargs)


def _stop_process(process: Optional[subprocess.Popen]) -> dict:
    if process is None or process.poll() is not None:
        return {"status": "stopped", "message": "not running"}
    try:
        if hasattr(os, "killpg"):
            os.killpg(os.getpgid(process.pid), signal.SIGTERM)
        else:
            process.terminate()
        process.wait(timeout=5.0)
    except subprocess.TimeoutExpired:
        if hasattr(os, "killpg"):
            os.killpg(os.getpgid(process.pid), signal.SIGKILL)
        else:
            process.kill()
        process.wait(timeout=2.0)
    return {"status": "stopped", "pid": process.pid}


def _find_pids_by_patterns(patterns: tuple[str, ...]) -> set[int]:
    if os.name == "nt":
        return set()

    matched: set[int] = set()
    for pattern in patterns:
        result = subprocess.run(
            ["pgrep", "-f", pattern],
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode not in (0, 1):
            continue
        for line in result.stdout.splitlines():
            line = line.strip()
            if not line:
                continue
            try:
                pid = int(line)
            except ValueError:
                continue
            if pid != os.getpid():
                matched.add(pid)
    return matched


def _kill_matching_processes(patterns: tuple[str, ...]) -> list[int]:
    pids = _find_pids_by_patterns(patterns)
    if not pids:
        return []

    for pid in pids:
        try:
            os.kill(pid, signal.SIGTERM)
        except ProcessLookupError:
            pass

    deadline = time.time() + 3.0
    while time.time() < deadline:
        remaining = [pid for pid in pids if Path(f"/proc/{pid}").exists()]
        if not remaining:
            return sorted(pids)
        time.sleep(0.1)

    for pid in pids:
        if Path(f"/proc/{pid}").exists():
            try:
                os.kill(pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
    return sorted(pids)


def _stop_stack_process(process: Optional[subprocess.Popen], patterns: tuple[str, ...]) -> dict:
    result = _stop_process(process)
    killed_pids = _kill_matching_processes(patterns)
    if killed_pids:
        result["killed_pids"] = killed_pids
    return result


def _process_component_entry(
    component_id: str,
    label: str,
    host_device: str,
    action: str,
    running: bool,
    pid: Optional[int],
    launch_file: str,
    description: str,
    capabilities: Optional[dict] = None,
) -> dict:
    return {
        "id": component_id,
        "label": label,
        "host_device": host_device,
        "action": action,
        "allowed_here": action in ALLOWED_ACTIONS,
        "running": running,
        "pid": pid,
        "launch_file": launch_file,
        "description": description,
        "capabilities": capabilities or {},
    }


def _agent_base_url(device: str) -> str:
    key = device.strip().lower()
    if key == "pi":
        return PI_BRIDGE_URL
    if key == "jetson":
        return JETSON_BRIDGE_URL
    return ""


def _agent_request_sync(method: str, device: str, endpoint: str, payload: Optional[dict] = None) -> dict:
    base_url = _agent_base_url(device)
    if not base_url:
        raise HTTPException(status_code=503, detail=f"Runtime bridge URL for device '{device}' is not configured")

    parsed_base = urllib.parse.urlparse(base_url)
    if parsed_base.scheme not in {"http", "https"} or not parsed_base.netloc:
        raise HTTPException(status_code=503, detail=f"Runtime bridge URL for device '{device}' is not configured")

    # urllib.request.urlopen ignores `timeout` for the TCP connect phase, so an
    # unreachable peer can park the executor thread for ~tcp_syn_retries ×
    # ~75s on Linux. Use http.client directly with a real connect timeout.
    path = urllib.parse.urljoin(f"{base_url.rstrip('/')}/", endpoint.lstrip('/'))
    parsed_path = urllib.parse.urlparse(path)
    host = parsed_base.hostname or ""
    port = parsed_base.port or (443 if parsed_base.scheme == "https" else 80)
    request_path = parsed_path.path or "/"
    if parsed_path.query:
        request_path = f"{request_path}?{parsed_path.query}"

    body = None
    headers = {"Content-Type": "application/json", "X-RAI-Web-Api-Proxy": "1"}
    if payload is not None:
        body = json.dumps(payload).encode("utf-8")
        headers["Content-Length"] = str(len(body))

    connect_timeout = float(os.getenv("RAI_BRIDGE_CONNECT_TIMEOUT", "1.0"))
    read_timeout = float(os.getenv("RAI_BRIDGE_READ_TIMEOUT", "3.0"))

    connection_cls = http.client.HTTPSConnection if parsed_base.scheme == "https" else http.client.HTTPConnection
    conn = connection_cls(host, port, timeout=connect_timeout)
    try:
        try:
            conn.request(method.upper(), request_path, body=body, headers=headers)
            # getresponse() with timeout= caps the read phase. Without it, the
            # connect-timeout would protect us from an unreachable peer but a
            # peer that accepts the TCP and then stalls would still block.
            response = conn.getresponse(timeout=read_timeout)
            raw = response.read(BRIDGE_RESPONSE_LIMIT_BYTES + 1)
        except (http.client.HTTPException, ConnectionError, OSError, socket.timeout) as exc:
            logger.warning("Cannot reach runtime bridge %s %s: %s", device, endpoint, exc)
            raise HTTPException(status_code=503, detail=f"Cannot reach runtime bridge '{device}'") from exc

        if len(raw) > BRIDGE_RESPONSE_LIMIT_BYTES:
            raise HTTPException(status_code=502, detail="Runtime bridge response is too large")
        if response.status >= 400:
            logger.warning("Runtime bridge %s %s returned HTTP %s", device, endpoint, response.status)
            raise HTTPException(status_code=response.status, detail=f"Runtime bridge '{device}' rejected the request")
        return json.loads(raw.decode("utf-8") or "{}")
    finally:
        try:
            conn.close()
        except Exception:
            pass


def _ensure_web_api_role() -> None:
    if DEVICE_ROLE in WEB_API_ROLES:
        return
    raise RuntimeError(
        f"rai_web_api is hub-only and must not run with RAI_DEVICE_ROLE={DEVICE_ROLE!r}. "
        "Use rai_runtime_bridge on Pi, Jetson, or other ROS runtime devices."
    )


async def _agent_request(method: str, device: str, endpoint: str, payload: Optional[dict] = None) -> dict:
    return await asyncio.to_thread(_agent_request_sync, method, device, endpoint, payload)


async def _pi_agent_request(method: str, endpoint: str, payload: Optional[dict] = None) -> dict:
    return await _agent_request(method, "pi", endpoint, payload)


def _bridge_ws_url(device: str, endpoint: str) -> str:
    base_url = _agent_base_url(device)
    if not base_url:
        raise HTTPException(status_code=503, detail=f"Runtime bridge URL for device '{device}' is not configured")
    parsed = urllib.parse.urlparse(base_url)
    if parsed.scheme not in {"http", "https"} or not parsed.netloc:
        raise HTTPException(status_code=503, detail=f"Runtime bridge URL for device '{device}' is invalid")
    ws_scheme = "wss" if parsed.scheme == "https" else "ws"
    return urllib.parse.urlunparse((ws_scheme, parsed.netloc, "/" + endpoint.lstrip("/"), "", "", ""))


async def _relay_websocket(websocket: WebSocket, device: str, endpoint: str) -> None:
    try:
        import websockets
    except ImportError as exc:
        logger.error("websockets package is not available: %s", exc)
        try:
            await websocket.close(code=1011, reason="websockets package is not available")
        except Exception:
            pass
        return

    bridge_url = _bridge_ws_url(device, endpoint)
    try:
        await websocket.accept()
    except Exception as exc:
        logger.warning("Failed to accept websocket from client for relay to %s: %s", device, exc)
        return
    try:
        async with websockets.connect(bridge_url) as bridge_ws:
            async def client_to_bridge() -> None:
                while True:
                    message = await websocket.receive()
                    if message.get("type") == "websocket.disconnect":
                        await bridge_ws.close()
                        return
                    if message.get("text") is not None:
                        await bridge_ws.send(message["text"])
                    elif message.get("bytes") is not None:
                        await bridge_ws.send(message["bytes"])

            async def bridge_to_client() -> None:
                async for message in bridge_ws:
                    if isinstance(message, bytes):
                        await websocket.send_bytes(message)
                    else:
                        await websocket.send_text(message)

            done, pending = await asyncio.wait(
                {asyncio.create_task(client_to_bridge()), asyncio.create_task(bridge_to_client())},
                return_when=asyncio.FIRST_COMPLETED,
            )
            for task in pending:
                task.cancel()
            for task in done:
                task.result()
    except WebSocketDisconnect:
        return
    except Exception as exc:
        logger.warning("Runtime bridge websocket relay failed for %s %s: %s", device, endpoint, exc)
        try:
            await websocket.close(code=1011)
        except Exception:
            pass


async def _runtime_telemetry() -> dict:
    try:
        payload = await _agent_request("GET", "pi", "/api/telemetry/current")
        telemetry = payload.get("telemetry", {})
        if not isinstance(telemetry, dict):
            logger.warning("Unexpected telemetry payload type from pi bridge: %s", type(telemetry))
            return {}
        return telemetry
    except HTTPException:
        return {}
    except Exception as exc:
        logger.warning("Cannot fetch runtime telemetry from pi bridge: %s", exc)
        return {}


async def _proxy_system_component(device: str, route_component: str, action: str, payload: Optional[dict] = None) -> Optional[dict]:
    return await _agent_request("POST", device, f"/api/system/components/{route_component}/{action}", payload)


def _merge_peer_component(local_component: dict, peer_component: dict) -> dict:
    merged_capabilities = {
        **local_component.get("capabilities", {}),
        **peer_component.get("capabilities", {}),
    }
    return {
        **local_component,
        "host_device": peer_component.get("host_device", local_component.get("host_device")),
        "allowed_here": bool(peer_component.get("allowed_here", local_component.get("allowed_here", False))),
        "running": bool(peer_component.get("running", local_component.get("running", False))),
        "pid": peer_component.get("pid"),
        "capabilities": merged_capabilities,
    }


def _local_system_components() -> list[dict]:
    sim_running = simulation_process is not None and simulation_process.poll() is None
    simulation_pid = simulation_process.pid if sim_running and simulation_process is not None else None
    return [
        _process_component_entry(
            "simulation",
            "Simulation",
            "laptop",
            "simulation",
            sim_running,
            simulation_pid,
            "rai_simulation.launch.py",
            "Gazebo simulation runtime.",
        ),
    ]


def _can_proxy_to_peer(device: str) -> bool:
    return bool(_agent_base_url(device))


def _safe_map_name(name: str) -> str:
    cleaned = "".join(char if char.isalnum() or char in {"_", "-"} else "_" for char in name.strip())
    return cleaned.strip("_") or datetime.utcnow().strftime("RAI_%Y%m%d_%H%M%S")


def _decode_grid_data(grid_data: str) -> bytes:
    return base64.b64decode(grid_data.encode("utf-8"))


def _write_saved_map_files(map_name: str, map_payload: dict) -> tuple[Path, Path]:
    MAP_STORAGE_DIR.mkdir(parents=True, exist_ok=True)
    base_name = _safe_map_name(map_name)
    yaml_path = MAP_STORAGE_DIR / f"{base_name}.yaml"
    pgm_path = MAP_STORAGE_DIR / f"{base_name}.pgm"
    suffix = 2
    while yaml_path.exists() or pgm_path.exists():
        yaml_path = MAP_STORAGE_DIR / f"{base_name}_{suffix}.yaml"
        pgm_path = MAP_STORAGE_DIR / f"{base_name}_{suffix}.pgm"
        suffix += 1

    width = int(map_payload["width"])
    height = int(map_payload["height"])
    raw = _decode_grid_data(map_payload["grid_data"])
    if len(raw) != width * height:
        raise ValueError("Occupancy grid size does not match width/height")

    pixels = bytearray(width * height)
    for row in range(height):
        for col in range(width):
            source_index = (height - 1 - row) * width + col
            value = raw[source_index]
            if value == 255:
                color = 205
            elif value >= 100:
                color = 0
            elif value == 0:
                color = 254
            else:
                color = max(0, 254 - int(value * 2.54))
            pixels[row * width + col] = color

    with pgm_path.open("wb") as handle:
        handle.write(f"P5\n{width} {height}\n255\n".encode("ascii"))
        handle.write(pixels)

    yaml_payload = {
        "image": pgm_path.name,
        "mode": "trinary",
        "resolution": float(map_payload["resolution"]),
        "origin": [float(map_payload["origin_x"]), float(map_payload["origin_y"]), 0.0],
        "negate": 0,
        "occupied_thresh": 0.65,
        "free_thresh": 0.25,
    }
    with yaml_path.open("w", encoding="utf-8") as handle:
        yaml.safe_dump(yaml_payload, handle, sort_keys=False)

    return yaml_path, pgm_path


def _saved_map_summary(saved_map: SavedMap) -> dict:
    return {
        "id": saved_map.id,
        "name": saved_map.name,
        "width": saved_map.width,
        "height": saved_map.height,
        "resolution": saved_map.resolution,
        "origin_x": saved_map.origin_x,
        "origin_y": saved_map.origin_y,
        "yaml_path": saved_map.yaml_path,
        "pgm_path": saved_map.pgm_path,
        "created_at": saved_map.created_at.isoformat() if saved_map.created_at else None,
    }


def _saved_map_detail(saved_map: SavedMap) -> dict:
    return {
        **_saved_map_summary(saved_map),
        "grid_data": saved_map.grid_data,
    }


def _delete_map_file(path_value: str | None) -> None:
    if not path_value:
        return
    try:
        path = Path(path_value).expanduser()
        if path.exists() and path.is_file():
            path.unlink()
    except Exception as error:
        logger.warning("Failed to delete map file %s: %s", path_value, error)


@app.on_event("startup")
async def startup() -> None:
    _ensure_web_api_role()
    await init_db()
    await _load_runtime_settings()
    DATASET_BASE_PATH.mkdir(parents=True, exist_ok=True)
    MAP_STORAGE_DIR.mkdir(parents=True, exist_ok=True)
    WORLD_STORAGE_DIR.mkdir(parents=True, exist_ok=True)
    logger.info("Rai Web API started with role=%s label=%s.", DEVICE_ROLE, DEVICE_LABEL)


@app.on_event("shutdown")
async def shutdown() -> None:
    global simulation_process
    _stop_stack_process(simulation_process, SIMULATION_KILL_PATTERNS)
    simulation_process = None

    logger.info("Rai Web API stopped.")


@app.get("/api/health")
async def health() -> dict:
    pi_online = False
    jetson_online = False

    if PI_BRIDGE_URL:
        try:
            await asyncio.wait_for(_agent_request("GET", "pi", "/api/health"), timeout=2.0)
            pi_online = True
        except Exception:
            pass

    if JETSON_BRIDGE_URL:
        try:
            await asyncio.wait_for(_agent_request("GET", "jetson", "/api/health"), timeout=2.0)
            jetson_online = True
        except Exception:
            pass

    return {
        "status": "ok",
        "timestamp": datetime.utcnow().isoformat(),
        **_system_runtime_payload(pi_online=pi_online, jetson_online=jetson_online),
    }


@app.get("/api/system/runtime")
async def system_runtime() -> dict:
    await _load_runtime_settings()

    pi_online = False
    jetson_online = False

    if PI_BRIDGE_URL:
        try:
            await asyncio.wait_for(_agent_request("GET", "pi", "/api/health"), timeout=2.0)
            pi_online = True
        except Exception:
            pass

    if JETSON_BRIDGE_URL:
        try:
            await asyncio.wait_for(_agent_request("GET", "jetson", "/api/health"), timeout=2.0)
            jetson_online = True
        except Exception:
            pass

    return _system_runtime_payload(pi_online=pi_online, jetson_online=jetson_online)


@app.post("/api/system/operation-mode")
async def set_system_operation_mode(request: SystemOperationModeRequest) -> dict:
    _require_action("system")
    global system_operation_mode
    system_operation_mode = request.mode
    await _set_runtime_setting(RUNTIME_SETTING_OPERATION_MODE, request.mode)

    pi_online = False
    jetson_online = False

    for device in ("pi", "jetson"):
        if _can_proxy_to_peer(device):
            try:
                await _agent_request("POST", device, "/api/system/operation-mode", payload={"mode": request.mode})
                if device == "pi":
                    pi_online = True
                elif device == "jetson":
                    jetson_online = True
            except Exception:
                logger.exception("Failed to sync operation mode to %s", device)
    return {
        "success": True,
        "message": f"System operation mode switched to {request.mode}.",
        **_system_runtime_payload(pi_online=pi_online, jetson_online=jetson_online),
    }


@app.get("/api/system/components")
async def system_components(request: Request) -> dict:
    _require_action("system")
    await _load_runtime_settings()

    pi_online = False
    jetson_online = False

    if PI_BRIDGE_URL:
        try:
            await asyncio.wait_for(_agent_request("GET", "pi", "/api/health"), timeout=2.0)
            pi_online = True
        except Exception:
            pass

    if JETSON_BRIDGE_URL:
        try:
            await asyncio.wait_for(_agent_request("GET", "jetson", "/api/health"), timeout=2.0)
            jetson_online = True
        except Exception:
            pass

    components_by_id = {component["id"]: component for component in _local_system_components()}
    for device in ("pi", "jetson"):
        if _can_proxy_to_peer(device):
            try:
                # Inner _agent_request_sync now bounds the TCP connect and the
                # read phase. wait_for is the outer ceiling so a misbehaving
                # executor thread can never stall the request handler.
                peer_components = await asyncio.wait_for(
                    _agent_request("GET", device, "/api/system/components"),
                    timeout=3.5,
                )
                for component in peer_components.get("components", []):
                    component_id = component.get("id")
                    if component.get("host_device") != device or not component_id:
                        continue
                    if component_id in components_by_id:
                        components_by_id[component_id] = _merge_peer_component(components_by_id[component_id], component)
                    else:
                        components_by_id[component_id] = component
            except Exception:
                logger.exception("Failed to fetch components from %s", device)
    return {
        **_system_runtime_payload(pi_online=pi_online, jetson_online=jetson_online),
        "components": list(components_by_id.values()),
    }


@app.post("/api/system/components/robot/start")
async def start_robot_base() -> dict:
    return await _proxy_system_component("pi", "robot", "start")


@app.post("/api/system/components/robot/stop")
async def stop_robot_base() -> dict:
    return await _proxy_system_component("pi", "robot", "stop")


@app.post("/api/system/components/lidar/start")
async def start_lidar() -> dict:
    return await _proxy_system_component("pi", "lidar", "start")


@app.post("/api/system/components/lidar/stop")
async def stop_lidar() -> dict:
    return await _proxy_system_component("pi", "lidar", "stop")


@app.post("/api/system/components/camera/start")
async def start_camera(request: SystemComponentCommandRequest) -> dict:
    global camera_depth_enabled
    camera_depth_enabled = request.enable_depth
    await _set_runtime_setting(RUNTIME_SETTING_CAMERA_DEPTH, "true" if request.enable_depth else "false")
    return await _proxy_system_component("jetson", "camera", "start", request.dict())


@app.post("/api/system/components/camera/stop")
async def stop_camera() -> dict:
    return await _proxy_system_component("jetson", "camera", "stop")


@app.post("/api/system/components/slam/start")
async def start_system_slam() -> dict:
    return await _proxy_system_component("pi", "slam", "start")


@app.post("/api/system/components/slam/stop")
async def stop_system_slam() -> dict:
    return await _proxy_system_component("pi", "slam", "stop")


@app.post("/api/system/components/navigation/start")
async def start_system_navigation() -> dict:
    return await _proxy_system_component("pi", "navigation", "start")


@app.post("/api/system/components/navigation/stop")
async def stop_system_navigation() -> dict:
    return await _proxy_system_component("pi", "navigation", "stop")


@app.post("/api/system/components/simulation/start")
async def start_system_simulation() -> dict:
    request = SimulationStartRequest(
        world_id=simulation_runtime_config.get("world_id") or "s1_open_zone",
        robot_model=simulation_runtime_config.get("robot_model") or "mini_mec_robot",
        start_slam=bool(simulation_runtime_config.get("start_slam", True)),
        start_navigation=bool(simulation_runtime_config.get("start_navigation", False)),
    )
    return await start_simulation(request)


@app.post("/api/system/components/simulation/stop")
async def stop_system_simulation() -> dict:
    return await stop_simulation()


@app.post("/api/system/components/dataset/start")
async def start_system_dataset() -> dict:
    return await _proxy_system_component("pi", "dataset", "start")


@app.post("/api/system/components/dataset/stop")
async def stop_system_dataset() -> dict:
    return await _proxy_system_component("pi", "dataset", "stop")


@app.get("/api/rviz/topics")
async def get_rviz_topics() -> dict:
    return await _agent_request("GET", "pi", "/api/rviz/topics")


@app.post("/api/rviz/topics")
async def set_rviz_topics(request: RvizTopicsRequest) -> dict:
    topics = await _agent_request("POST", "pi", "/api/rviz/topics", payload=request.dict())
    return {"success": True, **topics}


@app.get("/api/telemetry/current")
async def current_telemetry() -> dict:
    payload = await _agent_request("GET", "pi", "/api/telemetry/current")
    return {
        "timestamp": datetime.utcnow().isoformat(),
        "telemetry": payload.get("telemetry", {}),
        "active_dataset_run_id": active_dataset_run_id,
    }


@app.post("/api/webrtc/offer")
async def webrtc_offer(offer: WebRtcOffer) -> dict:
    return await _agent_request("POST", "jetson", "/api/webrtc/offer", payload=offer.dict())


@app.websocket("/api/ws/telemetry")
async def telemetry_ws(websocket: WebSocket) -> None:
    await _relay_websocket(websocket, "pi", "/api/ws/telemetry")


@app.websocket("/api/ws/map")
async def map_ws(websocket: WebSocket) -> None:
    await _relay_websocket(websocket, "pi", "/api/ws/map")


@app.websocket("/api/ws/paths")
async def paths_ws(websocket: WebSocket) -> None:
    await _relay_websocket(websocket, "pi", "/api/ws/paths")


@app.websocket("/api/ws/dataset")
async def dataset_ws(websocket: WebSocket) -> None:
    await websocket.accept()
    try:
        while True:
            payload = await _active_dataset_payload()
            await websocket.send_json(payload)
            await asyncio.sleep(0.2)
    except WebSocketDisconnect:
        pass


@app.websocket("/api/ws/control")
async def control_ws(websocket: WebSocket) -> None:
    await _relay_websocket(websocket, "pi", "/api/ws/control")


@app.post("/api/robot/cmd_vel")
async def publish_cmd_vel(command: VelocityCommand) -> dict:
    return await _pi_agent_request("POST", "/api/robot/cmd_vel", payload=command.dict())


@app.post("/api/robot/nav/goal")
async def send_nav_goal(goal: NavGoalRequest) -> dict:
    return await _pi_agent_request("POST", "/api/robot/nav/goal", payload=goal.dict())


@app.post("/api/robot/nav/route")
async def send_nav_route(route: RoutePlanRequest) -> dict:
    return await _pi_agent_request("POST", "/api/robot/nav/route", payload=route.dict())


@app.post("/api/robot/nav/cancel")
async def cancel_nav_goal() -> dict:
    return await _pi_agent_request("POST", "/api/robot/nav/cancel")


@app.get("/api/robot/anchors")
async def get_robot_anchors() -> dict:
    return await _pi_agent_request("GET", "/api/robot/anchors")


@app.post("/api/robot/initial_pose")
async def set_initial_pose(request: InitialPoseRequest) -> dict:
    return await _pi_agent_request("POST", "/api/robot/initial_pose", payload=request.dict())


@app.post("/api/robot/home")
async def set_home_pose(request: PoseRequest) -> dict:
    return await _pi_agent_request("POST", "/api/robot/home", payload=request.dict())


@app.post("/api/robot/nav/home")
async def send_home_goal() -> dict:
    return await _pi_agent_request("POST", "/api/robot/nav/home")


@app.get("/api/rai-navigation/options")
async def rai_navigation_options() -> dict:
    return await _pi_agent_request("GET", "/api/rai-navigation/options")


@app.get("/api/rai-navigation/config")
async def rai_navigation_config() -> dict:
    return await _pi_agent_request("GET", "/api/rai-navigation/config")


@app.post("/api/rai-navigation/config")
async def set_rai_navigation_config(request: RaiNavigationConfigRequest, db: AsyncSession = Depends(get_db)) -> dict:
    local_planner = request.local_planner.upper()
    global_planner = request.global_planner.upper()

    if request.map_id is not None:
        saved_map = await db.get(SavedMap, request.map_id)
        if saved_map is None:
            raise HTTPException(status_code=404, detail="Saved map not found")
        if not saved_map.yaml_path:
            raise HTTPException(status_code=400, detail="Saved map does not have an exported YAML path")
        request.map_path = saved_map.yaml_path

    payload = request.dict()
    payload["local_planner"] = local_planner
    payload["global_planner"] = global_planner
    return await _pi_agent_request("POST", "/api/rai-navigation/config", payload=payload)


@app.post("/api/rai-navigation/start")
async def start_rai_navigation_stack() -> dict:
    return await _agent_request("POST", "pi", "/api/rai-navigation/start")


@app.post("/api/rai-navigation/stop")
async def stop_rai_navigation_stack() -> dict:
    return await _agent_request("POST", "pi", "/api/rai-navigation/stop")


@app.post("/api/robot/slam/start")
async def start_slam() -> dict:
    return await _agent_request("POST", "pi", "/api/robot/slam/start")


@app.post("/api/robot/slam/stop")
async def stop_slam() -> dict:
    return await _agent_request("POST", "pi", "/api/robot/slam/stop")


@app.get("/api/simulation/worlds")
async def list_simulation_worlds() -> list[dict]:
    _require_action("simulation")
    return [_world_summary(world_id, path, source) for world_id, path, source in _list_world_files()]


@app.post("/api/simulation/worlds")
async def create_simulation_world(request: SimulationWorldCreateRequest) -> dict:
    _require_action("simulation")
    world_path = _write_virtual_world(request)
    world_id = f"generated:{world_path.stem}"
    return {
        "success": True,
        "message": f"Created virtual world {world_path.name}.",
        "world": _world_summary(world_id, world_path, "generated"),
    }


@app.get("/api/simulation/status")
async def simulation_status() -> dict:
    running = simulation_process is not None and simulation_process.poll() is None
    simulation_runtime_config["running"] = running
    return {**simulation_runtime_config, "running": running}


@app.post("/api/simulation/start")
async def start_simulation(request: SimulationStartRequest) -> dict:
    _require_action("simulation")
    global simulation_process
    if simulation_process is not None and simulation_process.poll() is None:
        return {"success": True, "message": "Simulation is already running", "pid": simulation_process.pid}

    world_path = _safe_world_path(request.world_id)
    command = _simulation_launch_command(request, world_path)
    simulation_process = _start_process(command, env=_ros_runtime_env())
    simulation_runtime_config.update({
        "running": True,
        "world_id": request.world_id,
        "world_path": str(world_path),
        "robot_model": request.robot_model,
        "start_slam": request.start_slam,
        "start_navigation": request.start_navigation,
        "last_command": command,
    })
    return {
        "success": True,
        "message": "Simulation launch started.",
        "pid": simulation_process.pid,
        "command": command,
        "world_path": str(world_path),
    }


@app.post("/api/simulation/stop")
async def stop_simulation() -> dict:
    _require_action("simulation")
    global simulation_process
    result = _stop_stack_process(simulation_process, SIMULATION_KILL_PATTERNS)
    simulation_process = None
    simulation_runtime_config["running"] = False
    return {"success": True, "message": "Simulation stopped.", **result}


@app.post("/api/map/save")
async def save_map(request: SaveMapRequest, db: AsyncSession = Depends(get_db)) -> dict:
    _require_action("maps")
    snapshot = await _agent_request("GET", "pi", "/api/map/snapshot")
    current_map = snapshot.get("map")

    if current_map is None:
        raise HTTPException(
            status_code=404, 
            detail="No map data available from /map topic. Make sure SLAM or map_server is active."
        )

    
    try:
        yaml_path, pgm_path = _write_saved_map_files(request.name, current_map)
    except Exception as exc:
        raise HTTPException(status_code=500, detail=f"Failed to export map files: {exc}") from exc

    saved_map = SavedMap(
        name=request.name,
        width=current_map["width"],
        height=current_map["height"],
        resolution=current_map["resolution"],
        origin_x=current_map["origin_x"],
        origin_y=current_map["origin_y"],
        grid_data=current_map["grid_data"],
        yaml_path=str(yaml_path),
        pgm_path=str(pgm_path),
        created_at=datetime.utcnow()
    )
    db.add(saved_map)
    await db.commit()
    await db.refresh(saved_map)

    return {
        "success": True,
        "message": f"Map '{request.name}' saved successfully",
        "map_id": saved_map.id,
        "yaml_path": saved_map.yaml_path,
        "pgm_path": saved_map.pgm_path,
    }


@app.get("/api/map/list")
async def list_maps(db: AsyncSession = Depends(get_db)) -> list[dict]:
    _require_action("maps")
    result = await db.execute(select(SavedMap).order_by(desc(SavedMap.created_at)))
    maps = result.scalars().all()
    return [_saved_map_summary(saved_map) for saved_map in maps]


@app.get("/api/map/{map_id}")
async def get_map(map_id: int, db: AsyncSession = Depends(get_db)) -> dict:
    _require_action("maps")
    saved_map = await db.get(SavedMap, map_id)
    if saved_map is None:
        raise HTTPException(status_code=404, detail="Saved map not found")
    return _saved_map_detail(saved_map)


@app.get("/api/robot-models")
async def list_robot_models() -> list[dict]:
    urdf_dir = _rai_robot_urdf_share() / "urdf"
    if not urdf_dir.exists():
        return []
    models = []
    for path in sorted(urdf_dir.glob("*.urdf")):
        model_id = path.stem
        models.append({
            "id": model_id,
            "label": _robot_model_label(model_id),
            "urdf_path": f"urdf/{path.name}",
        })
    return models


@app.get("/api/robot-models/{model_id}/urdf")
async def get_robot_model_urdf(model_id: str) -> FileResponse:
    if "/" in model_id or "\\" in model_id:
        raise HTTPException(status_code=400, detail="Invalid robot model id")
    path = _safe_robot_model_path(f"urdf/{model_id}.urdf")
    return FileResponse(path, media_type="application/xml")


@app.get("/api/robot-models/assets/{asset_path:path}")
async def get_robot_model_asset(asset_path: str) -> FileResponse:
    path = _safe_robot_model_path(asset_path)
    return FileResponse(path)


@app.patch("/api/map/{map_id}")
async def update_map(map_id: int, request: UpdateMapRequest, db: AsyncSession = Depends(get_db)) -> dict:
    _require_action("maps")
    saved_map = await db.get(SavedMap, map_id)
    if saved_map is None:
        raise HTTPException(status_code=404, detail="Saved map not found")
    saved_map.name = request.name.strip()
    await db.commit()
    await db.refresh(saved_map)
    return {"success": True, "map": _saved_map_summary(saved_map)}


@app.delete("/api/map/{map_id}")
async def delete_map(map_id: int, db: AsyncSession = Depends(get_db)) -> dict:
    _require_action("maps")
    saved_map = await db.get(SavedMap, map_id)
    if saved_map is None:
        raise HTTPException(status_code=404, detail="Saved map not found")
    yaml_path = saved_map.yaml_path
    pgm_path = saved_map.pgm_path
    await db.delete(saved_map)
    await db.commit()
    _delete_map_file(yaml_path)
    _delete_map_file(pgm_path)
    return {"success": True, "map_id": map_id}


@app.post("/api/map/{map_id}/delete")
async def delete_map_post(map_id: int, db: AsyncSession = Depends(get_db)) -> dict:
    _require_action("maps")
    return await delete_map(map_id, db)


@app.post("/api/dataset/capture")
async def capture_dataset(request: DatasetCaptureRequest) -> dict:
    logger.info("Simulating dataset capture for tag=%s class=%s", request.tag, request.class_name)
    await asyncio.sleep(0.3)
    return {
        "success": True,
        "message": f"Captured image and metadata with tag={request.tag} class={request.class_name}",
        "timestamp": datetime.utcnow().isoformat(),
        "filename": f"capture_{request.tag}_{datetime.utcnow().strftime('%Y%m%d_%H%M%S')}.png",
        "tag": request.tag
        ,
        "class_name": request.class_name,
    }


@app.get("/api/dataset/artifacts")
async def dataset_artifacts() -> dict:
    _require_action("dataset")
    _ensure_dataset_layout()
    return _dataset_artifact_status()


@app.post("/api/dataset/prepare")
async def prepare_dataset_artifacts() -> dict:
    _require_action("dataset")
    _ensure_dataset_layout()
    return {"success": True, **_dataset_artifact_status()}


@app.get("/api/dataset/launch/status")
async def dataset_launch_status() -> dict:
    payload = await _agent_request("GET", "pi", "/api/dataset/launch/status")
    try:
        payload["record"] = await _agent_request("GET", "pi", "/api/dataset/record/status")
    except Exception:
        payload["record"] = {"running": False, "unavailable": True}
    payload["active_run_id"] = active_dataset_run_id
    return payload


@app.post("/api/dataset/launch/start")
async def start_dataset_launch(request: DatasetLaunchRequest) -> dict:
    return await _agent_request("POST", "pi", "/api/dataset/launch/start", payload=request.dict())


@app.post("/api/dataset/launch/stop")
async def stop_dataset_launch() -> dict:
    return await _agent_request("POST", "pi", "/api/dataset/launch/stop")


@app.post("/api/dataset/pipeline")
async def run_dataset_pipeline(request: DatasetPipelineRequest, db: AsyncSession = Depends(get_db)) -> dict:
    _require_action("dataset")
    _ensure_dataset_layout()
    if request.run_id and not request.bag_path:
        run = await db.get(DatasetRun, request.run_id)
        if run is None:
            raise HTTPException(status_code=404, detail="Dataset run not found")
        request.bag_path = run.raw_bag_path
    commands = _dataset_pipeline_commands(request)
    results = []
    for command in commands:
        started = datetime.utcnow()
        completed = await asyncio.to_thread(_run_pipeline_command, command)
        completed["started_at"] = started.isoformat()
        completed["finished_at"] = datetime.utcnow().isoformat()
        results.append(completed)
        if completed["returncode"] != 0:
            break
    return {
        "success": all(item["returncode"] == 0 for item in results),
        "action": request.action,
        "results": results,
        "artifacts": _dataset_artifact_status(),
    }


@app.post("/api/dataset/start")
async def start_dataset(request: DatasetStartRequest, db: AsyncSession = Depends(get_db)) -> dict:
    _require_action("dataset")
    global active_dataset_run_id
    if active_dataset_run_id is not None:
        raise HTTPException(status_code=409, detail="A dataset run is already active")

    record_status = await _agent_request("GET", "pi", "/api/dataset/record/status")
    if record_status.get("running"):
        raise HTTPException(status_code=409, detail="ros2 bag record is already running")

    _ensure_dataset_layout()
    scenario = await _get_or_create_scenario(db, request.scenario_name)
    if request.run_index is None:
        run_id = f"run_{datetime.utcnow().strftime('%Y%m%d_%H%M%S')}"
    else:
        run_id = f"run_{request.run_index:03d}"

    run_name = f"{request.scenario_name}_{request.controller_id}_{run_id}"
    run_path = DATASET_BASE_PATH / "raw" / request.environment / request.scenario_name / request.controller_id / run_id
    run_path.mkdir(parents=True, exist_ok=False)
    rosbag_path = run_path / "rosbag2"
    rosbag_log_path = run_path / "rosbag_record.log"
    metadata_path = run_path / "metadata.json"
    start_time = datetime.utcnow()

    telemetry_snapshot = await _runtime_telemetry()
    voltage = (telemetry_snapshot.get("battery") or {}).get("voltage")

    initial_metadata = {
        "schema_version": "2.0.0",
        "flow": "continuous_context_adaptive_ca_nmpc",
        "environment": request.environment,
        "scenario_id": request.scenario_name,
        "controller_id": request.controller_id,
        "run_id": run_id,
        "run_index": request.run_index,
        "split": request.split,
        "status": "RECORDING",
        "start_time": start_time.isoformat(),
        "end_time": "",
        "duration_sec": "",
        "bag_path": str(rosbag_path),
        "required_topics": DATASET_REQUIRED_TOPICS,
        "camera_topics": DATASET_CAMERA_TOPICS if request.record_camera else [],
        "random_seed": request.random_seed,
        "robot_start": {
            "x": request.robot_start_x,
            "y": request.robot_start_y,
            "theta": request.robot_start_theta,
        },
        "goal": {
            "x": request.goal_x,
            "y": request.goal_y,
            "theta": request.goal_theta,
        },
        "human_behavior": request.human_behavior,
        "intervention": request.intervention,
        "record_camera": request.record_camera,
        "notes": request.notes,
        "telemetry_at_start": telemetry_snapshot,
    }
    metadata_path.write_text(json.dumps(initial_metadata, indent=2), encoding="utf-8")
    _upsert_run_index(_run_index_row(initial_metadata))

    run = DatasetRun(
        scenario_id=scenario.id,
        run_name=run_name,
        environment=request.environment,
        controller_id=request.controller_id,
        run_index=request.run_index,
        split=request.split,
        data_path=str(run_path),
        raw_bag_path=str(rosbag_path),
        metadata_path=str(metadata_path),
        status="RECORDING",
        start_time=start_time,
        start_voltage=voltage,
        notes=request.notes,
    )
    db.add(run)
    recorded_topics = DATASET_REQUIRED_TOPICS + (DATASET_CAMERA_TOPICS if request.record_camera else [])
    record_payload = {
        "rosbag_path": str(rosbag_path),
        "log_path": str(rosbag_log_path),
        "topics": recorded_topics,
    }
    try:
        record_result = await _agent_request("POST", "pi", "/api/dataset/record/start", payload=record_payload)
        initial_metadata["rosbag_pid"] = record_result.get("pid")
        initial_metadata["rosbag_log_path"] = str(rosbag_log_path)
        metadata_path.write_text(json.dumps(initial_metadata, indent=2), encoding="utf-8")
        await db.commit()
        await db.refresh(run)
        active_dataset_run_id = run.id
    except Exception as exc:
        await db.rollback()
        shutil.rmtree(run_path, ignore_errors=True)
        logger.exception("Failed to start dataset recording")
        raise HTTPException(status_code=500, detail=f"Failed to start ros2 bag record: {exc}") from exc

    return {
        "success": True,
        "run_id": run.id,
        "run_name": run_name,
        "scenario_name": request.scenario_name,
        "controller_id": request.controller_id,
        "environment": request.environment,
        "run_key": run_id,
        "data_path": str(run_path),
        "rosbag_path": str(rosbag_path),
        "rosbag_pid": initial_metadata.get("rosbag_pid"),
        "rosbag_log_path": str(rosbag_log_path),
        "metadata_path": str(metadata_path),
        "run_index_path": str(DATASET_BASE_PATH / "metadata" / "run_index.csv"),
        "recorded_topics": recorded_topics,
        "record_bridge": record_result,
        "collector_parameters": {
            "base_path": str(DATASET_BASE_PATH),
            "environment": request.environment,
            "scenario_id": request.scenario_name,
            "controller_id": request.controller_id,
            "run_id": run_id,
            "split": request.split,
            "auto_start": True,
        },
        "collector_launch_example": (
            "ros2 launch rai_dataset_collection dataset_collection.launch.py "
            f"scenario:={request.scenario_name} controller:={request.controller_id} "
            f"environment:={request.environment} run_id:={run_id} split:={request.split} auto_start:=true"
        ),
    }


@app.get("/api/dataset/active")
async def active_dataset() -> dict:
    _require_action("dataset")
    return await _active_dataset_payload()


@app.post("/api/dataset/stop")
async def stop_dataset(background_tasks: BackgroundTasks, db: AsyncSession = Depends(get_db)) -> dict:
    _require_action("dataset")
    global active_dataset_run_id
    if active_dataset_run_id is None:
        raise HTTPException(status_code=409, detail="No active dataset run")

    run = await db.get(DatasetRun, active_dataset_run_id)
    if run is None:
        active_dataset_run_id = None
        raise HTTPException(status_code=404, detail="Active dataset run not found")

    end_time = datetime.utcnow()
    run.end_time = end_time
    run.duration = (end_time - run.start_time).total_seconds()
    run.status = "COMPLETED"
    rosbag_status = await _agent_request("POST", "pi", "/api/dataset/record/stop")
    run_metadata = _load_run_metadata(run.metadata_path)
    if run_metadata:
        run_metadata["status"] = "COMPLETED"
        run_metadata["end_time"] = end_time.isoformat()
        run_metadata["duration_sec"] = run.duration
        run_metadata["rosbag_stop"] = rosbag_status
        run_metadata["success"] = run_metadata.get("success", "")
        run_metadata["collision"] = run_metadata.get("collision", "")
        run_metadata["timeout"] = run_metadata.get("timeout", "")
        metadata_path = Path(run.metadata_path) if run.metadata_path else None
        if metadata_path:
            metadata_path.write_text(json.dumps(run_metadata, indent=2), encoding="utf-8")
        _upsert_run_index(_run_index_row(run_metadata))
    if run_metadata:
        run.samples_count = int(run_metadata.get("samples", 0) or 0)
        context = run_metadata.get("continuous_context", {})
        phi_samples = context.get("phi_h_samples", []) or []
        d_h_samples = context.get("d_h_samples", []) or []
        if phi_samples:
            run.phi_h_max = max(phi_samples)
        if d_h_samples:
            run.min_human_clearance = min(d_h_samples)
        solver = run_metadata.get("solver", {})
        solver_samples = float(solver.get("samples", 0) or 0)
        if solver_samples > 0:
            run.timeout_rate = float(solver.get("timeouts", 0) or 0) / solver_samples
        _apply_metrics_to_run(run, run_metadata.get("metrics", {}))

    telemetry_snapshot = await _runtime_telemetry()
    battery = telemetry_snapshot.get("battery") or {}
    run.end_voltage = battery.get("voltage")
    run.min_voltage = battery.get("voltage")
    run.avg_voltage = battery.get("voltage")
    run.min_percentage = battery.get("percentage")
    run.avg_percentage = battery.get("percentage")

    zip_path = f"{run.data_path}.zip"
    run.zip_path = zip_path
    background_tasks.add_task(_compress_dataset_run, run.id, run.data_path, zip_path)
    await db.commit()
    active_dataset_run_id = None
    return {
        "success": True,
        "run_id": run.id,
        "zip_path": zip_path,
        "download_url": f"/api/dataset/download/{run.id}",
        "rosbag": rosbag_status,
    }


@app.get("/api/dataset/runs")
async def list_dataset_runs(db: AsyncSession = Depends(get_db)) -> list[dict]:
    _require_action("dataset")
    result = await db.execute(select(DatasetRun).order_by(desc(DatasetRun.start_time)).limit(100))
    runs = result.scalars().all()
    payload = []
    for run in runs:
        metadata = _load_run_metadata(run.metadata_path)
        payload.append({
            "id": run.id,
            "run_name": run.run_name,
            "scenario_name": metadata.get("scenario_id"),
            "environment": run.environment,
            "controller_id": run.controller_id,
            "run_index": run.run_index,
            "split": run.split,
            "status": run.status,
            "validation_status": run.validation_status,
            "success": run.success,
            "data_path": run.data_path,
            "raw_bag_path": run.raw_bag_path,
            "metadata_path": run.metadata_path,
            "zip_path": run.zip_path,
            "duration": run.duration,
            "samples_count": run.samples_count,
            "phi_h_max": run.phi_h_max,
            "min_human_clearance": run.min_human_clearance,
            "timeout_rate": run.timeout_rate,
            "metrics": _run_metrics_dict(run),
            "start_voltage": run.start_voltage,
            "end_voltage": run.end_voltage,
            "avg_percentage": run.avg_percentage,
            "file_size_bytes": run.file_size_bytes,
            "start_time": run.start_time.isoformat() if run.start_time else None,
        })
    return payload


@app.get("/api/dataset/runs/{run_id}/artifacts")
async def get_dataset_run_artifacts(run_id: int, db: AsyncSession = Depends(get_db)) -> dict:
    _require_action("dataset")
    run = await db.get(DatasetRun, run_id)
    if run is None:
        raise HTTPException(status_code=404, detail="Dataset run not found")
    run_path = Path(run.data_path)
    derived_path = run_path / "derived"
    files = []
    if derived_path.exists():
      for path in sorted(derived_path.glob("*.csv")):
          files.append({
              "name": path.name,
              "relative_path": f"derived/{path.name}",
              "size_bytes": path.stat().st_size,
          })
    metadata_file = run_path / "metadata.json"
    if metadata_file.exists():
        files.append({
            "name": "metadata.json",
            "relative_path": "metadata.json",
            "size_bytes": metadata_file.stat().st_size,
        })
    return {
        "run_id": run.id,
        "run_name": run.run_name,
        "files": files,
    }


@app.get("/api/dataset/runs/{run_id}/download/{relative_path:path}")
async def download_dataset_artifact(run_id: int, relative_path: str, db: AsyncSession = Depends(get_db)) -> FileResponse:
    _require_action("dataset")
    run = await db.get(DatasetRun, run_id)
    if run is None:
        raise HTTPException(status_code=404, detail="Dataset run not found")
    base = Path(run.data_path).resolve()
    target = (base / relative_path).resolve()
    if base != target and base not in target.parents:
        raise HTTPException(status_code=400, detail="Invalid artifact path")
    if not target.exists() or not target.is_file():
        raise HTTPException(status_code=404, detail="Artifact not found")
    return FileResponse(path=target, filename=target.name, media_type="text/csv" if target.suffix == ".csv" else None)


@app.get("/api/dataset/runs/{run_id}/metrics")
async def get_dataset_run_metrics(run_id: int, db: AsyncSession = Depends(get_db)) -> dict:
    _require_action("dataset")
    run = await db.get(DatasetRun, run_id)
    if run is None:
        raise HTTPException(status_code=404, detail="Dataset run not found")
    metadata = _load_run_metadata(run.metadata_path)
    return {
        "run_id": run.id,
        "run_name": run.run_name,
        "metrics": metadata.get("metrics") or _run_metrics_dict(run),
    }


@app.get("/api/dataset/scenarios")
async def list_dataset_scenarios(db: AsyncSession = Depends(get_db)) -> list[dict]:
    _require_action("dataset")
    result = await db.execute(select(DatasetScenario).order_by(DatasetScenario.name))
    scenarios = result.scalars().all()
    return [
        {
            "id": scenario.id,
            "name": scenario.name,
            "context_type": scenario.context_type,
            "difficulty": scenario.difficulty,
            "human_mode": scenario.human_mode,
            "expected_runs": scenario.expected_runs,
            "description": scenario.description,
            "scientific_goal": scenario.scientific_goal,
            "layout": scenario.layout,
            "human_count": scenario.human_count,
            "human_speed": scenario.human_speed,
            "human_trajectory": scenario.human_trajectory,
            "primary_metric": scenario.primary_metric,
        }
        for scenario in scenarios
    ]


@app.get("/api/dataset/download/{run_id}")
async def download_dataset(run_id: int, db: AsyncSession = Depends(get_db)) -> FileResponse:
    _require_action("dataset")
    run = await db.get(DatasetRun, run_id)
    if run is None:
        raise HTTPException(status_code=404, detail="Dataset run not found")

    zip_path = Path(run.zip_path) if run.zip_path else Path(f"{run.data_path}.zip")
    if not zip_path.exists():
        data_path = Path(run.data_path)
        if not data_path.exists():
            raise HTTPException(status_code=404, detail="Dataset data path not found")
        await asyncio.to_thread(_make_archive, str(data_path), str(zip_path))
        run.zip_path = str(zip_path)
        run.file_size_bytes = zip_path.stat().st_size if zip_path.exists() else 0
        await db.commit()

    return FileResponse(path=zip_path, filename=zip_path.name, media_type="application/zip")


def _ensure_dataset_layout() -> None:
    directories = [
        "raw/sim",
        "raw/real",
        "derived/human_motion_dataset",
        "derived/context_dataset",
        "derived/navigation_dataset",
        "derived/baseline_comparison_dataset",
        "derived/aggregates",
        "metadata/calibration",
        "metadata/splits",
        "metadata/scenarios",
        "metadata/controllers",
        "metadata/videos",
        "figures/system",
        "figures/trajectories",
        "figures/timeseries",
        "figures/boxplots",
        "figures/latency",
        "figures/ablation",
        "figures/context",
        "figures/prediction",
        "figures/benchmark",
        "tables/csv",
        "tables/latex",
        "tables/ablation",
        "videos",
    ]
    for relative in directories:
        (DATASET_BASE_PATH / relative).mkdir(parents=True, exist_ok=True)

    _ensure_csv_header(DATASET_BASE_PATH / "metadata" / "run_index.csv", RUN_INDEX_FIELDS)
    _ensure_csv_header(DATASET_BASE_PATH / "derived" / "navigation_dataset" / "robot.csv", [
        "time", "x_r", "y_r", "theta_r", "vx_odom", "vy_odom", "omega_odom",
        "vx_cmd", "vy_cmd", "omega_cmd", "reference_x", "reference_y", "reference_theta",
        "path_length", "travel_time", "tracking_error_xy", "tracking_error_theta",
    ])
    _ensure_csv_header(DATASET_BASE_PATH / "derived" / "human_motion_dataset" / "human.csv", [
        "time", "human_id", "x_h", "y_h", "vx_h", "vy_h", "tracking_confidence",
        "age_sec", "covariance_x", "covariance_y", "covariance_vx", "covariance_vy",
        "covariance_trace", "source",
    ])
    _ensure_csv_header(DATASET_BASE_PATH / "derived" / "context_dataset" / "context.csv", [
        "time", "phi_h", "nearest_human_id", "nearest_human_distance", "relative_speed",
        "relative_heading", "human_confidence", "human_count", "tracking_quality",
        "occlusion_flag", "context_source",
    ])
    _ensure_csv_header(DATASET_BASE_PATH / "derived" / "context_dataset" / "adaptive_constraints.csv", [
        "time", "phi_h", "d_safe", "vx_max", "vy_max", "omega_max", "q_scale",
    ])
    _ensure_csv_header(DATASET_BASE_PATH / "derived" / "human_motion_dataset" / "predicted_human.csv", [
        "time", "human_id", "x_pred", "y_pred", "vx_pred", "vy_pred", "horizon_step", "prediction_source",
    ])
    _ensure_csv_header(DATASET_BASE_PATH / "derived" / "navigation_dataset" / "solver.csv", [
        "time", "solve_time_ms", "sample_count", "best_cost", "timeout", "collision", "status",
    ])
    _ensure_csv_header(DATASET_BASE_PATH / "derived" / "baseline_comparison_dataset" / "run_summary.csv", [
        "run_id", "scenario_id", "controller_id", "environment", "duration_sec",
        "mean_phi_h", "peak_phi_h", "context_activation_duration", "adaptive_distance_gain",
        "adaptive_velocity_reduction", "rmse_xy", "rmse_theta", "path_length", "travel_time",
        "goal_reaching_rate", "minimum_human_distance", "safety_violation_count",
        "safety_violation_duration", "near_miss_count", "time_to_collision", "jerk_mean",
        "jerk_max", "mean_delta_u", "control_energy", "solve_time_mean_ms", "solve_time_max_ms",
        "solve_time_std_ms", "timeout_rate", "prediction_rmse", "prediction_mae",
        "detection_precision", "detection_recall", "tracking_rate", "id_switches",
    ])
    _ensure_csv_header(DATASET_BASE_PATH / "metadata" / "videos" / "video_metadata.csv", [
        "scenario", "controller", "video", "humans", "environment", "operator", "run_id", "notes",
    ])
    _ensure_csv_header(DATASET_BASE_PATH / "derived" / "aggregates" / "controller_timeseries.csv", [
        "timestamp", "run_id", "scenario_id", "controller_id", "x_r", "y_r", "theta_r",
        "x_ref", "y_ref", "theta_ref", "tracking_error_xy", "tracking_error_theta",
        "vx_cmd", "vy_cmd", "omega_cmd", "vx_odom", "vy_odom", "omega_odom", "d_h",
        "phi_h", "d_safe", "vx_max_adaptive", "vy_max_adaptive", "omega_max_adaptive",
        "q_x_adaptive", "q_y_adaptive", "q_theta_adaptive", "sample_count",
        "evaluation_time_ms", "controller_status", "timeout_flag", "collision_flag",
        "occlusion_flag",
    ])
    _ensure_csv_header(DATASET_BASE_PATH / "derived" / "aggregates" / "human_states.csv", [
        "timestamp", "run_id", "scenario_id", "human_id", "x_h", "y_h", "vx_h", "vy_h",
        "confidence", "age_sec", "cov_x", "cov_y", "cov_vx", "cov_vy", "x_h_gt",
        "y_h_gt", "vx_h_gt", "vy_h_gt",
    ])
    _ensure_csv_header(DATASET_BASE_PATH / "derived" / "aggregates" / "human_prediction.csv", [
        "timestamp", "run_id", "scenario_id", "human_id", "horizon_i", "x_h_pred",
        "y_h_pred", "x_h_gt", "y_h_gt", "prediction_error",
    ])
    _ensure_csv_header(DATASET_BASE_PATH / "derived" / "aggregates" / "adaptation_timeseries.csv", [
        "timestamp", "run_id", "scenario_id", "controller_id", "d_h", "phi_h", "d_safe",
        "vx_max_adaptive", "vy_max_adaptive", "omega_max_adaptive", "q_x_adaptive",
        "q_y_adaptive", "q_theta_adaptive", "vx_cmd", "vy_cmd", "omega_cmd",
        "occlusion_flag",
    ])
    _ensure_csv_header(DATASET_BASE_PATH / "derived" / "aggregates" / "metrics_per_run.csv", [
        "run_id", "scenario_id", "controller_id", "environment", "success", "collision",
        "timeout", "duration_sec", "rmse_xy", "rmse_theta", "max_lateral_error", "d_min",
        "d_avg", "d_5percentile", "violation_count", "violation_duration",
        "collision_count", "jerk_mean", "jerk_max", "mean_abs_delta_u", "max_abs_delta_u",
        "control_effort", "mean_abs_vx", "mean_abs_vy", "mean_abs_omega",
        "solve_time_mean_ms", "solve_time_median_ms", "solve_time_p95_ms",
        "solve_time_max_ms", "timeout_rate",
    ])
    _ensure_csv_header(DATASET_BASE_PATH / "derived" / "aggregates" / "metrics_summary.csv", [
        "scenario_id", "controller_id", "environment", "n_runs", "success_rate",
        "collision_rate", "timeout_rate_mean", "rmse_xy_mean", "rmse_xy_std",
        "d_min_mean", "d_min_std", "solve_time_p95_mean_ms",
    ])

    metadata_files = {
        "metadata/sensors.yaml": {
            "required_topics": DATASET_REQUIRED_TOPICS,
            "optional_camera_topics": DATASET_CAMERA_TOPICS,
        },
        "metadata/robot_mecanum.yaml": {
            "robot": "rai_mecanum",
            "kinematics": "mecanum_omnidirectional",
            "command_topic": "/cmd_vel",
            "precondition": "All robot, human, path, and costmap samples must be transformed into one control frame before metric extraction.",
        },
        "metadata/controllers.yaml": {
            "controllers": BASELINE_CONTROLLERS,
        },
        "metadata/ablation.yaml": {
            "ablation_controllers": ABLATION_CONTROLLERS,
        },
        "metadata/dataset_groups.yaml": {
            "dataset_groups": DATASET_GROUPS,
        },
        "metadata/statistical_validation.yaml": STATISTICAL_PLAN,
        "metadata/required_figures.yaml": {
            "figures": REQUIRED_FIGURES,
        },
        "metadata/scenarios.yaml": {
            "scenarios": SCENARIO_SPECS,
        },
    }
    for relative, payload in metadata_files.items():
        path = DATASET_BASE_PATH / relative
        if not path.exists():
            path.write_text(yaml.safe_dump(payload, sort_keys=False), encoding="utf-8")

    for split_name in ("train_runs.txt", "val_runs.txt", "test_runs.txt"):
        path = DATASET_BASE_PATH / "metadata" / "splits" / split_name
        if not path.exists():
            path.write_text("", encoding="utf-8")


def _ensure_csv_header(path: Path, fields: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists() and path.stat().st_size > 0:
        return
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()


def _dataset_artifact_status() -> dict:
    expected_files = [
        "metadata/run_index.csv",
        "metadata/sensors.yaml",
        "metadata/robot_mecanum.yaml",
        "metadata/controllers.yaml",
        "metadata/ablation.yaml",
        "metadata/dataset_groups.yaml",
        "metadata/statistical_validation.yaml",
        "metadata/required_figures.yaml",
        "metadata/scenarios.yaml",
        "derived/human_motion_dataset/human.csv",
        "derived/human_motion_dataset/predicted_human.csv",
        "derived/context_dataset/context.csv",
        "derived/context_dataset/adaptive_constraints.csv",
        "derived/navigation_dataset/robot.csv",
        "derived/navigation_dataset/solver.csv",
        "derived/baseline_comparison_dataset/run_summary.csv",
        "metadata/videos/video_metadata.csv",
    ]
    return {
        "base_path": str(DATASET_BASE_PATH),
        "required_topics": DATASET_REQUIRED_TOPICS,
        "optional_camera_topics": DATASET_CAMERA_TOPICS,
        "dataset_groups": DATASET_GROUPS,
        "baseline_controllers": BASELINE_CONTROLLERS,
        "ablation_controllers": ABLATION_CONTROLLERS,
        "required_figures": REQUIRED_FIGURES,
        "statistical_validation": STATISTICAL_PLAN,
        "files": {
            relative: (DATASET_BASE_PATH / relative).exists()
            for relative in expected_files
        },
    }


def _run_index_row(metadata: dict) -> dict:
    robot_start = metadata.get("robot_start", {}) or {}
    goal = metadata.get("goal", {}) or {}
    return {
        "run_id": metadata.get("run_id", ""),
        "scenario_id": metadata.get("scenario_id", ""),
        "controller_id": metadata.get("controller_id", ""),
        "environment": metadata.get("environment", ""),
        "start_time": metadata.get("start_time", ""),
        "end_time": metadata.get("end_time", ""),
        "duration_sec": metadata.get("duration_sec", ""),
        "bag_path": metadata.get("bag_path", ""),
        "success": metadata.get("success", ""),
        "collision": metadata.get("collision", ""),
        "timeout": metadata.get("timeout", ""),
        "intervention": metadata.get("intervention", ""),
        "random_seed": metadata.get("random_seed", ""),
        "robot_start_x": robot_start.get("x", ""),
        "robot_start_y": robot_start.get("y", ""),
        "robot_start_theta": robot_start.get("theta", ""),
        "goal_x": goal.get("x", ""),
        "goal_y": goal.get("y", ""),
        "goal_theta": goal.get("theta", ""),
        "human_behavior": metadata.get("human_behavior", ""),
        "notes": metadata.get("notes", ""),
    }


def _upsert_run_index(row: dict) -> None:
    path = DATASET_BASE_PATH / "metadata" / "run_index.csv"
    _ensure_csv_header(path, RUN_INDEX_FIELDS)
    rows = []
    with path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        rows = [item for item in reader if item.get("run_id") != row.get("run_id")]
    rows.append({field: row.get(field, "") for field in RUN_INDEX_FIELDS})
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=RUN_INDEX_FIELDS)
        writer.writeheader()
        writer.writerows(rows)


def _dataset_pipeline_commands(request: DatasetPipelineRequest) -> list[list[str]]:
    python = os.getenv("PYTHON", "python3")
    dataset = str(DATASET_BASE_PATH)
    bag_path = request.bag_path

    scripts = {
        "validate": [[python, "scripts/dataset/validate_bag.py", "--dataset", dataset, *(["--bag", bag_path] if bag_path else [])]],
        "bag_to_csv": [
            [python, "scripts/dataset/bag_to_csv.py", "--dataset", dataset, *(["--bag", bag_path] if bag_path else [])],
            [python, "scripts/dataset/extract_controller_timeseries.py", "--dataset", dataset, *(["--bag", bag_path] if bag_path else [])],
            [python, "scripts/dataset/extract_human_states.py", "--dataset", dataset, *(["--bag", bag_path] if bag_path else [])],
        ],
        "metrics": [
            [python, "scripts/dataset/build_run_index.py", "--dataset", dataset],
            [python, "scripts/dataset/extract_metrics.py", "--dataset", dataset],
        ],
        "plots": [
            [python, "scripts/plot/plot_timeseries.py", "--dataset", dataset],
            [python, "scripts/plot/plot_trajectories.py", "--dataset", dataset],
            [python, "scripts/plot/plot_boxplots.py", "--dataset", dataset],
            [python, "scripts/plot/plot_latency.py", "--dataset", dataset],
        ],
        "tables": [[python, "scripts/tables/make_latex_tables.py", "--dataset", dataset]],
    }
    if request.action == "all":
        return scripts["validate"] + scripts["bag_to_csv"] + scripts["metrics"] + scripts["plots"] + scripts["tables"]
    return scripts[request.action]


def _run_pipeline_command(command: list[str]) -> dict:
    started = time.monotonic()
    try:
        completed = subprocess.run(
            command,
            cwd=str(DEFAULT_WORKSPACE_DIR),
            text=True,
            capture_output=True,
            timeout=600,
            env=_ros_runtime_env(),
        )
        return {
            "command": command,
            "returncode": completed.returncode,
            "stdout": completed.stdout[-4000:],
            "stderr": completed.stderr[-4000:],
            "duration_sec": round(time.monotonic() - started, 3),
        }
    except subprocess.TimeoutExpired as exc:
        return {
            "command": command,
            "returncode": 124,
            "stdout": (exc.stdout or "")[-4000:] if isinstance(exc.stdout, str) else "",
            "stderr": "Pipeline command timed out",
            "duration_sec": round(time.monotonic() - started, 3),
        }


async def _get_or_create_scenario(db: AsyncSession, scenario_name: str) -> DatasetScenario:
    result = await db.execute(select(DatasetScenario).where(DatasetScenario.name == scenario_name))
    scenario = result.scalars().first()
    if scenario is not None:
        return scenario

    spec = next((item for item in SCENARIO_SPECS if item["id"] == scenario_name), None)
    scenario = DatasetScenario(
        name=scenario_name,
        context_type="CONTINUOUS",
        difficulty=(spec or {}).get("difficulty", "manual"),
        human_mode="manual",
        expected_runs=(spec or {}).get("minimum_runs", 0),
        description=(spec or {}).get("scientific_goal", "Manual/no-map CCA-NMPC dataset collection scenario"),
        scientific_goal=(spec or {}).get("scientific_goal"),
        layout=(spec or {}).get("layout"),
        human_count=(spec or {}).get("human_count"),
        human_speed=(spec or {}).get("human_speed"),
        human_trajectory=(spec or {}).get("human_trajectory"),
        primary_metric=(spec or {}).get("primary_metric"),
    )
    db.add(scenario)
    await db.commit()
    await db.refresh(scenario)
    return scenario


async def _compress_dataset_run(run_id: int, data_path: str, zip_path: str) -> None:
    await asyncio.to_thread(_make_archive, data_path, zip_path)
    session = create_session()
    try:
        run = await session.get(DatasetRun, run_id)
        if run is None:
            return
        run.status = "COMPRESSED"
        run.file_size_bytes = Path(zip_path).stat().st_size if Path(zip_path).exists() else 0
        await session.commit()
    finally:
        await session.close()


def _make_archive(data_path: str, zip_path: str) -> None:
    archive_base = zip_path[:-4] if zip_path.endswith(".zip") else zip_path
    shutil.make_archive(archive_base, "zip", data_path)


def _load_run_metadata(metadata_path: str | None) -> dict:
    if not metadata_path:
        return {}
    path = Path(metadata_path)
    if not path.exists():
        return {}
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return {}


def _apply_metrics_to_run(run: DatasetRun, metrics: dict) -> None:
    tracking = metrics.get("tracking", {}) if isinstance(metrics, dict) else {}
    safety = metrics.get("safety", {}) if isinstance(metrics, dict) else {}
    smoothness = metrics.get("smoothness", {}) if isinstance(metrics, dict) else {}
    control = metrics.get("control", {}) if isinstance(metrics, dict) else {}
    realtime = metrics.get("realtime", {}) if isinstance(metrics, dict) else {}

    run.rmse_xy = tracking.get("rmse_xy")
    run.rmse_theta = tracking.get("rmse_theta")
    run.max_lateral_error = tracking.get("max_lateral_error")
    run.d_min = safety.get("d_min")
    run.d_avg = safety.get("d_avg")
    run.d_5percentile = safety.get("d_5percentile")
    run.violation_count = safety.get("violation_count")
    run.violation_duration = safety.get("violation_duration")
    run.collision_count = safety.get("collision_count")
    run.jerk_mean = smoothness.get("jerk_mean")
    run.jerk_max = smoothness.get("jerk_max")
    run.mean_abs_delta_u = smoothness.get("mean_abs_delta_u")
    run.max_abs_delta_u = smoothness.get("max_abs_delta_u")
    run.control_effort = control.get("control_effort")
    run.mean_abs_vx = control.get("mean_abs_vx")
    run.mean_abs_vy = control.get("mean_abs_vy")
    run.mean_abs_omega = control.get("mean_abs_omega")
    run.solve_time_mean_ms = realtime.get("solve_time_mean_ms")
    run.solve_time_median_ms = realtime.get("solve_time_median_ms")
    run.solve_time_p95_ms = realtime.get("solve_time_p95_ms")
    run.solve_time_max_ms = realtime.get("solve_time_max_ms")
    if realtime.get("timeout_rate") is not None:
        run.timeout_rate = realtime.get("timeout_rate")


def _run_metrics_dict(run: DatasetRun) -> dict:
    return {
        "tracking": {
            "rmse_xy": run.rmse_xy,
            "rmse_theta": run.rmse_theta,
            "max_lateral_error": run.max_lateral_error,
        },
        "safety": {
            "d_min": run.d_min if run.d_min is not None else run.min_human_clearance,
            "d_avg": run.d_avg,
            "d_5percentile": run.d_5percentile,
            "violation_count": run.violation_count,
            "violation_duration": run.violation_duration,
            "collision_count": run.collision_count,
        },
        "smoothness": {
            "jerk_mean": run.jerk_mean,
            "jerk_max": run.jerk_max,
            "mean_abs_delta_u": run.mean_abs_delta_u,
            "max_abs_delta_u": run.max_abs_delta_u,
        },
        "control": {
            "control_effort": run.control_effort,
            "mean_abs_vx": run.mean_abs_vx,
            "mean_abs_vy": run.mean_abs_vy,
            "mean_abs_omega": run.mean_abs_omega,
        },
        "realtime": {
            "solve_time_mean_ms": run.solve_time_mean_ms,
            "solve_time_median_ms": run.solve_time_median_ms,
            "solve_time_p95_ms": run.solve_time_p95_ms,
            "solve_time_max_ms": run.solve_time_max_ms,
            "timeout_rate": run.timeout_rate,
        },
    }


async def _active_dataset_payload() -> dict:
    telemetry = await _runtime_telemetry()

    payload = {
        "timestamp": datetime.utcnow().isoformat(),
        "active": active_dataset_run_id is not None,
        "run": None,
        "metadata": None,
        "telemetry": telemetry,
    }

    if active_dataset_run_id is None:
        return payload

    session = create_session()
    try:
        run = await session.get(DatasetRun, active_dataset_run_id)
        if run is None:
            return payload
        metadata = _load_run_metadata(run.metadata_path)
        payload["run"] = {
            "id": run.id,
            "run_name": run.run_name,
            "environment": run.environment,
            "controller_id": run.controller_id,
            "scenario_name": metadata.get("scenario_id"),
            "data_path": run.data_path,
            "raw_bag_path": run.raw_bag_path,
            "metadata_path": run.metadata_path,
            "status": run.status,
            "start_time": run.start_time.isoformat() if run.start_time else None,
        }
        payload["metadata"] = metadata
        return payload
    finally:
        await session.close()


def main() -> None:
    uvicorn.run(app, host=DEFAULT_HOST, port=DEFAULT_PORT, log_level="info", ws="wsproto")


if __name__ == "__main__":
    main()
