import { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { useOrders } from '@/contexts/OrderContext';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Tabs, TabsContent, TabsList, TabsTrigger } from '@/components/ui/tabs';
import { LogOut, TrendingUp, DollarSign, Package, Archive, Trash2, Users } from 'lucide-react';
import { BarChart, Bar, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts';
import { archiveOldOrders, clearAllOrders } from '@/services/archive.service';
import { toast } from 'sonner';

const AdminReports = () => {
  const navigate = useNavigate();
  const { orders } = useOrders();
  const [period, setPeriod] = useState<'daily' | 'weekly' | 'monthly'>('daily');
  const [isArchiving, setIsArchiving] = useState(false);
  const [isClearing, setIsClearing] = useState(false);

  useEffect(() => {
    if (sessionStorage.getItem('adminAuth') !== 'true') {
      navigate('/admin/login');
    }
  }, [navigate]);

  const handleLogout = () => {
    sessionStorage.removeItem('adminAuth');
    navigate('/admin/login');
  };

  const handleArchiveOrders = async () => {
    if (!confirm('📦 Archive orders older than 1 month?\n\nThis will move old orders to a backup file and keep only recent orders in the database for better performance.')) {
      return;
    }

    setIsArchiving(true);
    try {
      const result = await archiveOldOrders(1);
      
      if (result.archivedCount === 0) {
        toast.info('No old orders to archive', {
          description: 'All orders are from the last month.',
        });
      } else {
        toast.success('Orders Archived Successfully!', {
          description: `Archived ${result.archivedCount} old orders. ${result.remainingCount} recent orders remain. Backup: ${result.archiveFile}`,
        });
        
        // Refresh page to show updated data
        setTimeout(() => window.location.reload(), 2000);
      }
    } catch (error) {
      toast.error('Archive Failed', {
        description: 'Failed to archive orders. Please try again.',
      });
    } finally {
      setIsArchiving(false);
    }
  };

  const handleClearAllOrders = async () => {
    if (!confirm('⚠️ DANGER: Clear ALL orders?\n\nThis will delete all orders from the database. A backup file will be created first.\n\nAre you sure?')) {
      return;
    }
    if (!confirm('⚠️ FINAL WARNING!\n\nThis action will clear ALL orders. This is typically done at the end of each month.\n\nClick OK to proceed.')) {
      return;
    }

    setIsClearing(true);
    try {
      const result = await clearAllOrders();
      
      if (result.clearedCount === 0) {
        toast.info('No orders to clear', {
          description: 'Database is already empty.',
        });
      } else {
        toast.success('All Orders Cleared!', {
          description: `Cleared ${result.clearedCount} orders. Backup saved to: ${result.backupFile}`,
        });
        
        // Refresh page to show empty data
        setTimeout(() => window.location.reload(), 2000);
      }
    } catch (error) {
      toast.error('Clear Failed', {
        description: 'Failed to clear orders. Please try again.',
      });
    } finally {
      setIsClearing(false);
    }
  };

  // Filter completed orders (sent = completed/cleared)
  const completedOrders = orders.filter(o => o.status === 'sent');

  const totalOrders = completedOrders.length;
  const totalEarnings = completedOrders.reduce((sum, order) => sum + order.total, 0);

  // Count item frequencies
  const itemCounts: { [key: string]: number } = {};
  completedOrders.forEach(order => {
    order.items.forEach(item => {
      itemCounts[item.name] = (itemCounts[item.name] || 0) + item.quantity;
    });
  });

  const favoriteItem = Object.entries(itemCounts).sort((a, b) => b[1] - a[1])[0];

  // Chart data
  const chartData = Object.entries(itemCounts).map(([name, count]) => ({
    name,
    orders: count
  }));

  return (
    <div className="min-h-screen bg-background">
      <header className="bg-card border-b shadow-sm">
        <div className="container mx-auto px-4 py-4 flex justify-between items-center">
          <h1 className="text-2xl font-bold">Admin Reports</h1>
          <div className="flex gap-2">
            <Button 
              variant="outline" 
              onClick={() => navigate('/admin/users')}
              className="border-purple-500 text-purple-600 hover:bg-purple-50"
            >
              <Users className="h-4 w-4 mr-2" />
              Manage Users
            </Button>
            <Button 
              variant="outline" 
              onClick={handleArchiveOrders}
              disabled={isArchiving}
              className="border-orange-500 text-orange-600 hover:bg-orange-50"
            >
              <Archive className="h-4 w-4 mr-2" />
              {isArchiving ? 'Archiving...' : 'Archive Old Orders'}
            </Button>
            <Button 
              variant="destructive" 
              onClick={handleClearAllOrders}
              disabled={isClearing}
            >
              <Trash2 className="h-4 w-4 mr-2" />
              {isClearing ? 'Clearing...' : 'Clear All'}
            </Button>
            <Button variant="outline" onClick={handleLogout}>
              <LogOut className="h-4 w-4 mr-2" />
              Logout
            </Button>
          </div>
        </div>
      </header>

      <main className="container mx-auto px-4 py-8">
        <Tabs value={period} onValueChange={(v) => setPeriod(v as any)} className="space-y-6">
          <TabsList className="grid w-full max-w-md grid-cols-3">
            <TabsTrigger value="daily">Daily</TabsTrigger>
            <TabsTrigger value="weekly">Weekly</TabsTrigger>
            <TabsTrigger value="monthly">Monthly</TabsTrigger>
          </TabsList>

          <TabsContent value={period} className="space-y-6">
            <div className="grid grid-cols-1 md:grid-cols-3 gap-6">
              <Card>
                <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
                  <CardTitle className="text-sm font-medium">Total Orders</CardTitle>
                  <Package className="h-4 w-4 text-muted-foreground" />
                </CardHeader>
                <CardContent>
                  <div className="text-3xl font-bold">{totalOrders}</div>
                  <p className="text-xs text-muted-foreground mt-1">
                    Completed orders
                  </p>
                </CardContent>
              </Card>

              <Card>
                <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
                  <CardTitle className="text-sm font-medium">Total Earnings</CardTitle>
                  <DollarSign className="h-4 w-4 text-muted-foreground" />
                </CardHeader>
                <CardContent>
                  <div className="text-3xl font-bold">${totalEarnings.toFixed(2)}</div>
                  <p className="text-xs text-muted-foreground mt-1">
                    From all orders
                  </p>
                </CardContent>
              </Card>

              <Card>
                <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
                  <CardTitle className="text-sm font-medium">Favorite Item</CardTitle>
                  <TrendingUp className="h-4 w-4 text-muted-foreground" />
                </CardHeader>
                <CardContent>
                  <div className="text-2xl font-bold">
                    {favoriteItem ? favoriteItem[0] : 'N/A'}
                  </div>
                  <p className="text-xs text-muted-foreground mt-1">
                    {favoriteItem ? `${favoriteItem[1]} orders` : 'No data yet'}
                  </p>
                </CardContent>
              </Card>
            </div>

            <Card>
              <CardHeader>
                <CardTitle>Orders by Item</CardTitle>
              </CardHeader>
              <CardContent>
                {chartData.length > 0 ? (
                  <ResponsiveContainer width="100%" height={300}>
                    <BarChart data={chartData}>
                      <CartesianGrid strokeDasharray="3 3" />
                      <XAxis dataKey="name" />
                      <YAxis />
                      <Tooltip />
                      <Legend />
                      <Bar dataKey="orders" fill="hsl(var(--primary))" />
                    </BarChart>
                  </ResponsiveContainer>
                ) : (
                  <div className="h-[300px] flex items-center justify-center text-muted-foreground">
                    No data available yet
                  </div>
                )}
              </CardContent>
            </Card>
          </TabsContent>
        </Tabs>
      </main>
    </div>
  );
};

export default AdminReports;
