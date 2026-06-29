const DEFAULT_ROBOT_API_PORT = "8080";

function trimTrailingSlash(url: string): string {
  return url.replace(/\/$/, "");
}

function configuredApiBaseUrl(): string | undefined {
  return process.env.NEXT_PUBLIC_API_URL?.trim();
}

function configuredPiApiBaseUrl(): string | undefined {
  return process.env.NEXT_PUBLIC_PI_API_URL?.trim();
}

function configuredJetsonApiBaseUrl(): string | undefined {
  return process.env.NEXT_PUBLIC_JETSON_API_URL?.trim();
}

function buildOrigin(protocol: string, hostname: string, port?: string): string {
  return port ? `${protocol}//${hostname}:${port}` : `${protocol}//${hostname}`;
}

function getErrorDetail(payload: unknown): string | undefined {
  if (!payload || typeof payload !== "object" || !("detail" in payload)) {
    return undefined;
  }

  const detail = payload.detail;
  return typeof detail === "string" ? detail : undefined;
}

function toWebSocketOrigin(apiBaseUrl: string): string {
  const socketUrl = new URL(apiBaseUrl);
  if (socketUrl.protocol === "http:") {
    socketUrl.protocol = "ws:";
  } else if (socketUrl.protocol === "https:") {
    socketUrl.protocol = "wss:";
  }
  return trimTrailingSlash(socketUrl.toString());
}

function resolveWebSocketPath(path: string): string {
  if (path.startsWith("/api/ws/")) return path;
  if (path.startsWith("/ws/")) return `/api${path}`;
  return path;
}

function resolveDirectWebSocketBase(path: string): string | undefined {
  const normalized = resolveWebSocketPath(path);

  if (normalized === "/api/webrtc/offer") {
    const configuredJetson = configuredJetsonApiBaseUrl();
    return configuredJetson ? toWebSocketOrigin(configuredJetson) : undefined;
  }

  if (
    normalized.startsWith("/api/ws/telemetry") ||
    normalized.startsWith("/api/ws/map") ||
    normalized.startsWith("/api/ws/paths") ||
    normalized.startsWith("/api/ws/dataset") ||
    normalized.startsWith("/api/ws/control")
  ) {
    const configuredPi = configuredPiApiBaseUrl();
    return configuredPi ? toWebSocketOrigin(configuredPi) : undefined;
  }

  return undefined;
}

export function getApiBaseUrl(): string {
  const configured = configuredApiBaseUrl();
  if (configured) return trimTrailingSlash(configured);

  if (typeof window !== "undefined") {
    if (window.location.port === "3000") {
      return buildOrigin(window.location.protocol, window.location.hostname, DEFAULT_ROBOT_API_PORT);
    }
    return window.location.origin;
  }

  return `http://localhost:${DEFAULT_ROBOT_API_PORT}`;
}

export function getWebSocketBaseUrl(path?: string): string {
  const configured = process.env.NEXT_PUBLIC_WS_URL?.trim();
  if (!path && configured) return trimTrailingSlash(configured);

  if (path) {
    const directBase = resolveDirectWebSocketBase(path);
    if (directBase) return directBase;
  }

  if (configured) return trimTrailingSlash(configured);
  return toWebSocketOrigin(getApiBaseUrl());
}

export function resolveApiEndpoint(endpoint: string): string {
  if (/^https?:\/\//.test(endpoint)) return endpoint;
  return new URL(endpoint, `${getApiBaseUrl()}/`).toString();
}

export function resolveWebSocketEndpoint(path: string): string {
  if (/^wss?:\/\//.test(path)) return path;
  return new URL(resolveWebSocketPath(path), `${getWebSocketBaseUrl(path)}/`).toString();
}

export class ApiError extends Error {
  status: number;

  constructor(message: string, status: number) {
    super(message);
    this.status = status;
  }
}

export async function fetchWithAuth(endpoint: string, options: RequestInit = {}): Promise<Response> {
  const headers = new Headers(options.headers || {});
  if (!headers.has("Content-Type") && !(options.body instanceof FormData)) {
    headers.set("Content-Type", "application/json");
  }

  const config: RequestInit = {
    ...options,
    headers,
  };

  let response: Response;
  try {
    response = await fetch(resolveApiEndpoint(endpoint), config);
  } catch {
    throw new ApiError("Cannot reach the robot API.", 0);
  }

  if (!response.ok) {
    let message = "An error occurred";
    try {
      message = getErrorDetail(await response.json()) || message;
    } catch {
      message = response.statusText;
    }
    throw new ApiError(message, response.status);
  }

  return response;
}
