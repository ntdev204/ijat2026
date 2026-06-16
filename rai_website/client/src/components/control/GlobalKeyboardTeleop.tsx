"use client";

import { Button } from "@/components/ui/button";
import { useWebSocket } from "@/hooks/useWebSocket";
import { cn } from "@/lib/utils";
import { CircleHelp, Keyboard, RotateCw, Signal, Square } from "lucide-react";
import { useCallback, useEffect, useRef, useState } from "react";

const LINEAR_STEP = 0.1;
const ANGULAR_STEP = 0.1;
const MIN_LINEAR_SPEED = 0.1;
const MAX_LINEAR_SPEED = 1.0;
const MIN_ANGULAR_SPEED = 0.1;
const MAX_ANGULAR_SPEED = 2.0;
const VALID_KEYS = new Set(["w", "a", "s", "d", "q", "e", "z", "c", "x", " ", "shift", "u", "i", "o", "p"]);

function shouldIgnoreKeyboardEvent(event: KeyboardEvent) {
  if (event.metaKey || event.altKey) return true;
  if (event.ctrlKey) return true;

  const target = event.target;
  if (!(target instanceof HTMLElement)) return false;

  const tag = target.tagName;
  return tag === "INPUT" || tag === "TEXTAREA" || tag === "SELECT" || target.isContentEditable;
}

function clamp(value: number, min: number, max: number) {
  return Math.min(max, Math.max(min, value));
}

function calculateVelocity(keys: Set<string>, linearSpeed: number, angularSpeed: number) {
  let x = 0;
  let y = 0;
  let z = 0;

  if (keys.has(" ")) {
    return { x: 0, y: 0, z: 0 };
  }

  if (keys.has("w")) x += linearSpeed;
  if (keys.has("s")) x -= linearSpeed;
  if (keys.has("a")) y += linearSpeed;
  if (keys.has("d")) y -= linearSpeed;

  if (keys.has("q")) {
    x += linearSpeed;
    y += linearSpeed;
  }
  if (keys.has("e")) {
    x += linearSpeed;
    y -= linearSpeed;
  }
  if (keys.has("z")) {
    x -= linearSpeed;
    y += linearSpeed;
  }
  if (keys.has("c")) {
    x -= linearSpeed;
    y -= linearSpeed;
  }

  if (keys.has("x")) {
    z += keys.has("shift") ? angularSpeed : -angularSpeed;
  }

  const magnitude = Math.sqrt(x * x + y * y);
  if (magnitude > linearSpeed && magnitude > 0) {
    x = (x / magnitude) * linearSpeed;
    y = (y / magnitude) * linearSpeed;
  }

  return { x, y, z };
}

