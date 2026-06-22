"use client";

import { Button } from "@/components/ui/button";
import { DropdownField } from "@/components/ui/dropdown-field";
import { Input } from "@/components/ui/input";
import { useOperationMode } from "@/contexts/OperationModeContext";
import { useRvizRuntime } from "@/hooks/useRvizRuntime";
import type { MapPayload, PathsPayload, RobotModelOption, RvizTopics } from "@/types/robot-runtime";
import type { RobotUiTelemetry } from "@/lib/robot-telemetry";
import { Bot, Compass, Map as MapIcon, RefreshCw, Save, ScanSearch, Users, Waypoints } from "lucide-react";
import type { RefObject } from "react";

export default function Rviz2Page() {
  const { operationMode } = useOperationMode();
  const rviz = useRvizRuntime();
  const {
    currentMap,
    applyTopics,
    loadMaps,
    maps,
    mountRef,
    paths,
    robotModels,
    selectedRobotModelId,
    selectRobotModel,
    selectSavedMap,
    setTopicDraft,
    setViewMode,
    status,
    telemetry,
    topicDraft,
    topics,
    viewMode,
  } = rviz;

  return (
    <div className="space-y-5">
      <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
        <div>
          <h2 className="text-2xl font-bold tracking-tight text-slate-800">RViz2 View</h2>
          <p className="text-sm text-slate-500">Map, TF-style robot model, planners, and live scene overlays in an RViz-oriented viewer.</p>
          <p className="mt-1 text-xs text-slate-500">
            {operationMode === "real" ? "RViz is available as a live inspection view." : operationMode === "hybrid" ? "RViz is available for hybrid overlay workflows." : "RViz stays in read-oriented simulation mode."}
          </p>
        </div>
        <div className="flex flex-wrap gap-2 max-sm:[&_[data-slot=button]]:flex-1">
          <Button variant={viewMode === "perspective" ? "default" : "outline"} className="gap-2" onClick={() => setViewMode("perspective")}>
            <Compass className="size-4" />
            Perspective
          </Button>
          <Button variant={viewMode === "top" ? "default" : "outline"} className="gap-2" onClick={() => setViewMode("top")}>
            <MapIcon className="size-4" />
            Top
          </Button>
          <Button variant={viewMode === "follow" ? "default" : "outline"} className="gap-2" onClick={() => setViewMode("follow")}>
            <ScanSearch className="size-4" />
            Follow
          </Button>
          <Button variant="outline" className="gap-2" onClick={() => void loadMaps()}>
            <RefreshCw className="size-4" />
            Refresh
          </Button>
        </div>
      </div>

      <div className="grid grid-cols-1 gap-5 xl:grid-cols-[minmax(0,1fr)_340px]">
        <RvizScene mountRef={mountRef} />
        <RvizSidebar
          currentMap={currentMap}
          maps={maps}
          paths={paths}
          robotModels={robotModels}
          selectedRobotModelId={selectedRobotModelId}
          selectRobotModel={selectRobotModel}
          selectSavedMap={selectSavedMap}
          setTopicDraft={setTopicDraft}
          status={status}
          telemetry={telemetry}
          topicDraft={topicDraft}
          topics={topics}
          applyTopics={applyTopics}
        />
      </div>
    </div>
  );
}

interface RvizSceneProps {
  mountRef: RefObject<HTMLDivElement | null>;
}

function RvizScene({ mountRef }: RvizSceneProps) {
  return (
    <section className="overflow-hidden rounded-lg border border-slate-200 bg-white shadow-sm">
      <div className="flex items-center gap-2 border-b border-slate-200 px-4 py-3 text-sm text-slate-600">
        <MapIcon className="size-4 text-blue-600" />
        <span className="font-medium text-slate-800">RViz2 Scene</span>
        <span className="ml-auto text-xs text-slate-400">Grid + Map + TF + Path + Humans</span>
      </div>
      <div ref={mountRef} className="h-[420px] w-full bg-slate-100 sm:h-[560px] xl:h-[760px]" />
    </section>
  );
}

interface RvizSidebarProps {
  currentMap: MapPayload | null;
  maps: MapPayload[];
  paths: PathsPayload;
  robotModels: RobotModelOption[];
  selectedRobotModelId: string;
  selectRobotModel: (modelId: string) => void;
  selectSavedMap: (map: MapPayload) => Promise<void>;
  setTopicDraft: (value: RvizTopics) => void;
  status: string;
  telemetry: RobotUiTelemetry | null;
  topicDraft: RvizTopics;
  topics: RvizTopics;
  applyTopics: () => Promise<void>;
}

function RvizSidebar({
  currentMap,
  maps,
  paths,
  robotModels,
  selectedRobotModelId,
  selectRobotModel,
  selectSavedMap,
  setTopicDraft,
  status,
  telemetry,
  topicDraft,
  topics,
  applyTopics,
}: RvizSidebarProps) {
  return (
    <aside className="space-y-4">
      <RobotPanel
        currentMap={currentMap}
        robotModels={robotModels}
        selectedRobotModelId={selectedRobotModelId}
        selectRobotModel={selectRobotModel}
        telemetry={telemetry}
      />
      <TopicPanel applyTopics={applyTopics} setTopicDraft={setTopicDraft} topicDraft={topicDraft} topics={topics} />
      <RuntimePanel paths={paths} telemetry={telemetry} />
      <SavedMapsPanel maps={maps} selectSavedMap={selectSavedMap} />
      <section className="rounded-lg border border-slate-200 bg-white p-4 text-sm text-slate-600 shadow-sm">{status}</section>
    </aside>
  );
}

