"use client";

import { fetchWithAuth } from "@/lib/api";
import { Clock, Plus } from "lucide-react";
import { useEffect, useState } from "react";

interface PatrolRoute {
  id: number;
  name: string;
  description?: string | null;
  waypoints_json: unknown[];
  waypoint_tolerance?: number | null;
  created_at: string;
}

interface PatrolSchedule {
  id: number;
  route_id: number;
  cron_expression: string;
  is_enabled?: boolean;
}

export default function PatrolPage() {
  const [routes, setRoutes] = useState<PatrolRoute[]>([]);
  const [schedules, setSchedules] = useState<PatrolSchedule[]>([]);

  useEffect(() => {
    Promise.all([
      fetchWithAuth("/api/patrol/routes").then((res) => res.json()),
      fetchWithAuth("/api/patrol/schedules").then((res) => res.json()),
    ])
      .then(([routeData, scheduleData]: [PatrolRoute[], PatrolSchedule[]]) => {
        setRoutes(routeData);
        setSchedules(scheduleData);
      })
      .catch(() => {
        setRoutes([]);
        setSchedules([]);
      });
  }, []);

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <h2 className="text-2xl font-bold text-slate-800 tracking-tight">Patrol Routes</h2>
        <button className="flex items-center gap-2 px-4 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 transition shadow-sm text-sm font-medium">
          <Plus className="w-4 h-4" /> New Route
        </button>
      </div>

      <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
        <div className="lg:col-span-2 space-y-4">
          {routes.length === 0 ? (
            <div className="bg-white border border-slate-200 rounded-xl p-12 shadow-sm flex flex-col items-center justify-center text-slate-400">
              <p>No patrol routes found in database.</p>
            </div>
          ) : (
            routes.map((route) => (
              <div key={route.id} className="bg-white border border-slate-200 rounded-xl p-5 shadow-sm">
                <p className="font-semibold text-slate-800">{route.name}</p>
                {route.description && <p className="text-sm text-slate-500 mt-1">{route.description}</p>}
                <p className="text-xs text-slate-400 mt-3">
                  {route.waypoints_json.length} waypoints, tolerance {route.waypoint_tolerance ?? "-"} m
                </p>
              </div>
            ))
          )}
        </div>

        <div className="bg-white rounded-xl border border-slate-200 p-6 shadow-sm">
          <h3 className="font-semibold text-lg flex items-center gap-2 mb-6 text-slate-800">
            <Clock className="w-5 h-5 text-blue-500" /> Active Schedules
          </h3>

          <div className="space-y-4 text-slate-500 text-sm">
            {schedules.filter((item) => item.is_enabled).length === 0 ? (
              <p>No active schedules found in database.</p>
            ) : (
              schedules
                .filter((item) => item.is_enabled)
                .map((item) => (
                  <div key={item.id} className="border border-slate-200 rounded-lg p-3">
                    <p className="font-mono">{item.cron_expression}</p>
                    <p className="text-xs mt-1">Route #{item.route_id}</p>
                  </div>
                ))
            )}
          </div>
        </div>
      </div>
    </div>
  );
}
