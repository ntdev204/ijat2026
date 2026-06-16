import { cn } from "@/lib/utils";

type Status = "success" | "warning" | "error" | "default" | "info";

interface StatusBadgeProps {
  status: Status;
  children: React.ReactNode;
  className?: string;
}

const statusStyles: Record<Status, string> = {
  success: "bg-emerald-100 text-emerald-800 border-emerald-200",
  warning: "bg-amber-100 text-amber-800 border-amber-200",
  error: "bg-rose-100 text-rose-800 border-rose-200",
  info: "bg-blue-100 text-blue-800 border-blue-200",
  default: "bg-slate-100 text-slate-800 border-slate-200",
};

export function StatusBadge({ status, children, className }: StatusBadgeProps) {
  return (
    <span
      className={cn(
        "inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-semibold border",
        statusStyles[status],
        className
      )}
    >
      {children}
    </span>
  );
}
