"use client";

import { useEffect, useState } from "react";
import { fetchWithAuth } from "@/lib/api";
import type { SystemRuntime } from "@/types/robot-runtime";

const DEFAULT_RUNTIME: SystemRuntime = {
  device_role: "unknown",
  device_label: "unknown",
  allowed_actions: [],
  api_host: "",
  api_port: 8080,
};

export function useSystemRuntime() {
  const [runtime, setRuntime] = useState<SystemRuntime>(DEFAULT_RUNTIME);
  const [ready, setReady] = useState(false);

  useEffect(() => {
    let active = true;
    const timer = window.setTimeout(() => {
      void fetchWithAuth("/api/system/runtime")
        .then((response) => response.json())
        .then((payload) => {
          if (!active) return;
          setRuntime(payload as SystemRuntime);
          setReady(true);
        })
        .catch(() => {
          if (!active) return;
          setReady(true);
        });
    }, 0);
    return () => {
      active = false;
      window.clearTimeout(timer);
    };
  }, []);

  return { runtime, ready };
}
