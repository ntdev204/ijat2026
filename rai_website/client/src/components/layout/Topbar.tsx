import { useAuth } from "@/contexts/AuthContext";
import { LogOut, User as UserIcon } from "lucide-react";

export function Topbar() {
  const { user, logout, authEnabled } = useAuth();

  return (
    <header className="h-16 bg-white border-b border-slate-200 flex items-center justify-between px-6 shadow-sm z-10">
      <div className="flex items-center gap-4">
        {}
        <h1 className="text-lg font-semibold text-slate-800">Dashboard</h1>
      </div>
      
      <div className="flex items-center gap-4">
        {}
        <div className="flex items-center gap-2 px-3 py-1.5 rounded-full bg-slate-100 border border-slate-200">
          <UserIcon className="w-4 h-4 text-slate-600" />
          <span className="text-sm font-medium text-slate-700">{user?.username || 'User'}</span>
          <span className="text-xs bg-blue-100 text-blue-700 px-2 py-0.5 rounded-full ml-2 uppercase font-bold tracking-wider">
            {user?.role || 'VIEWER'}
          </span>
        </div>
        
        {authEnabled && (
          <button 
            onClick={logout}
            className="p-2 text-slate-500 hover:text-red-600 hover:bg-red-50 rounded-lg transition-colors"
            title="Logout"
          >
            <LogOut className="w-5 h-5" />
          </button>
        )}
      </div>
    </header>
  );
}
