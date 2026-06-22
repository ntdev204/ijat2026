#!/usr/bin/env python3
import argparse
from pathlib import Path


def parse_args():
    parser = argparse.ArgumentParser(description="Export fine-tuned YOLO26m weights to TensorRT engine.")
    parser.add_argument("--weights", required=True, help="Path to YOLO .pt weights.")
    parser.add_argument("--imgsz", type=int, default=640, help="Export image size.")
    parser.add_argument("--output", default="model.engine", help="Output TensorRT engine path.")
    parser.add_argument("--fp16", action="store_true", help="Enable FP16 TensorRT export.")
    parser.add_argument("--dynamic", action="store_true", help="Export with dynamic shape support.")
    return parser.parse_args()


def main():
    args = parse_args()
    try:
        from ultralytics import YOLO
    except ImportError as exc:
        raise SystemExit("Install ultralytics first: pip3 install ultralytics") from exc

    weights = Path(args.weights)
    if not weights.exists():
        raise SystemExit(f"Weights not found: {weights}")

    model = YOLO(str(weights))
    exported = model.export(
        format="engine",
        imgsz=args.imgsz,
        half=args.fp16,
        dynamic=args.dynamic,
        device=0,
    )
    exported_path = Path(exported)
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    if exported_path.resolve() != output_path.resolve():
        output_path.write_bytes(exported_path.read_bytes())
    print(f"TensorRT engine exported: {output_path}")


if __name__ == "__main__":
    main()
