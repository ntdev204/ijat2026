import asyncio
import base64
import csv
import json
import logging
import os
import signal
import shutil
import subprocess
import threading
import tempfile
import time
from datetime import datetime
from pathlib import Path
from typing import Optional

import rclpy
import uvicorn
import yaml
from ament_index_python.packages import get_package_share_directory
from aiortc import RTCPeerConnection, RTCSessionDescription
from fastapi import BackgroundTasks, Depends, FastAPI, HTTPException, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse
from pydantic import BaseModel, Field
from sqlalchemy import desc, select
from sqlalchemy.ext.asyncio import AsyncSession

from rai_web_api.database import DatasetRun, DatasetScenario, SavedMap, create_session, get_db, init_db
from rai_web_api.node import WebBridgeNode
from rai_web_api.webrtc import RosImageVideoTrack

logger = logging.getLogger("rai_web_api")
logging.basicConfig(level=os.getenv("RAI_API_LOG_LEVEL", "INFO"))

DATASET_BASE_PATH = Path(os.getenv("RAI_DATASET_PATH", "/home/rai/ijat2026/dataset")).expanduser()
DEFAULT_HOST = os.getenv("RAI_API_HOST", "0.0.0.0")
DEFAULT_PORT = int(os.getenv("RAI_API_PORT", "8080"))
DEVICE_ROLE = os.getenv("RAI_DEVICE_ROLE", "unknown").strip().lower()
DEVICE_LABEL = os.getenv("RAI_DEVICE_LABEL", DEVICE_ROLE or "unknown")

