import type { RefObject } from "react";
import type { RaiTelemetry, RobotUiTelemetry } from "@/lib/robot-telemetry";
import type { MapPayload, PathsPayload, StreamState } from "@/types/robot-runtime";

export const DEFAULT_MONITOR_MESSAGE = "Camera stream is idle.";
export const FALLBACK_MAP_DELAY_MS = 1200;

export function createEmptyPaths(): PathsPayload {
  return {};
}

export function createTelemetryMessage(
  error: unknown,
  fallback: string,
) {
  return error instanceof Error ? error.message : fallback;
}

export function attachRemoteStream(
  video: HTMLVideoElement | null,
  event: RTCTrackEvent,
  setState: (state: StreamState) => void,
  setMessage: (message: string) => void,
) {
  const [stream] = event.streams;
  if (!video || !stream) return;
  video.srcObject = stream;
  setState("live");
  setMessage("Receiving /camera/color/image_raw through WebRTC.");
}

export function updateConnectionState(
  pc: RTCPeerConnection,
  setState: (state: StreamState) => void,
  setMessage: (message: string) => void,
) {
  if (pc.connectionState === "failed" || pc.connectionState === "disconnected") {
    setState("error");
    setMessage(`WebRTC connection ${pc.connectionState}.`);
  }
  if (pc.connectionState === "closed") {
    setState("idle");
  }
}

export function releaseVideoStream(
  videoRef: RefObject<HTMLVideoElement | null>,
  pcRef: RefObject<RTCPeerConnection | null>,
  setState: (state: StreamState) => void,
  setMessage: (message: string) => void,
) {
  pcRef.current?.close();
  pcRef.current = null;
  if (videoRef.current?.srcObject) {
    const stream = videoRef.current.srcObject as MediaStream;
    stream.getTracks().forEach((track) => track.stop());
    videoRef.current.srcObject = null;
  }
  setState("idle");
  setMessage("Camera stream stopped.");
}

export function parseTelemetryPayload(
  event: MessageEvent,
  normalize: (payload: RaiTelemetry | null | undefined) => { robot: RobotUiTelemetry },
) {
  return normalize(JSON.parse(String(event.data))).robot;
}

export function parseJsonPayload<T>(event: MessageEvent) {
  return JSON.parse(String(event.data)) as T;
}

export function preserveCurrentMap(
  current: MapPayload | null,
  next: MapPayload,
) {
  return current ?? next;
}
