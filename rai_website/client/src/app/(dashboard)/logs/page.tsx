"use client";

import { StatusBadge } from "@/components/ui/StatusBadge";
import { Button } from "@/components/ui/button";
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuRadioGroup,
  DropdownMenuRadioItem,
  DropdownMenuTrigger,
} from "@/components/ui/dropdown-menu";
import { fetchWithAuth } from "@/lib/api";
import { ChevronDown, FileText, RefreshCcw, Search } from "lucide-react";
import { useCallback, useEffect, useMemo, useState } from "react";

interface EventLog {
  id: number | string;
  event_type?: string;
  severity: string;
  source: string;
  message: string;
  created_at: string;
}

const sources = [
  { label: "All sources", value: "" },
  { label: "adaptive-context-aware", value: "adaptive-context-aware" },
  { label: "wheeltec_ros2", value: "wheeltec_ros2" },
  { label: "rai_website.server", value: "rai_website.server" },
  { label: "rai_website.client", value: "rai_website.client" },
  { label: "database", value: "database" },
];

const severities = [
  { label: "All levels", value: "" },
  { label: "Info", value: "INFO" },
  { label: "Warning", value: "WARNING" },
  { label: "Error", value: "ERROR" },
  { label: "Critical", value: "CRITICAL" },
];

const ALL_VALUE = "__all";

function severityStatus(severity: string): "error" | "warning" | "info" {
  const level = severity.toUpperCase();
  if (level === "ERROR" || level === "CRITICAL") return "error";
  if (level === "WARNING") return "warning";
  return "info";
}

function FilterDropdown({
  value,
  options,
  onChange,
}: {
  value: string;
  options: Array<{ label: string; value: string }>;
  onChange: (value: string) => void;
}) {
  const selected = options.find((item) => item.value === value) ?? options[0];
  return (
    <DropdownMenu>
      <DropdownMenuTrigger asChild>
        <Button
          type="button"
          variant="outline"
          className="h-10 w-full justify-between rounded-lg border-slate-300 bg-white px-3 text-slate-700 shadow-sm"
        >
          <span className="truncate">{selected.label}</span>
          <ChevronDown className="h-4 w-4 text-slate-400" />
        </Button>
      </DropdownMenuTrigger>
      <DropdownMenuContent className="w-(--radix-dropdown-menu-trigger-width)">
        <DropdownMenuRadioGroup
          value={value || ALL_VALUE}
          onValueChange={(nextValue) => onChange(nextValue === ALL_VALUE ? "" : nextValue)}
        >
          {options.map((item) => (
            <DropdownMenuRadioItem key={item.value || ALL_VALUE} value={item.value || ALL_VALUE}>
              {item.label}
            </DropdownMenuRadioItem>
          ))}
        </DropdownMenuRadioGroup>
      </DropdownMenuContent>
    </DropdownMenu>
  );
}

export default function LogsPage() {
  const [logs, setLogs] = useState<EventLog[]>([]);
  const [source, setSource] = useState("");
  const [severity, setSeverity] = useState("");
  const [query, setQuery] = useState("");
  const [loading, setLoading] = useState(false);
  const [lastUpdated, setLastUpdated] = useState<Date | null>(null);

  const endpoint = useMemo(() => {
    const params = new URLSearchParams({ limit: "200", include_external: "true" });
    if (source) params.set("source", source);
    if (severity) params.set("severity", severity);
    if (query.trim()) params.set("query", query.trim());
    return `/api/logs/?${params.toString()}`;
  }, [query, severity, source]);

  const loadLogs = useCallback(async () => {
    setLoading(true);
    try {
      const res = await fetchWithAuth(endpoint);
      const data = (await res.json()) as EventLog[];
      setLogs(data);
      setLastUpdated(new Date());
    } catch {
      setLogs([]);
    } finally {
      setLoading(false);
    }
  }, [endpoint]);

  useEffect(() => {
    const initialLoad = window.setTimeout(() => void loadLogs(), 0);
    const interval = window.setInterval(loadLogs, 5000);
    return () => {
      window.clearTimeout(initialLoad);
      window.clearInterval(interval);
    };
  }, [loadLogs]);

  return (
    <div className="space-y-6 flex flex-col h-full">
      <div className="flex flex-col gap-4">
        <div className="flex flex-col sm:flex-row sm:items-center sm:justify-between gap-3">
          <div>
            <h2 className="text-2xl font-bold text-slate-800 tracking-tight">System Logs</h2>
            <p className="mt-1 text-sm text-slate-500">
              {lastUpdated ? `Updated ${lastUpdated.toLocaleTimeString()}` : "Waiting for log sources"}
            </p>
          </div>
          <button
            onClick={loadLogs}
            disabled={loading}
            className="flex items-center gap-2 px-4 py-2 bg-white border border-slate-300 text-slate-700 rounded-lg hover:bg-slate-50 disabled:opacity-60 transition shadow-sm text-sm font-medium"
          >
            <RefreshCcw className={`h-4 w-4 ${loading ? "animate-spin" : ""}`} />
            Refresh
          </button>
        </div>

        <div className="grid grid-cols-1 gap-3 md:grid-cols-[minmax(180px,220px)_minmax(160px,200px)_1fr]">
          <FilterDropdown value={source} options={sources} onChange={setSource} />
          <FilterDropdown value={severity} options={severities} onChange={setSeverity} />
          <div className="relative">
            <Search className="pointer-events-none absolute left-3 top-1/2 h-4 w-4 -translate-y-1/2 text-slate-400" />
            <input
              value={query}
              onChange={(event) => setQuery(event.target.value)}
              placeholder="Search logs"
              className="h-10 w-full rounded-lg border border-slate-300 bg-white pl-9 pr-3 text-sm text-slate-700 shadow-sm outline-none focus:border-slate-500"
            />
          </div>
        </div>
      </div>

      <div className="bg-white border border-slate-200 rounded-xl shadow-sm flex-1 overflow-hidden flex flex-col">
        <div className="overflow-x-auto flex-1">
          <table className="w-full text-sm text-left text-slate-600">
            <thead className="text-xs text-slate-400 uppercase bg-slate-50 border-b border-slate-200 sticky top-0 z-10">
              <tr>
                <th className="px-6 py-4 font-semibold w-48">Timestamp</th>
                <th className="px-6 py-4 font-semibold w-28">Level</th>
                <th className="px-6 py-4 font-semibold w-48">Source</th>
                <th className="px-6 py-4 font-semibold w-40">Event</th>
                <th className="px-6 py-4 font-semibold">Message</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-100 font-mono text-[13px]">
              {logs.length === 0 ? (
                <tr>
                  <td colSpan={5} className="px-6 py-10 text-center text-slate-400 font-medium font-sans">
                    <FileText className="w-8 h-8 mx-auto mb-3 opacity-50" />
                    No logs returned from the selected sources.
                  </td>
                </tr>
              ) : (
                logs.map((log) => (
                  <tr key={`${log.source}-${log.id}-${log.created_at}`} className="align-top">
                    <td className="px-6 py-3 whitespace-nowrap">{new Date(log.created_at).toLocaleString()}</td>
                    <td className="px-6 py-3">
                      <StatusBadge status={severityStatus(log.severity)}>{log.severity.toUpperCase()}</StatusBadge>
                    </td>
                    <td className="px-6 py-3 whitespace-nowrap">{log.source}</td>
                    <td className="px-6 py-3 whitespace-nowrap">{log.event_type ?? "-"}</td>
                    <td className="px-6 py-3 min-w-[320px] whitespace-pre-wrap break-words">{log.message}</td>
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