app = FastAPI(title="Rai Robot Web API", version="1.0.0")
app.add_middleware(
    CORSMiddleware,
    allow_origins=os.getenv("RAI_API_CORS", "*").split(","),
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

bridge_node: Optional[WebBridgeNode] = None
spin_thread: Optional[threading.Thread] = None
peer_connections: set[RTCPeerConnection] = set()
active_dataset_run_id: Optional[int] = None
dataset_bag_process: Optional[subprocess.Popen] = None
rai_navigation_process: Optional[subprocess.Popen] = None
slam_process: Optional[subprocess.Popen] = None

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

RAI_NAVIGATION_KILL_PATTERNS = (
    "ros2 launch rai_navigation rai_navigation.launch.py",
    "rai_controller_server",
    "__node:=rai_controller",
)
SLAM_KILL_PATTERNS = (
    "ros2 launch rai_slam_toolbox online_async_launch.py",
    "async_slam_toolbox_node",
    "__node:=slam_toolbox",
)

RAI_CONTROLLER_OPTIONS = [
    {"id": "CCA_NMPC", "label": "CCA-NMPC", "plugin": "rai_controller_cca_nmpc", "native": True},
]
RAI_GLOBAL_PLANNER_OPTIONS = [
    {"id": "A_STAR", "label": "A*", "plugin": "rai_planner_a_star"},
    {"id": "DIJKSTRA", "label": "Dijkstra", "plugin": "rai_planner_dijkstra"},
    {"id": "STRAIGHT_LINE", "label": "Straight line", "plugin": "rai_planner_straight_line"},
]
default_local_planner = os.getenv("RAI_NAVIGATION_CONTROLLER", "CCA_NMPC").upper()
if default_local_planner not in {item["id"] for item in RAI_CONTROLLER_OPTIONS}:
    default_local_planner = "CCA_NMPC"

rai_navigation_runtime_config = {
    "local_planner": default_local_planner,
    "global_planner": os.getenv("RAI_NAVIGATION_GLOBAL_PLANNER", "A_STAR").upper(),
    "map_path": os.getenv("RAI_NAVIGATION_MAP", "/home/rai/rai_ros2/data/map/RAI.yaml"),
    "selected_map_id": None,
    "params_path": os.getenv("RAI_NAVIGATION_PARAMS", ""),
    "last_command": None,
}

training_state = {
    "running": False,
    "progress": 0,
    "epoch": 0,
    "loss": 0.0,
    "accuracy": 0.0,
    "history": []
}
MAP_STORAGE_DIR = Path(os.getenv("RAI_MAP_STORAGE_DIR", "/home/rai/rai_ros2/data/map")).expanduser()

ROLE_ALLOWED_ACTIONS = {
    "pi": {"teleop", "navigation", "nav2", "slam", "dataset", "maps"},
    "jetson": {"controller"},
}
ALLOWED_ACTIONS = ROLE_ALLOWED_ACTIONS.get(DEVICE_ROLE, set())


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


class TrainStartRequest(BaseModel):
    epochs: int = Field(default=50, ge=1)
    learning_rate: float = Field(default=0.001, gt=0.0)
    batch_size: int = Field(default=32, ge=1)
    architecture: str = Field(default="ResNet18")


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


def _rai_navigation_package_share() -> Path:
    return Path(get_package_share_directory("rai_navigation")).resolve()


def _default_rai_navigation_params_path() -> Path:
    package_share = _rai_navigation_package_share()
    return package_share / "config" / "rai_navigation.yaml"


def _default_rai_navigation_map_path() -> Path:
    return Path("/home/rai/rai_ros2/data/map/RAI.yaml")


def _runtime_rai_navigation_params_path(local_planner: str, global_planner: str, base_params_path: Path) -> Path:
    with base_params_path.open("r", encoding="utf-8") as handle:
        config = yaml.safe_load(handle) or {}

    controller_params = config.setdefault("rai_controller", {}).setdefault("ros__parameters", {})
    controller_params["controller_id"] = local_planner
    controller_params["global_planner_algorithm"] = global_planner

    temp_dir = Path(tempfile.mkdtemp(prefix="rai_web_api_navigation_"))
    params_path = temp_dir / "rai_navigation_runtime.yaml"
    with params_path.open("w", encoding="utf-8") as handle:
        yaml.safe_dump(config, handle, sort_keys=False)
    return params_path


def _is_cca_nmpc(local_planner: str | None = None) -> bool:
    planner = local_planner or rai_navigation_runtime_config["local_planner"]
    return planner.upper() == "CCA_NMPC"


def _rai_navigation_launch_command(
    map_path: Path,
    params_path: Path,
    local_planner: str,
    global_planner: str,
    disable_controller_server: bool = False,
) -> str:
    _ = disable_controller_server
    return (
        "ros2 launch rai_navigation rai_navigation.launch.py "
        f"params:={params_path} controller_id:={local_planner} "
        f"global_planner_algorithm:={global_planner} map:={map_path} map_topic:=/map"
    )


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


def _system_runtime_payload() -> dict:
    return {
        "device_role": DEVICE_ROLE,
        "device_label": DEVICE_LABEL,
        "allowed_actions": sorted(ALLOWED_ACTIONS),
        "api_host": DEFAULT_HOST,
        "api_port": DEFAULT_PORT,
    }


def _require_action(action: str) -> None:
    if action == "navigation" and "nav2" in ALLOWED_ACTIONS:
        return
    if action == "nav2" and "navigation" in ALLOWED_ACTIONS:
        return
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


def _publish_anchor_pose_after_delay(delay_sec: float = 3.0, attempts: int = 5, interval_sec: float = 0.5) -> None:
    if bridge_node is None:
        return

    def _worker() -> None:
        time.sleep(delay_sec)
        for _ in range(attempts):
            if bridge_node is None:
                return
            anchors = bridge_node.get_anchor_state()
            initial_pose = anchors.get("initial_pose")
            if initial_pose:
                bridge_node.publish_initial_pose(
                    float(initial_pose["x"]),
                    float(initial_pose["y"]),
                    float(initial_pose["yaw"]),
                    set_home=anchors.get("home_pose") is None,
                )
                return
            time.sleep(interval_sec)

    threading.Thread(target=_worker, daemon=True).start()


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


def _spin_ros_node() -> None:
    if bridge_node is None:
        return
    rclpy.spin(bridge_node)


@app.on_event("startup")
async def startup() -> None:
    global bridge_node, spin_thread
    await init_db()
    if not rclpy.ok():
        rclpy.init(args=None)
    bridge_node = WebBridgeNode()
    spin_thread = threading.Thread(target=_spin_ros_node, daemon=True)
    spin_thread.start()
    DATASET_BASE_PATH.mkdir(parents=True, exist_ok=True)
    if not rai_navigation_runtime_config["params_path"]:
        rai_navigation_runtime_config["params_path"] = str(_default_rai_navigation_params_path())
    if not Path(rai_navigation_runtime_config["map_path"]).exists():
        rai_navigation_runtime_config["map_path"] = ""
    logger.info("Rai Web API started on robot with role=%s label=%s.", DEVICE_ROLE, DEVICE_LABEL)


@app.on_event("shutdown")
async def shutdown() -> None:
    global rai_navigation_process, slam_process
    close_tasks = [pc.close() for pc in list(peer_connections)]
    if close_tasks:
        await asyncio.gather(*close_tasks, return_exceptions=True)
    peer_connections.clear()

    _stop_stack_process(rai_navigation_process, RAI_NAVIGATION_KILL_PATTERNS)
    _stop_stack_process(slam_process, SLAM_KILL_PATTERNS)
    rai_navigation_process = None
    slam_process = None

    if bridge_node is not None:
        bridge_node.destroy_node()
    if rclpy.ok():
        rclpy.shutdown()
    logger.info("Rai Web API stopped.")


@app.get("/api/health")
async def health() -> dict:
    return {"status": "ok", "timestamp": datetime.utcnow().isoformat(), **_system_runtime_payload()}


@app.get("/api/system/runtime")
async def system_runtime() -> dict:
    return _system_runtime_payload()


@app.get("/api/telemetry/current")
async def current_telemetry() -> dict:
    if bridge_node is None:
        raise HTTPException(status_code=503, detail="ROS2 bridge is not ready")
    bridge_node.ensure_telemetry_subscriptions()
    with bridge_node.lock:
        telemetry = json.loads(json.dumps(bridge_node.telemetry))
    return {
        "timestamp": datetime.utcnow().isoformat(),
        "telemetry": telemetry,
        "active_dataset_run_id": active_dataset_run_id,
    }


@app.post("/api/webrtc/offer")
async def webrtc_offer(offer: WebRtcOffer) -> dict:
    if bridge_node is None:
        raise HTTPException(status_code=503, detail="ROS2 bridge is not ready")

    pc = RTCPeerConnection()
    peer_connections.add(pc)
    bridge_node.register_camera_client()
    pc.addTrack(RosImageVideoTrack(bridge_node))

    @pc.on("connectionstatechange")
    async def on_connection_state_change() -> None:
        if pc.connectionState in {"failed", "closed", "disconnected"}:
            await pc.close()
            peer_connections.discard(pc)
            bridge_node.unregister_camera_client()

    await pc.setRemoteDescription(RTCSessionDescription(sdp=offer.sdp, type=offer.type))
    answer = await pc.createAnswer()
    await pc.setLocalDescription(answer)
    return {"sdp": pc.localDescription.sdp, "type": pc.localDescription.type}


@app.websocket("/api/ws/telemetry")
async def telemetry_ws(websocket: WebSocket) -> None:
    if bridge_node is None:
        await websocket.close(code=1011)
        return

    await websocket.accept()
    bridge_node.register_telemetry_client()
    try:
        while True:
            with bridge_node.lock:
                telemetry = json.loads(json.dumps(bridge_node.telemetry))
            await websocket.send_json(telemetry)
            await asyncio.sleep(0.1)
    except WebSocketDisconnect:
        pass
    finally:
        bridge_node.unregister_telemetry_client()


@app.websocket("/api/ws/map")
async def map_ws(websocket: WebSocket) -> None:
    if bridge_node is None:
        await websocket.close(code=1011)
        return
    if "maps" not in ALLOWED_ACTIONS:
        await websocket.close(code=1008, reason=f"maps not allowed on {DEVICE_ROLE}")
        return

    bridge_node.ensure_map_subscription()
    await websocket.accept()
    bridge_node.register_map_client()
    try:
        while True:
            with bridge_node.lock:
                current_map = bridge_node.latest_map
            
            if current_map is not None:
                await websocket.send_json(current_map)
            await asyncio.sleep(1.0)
    except WebSocketDisconnect:
        pass
    finally:
        bridge_node.unregister_map_client()


@app.websocket("/api/ws/paths")
async def paths_ws(websocket: WebSocket) -> None:
    if bridge_node is None:
        await websocket.close(code=1011)
        return

    await websocket.accept()
    bridge_node.register_paths_client()
    try:
        while True:
            with bridge_node.lock:
                global_plan = list(bridge_node.latest_global_plan)
                local_plan = list(bridge_node.latest_local_plan)
            
            await websocket.send_json({
                "global_plan": global_plan,
                "local_plan": local_plan,
                "timestamp": datetime.utcnow().isoformat()
            })
            await asyncio.sleep(0.2)
    except WebSocketDisconnect:
        pass
    finally:
        bridge_node.unregister_paths_client()


@app.websocket("/api/ws/dataset")
async def dataset_ws(websocket: WebSocket) -> None:
    if bridge_node is None:
        await websocket.close(code=1011)
        return
    if "dataset" not in ALLOWED_ACTIONS:
        await websocket.close(code=1008, reason=f"dataset not allowed on {DEVICE_ROLE}")
        return

    await websocket.accept()
    bridge_node.register_telemetry_client()
    try:
        while True:
            payload = await _active_dataset_payload()
            await websocket.send_json(payload)
            await asyncio.sleep(0.2)
    except WebSocketDisconnect:
        pass
    finally:
        bridge_node.unregister_telemetry_client()


@app.websocket("/api/ws/control")
async def control_ws(websocket: WebSocket) -> None:
    if bridge_node is None:
        await websocket.close(code=1011)
        return
    if "teleop" not in ALLOWED_ACTIONS:
        await websocket.close(code=1008, reason=f"teleop not allowed on {DEVICE_ROLE}")
        return

    await websocket.accept()
    try:
        while True:
            payload = await websocket.receive_json()
            command = VelocityCommand(**payload)
            bridge_node.publish_cmd_vel(command.linear_x, command.linear_y, command.angular_z)
            await websocket.send_json({"success": True})
    except WebSocketDisconnect:
        bridge_node.publish_cmd_vel(0.0, 0.0, 0.0)


@app.post("/api/robot/cmd_vel")
async def publish_cmd_vel(command: VelocityCommand) -> dict:
    _require_action("teleop")
    if bridge_node is None:
        raise HTTPException(status_code=503, detail="ROS2 bridge is not ready")
    bridge_node.publish_cmd_vel(command.linear_x, command.linear_y, command.angular_z)
    return {"success": True}


@app.post("/api/robot/nav/goal")
async def send_nav_goal(goal: NavGoalRequest) -> dict:
    if bridge_node is None:
        raise HTTPException(status_code=503, detail="ROS2 bridge is not ready")
    success = bridge_node.send_cca_nmpc_goal(goal.x, goal.y, goal.yaw)
    if not success:
        raise HTTPException(status_code=503, detail="RAI navigation goal publisher is not available")
    return {"success": True, "controller": "CCA_NMPC", "message": "RAI navigation goal published."}


@app.post("/api/robot/nav/route")
async def send_nav_route(route: RoutePlanRequest) -> dict:
    if bridge_node is None:
        raise HTTPException(status_code=503, detail="ROS2 bridge is not ready")
    success = bridge_node.send_cca_nmpc_route(
        {"x": route.start.x, "y": route.start.y, "yaw": route.start.yaw},
        {"x": route.goal.x, "y": route.goal.y, "yaw": route.goal.yaw},
        route.start_tolerance,
    )
    if not success:
        raise HTTPException(status_code=503, detail="RAI navigation route publisher is not available")
    return {
        "success": True,
        "message": "Route dispatched. Robot will go to start first if needed, then continue to goal.",
    }


@app.post("/api/robot/nav/cancel")
async def cancel_nav_goal() -> dict:
    if bridge_node is None:
        raise HTTPException(status_code=503, detail="ROS2 bridge is not ready")
    bridge_node.cancel_nav_goal()
    bridge_node.publish_cmd_vel(0.0, 0.0, 0.0)
    return {"success": True}


@app.get("/api/robot/anchors")
async def get_robot_anchors() -> dict:
    if bridge_node is None:
        raise HTTPException(status_code=503, detail="ROS2 bridge is not ready")
    return bridge_node.get_anchor_state()


@app.post("/api/robot/initial_pose")
async def set_initial_pose(request: InitialPoseRequest) -> dict:
    if bridge_node is None:
        raise HTTPException(status_code=503, detail="ROS2 bridge is not ready")
    pose = bridge_node.publish_initial_pose(request.x, request.y, request.yaw, set_home=request.set_home)
    anchors = bridge_node.get_anchor_state()
    return {
        "success": True,
        "message": "Initial pose published.",
        "initial_pose": pose,
        "home_pose": anchors.get("home_pose"),
    }


@app.post("/api/robot/home")
async def set_home_pose(request: PoseRequest) -> dict:
    if bridge_node is None:
        raise HTTPException(status_code=503, detail="ROS2 bridge is not ready")
    pose = bridge_node.set_home_pose(request.x, request.y, request.yaw)
    return {"success": True, "message": "Home pose updated.", "home_pose": pose}


@app.post("/api/robot/nav/home")
async def send_home_goal() -> dict:
    if bridge_node is None:
        raise HTTPException(status_code=503, detail="ROS2 bridge is not ready")
    anchors = bridge_node.get_anchor_state()
    home_pose = anchors.get("home_pose")
    if home_pose is None:
        raise HTTPException(status_code=503, detail="Home pose is not set")
    bridge_node.send_cca_nmpc_goal(home_pose["x"], home_pose["y"], home_pose.get("yaw", 0.0))
    return {"success": True, "message": "RAI navigation home goal published."}


@app.get("/api/rai-navigation/options")
@app.get("/api/nav2/options")
async def rai_navigation_options() -> dict:
    _require_action("navigation")
    return {
        "local_planners": RAI_CONTROLLER_OPTIONS,
        "global_planners": RAI_GLOBAL_PLANNER_OPTIONS,
    }


@app.get("/api/rai-navigation/config")
@app.get("/api/nav2/config")
async def rai_navigation_config() -> dict:
    _require_action("navigation")
    return {
        **rai_navigation_runtime_config,
        "running": rai_navigation_process is not None and rai_navigation_process.poll() is None,
        "controller_server_enabled": True,
        "rai_controller_running": rai_navigation_process is not None and rai_navigation_process.poll() is None,
    }


@app.post("/api/rai-navigation/config")
@app.post("/api/nav2/config")
async def set_rai_navigation_config(request: RaiNavigationConfigRequest, db: AsyncSession = Depends(get_db)) -> dict:
    _require_action("navigation")
    global rai_navigation_process
    local_planner = request.local_planner.upper()
    global_planner = request.global_planner.upper()
    if local_planner not in {item["id"] for item in RAI_CONTROLLER_OPTIONS}:
        raise HTTPException(status_code=400, detail=f"Unsupported local planner: {local_planner}")
    if global_planner not in {item["id"] for item in RAI_GLOBAL_PLANNER_OPTIONS}:
        raise HTTPException(status_code=400, detail=f"Unsupported global planner: {global_planner}")

    if request.map_id is not None:
        saved_map = await db.get(SavedMap, request.map_id)
        if saved_map is None:
            raise HTTPException(status_code=404, detail="Saved map not found")
        if not saved_map.yaml_path:
            raise HTTPException(status_code=400, detail="Saved map does not have an exported YAML path")
        rai_navigation_runtime_config["map_path"] = saved_map.yaml_path
        rai_navigation_runtime_config["selected_map_id"] = saved_map.id

    if request.map_path:
        rai_navigation_runtime_config["map_path"] = request.map_path
        rai_navigation_runtime_config["selected_map_id"] = None
    if request.params_path:
        rai_navigation_runtime_config["params_path"] = request.params_path
    rai_navigation_runtime_config["local_planner"] = local_planner
    rai_navigation_runtime_config["global_planner"] = global_planner

    if rai_navigation_process is not None and rai_navigation_process.poll() is None:
        _stop_stack_process(rai_navigation_process, RAI_NAVIGATION_KILL_PATTERNS)
        rai_navigation_process = None
        return await start_rai_navigation_stack(db)
    return await rai_navigation_config()


@app.post("/api/rai-navigation/start")
@app.post("/api/nav2/start")
async def start_rai_navigation_stack(db: AsyncSession = Depends(get_db)) -> dict:
    _require_action("navigation")
    global rai_navigation_process
    if rai_navigation_process is not None and rai_navigation_process.poll() is None:
        return {"success": True, "message": "RAI navigation is already running", **(await rai_navigation_config())}

    if not rai_navigation_runtime_config["map_path"]:
        raise HTTPException(status_code=400, detail="No navigation map selected. Choose a saved map before starting RAI navigation.")

    map_path = Path(rai_navigation_runtime_config["map_path"])
    if not map_path.exists():
        raise HTTPException(status_code=404, detail=f"Selected navigation map file not found: {map_path}")

    base_params_path = Path(rai_navigation_runtime_config["params_path"] or _default_rai_navigation_params_path())
    if not base_params_path.exists():
        raise HTTPException(status_code=404, detail=f"RAI navigation params file not found: {base_params_path}")

    runtime_params_path = _runtime_rai_navigation_params_path(
        rai_navigation_runtime_config["local_planner"],
        rai_navigation_runtime_config["global_planner"],
        base_params_path,
    )
    command = _rai_navigation_launch_command(
        map_path,
        runtime_params_path,
        rai_navigation_runtime_config["local_planner"],
        rai_navigation_runtime_config["global_planner"],
    )
    rai_navigation_process = _start_process(command, env=_ros_runtime_env())
    rai_navigation_runtime_config["last_command"] = command
    anchor = None
    if bridge_node is not None:
        with bridge_node.lock:
            bridge_node.telemetry["context"]["navigation_mode"] = "rai_navigation"
        anchor = bridge_node.get_anchor_state().get("initial_pose")
        if anchor is None:
            anchor = bridge_node.capture_current_pose_as_anchor(prefer_map=False, set_home=False)
        if anchor is not None:
            _publish_anchor_pose_after_delay()
    return {
        "success": True,
        "pid": rai_navigation_process.pid,
        "command": command,
        "initial_pose": anchor,
        **(await rai_navigation_config()),
    }


@app.post("/api/rai-navigation/stop")
@app.post("/api/nav2/stop")
async def stop_rai_navigation_stack() -> dict:
    _require_action("navigation")
    global rai_navigation_process
    result = _stop_stack_process(rai_navigation_process, RAI_NAVIGATION_KILL_PATTERNS)
    rai_navigation_process = None
    if bridge_node is not None:
        with bridge_node.lock:
            bridge_node.telemetry["context"]["navigation_mode"] = "idle"
    return {"success": True, **result, **(await rai_navigation_config())}


@app.post("/api/robot/slam/start")
async def start_slam() -> dict:
    _require_action("slam")
    global slam_process
    if slam_process is not None and slam_process.poll() is None:
        return {"success": True, "message": "SLAM is already running"}
    command = "ros2 launch rai_slam_toolbox online_async_launch.py"
    slam_process = _start_process(command, env=_ros_runtime_env())
    anchor = None
    if bridge_node is not None:
        with bridge_node.lock:
            bridge_node.telemetry["context"]["navigation_mode"] = "slam"
        anchor = bridge_node.capture_current_pose_as_anchor(prefer_map=False, set_home=True)
    return {
        "success": True,
        "message": "SLAM launch started",
        "pid": slam_process.pid,
        "command": command,
        "initial_pose": anchor,
    }


@app.post("/api/robot/slam/stop")
async def stop_slam() -> dict:
    _require_action("slam")
    global slam_process
    result = _stop_stack_process(slam_process, SLAM_KILL_PATTERNS)
    slam_process = None
    if bridge_node is not None:
        with bridge_node.lock:
            bridge_node.telemetry["context"]["navigation_mode"] = "idle"
    return {"success": True, "message": "SLAM stopped", **result}


@app.post("/api/map/save")
async def save_map(request: SaveMapRequest, db: AsyncSession = Depends(get_db)) -> dict:
    _require_action("maps")
    if bridge_node is None:
        raise HTTPException(status_code=503, detail="ROS2 bridge is not ready")

    # Đảm bảo đã subscribe topic /map
    bridge_node.ensure_map_subscription()

    # Chờ bản đồ được cập nhật từ topic (nếu mới subscribe)
    max_wait = 15
    current_map = None
    for _ in range(max_wait):
        with bridge_node.lock:
            current_map = bridge_node.latest_map
        if current_map is not None:
            break
        await asyncio.sleep(0.2)

    if current_map is None:
        raise HTTPException(
            status_code=404, 
            detail="No map data available from /map topic. Make sure SLAM or map_server is active."
        )

    # Lưu bản đồ vào cơ sở dữ liệu
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


@app.post("/api/train/start")
async def start_training(request: TrainStartRequest, background_tasks: BackgroundTasks) -> dict:
    global training_state
    if training_state["running"]:
        raise HTTPException(status_code=409, detail="Training is already running")

    training_state["running"] = True
    training_state["progress"] = 0
    training_state["epoch"] = 0
    training_state["loss"] = 0.95
    training_state["accuracy"] = 0.35
    training_state["history"] = []

    background_tasks.add_task(_simulate_training_job, request.epochs)
    return {
        "success": True,
        "message": "Training job started successfully in background",
        "config": {
            "epochs": request.epochs,
            "learning_rate": request.learning_rate,
            "batch_size": request.batch_size,
            "architecture": request.architecture
        }
    }


@app.get("/api/train/status")
async def get_training_status() -> dict:
    return training_state


async def _simulate_training_job(epochs: int) -> None:
    global training_state
    import random
    loss = 0.95
    accuracy = 0.35
    for epoch in range(1, epochs + 1):
        if not training_state["running"]:
            break
        
        # Mô phỏng thời gian huấn luyện 1 epoch
        await asyncio.sleep(0.4)
        
        loss = max(0.02, loss - random.uniform(0.015, 0.045))
        accuracy = min(0.995, accuracy + random.uniform(0.01, 0.035))
        progress = int((epoch / epochs) * 100)
        
        training_state.update({
            "progress": progress,
            "epoch": epoch,
            "loss": round(loss, 4),
            "accuracy": round(accuracy, 4)
        })
        training_state["history"].append({
            "epoch": epoch,
            "loss": round(loss, 4),
            "accuracy": round(accuracy, 4)
        })
        
    training_state["running"] = False


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
    global active_dataset_run_id, dataset_bag_process
    if active_dataset_run_id is not None:
        raise HTTPException(status_code=409, detail="A dataset run is already active")
    if dataset_bag_process is not None and dataset_bag_process.poll() is None:
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

    voltage = None
    if bridge_node is not None:
        with bridge_node.lock:
            voltage = bridge_node.telemetry["battery"].get("voltage")
            telemetry_snapshot = json.loads(json.dumps(bridge_node.telemetry))
    else:
        telemetry_snapshot = {}

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
    try:
        dataset_bag_process = _start_rosbag_record(
            rosbag_path,
            rosbag_log_path,
            record_camera=request.record_camera,
        )
        initial_metadata["rosbag_pid"] = dataset_bag_process.pid
        initial_metadata["rosbag_log_path"] = str(rosbag_log_path)
        metadata_path.write_text(json.dumps(initial_metadata, indent=2), encoding="utf-8")
        await db.commit()
        await db.refresh(run)
        active_dataset_run_id = run.id
    except Exception as exc:
        await db.rollback()
        shutil.rmtree(run_path, ignore_errors=True)
        dataset_bag_process = None
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
        "rosbag_pid": dataset_bag_process.pid if dataset_bag_process else None,
        "rosbag_log_path": str(rosbag_log_path),
        "metadata_path": str(metadata_path),
        "run_index_path": str(DATASET_BASE_PATH / "metadata" / "run_index.csv"),
        "recorded_topics": DATASET_REQUIRED_TOPICS + (DATASET_CAMERA_TOPICS if request.record_camera else []),
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
    global active_dataset_run_id, dataset_bag_process
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
    rosbag_status = _stop_rosbag_record(dataset_bag_process)
    dataset_bag_process = None
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

    if bridge_node is not None:
        with bridge_node.lock:
            battery = bridge_node.telemetry["battery"]
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
        "derived",
        "metadata/calibration",
        "metadata/splits",
        "figures/system",
        "figures/trajectories",
        "figures/timeseries",
        "figures/boxplots",
        "figures/latency",
        "figures/ablation",
        "tables/csv",
        "tables/latex",
    ]
    for relative in directories:
        (DATASET_BASE_PATH / relative).mkdir(parents=True, exist_ok=True)

    _ensure_csv_header(DATASET_BASE_PATH / "metadata" / "run_index.csv", RUN_INDEX_FIELDS)
    _ensure_csv_header(DATASET_BASE_PATH / "derived" / "controller_timeseries.csv", [
        "timestamp", "run_id", "scenario_id", "controller_id", "x_r", "y_r", "theta_r",
        "x_ref", "y_ref", "theta_ref", "tracking_error_xy", "tracking_error_theta",
        "vx_cmd", "vy_cmd", "omega_cmd", "vx_odom", "vy_odom", "omega_odom", "d_h",
        "phi_h", "d_safe", "vx_max_adaptive", "vy_max_adaptive", "omega_max_adaptive",
        "q_x_adaptive", "q_y_adaptive", "q_theta_adaptive", "sample_count",
        "evaluation_time_ms", "controller_status", "timeout_flag", "collision_flag",
        "occlusion_flag",
    ])
    _ensure_csv_header(DATASET_BASE_PATH / "derived" / "human_states.csv", [
        "timestamp", "run_id", "scenario_id", "human_id", "x_h", "y_h", "vx_h", "vy_h",
        "confidence", "age_sec", "cov_x", "cov_y", "cov_vx", "cov_vy", "x_h_gt",
        "y_h_gt", "vx_h_gt", "vy_h_gt",
    ])
    _ensure_csv_header(DATASET_BASE_PATH / "derived" / "human_prediction.csv", [
        "timestamp", "run_id", "scenario_id", "human_id", "horizon_i", "x_h_pred",
        "y_h_pred", "x_h_gt", "y_h_gt", "prediction_error",
    ])
    _ensure_csv_header(DATASET_BASE_PATH / "derived" / "adaptation_timeseries.csv", [
        "timestamp", "run_id", "scenario_id", "controller_id", "d_h", "phi_h", "d_safe",
        "vx_max_adaptive", "vy_max_adaptive", "omega_max_adaptive", "q_x_adaptive",
        "q_y_adaptive", "q_theta_adaptive", "vx_cmd", "vy_cmd", "omega_cmd",
        "occlusion_flag",
    ])
    _ensure_csv_header(DATASET_BASE_PATH / "derived" / "metrics_per_run.csv", [
        "run_id", "scenario_id", "controller_id", "environment", "success", "collision",
        "timeout", "duration_sec", "rmse_xy", "rmse_theta", "max_lateral_error", "d_min",
        "d_avg", "d_5percentile", "violation_count", "violation_duration",
        "collision_count", "jerk_mean", "jerk_max", "mean_abs_delta_u", "max_abs_delta_u",
        "control_effort", "mean_abs_vx", "mean_abs_vy", "mean_abs_omega",
        "solve_time_mean_ms", "solve_time_median_ms", "solve_time_p95_ms",
        "solve_time_max_ms", "timeout_rate",
    ])
    _ensure_csv_header(DATASET_BASE_PATH / "derived" / "metrics_summary.csv", [
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
            "controllers": [
                {"id": "CCA_NMPC", "type": "sampling_based_cca_predictive_control"},
                {"id": "DWB", "type": "nav2_baseline"},
                {"id": "MPPI", "type": "nav2_baseline"},
            ],
        },
        "metadata/scenarios.yaml": {
            "scenarios": [
                {"id": "S1_open_zone", "description": "Open zone with sparse human interaction"},
                {"id": "S2_crossing_human", "description": "Single human crossing the robot path"},
                {"id": "S3_corridor", "description": "Constrained corridor navigation"},
                {"id": "S4_occlusion", "description": "Human detection with partial occlusion"},
                {"id": "S5_dense_dynamic", "description": "Multiple dynamic humans"},
            ],
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
        "metadata/scenarios.yaml",
        "derived/controller_timeseries.csv",
        "derived/human_states.csv",
        "derived/human_prediction.csv",
        "derived/metrics_per_run.csv",
        "derived/metrics_summary.csv",
        "derived/adaptation_timeseries.csv",
    ]
    return {
        "base_path": str(DATASET_BASE_PATH),
        "required_topics": DATASET_REQUIRED_TOPICS,
        "optional_camera_topics": DATASET_CAMERA_TOPICS,
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


def _start_rosbag_record(rosbag_path: Path, log_path: Path, record_camera: bool) -> subprocess.Popen:
    topics = DATASET_REQUIRED_TOPICS + (DATASET_CAMERA_TOPICS if record_camera else [])
    rosbag_path.parent.mkdir(parents=True, exist_ok=True)
    command = ["ros2", "bag", "record", "-o", str(rosbag_path), *topics]
    kwargs = {}
    if hasattr(os, "setsid"):
        kwargs["preexec_fn"] = os.setsid
    log_handle = log_path.open("a", encoding="utf-8")
    log_handle.write(f"$ {' '.join(command)}\n")
    log_handle.flush()
    try:
        return subprocess.Popen(
            command,
            stdout=log_handle,
            stderr=subprocess.STDOUT,
            env=_ros_runtime_env(),
            **kwargs,
        )
    except Exception:
        log_handle.close()
        raise


def _stop_rosbag_record(process: Optional[subprocess.Popen]) -> dict:
    if process is None:
        return {"status": "stopped", "message": "no rosbag process"}
    if process.poll() is not None:
        return {"status": "stopped", "returncode": process.returncode}
    try:
        if hasattr(os, "killpg"):
            os.killpg(os.getpgid(process.pid), signal.SIGINT)
        else:
            process.send_signal(signal.SIGINT)
        process.wait(timeout=10.0)
        return {"status": "stopped", "returncode": process.returncode}
    except subprocess.TimeoutExpired:
        if hasattr(os, "killpg"):
            os.killpg(os.getpgid(process.pid), signal.SIGTERM)
        else:
            process.terminate()
        try:
            process.wait(timeout=5.0)
        except subprocess.TimeoutExpired:
            if hasattr(os, "killpg"):
                os.killpg(os.getpgid(process.pid), signal.SIGKILL)
            else:
                process.kill()
            process.wait(timeout=2.0)
        return {"status": "forced_stop", "returncode": process.returncode}


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
            cwd="/home/rai/ijat2026",
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

    scenario = DatasetScenario(
        name=scenario_name,
        context_type="CONTINUOUS",
        difficulty="manual",
        human_mode="manual",
        expected_runs=0,
        description="Manual/no-map CCA-NMPC dataset collection scenario",
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
    telemetry = {}
    if bridge_node is not None:
        bridge_node.ensure_telemetry_subscriptions()
        with bridge_node.lock:
            telemetry = json.loads(json.dumps(bridge_node.telemetry))

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
    uvicorn.run(app, host=DEFAULT_HOST, port=DEFAULT_PORT, log_level="info")


if __name__ == "__main__":
    main()
