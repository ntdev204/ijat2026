"use client";

import { getWebSocketBaseUrl } from "@/lib/api";
import { useCallback, useEffect, useRef, useState } from "react";

function resolveWebSocketPath(path: string) {
  if (path.startsWith("/api/ws/")) return path;
  if (path.startsWith("/ws/")) return `/api${path}`;
  return path;
}

interface UseWebSocketOptions {
  binaryType?: "blob" | "arraybuffer";
  autoReconnect?: boolean;
  onMessage?: (event: MessageEvent) => void;
}

export function useWebSocket(path: string, options: UseWebSocketOptions = {}) {
  const { binaryType = "blob", autoReconnect = true, onMessage } = options;
  const [isConnected, setIsConnected] = useState(false);
  const [serverOffline, setServerOffline] = useState(false);
  const wsRef = useRef<WebSocket | null>(null);
  const reconnectCount = useRef(0);
  const reconnectTimer = useRef<ReturnType<typeof setTimeout> | null>(null);
  const isMounted = useRef(true);
  const onMessageRef = useRef(onMessage);
  const connectRef = useRef<() => void>(() => {});
  const connectionGenerationRef = useRef(0);
  const maxReconnects = 3;

  useEffect(() => {
    onMessageRef.current = onMessage;
  }, [onMessage]);

  useEffect(() => {
    connectRef.current = () => {
      if (!isMounted.current) return;
      const connectionGeneration = connectionGenerationRef.current + 1;
      connectionGenerationRef.current = connectionGeneration;
      void (async () => {
        if (!isMounted.current || connectionGenerationRef.current !== connectionGeneration) {
          return;
        }

        const resolvedPath = resolveWebSocketPath(path);
        const url = new URL(`${getWebSocketBaseUrl(resolvedPath)}${resolvedPath}`);
        const ws = new WebSocket(url.toString());
        ws.binaryType = binaryType;
        wsRef.current = ws;

        ws.onopen = () => {
          if (!isMounted.current || connectionGenerationRef.current !== connectionGeneration || wsRef.current !== ws) {
            ws.close();
            return;
          }
          setIsConnected(true);
          setServerOffline(false);
          reconnectCount.current = 0;
        };

        ws.onclose = () => {
          if (!isMounted.current || connectionGenerationRef.current !== connectionGeneration || wsRef.current !== ws) return;
          setIsConnected(false);
          wsRef.current = null;
          if (autoReconnect && reconnectCount.current < maxReconnects) {
            reconnectCount.current += 1;
            const delay = Math.min(1000 * Math.pow(2, reconnectCount.current), 10000);
            reconnectTimer.current = setTimeout(() => connectRef.current(), delay);
          } else if (reconnectCount.current >= maxReconnects) {
            setServerOffline(true);
          }
        };

        ws.onerror = () => {
          if (connectionGenerationRef.current !== connectionGeneration || wsRef.current !== ws) {
            return;
          }
          ws.close();
        };

        ws.onmessage = (event) => {
          if (connectionGenerationRef.current !== connectionGeneration || wsRef.current !== ws) {
            return;
          }
          onMessageRef.current?.(event);
        };
      })();
    };
  }, [path, binaryType, autoReconnect]);

  useEffect(() => {
    isMounted.current = true;
    connectRef.current();

    return () => {
      isMounted.current = false;
      connectionGenerationRef.current += 1;
      if (reconnectTimer.current) {
        clearTimeout(reconnectTimer.current);
        reconnectTimer.current = null;
      }
      const ws = wsRef.current;
      wsRef.current = null;
      if (ws && ws.readyState !== WebSocket.CLOSED) {
        ws.onclose = null;
        ws.close();
      }
    };
  }, [path, binaryType, autoReconnect]);

  const sendMessage = useCallback((data: string | ArrayBuffer | Blob) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(data);
    }
  }, []);

  return { isConnected, serverOffline, sendMessage };
}
