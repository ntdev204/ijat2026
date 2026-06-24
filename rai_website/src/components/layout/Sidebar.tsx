import { Sheet, SheetContent, SheetDescription, SheetHeader, SheetTitle } from "@/components/ui/sheet";
import Link from "next/link";
import { usePathname } from "next/navigation";
import { useIsMobile } from "@/hooks/use-mobile";
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

interface SidebarProps {
  mobileOpen: boolean;
  onMobileOpenChange: (open: boolean) => void;
}

function SidebarNav({ onNavigate }: { onNavigate?: () => void }) {
  const pathname = usePathname();

  return (
    <>
      <div className="flex h-16 items-center border-b border-slate-200 px-6">
        <span className="text-xl font-bold tracking-wide text-slate-900">ROBOT<span className="text-blue-600">OS</span></span>
      </div>
      <nav className="flex-1 overflow-y-auto py-4">
        <ul className="space-y-1 px-3">
          {NAV_ITEMS.map((item) => {
            const isActive = pathname === item.href;
            const Icon = item.icon;
            return (
              <li key={item.name}>
                <Link
                  href={item.href}
                  onClick={onNavigate}
                  className={cn(
                    "flex min-h-11 items-center gap-3 rounded-lg px-3 py-2.5 text-sm font-medium transition-colors",
                    isActive 
                      ? "bg-blue-50 text-blue-700 ring-1 ring-blue-100"
                      : "text-slate-600 hover:bg-slate-100 hover:text-slate-900"
                  )}
                >
                  <Icon className="h-5 w-5 shrink-0" />
                  <span className="truncate">{item.name}</span>
                </Link>
              </li>
            );
          })}
        </ul>
      </nav>
    </>
  );
}

export function Sidebar({ mobileOpen, onMobileOpenChange }: SidebarProps) {
  const isMobile = useIsMobile();

  if (isMobile) {
    return (
      <Sheet open={mobileOpen} onOpenChange={onMobileOpenChange}>
        <SheetContent side="left" className="w-[288px] gap-0 p-0" showCloseButton={false}>
          <SheetHeader className="sr-only">
            <SheetTitle>Navigation menu</SheetTitle>
            <SheetDescription>Open pages in the robot dashboard.</SheetDescription>
          </SheetHeader>
          <div className="flex h-full flex-col bg-white text-slate-700">
            <SidebarNav onNavigate={() => onMobileOpenChange(false)} />
          </div>
        </SheetContent>
      </Sheet>
    );
  }

  return (
    <aside className="hidden h-full w-64 flex-col border-r border-slate-200 bg-white text-slate-700 sm:flex">
      <SidebarNav />
    </aside>
  );
}
