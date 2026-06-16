"use client";

import { useCallback, useEffect, useRef, useState, type RefObject } from "react";
import { useWebSocket } from "@/hooks/useWebSocket";
import { fetchWithAuth } from "@/lib/api";
import { normalizeRobotTelemetry, type RobotUiTelemetry } from "@/lib/robot-telemetry";
import type { MapPayload, PathsPayload, StreamState } from "@/types/robot-runtime";

export interface MonitorRuntime {
  videoRef: RefObject<HTMLVideoElement | null>;
  mapCanvasRef: RefObject<HTMLCanvasElement | null>;
  state: StreamState;
  message: string;
  telemetry: RobotUiTelemetry | null;
  mapData: MapPayload | null;
  paths: PathsPayload;
  live: boolean;
}

export function useMonitorRuntime(): MonitorRuntime {
  const videoRef = useRef<HTMLVideoElement | null>(null);
  const mapCanvasRef = useRef<HTMLCanvasElement | null>(null);
  const pcRef = useRef<RTCPeerConnection | null>(null);
  const [state, setState] = useState<StreamState>("idle");
  const [message, setMessage] = useState("Camera stream is idle.");
  const [telemetry, setTelemetry] = useState<RobotUiTelemetry | null>(null);
  const [mapData, setMapData] = useState<MapPayload | null>(null);
  const [paths, setPaths] = useState<PathsPayload>({});

  const loadFallbackMap = useCallback(async () => {
    try {
      const response = await fetchWithAuth("/api/map/list");
      const savedMaps = (await response.json()) as MapPayload[];
      const latestMapId = savedMaps[0]?.id;
      if (latestMapId == null) return;
      const mapResponse = await fetchWithAuth(`/api/map/${latestMapId}`);
      const fallbackMap = (await mapResponse.json()) as MapPayload;
      setMapData((current) => current ?? fallbackMap);
    } catch {
      // Leave monitor map empty when neither live nor saved maps are available.
    }
  }, []);

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
        setMapData(JSON.parse(String(event.data)) as MapPayload);
      } catch {
        setMapData(null);
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

  const stopStream = useCallback(async () => {
    pcRef.current?.close();
    pcRef.current = null;
    if (videoRef.current?.srcObject) {
      const stream = videoRef.current.srcObject as MediaStream;
      stream.getTracks().forEach((track) => track.stop());
      videoRef.current.srcObject = null;
    }
    setState("idle");
    setMessage("Camera stream stopped.");
  }, []);

  const startStream = useCallback(async () => {
    await stopStream();
    setState("connecting");
    setMessage("Negotiating WebRTC with rai_web_api...");

    try {
      const pc = new RTCPeerConnection();
      pcRef.current = pc;
      pc.addTransceiver("video", { direction: "recvonly" });
      pc.ontrack = (event) => attachRemoteStream(videoRef.current, event, setState, setMessage);
      pc.onconnectionstatechange = () => updateConnectionState(pc, setState, setMessage);

      const offer = await pc.createOffer();
      await pc.setLocalDescription(offer);
      const response = await fetchWithAuth("/api/webrtc/offer", {
        method: "POST",
        body: JSON.stringify({ sdp: offer.sdp, type: offer.type }),
      });
      await pc.setRemoteDescription(await response.json());
    } catch (error) {
      pcRef.current?.close();
      pcRef.current = null;
      setState("error");
      setMessage(error instanceof Error ? error.message : "Cannot start WebRTC stream.");
    }
  }, [stopStream]);

  useEffect(() => {
    return () => {
      pcRef.current?.close();
      pcRef.current = null;
    };
  }, []);

  useEffect(() => {
    const timer = window.setTimeout(() => {
      void startStream();
    }, 0);
    return () => window.clearTimeout(timer);
  }, [startStream]);

  useEffect(() => {
    if (mapData != null) return;
    const timer = window.setTimeout(() => {
      void loadFallbackMap();
    }, 1200);
    return () => window.clearTimeout(timer);
  }, [loadFallbackMap, mapData]);

  return {
    videoRef,
    mapCanvasRef,
    state,
    message,
    telemetry,
    mapData,
    paths,
    live: state === "live",
  };
}

function attachRemoteStream(
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

function updateConnectionState(
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
