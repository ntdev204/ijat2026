"use client";

import { StatusBadge } from "@/components/ui/StatusBadge";
import { fetchWithAuth } from "@/lib/api";
import { Database, RotateCcw, Server, Wifi } from "lucide-react";
import { useCallback, useEffect, useState } from "react";

interface NodeState {
  id: number;
  node_name: string;
  package_name?: string | null;
  status: string;
  source?: string;
  last_changed_at?: string | null;
}

export default function NodesPage() {
  const [nodes, setNodes] = useState<NodeState[]>([]);
  const [loading, setLoading] = useState(false);
  const [lastUpdated, setLastUpdated] = useState<Date | null>(null);

  const loadNodes = useCallback(async () => {
    setLoading(true);
    try {
      const res = await fetchWithAuth("/api/nodes/");
      const data = (await res.json()) as NodeState[];
      setNodes(data);
      setLastUpdated(new Date());
    } catch {
      setNodes([]);
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    const initialLoad = window.setTimeout(() => void loadNodes(), 0);
    const interval = window.setInterval(loadNodes, 5000);
    return () => {
      window.clearTimeout(initialLoad);
      window.clearInterval(interval);
    };
  }, [loadNodes]);

  const isPiSource = nodes.some((node) => node.source === "raspi_ros2");

  return (
    <div className="space-y-6">
      <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
        <div>
          <h2 className="text-2xl font-bold text-slate-800 tracking-tight">System Nodes</h2>
          <div className="mt-2 flex flex-wrap items-center gap-2 text-sm text-slate-500">
            <StatusBadge status={isPiSource ? "success" : "warning"}>
              {isPiSource ? "raspi_ros2 live" : "database fallback"}
            </StatusBadge>
            {lastUpdated && <span>Updated {lastUpdated.toLocaleTimeString()}</span>}
          </div>
        </div>
        <button
          onClick={loadNodes}
          disabled={loading}
          className="flex items-center gap-2 px-4 py-2 bg-white border border-slate-300 text-slate-700 rounded-lg hover:bg-slate-50 disabled:opacity-60 transition shadow-sm text-sm font-medium"
        >
          <RotateCcw className={`w-4 h-4 ${loading ? "animate-spin" : ""}`} /> Refresh
        </button>
      </div>

      <div className="bg-white border border-slate-200 rounded-xl shadow-sm overflow-hidden">
        <div className="overflow-x-auto">
          <table className="w-full text-sm text-left text-slate-600">
            <thead className="text-xs text-slate-400 uppercase bg-slate-50 border-b border-slate-200">
              <tr>
                <th className="px-6 py-4 font-semibold">Node Name</th>
                <th className="px-6 py-4 font-semibold">Package</th>
                <th className="px-6 py-4 font-semibold">Status</th>
                <th className="px-6 py-4 font-semibold">Source</th>
                <th className="px-6 py-4 font-semibold">Last Changed</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-100">
              {nodes.length === 0 ? (
                <tr>
                  <td colSpan={5} className="px-6 py-12 text-center text-slate-400 font-medium">
                    <Server className="w-8 h-8 mx-auto mb-3 opacity-50" />
                    No active ROS2 nodes reported by Raspberry Pi.
                  </td>
                </tr>
              ) : (
                nodes.map((node) => (
                  <tr key={`${node.source ?? "node"}-${node.node_name}-${node.id}`}>
                    <td className="px-6 py-3 font-medium text-slate-800">{node.node_name}</td>
                    <td className="px-6 py-3">{node.package_name ?? "-"}</td>
                    <td className="px-6 py-3">
                      <StatusBadge status={node.status === "active" ? "success" : "default"}>
                        {node.status}
                      </StatusBadge>
                    </td>
                    <td className="px-6 py-3">
                      <span className="inline-flex items-center gap-2">
                        {node.source === "raspi_ros2" ? (
                          <Wifi className="h-4 w-4 text-emerald-600" />
                        ) : (
                          <Database className="h-4 w-4 text-amber-600" />
                        )}
                        {node.source ?? "-"}
                      </span>
                    </td>
                    <td className="px-6 py-3">
                      {node.last_changed_at ? new Date(node.last_changed_at).toLocaleString() : "-"}
                    </td>
                  </tr>
                ))
              )}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  );
}
