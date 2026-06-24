import { useAuth } from "@/contexts/AuthContext";
import { useOperationMode } from "@/contexts/OperationModeContext";
import { Button } from "@/components/ui/button";
import { PanelLeft, User as UserIcon } from "lucide-react";

interface TopbarProps {
  onOpenMobileNav: () => void;
}

export function Topbar({ onOpenMobileNav }: TopbarProps) {
  const { user } = useAuth();
  const { operationMode } = useOperationMode();

  return (
    <header className="z-10 flex min-h-16 items-center justify-between gap-3 border-b border-slate-200 bg-white px-4 py-3 sm:h-16 sm:px-6 sm:py-0">
      <div className="flex items-center gap-4">
        <Button type="button" variant="outline" size="icon-sm" className="sm:hidden" onClick={onOpenMobileNav}>
          <PanelLeft className="h-4 w-4" />
          <span className="sr-only">Open menu</span>
        </Button>
        <h1 className="text-lg font-semibold text-slate-800">Dashboard</h1>
      </div>

      <div className="flex min-w-0 items-center gap-2 sm:gap-4">
        <span className="hidden rounded-full border border-slate-200 bg-slate-100 px-3 py-1 text-xs font-semibold uppercase tracking-wide text-slate-600 lg:inline-flex">
          {operationMode}
        </span>
        <div className="flex min-w-0 items-center gap-2 rounded-full border border-slate-200 bg-slate-100 px-2 py-1.5 sm:px-3">
          <UserIcon className="h-4 w-4 shrink-0 text-slate-600" />
          <span className="max-w-20 truncate text-sm font-medium text-slate-700 sm:max-w-40">
            {user?.username || "User"}
          </span>
          <span className="ml-0 rounded-full bg-blue-100 px-2 py-0.5 text-[10px] font-bold uppercase tracking-wider text-blue-700 sm:ml-2 sm:text-xs">
            {user?.role || "VIEWER"}
          </span>
        </div>
      </div>
    </header>
  );
}
