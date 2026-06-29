import asyncio
import os
import signal
import subprocess
import json
import threading
import time
from pathlib import Path
from typing import Optional

import uvicorn
from aiortc import RTCPeerConnection, RTCSessionDescription
from fastapi import FastAPI, HTTPException, WebSocket, WebSocketDisconnect
from pydantic import BaseModel, Field
import rclpy
from rai_runtime_bridge.node import WebBridgeNode
from rai_runtime_bridge.webrtc import RosImageVideoTrack


DEFAULT_HOST = os.getenv("RAI_BRIDGE_HOST", "0.0.0.0")
DEFAULT_PORT = int(os.getenv("RAI_BRIDGE_PORT", "8090"))
DEVICE_ROLE = os.getenv("RAI_DEVICE_ROLE", "pi").strip().lower()
DEVICE_LABEL = os.getenv("RAI_DEVICE_LABEL", DEVICE_ROLE or "unknown")

app = FastAPI(title="RAI Runtime Bridge", version="1.0.0")
control_node: Optional[WebBridgeNode] = None
spin_thread: Optional[threading.Thread] = None
peer_connections: set[RTCPeerConnection] = set()

robot_base_process: Optional[subprocess.Popen] = None
lidar_process: Optional[subprocess.Popen] = None
camera_process: Optional[subprocess.Popen] = None
slam_process: Optional[subprocess.Popen] = None
navigation_process: Optional[subprocess.Popen] = None
dataset_process: Optional[subprocess.Popen] = None
dataset_bag_process: Optional[subprocess.Popen] = None

camera_depth_enabled = os.getenv("RAI_CAMERA_ENABLE_DEPTH", "false").strip().lower() == "true"
navigation_config = {
    "local_planner": os.getenv("RAI_NAVIGATION_CONTROLLER", "CCA_NMPC").upper(),
    "global_planner": os.getenv("RAI_NAVIGATION_GLOBAL_PLANNER", "A_STAR").upper(),
    "map_path": os.getenv("RAI_NAVIGATION_MAP", ""),
    "params_path": os.getenv("RAI_NAVIGATION_PARAMS", ""),
}

ROLE_ALLOWED_ACTIONS = {
    "pi": {"hardware", "lidar", "slam", "navigation", "dataset", "system"},
    "jetson": {"camera", "perception", "system"},
    "laptop": {"simulation", "system"},
    "hub": {"simulation", "system"},
    "sim": {"simulation", "slam", "navigation", "system"},
}
ALLOWED_ACTIONS = ROLE_ALLOWED_ACTIONS.get(DEVICE_ROLE, {"system"})

KILL_PATTERNS = {
    "robot_base": (
        "ros2 launch turn_on_rai_robot turn_on_rai_robot.launch.py",
        "rai_robot_node",
        "twist_mux",
        "robot_state_publisher",
    ),
    "lidar": (
        "ros2 launch turn_on_rai_robot rai_lidar.launch.py",
        "lslidar_driver_node",
        "lsn10p_launch.py",
    ),
    "camera": (
        "ros2 launch turn_on_rai_robot rai_camera.launch.py",
        "astra_camera_node",
        "astra.launch.xml",
    ),
    "slam": (
        "ros2 launch rai_slam_toolbox online_async_launch.py",
        "async_slam_toolbox_node",
        "__node:=slam_toolbox",
    ),
    "navigation": (
        "ros2 launch rai_navigation rai_navigation.launch.py",
        "rai_controller_server",
        "__node:=rai_controller",
    ),
    "dataset": (
        "ros2 launch rai_dataset_collection dataset_collection.launch.py",
        "dataset_collector",
        "context_monitor",
    ),
}


class CameraStartRequest(BaseModel):
    enable_depth: bool = False


class DatasetLaunchRequest(BaseModel):
    scenario_name: str = Field(default="S1_open_zone")
    controller_id: str = Field(default="CCA_NMPC")
    environment: str = Field(default="real", pattern="^(sim|real)$")
    split: str = Field(default="unsplit")
    run_id: str = Field(default="")
    auto_start: bool = True


class DatasetRecordRequest(BaseModel):
    rosbag_path: str = Field(..., min_length=1)
    log_path: str = Field(..., min_length=1)
    topics: list[str] = Field(..., min_length=1)


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


class RaiNavigationConfigRequest(BaseModel):
    local_planner: str = Field(default="CCA_NMPC")
    global_planner: str = Field(default="A_STAR")
    map_path: Optional[str] = None
    map_id: Optional[int] = None
    params_path: Optional[str] = None


