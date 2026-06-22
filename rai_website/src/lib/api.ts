const DEFAULT_ROBOT_API_PORT = "8080";
const DEFAULT_LAN_HOST = "100.77.136.102";

function configuredApiBaseUrl() {
  return process.env.NEXT_PUBLIC_API_URL?.trim();
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

export function getWebSocketBaseUrl() {
  const configured = process.env.NEXT_PUBLIC_WS_URL?.trim();
  if (configured) return configured.replace(/\/$/, "");

  const apiUrl = getApiBaseUrl();
  return apiUrl.replace(/^http:/, "ws:").replace(/^https:/, "wss:");
}

export function resolveApiEndpoint(endpoint: string) {
  if (/^https?:\/\//.test(endpoint)) return endpoint;
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
