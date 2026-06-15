"use client";

import { StatusBadge } from "@/components/ui/StatusBadge";
import { fetchWithAuth } from "@/lib/api";
import {
  Activity, AlertTriangle, BarChart3, Bot, Brain, Download,
  Gauge, RefreshCw, Shield, TrendingUp, Users,
} from "lucide-react";
import { useCallback, useEffect, useMemo, useState } from "react";
import {
  CartesianGrid, Cell, Line, LineChart, Pie, PieChart,
  ResponsiveContainer, Tooltip, XAxis, YAxis,
} from "recharts";

interface Snapshot {
  id: number;
  connected: boolean;
  navigation_mode?: string | null;
  speed?: number | null;
  ai_fps?: number | null;
  ai_persons?: number | null;
  created_at: string;
}

interface AnalyticsSummary {
  collector: { running: boolean; interval_sec: number; retention_hours: number };
  current: Snapshot | null;
  window: {
    hours: number;
    samples: number;
    avg_speed?: number | null;
    max_speed?: number | null;
    avg_ai_fps?: number | null;
    person_observations: number;
    obstacle_observations: number;
    navigation_modes: Record<string, number>;
  };
  logs: {
    by_severity: Record<string, number>;
    recent_alerts: Array<{
      id: number;
      severity: string;
      source: string;
      event_type: string;
      message: string;
      created_at: string;
    }>;
  };
}

function fmt(v: number | null | undefined, digits = 1, suffix = "") {
  if (v == null || Number.isNaN(v)) return "—";
  return `${Number(v).toFixed(digits)}${suffix}`;
}

function percent(part: number, total: number) {
  if (!total) return "0%";
  return `${((part / total) * 100).toFixed(1)}%`;
}

function useCSVDownload() {
  return useCallback(async (endpoint: string, filename: string) => {
    try {
      const res = await fetchWithAuth(endpoint);
      const blob = await res.blob();
      const url = URL.createObjectURL(blob);
      const a = document.createElement("a");
      a.href = url;
      a.download = filename;
      a.click();
      URL.revokeObjectURL(url);
    } catch {
      alert(`Download failed: ${filename}`);
    }
  }, []);
}

function DownloadBtn({ onClick, label }: { onClick: () => void; label: string }) {
  return (
    <button
      onClick={onClick}
      className="flex items-center gap-1 text-xs font-medium text-slate-500 hover:text-blue-600 hover:bg-blue-50 px-2 py-1 rounded-lg transition-colors border border-slate-200 hover:border-blue-200"
    >
      <Download className="w-3 h-3" />
      {label}
    </button>
  );
}

function SkeletonLine({ w = "100%" }: { w?: string }) {
  return <div className="h-3 rounded bg-slate-100 animate-pulse" style={{ width: w }} />;
}

function EmptyChart({ label }: { label: string }) {
  return (
    <div className="h-full flex flex-col items-center justify-center gap-2 border border-dashed border-slate-200 rounded-xl text-slate-400">
      <Activity className="w-6 h-6 opacity-40" />
      <p className="text-xs text-center px-4">{label}</p>
    </div>
  );
}

function SectionHeader({ icon: Icon, title, subtitle, accent, action }: {
  icon: React.ElementType;
  title: string;
  subtitle: string;
  accent: string;
  action?: React.ReactNode;
}) {
  return (
    <div className="flex items-center justify-between gap-3 mb-5">
      <div className="flex items-center gap-3">
        <div className={`p-2.5 rounded-xl ${accent}`}>
          <Icon className="w-5 h-5 text-white" />
        </div>
        <div>
          <h3 className="text-base font-bold text-slate-800 leading-tight">{title}</h3>
          <p className="text-xs text-slate-500 mt-0.5">{subtitle}</p>
        </div>
      </div>
      {action}
    </div>
  );
}

