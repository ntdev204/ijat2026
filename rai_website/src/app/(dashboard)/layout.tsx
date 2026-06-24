import { DashboardLayout } from "@/components/layout/DashboardLayout";
import { OperationModeProvider } from "@/contexts/OperationModeContext";

export default function Layout({ children }: { children: React.ReactNode }) {
  return (
    <OperationModeProvider>
      <DashboardLayout>
        {children}
      </DashboardLayout>
    </OperationModeProvider>
  );
}
