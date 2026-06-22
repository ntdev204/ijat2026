import type { SystemComponent } from "@/types/robot-runtime";

export interface SystemComponentViewModel extends SystemComponent {
  statusLabel: string;
  statusVariant: "success" | "warning" | "default";
  toggleLabel: string;
  hostLabel: string;
  launchLabel: string;
  starting: boolean;
  stopping: boolean;
}
