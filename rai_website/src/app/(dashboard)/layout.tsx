import { DashboardLayout } from "@/components/layout/DashboardLayout";
import { GlobalKeyboardTeleop } from "@/components/control/GlobalKeyboardTeleop";
import { OperationModeProvider } from "@/contexts/OperationModeContext";

export default function Layout({ children }: { children: React.ReactNode }) {
  return (
    <OperationModeProvider>
      <DashboardLayout>
        {children}
        <GlobalKeyboardTeleop />
      </DashboardLayout>
    </OperationModeProvider>
  );
}
