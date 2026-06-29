"use client";

import { useAuth } from "@/contexts/AuthContext";
import { Sidebar } from "./Sidebar";
import { Topbar } from "./Topbar";
import { Loader2 } from "lucide-react";
import { GlobalKeyboardTeleop } from "@/components/control/GlobalKeyboardTeleop";
import { useState } from "react";

export function DashboardLayout({ children }: { children: React.ReactNode }) {
  const { user, isLoading } = useAuth();
  const [mobileNavOpen, setMobileNavOpen] = useState(false);

  if (isLoading) {
    return (
      <div className="h-screen w-screen flex items-center justify-center bg-slate-50">
        <div className="flex flex-col items-center gap-4">
          <Loader2 className="w-10 h-10 animate-spin text-blue-600" />
          <p className="text-slate-500 font-medium">Loading application...</p>
        </div>
      </div>
    );
  }

  if (!user) {
    return null; 
  }

  return (
    <div className="flex h-dvh bg-slate-50 overflow-hidden">
      <GlobalKeyboardTeleop />
      <Sidebar mobileOpen={mobileNavOpen} onMobileOpenChange={setMobileNavOpen} />
      <div className="flex-1 flex flex-col min-w-0">
        <Topbar onOpenMobileNav={() => setMobileNavOpen(true)} />
        <main className="flex-1 overflow-auto px-4 py-4 sm:p-6">
          {children}
        </main>
      </div>
    </div>
  );
}