class SystemOperationModeRequest(BaseModel):
    mode: str = Field(default="real", pattern="^(real|sim|hybrid)$")


def _require_action(action: str) -> None:
    if action in ALLOWED_ACTIONS:
        return
    raise HTTPException(
        status_code=403,
        detail=f"Action '{action}' is not allowed on runtime bridge role '{DEVICE_ROLE}'.",
    )


def _require_control_node() -> WebBridgeNode:
    if control_node is None:
        raise HTTPException(status_code=503, detail="Runtime control node is not ready")
    return control_node


def _spin_control_node() -> None:
    if control_node is not None:
        rclpy.spin(control_node)


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


def _start_process(command: str) -> subprocess.Popen:
    kwargs = {"env": _ros_runtime_env()}
    if hasattr(os, "setsid"):
        kwargs["preexec_fn"] = os.setsid
    return subprocess.Popen(command, shell=True, **kwargs)


def _stop_process(process: Optional[subprocess.Popen]) -> dict:
    if process is None or process.poll() is not None:
        return {"status": "stopped", "message": "not running"}
    try:
        if hasattr(os, "killpg"):
            os.killpg(os.getpgid(process.pid), signal.SIGINT)
        else:
            process.send_signal(signal.SIGINT)
        process.wait(timeout=8.0)
        return {"status": "stopped", "returncode": process.returncode}
    except subprocess.TimeoutExpired:
        process.terminate()
        try:
            process.wait(timeout=3.0)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=2.0)
        return {"status": "forced_stop", "returncode": process.returncode}


def _start_rosbag_record(rosbag_path: Path, log_path: Path, topics: list[str]) -> subprocess.Popen:
    cleaned_topics = [topic for topic in topics if topic.startswith("/")]
    if not cleaned_topics:
        raise HTTPException(status_code=400, detail="At least one absolute ROS topic is required")

    rosbag_path.parent.mkdir(parents=True, exist_ok=True)
    log_path.parent.mkdir(parents=True, exist_ok=True)
    command = ["ros2", "bag", "record", "-o", str(rosbag_path), *cleaned_topics]
    kwargs = {"env": _ros_runtime_env()}
    if hasattr(os, "setsid"):
        kwargs["preexec_fn"] = os.setsid

    with log_path.open("a", encoding="utf-8") as log_handle:
        log_handle.write(f"$ {' '.join(command)}\n")
    with log_path.open("ab", buffering=0) as log_handle:
        return subprocess.Popen(
            command,
            stdout=log_handle,
            stderr=subprocess.STDOUT,
            **kwargs,
        )


def _stop_rosbag_record(process: Optional[subprocess.Popen]) -> dict:
    return _stop_process(process)


def _find_pids_by_patterns(patterns: tuple[str, ...]) -> list[int]:
    proc_root = Path("/proc")
    if not proc_root.exists():
        return []
    matched = []
    current_pid = os.getpid()
    for proc_dir in proc_root.iterdir():
        if not proc_dir.name.isdigit():
            continue
        pid = int(proc_dir.name)
        if pid == current_pid:
            continue
        try:
            cmdline = (proc_dir / "cmdline").read_bytes().replace(b"\x00", b" ").decode("utf-8", errors="ignore")
        except OSError:
            continue
        if any(pattern in cmdline for pattern in patterns):
            matched.append(pid)
    return sorted(set(matched))


