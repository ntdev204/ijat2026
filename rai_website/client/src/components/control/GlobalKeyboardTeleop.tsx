"use client";

import { useWebSocket } from "@/hooks/useWebSocket";
import { useCallback, useEffect, useRef } from "react";

const LINEAR_SPEED = 0.3;
const ANGULAR_SPEED = 0.5;
const VALID_KEYS = new Set(["w", "a", "s", "d", "q", "e", "z", "c", "x", " ", "control"]);

function shouldIgnoreKeyboardEvent(event: KeyboardEvent, key: string) {
  if (event.metaKey || event.altKey) return true;
  if (event.ctrlKey && key !== "x" && key !== "control") return true;

  const target = event.target;
  if (!(target instanceof HTMLElement)) return false;

  const tag = target.tagName;
  return (
    tag === "INPUT" ||
    tag === "TEXTAREA" ||
    tag === "SELECT" ||
    target.isContentEditable
  );
}

function calculateVelocity(keys: Set<string>) {
  let x = 0;
  let y = 0;
  let z = 0;

  if (keys.has(" ")) {
    return { x: 0, y: 0, z: 0 };
  }

  if (keys.has("w")) x += LINEAR_SPEED;
  if (keys.has("s")) x -= LINEAR_SPEED;
  if (keys.has("a")) y += LINEAR_SPEED;
  if (keys.has("d")) y -= LINEAR_SPEED;

  if (keys.has("q")) {
    x += LINEAR_SPEED;
    y += LINEAR_SPEED;
  }
  if (keys.has("e")) {
    x += LINEAR_SPEED;
    y -= LINEAR_SPEED;
  }
  if (keys.has("z")) {
    x -= LINEAR_SPEED;
    y += LINEAR_SPEED;
  }
  if (keys.has("c")) {
    x -= LINEAR_SPEED;
    y -= LINEAR_SPEED;
  }

  if (keys.has("x")) {
    z += keys.has("control") ? ANGULAR_SPEED : -ANGULAR_SPEED;
  }

  const magnitude = Math.sqrt(x * x + y * y);
  if (magnitude > LINEAR_SPEED && magnitude > 0) {
    x = (x / magnitude) * LINEAR_SPEED;
    y = (y / magnitude) * LINEAR_SPEED;
  }

  return { x, y, z };
}

export function GlobalKeyboardTeleop() {
  const { isConnected, sendMessage } = useWebSocket("/ws/control");
  const activeKeysRef = useRef<Set<string>>(new Set());
  const isConnectedRef = useRef(isConnected);

  useEffect(() => {
    isConnectedRef.current = isConnected;
  }, [isConnected]);

  const sendCommand = useCallback(
    (x: number, y: number, z: number) => {
      if (!isConnectedRef.current) return;

      const roundedX = Math.round(x * 100) / 100;
      const roundedY = Math.round(y * 100) / 100;
      const roundedZ = Math.round(z * 100) / 100;

      sendMessage(
        JSON.stringify({
          linear_x: roundedX,
          linear_y: roundedY,
          angular_z: roundedZ,
        })
      );
    },
    [sendMessage]
  );

  const sendCurrentVelocity = useCallback(() => {
    const velocity = calculateVelocity(activeKeysRef.current);
    sendCommand(velocity.x, velocity.y, velocity.z);
  }, [sendCommand]);

  useEffect(() => {
    const handleKeyDown = (event: KeyboardEvent) => {
      const key = event.key.toLowerCase();
      if (!VALID_KEYS.has(key) || shouldIgnoreKeyboardEvent(event, key)) return;

      if (key === " " || (key === "x" && event.ctrlKey)) {
        event.preventDefault();
      }
      if (event.repeat || activeKeysRef.current.has(key)) return;

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

    window.addEventListener("keydown", handleKeyDown);
    window.addEventListener("keyup", handleKeyUp);
    window.addEventListener("blur", stopRobot);
    document.addEventListener("visibilitychange", stopRobot);

    return () => {
      stopRobot();
      window.removeEventListener("keydown", handleKeyDown);
      window.removeEventListener("keyup", handleKeyUp);
      window.removeEventListener("blur", stopRobot);
      document.removeEventListener("visibilitychange", stopRobot);
    };
  }, [sendCommand, sendCurrentVelocity]);

  return null;
}
