import { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { useOrders } from '@/contexts/OrderContext';
import { TableCard } from '@/components/TableCard';
import { Button } from '@/components/ui/button';
import { LogOut, Rocket, AlertTriangle, CheckCircle, XCircle } from 'lucide-react';
import { toast } from 'sonner';

const StaffDashboard = () => {
  const navigate = useNavigate();
  const { orders, updateOrderStatus } = useOrders();
  const [emergencyAlert, setEmergencyAlert] = useState<{
    active: boolean;
    message: string;
    type: string;
    timestamp: number | null;
  }>({ active: false, message: '', type: '', timestamp: null });

  useEffect(() => {
    if (sessionStorage.getItem('staffAuth') !== 'true') {
      navigate('/staff/login');
    }
  }, [navigate]);

  // Check for emergency notifications every 2 seconds
  useEffect(() => {
    const checkEmergency = async () => {
      try {
        const response = await fetch('http://localhost:3001/api/emergency/check');
        const data = await response.json();
        
        if (data.active && !emergencyAlert.active) {
          // New emergency detected
          setEmergencyAlert({
            active: true,
            message: data.message,
            type: data.type,
            timestamp: data.timestamp
          });
          
          // Play alert sound
          const audio = new Audio('/alert.mp3');
          audio.play().catch(err => console.log('Audio play failed:', err));
          
          toast.error('🚨 AVIV needs help!', {
            description: data.message,
            duration: 10000
          });
        }
      } catch (error) {
        console.log('Emergency check failed:', error);
      }
    };

    const interval = setInterval(checkEmergency, 2000);
    return () => clearInterval(interval);
  }, [emergencyAlert.active]);

  const handleEmergencyResponse = async (response: 'accept' | 'deny') => {
    try {
      await fetch('http://localhost:3001/api/emergency/respond', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ response })
      });
      
      if (response === 'accept') {
        toast.success('✅ Accepted - AVIV will wait for obstacle clearance', {
          duration: 5000
        });
      } else {
        toast.info('🔄 Denied - AVIV running emergency recovery', {
          duration: 5000
        });
      }
      
      setEmergencyAlert({ active: false, message: '', type: '', timestamp: null });
    } catch (error) {
      console.error('Emergency response failed:', error);
      toast.error('Failed to send response to AVIV');
    }
  };

  const handleLogout = () => {
    sessionStorage.removeItem('staffAuth');
    navigate('/staff/login');
  };

  const getTableOrders = (tableNumber: number) => {
    // Show orders that are NOT sent (sent = cleared/hidden)
    return orders
      .filter(o => o.tableNumber === tableNumber && o.status !== 'sent')
      .sort((a, b) => a.timestamp.getTime() - b.timestamp.getTime());
  };

  const handleMarkAsPaid = (tableNumber: number) => {
    const tableOrders = getTableOrders(tableNumber);
    // Mark pending orders as 'paid' - this changes color to GREEN but keeps visible
    tableOrders.forEach(order => {
      if (order.status === 'pending') {
        updateOrderStatus(order.id, 'paid');
      }
    });
  };

  const handleMarkAsSent = (tableNumber: number) => {
    const tableOrders = getTableOrders(tableNumber);
    
    // Check if ALL orders are already paid
    const allPaid = tableOrders.every(order => order.status === 'paid');
    
    if (allPaid && tableOrders.length > 0) {
      // Only clear/refresh the table if all orders are paid
      tableOrders.forEach(order => {
        updateOrderStatus(order.id, 'sent');
      });
    }
    // If not all paid, do nothing (require paid first)
  };

  const handleLaunchAviv = () => {
    // Get all paid orders (ready to be sent)
    const paidOrders = orders.filter(o => o.status === 'paid');
    
    if (paidOrders.length === 0) {
      toast.error('No orders ready to send! Mark orders as paid first.');
      return;
    }
    
    // Send order info to robot tablet interface
    const tableNumbers = [...new Set(paidOrders.map(o => o.tableNumber))];
    
    // Store in localStorage for robot tablet to access
    localStorage.setItem('robotMission', JSON.stringify({
      tables: tableNumbers,
      orders: paidOrders,
      timestamp: new Date().toISOString()
    }));
    
    toast.success(`🚀 AVIV Launched! Delivering to Table${tableNumbers.length > 1 ? 's' : ''} ${tableNumbers.join(', ')}`);
    
    // Open robot tablet interface in new window/tab
    window.open('/robot-tablet', '_blank');
  };

  return (
    <div className="min-h-screen bg-background">
      <header className="bg-card border-b shadow-sm">
        <div className="container mx-auto px-4 py-4 flex justify-between items-center">
          <h1 className="text-2xl font-bold">Staff Dashboard</h1>
          <Button variant="outline" onClick={handleLogout}>
            <LogOut className="h-4 w-4 mr-2" />
            Logout
          </Button>
        </div>
      </header>

      <main className="container mx-auto px-4 py-8">
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 xl:grid-cols-5 gap-6">
          {[1, 2, 3, 4, 5].map(tableNum => {
            const tableOrders = getTableOrders(tableNum);
            return (
              <TableCard
                key={tableNum}
                tableNumber={tableNum}
                orders={tableOrders}
                onSent={() => handleMarkAsSent(tableNum)}
                onPaid={() => handleMarkAsPaid(tableNum)}
              />
            );
          })}
        </div>

        {/* Launch AVIV Button */}
        <div className="mt-12 flex justify-center gap-6">
          <Button
            onClick={handleLaunchAviv}
            size="lg"
            className="group relative px-12 py-8 text-2xl font-bold bg-gradient-to-r from-blue-600 via-indigo-600 to-purple-600 hover:from-blue-700 hover:via-indigo-700 hover:to-purple-700 text-white shadow-2xl hover:shadow-3xl transform hover:scale-105 transition-all duration-300 rounded-2xl overflow-hidden"
          >
            {/* Animated background */}
            <div className="absolute inset-0 bg-gradient-to-r from-blue-400 via-indigo-400 to-purple-400 opacity-0 group-hover:opacity-100 transition-opacity duration-300 animate-pulse" />
            
            {/* Content */}
            <div className="relative flex items-center gap-4">
              <Rocket className="h-8 w-8 animate-bounce" />
              <span className="tracking-wide">🤖 LAUNCH AVIV! 🚀</span>
              <Rocket className="h-8 w-8 animate-bounce" />
            </div>
            
            {/* Shine effect */}
            <div className="absolute inset-0 -left-full group-hover:left-full transition-all duration-1000 bg-gradient-to-r from-transparent via-white/30 to-transparent skew-x-12" />
          </Button>

          {/* Emergency Test Button (for demo purposes) */}
          <Button
            onClick={() => setEmergencyAlert({
              active: true,
              message: 'Come help me! Obstacle blocking path.',
              type: 'help_request',
              timestamp: Date.now()
            })}
            size="lg"
            variant="outline"
            className="group relative px-8 py-8 text-xl font-bold border-2 border-amber-500 text-amber-600 hover:bg-amber-50 shadow-xl hover:shadow-2xl transform hover:scale-105 transition-all duration-300 rounded-2xl"
          >
            <div className="relative flex items-center gap-3">
              <AlertTriangle className="h-7 w-7 animate-pulse" />
              <span className="tracking-wide">🚨 Test Emergency</span>
            </div>
          </Button>
        </div>

        {/* Emergency Alert Modal */}
        {emergencyAlert.active && (
          <div className="fixed inset-0 bg-black/60 backdrop-blur-sm flex items-center justify-center z-50 animate-in fade-in duration-300">
            <div className="bg-white rounded-3xl shadow-2xl p-8 max-w-md w-full mx-4 transform animate-in zoom-in duration-300">
              {/* Alert Header */}
              <div className="flex items-center justify-center mb-6">
                <div className="relative">
                  <div className="absolute inset-0 bg-red-500 rounded-full animate-ping opacity-75" />
                  <div className="relative bg-red-100 rounded-full p-4">
                    <AlertTriangle className="h-12 w-12 text-red-600" />
                  </div>
                </div>
              </div>

              {/* Alert Content */}
              <div className="text-center mb-8">
                <h2 className="text-3xl font-bold text-gray-900 mb-3">
                  🚨 AVIV Needs Help!
                </h2>
                <p className="text-lg text-gray-700 mb-2">
                  {emergencyAlert.message}
                </p>
                <p className="text-sm text-gray-500">
                  {emergencyAlert.timestamp && 
                    `Detected at ${new Date(emergencyAlert.timestamp).toLocaleTimeString()}`}
                </p>
              </div>

              {/* Action Buttons */}
              <div className="space-y-3">
                <Button
                  onClick={() => handleEmergencyResponse('accept')}
                  size="lg"
                  className="w-full bg-gradient-to-r from-green-500 to-emerald-600 hover:from-green-600 hover:to-emerald-700 text-white text-xl font-bold py-6 rounded-xl shadow-lg hover:shadow-xl transform hover:scale-105 transition-all"
                >
                  <CheckCircle className="h-6 w-6 mr-3" />
                  Accept - I'll Clear It
                </Button>

                <Button
                  onClick={() => handleEmergencyResponse('deny')}
                  size="lg"
                  variant="destructive"
                  className="w-full bg-gradient-to-r from-red-500 to-rose-600 hover:from-red-600 hover:to-rose-700 text-xl font-bold py-6 rounded-xl shadow-lg hover:shadow-xl transform hover:scale-105 transition-all"
                >
                  <XCircle className="h-6 w-6 mr-3" />
                  Deny - Run Emergency Module
                </Button>
              </div>

              {/* Info Box */}
              <div className="mt-6 p-4 bg-blue-50 border border-blue-200 rounded-lg">
                <p className="text-sm text-blue-800">
                  <strong>Accept:</strong> AVIV will wait for you to clear the obstacle<br />
                  <strong>Deny:</strong> AVIV will execute emergency recovery (turn right, find line, realign)
                </p>
              </div>
            </div>
          </div>
        )}
      </main>
    </div>
  );
};

export default StaffDashboard;