function StatRow({ label, value, badge, badgeOk, loading }: {
  label: string;
  value?: string;
  badge?: string;
  badgeOk?: boolean;
  loading: boolean;
}) {
  return (
    <div className="flex items-center justify-between py-2.5 border-b border-slate-100 last:border-0">
      <span className="text-sm text-slate-500">{label}</span>
      <div className="flex items-center gap-2">
        {loading ? <SkeletonLine w="90px" /> : (
          <>
            <span className="text-sm font-semibold text-slate-900">{value ?? "—"}</span>
            {badge && (
              <span className={`text-xs font-medium px-1.5 py-0.5 rounded-md ${
                badgeOk === true ? "bg-emerald-100 text-emerald-700"
                  : badgeOk === false ? "bg-rose-100 text-rose-700"
                  : "bg-amber-50 text-amber-600"
              }`}>
                {badge}
              </span>
            )}
          </>
        )}
      </div>
    </div>
  );
}

function OverviewCards({ summary, loading }: { summary: AnalyticsSummary | null; loading: boolean }) {
  const current = summary?.current;
  const windowData = summary?.window;
  const alerts = summary?.logs.by_severity ?? {};
  const criticalAlerts = (alerts.ERROR ?? 0) + (alerts.CRITICAL ?? 0);

  const cards = [
    {
      label: "Kết nối robot",
      value: current?.connected ? "Online" : "Offline",
      icon: Bot,
      accent: current?.connected ? "bg-emerald-500" : "bg-rose-500",
      sub: current?.connected ? "telemetry đang cập nhật" : "mất trạng thái live",
    },
    {
      label: "Tốc độ trung bình",
      value: fmt(windowData?.avg_speed, 3, " m/s"),
      icon: Gauge,
      accent: "bg-blue-500",
      sub: `đỉnh ${fmt(windowData?.max_speed, 2, " m/s")}`,
    },
    {
      label: "AI FPS trung bình",
      value: fmt(windowData?.avg_ai_fps, 1, " FPS"),
      icon: Brain,
      accent: "bg-violet-500",
      sub: (windowData?.avg_ai_fps ?? 0) >= 20 ? "đạt realtime" : "cần theo dõi tải",
    },
    {
      label: "Cảnh báo mức cao",
      value: String(criticalAlerts),
      icon: Shield,
      accent: criticalAlerts > 0 ? "bg-amber-500" : "bg-slate-500",
      sub: `trong ${windowData?.hours ?? 24} giờ gần nhất`,
    },
  ];

  return (
    <div className="grid grid-cols-1 md:grid-cols-2 xl:grid-cols-4 gap-4">
      {cards.map((card) => {
        const Icon = card.icon;
        return (
          <div key={card.label} className="rounded-2xl border border-slate-200 bg-white p-5 shadow-sm">
            <div className="flex items-start justify-between gap-3">
              <div>
                <p className="text-sm text-slate-500">{card.label}</p>
                <div className="mt-2">
                  {loading ? <SkeletonLine w="96px" /> : (
                    <p className="text-2xl font-bold tracking-tight text-slate-900">{card.value}</p>
                  )}
                </div>
                <p className="mt-2 text-xs text-slate-400">{card.sub}</p>
              </div>
              <div className={`rounded-xl p-2.5 ${card.accent}`}>
                <Icon className="w-5 h-5 text-white" />
              </div>
            </div>
          </div>
        );
      })}
    </div>
  );
}

