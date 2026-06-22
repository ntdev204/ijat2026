"use client";

import { Button } from "@/components/ui/button";
import { DropdownField } from "@/components/ui/dropdown-field";
import { Input } from "@/components/ui/input";
import { useOperationMode } from "@/contexts/OperationModeContext";
import { useGazeboRuntime } from "@/hooks/useGazeboRuntime";
import { Bot, Box, Boxes, Play, Radar, RefreshCw, Square, Waypoints } from "lucide-react";

export default function GazeboPage() {
  const gazebo = useGazeboRuntime();
  const { operationMode } = useOperationMode();
  const simulationAllowed = (gazebo.systemRuntime?.allowed_actions.includes("simulation") ?? true) && operationMode !== "real";

  return (
    <div className="space-y-5">
      <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
        <div>
          <h2 className="text-2xl font-bold tracking-tight text-slate-800">Gazebo</h2>
          <p className="text-sm text-slate-500">
            Virtual worlds, robot model selection, and launch control for the full simulation stack.
          </p>
          <p className="mt-1 text-xs text-slate-500">
            Mode gate: {operationMode === "real" ? "Gazebo is disabled in Real mode." : operationMode === "hybrid" ? "Hybrid allows a real robot with Gazebo or RViz environment overlays." : "Simulation tools are fully enabled."}
          </p>
          {gazebo.systemRuntime && (
            <p className="mt-1 text-xs text-slate-500">
              API target: {gazebo.systemRuntime.device_label} ({gazebo.systemRuntime.device_role})
            </p>
          )}
        </div>
        <Button variant="outline" className="gap-2 self-start sm:self-auto" disabled={gazebo.busy} onClick={() => void gazebo.refresh()}>
          <RefreshCw className="size-4" />
          Refresh
        </Button>
      </div>

      <div className="grid grid-cols-1 gap-5 xl:grid-cols-[minmax(0,1.15fr)_380px]">
        <section className="overflow-hidden rounded-lg border border-slate-200 bg-white shadow-sm">
          <div className="flex items-center gap-2 border-b border-slate-200 px-4 py-3 text-sm text-slate-600">
            <Boxes className="size-4 text-emerald-600" />
            <span className="font-medium text-slate-800">Simulation Runtime</span>
          </div>
          <div className="grid gap-4 p-4 md:grid-cols-3">
            <StatusCard
              icon={<Play className="size-4 text-emerald-600" />}
              label="Runtime"
              value={gazebo.running ? "Running" : "Stopped"}
              detail={gazebo.running ? "Gazebo, bridge, and sim clock are active." : "No simulation process is active."}
            />
            <StatusCard
              icon={<Waypoints className="size-4 text-blue-600" />}
              label="World"
              value={gazebo.worlds.find((world) => world.id === gazebo.selectedWorldId)?.label ?? "No world"}
              detail={gazebo.selectedWorldId || "Choose a package or generated world."}
            />
            <StatusCard
              icon={<Bot className="size-4 text-amber-600" />}
              label="Robot Model"
              value={gazebo.robotModels.find((model) => model.id === gazebo.selectedRobotModelId)?.label ?? "No model"}
              detail={gazebo.selectedRobotModelId}
            />
          </div>
          <div className="border-t border-slate-200 bg-slate-50 px-4 py-4">
            <div className="grid gap-4 lg:grid-cols-[minmax(0,1fr)_320px]">
              <div className="rounded-lg border border-slate-200 bg-white p-4">
                <div className="mb-3 flex items-center gap-2 text-sm font-semibold text-slate-800">
                  <Radar className="size-4 text-blue-600" />
                  Simulation Playbook
                </div>
                <div className="space-y-3 text-sm text-slate-600">
                  <p>
                    Start the simulation here, then switch to <span className="font-medium text-slate-800">Map</span> to run 2D scan mapping and to{" "}
                    <span className="font-medium text-slate-800">RViz2</span> to inspect TF, topics, and trajectories.
                  </p>
                  <p>
                    The launch uses the simulation profile only, so it does not pull serial, lidar USB, IMU hardware, or real-robot bringup.
                  </p>
                  <div className="flex flex-wrap gap-2 max-sm:[&_[data-slot=button]]:flex-1">
                    <Button
                      type="button"
                      className="gap-2"
                      disabled={gazebo.busy || !simulationAllowed || gazebo.running || !gazebo.selectedWorldId}
                      onClick={() => void gazebo.startSimulation()}
                    >
                      <Play className="size-4" />
                      Start Gazebo
                    </Button>
                    <Button
                      type="button"
                      variant="outline"
                      className="gap-2"
                      disabled={gazebo.busy || !simulationAllowed || !gazebo.running}
                      onClick={() => void gazebo.stopSimulation()}
                    >
                      <Square className="size-4" />
                      Stop
                    </Button>
                  </div>
                </div>
              </div>

              <div className="rounded-lg border border-slate-200 bg-white p-4">
                <div className="mb-3 text-sm font-semibold text-slate-800">Active Status</div>
                <p className="text-sm leading-6 text-slate-600">{gazebo.status}</p>
              </div>
            </div>
          </div>
        </section>

        <aside className="space-y-4">
          <LaunchPanel
            robotModels={gazebo.robotModels}
            selectedRobotModelId={gazebo.selectedRobotModelId}
            selectedWorldId={gazebo.selectedWorldId}
            setSelectedRobotModelId={gazebo.setSelectedRobotModelId}
            setSelectedWorldId={gazebo.setSelectedWorldId}
            simulationAllowed={simulationAllowed}
            worlds={gazebo.worlds}
          />
          <WorldBuilderPanel
            busy={gazebo.busy}
            corridor={gazebo.corridor}
            createWorld={gazebo.createWorld}
            obstacleCount={gazebo.obstacleCount}
            setCorridor={gazebo.setCorridor}
            setObstacleCount={gazebo.setObstacleCount}
            setWorldHeight={gazebo.setWorldHeight}
            setWorldName={gazebo.setWorldName}
            setWorldWidth={gazebo.setWorldWidth}
            simulationAllowed={simulationAllowed}
            worldHeight={gazebo.worldHeight}
            worldName={gazebo.worldName}
            worldWidth={gazebo.worldWidth}
          />
        </aside>
      </div>
    </div>
  );
}