def _kill_matching_processes(patterns: tuple[str, ...]) -> list[int]:
    pids = _find_pids_by_patterns(patterns)
    for pid in pids:
        try:
            os.kill(pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
    deadline = time.time() + 3.0
    while time.time() < deadline:
        remaining = [pid for pid in pids if Path(f"/proc/{pid}").exists()]
        if not remaining:
            return pids
        time.sleep(0.1)
    for pid in pids:
        if Path(f"/proc/{pid}").exists():
            try:
                os.kill(pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
    return pids


def _stop_stack(process: Optional[subprocess.Popen], component_id: str) -> dict:
    result = _stop_process(process)
    killed_pids = _kill_matching_processes(KILL_PATTERNS.get(component_id, ()))
    if killed_pids:
        result["killed_pids"] = killed_pids
    return result


def _component_state(component_id: str, label: str, host_device: str, action: str, process: Optional[subprocess.Popen], launch_file: str, description: str, capabilities: Optional[dict] = None) -> dict:
    running = process is not None and process.poll() is None
    return {
        "id": component_id,
        "label": label,
        "host_device": host_device,
        "action": action,
        "allowed_here": action in ALLOWED_ACTIONS,
        "running": running,
        "pid": process.pid if running and process is not None else None,
        "launch_file": launch_file,
        "description": description,
        "capabilities": capabilities or {},
    }


def _components() -> list[dict]:
    return [
        _component_state("robot_base", "Robot Base", "pi", "hardware", robot_base_process, "turn_on_rai_robot.launch.py", "Base serial, IMU filter, EKF, TF, joint states, and twist mux."),
        _component_state("lidar", "LiDAR", "pi", "lidar", lidar_process, "rai_lidar.launch.py", "Independent LSLiDAR bringup."),
        _component_state("camera", "Camera", "jetson", "camera", camera_process, "rai_camera.launch.py", "Astra RGB-D camera bringup.", {"enable_depth_toggle": True, "enable_depth": camera_depth_enabled}),
        _component_state("slam", "SLAM", "pi", "slam", slam_process, "online_async_launch.py", "SLAM Toolbox runtime."),
        _component_state("navigation", "Navigation", "pi", "navigation", navigation_process, "rai_navigation.launch.py", "RAI navigation runtime."),
        _component_state("dataset", "Dataset", "pi", "dataset", dataset_process, "dataset_collection.launch.py", "Dataset collection launch stack."),
    ]


def _component_by_id(component_id: str) -> dict:
    for component in _components():
        if component["id"] == component_id:
            return component
    raise HTTPException(status_code=404, detail=f"Unknown component: {component_id}")


def _dataset_command(request: DatasetLaunchRequest) -> str:
    args = [
        "ros2",
        "launch",
        "rai_dataset_collection",
        "dataset_collection.launch.py",
        f"scenario:={request.scenario_name}",
        f"controller:={request.controller_id}",
        f"environment:={request.environment}",
        f"split:={request.split}",
        f"run_id:={request.run_id}",
        f"auto_start:={str(request.auto_start).lower()}",
    ]
    return " ".join(args)


@app.get("/health")
@app.get("/api/health")
async def health() -> dict:
    return {"status": "ok", "device_role": DEVICE_ROLE, "device_label": DEVICE_LABEL}


@app.get("/api/system/runtime")
async def runtime() -> dict:
    return {
        "device_role": DEVICE_ROLE,
        "device_label": DEVICE_LABEL,
        "allowed_actions": sorted(ALLOWED_ACTIONS),
        "bridge_host": DEFAULT_HOST,
        "bridge_port": DEFAULT_PORT,
        "operation_mode": os.getenv("RAI_OPERATION_MODE", "real"),
    }


@app.post("/api/system/operation-mode")
async def set_operation_mode(request: SystemOperationModeRequest) -> dict:
    os.environ["RAI_OPERATION_MODE"] = request.mode
    return {"success": True, **(await runtime())}


@app.get("/api/system/components")
async def components() -> dict:
    return {**(await runtime()), "components": _components()}


@app.get("/api/rviz/topics")
async def rviz_topics() -> dict:
    return _require_control_node().get_visual_topics()


@app.post("/api/rviz/topics")
async def set_rviz_topics(request: dict) -> dict:
    node = _require_control_node()
    return node.configure_visual_topics(
        str(request.get("map_topic", "/map")),
        str(request.get("global_path_topic", "/rai_navigation/global_path")),
        str(request.get("local_path_topic", "/canmpc/predicted_trajectory")),
    )


@app.get("/api/telemetry/current")
async def telemetry_current() -> dict:
    node = _require_control_node()
    node.ensure_telemetry_subscriptions()
    with node.lock:
        return {"timestamp": time.time(), "telemetry": json.loads(json.dumps(node.telemetry))}


@app.get("/api/map/snapshot")
async def map_snapshot() -> dict:
    node = _require_control_node()
    node.ensure_map_subscription()
    deadline = time.time() + 3.0
    while time.time() < deadline:
        with node.lock:
            latest_map = json.loads(json.dumps(node.latest_map)) if node.latest_map is not None else None
        if latest_map is not None:
            return {"available": True, "map": latest_map}
        await asyncio.sleep(0.1)
    return {"available": False, "map": None}


@app.post("/api/system/components/robot/start")
async def start_robot() -> dict:
    _require_action("hardware")
    return _start_component("robot_base", "ros2 launch turn_on_rai_robot turn_on_rai_robot.launch.py")


@app.post("/api/system/components/robot/stop")
async def stop_robot() -> dict:
    return _stop_component("robot_base")


@app.post("/api/system/components/lidar/start")
async def start_lidar() -> dict:
    _require_action("lidar")
    return _start_component("lidar", "ros2 launch turn_on_rai_robot rai_lidar.launch.py")


@app.post("/api/system/components/lidar/stop")
async def stop_lidar() -> dict:
    return _stop_component("lidar")


@app.post("/api/system/components/camera/start")
async def start_camera(request: CameraStartRequest) -> dict:
    _require_action("camera")
    global camera_depth_enabled
    camera_depth_enabled = request.enable_depth
    enable_depth_arg = "true" if request.enable_depth else "false"
    return _start_component("camera", f"ros2 launch turn_on_rai_robot rai_camera.launch.py enable_depth:={enable_depth_arg}")


@app.post("/api/system/components/camera/stop")
async def stop_camera() -> dict:
    return _stop_component("camera")


@app.post("/api/system/components/slam/start")
@app.post("/api/robot/slam/start")
async def start_slam() -> dict:
    _require_action("slam")
    return _start_component("slam", "ros2 launch rai_slam_toolbox online_async_launch.py")


@app.post("/api/system/components/slam/stop")
@app.post("/api/robot/slam/stop")
async def stop_slam() -> dict:
    return _stop_component("slam")


@app.post("/api/system/components/navigation/start")
@app.post("/api/rai-navigation/start")
async def start_navigation() -> dict:
    _require_action("navigation")
    args = ["ros2 launch rai_navigation rai_navigation.launch.py"]
    if navigation_config["params_path"]:
        args.append(f"params:={navigation_config['params_path']}")
    args.extend([
        f"controller_id:={navigation_config['local_planner']}",
        f"global_planner_algorithm:={navigation_config['global_planner']}",
    ])
    if navigation_config["map_path"]:
        args.append(f"map:={navigation_config['map_path']}")
    args.append("map_topic:=/map")
    return _start_component("navigation", " ".join(args))


@app.post("/api/system/components/navigation/stop")
@app.post("/api/rai-navigation/stop")
async def stop_navigation() -> dict:
    return _stop_component("navigation")


@app.get("/api/dataset/launch/status")
async def dataset_status() -> dict:
    running = dataset_process is not None and dataset_process.poll() is None
    return {"running": running, "pid": dataset_process.pid if running and dataset_process else None}


@app.get("/api/dataset/record/status")
async def dataset_record_status() -> dict:
    running = dataset_bag_process is not None and dataset_bag_process.poll() is None
    return {"running": running, "pid": dataset_bag_process.pid if running and dataset_bag_process else None}


@app.post("/api/dataset/record/start")
async def start_dataset_record(request: DatasetRecordRequest) -> dict:
    _require_action("dataset")
    global dataset_bag_process
    if dataset_bag_process is not None and dataset_bag_process.poll() is None:
        raise HTTPException(status_code=409, detail="ros2 bag record is already running")

    rosbag_path = Path(request.rosbag_path).expanduser()
    log_path = Path(request.log_path).expanduser()
    dataset_bag_process = _start_rosbag_record(rosbag_path, log_path, request.topics)
    return {
        "success": True,
        "pid": dataset_bag_process.pid,
        "rosbag_path": str(rosbag_path),
        "log_path": str(log_path),
        "topics": request.topics,
    }


@app.post("/api/dataset/record/stop")
async def stop_dataset_record() -> dict:
    _require_action("dataset")
    global dataset_bag_process
    result = _stop_rosbag_record(dataset_bag_process)
    dataset_bag_process = None
    return {"success": True, **result}


@app.post("/api/system/components/dataset/start")
async def start_system_dataset() -> dict:
    return await start_dataset(DatasetLaunchRequest())


@app.post("/api/dataset/launch/start")
async def start_dataset(request: DatasetLaunchRequest) -> dict:
    _require_action("dataset")
    return _start_component("dataset", _dataset_command(request))


@app.post("/api/system/components/dataset/stop")
@app.post("/api/dataset/launch/stop")
async def stop_dataset() -> dict:
    return _stop_component("dataset")


def _start_component(component_id: str, command: str) -> dict:
    global robot_base_process, lidar_process, camera_process, slam_process, navigation_process, dataset_process
    process = {
        "robot_base": robot_base_process,
        "lidar": lidar_process,
        "camera": camera_process,
        "slam": slam_process,
        "navigation": navigation_process,
        "dataset": dataset_process,
    }[component_id]
    if process is not None and process.poll() is None:
        return {"success": True, "message": f"{component_id} is already running.", "component": _component_by_id(component_id)}
    process = _start_process(command)
    if component_id == "robot_base":
        robot_base_process = process
    elif component_id == "lidar":
        lidar_process = process
    elif component_id == "camera":
        camera_process = process
    elif component_id == "slam":
        slam_process = process
    elif component_id == "navigation":
        navigation_process = process
    elif component_id == "dataset":
        dataset_process = process
    return {"success": True, "message": f"{component_id} started.", "pid": process.pid, "command": command, "component": _component_by_id(component_id)}


def _stop_component(component_id: str) -> dict:
    global robot_base_process, lidar_process, camera_process, slam_process, navigation_process, dataset_process
    _require_action(_component_by_id(component_id)["action"])
    process = {
        "robot_base": robot_base_process,
        "lidar": lidar_process,
        "camera": camera_process,
        "slam": slam_process,
        "navigation": navigation_process,
        "dataset": dataset_process,
    }[component_id]
    result = _stop_stack(process, component_id)
    if component_id == "robot_base":
        robot_base_process = None
    elif component_id == "lidar":
        lidar_process = None
    elif component_id == "camera":
        camera_process = None
    elif component_id == "slam":
        slam_process = None
    elif component_id == "navigation":
        navigation_process = None
    elif component_id == "dataset":
        dataset_process = None
    return {"success": True, "message": f"{component_id} stopped.", "component": _component_by_id(component_id), **result}


@app.post("/api/robot/cmd_vel")
async def publish_cmd_vel(command: VelocityCommand) -> dict:
    _require_action("hardware")
    _require_control_node().publish_cmd_vel(command.linear_x, command.linear_y, command.angular_z)
    return {"success": True}


@app.post("/api/robot/nav/goal")
async def send_nav_goal(goal: NavGoalRequest) -> dict:
    _require_action("navigation")
    _require_control_node().send_cca_nmpc_goal(goal.x, goal.y, goal.yaw)
    return {"success": True, "controller": "CCA_NMPC", "message": "RAI navigation goal published."}


@app.post("/api/robot/nav/route")
async def send_nav_route(route: RoutePlanRequest) -> dict:
    _require_action("navigation")
    node = _require_control_node()
    node.send_cca_nmpc_route(
        {"x": route.start.x, "y": route.start.y, "yaw": route.start.yaw},
        {"x": route.goal.x, "y": route.goal.y, "yaw": route.goal.yaw},
        route.start_tolerance,
    )
    return {"success": True, "message": "Route dispatched."}


@app.post("/api/robot/nav/cancel")
async def cancel_nav_goal() -> dict:
    _require_action("navigation")
    node = _require_control_node()
    node.cancel_nav_goal()
    node.publish_cmd_vel(0.0, 0.0, 0.0)
    return {"success": True}


@app.get("/api/robot/anchors")
async def get_anchors() -> dict:
    return _require_control_node().get_anchor_state()


@app.post("/api/robot/initial_pose")
async def set_initial_pose(request: InitialPoseRequest) -> dict:
    pose = _require_control_node().publish_initial_pose(request.x, request.y, request.yaw, request.set_home)
    anchors = _require_control_node().get_anchor_state()
    return {"success": True, "message": "Initial pose published.", "initial_pose": pose, "home_pose": anchors.get("home_pose")}


@app.post("/api/robot/home")
async def set_home_pose(request: PoseRequest) -> dict:
    pose = _require_control_node().set_home_pose(request.x, request.y, request.yaw)
    return {"success": True, "message": "Home pose updated.", "home_pose": pose}


@app.post("/api/robot/nav/home")
async def send_home_goal() -> dict:
    node = _require_control_node()
    home_pose = node.get_anchor_state().get("home_pose")
    if home_pose is None:
        raise HTTPException(status_code=503, detail="Home pose is not set")
    node.send_cca_nmpc_goal(home_pose["x"], home_pose["y"], home_pose.get("yaw", 0.0))
    return {"success": True, "message": "RAI navigation home goal published."}


@app.post("/api/webrtc/offer")
async def webrtc_offer(offer: dict) -> dict:
    node = _require_control_node()
    pc = RTCPeerConnection()
    peer_connections.add(pc)
    node.register_camera_client()
    pc.addTrack(RosImageVideoTrack(node))

    @pc.on("connectionstatechange")
    async def on_connectionstatechange():
        if pc.connectionState in {"failed", "closed", "disconnected"}:
            await pc.close()
            peer_connections.discard(pc)
            node.unregister_camera_client()

    await pc.setRemoteDescription(RTCSessionDescription(sdp=offer["sdp"], type=offer["type"]))
    answer = await pc.createAnswer()
    await pc.setLocalDescription(answer)
    return {"sdp": pc.localDescription.sdp, "type": pc.localDescription.type}


@app.websocket("/api/ws/telemetry")
async def telemetry_ws(websocket: WebSocket):
    node = _require_control_node()
    await websocket.accept()
    node.register_telemetry_client()
    try:
        while True:
            with node.lock:
                telemetry = json.loads(json.dumps(node.telemetry))
            await websocket.send_json(telemetry)
            await asyncio.sleep(0.1)
    except WebSocketDisconnect:
        node.unregister_telemetry_client()


@app.websocket("/api/ws/map")
async def map_ws(websocket: WebSocket):
    node = _require_control_node()
    await websocket.accept()
    node.ensure_map_subscription()
    node.register_map_client()
    try:
        while True:
            with node.lock:
                latest_map = json.loads(json.dumps(node.latest_map)) if node.latest_map is not None else None
            if latest_map is not None:
                await websocket.send_json(latest_map)
            await asyncio.sleep(0.5)
    except WebSocketDisconnect:
        node.unregister_map_client()


@app.websocket("/api/ws/paths")
async def paths_ws(websocket: WebSocket):
    node = _require_control_node()
    await websocket.accept()
    node.register_paths_client()
    try:
        while True:
            with node.lock:
                payload = {
                    "global_path": list(node.latest_global_plan),
                    "local_path": list(node.latest_local_plan),
                }
            await websocket.send_json(payload)
            await asyncio.sleep(0.2)
    except WebSocketDisconnect:
        node.unregister_paths_client()


@app.websocket("/api/ws/control")
async def control_ws(websocket: WebSocket):
    node = _require_control_node()
    await websocket.accept()
    try:
        while True:
            payload = await websocket.receive_json()
            command = VelocityCommand(**payload)
            node.publish_cmd_vel(command.linear_x, command.linear_y, command.angular_z)
            await websocket.send_json({"success": True})
    except WebSocketDisconnect:
        node.publish_cmd_vel(0.0, 0.0, 0.0)


@app.get("/api/rai-navigation/options")
async def navigation_options() -> dict:
    return {
        "local_planners": [
            {"id": "CCA_NMPC", "label": "CCA-NMPC", "plugin": "rai_controller_cca_nmpc", "native": True},
            {"id": "NMPC", "label": "NMPC", "plugin": "rai_controller_cca_nmpc", "native": True},
        ],
        "global_planners": [
            {"id": "A_STAR", "label": "A*", "plugin": "rai_planner_a_star"},
            {"id": "DIJKSTRA", "label": "Dijkstra", "plugin": "rai_planner_dijkstra"},
            {"id": "STRAIGHT_LINE", "label": "Straight line", "plugin": "rai_planner_straight_line"},
        ],
    }


@app.get("/api/rai-navigation/config")
async def get_navigation_config() -> dict:
    running = navigation_process is not None and navigation_process.poll() is None
    return {
        **navigation_config,
        "running": running,
        "controller_server_enabled": True,
        "rai_controller_running": running,
    }


@app.post("/api/rai-navigation/config")
async def set_navigation_config(request: RaiNavigationConfigRequest) -> dict:
    navigation_config["local_planner"] = request.local_planner.upper()
    navigation_config["global_planner"] = request.global_planner.upper()
    if request.map_path:
        navigation_config["map_path"] = request.map_path
    if request.params_path:
        navigation_config["params_path"] = request.params_path
    return await get_navigation_config()


def main() -> None:
    global control_node, spin_thread
    rclpy.init()
    control_node = WebBridgeNode()
    spin_thread = threading.Thread(target=_spin_control_node, daemon=True)
    spin_thread.start()
    try:
        uvicorn.run(app, host=DEFAULT_HOST, port=DEFAULT_PORT, log_level="info")
    finally:
        for pc in list(peer_connections):
            try:
                asyncio.run(pc.close())
            except Exception:
                pass
        if control_node is not None:
            control_node.destroy_node()
            control_node = None
        rclpy.shutdown()


if __name__ == "__main__":
    main()
