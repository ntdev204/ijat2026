"use client";

import { StatusBadge } from "@/components/ui/StatusBadge";
import { Button } from "@/components/ui/button";
import { fetchWithAuth } from "@/lib/api";
import { BrainCircuit, Play, RefreshCw } from "lucide-react";
import { useCallback, useEffect, useState } from "react";

interface TrainingStatus {
  running: boolean;
  progress: number;
  epoch: number;
  loss: number;
  accuracy: number;
  history: Array<{ epoch: number; loss: number; accuracy: number }>;
}

const initialStatus: TrainingStatus = {
  running: false,
  progress: 0,
  epoch: 0,
  loss: 0,
  accuracy: 0,
  history: [],
};

export default function TrainingPage() {
  const [status, setStatus] = useState<TrainingStatus>(initialStatus);
  const [epochs, setEpochs] = useState(50);
  const [learningRate, setLearningRate] = useState(0.001);
  const [batchSize, setBatchSize] = useState(32);
  const [architecture, setArchitecture] = useState("ResNet18");
  const [busy, setBusy] = useState(false);
  const [message, setMessage] = useState("");

  const loadStatus = useCallback(async () => {
    const response = await fetchWithAuth("/api/train/status");
    setStatus((await response.json()) as TrainingStatus);
  }, []);

  useEffect(() => {
    const timer = window.setTimeout(() => {
      void loadStatus().catch(() => setMessage("Cannot reach rai_web_api training status."));
    }, 0);
    return () => window.clearTimeout(timer);
  }, [loadStatus]);

  useEffect(() => {
    if (!status.running) return;
    const timer = window.setInterval(() => void loadStatus().catch(() => undefined), 1000);
    return () => window.clearInterval(timer);
  }, [loadStatus, status.running]);

  async function startTraining() {
    setBusy(true);
    setMessage("");
    try {
      const response = await fetchWithAuth("/api/train/start", {
        method: "POST",
        body: JSON.stringify({
          epochs,
          learning_rate: learningRate,
          batch_size: batchSize,
          architecture,
        }),
      });
      const payload = await response.json();
      setMessage(payload.message ?? "Training started.");
      await loadStatus();
    } catch (error) {
      setMessage(error instanceof Error ? error.message : "Cannot start training.");
    } finally {
      setBusy(false);
    }
  }

  return (
    <div className="space-y-6">
      <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
        <div>
          <h2 className="text-2xl font-bold tracking-tight text-slate-800">Training</h2>
          <p className="mt-1 text-sm text-slate-500">FastAPI training simulator from rai_web_api.</p>
        </div>
        <StatusBadge status={status.running ? "success" : "default"}>
          {status.running ? "RUNNING" : "IDLE"}
        </StatusBadge>
      </div>

      <section className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
        <div className="mb-4 flex items-center gap-2 text-sm font-semibold text-slate-800">
          <BrainCircuit className="h-4 w-4 text-blue-600" />
          Job Parameters
        </div>
        <div className="grid gap-3 md:grid-cols-4">
          <NumberInput label="Epochs" value={epochs} onChange={setEpochs} />
          <NumberInput label="Learning rate" value={learningRate} step="0.0001" onChange={setLearningRate} />
          <NumberInput label="Batch size" value={batchSize} onChange={setBatchSize} />
          <label className="block text-sm">
            <span className="text-xs font-medium text-slate-500">Architecture</span>
            <input
              value={architecture}
              onChange={(event) => setArchitecture(event.target.value)}
              className="mt-1 w-full rounded-md border border-slate-300 px-3 py-2 text-sm"
            />
          </label>
        </div>
        <div className="mt-4 flex gap-2">
          <Button type="button" onClick={startTraining} disabled={busy || status.running} className="gap-2">
            <Play className="h-4 w-4" />
            Start
          </Button>
          <Button type="button" variant="outline" onClick={() => void loadStatus()} disabled={busy} className="gap-2">
            <RefreshCw className="h-4 w-4" />
            Refresh
          </Button>
        </div>
      </section>

      <section className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
        <div className="mb-2 flex justify-between text-sm text-slate-500">
          <span>Progress</span>
          <span>{status.progress}%</span>
        </div>
        <div className="h-2 overflow-hidden rounded-full bg-slate-100">
          <div className="h-full bg-blue-600 transition-all" style={{ width: `${status.progress}%` }} />
        </div>
        <div className="mt-4 grid grid-cols-2 gap-3 md:grid-cols-4">
          <Metric label="Epoch" value={String(status.epoch)} />
          <Metric label="Loss" value={status.loss.toFixed(4)} />
          <Metric label="Accuracy" value={`${(status.accuracy * 100).toFixed(1)}%`} />
          <Metric label="History" value={`${status.history.length} rows`} />
        </div>
      </section>

      {message && <div className="rounded-md border border-slate-200 bg-white px-4 py-3 text-sm text-slate-700">{message}</div>}
    </div>
  );
}

function NumberInput({
  label,
  value,
  step = "1",
  onChange,
}: {
  label: string;
  value: number;
  step?: string;
  onChange: (value: number) => void;
}) {
  return (
    <label className="block text-sm">
      <span className="text-xs font-medium text-slate-500">{label}</span>
      <input
        type="number"
        step={step}
        value={value}
        onChange={(event) => onChange(Number(event.target.value))}
        className="mt-1 w-full rounded-md border border-slate-300 px-3 py-2 text-sm"
      />
    </label>
  );
}

function Metric({ label, value }: { label: string; value: string }) {
  return (
    <div className="rounded-lg border border-slate-200 bg-slate-50 px-4 py-3">
      <div className="text-xs text-slate-500">{label}</div>
      <div className="mt-1 text-sm font-semibold text-slate-900">{value}</div>
    </div>
  );
}
