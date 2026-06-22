"use client";

import { MapPoseDialog } from "@/components/map/MapPoseDialog";
import {
  AnchorPanel,
  MonitorMessage,
  RealtimeMapPanel,
  RoutePanel,
  VideoPanel,
} from "@/components/monitor/MonitorPanels";
import { RaiNavigationPlannerPanel } from "@/components/rai-navigation/RaiNavigationPlannerPanel";
import { StatusBadge } from "@/components/ui/StatusBadge";
import { useOperationMode } from "@/contexts/OperationModeContext";
import { useMonitorRouteControl } from "@/hooks/useMonitorRouteControl";
import { useMonitorRuntime } from "@/hooks/useMonitorRuntime";
import { useRaiNavigationControl } from "@/hooks/useRaiNavigationControl";

export default function MonitorPage() {
  const { operationMode } = useOperationMode();
  const raiNavigation = useRaiNavigationControl();
  const monitor = useMonitorRuntime();
  const route = useMonitorRouteControl(
    monitor.mapCanvasRef,
    monitor.mapData,
    monitor.telemetry,
    monitor.paths,
    raiNavigation.raiNavigationConfig,
  );

  const handleSelectRaiNavigationMap = async (mapId: string) => {
    await raiNavigation.selectRaiNavigationMap(mapId);
    const parsedMapId = Number.parseInt(mapId, 10);
    if (Number.isFinite(parsedMapId)) {
      await monitor.showSavedMap(parsedMapId);
    }
  };

  return (
    <div className="space-y-5">
      <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
        <div>
          <h2 className="text-2xl font-bold tracking-tight text-slate-800">
            Monitor
          </h2>
          <p className="mt-1 text-sm text-slate-500">
            Video stream, realtime map, planner traces, and route control.
          </p>
          <p className="mt-1 text-xs text-slate-500">
            {operationMode === "sim"
              ? "Navigation dispatch is disabled in Simulation mode."
              : "Monitor controls follow the shared system mode."}
          </p>
          {raiNavigation.systemRuntime && (
            <p className="mt-1 text-xs text-slate-500">
              API target: {raiNavigation.systemRuntime.device_label} (
              {raiNavigation.systemRuntime.device_role})
            </p>
          )}
        </div>
        <div className="self-start sm:self-auto">
          <StatusBadge
          status={
            monitor.live ? "success" : monitor.state === "error" ? "error" : "warning"
          }
          >
            {monitor.live ? "LIVE" : monitor.state.toUpperCase()}
          </StatusBadge>
        </div>
      </div>

      <div className="grid grid-cols-1 gap-5 xl:grid-cols-[minmax(0,1.2fr)_minmax(0,1fr)]">
        <VideoPanel
          live={monitor.live}
          message={monitor.message}
          state={monitor.state}
          humanCount={monitor.telemetry?.human_count ?? 0}
          videoRef={monitor.videoRef}
        />
        <RealtimeMapPanel
          mapCanvasRef={monitor.mapCanvasRef}
          mapData={monitor.mapData}
          raiNavigationRunning={raiNavigation.raiNavigationConfig.running}
          paths={monitor.paths}
          selectionMode={route.selectionMode}
          statusMessage={route.statusMessage}
          onMapClick={route.handleMapClick}
        />
      </div>

      <div className="grid grid-cols-1 gap-5 xl:grid-cols-[minmax(0,1fr)_minmax(0,1fr)]">
        <RoutePanel
          busy={route.busy}
          raiNavigationRunning={raiNavigation.raiNavigationConfig.running}
          routeGoalPose={route.routeGoalPose}
          runSelectedRoute={route.runSelectedRoute}
          clearRoute={route.clearRoute}
          cancelGoal={route.cancelGoal}
          openRouteSelection={route.openRouteSelection}
        />
        <AnchorPanel
          anchors={route.anchors}
          busy={route.busy}
          goHome={route.goHome}
          openHomePoseSelection={route.openHomePoseSelection}
          openInitialPoseSelection={route.openInitialPoseSelection}
        />
      </div>

      <MonitorMessage message={monitor.message} telemetry={monitor.telemetry} />

      <div className="rounded-md border border-slate-200 bg-white px-4 py-3 text-sm text-slate-700">
        {route.statusMessage}
      </div>

      <RaiNavigationPlannerPanel
        busy={raiNavigation.busy}
        message={raiNavigation.message}
        systemRuntime={raiNavigation.systemRuntime}
        raiNavigationConfig={raiNavigation.raiNavigationConfig}
        raiNavigationMaps={raiNavigation.raiNavigationMaps}
        selectedMapId={raiNavigation.selectedMapId}
        raiNavigationGlobalOptions={raiNavigation.raiNavigationGlobalOptions}
        raiNavigationLocalOptions={raiNavigation.raiNavigationLocalOptions}
        loadRaiNavigationState={raiNavigation.loadRaiNavigationState}
        updateRaiNavigationConfig={raiNavigation.updateRaiNavigationConfig}
        selectRaiNavigationMap={handleSelectRaiNavigationMap}
        startRaiNavigation={raiNavigation.startRaiNavigation}
        stopRaiNavigation={raiNavigation.stopRaiNavigation}
      />

      <MapPoseDialog
        open={route.poseDialogOpen}
        title={route.poseDialogTitle}
        description={route.poseDialogDescription}
        pose={route.poseDraft}
        busy={route.busy}
        onClose={route.closePoseDialog}
        onChange={route.setPoseDraft}
        onConfirm={() => void route.confirmPoseDraft()}
      />
    </div>
  );
}