function TopicPanel({
  applyTopics,
  setTopicDraft,
  topicDraft,
  topics,
}: {
  applyTopics: () => Promise<void>;
  setTopicDraft: (value: RvizTopics) => void;
  topicDraft: RvizTopics;
  topics: RvizTopics;
}) {
  const updateField = (key: keyof RvizTopics, value: string) => setTopicDraft({ ...topicDraft, [key]: value });
  return (
    <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
      <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">Topics</h3>
      <div className="space-y-2">
        <TopicInput label="Map" value={topicDraft.map_topic} onChange={(value) => updateField("map_topic", value)} />
        <TopicInput label="Global path" value={topicDraft.global_path_topic} onChange={(value) => updateField("global_path_topic", value)} />
        <TopicInput label="Local path" value={topicDraft.local_path_topic} onChange={(value) => updateField("local_path_topic", value)} />
        <Button type="button" variant="outline" className="w-full gap-2" onClick={() => void applyTopics()}>
          <Save className="size-4" />
          Apply Topics
        </Button>
        <p className="text-xs text-slate-400">
          Active: {topics.map_topic}, {topics.global_path_topic}, {topics.local_path_topic}
        </p>
      </div>
    </section>
  );
}

function TopicInput({ label, onChange, value }: { label: string; onChange: (value: string) => void; value: string }) {
  return (
    <label className="block">
      <span className="text-xs font-medium text-slate-500">{label}</span>
      <Input value={value} onChange={(event) => onChange(event.target.value)} className="mt-1 font-mono text-xs" />
    </label>
  );
}

function RobotPanel({
  currentMap,
  robotModels,
  selectedRobotModelId,
  selectRobotModel,
  telemetry,
}: {
  currentMap: MapPayload | null;
  robotModels: RobotModelOption[];
  selectedRobotModelId: string;
  selectRobotModel: (modelId: string) => void;
  telemetry: RobotUiTelemetry | null;
}) {
  const selectedModel = robotModels.find((model) => model.id === selectedRobotModelId);
  return (
    <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
      <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">Robot Model</h3>
      <div className="space-y-2 text-sm text-slate-600">
        <label className="block">
          <span className="text-xs font-medium text-slate-500">URDF model</span>
          <DropdownField
            value={selectedRobotModelId}
            onValueChange={selectRobotModel}
            options={robotModels.map((model) => ({ value: model.id, label: model.label }))}
            placeholder="Select robot model"
          />
        </label>
        <div className="flex min-w-0 items-center gap-2">
          <Bot className="size-4 text-emerald-600" />
          <span className="min-w-0 break-words">URDF: `{selectedModel?.urdf_path ?? `${selectedRobotModelId}.urdf`}`</span>
        </div>
        <div className="flex min-w-0 items-center gap-2">
          <MapIcon className="size-4 text-blue-600" />
          <span className="min-w-0 break-words">{currentMap ? `${currentMap.width}x${currentMap.height} @ ${currentMap.resolution}m` : "No map loaded"}</span>
        </div>
        <div className="rounded-md border border-slate-200 bg-slate-50 px-3 py-2 text-xs text-slate-500">
          Pose: x={formatPose(telemetry?.map_pose.x)}, y={formatPose(telemetry?.map_pose.y)}, yaw={formatPose(telemetry?.map_pose.yaw)}
        </div>
      </div>
    </section>
  );
}

function RuntimePanel({ paths, telemetry }: { paths: PathsPayload; telemetry: RobotUiTelemetry | null }) {
  return (
    <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
      <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">Overlays</h3>
      <div className="space-y-2 text-sm text-slate-600">
        <div className="flex items-center gap-2">
          <Waypoints className="size-4 text-blue-600" />
          <span>Global plan: {paths.global_plan?.length ?? 0}</span>
        </div>
        <div className="flex items-center gap-2">
          <Waypoints className="size-4 text-amber-600" />
          <span>Local plan: {paths.local_plan?.length ?? 0}</span>
        </div>
        <div className="flex items-center gap-2">
          <Users className="size-4 text-rose-600" />
          <span>Humans: {telemetry?.humans.length ?? 0}</span>
        </div>
      </div>
    </section>
  );
}

function SavedMapsPanel({ maps, selectSavedMap }: { maps: MapPayload[]; selectSavedMap: (map: MapPayload) => Promise<void> }) {
  return (
    <section className="rounded-lg border border-slate-200 bg-white p-4 shadow-sm">
      <h3 className="mb-3 text-sm font-bold uppercase tracking-wider text-slate-400">Saved Maps</h3>
      <div className="space-y-2">
        {maps.map((item) => (
          <button
            key={item.id ?? item.name}
            type="button"
            className="w-full rounded-md border border-slate-200 bg-white p-3 text-left hover:bg-slate-50"
            onClick={() => void selectSavedMap(item)}
          >
            <div className="font-semibold text-slate-800">{item.name ?? `Map #${item.id}`}</div>
            <div className="mt-1 text-xs text-slate-400">
              {item.width}x{item.height}
            </div>
            <div className="mt-1 truncate text-xs text-slate-400">{item.yaml_path ?? "-"}</div>
          </button>
        ))}
      </div>
    </section>
  );
}

function formatPose(value: number | null | undefined) {
  return typeof value === "number" && Number.isFinite(value) ? value.toFixed(3) : "-";
}
