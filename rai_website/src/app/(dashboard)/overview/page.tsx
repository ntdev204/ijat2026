"use client";

import { OverviewMetrics, OverviewPositionSection, OverviewRuntimeSources } from "@/components/overview/OverviewSections";
import { StatusBadge } from "@/components/ui/StatusBadge";
import { useOverviewTelemetry } from "@/hooks/useOverviewTelemetry";

export default function OverviewPage() {
  const telemetry = useOverviewTelemetry();

  return (
    <div className="space-y-5 sm:space-y-6">
      <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
        <h2 className="text-2xl font-bold tracking-tight text-slate-800">
          Overview
        </h2>
        <div className="flex items-center gap-3">
          <span className="text-sm font-medium text-slate-500">Robot Status</span>
          <StatusBadge status={telemetry.robotOnline ? "success" : "error"}>
            {telemetry.robotOnline ? "ONLINE" : "OFFLINE"}
          </StatusBadge>
        </div>
      </div>

      <OverviewMetrics
        ai={telemetry.ai}
        batteryPercent={telemetry.robot.battery_percent}
      />

      <div className="grid grid-cols-1 gap-6 lg:grid-cols-3">
        <OverviewPositionSection
          robotOnline={telemetry.robotOnline}
          robotPose={telemetry.robotPose}
        />
        <OverviewRuntimeSources
          isConnected={telemetry.isConnected}
          robotOnline={telemetry.robotOnline}
          adaptiveState={telemetry.ai.state}
          batteryPercent={telemetry.robot.battery_percent}
        />
      </div>
    </div>
  );
}
