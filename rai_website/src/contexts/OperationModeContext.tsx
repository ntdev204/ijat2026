"use client";

import { createContext, useCallback, useContext, useEffect, useMemo, useState } from "react";
import { fetchWithAuth } from "@/lib/api";
import type { SystemRuntime } from "@/types/robot-runtime";

type OperationMode = "real" | "sim" | "hybrid";

interface OperationModeContextValue {
  runtime: SystemRuntime | null;
  operationMode: OperationMode;
  busy: boolean;
  refresh: () => Promise<void>;
  setOperationMode: (mode: OperationMode) => Promise<void>;
}

const OperationModeContext = createContext<OperationModeContextValue | null>(null);

export function OperationModeProvider({ children }: { children: React.ReactNode }) {
  const [runtime, setRuntime] = useState<SystemRuntime | null>(null);
  const [busy, setBusy] = useState(false);

  const refresh = useCallback(async () => {
    const response = await fetchWithAuth("/api/system/runtime");
    const payload = (await response.json()) as SystemRuntime;
    setRuntime(payload);
  }, []);

  useEffect(() => {
    const timer = window.setTimeout(() => {
      void refresh().catch(() => undefined);
    }, 0);
    return () => window.clearTimeout(timer);
  }, [refresh]);

  const setOperationMode = useCallback(async (mode: OperationMode) => {
    setBusy(true);
    try {
      const response = await fetchWithAuth("/api/system/operation-mode", {
        method: "POST",
        body: JSON.stringify({ mode }),
      });
      const payload = (await response.json()) as SystemRuntime;
      setRuntime(payload);
    } finally {
      setBusy(false);
    }
  }, []);

  const value = useMemo<OperationModeContextValue>(() => ({
    runtime,
    operationMode: runtime?.operation_mode ?? "real",
    busy,
    refresh,
    setOperationMode,
  }), [busy, refresh, runtime, setOperationMode]);

  return <OperationModeContext.Provider value={value}>{children}</OperationModeContext.Provider>;
}

export function useOperationMode() {
  const context = useContext(OperationModeContext);
  if (!context) {
    throw new Error("useOperationMode must be used within OperationModeProvider");
  }
  return context;
}

export type { OperationMode };
