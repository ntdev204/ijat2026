const DEFAULT_ROBOT_API_PORT = "8080";
const DEFAULT_LAN_HOST = "100.116.199.115";

function configuredApiBaseUrl() {
  return process.env.NEXT_PUBLIC_API_URL?.trim();
}

function configuredPiApiBaseUrl() {
  return process.env.NEXT_PUBLIC_PI_API_URL?.trim();
}

function configuredJetsonApiBaseUrl() {
  return process.env.NEXT_PUBLIC_JETSON_API_URL?.trim();
}

function isFrontendDevHost(location: Location) {
  return location.port === "3000";
}

export function getApiBaseUrl() {
  const configured = configuredApiBaseUrl();
  if (configured) return configured.replace(/\/$/, "");

  if (typeof window !== "undefined") {
    if (isFrontendDevHost(window.location)) {
      return `${window.location.protocol}//${window.location.hostname}:${DEFAULT_ROBOT_API_PORT}`;
    }
    return window.location.origin;
  }

  return `http://${DEFAULT_LAN_HOST}:${DEFAULT_ROBOT_API_PORT}`;
}

function resolveDirectApiBase(endpoint: string) {
  const normalized = endpoint.startsWith("/") ? endpoint : `/${endpoint}`;

  if (normalized === "/api/webrtc/offer") {
    return configuredJetsonApiBaseUrl();
  }

  if (
    normalized.startsWith("/api/dataset") ||
    normalized.startsWith("/api/map") ||
    normalized.startsWith("/api/rai-navigation") ||
    normalized.startsWith("/api/robot") ||
    normalized.startsWith("/api/rviz") ||
    normalized.startsWith("/api/telemetry/current")
  ) {
    return configuredPiApiBaseUrl();
  }

  if (normalized.startsWith("/api/system/components/camera/")) {
    return configuredJetsonApiBaseUrl();
  }

  if (
    normalized.startsWith("/api/system/components/robot/") ||
    normalized.startsWith("/api/system/components/lidar/") ||
    normalized.startsWith("/api/system/components/slam/") ||
    normalized.startsWith("/api/system/components/navigation/") ||
    normalized.startsWith("/api/system/components/dataset/")
  ) {
    return configuredPiApiBaseUrl();
  }

  return undefined;
}

export function getWebSocketBaseUrl(path?: string) {
  const configured = process.env.NEXT_PUBLIC_WS_URL?.trim();
  if (configured && !path) return configured.replace(/\/$/, "");

  const normalized = path?.startsWith("/") ? path : path ? `/${path}` : "";
  if (
    normalized.startsWith("/api/ws/telemetry") ||
    normalized.startsWith("/api/ws/map") ||
    normalized.startsWith("/api/ws/paths") ||
    normalized.startsWith("/api/ws/dataset") ||
    normalized.startsWith("/api/ws/control")
  ) {
    const piBase = configuredPiApiBaseUrl();
    if (piBase) {
      return piBase.replace(/^http:/, "ws:").replace(/^https:/, "wss:").replace(/\/$/, "");
    }
  }

  const apiUrl = getApiBaseUrl();
  return apiUrl.replace(/^http:/, "ws:").replace(/^https:/, "wss:");
}

export function resolveApiEndpoint(endpoint: string) {
  if (/^https?:\/\//.test(endpoint)) return endpoint;
  const directBase = resolveDirectApiBase(endpoint);
  if (directBase) {
    return `${directBase.replace(/\/$/, "")}${endpoint}`;
  }
  return `${getApiBaseUrl()}${endpoint}`;
}

export class ApiError extends Error {
  status: number;
  constructor(message: string, status: number) {
    super(message);
    this.status = status;
  }
}

export async function fetchWithAuth(endpoint: string, options: RequestInit = {}) {
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
      const errData = await response.json();
      message = errData.detail || message;
    } catch {
      message = response.statusText;
    }
    throw new ApiError(message, response.status);
  }

  return response;
}
