"use client";

import React, { createContext, useContext, useState, useEffect } from "react";
import { useRouter, usePathname } from "next/navigation";
import { getValidAccessToken } from "@/lib/api";

interface User {
  id: number;
  username: string;
  role: string;
  face_auth_enabled?: boolean;
  face_id?: string | null;
  avatar_url?: string;
}

interface AuthContextType {
  user: User | null;
  isLoading: boolean;
  authEnabled: boolean;
  login: (access: string, refresh: string) => Promise<void>;
  logout: () => void;
}

const AuthContext = createContext<AuthContextType | undefined>(undefined);
const AUTH_ENABLED = process.env.NEXT_PUBLIC_AUTH_ENABLED === "true";
const LOCAL_ROBOT_USER: User = {
  id: 0,
  username: "robot",
  role: "admin",
};

export function AuthProvider({ children }: { children: React.ReactNode }) {
  const [user, setUser] = useState<User | null>(null);
  const [isLoading, setIsLoading] = useState(true);
  const router = useRouter();
  const pathname = usePathname();

  useEffect(() => {
    const initAuth = async () => {
      if (!AUTH_ENABLED) {
        setUser(LOCAL_ROBOT_USER);
        setIsLoading(false);
        return;
      }

      const token = localStorage.getItem("access_token");
      if (!token) {
        setIsLoading(false);
        if (pathname !== "/login") router.push("/login");
        return;
      }

      try {
        const validToken = await getValidAccessToken();
        if (!validToken) {
          throw new Error("Token expired");
        }
        const payload = JSON.parse(atob(validToken.split('.')[1]));
        setUser({
          id: parseInt(payload.sub),
          username: payload.username ?? payload.sub,
          role: payload.role,
          face_auth_enabled: Boolean(payload.face_auth_enabled),
          face_id: payload.face_id ?? null,
        });
    } catch {
        localStorage.removeItem("access_token");
        localStorage.removeItem("refresh_token");
        if (pathname !== "/login") router.push("/login");
      } finally {
        setIsLoading(false);
      }
    };

    initAuth();
  }, [pathname, router]);

  const login = async (access: string, refresh: string) => {
    localStorage.setItem("access_token", access);
    localStorage.setItem("refresh_token", refresh);
    const payload = JSON.parse(atob(access.split('.')[1]));
    setUser({
      id: parseInt(payload.sub),
      username: payload.username ?? payload.sub,
      role: payload.role,
      face_auth_enabled: Boolean(payload.face_auth_enabled),
      face_id: payload.face_id ?? null,
    });
    router.push("/");
  };

  const logout = () => {
    if (!AUTH_ENABLED) {
      setUser(LOCAL_ROBOT_USER);
      return;
    }

    localStorage.removeItem("access_token");
    localStorage.removeItem("refresh_token");
    setUser(null);
    router.push("/login");
  };

  return (
    <AuthContext.Provider value={{ user, isLoading, authEnabled: AUTH_ENABLED, login, logout }}>
      {children}
    </AuthContext.Provider>
  );
}

export function useAuth() {
  const context = useContext(AuthContext);
  if (context === undefined) {
    throw new Error("useAuth must be used within an AuthProvider");
  }
  return context;
}
