"use client";

import { RefreshCw } from "lucide-react";
import { OperationModeMenu } from "@/components/layout/OperationModeMenu";
import { SystemComponentsGrid } from "@/components/system/SystemComponentsGrid";
import { Button } from "@/components/ui/button";
import { useOperationMode } from "@/contexts/OperationModeContext";
import { useSystemComponentViewModels } from "@/hooks/useSystemComponentViewModels";
import { useSystemControl } from "@/hooks/useSystemControl";

export default function SystemPage() {
  const system = useSystemControl();
  const { operationMode, runtime } = useOperationMode();
  const components = useSystemComponentViewModels({
    components: system.components,
    busyId: system.busyId,
  });

  return (
    <div className="space-y-5 sm:space-y-6">
      <div className="flex flex-col gap-3 lg:flex-row lg:items-center lg:justify-between">
        <div>
          <h2 className="text-2xl font-bold tracking-tight text-slate-900">
            System
          </h2>
          {runtime && (
            <p className="mt-1 text-sm text-slate-500">
              API target: {runtime.device_label} ({runtime.device_role})
            </p>
          )}
          <p className="mt-1 text-xs text-slate-500">
            {operationMode === "real"
              ? "Real mode uses physical robot workflows."
              : operationMode === "sim"
                ? "Simulation mode disables physical-device workflows."
                : "Hybrid mode allows a physical robot with Gazebo or RViz environment support."}
          </p>
        </div>
        <div className="flex flex-wrap gap-2 max-sm:[&_[data-slot=button]]:flex-1">
          <OperationModeMenu />
          <Button
            type="button"
            variant="outline"
            className="gap-2"
            onClick={() => void system.refresh()}
            disabled={Boolean(system.busyId)}
          >
            <RefreshCw className="h-4 w-4" />
            Refresh
          </Button>
        </div>
      </div>

      <SystemComponentsGrid
        components={components}
        cameraDepthEnabled={system.cameraDepthEnabled}
        onCameraDepthChange={system.setCameraDepthEnabled}
        onStart={system.startComponent}
        onStop={system.stopComponent}
      />
    </div>
  );
}
