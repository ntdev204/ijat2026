"use client";

import { Button } from "@/components/ui/button";
import { Slider } from "@/components/ui/slider";
import { useIsMobile } from "@/hooks/use-mobile";
import { useWebSocket } from "@/hooks/useWebSocket";
import { cn } from "@/lib/utils";
import { Gamepad2, Grip, RotateCcw, RotateCw, X } from "lucide-react";
import { useCallback, useEffect, useRef, useState } from "react";

const MAX_RADIUS = 54;
const MAX_LINEAR_SPEED = 1.0;
const MIN_LINEAR_SPEED = 0.15;
const ROTATION_SPEED = 1.0;

function clamp(value: number, min: number, max: number) {
  return Math.min(max, Math.max(min, value));
}

export function GlobalKeyboardTeleop() {
  const isMobile = useIsMobile();
  const { isConnected, sendMessage } = useWebSocket("/ws/control");
  const [panelOpen, setPanelOpen] = useState(false);
  const [speed, setSpeed] = useState(0.45);
  const [stick, setStick] = useState({ x: 0, y: 0 });
  const [rotationDirection, setRotationDirection] = useState<"cw" | "ccw" | null>(null);
  const joystickRef = useRef<HTMLDivElement | null>(null);
  const activePointerIdRef = useRef<number | null>(null);
  const speedRef = useRef(speed);
  const rotationDirectionRef = useRef<"cw" | "ccw" | null>(null);

  useEffect(() => {
    speedRef.current = speed;
  }, [speed]);

  const sendCommand = useCallback(
    (linearX: number, linearY: number, angularZ: number) => {
      if (!isConnected) return;
      sendMessage(
        JSON.stringify({
          linear_x: Math.round(linearX * 100) / 100,
          linear_y: Math.round(linearY * 100) / 100,
          angular_z: Math.round(angularZ * 100) / 100,
        }),
      );
    },
    [isConnected, sendMessage],
  );

  const stopRobot = useCallback(() => {
    activePointerIdRef.current = null;
    rotationDirectionRef.current = null;
    setRotationDirection(null);
    setStick({ x: 0, y: 0 });
    sendCommand(0, 0, 0);
  }, [sendCommand]);

  const setPanelOpenSafely = useCallback(
    (nextOpen: boolean | ((current: boolean) => boolean)) => {
      setPanelOpen((current) => {
        const resolved = typeof nextOpen === "function" ? nextOpen(current) : nextOpen;
        if (!resolved) {
          activePointerIdRef.current = null;
          rotationDirectionRef.current = null;
          setRotationDirection(null);
          setStick({ x: 0, y: 0 });
          sendCommand(0, 0, 0);
        }
        return resolved;
      });
    },
    [sendCommand],
  );

  const emitJoystickVelocity = useCallback(
    (nextStick: { x: number; y: number }) => {
      const speedLimit = speedRef.current;
      const lateral = (nextStick.x / MAX_RADIUS) * speedLimit;
      const forward = (-nextStick.y / MAX_RADIUS) * speedLimit;
      sendCommand(forward, lateral, 0);
    },
    [sendCommand],
  );

  const updateStickFromPointer = useCallback(
    (clientX: number, clientY: number) => {
      const container = joystickRef.current;
      if (!container) return;

      const rect = container.getBoundingClientRect();
      const centerX = rect.left + rect.width / 2;
      const centerY = rect.top + rect.height / 2;
      const deltaX = clientX - centerX;
      const deltaY = clientY - centerY;
      const distance = Math.sqrt(deltaX * deltaX + deltaY * deltaY);
      const scale = distance > MAX_RADIUS ? MAX_RADIUS / distance : 1;
      const nextStick = {
        x: Math.round(deltaX * scale),
        y: Math.round(deltaY * scale),
      };

      rotationDirectionRef.current = null;
      setRotationDirection(null);
      setStick(nextStick);
      emitJoystickVelocity(nextStick);
    },
    [emitJoystickVelocity],
  );

  useEffect(() => {
    const release = () => stopRobot();
    window.addEventListener("pointerup", release, true);
    window.addEventListener("pointercancel", release, true);
    window.addEventListener("blur", release, true);
    document.addEventListener("visibilitychange", release, true);

    return () => {
      window.removeEventListener("pointerup", release, true);
      window.removeEventListener("pointercancel", release, true);
      window.removeEventListener("blur", release, true);
      document.removeEventListener("visibilitychange", release, true);
    };
  }, [stopRobot]);

  const handleJoystickPointerDown = useCallback(
    (event: React.PointerEvent<HTMLDivElement>) => {
      activePointerIdRef.current = event.pointerId;
      event.currentTarget.setPointerCapture(event.pointerId);
      updateStickFromPointer(event.clientX, event.clientY);
    },
    [updateStickFromPointer],
  );

  const handleJoystickPointerMove = useCallback(
    (event: React.PointerEvent<HTMLDivElement>) => {
      if (activePointerIdRef.current !== event.pointerId) return;
      updateStickFromPointer(event.clientX, event.clientY);
    },
    [updateStickFromPointer],
  );

  const handleJoystickPointerUp = useCallback(
    (event: React.PointerEvent<HTMLDivElement>) => {
      if (activePointerIdRef.current !== event.pointerId) return;
      stopRobot();
    },
    [stopRobot],
  );

  const startRotation = useCallback(
    (direction: "cw" | "ccw") => {
      activePointerIdRef.current = null;
      setStick({ x: 0, y: 0 });
      setRotationDirection(direction);
      rotationDirectionRef.current = direction;
      sendCommand(0, 0, direction === "cw" ? -ROTATION_SPEED : ROTATION_SPEED);
    },
    [sendCommand],
  );

  const panelWidth = isMobile ? "w-[min(92vw,22rem)]" : "w-80";

  return (
    <div className="pointer-events-none fixed right-4 bottom-4 z-50 flex flex-col items-end gap-3 sm:right-6 sm:bottom-6">
      {panelOpen && (
        <div
          className={cn(
            "pointer-events-auto rounded-[28px] border border-slate-200 bg-white/96 p-4 shadow-[0_22px_60px_rgba(15,23,42,0.18)] backdrop-blur",
            panelWidth,
          )}
        >
          <div className="mb-4 flex items-center justify-between gap-3">
            <div>
              <div className="flex items-center gap-2 text-sm font-semibold text-slate-900">
                <Gamepad2 className="size-4 text-slate-700" />
                Manual drive
              </div>
              <p className="mt-1 text-xs text-slate-500">Kéo cần để chạy, thả ra để dừng.</p>
            </div>
            <div className="flex items-center gap-2">
              <span
                className={cn(
                  "rounded-full px-2.5 py-1 text-[11px] font-semibold tracking-wide",
                  isConnected ? "bg-emerald-100 text-emerald-700" : "bg-rose-100 text-rose-700",
                )}
              >
                {isConnected ? "ONLINE" : "OFFLINE"}
              </span>
              <Button type="button" variant="ghost" size="icon-sm" onClick={() => setPanelOpenSafely(false)}>
                <X className="size-4" />
                <span className="sr-only">Close control panel</span>
              </Button>
            </div>
          </div>

          <div className="grid gap-4">
            <div className="rounded-3xl border border-slate-200 bg-slate-50 p-4">
              <div className="mb-3 flex items-center justify-between text-xs font-medium text-slate-500">
                <span>Joystick</span>
                <span>{speed.toFixed(2)} m/s</span>
              </div>
              <div className="flex items-center justify-center">
                <div
                  ref={joystickRef}
                  className="relative flex h-40 w-40 touch-none items-center justify-center rounded-full border border-slate-300 bg-[radial-gradient(circle_at_center,_rgba(59,130,246,0.16),_rgba(255,255,255,0.98)_68%)]"
                  onPointerDown={handleJoystickPointerDown}
                  onPointerMove={handleJoystickPointerMove}
                  onPointerUp={handleJoystickPointerUp}
                >
                  <div className="pointer-events-none absolute inset-[18px] rounded-full border border-dashed border-slate-300" />
                  <div className="pointer-events-none absolute inset-1/2 h-px w-24 -translate-x-1/2 bg-slate-200" />
                  <div className="pointer-events-none absolute inset-1/2 h-24 w-px -translate-y-1/2 bg-slate-200" />
                  <div
                    className="pointer-events-none flex h-14 w-14 items-center justify-center rounded-full border border-blue-200 bg-blue-600 text-white shadow-lg transition-transform"
                    style={{ transform: `translate(${stick.x}px, ${stick.y}px)` }}
                  >
                    <Grip className="size-5" />
                  </div>
                </div>
              </div>
            </div>

            <div className="rounded-3xl border border-slate-200 bg-slate-50 p-4">
              <div className="mb-3 flex items-center justify-between text-xs font-medium text-slate-500">
                <span>Tốc độ</span>
                <span>{Math.round((speed / MAX_LINEAR_SPEED) * 100)}%</span>
              </div>
              <Slider
                min={MIN_LINEAR_SPEED}
                max={MAX_LINEAR_SPEED}
                step={0.05}
                value={[speed]}
                onValueChange={(values) => {
                  const nextSpeed = clamp(values[0] ?? speed, MIN_LINEAR_SPEED, MAX_LINEAR_SPEED);
                  setSpeed(nextSpeed);
                  if (activePointerIdRef.current !== null) {
                    emitJoystickVelocity(stick);
                  }
                }}
              />
            </div>

            <div className="grid grid-cols-2 gap-3">
              <Button
                type="button"
                variant={rotationDirection === "ccw" ? "default" : "outline"}
                className="h-12 gap-2"
                onPointerDown={() => startRotation("ccw")}
                onPointerUp={stopRobot}
                onPointerLeave={() => {
                  if (rotationDirectionRef.current === "ccw") stopRobot();
                }}
              >
                <RotateCcw className="size-4" />
                Xoay trái
              </Button>
              <Button
                type="button"
                variant={rotationDirection === "cw" ? "default" : "outline"}
                className="h-12 gap-2"
                onPointerDown={() => startRotation("cw")}
                onPointerUp={stopRobot}
                onPointerLeave={() => {
                  if (rotationDirectionRef.current === "cw") stopRobot();
                }}
              >
                <RotateCw className="size-4" />
                Xoay phải
              </Button>
            </div>
          </div>
        </div>
      )}

      <Button
        type="button"
        size="icon-lg"
        className="pointer-events-auto rounded-full shadow-[0_18px_40px_rgba(37,99,235,0.32)]"
        onClick={() => setPanelOpenSafely((current) => !current)}
      >
        <Gamepad2 className="size-5" />
        <span className="sr-only">Toggle manual drive controls</span>
      </Button>
    </div>
  );
}
