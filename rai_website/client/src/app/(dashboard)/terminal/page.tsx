"use client";

import { useEffect, useMemo, useRef, useState } from "react";
import { AlertTriangle, CornerDownLeft, Trash2 } from "lucide-react";

import { StatusBadge } from "@/components/ui/StatusBadge";
import {
  Select,
  SelectContent,
  SelectItem,
  SelectTrigger,
  SelectValue,
} from "@/components/ui/select";
import { useAuth } from "@/contexts/AuthContext";
import { useWebSocket } from "@/hooks/useWebSocket";

type TerminalMessage = {
  id: number;
  text: string;
  kind: "stdout" | "stderr" | "system" | "input";
};

const TARGETS = [
  { value: "laptop", label: "Laptop" },
  { value: "raspi", label: "RasPi" },
] as const;

export default function TerminalPage() {
  const { user } = useAuth();
  const [target, setTarget] = useState<(typeof TARGETS)[number]["value"]>("laptop");
  const [command, setCommand] = useState("");
  const [messages, setMessages] = useState<TerminalMessage[]>([]);
  const nextIdRef = useRef(1);
  const viewportRef = useRef<HTMLDivElement | null>(null);
  const socketPath = `/ws/ssh?target=${target}`;

  const isAdmin = user?.role === "admin";
  const { isConnected, serverOffline, sendMessage } = useWebSocket(socketPath, {
    autoReconnect: isAdmin,
    onMessage: (event) => {
      try {
        const payload = JSON.parse(event.data as string) as {
          type?: string;
          stream?: "stdout" | "stderr";
          data?: string;
          status?: string;
          target?: string;
          label?: string;
          message?: string;
          code?: number | null;
        };

        if (payload.type === "output" && payload.data) {
          appendMessage(payload.data, payload.stream === "stderr" ? "stderr" : "stdout");
          return;
        }

        if (payload.type === "status") {
          if (payload.status === "connected") {
            appendMessage(`Connected to ${payload.label ?? payload.target}`, "system");
          } else if (payload.status === "closed") {
            appendMessage(`Connection closed${payload.code != null ? ` (code ${payload.code})` : ""}`, "system");
          }
          return;
        }

        if (payload.type === "error") {
          appendMessage(payload.message || "SSH terminal error", "stderr");
        }
      } catch {
        appendMessage(String(event.data), "stdout");
      }
    },
  });

  function appendMessage(text: string, kind: TerminalMessage["kind"]) {
    setMessages((current) => [
      ...current,
      {
        id: nextIdRef.current++,
        text,
        kind,
      },
    ]);
  }

  useEffect(() => {
    viewportRef.current?.scrollTo({ top: viewportRef.current.scrollHeight, behavior: "smooth" });
  }, [messages]);

  const connectionLabel = useMemo(() => {
    if (!isAdmin) return "ADMIN ONLY";
    if (serverOffline) return "OFFLINE";
    return isConnected ? "CONNECTED" : "CONNECTING";
  }, [isAdmin, isConnected, serverOffline]);

  const connectionStatus = useMemo(() => {
    if (!isAdmin) return "warning" as const;
    if (serverOffline) return "error" as const;
    return isConnected ? "success" as const : "info" as const;
  }, [isAdmin, isConnected, serverOffline]);

  const handleSubmit = (event: React.FormEvent<HTMLFormElement>) => {
    event.preventDefault();
    const trimmed = command.trim();
    if (!trimmed || !isConnected || !isAdmin) return;

    appendMessage(`$ ${trimmed}`, "input");
    sendMessage(JSON.stringify({ type: "input", data: `${trimmed}\n` }));
    setCommand("");
  };

  return (
    <div className="mx-auto flex max-w-7xl flex-col gap-6">
      <div className="flex flex-col gap-3 md:flex-row md:items-center md:justify-between">
        <div>
          <h2 className="text-2xl font-bold tracking-tight text-slate-800">SSH Terminal</h2>
          <p className="text-sm text-slate-500">
            Browser terminal bridge for the laptop runtime and Raspberry Pi over the backend WebSocket.
          </p>
        </div>
        <div className="flex items-center gap-3">
          <StatusBadge status={connectionStatus}>{connectionLabel}</StatusBadge>
          <Select
            value={target}
            onValueChange={(value) => {
              setMessages([]);
              nextIdRef.current = 1;
              setTarget(value as "laptop" | "raspi");
            }}
          >
            <SelectTrigger className="w-[140px] rounded-lg border-slate-300 bg-white text-slate-700 shadow-sm">
              <SelectValue placeholder="Select target" />
            </SelectTrigger>
            <SelectContent>
              {TARGETS.map((item) => (
                <SelectItem key={item.value} value={item.value}>
                  {item.label}
                </SelectItem>
              ))}
            </SelectContent>
          </Select>
          <button
            type="button"
            onClick={() => setMessages([])}
            className="inline-flex items-center gap-2 rounded-lg border border-slate-300 bg-white px-3 py-2 text-sm text-slate-700 shadow-sm hover:bg-slate-50"
          >
            <Trash2 className="h-4 w-4" />
            Clear
          </button>
        </div>
      </div>

      {!isAdmin && (
        <div className="rounded-xl border border-amber-200 bg-amber-50 p-4 text-amber-900">
          <div className="flex items-start gap-3">
            <AlertTriangle className="mt-0.5 h-5 w-5" />
            <div>
              <div className="font-semibold">Terminal access is restricted</div>
              <p className="mt-1 text-sm">
                Only users with the <code>admin</code> role can open SSH sessions from the website.
              </p>
            </div>
          </div>
        </div>
      )}

      <div className="rounded-2xl border border-slate-200 bg-slate-950 shadow-sm">
        <div className="border-b border-slate-800 px-4 py-3 text-xs uppercase tracking-[0.2em] text-slate-400">
          {target} shell
        </div>
        <div
          ref={viewportRef}
          className="h-[60vh] overflow-y-auto whitespace-pre-wrap break-words px-4 py-4 font-mono text-sm leading-6"
        >
          {messages.length === 0 ? (
            <div className="text-slate-500">No output yet.</div>
          ) : (
            messages.map((message) => (
              <div
                key={message.id}
                className={
                  message.kind === "stderr"
                    ? "text-rose-300"
                    : message.kind === "system"
                      ? "text-sky-300"
                      : message.kind === "input"
                        ? "text-emerald-300"
                        : "text-slate-100"
                }
              >
                {message.text}
              </div>
            ))
          )}
        </div>
        <form onSubmit={handleSubmit} className="border-t border-slate-800 p-3">
          <label className="mb-2 block text-xs uppercase tracking-[0.2em] text-slate-500">Command</label>
          <div className="flex items-center gap-3 rounded-xl border border-slate-800 bg-slate-900 px-3 py-2">
            <span className="font-mono text-emerald-400">$</span>
            <input
              value={command}
              onChange={(event) => setCommand(event.target.value)}
              disabled={!isConnected || !isAdmin}
              placeholder={isAdmin ? "Enter a shell command" : "Admin access required"}
              className="flex-1 bg-transparent font-mono text-sm text-slate-100 outline-none placeholder:text-slate-500 disabled:cursor-not-allowed"
            />
            <button
              type="submit"
              disabled={!isConnected || !isAdmin || !command.trim()}
              className="inline-flex items-center gap-2 rounded-lg bg-blue-600 px-3 py-2 text-sm font-medium text-white transition hover:bg-blue-500 disabled:cursor-not-allowed disabled:bg-slate-700"
            >
              <CornerDownLeft className="h-4 w-4" />
              Send
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}
