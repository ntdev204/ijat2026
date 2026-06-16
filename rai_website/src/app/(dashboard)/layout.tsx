import { DashboardLayout } from "@/components/layout/DashboardLayout";
import { GlobalKeyboardTeleop } from "@/components/control/GlobalKeyboardTeleop";

export default function Layout({ children }: { children: React.ReactNode }) {
  return (
    <DashboardLayout>
      {children}
      <GlobalKeyboardTeleop />
    </DashboardLayout>
  );
}
