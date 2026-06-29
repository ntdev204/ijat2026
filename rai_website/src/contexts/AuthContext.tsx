"use client";

import React, { createContext, useContext } from "react";

interface User {
  id: number;
  username: string;
  role: string;
}

interface AuthContextType {
  user: User | null;
  isLoading: boolean;
}

const AuthContext = createContext<AuthContextType | undefined>(undefined);
const LOCAL_ROBOT_USER: User = {
  id: 0,
  username: "robot",
  role: "admin",
};

export function AuthProvider({ children }: { children: React.ReactNode }) {
  const user = LOCAL_ROBOT_USER;
  const isLoading = false;

  return (
    <AuthContext.Provider value={{ user, isLoading }}>
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
