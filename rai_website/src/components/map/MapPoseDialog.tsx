"use client";

import { Button } from "@/components/ui/button";
import {
  Dialog,
  DialogContent,
  DialogDescription,
  DialogFooter,
  DialogHeader,
  DialogTitle,
} from "@/components/ui/dialog";
import { Input } from "@/components/ui/input";
import type { Pose2D } from "@/types/robot-runtime";

export interface MapPoseDialogProps {
  open: boolean;
  title: string;
  description: string;
  pose: Pose2D | null;
  busy?: boolean;
  onClose: () => void;
  onChange: (pose: Pose2D) => void;
  onConfirm: () => void;
}

export function MapPoseDialog({
  open,
  title,
  description,
  pose,
  busy = false,
  onClose,
  onChange,
  onConfirm,
}: MapPoseDialogProps) {
  return (
    <Dialog open={open} onOpenChange={(nextOpen) => !nextOpen && onClose()}>
      <DialogContent>
        <DialogHeader>
          <DialogTitle>{title}</DialogTitle>
          <DialogDescription>{description}</DialogDescription>
        </DialogHeader>

        <div className="grid gap-3">
          <PoseField
            label="X"
            value={pose?.x ?? 0}
            onChange={(value) => onChange({ x: value, y: pose?.y ?? 0, yaw: pose?.yaw ?? 0 })}
          />
          <PoseField
            label="Y"
            value={pose?.y ?? 0}
            onChange={(value) => onChange({ x: pose?.x ?? 0, y: value, yaw: pose?.yaw ?? 0 })}
          />
          <PoseField
            label="Theta (rad)"
            value={pose?.yaw ?? 0}
            onChange={(value) => onChange({ x: pose?.x ?? 0, y: pose?.y ?? 0, yaw: value })}
          />
        </div>

        <DialogFooter>
          <Button type="button" variant="outline" onClick={onClose} disabled={busy}>
            Cancel
          </Button>
          <Button type="button" onClick={onConfirm} disabled={busy || pose == null}>
            Confirm
          </Button>
        </DialogFooter>
      </DialogContent>
    </Dialog>
  );
}

interface PoseFieldProps {
  label: string;
  value: number;
  onChange: (value: number) => void;
}

function PoseField({ label, value, onChange }: PoseFieldProps) {
  return (
    <label className="grid gap-1 text-sm">
      <span className="text-xs font-medium text-slate-500">{label}</span>
      <Input
        type="number"
        step="0.01"
        value={Number.isFinite(value) ? value : 0}
        onChange={(event) => onChange(Number(event.target.value))}
      />
    </label>
  );
}
