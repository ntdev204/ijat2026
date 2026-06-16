"use client";

import { StatusBadge } from "@/components/ui/StatusBadge";
import { fetchWithAuth } from "@/lib/api";
import { useWebSocket } from "@/hooks/useWebSocket";
import { normalizeRobotTelemetry, type RobotUiTelemetry } from "@/lib/robot-telemetry";
import { Map as MapIcon, Users, Video, VideoOff, Waypoints } from "lucide-react";
import { useCallback, useEffect, useRef, useState } from "react";

interface MapPayload {
  width: number;
  height: number;
  resolution: number;
  origin_x: number;
  origin_y: number;
  grid_data: string;
}

interface PathsPayload {
  global_plan?: Array<{ x: number; y: number }>;
  local_plan?: Array<{ x: number; y: number }>;
}

type StreamState = "idle" | "connecting" | "live" | "error";

function worldToPixel(point: { x: number; y: number }, map: MapPayload) {
  return {
    x: (point.x - map.origin_x) / map.resolution,
    y: map.height - (point.y - map.origin_y) / map.resolution,
  };
}

function decodeGridData(gridData: string) {
  const binary = window.atob(gridData);
  const values = new Uint8Array(binary.length);
  for (let index = 0; index < binary.length; index += 1) {
    values[index] = binary.charCodeAt(index);
  }
  return values;
}

function occupancyColor(value: number) {
  if (value === 255) return 205;
  if (value >= 100) return 24;
  if (value === 0) return 245;
  return Math.max(40, 245 - Math.round(value * 2));
}