export function GlobalKeyboardTeleop() {
  const { isConnected, sendMessage } = useWebSocket("/ws/control");
  const [linearSpeed, setLinearSpeed] = useState(0.3);
  const [angularSpeed, setAngularSpeed] = useState(0.5);
  const [isHelpOpen, setIsHelpOpen] = useState(false);
  const activeKeysRef = useRef<Set<string>>(new Set());
  const isConnectedRef = useRef(isConnected);
  const linearSpeedRef = useRef(linearSpeed);
  const angularSpeedRef = useRef(angularSpeed);

  useEffect(() => {
    isConnectedRef.current = isConnected;
  }, [isConnected]);

  useEffect(() => {
    linearSpeedRef.current = linearSpeed;
  }, [linearSpeed]);

  useEffect(() => {
    angularSpeedRef.current = angularSpeed;
  }, [angularSpeed]);

  const sendCommand = useCallback(
    (x: number, y: number, z: number) => {
      if (!isConnectedRef.current) return;

      sendMessage(
        JSON.stringify({
          linear_x: Math.round(x * 100) / 100,
          linear_y: Math.round(y * 100) / 100,
          angular_z: Math.round(z * 100) / 100,
        }),
      );
    },
    [sendMessage],
  );

  const sendCurrentVelocity = useCallback(() => {
    const velocity = calculateVelocity(activeKeysRef.current, linearSpeedRef.current, angularSpeedRef.current);
    sendCommand(velocity.x, velocity.y, velocity.z);
  }, [sendCommand]);

  const adjustLinearSpeed = useCallback((delta: number) => {
    setLinearSpeed((current) => {
      const next = clamp(current + delta, MIN_LINEAR_SPEED, MAX_LINEAR_SPEED);
      linearSpeedRef.current = next;
      return next;
    });
    queueMicrotask(sendCurrentVelocity);
  }, [sendCurrentVelocity]);

  const adjustAngularSpeed = useCallback((delta: number) => {
    setAngularSpeed((current) => {
      const next = clamp(current + delta, MIN_ANGULAR_SPEED, MAX_ANGULAR_SPEED);
      angularSpeedRef.current = next;
      return next;
    });
    queueMicrotask(sendCurrentVelocity);
  }, [sendCurrentVelocity]);

  useEffect(() => {
    const handleKeyDown = (event: KeyboardEvent) => {
      const key = event.key.toLowerCase();
      if (!VALID_KEYS.has(key) || shouldIgnoreKeyboardEvent(event)) return;

      event.preventDefault();

      if (event.repeat) {
        if (key === "u") adjustLinearSpeed(LINEAR_STEP);
        if (key === "i") adjustLinearSpeed(-LINEAR_STEP);
        if (key === "o") adjustAngularSpeed(ANGULAR_STEP);
        if (key === "p") adjustAngularSpeed(-ANGULAR_STEP);
        return;
      }

      if (key === "u") {
        adjustLinearSpeed(LINEAR_STEP);
        return;
      }
      if (key === "i") {
        adjustLinearSpeed(-LINEAR_STEP);
        return;
      }
      if (key === "o") {
        adjustAngularSpeed(ANGULAR_STEP);
        return;
      }
      if (key === "p") {
        adjustAngularSpeed(-ANGULAR_STEP);
        return;
      }

      if (activeKeysRef.current.has(key)) return;
      const nextKeys = new Set(activeKeysRef.current);
      nextKeys.add(key);
      activeKeysRef.current = nextKeys;
      sendCurrentVelocity();
    };

    const handleKeyUp = (event: KeyboardEvent) => {
      const key = event.key.toLowerCase();
      if (!activeKeysRef.current.has(key)) return;

      const nextKeys = new Set(activeKeysRef.current);
      nextKeys.delete(key);
      activeKeysRef.current = nextKeys;
      sendCurrentVelocity();
    };

    const stopRobot = () => {
      if (activeKeysRef.current.size === 0) return;
      activeKeysRef.current = new Set();
      sendCommand(0, 0, 0);
    };

    window.addEventListener("keydown", handleKeyDown, true);
    window.addEventListener("keyup", handleKeyUp, true);
    window.addEventListener("blur", stopRobot, true);
    document.addEventListener("visibilitychange", stopRobot, true);

    return () => {
      stopRobot();
      window.removeEventListener("keydown", handleKeyDown, true);
      window.removeEventListener("keyup", handleKeyUp, true);
      window.removeEventListener("blur", stopRobot, true);
      document.removeEventListener("visibilitychange", stopRobot, true);
    };
  }, [adjustAngularSpeed, adjustLinearSpeed, sendCommand, sendCurrentVelocity]);

  return (
    <div className="pointer-events-none fixed right-6 bottom-6 z-50 flex max-w-sm flex-col items-end gap-3">
      {isHelpOpen && (
        <div className="pointer-events-auto w-80 rounded-2xl border border-slate-200 bg-white/96 p-4 shadow-xl backdrop-blur">
          <div className="mb-3 flex items-center justify-between">
            <div className="flex items-center gap-2 text-sm font-semibold text-slate-900">
              <Keyboard className="size-4 text-slate-700" />
              Global Teleop
            </div>
            <span
              className={cn(
                "rounded-full px-2 py-0.5 text-xs font-semibold",
                isConnected ? "bg-emerald-100 text-emerald-700" : "bg-rose-100 text-rose-700",
              )}
            >
              {isConnected ? "CONNECTED" : "OFFLINE"}
            </span>
          </div>

          <div className="space-y-2 text-sm text-slate-600">
            <p><strong>W/S</strong> tiến/lùi, <strong>A/D</strong> ngang trái/phải</p>
            <p><strong>Q/E/Z/C</strong> chéo, <strong>X</strong> quay CW, <strong>Shift+X</strong> quay CCW</p>
            <p><strong>Space</strong> dừng khẩn</p>
            <p><strong>U/I</strong> tăng/giảm tốc độ tuyến tính</p>
            <p><strong>O/P</strong> tăng/giảm tốc độ xoay</p>
          </div>

          <div className="mt-4 grid grid-cols-2 gap-3">
            <div className="rounded-xl border border-slate-200 bg-slate-50 px-3 py-2">
              <div className="text-xs text-slate-500">Linear speed</div>
              <div className="mt-1 font-mono text-sm font-semibold text-slate-900">{linearSpeed.toFixed(2)} m/s</div>
            </div>
            <div className="rounded-xl border border-slate-200 bg-slate-50 px-3 py-2">
              <div className="text-xs text-slate-500">Angular speed</div>
              <div className="mt-1 font-mono text-sm font-semibold text-slate-900">{angularSpeed.toFixed(2)} rad/s</div>
            </div>
          </div>

          <div className="mt-4 flex gap-2">
            <Button type="button" variant="outline" size="sm" className="gap-2" onClick={() => adjustLinearSpeed(-LINEAR_STEP)}>
              <Signal className="size-4" />
              I
            </Button>
            <Button type="button" size="sm" className="gap-2" onClick={() => adjustLinearSpeed(LINEAR_STEP)}>
              <Signal className="size-4" />
              U
            </Button>
            <Button type="button" variant="outline" size="sm" className="gap-2" onClick={() => adjustAngularSpeed(-ANGULAR_STEP)}>
              <RotateCw className="size-4" />
              P
            </Button>
            <Button type="button" size="sm" className="gap-2" onClick={() => adjustAngularSpeed(ANGULAR_STEP)}>
              <RotateCw className="size-4" />
              O
            </Button>
          </div>
        </div>
      )}

      <div className="pointer-events-auto flex gap-2">
        <Button
          type="button"
          variant="outline"
          size="icon"
          className="rounded-full bg-white/96 shadow-lg backdrop-blur"
          onClick={() => {
            activeKeysRef.current = new Set([" "]);
            sendCurrentVelocity();
            activeKeysRef.current = new Set();
            sendCommand(0, 0, 0);
          }}
          title="Emergency stop"
        >
          <Square className="size-4" />
        </Button>
        <Button
          type="button"
          size="icon"
          className="rounded-full shadow-lg"
          onClick={() => setIsHelpOpen((current) => !current)}
          title="Keyboard teleop help"
        >
          <CircleHelp className="size-4" />
        </Button>
      </div>
    </div>
  );
}
