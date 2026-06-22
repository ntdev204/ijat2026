#!/usr/bin/env python3
import argparse
import statistics
import time
from pathlib import Path

import cv2
import numpy as np


def parse_args():
    parser = argparse.ArgumentParser(description="Benchmark a YOLO TensorRT/OpenCV model.")
    parser.add_argument("--model", required=True, help="Path to .engine or .onnx model.")
    parser.add_argument("--image", default="", help="Optional image path.")
    parser.add_argument("--imgsz", type=int, default=640)
    parser.add_argument("--warmup", type=int, default=10)
    parser.add_argument("--iters", type=int, default=100)
    return parser.parse_args()


def main():
    args = parse_args()
    model = Path(args.model)
    if not model.exists():
        raise SystemExit(f"Model not found: {model}")

    if model.suffix == ".engine":
        try:
            from ultralytics import YOLO
        except ImportError as exc:
            raise SystemExit("Install ultralytics to benchmark TensorRT engines.") from exc
        yolo = YOLO(str(model))
        image = np.zeros((args.imgsz, args.imgsz, 3), dtype=np.uint8)
        if args.image:
            image = cv2.imread(args.image)
        for _ in range(args.warmup):
            yolo.predict(image, imgsz=args.imgsz, classes=[0], verbose=False)
        times = []
        for _ in range(args.iters):
            t0 = time.perf_counter()
            yolo.predict(image, imgsz=args.imgsz, classes=[0], verbose=False)
            times.append((time.perf_counter() - t0) * 1000.0)
    else:
        net = cv2.dnn.readNet(str(model))
        image = np.zeros((args.imgsz, args.imgsz, 3), dtype=np.uint8)
        blob = cv2.dnn.blobFromImage(image, 1.0 / 255.0, (args.imgsz, args.imgsz), swapRB=True)
        for _ in range(args.warmup):
            net.setInput(blob)
            net.forward()
        times = []
        for _ in range(args.iters):
            t0 = time.perf_counter()
            net.setInput(blob)
            net.forward()
            times.append((time.perf_counter() - t0) * 1000.0)

    print(f"mean_ms={statistics.mean(times):.2f}")
    print(f"median_ms={statistics.median(times):.2f}")
    print(f"p95_ms={np.percentile(times, 95):.2f}")


if __name__ == "__main__":
    main()