function OperationsSection({ summary, series, loading }: {
  summary: AnalyticsSummary | null;
  series: Snapshot[];
  loading: boolean;
}) {
  const current = summary?.current;
  const windowData = summary?.window;
  const chartData = useMemo(
    () => series.map((s) => ({
      t: new Date(s.created_at).toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" }),
      speed: s.speed ?? null,
      persons: s.ai_persons ?? null,
    })),
    [series],
  );

  const currentMode = current?.navigation_mode ?? "—";
  const currentPersons = current?.ai_persons ?? 0;
  const density =
    (windowData?.samples ?? 0) > 0
      ? (windowData?.person_observations ?? 0) / (windowData?.samples ?? 1)
      : 0;

  return (
    <div className="bg-white rounded-2xl border border-slate-200 shadow-sm p-6">
      <SectionHeader
        icon={Gauge}
        title="Phân tích Vận hành"
        subtitle="trạng thái robot · mode điều hướng · vận tốc · mật độ người theo thời gian"
        accent="bg-blue-500"
      />
      <div className="grid grid-cols-1 xl:grid-cols-2 gap-8">
        <div className="space-y-0">
          <StatRow label="Mode điều hướng hiện tại" value={currentMode} loading={loading} />
          <StatRow label="Tốc độ hiện tại" value={fmt(current?.speed, 3, " m/s")} loading={loading} />
          <StatRow label="Người đang quan sát" value={String(currentPersons)} loading={loading} />
          <StatRow
            label="Mật độ người trung bình"
            value={fmt(density, 2, " người / snapshot")}
            badge={(density > 2 ? "cao" : density > 0.8 ? "trung bình" : "thấp")}
            loading={loading}
          />
          <StatRow
            label="Tổng điểm quan sát người"
            value={String(windowData?.person_observations ?? "—")}
            loading={loading}
          />
          <StatRow
            label="Tổng điểm quan sát vật cản"
            value={String(windowData?.obstacle_observations ?? "—")}
            loading={loading}
          />
          {!loading && windowData && (
            <p className="pt-3 text-xs text-slate-400">
              {windowData.samples} snapshots trong cửa sổ {windowData.hours} giờ
            </p>
          )}
        </div>
        <div>
          <p className="text-xs font-medium uppercase tracking-wider text-slate-400 mb-3">
            Vận tốc theo thời gian
          </p>
          <div className="h-[240px]">
            {loading ? <div className="h-full bg-slate-50 rounded-xl animate-pulse" />
              : chartData.length === 0 ? <EmptyChart label="Chưa có snapshot vận hành để vẽ xu hướng" />
              : (
                <ResponsiveContainer width="100%" height="100%">
                  <LineChart data={chartData}>
                    <CartesianGrid strokeDasharray="3 3" stroke="#f1f5f9" />
                    <XAxis dataKey="t" fontSize={10} stroke="#94a3b8" tickLine={false} />
                    <YAxis fontSize={10} stroke="#94a3b8" tickLine={false} width={38} tickFormatter={(v) => Number(v).toFixed(2)} />
                    <Tooltip formatter={(v: unknown) => [`${Number(v).toFixed(3)} m/s`, "Tốc độ"]} />
                    <Line type="monotone" dataKey="speed" stroke="#2563eb" strokeWidth={2} dot={false} connectNulls />
                  </LineChart>
                </ResponsiveContainer>
              )}
          </div>
        </div>
      </div>
    </div>
  );
}

function PerceptionSection({ summary, series, loading }: {
  summary: AnalyticsSummary | null;
  series: Snapshot[];
  loading: boolean;
}) {
  const windowData = summary?.window;
  const fpsData = useMemo(
    () => series.map((s) => ({
      t: new Date(s.created_at).toLocaleTimeString([], { hour: "2-digit", minute: "2-digit" }),
      fps: s.ai_fps ?? null,
      persons: s.ai_persons ?? null,
    })),
    [series],
  );

  const fpsValues = series.map((s) => s.ai_fps).filter((v): v is number => typeof v === "number");
  const minFps = fpsValues.length ? Math.min(...fpsValues) : null;
  const maxFps = fpsValues.length ? Math.max(...fpsValues) : null;
  const below20 = fpsValues.filter((v) => v < 20).length;

  return (
    <div className="bg-white rounded-2xl border border-slate-200 shadow-sm p-6">
      <SectionHeader
        icon={Brain}
        title="Phân tích Perception"
        subtitle="năng lực AI realtime · tải theo số người · độ ổn định pipeline"
        accent="bg-violet-500"
      />
      <div className="grid grid-cols-1 xl:grid-cols-2 gap-8">
        <div className="space-y-0">
          <StatRow label="AI FPS trung bình" value={fmt(windowData?.avg_ai_fps, 1, " FPS")} loading={loading} />
          <StatRow label="AI FPS thấp nhất" value={fmt(minFps, 1, " FPS")} loading={loading} />
          <StatRow label="AI FPS cao nhất" value={fmt(maxFps, 1, " FPS")} loading={loading} />
          <StatRow
            label="Tỷ lệ snapshot dưới 20 FPS"
            value={fpsValues.length ? percent(below20, fpsValues.length) : "—"}
            badge={fpsValues.length ? `${below20}/${fpsValues.length}` : undefined}
            badgeOk={fpsValues.length ? below20 === 0 : undefined}
            loading={loading}
          />
          <StatRow
            label="Số người quan sát / mẫu"
            value={fmt(
              (windowData?.samples ?? 0) > 0
                ? (windowData?.person_observations ?? 0) / (windowData?.samples ?? 1)
                : null,
              2,
            )}
            loading={loading}
          />
          <StatRow
            label="Mức tải perception"
            value={
              windowData?.avg_ai_fps == null
                ? "—"
                : windowData.avg_ai_fps >= 20
                  ? "Ổn định"
                  : windowData.avg_ai_fps >= 12
                    ? "Cần theo dõi"
                    : "Quá tải"
            }
            badge={
              windowData?.avg_ai_fps == null
                ? undefined
                : windowData.avg_ai_fps >= 20
                  ? "realtime"
                  : "attention"
            }
            badgeOk={windowData?.avg_ai_fps != null ? windowData.avg_ai_fps >= 20 : undefined}
            loading={loading}
          />
        </div>
        <div>
          <p className="text-xs font-medium uppercase tracking-wider text-slate-400 mb-3">
            AI FPS theo thời gian
          </p>
          <div className="h-[240px]">
            {loading ? <div className="h-full bg-slate-50 rounded-xl animate-pulse" />
              : fpsData.length === 0 ? <EmptyChart label="Chưa có snapshot AI để phân tích" />
              : (
                <ResponsiveContainer width="100%" height="100%">
                  <LineChart data={fpsData}>
                    <CartesianGrid strokeDasharray="3 3" stroke="#f1f5f9" />
                    <XAxis dataKey="t" fontSize={10} stroke="#94a3b8" tickLine={false} />
                    <YAxis fontSize={10} stroke="#94a3b8" tickLine={false} width={38} />
                    <Tooltip formatter={(v: unknown) => [`${Number(v).toFixed(1)} FPS`, "AI FPS"]} />
                    <Line type="monotone" dataKey="fps" stroke="#7c3aed" strokeWidth={2} dot={false} connectNulls />
                  </LineChart>
                </ResponsiveContainer>
              )}
          </div>
        </div>
      </div>
    </div>
  );
}

