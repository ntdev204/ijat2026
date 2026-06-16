import { decodeGridData } from "@/lib/map-canvas";
import type { MapPayload } from "@/types/robot-runtime";
import * as THREE from "three";

export function createMapTexture(map: MapPayload) {
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
      const color = gazeboOccupancyColor(grid[src] ?? 255);
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

function gazeboOccupancyColor(value: number) {
  if (value === 255) return 210;
  if (value >= 100) return 36;
  if (value === 0) return 245;
  return Math.max(48, 245 - Math.round(value * 2));
}
