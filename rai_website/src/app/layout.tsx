import "@/styles/globals.css";

import type { Metadata } from "next";

import { cn } from "@/lib/utils";

import { TooltipProvider } from "@/components/ui/tooltip";
import { Toaster } from "@/components/ui/sonner";
import { AuthProvider } from "@/contexts/AuthContext";
import { ClientLogReporter } from "@/components/logging/ClientLogReporter";

export const metadata: Metadata = {
  title: "Rai Dashboard",
  description: "A dashboard for monitoring and managing your AI agents.",
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html
      lang="en"
      className={cn("h-full", "antialiased", "font-sans")}
    >
      <body className="min-h-full flex flex-col">
        <AuthProvider>
          <TooltipProvider>
            <ClientLogReporter />
            <Toaster richColors position="top-right" />
            {children}
          </TooltipProvider>
        </AuthProvider>
      </body>
    </html>
  );
}
