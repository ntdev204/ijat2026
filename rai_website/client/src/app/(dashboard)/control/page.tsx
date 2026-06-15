"use client";

import { useState, useCallback, useRef } from "react";
import { AlertCircle, ArrowUp, ArrowDown, ArrowLeft, ArrowRight, ArrowUpLeft, ArrowUpRight, ArrowDownLeft, ArrowDownRight, RotateCw, StopCircle } from "lucide-react";
import { StatusBadge } from "@/components/ui/StatusBadge";
import { useWebSocket } from "@/hooks/useWebSocket";

export default function ControlPage() {
  const [linearVel, setLinearVel] = useState(0.3); 
  const [angularVel, setAngularVel] = useState(0.5);
  const [activeKeys, setActiveKeys] = useState<Set<string>>(new Set());
  const activeKeysRef = useRef<Set<string>>(new Set());
  const { isConnected, sendMessage } = useWebSocket("/ws/control");
  const [currentCmd, setCurrentCmd] = useState({ x: 0, y: 0, z: 0 });

  const sendCommand = useCallback((x: number, y: number, z: number) => {
    
    const roundedX = Math.round(x * 100) / 100;
    const roundedY = Math.round(y * 100) / 100;
    const roundedZ = Math.round(z * 100) / 100;

    if (!isConnected) return;

    const payload = JSON.stringify({
      linear_x: roundedX,
      linear_y: roundedY,
      angular_z: roundedZ
    });
    sendMessage(payload);
    setCurrentCmd({ x: roundedX, y: roundedY, z: roundedZ });
  }, [isConnected, sendMessage]);

  const calculateVelocity = useCallback((keys: Set<string>) => {
    let x = 0;
    let y = 0;
    let z = 0;

    if (keys.has(" ")) {
      
      return { x: 0, y: 0, z: 0 };
    }

    if (keys.has("w")) x += linearVel;
    if (keys.has("s")) x -= linearVel;
    if (keys.has("a")) y += linearVel;
    if (keys.has("d")) y -= linearVel;
    
    
    if (keys.has("q")) { x += linearVel; y += linearVel; }
    if (keys.has("e")) { x += linearVel; y -= linearVel; }
    if (keys.has("z")) { x -= linearVel; y += linearVel; }
    if (keys.has("c")) { x -= linearVel; y -= linearVel; }

    
    if (keys.has("x")) z -= angularVel; 
    

    
    const magnitude = Math.sqrt(x*x + y*y);
    if (magnitude > linearVel && magnitude > 0) {
      x = (x / magnitude) * linearVel;
      y = (y / magnitude) * linearVel;
    }

    return { x, y, z };
  }, [linearVel, angularVel]);

  const handleButtonDown = (key: string) => {
    const newSet = new Set(activeKeysRef.current);
    newSet.add(key);
    activeKeysRef.current = newSet;
    setActiveKeys(new Set(newSet));
    const vel = calculateVelocity(newSet);
    sendCommand(vel.x, vel.y, vel.z);
  };

  const handleButtonUp = (key: string) => {
    const newSet = new Set(activeKeysRef.current);
    newSet.delete(key);
    activeKeysRef.current = newSet;
    setActiveKeys(new Set(newSet));
    const vel = calculateVelocity(newSet);
    sendCommand(vel.x, vel.y, vel.z);
  };

  const renderButton = (key: string, icon: React.ReactNode, label?: string) => {
    const isActive = activeKeys.has(key);
    return (
      <button 
        onMouseDown={() => handleButtonDown(key)}
        onMouseUp={() => handleButtonUp(key)}
        onMouseLeave={() => handleButtonUp(key)}
        onTouchStart={(e) => { e.preventDefault(); handleButtonDown(key); }}
        onTouchEnd={(e) => { e.preventDefault(); handleButtonUp(key); }}
        className={`w-16 h-16 rounded-xl flex flex-col items-center justify-center transition shadow-sm border select-none
          ${isActive ? "bg-blue-200 text-blue-700 border-blue-400" : "bg-white hover:bg-slate-50 text-slate-700 border-slate-200"}
          ${key === ' ' ? 'col-span-3 w-full bg-rose-50 text-rose-600 border-rose-200 hover:bg-rose-100' : ''}
        `}
      >
        {icon}
        {label && <span className="text-[10px] font-bold mt-1 uppercase">{label}</span>}
      </button>
    );
  };

  return (
    <div className="space-y-6 max-w-5xl mx-auto">
      <div className="flex items-center justify-between">
        <h2 className="text-2xl font-bold text-slate-800 tracking-tight">Mecanum Control</h2>
        <StatusBadge status={isConnected ? "success" : "error"}>
          {isConnected ? "CONNECTED" : "DISCONNECTED"}
        </StatusBadge>
      </div>

      <div className="bg-amber-50 border-l-4 border-amber-500 p-4 rounded-r-lg">
        <div className="flex">
          <AlertCircle className="h-5 w-5 text-amber-500" />
          <div className="ml-3">
            <h3 className="text-sm font-medium text-amber-800">Omnidirectional Control Active</h3>
            <p className="text-sm text-amber-700 mt-1">
              Press and hold to move. Release to stop. Uses WebSocket command bridging.
            </p>
          </div>
        </div>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 gap-8">
        {}
        <div className="bg-white p-8 rounded-xl border border-slate-200 shadow-sm flex flex-col items-center justify-center">
          <div className="grid grid-cols-3 gap-3">
            {renderButton('q', <ArrowUpLeft className="w-6 h-6" />, 'Q')}
            {renderButton('w', <ArrowUp className="w-6 h-6" />, 'W')}
            {renderButton('e', <ArrowUpRight className="w-6 h-6" />, 'E')}

            {renderButton('a', <ArrowLeft className="w-6 h-6" />, 'A')}
            {renderButton('x', <RotateCw className="w-6 h-6" />, 'X')}
            {renderButton('d', <ArrowRight className="w-6 h-6" />, 'D')}

            {renderButton('z', <ArrowDownLeft className="w-6 h-6" />, 'Z')}
            {renderButton('s', <ArrowDown className="w-6 h-6" />, 'S')}
            {renderButton('c', <ArrowDownRight className="w-6 h-6" />, 'C')}
            
            {renderButton(' ', <StopCircle className="w-6 h-6" />, 'SPACE (STOP)')}
          </div>

          <div className="mt-8 text-center text-sm text-slate-500 space-y-1">
            <p><strong>W/S</strong>: Forward/Backward | <strong>A/D</strong>: Strafe Left/Right</p>
            <p><strong>Q/E/Z/C</strong>: Diagonals | <strong>X</strong>: Rotate CW | <strong>Ctrl+X</strong>: Rotate CCW</p>
            <p>Keyboard teleop is global across dashboard pages. <strong>Space</strong>: Stop</p>
          </div>
        </div>

        {}
        <div className="bg-white p-8 rounded-xl border border-slate-200 shadow-sm flex flex-col">
          <h3 className="text-lg font-semibold text-slate-800 mb-6 border-b border-slate-100 pb-2">Velocity Control</h3>
          
          <div className="space-y-8 flex-1">
            <div>
              <div className="flex justify-between mb-2">
                <label className="text-sm font-medium text-slate-700">Linear Velocity (m/s)</label>
                <span className="text-sm font-mono text-blue-600 font-bold">{linearVel.toFixed(2)}</span>
              </div>
              <input 
                type="range" 
                className="w-full h-2 bg-slate-200 rounded-lg appearance-none cursor-pointer" 
                min="0.1" max="1.0" step="0.1" 
                value={linearVel}
                onChange={(e) => setLinearVel(parseFloat(e.target.value))}
              />
              <div className="flex justify-between text-xs text-slate-400 mt-1">
                <span>0.1 (Key: I)</span>
                <span>1.0 max (Key: U)</span>
              </div>
            </div>

            <div>
              <div className="flex justify-between mb-2">
                <label className="text-sm font-medium text-slate-700">Angular Velocity (rad/s)</label>
                <span className="text-sm font-mono text-emerald-600 font-bold">{angularVel.toFixed(2)}</span>
              </div>
              <input 
                type="range" 
                className="w-full h-2 bg-slate-200 rounded-lg appearance-none cursor-pointer" 
                min="0.1" max="2.0" step="0.1" 
                value={angularVel}
                onChange={(e) => setAngularVel(parseFloat(e.target.value))}
              />
              <div className="flex justify-between text-xs text-slate-400 mt-1">
                <span>0.1</span>
                <span>2.0 max</span>
              </div>
            </div>

            <div className="p-4 bg-slate-50 rounded-lg border border-slate-200 mt-auto">
               <p className="text-sm text-slate-600 font-mono">
                 Current Cmd: <br/>
                 vX: {currentCmd.x.toFixed(2)} | vY: {currentCmd.y.toFixed(2)} | vZ: {currentCmd.z.toFixed(2)}
               </p>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
