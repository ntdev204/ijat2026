"use client";

import { useMemo } from "react";
import type { SystemComponent } from "@/types/robot-runtime";
import type { SystemComponentViewModel } from "@/types/system";

export function useSystemComponentViewModels({
  components,
  busyId,
}: {
  components: SystemComponent[];
  busyId: string | null;
}) {
  return useMemo<SystemComponentViewModel[]>(
    () =>
      components.map((component) => {
        const starting = busyId === `${component.id}:start`;
        const stopping = busyId === `${component.id}:stop`;

        return {
          ...component,
          starting,
          stopping,
          statusLabel: component.running
            ? "RUNNING"
            : component.proxy_error
              ? "UNREACHABLE"
              : "STOPPED",
          statusVariant: component.running
            ? "success"
            : component.proxy_error
              ? "warning"
              : "default",
          toggleLabel: starting
            ? "Starting..."
            : stopping
              ? "Stopping..."
              : component.running
                ? "On"
                : "Off",
          hostLabel: formatHostLabel(component.host_device),
          launchLabel: formatLaunchName(component.launch_file),
        };
      }),
    [busyId, components],
  );
}

function formatLaunchName(value: string) {
  return value.replace(/\.launch\.py$/i, "").replace(/_/g, " ");
}

function formatHostLabel(value: string) {
  if (value === "pi") return "Raspberry Pi 4";
  if (value === "jetson") return "Jetson Orin Nano";
  if (value === "sim") return "Simulation Host";
  return value.replace(/_/g, " ");
}
