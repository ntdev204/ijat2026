"use client";

import { useCallback, useEffect, useRef, useState } from "react";
import { useWebSocket } from "@/hooks/useWebSocket";
import { fetchWithAuth } from "@/lib/api";
import {
  attachRemoteStream,
  createEmptyPaths,
  createTelemetryMessage,
  DEFAULT_MONITOR_MESSAGE,
  FALLBACK_MAP_DELAY_MS,
  parseJsonPayload,
  parseTelemetryPayload,
  preserveCurrentMap,
  releaseVideoStream,
  updateConnectionState,
} from "@/lib/monitor-runtime";
import { normalizeRobotTelemetry } from "@/lib/robot-telemetry";
import type { MonitorRuntime } from "@/types/monitor-runtime";
import type { MapPayload, PathsPayload, StreamState } from "@/types/robot-runtime";

export function useMonitorRuntime(): MonitorRuntime {
  const videoRef = useRef<HTMLVideoElement | null>(null);
  const mapCanvasRef = useRef<HTMLCanvasElement | null>(null);
  const pcRef = useRef<RTCPeerConnection | null>(null);

  const [state, setState] = useState<StreamState>("idle");
  const [message, setMessage] = useState(DEFAULT_MONITOR_MESSAGE);
  const [telemetry, setTelemetry] = useState<MonitorRuntime["telemetry"]>(null);
  const [mapData, setMapData] = useState<MapPayload | null>(null);
  const [paths, setPaths] = useState<PathsPayload>(createEmptyPaths);

  const loadFallbackMap = useCallback(async () => {
    try {
      const response = await fetchWithAuth("/api/map/list");
      const savedMaps = (await response.json()) as MapPayload[];
      const latestMapId = savedMaps[0]?.id;
      if (latestMapId == null) {
        return;
      }

      const mapResponse = await fetchWithAuth(`/api/map/${latestMapId}`);
      const fallbackMap = (await mapResponse.json()) as MapPayload;
      setMapData((current) => preserveCurrentMap(current, fallbackMap));
    } catch {
      // keep empty
    }
  }, []);

  const showSavedMap = useCallback(async (mapId: number) => {
    const response = await fetchWithAuth(`/api/map/${mapId}`);
    setMapData((await response.json()) as MapPayload);
  }, []);

  useWebSocket("/ws/telemetry", {
    onMessage: (event) => {
      try {
        setTelemetry(parseTelemetryPayload(event, normalizeRobotTelemetry));
      } catch {
        setTelemetry(null);
      }
    },
  });

  useWebSocket("/ws/map", {
    onMessage: (event) => {
      try {
        setMapData(parseJsonPayload<MapPayload>(event));
      } catch {
        setMapData(null);
      }
    },
  });

  useWebSocket("/ws/paths", {
    onMessage: (event) => {
      try {
        setPaths(parseJsonPayload<PathsPayload>(event));
      } catch {
        setPaths(createEmptyPaths());
      }
    },
  });

  const stopStream = useCallback(async () => {
    releaseVideoStream(videoRef, pcRef, setState, setMessage);
  }, []);

  const startStream = useCallback(async () => {
    await stopStream();
    setState("connecting");
    setMessage("Negotiating WebRTC with rai_web_api...");

    try {
      const pc = new RTCPeerConnection();
      pcRef.current = pc;
      pc.addTransceiver("video", { direction: "recvonly" });
      pc.ontrack = (event) =>
        attachRemoteStream(videoRef.current, event, setState, setMessage);
      pc.onconnectionstatechange = () =>
        updateConnectionState(pc, setState, setMessage);

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
      setMessage(createTelemetryMessage(error, "Cannot start WebRTC stream."));
    }
  }, [stopStream]);

  useEffect(
    () => () => {
      pcRef.current?.close();
      pcRef.current = null;
    },
    [],
  );

  useEffect(() => {
    const timer = window.setTimeout(() => {
      void startStream();
    }, 0);
    return () => window.clearTimeout(timer);
  }, [startStream]);

  useEffect(() => {
    if (mapData != null) {
      return;
    }
    const timer = window.setTimeout(() => {
      void loadFallbackMap();
    }, FALLBACK_MAP_DELAY_MS);
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
    showSavedMap,
  };
}
