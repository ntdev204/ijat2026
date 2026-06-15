const DEFAULT_ROBOT_API_PORT = "8080";

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

  return `http://localhost:${DEFAULT_ROBOT_API_PORT}`;
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

function parseJwtExp(token: string): number | null {
  try {
    const payload = JSON.parse(atob(token.split(".")[1] || ""));
    const exp = Number(payload?.exp);
    return Number.isFinite(exp) ? exp : null;
  } catch {
    return null;
  }
}

async function refreshAccessToken(refreshToken: string): Promise<{ access: string; refresh: string } | null> {
  try {
    const refreshRes = await fetch(resolveApiEndpoint("/api/auth/refresh"), {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ refresh_token: refreshToken }),
    });

    if (!refreshRes.ok) return null;
    const data = await refreshRes.json();
    if (!data?.access_token || !data?.refresh_token) return null;
    return { access: data.access_token, refresh: data.refresh_token };
  } catch {
    return null;
  }
}

export async function getValidAccessToken(): Promise<string | null> {
  const token = localStorage.getItem("access_token");
  if (!token) return null;

  const exp = parseJwtExp(token);
  const now = Math.floor(Date.now() / 1000);
  const skew = 30;
  if (exp && exp > now + skew) return token;

  const refreshToken = localStorage.getItem("refresh_token");
  if (!refreshToken) return null;

  const refreshed = await refreshAccessToken(refreshToken);
  if (!refreshed) return null;

  localStorage.setItem("access_token", refreshed.access);
  localStorage.setItem("refresh_token", refreshed.refresh);
  return refreshed.access;
}

export async function fetchWithAuth(endpoint: string, options: RequestInit = {}) {
  const token = await getValidAccessToken();
  
  const headers = new Headers(options.headers || {});
  if (token) {
    headers.set("Authorization", `Bearer ${token}`);
  }
  if (!headers.has("Content-Type") && !(options.body instanceof FormData)) {
    headers.set("Content-Type", "application/json");
  }

  const config: RequestInit = {
    ...options,
    headers,
  };

  let response = await fetch(resolveApiEndpoint(endpoint), config);

  if (response.status === 401) {
    const refreshToken = localStorage.getItem("refresh_token");
    const refreshed = refreshToken ? await refreshAccessToken(refreshToken) : null;

    if (refreshed) {
      localStorage.setItem("access_token", refreshed.access);
      localStorage.setItem("refresh_token", refreshed.refresh);
      headers.set("Authorization", `Bearer ${refreshed.access}`);
      response = await fetch(resolveApiEndpoint(endpoint), { ...options, headers });
    } else {
      localStorage.removeItem("access_token");
      localStorage.removeItem("refresh_token");
      window.location.href = "/login";
    }
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
