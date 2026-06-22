"use client";

import { Radar } from "lucide-react";
import { Button } from "@/components/ui/button";
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuRadioGroup,
  DropdownMenuRadioItem,
  DropdownMenuTrigger,
} from "@/components/ui/dropdown-menu";
import { useOperationMode, type OperationMode } from "@/contexts/OperationModeContext";

const MODE_LABELS: Record<OperationMode, string> = {
  real: "Real",
  sim: "Simulation",
  hybrid: "Hybrid",
};

export function OperationModeMenu() {
  const { busy, operationMode, setOperationMode } = useOperationMode();

  return (
    <DropdownMenu>
      <DropdownMenuTrigger asChild>
        <Button type="button" variant="outline" disabled={busy} className="min-w-[180px] justify-between">
          <span>Mode: {MODE_LABELS[operationMode]}</span>
          <Radar className="h-4 w-4" />
        </Button>
      </DropdownMenuTrigger>
      <DropdownMenuContent align="end">
        <DropdownMenuRadioGroup
          value={operationMode}
          onValueChange={(value) => void setOperationMode(value as OperationMode)}
        >
          <DropdownMenuRadioItem value="real">Real</DropdownMenuRadioItem>
          <DropdownMenuRadioItem value="sim">Simulation</DropdownMenuRadioItem>
          <DropdownMenuRadioItem value="hybrid">Hybrid</DropdownMenuRadioItem>
        </DropdownMenuRadioGroup>
      </DropdownMenuContent>
    </DropdownMenu>
  );
}