function StatusCard({
  detail,
  icon,
  label,
  value,
}: {
  detail: string;
  icon: React.ReactNode;
  label: string;
  value: string;
}) {
  return (
    <div className="rounded-lg border border-slate-200 bg-slate-50 p-4">
      <div className="mb-3 flex items-center gap-2 text-sm text-slate-500">
        {icon}
        <span>{label}</span>
      </div>
      <div className="text-lg font-semibold text-slate-900">{value}</div>
      <p className="mt-2 text-sm leading-6 text-slate-500">{detail}</p>
    </div>
  );
}

function LaunchPanel({
  robotModels,
  selectedRobotModelId,
  selectedWorldId,
  setSelectedRobotModelId,
  setSelectedWorldId,
  simulationAllowed,
  worlds,
}: {
  robotModels: Array<{ id: string; label: string }>;
  selectedRobotModelId: string;
  selectedWorldId: string;
  setSelectedRobotModelId: (value: string) => void;
  setSelectedWorldId: (value: string) => void;
  simulationAllowed: boolean;
  worlds: Array<{ id: string; label: string; source: string }>;
}) {
  return (
    <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
      <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">Launch Profile</h3>
      <div className="space-y-3">
        <label className="block text-xs font-medium text-slate-500">
          World
          <DropdownField
            value={selectedWorldId}
            onValueChange={setSelectedWorldId}
            options={worlds.map((world) => ({ value: world.id, label: `${world.label} (${world.source})` }))}
            placeholder="Select world"
          />
        </label>
        <label className="block text-xs font-medium text-slate-500">
          Robot model
          <DropdownField
            value={selectedRobotModelId}
            onValueChange={setSelectedRobotModelId}
            options={robotModels.map((model) => ({ value: model.id, label: model.label }))}
            placeholder="Select robot model"
          />
        </label>
        <div className="rounded-md border border-slate-200 bg-slate-50 px-3 py-3 text-sm text-slate-600">
          This profile starts Gazebo, ROS-GZ bridge, simulated clock, and SLAM-friendly topics. Navigation can stay off until you pick a scanned map.
        </div>
        <p className="text-xs text-slate-500">
          {simulationAllowed ? "Simulation actions are enabled for this API target." : "Simulation actions are blocked on the current API role."}
        </p>
      </div>
    </section>
  );
}

function WorldBuilderPanel({
  busy,
  corridor,
  createWorld,
  obstacleCount,
  setCorridor,
  setObstacleCount,
  setWorldHeight,
  setWorldName,
  setWorldWidth,
  simulationAllowed,
  worldHeight,
  worldName,
  worldWidth,
}: {
  busy: boolean;
  corridor: boolean;
  createWorld: () => Promise<void>;
  obstacleCount: number;
  setCorridor: (value: boolean) => void;
  setObstacleCount: (value: number) => void;
  setWorldHeight: (value: number) => void;
  setWorldName: (value: string) => void;
  setWorldWidth: (value: number) => void;
  simulationAllowed: boolean;
  worldHeight: number;
  worldName: string;
  worldWidth: number;
}) {
  return (
    <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
      <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">World Builder</h3>
      <div className="space-y-3">
        <Input value={worldName} onChange={(event) => setWorldName(event.target.value)} placeholder="World name" disabled={busy} />
        <div className="grid grid-cols-1 gap-2 min-[380px]:grid-cols-3">
          <NumericInput label="Width" value={worldWidth} min={2} max={50} onChange={setWorldWidth} />
          <NumericInput label="Height" value={worldHeight} min={2} max={50} onChange={setWorldHeight} />
          <NumericInput label="Boxes" value={obstacleCount} min={0} max={24} onChange={setObstacleCount} />
        </div>
        <label className="flex items-center gap-2 text-sm text-slate-600">
          <input
            type="checkbox"
            checked={corridor}
            onChange={(event) => setCorridor(event.target.checked)}
            disabled={busy}
            className="size-4 rounded border-slate-300"
          />
          Add corridor walls
        </label>
        <Button type="button" className="w-full gap-2" disabled={busy || !simulationAllowed} onClick={() => void createWorld()}>
          <Box className="size-4" />
          Create Virtual World
        </Button>
      </div>
    </section>
  );
}

function NumericInput({
  label,
  max,
  min,
  onChange,
  value,
}: {
  label: string;
  max: number;
  min: number;
  onChange: (value: number) => void;
  value: number;
}) {
  return (
    <label className="block">
      <span className="text-[11px] font-medium text-slate-500">{label}</span>
      <Input type="number" min={min} max={max} value={value} onChange={(event) => onChange(Number(event.target.value))} />
    </label>
  );
}
