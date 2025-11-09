import { useEffect, useState } from 'react';
import { Button } from '@/components/ui/button';
import { Card } from '@/components/ui/card';
import { CheckCircle2 } from 'lucide-react';
import { useOrders } from '@/contexts/OrderContext';
import { getESP32BaseUrl } from '@/config/esp32.config';

interface RobotMission {
  tables: number[];
  orders: any[];
  timestamp: string;
}

const cuteEmojis = [
  '(◕‿◕)',
  '(づ｡◕‿‿◕｡)づ',
  '(ﾉ◕ヮ◕)ﾉ*:･ﾟ✧',
  '(◠‿◠)',
  '(✿◠‿◠)',
  '(◕ᴗ◕✿)',
  '(｡♥‿♥｡)',
  '(◕‿◕✿)',
];

type RobotState = 'idle' | 'traveling' | 'arrived' | 'completed';

// ESP32 Communication Functions
const ESP32_BASE_URL = getESP32BaseUrl();

const esp32Commands = {
  // Send navigation command to ESP32
  navigateToTable: async (tableNumber: number) => {
    try {
      const response = await fetch(`${ESP32_BASE_URL}/navigate`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ table: tableNumber })
      });
      return await response.json();
    } catch (error) {
      console.error('ESP32 navigation error:', error);
      return { success: false };
    }
  },

  // Check if robot has arrived
  checkArrival: async () => {
    try {
      const response = await fetch(`${ESP32_BASE_URL}/status`);
      const data = await response.json();
      return data.arrived === true;
    } catch (error) {
      console.error('ESP32 status check error:', error);
      return false;
    }
  },

  // Send robot home
  returnToBase: async () => {
    try {
      const response = await fetch(`${ESP32_BASE_URL}/return-home`, {
        method: 'POST'
      });
      return await response.json();
    } catch (error) {
      console.error('ESP32 return home error:', error);
      return { success: false };
    }
  }
};

