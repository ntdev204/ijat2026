"use client";

export const SELECTED_MAP_STORAGE_KEY = "rai:selectedMapId";

export function readSelectedMapId(): number | null {
  if (typeof window === "undefined") return null;
  const rawValue = window.localStorage.getItem(SELECTED_MAP_STORAGE_KEY);
  if (!rawValue) return null;
  const parsed = Number.parseInt(rawValue, 10);
  return Number.isFinite(parsed) ? parsed : null;
}

export function writeSelectedMapId(mapId: number | null) {
  if (typeof window === "undefined") return;
  if (mapId == null) {
    window.localStorage.removeItem(SELECTED_MAP_STORAGE_KEY);
    return;
  }
  window.localStorage.setItem(SELECTED_MAP_STORAGE_KEY, String(mapId));
}