function ModeDistributionSection({ summary, loading }: {
  summary: AnalyticsSummary | null;
  loading: boolean;
}) {
  const modeEntries = Object.entries(summary?.window.navigation_modes ?? {});
  const total = modeEntries.reduce((acc, [, value]) => acc + value, 0);
  const pieData = modeEntries.map(([name, value]) => ({ name, value }));
  const colors = ["#2563eb", "#0f766e", "#7c3aed", "#f59e0b", "#ef4444", "#64748b"];

  return (
    <div className="bg-white rounded-2xl border border-slate-200 shadow-sm p-6">
      <SectionHeader
        icon={BarChart3}
        title="Phân bố Điều hướng"
        subtitle="tỷ trọng các mode vận hành trong cửa sổ phân tích hiện tại"
        accent="bg-cyan-500"
      />
      <div className="grid grid-cols-1 xl:grid-cols-2 gap-8">
        <div className="space-y-0">
          {loading ? (
            <>
              <SkeletonLine />
              <div className="h-3" />
              <SkeletonLine w="85%" />
              <div className="h-3" />
              <SkeletonLine w="65%" />
            </>
          ) : modeEntries.length === 0 ? (
            <p className="text-sm text-slate-400">Chưa có dữ liệu mode điều hướng để phân tích.</p>
          ) : (
            modeEntries
              .sort((a, b) => b[1] - a[1])
              .map(([name, value]) => (
                <StatRow
                  key={name}
                  label={name}
                  value={String(value)}
                  badge={percent(value, total)}
                  loading={false}
                />
              ))
          )}
        </div>
        <div>
          <p className="text-xs font-medium uppercase tracking-wider text-slate-400 mb-3">
            Tỷ trọng mode
          </p>
          <div className="h-[240px]">
            {loading ? <div className="h-full bg-slate-50 rounded-xl animate-pulse" />
              : pieData.length === 0 ? <EmptyChart label="Chưa có mode navigation trong cửa sổ đang xem" />
              : (
                <ResponsiveContainer width="100%" height="100%">
                  <PieChart>
                    <Pie
                      data={pieData}
                      dataKey="value"
                      nameKey="name"
                      innerRadius={52}
                      outerRadius={82}
                      paddingAngle={2}
                    >
                      {pieData.map((entry, index) => (
                        <Cell key={entry.name} fill={colors[index % colors.length]} />
                      ))}
                    </Pie>
                    <Tooltip formatter={(v: unknown) => [String(v), "Số mẫu"]} />
                  </PieChart>
                </ResponsiveContainer>
              )}
          </div>
        </div>
      </div>
    </div>
  );
}

