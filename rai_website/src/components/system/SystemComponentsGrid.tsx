"use client";

import { StatusBadge } from "@/components/ui/StatusBadge";
import { Card, CardContent, CardHeader } from "@/components/ui/card";
import { Switch } from "@/components/ui/switch";
import type { SystemComponent } from "@/types/robot-runtime";
import type { SystemComponentViewModel } from "@/types/system";

export function SystemComponentsGrid({
  components,
  cameraDepthEnabled,
  onCameraDepthChange,
  onStart,
  onStop,
}: {
  components: SystemComponentViewModel[];
  cameraDepthEnabled: boolean;
  onCameraDepthChange: (value: boolean) => void;
  onStart: (componentId: SystemComponent["id"]) => Promise<void>;
  onStop: (componentId: SystemComponent["id"]) => Promise<void>;
}) {
  return (
    <div className="grid gap-4 md:grid-cols-2 2xl:grid-cols-3">
      {components.map((component) => (
        <ComponentCard
          key={component.id}
          component={component}
          onStart={onStart}
          onStop={onStop}
          cameraDepthEnabled={cameraDepthEnabled}
          onCameraDepthChange={onCameraDepthChange}
        />
      ))}
    </div>
  );
}

function ComponentCard({
  component,
  onStart,
  onStop,
  cameraDepthEnabled,
  onCameraDepthChange,
}: {
  component: SystemComponentViewModel;
  onStart: (componentId: SystemComponent["id"]) => Promise<void>;
  onStop: (componentId: SystemComponent["id"]) => Promise<void>;
  cameraDepthEnabled: boolean;
  onCameraDepthChange: (value: boolean) => void;
}) {
  return (
    <Card className="h-full">
      <CardHeader className="pb-3">
        <div className="flex flex-col gap-3 sm:flex-row sm:items-start sm:justify-between">
          <div className="min-w-0 space-y-2">
            <div className="flex flex-wrap items-center gap-2">
              <div className="text-sm font-semibold text-slate-900">
                {component.label}
              </div>
              <StatusBadge status={component.statusVariant}>
                {component.statusLabel}
              </StatusBadge>
            </div>
          </div>
          <div className="flex shrink-0 items-center justify-between gap-3 sm:justify-start">
            <span className="text-sm font-medium text-slate-600">
              {component.toggleLabel}
            </span>
            <Switch
              checked={component.running}
              disabled={component.starting || component.stopping}
              onCheckedChange={(checked) => {
                if (checked) {
                  void onStart(component.id);
                  return;
                }
                void onStop(component.id);
              }}
            />
          </div>
        </div>
      </CardHeader>
      <CardContent className="space-y-3">
        <div className="space-y-1.5">
          <MetricRow label="Host" value={component.hostLabel} />
          <MetricRow label="PID" value={component.pid ? String(component.pid) : "-"} />
          <MetricRow label="Launch" value={component.launchLabel} wrap />
        </div>
        {component.capabilities?.enable_depth_toggle && (
          <label className="flex items-center gap-3 rounded-md border border-slate-200 bg-white px-3 py-2 text-sm text-slate-700">
            <Switch
              checked={cameraDepthEnabled}
              onCheckedChange={onCameraDepthChange}
              disabled={component.running}
            />
            <span>Enable depth stream</span>
          </label>
        )}
      </CardContent>
    </Card>
  );
}

function MetricRow({
  label,
  value,
  wrap = false,
}: {
  label: string;
  value: string;
  wrap?: boolean;
}) {
  return (
    <div className="flex items-start gap-2 text-sm">
      <div className="shrink-0 text-slate-500">{label}:</div>
      <div
        className={`min-w-0 font-medium text-slate-900 ${wrap ? "break-words whitespace-normal" : "break-words sm:whitespace-nowrap"}`}
      >
        {value}
      </div>
    </div>
  );
}
