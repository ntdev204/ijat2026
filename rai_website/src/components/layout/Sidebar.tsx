import Link from "next/link";
import { usePathname } from "next/navigation";
import { cn } from "@/lib/utils";
import { Bot, LayoutDashboard, Video, Map, Database, Boxes, Power } from "lucide-react";

const NAV_ITEMS = [
  { name: "System", href: "/", icon: Power },
  { name: "Overview", href: "/overview", icon: LayoutDashboard },
  { name: "Monitor", href: "/monitor", icon: Video },
  { name: "Dataset", href: "/dataset", icon: Database },
  { name: "Map", href: "/map", icon: Map },
  { name: "Gazebo", href: "/gazebo", icon: Boxes },
  { name: "RViz2", href: "/rviz2", icon: Bot },
];

export function Sidebar() {
  const pathname = usePathname();

  return (
    <div className="fixed inset-x-0 bottom-0 z-30 flex h-18 border-t border-slate-200 bg-white/95 text-slate-700 shadow-[0_-8px_24px_rgba(15,23,42,0.08)] backdrop-blur sm:static sm:h-full sm:w-64 sm:flex-col sm:border-r sm:border-t-0 sm:bg-white sm:shadow-none sm:backdrop-blur-none">
      <div className="hidden h-16 items-center border-b border-slate-200 px-6 sm:flex">
        <span className="text-xl font-bold tracking-wide text-slate-900">ROBOT<span className="text-blue-600">OS</span></span>
      </div>
      <nav className="w-full overflow-x-auto overflow-y-hidden py-2 sm:flex-1 sm:overflow-y-auto sm:py-4">
        <ul className="flex min-w-max items-center gap-1 px-2 sm:block sm:min-w-0 sm:space-y-1 sm:px-3">
          {NAV_ITEMS.map((item) => {
            const isActive = pathname === item.href;
            const Icon = item.icon;
            return (
              <li key={item.name} className="sm:block">
                <Link
                  href={item.href}
                  className={cn(
                    "flex min-h-14 min-w-16 flex-col items-center justify-center gap-1 rounded-lg px-2 py-2 text-[11px] font-medium transition-colors sm:min-h-0 sm:min-w-0 sm:flex-row sm:justify-start sm:gap-3 sm:px-3 sm:py-2.5 sm:text-sm",
                    isActive 
                      ? "bg-blue-50 text-blue-700 ring-1 ring-blue-100"
                      : "text-slate-600 hover:bg-slate-100 hover:text-slate-900"
                  )}
                >
                  <Icon className="h-5 w-5 shrink-0" />
                  <span className="max-w-14 truncate sm:max-w-none">{item.name}</span>
                </Link>
              </li>
            );
          })}
        </ul>
      </nav>
    </div>
  );
}