const RobotTablet = () => {
  const { orders } = useOrders();
  const [mission, setMission] = useState<RobotMission | null>(null);
  const [currentEmoji, setCurrentEmoji] = useState(0);
  const [robotState, setRobotState] = useState<RobotState>('idle');

  // Register service worker
  useEffect(() => {
    if ('serviceWorker' in navigator) {
      navigator.serviceWorker.register('/sw.js')
        .then(() => console.log('Service Worker registered'))
        .catch((err) => console.log('Service Worker registration failed:', err));
    }
  }, []);

  // Force fullscreen mode on mount
  useEffect(() => {
    const enterFullscreen = async () => {
      try {
        const elem = document.documentElement;
        if (elem.requestFullscreen) {
          await elem.requestFullscreen();
        } else if ((elem as any).webkitRequestFullscreen) {
          await (elem as any).webkitRequestFullscreen();
        } else if ((elem as any).mozRequestFullScreen) {
          await (elem as any).mozRequestFullScreen();
        } else if ((elem as any).msRequestFullscreen) {
          await (elem as any).msRequestFullscreen();
        }
      } catch (error) {
        console.log('Fullscreen not available:', error);
      }
    };

    // Enter fullscreen after a short delay to ensure page is loaded
    setTimeout(enterFullscreen, 500);

    // Re-enter fullscreen if user exits (kiosk mode)
    const handleFullscreenChange = () => {
      if (!document.fullscreenElement && 
          !(document as any).webkitFullscreenElement && 
          !(document as any).mozFullScreenElement && 
          !(document as any).msFullscreenElement) {
        // User exited fullscreen, re-enter it after a brief moment
        setTimeout(enterFullscreen, 1000);
      }
    };

    document.addEventListener('fullscreenchange', handleFullscreenChange);
    document.addEventListener('webkitfullscreenchange', handleFullscreenChange);
    document.addEventListener('mozfullscreenchange', handleFullscreenChange);
    document.addEventListener('MSFullscreenChange', handleFullscreenChange);

    // Prevent context menu (right-click)
    const preventContextMenu = (e: MouseEvent) => e.preventDefault();
    document.addEventListener('contextmenu', preventContextMenu);

    // Prevent text selection
    document.body.style.userSelect = 'none';
    document.body.style.webkitUserSelect = 'none';

    return () => {
      document.removeEventListener('fullscreenchange', handleFullscreenChange);
      document.removeEventListener('webkitfullscreenchange', handleFullscreenChange);
      document.removeEventListener('mozfullscreenchange', handleFullscreenChange);
      document.removeEventListener('MSFullscreenChange', handleFullscreenChange);
      document.removeEventListener('contextmenu', preventContextMenu);
      document.body.style.userSelect = '';
      document.body.style.webkitUserSelect = '';
    };
  }, []);

  useEffect(() => {
    // Load paid orders from context
    const paidOrders = orders.filter(o => o.status === 'paid');
    
    if (paidOrders.length > 0) {
      const tables = [...new Set(paidOrders.map(o => o.tableNumber))];
      setMission({
        tables,
        orders: paidOrders,
        timestamp: new Date().toISOString()
      });
      setRobotState('traveling');
      
      // Send ESP32 command to navigate to first table
      esp32Commands.navigateToTable(tables[0]);
    } else {
      // Check localStorage for mission (from staff dashboard)
      const missionData = localStorage.getItem('robotMission');
      if (missionData) {
        const loadedMission = JSON.parse(missionData);
        setMission(loadedMission);
        setRobotState('traveling');
        
        // Send ESP32 command
        esp32Commands.navigateToTable(loadedMission.tables[0]);
      }
    }

    // Emoji rotation animation - ALWAYS ACTIVE
    const emojiInterval = setInterval(() => {
      setCurrentEmoji((prev) => (prev + 1) % cuteEmojis.length);
    }, 800);

    // Check ESP32 arrival status periodically
    const arrivalCheckInterval = setInterval(async () => {
      if (robotState === 'traveling') {
        const arrived = await esp32Commands.checkArrival();
        if (arrived) {
          setRobotState('arrived');
        }
      }
    }, 2000); // Check every 2 seconds

    // FALLBACK: Simulate arrival after 10 seconds if ESP32 not responding
    const arrivalTimeout = setTimeout(() => {
      if (robotState === 'traveling') {
        console.log('Using simulated arrival (ESP32 not responding)');
        setRobotState('arrived');
      }
    }, 10000);

    return () => {
      clearInterval(emojiInterval);
      clearInterval(arrivalCheckInterval);
      clearTimeout(arrivalTimeout);
    };
  }, [orders, robotState]);

  const handleDone = async () => {
    setRobotState('completed');
    localStorage.removeItem('robotMission');
    
    // Send ESP32 command to return home
    await esp32Commands.returnToBase();
    
    // Close the window after a short delay
    setTimeout(() => {
      window.close();
    }, 2000);
  };

  // Always show interface, even without mission (for demo/testing)
  const displayMission = mission || {
    tables: [1],
    orders: [],
    timestamp: new Date().toISOString()
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-blue-50 via-indigo-50 to-purple-50 flex flex-col items-center justify-center p-6">
      {(robotState === 'idle' || robotState === 'traveling') && (
        <div className="text-center space-y-8 animate-fade-in">
          {/* AVIV Logo */}
          <div className="relative">
            <div className="absolute inset-0 bg-gradient-to-r from-blue-400 via-indigo-400 to-purple-400 rounded-full blur-3xl opacity-30 animate-pulse" />
            <div className="relative bg-white rounded-3xl shadow-2xl p-8 border-4 border-indigo-200">
              {/* AVIV Logo Image */}
              <img 
                src="/aviv-logo.svg" 
                alt="AVIV" 
                className="w-80 h-auto mx-auto"
              />
            </div>
          </div>

          {/* Cute Robot Face Animation */}
          <div className="space-y-4">
            <div className="text-8xl font-mono animate-bounce">
              {cuteEmojis[currentEmoji]}
            </div>
            <div className="text-3xl font-bold bg-gradient-to-r from-blue-600 to-purple-600 bg-clip-text text-transparent animate-pulse">
              {robotState === 'idle' ? 'Waiting for orders...' : 'On my way to deliver...'}
            </div>
            {mission && mission.tables.length > 0 && (
              <div className="text-xl text-gray-600">
                Delivering to Table{mission.tables.length > 1 ? 's' : ''}: {' '}
                <span className="font-bold text-indigo-600">
                  {mission.tables.join(', ')}
                </span>
              </div>
            )}
          </div>

          {/* Loading animation */}
          <div className="flex gap-2 justify-center">
            <div className="w-4 h-4 bg-blue-500 rounded-full animate-bounce" style={{ animationDelay: '0ms' }} />
            <div className="w-4 h-4 bg-indigo-500 rounded-full animate-bounce" style={{ animationDelay: '150ms' }} />
            <div className="w-4 h-4 bg-purple-500 rounded-full animate-bounce" style={{ animationDelay: '300ms' }} />
          </div>

          {/* Test button to simulate arrival */}
          {robotState === 'traveling' && (
            <Button
              onClick={() => setRobotState('arrived')}
              variant="outline"
              className="mt-8 text-sm"
            >
              🧪 Test: Simulate Arrival
            </Button>
          )}
        </div>
      )}

      {robotState === 'arrived' && mission && mission.orders.length > 0 && (
        <div className="max-w-2xl w-full space-y-6 animate-fade-in">
          {/* Success Animation */}
          <div className="text-center space-y-4">
            <div className="text-7xl animate-bounce">
              (◕‿◕✿)
            </div>
            <div className="text-3xl font-bold bg-gradient-to-r from-green-600 to-emerald-600 bg-clip-text text-transparent">
              I've arrived! 🎉
            </div>
          </div>

          {/* Order Information */}
          <Card className="p-8 bg-white/90 backdrop-blur-sm shadow-2xl border-2 border-indigo-200">
            <h2 className="text-2xl font-bold text-gray-800 mb-6 text-center">
              📋 Your Order
            </h2>
            
            <div className="space-y-4">
              {mission.orders.map((order) => (
                <div key={order.id} className="border-b border-gray-200 pb-4 last:border-0">
                  <div className="font-semibold text-indigo-600 mb-2">
                    Table {order.tableNumber}
                  </div>
                  <div className="space-y-2">
                    {order.items.map((item: any, idx: number) => (
                      <div key={idx} className="flex justify-between text-gray-700">
                        <span>
                          <span className="font-bold">{item.quantity}x</span> {item.name}
                        </span>
                        <span className="font-medium">
                          ${(item.price * item.quantity).toFixed(2)}
                        </span>
                      </div>
                    ))}
                  </div>
                  <div className="mt-2 pt-2 border-t border-gray-100 flex justify-between font-bold text-gray-800">
                    <span>Total:</span>
                    <span className="text-green-600">${order.total.toFixed(2)}</span>
                  </div>
                </div>
              ))}
            </div>
          </Card>

          {/* Done Button */}
          <div className="text-center">
            <Button
              onClick={handleDone}
              size="lg"
              className="group px-16 py-8 text-3xl font-bold bg-gradient-to-r from-green-500 via-emerald-500 to-teal-500 hover:from-green-600 hover:via-emerald-600 hover:to-teal-600 text-white shadow-2xl hover:shadow-3xl transform hover:scale-105 transition-all duration-300 rounded-2xl"
            >
              <CheckCircle2 className="inline-block mr-3 h-10 w-10" />
              DONE ✓
            </Button>
            <p className="mt-4 text-gray-600 text-sm">
              Click when you've picked up your order
            </p>
          </div>
        </div>
      )}

      {robotState === 'completed' && (
        <div className="text-center space-y-8 animate-fade-in">
          <div className="text-8xl animate-bounce">
            (づ｡◕‿‿◕｡)づ
          </div>
          <div className="text-4xl font-bold bg-gradient-to-r from-green-600 to-emerald-600 bg-clip-text text-transparent">
            Thank you! Enjoy your meal! 🍽️
          </div>
          <div className="text-xl text-gray-600">
            Returning to base...
          </div>
        </div>
      )}
    </div>
  );
};

export default RobotTablet;