export default function MonitorPage() {
  const videoRef = useRef<HTMLVideoElement | null>(null);
  const mapCanvasRef = useRef<HTMLCanvasElement | null>(null);
  const pcRef = useRef<RTCPeerConnection | null>(null);
  const [state, setState] = useState<StreamState>("idle");
  const [message, setMessage] = useState("Camera stream is idle.");
  const [telemetry, setTelemetry] = useState<RobotUiTelemetry | null>(null);
  const [mapData, setMapData] = useState<MapPayload | null>(null);
  const [paths, setPaths] = useState<PathsPayload>({});

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

      pc.ontrack = (event) => {
        const [stream] = event.streams;
        if (videoRef.current && stream) {
          videoRef.current.srcObject = stream;
          setState("live");
          setMessage("Receiving /camera/color/image_raw through WebRTC.");
        }
      };

      pc.onconnectionstatechange = () => {
        if (pc.connectionState === "failed" || pc.connectionState === "disconnected") {
          setState("error");
          setMessage(`WebRTC connection ${pc.connectionState}.`);
        }
        if (pc.connectionState === "closed") {
          setState("idle");
        }
      };

      const offer = await pc.createOffer();
      await pc.setLocalDescription(offer);
      const response = await fetchWithAuth("/api/webrtc/offer", {
        method: "POST",
        body: JSON.stringify({ sdp: offer.sdp, type: offer.type }),
      });
      const answer = await response.json();
      await pc.setRemoteDescription(answer);
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
    const canvas = mapCanvasRef.current;
    const map = mapData;
    if (!canvas || !map) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    canvas.width = map.width;
    canvas.height = map.height;
    const image = ctx.createImageData(map.width, map.height);
    const grid = decodeGridData(map.grid_data);
    for (let y = 0; y < map.height; y += 1) {
      for (let x = 0; x < map.width; x += 1) {
        const src = (map.height - 1 - y) * map.width + x;
        const dst = (y * map.width + x) * 4;
        const color = occupancyColor(grid[src] ?? 255);
        image.data[dst] = color;
        image.data[dst + 1] = color;
        image.data[dst + 2] = color;
        image.data[dst + 3] = 255;
      }
    }
    ctx.putImageData(image, 0, 0);

    const drawPath = (points: Array<{ x: number; y: number }> | undefined, color: string, width: number) => {
      if (!points || points.length < 2) return;
      ctx.save();
      ctx.strokeStyle = color;
      ctx.lineWidth = width;
      ctx.beginPath();
      points.forEach((point, index) => {
        const pixel = worldToPixel(point, map);
        if (index === 0) ctx.moveTo(pixel.x, pixel.y);
        else ctx.lineTo(pixel.x, pixel.y);
      });
      ctx.stroke();
      ctx.restore();
    };

    drawPath(paths.global_plan, "#2563eb", 3);
    drawPath(paths.local_plan, "#f97316", 2);

    for (const human of telemetry?.humans ?? []) {
      if (typeof human.x !== "number" || typeof human.y !== "number") continue;
      const pixel = worldToPixel({ x: human.x, y: human.y }, map);
      ctx.save();
      ctx.fillStyle = "rgba(220, 38, 38, 0.85)";
      ctx.beginPath();
      ctx.arc(pixel.x, pixel.y, 7, 0, Math.PI * 2);
      ctx.fill();
      ctx.restore();
    }
  }, [mapData, paths, telemetry]);

  const live = state === "live";

  return (
    <div className="space-y-5">
      <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
        <div>
          <h2 className="text-2xl font-bold tracking-tight text-slate-800">Monitor</h2>
          <p className="mt-1 text-sm text-slate-500">Video stream, realtime map, and live planner traces.</p>
        </div>
        <StatusBadge status={live ? "success" : state === "error" ? "error" : "warning"}>
          {live ? "LIVE" : state.toUpperCase()}
        </StatusBadge>
      </div>

      <div className="grid grid-cols-1 gap-5 xl:grid-cols-[minmax(0,1.2fr)_minmax(0,1fr)]">
        <section className="overflow-hidden rounded-lg border border-slate-800 bg-black shadow-lg">
          <div className="relative aspect-[4/3] bg-black">
            <video ref={videoRef} autoPlay playsInline muted className="h-full w-full object-contain" />
            {!live && (
              <div className="absolute inset-0 flex flex-col items-center justify-center gap-2 text-slate-500">
                {state === "error" ? <VideoOff className="h-12 w-12" /> : <Video className="h-12 w-12" />}
                <span>{message}</span>
              </div>
            )}
          </div>
          <div className="border-t border-slate-800 px-4 py-3 text-sm text-slate-300">
            2D bbox overlay is unavailable until the perception pipeline publishes image-space detections. Current `/canmpc/humans` only exposes tracked world poses.
          </div>
        </section>

        <section className="overflow-hidden rounded-lg border border-slate-200 bg-white shadow-sm">
          <div className="flex items-center gap-2 border-b border-slate-200 px-4 py-3 text-sm text-slate-600">
            <MapIcon className="size-4 text-blue-600" />
            <span className="font-medium text-slate-800">Realtime Map</span>
          </div>
          <div className="flex min-h-[360px] items-center justify-center overflow-auto bg-slate-100 p-4">
            {mapData ? (
              <canvas ref={mapCanvasRef} className="max-h-[420px] max-w-full rounded-md border border-slate-300 bg-white shadow-sm" />
            ) : (
              <div className="text-sm text-slate-400">Waiting for /api/ws/map.</div>
            )}
          </div>
          <div className="grid gap-3 border-t border-slate-200 px-4 py-3 text-sm text-slate-600 md:grid-cols-3">
            <div className="flex items-center gap-2">
              <Waypoints className="size-4 text-blue-600" />
              <span>Global {paths.global_plan?.length ?? 0}</span>
            </div>
            <div className="flex items-center gap-2">
              <Waypoints className="size-4 text-amber-600" />
              <span>Local {paths.local_plan?.length ?? 0}</span>
            </div>
            <div className="flex items-center gap-2">
              <Users className="size-4 text-rose-600" />
              <span>Humans {telemetry?.humans.length ?? 0}</span>
            </div>
          </div>
        </section>
      </div>

      <div className="rounded-md border border-slate-200 bg-white px-4 py-3 text-sm text-slate-700">
          {message}
          {(telemetry?.humans.length ?? 0) > 0 && (
            <div className="mt-3 grid gap-2 md:grid-cols-2">
              {telemetry?.humans.map((human, index) => (
                <div key={`human-${human.id ?? index}`} className="rounded-md border border-slate-200 bg-slate-50 px-3 py-2">
                  H{human.id ?? "?"}: ({human.x?.toFixed?.(2) ?? "-"}, {human.y?.toFixed?.(2) ?? "-"}) conf {human.confidence?.toFixed?.(2) ?? "-"}
                </div>
              ))}
            </div>
          )}
      </div>
    </div>
  );
}