function AlertsSection({ summary, loading }: { summary: AnalyticsSummary | null; loading: boolean }) {
  const alerts = summary?.logs.recent_alerts ?? [];
  const severity = summary?.logs.by_severity ?? {};
  const totalAlerts = Object.values(severity).reduce((acc, value) => acc + value, 0);
  const warningCount = severity.WARNING ?? 0;
  const errorCount = (severity.ERROR ?? 0) + (severity.CRITICAL ?? 0);

  return (
    <div className="bg-white rounded-2xl border border-slate-200 shadow-sm p-6">
      <SectionHeader
        icon={AlertTriangle}
        title="Phân tích Cảnh báo"
        subtitle="mức độ rủi ro · nguồn cảnh báo · nhật ký gần nhất"
        accent="bg-amber-500"
      />
      <div className="grid grid-cols-1 xl:grid-cols-2 gap-8">
        <div className="space-y-0">
          <StatRow label="Tổng số cảnh báo" value={String(totalAlerts)} loading={loading} />
          <StatRow label="Warning" value={String(warningCount)} loading={loading} />
          <StatRow label="Error / Critical" value={String(errorCount)} loading={loading} />
          <StatRow
            label="Mức rủi ro vận hành"
            value={errorCount > 0 ? "Cần can thiệp" : warningCount > 0 ? "Theo dõi" : "Ổn định"}
            badge={errorCount > 0 ? "high" : warningCount > 0 ? "medium" : "low"}
            badgeOk={errorCount === 0}
            loading={loading}
          />
        </div>
        <div>
          <p className="text-xs font-medium uppercase tracking-wider text-slate-400 mb-3">
            Cảnh báo gần nhất
          </p>
          <div className="space-y-2 max-h-[280px] overflow-y-auto">
            {loading ? (
              [0, 1, 2].map((i) => (
                <div key={i} className="rounded-lg border border-slate-100 p-3 space-y-2">
                  <SkeletonLine w="60%" />
                  <SkeletonLine w="90%" />
                </div>
              ))
            ) : alerts.length === 0 ? (
              <div className="rounded-xl border border-dashed border-slate-200 p-6 text-center text-slate-400 text-sm">
                Không có cảnh báo trong cửa sổ hiện tại
              </div>
            ) : (
              alerts.map((a) => (
                <div key={a.id} className="rounded-lg border border-slate-100 p-3 text-sm">
                  <div className="flex flex-wrap items-center gap-2 mb-1">
                    <StatusBadge status={a.severity === "WARNING" ? "warning" : "error"}>
                      {a.severity}
                    </StatusBadge>
                    <span className="font-medium text-slate-700">{a.source}</span>
                    <span className="text-slate-400 text-xs ml-auto">
                      {new Date(a.created_at).toLocaleTimeString()}
                    </span>
                  </div>
                  <p className="text-slate-600">{a.message}</p>
                </div>
              ))
            )}
          </div>
        </div>
      </div>
    </div>
  );
}

