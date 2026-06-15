"use client";

import { useAuth } from "@/contexts/AuthContext";
import { fetchWithAuth } from "@/lib/api";
import { Network, Shield, User } from "lucide-react";
import { useEffect, useState } from "react";

interface RobotConfig {
  id: number;
  config_key: string;
  config_value: string;
  category?: string | null;
  updated_at?: string | null;
}

export default function SettingsPage() {
  const { user } = useAuth();
  const [configs, setConfigs] = useState<RobotConfig[]>([]);

  useEffect(() => {
    fetchWithAuth("/api/configs/")
      .then((res) => res.json())
      .then((data: RobotConfig[]) => setConfigs(data))
      .catch(() => setConfigs([]));
  }, []);

  return (
    <div className="space-y-6 max-w-4xl mx-auto">
      <div className="flex items-center justify-between">
        <h2 className="text-2xl font-bold text-slate-800 tracking-tight">Settings</h2>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-4 gap-8">
        <div className="space-y-1">
          <button className="w-full flex items-center gap-3 px-4 py-3 bg-blue-50 text-blue-700 font-medium rounded-lg text-sm transition">
            <User className="w-4 h-4" /> Account
          </button>
          <button className="w-full flex items-center gap-3 px-4 py-3 text-slate-600 hover:bg-slate-100 font-medium rounded-lg text-sm transition">
            <Shield className="w-4 h-4" /> Security & Access
          </button>
          <button className="w-full flex items-center gap-3 px-4 py-3 text-slate-600 hover:bg-slate-100 font-medium rounded-lg text-sm transition">
            <Network className="w-4 h-4" /> Network & APIs
          </button>
        </div>

        <div className="md:col-span-3 space-y-8">
          <section className="bg-white rounded-xl border border-slate-200 p-6 shadow-sm">
            <h3 className="text-lg font-semibold text-slate-800 mb-4 border-b border-slate-100 pb-2">Profile Information</h3>
            <div className="grid grid-cols-2 gap-4">
              <div>
                <label className="block text-sm font-medium text-slate-700 mb-1">Username</label>
                <input
                  type="text"
                  disabled
                  value={user?.username ?? ""}
                  className="w-full px-3 py-2 border border-slate-300 rounded-lg bg-slate-50 text-slate-500 text-sm"
                />
              </div>
              <div>
                <label className="block text-sm font-medium text-slate-700 mb-1">Role</label>
                <input
                  type="text"
                  disabled
                  value={user?.role ?? ""}
                  className="w-full px-3 py-2 border border-slate-300 rounded-lg bg-slate-50 text-slate-500 text-sm"
                />
              </div>
            </div>
          </section>

          <section className="bg-white rounded-xl border border-slate-200 p-6 shadow-sm">
            <h3 className="text-lg font-semibold text-slate-800 mb-4 border-b border-slate-100 pb-2">Robot Configuration Records</h3>
            {configs.length === 0 ? (
              <p className="text-sm text-slate-400">No robot configuration records found in database.</p>
            ) : (
              <div className="divide-y divide-slate-100">
                {configs.map((config) => (
                  <div key={config.id} className="py-3 flex items-center justify-between gap-4">
                    <div>
                      <p className="font-medium text-slate-800">{config.config_key}</p>
                      <p className="text-xs text-slate-400">{config.category ?? "uncategorized"}</p>
                    </div>
                    <span className="font-mono text-sm text-slate-600">{config.config_value}</span>
                  </div>
                ))}
              </div>
            )}
          </section>
        </div>
      </div>
    </div>
  );
}
