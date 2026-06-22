# RAI Human Perception

`rai_human_perception` is the RGB-D perception layer for CCA-NMPC human-centric
navigation on the Rai Mecanum robot. YOLO is used only as a perception module;
the controller contribution remains the Continuous Context-Adaptive NMPC.

## Pipeline

```text
RGB image + depth image + camera intrinsics
  -> YOLO26m person detections
  -> median depth association in the bbox center
  -> camera-frame 3D point
  -> tf2 transform to map/odom
  -> linear Kalman tracker
  -> S_h = [x_h, y_h, v_hx, v_hy]^T
```

The state published by this package is directly compatible with the CCA-NMPC
human prediction model:

```text
p_hat_{h,k+i|k} =
  [x_h, y_h]^T + i T_s [v_hx, v_hy]^T
```

## Topics

Inputs:

- `/camera/color/image_raw`
- `/camera/depth/image_raw`
- `/camera/color/camera_info`
- TF from `camera_link` to `map` or `odom`

Outputs:

- `/cca_nmpc/humans` (`rai_human_perception/msg/HumanArray`)
- `/cca_nmpc/context_input` (`rai_human_perception/msg/ContextInput`)
- `/canmpc/humans` (`rai_controller/msg/HumanStates`) bridge for the existing controller
- `/perception/debug/image`
- `/perception/debug/depth`
- `/perception/debug/tracks`
- `/perception/debug/latency`

## Build

From the ROS2 workspace:

```bash
cd ~/rai_ros2
rosdep install --from-paths src --ignore-src -r -y
colcon build --packages-select rai_human_perception
source install/setup.bash
```

## Export TensorRT Engine

```bash
python3 src/rai_human_perception/scripts/export_engine.py \
  --weights best.pt \
  --imgsz 640 \
  --fp16 \
  --output model.engine
```

or:

```bash
ros2 launch rai_human_perception export_engine.launch.py \
  weights:=best.pt \
  output:=model.engine \
  imgsz:=640
```

If `engine_path` does not exist and `auto_export_engine` is true, the node calls
`export_engine.py` automatically. The C++ node is written to compile on machines
without TensorRT headers; in that case it falls back to OpenCV DNN using the ONNX
or weights path. On Jetson, use the export and benchmark scripts to validate the
TensorRT engine path before deployment.

## Run

```bash
ros2 launch rai_human_perception perception.launch.py
```

Override camera topics or target frame in `config/perception.yaml`.

## Performance Targets

Target platform: Jetson Orin Nano 8GB, Ubuntu 22.04, ROS2 Humble, CUDA,
TensorRT, Orbbec Astra RGB-D camera.

- YOLO inference: less than 15 ms
- Depth association: less than 5 ms
- Kalman update: less than 1 ms
- Total perception: less than 20 ms
- Runtime frequency: at least 20 Hz

Use:

```bash
python3 src/rai_human_perception/scripts/benchmark_engine.py --model model.engine
```

## CSV Logging

The node writes:

```text
timestamp,human_id,x_h,y_h,vx_h,vy_h,confidence,depth,latency_ms
```

Default path: `/tmp/rai_human_perception.csv`.