function BusinessInsightsSection({ summary, loading }: {
  summary: AnalyticsSummary | null;
  loading: boolean;
}) {
  const windowData = summary?.window;
  const current = summary?.current;
  const avgPersons =
    (windowData?.samples ?? 0) > 0
      ? (windowData?.person_observations ?? 0) / (windowData?.samples ?? 1)
      : 0;

  const insights = [
    {
      label: "Mức độ đông người",
      value: avgPersons > 2 ? "Cao" : avgPersons > 0.8 ? "Trung bình" : "Thấp",
      note: `${fmt(avgPersons, 2)} người / snapshot`,
      icon: Users,
      accent: "bg-teal-500",
    },
    {
      label: "Tải perception",
      value:
        (windowData?.avg_ai_fps ?? 0) >= 20
          ? "Ổn định"
          : (windowData?.avg_ai_fps ?? 0) >= 12
            ? "Dao động"
            : "Căng tải",
      note: `AI FPS avg ${fmt(windowData?.avg_ai_fps, 1)}`,
      icon: Brain,
      accent: "bg-violet-500",
    },
    {
      label: "Xu hướng điều hướng",
      value: current?.navigation_mode ?? "—",
      note: "mode nổi bật hiện tại",
      icon: TrendingUp,
      accent: "bg-blue-500",
    },
  ];

  return (
    <div className="bg-white rounded-2xl border border-slate-200 shadow-sm p-6">
      <SectionHeader
        icon={TrendingUp}
        title="Nhận định Nghiệp vụ"
        subtitle="đọc nhanh tình trạng vận hành từ dữ liệu analytics hiện có"
        accent="bg-emerald-500"
      />
      <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
        {insights.map((item) => {
          const Icon = item.icon;
          return (
            <div key={item.label} className="rounded-2xl border border-slate-200 bg-slate-50/60 p-4">
              <div className="flex items-center gap-3 mb-3">
                <div className={`rounded-xl p-2 ${item.accent}`}>
                  <Icon className="w-4 h-4 text-white" />
                </div>
                <p className="text-sm font-medium text-slate-700">{item.label}</p>
              </div>
              {loading ? <SkeletonLine w="90px" /> : (
                <p className="text-xl font-bold text-slate-900">{item.value}</p>
              )}
              <p className="mt-2 text-xs text-slate-400">{item.note}</p>
            </div>
          );
        })}
      </div>
    </div>
  );
}

export default function AnalyticsPage() {
  const [summary, setSummary] = useState<AnalyticsSummary | null>(null);
  const [series, setSeries] = useState<Snapshot[]>([]);
  const [loading, setLoading] = useState(true);
  const [refreshing, setRefreshing] = useState(false);
  const [lastUpdated, setLastUpdated] = useState<Date | null>(null);
  const download = useCSVDownload();

  const load = useCallback(async (manual = false) => {
    if (manual) setRefreshing(true);
    try {
      const [sumRes, serRes] = await Promise.all([
        fetchWithAuth("/api/analytics/summary?hours=24"),
        fetchWithAuth("/api/analytics/timeseries?hours=6&limit=240"),
      ]);
      setSummary((await sumRes.json()) as AnalyticsSummary);
      setSeries((await serRes.json()) as Snapshot[]);
      setLastUpdated(new Date());
    } catch {
      // keep stale
    } finally {
      setLoading(false);
      setRefreshing(false);
    }
  }, []);

  useEffect(() => {
    const run = async () => { await load(); };
    void run();
    const iv = setInterval(() => { void run(); }, 10_000);
    return () => clearInterval(iv);
  }, [load]);

  return (
    <div className="space-y-6">
      <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
        <div>
          <h2 className="text-2xl font-bold text-slate-800 tracking-tight">Analytics Dashboard</h2>
          <p className="text-sm text-slate-500 mt-1">
            Adaptive-context-aware · vận hành robot · perception · cảnh báo · xu hướng thời gian thực
          </p>
        </div>
        <div className="flex items-center gap-3">
          <DownloadBtn
            onClick={() => void download("/api/analytics/timeseries?hours=24&limit=1000", "analytics_timeseries.json")}
            label="Export Timeseries"
          />
          <StatusBadge status={summary?.collector.running ? "success" : "warning"}>
            {summary?.collector.running ? "collector live" : "collector offline"}
          </StatusBadge>
          <button
            onClick={() => void load(true)}
            disabled={refreshing}
            aria-label="Refresh"
            className="flex items-center gap-1.5 text-xs font-medium text-slate-500 hover:text-slate-800 transition-colors disabled:opacity-50"
          >
            <RefreshCw className={`w-3.5 h-3.5 ${refreshing ? "animate-spin" : ""}`} />
            {lastUpdated ? lastUpdated.toLocaleTimeString() : "—"}
          </button>
        </div>
      </div>

      <OverviewCards summary={summary} loading={loading} />
      <BusinessInsightsSection summary={summary} loading={loading} />
      <OperationsSection summary={summary} series={series} loading={loading} />
      <PerceptionSection summary={summary} series={series} loading={loading} />
      <ModeDistributionSection summary={summary} loading={loading} />
      <AlertsSection summary={summary} loading={loading} />
    </div>
  );
}
