import { Order } from '@/contexts/OrderContext';
import { Card, CardContent } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Badge } from '@/components/ui/badge';
import { cn } from '@/lib/utils';
import { Clock, DollarSign, UtensilsCrossed } from 'lucide-react';

interface TableCardProps {
  tableNumber: number;
  orders: Order[];
  onSent: () => void;
  onPaid: () => void;
}

export const TableCard = ({ tableNumber, orders, onSent, onPaid }: TableCardProps) => {
  // Get all active orders (not sent = not cleared)
  const activeOrders = orders.filter(o => o.status !== 'sent');
  const hasOrders = activeOrders.length > 0;
  
  // Determine current status
  const getStatus = () => {
    if (!hasOrders) return 'empty';
    
    // If ANY order is still pending, show as pending
    const hasPending = activeOrders.some(o => o.status === 'pending');
    if (hasPending) return 'pending';
    
    // All orders are paid (intermediate state - green color)
    return 'paid';
  };

  const status = getStatus();
  
  // Calculate total
  const totalAmount = activeOrders.reduce((sum, order) => sum + order.total, 0);
  
  // Get all items
  const allItems = activeOrders.flatMap(order => order.items);
  
  // Status configuration
  const getStatusConfig = () => {
    switch (status) {
      case 'pending':
        return {
          badge: 'Waiting',
          badgeClass: 'bg-gradient-to-r from-amber-500 to-orange-500 text-white border-0 shadow-lg shadow-amber-500/30',
          cardClass: 'border-amber-200 shadow-xl shadow-amber-100/50 bg-gradient-to-br from-amber-50 to-orange-50',
          iconClass: 'text-amber-600'
        };
      case 'paid':
        return {
          badge: 'Paid',
          badgeClass: 'bg-gradient-to-r from-green-500 to-emerald-600 text-white border-0 shadow-lg shadow-green-500/30',
          cardClass: 'border-green-200 shadow-xl shadow-green-100/50 bg-gradient-to-br from-green-50 to-emerald-50',
          iconClass: 'text-green-600'
        };
      default:
        return {
          badge: 'Available',
          badgeClass: 'bg-gradient-to-r from-gray-400 to-gray-500 text-white border-0',
          cardClass: 'border-gray-200 shadow-lg bg-gradient-to-br from-gray-50 to-slate-50',
          iconClass: 'text-gray-400'
        };
    }
  };

  const config = getStatusConfig();

  return (
    <Card className={cn(
      "relative overflow-hidden transition-all duration-300 hover:scale-[1.02] hover:shadow-2xl",
      config.cardClass
    )}>
      <div className="absolute top-0 right-0 w-32 h-32 bg-gradient-to-br from-white/60 to-transparent rounded-bl-full" />
      
      <CardContent className="p-6 relative">
        {/* Header */}
        <div className="flex items-center justify-between mb-6">
          <div className="flex items-center gap-3">
            <div className={cn(
              "w-12 h-12 rounded-xl flex items-center justify-center bg-white shadow-md",
              hasOrders && "ring-2 ring-offset-2",
              status === 'pending' && "ring-amber-400",
              status === 'paid' && "ring-green-400"
            )}>
              <UtensilsCrossed className={cn("h-6 w-6", config.iconClass)} />
            </div>
            <h3 className="text-2xl font-bold bg-gradient-to-br from-gray-800 to-gray-600 bg-clip-text text-transparent">
              Table {tableNumber}
            </h3>
          </div>
          
          <Badge className={cn("px-3 py-1.5 text-sm font-semibold", config.badgeClass)}>
            {config.badge}
          </Badge>
        </div>

        {hasOrders ? (
          <div className="space-y-5">
            {/* Order Items */}
            <div className="bg-white/80 backdrop-blur-sm rounded-xl p-4 border border-gray-200/50 shadow-sm">
              <div className="flex items-center gap-2 mb-3">
                <Clock className="h-4 w-4 text-gray-500" />
                <span className="text-xs font-semibold text-gray-600 uppercase tracking-wide">
                  Order Notes
                </span>
              </div>
              <div className="space-y-2 max-h-32 overflow-y-auto custom-scrollbar">
                {allItems.map((item, index) => (
                  <div 
                    key={`${item.id}-${index}`}
                    className="flex justify-between items-center py-1.5 px-2 rounded-lg hover:bg-gray-100/50 transition-colors"
                  >
                    <span className="text-sm text-gray-700">
                      <span className="font-semibold text-gray-900">{item.quantity}x</span> {item.name}
                    </span>
                    <span className="text-sm font-medium text-gray-600">
                      ${(item.price * item.quantity).toFixed(2)}
                    </span>
                  </div>
                ))}
              </div>
            </div>

            {/* Total */}
            <div className="flex items-center justify-between px-4 py-3 bg-white/90 backdrop-blur-sm rounded-xl border border-gray-200/50 shadow-sm">
              <div className="flex items-center gap-2">
                <DollarSign className="h-5 w-5 text-green-600" />
                <span className="text-sm font-medium text-gray-600">Total Amount</span>
              </div>
              <span className="text-xl font-bold bg-gradient-to-r from-green-600 to-emerald-600 bg-clip-text text-transparent">
                ${totalAmount.toFixed(2)}
              </span>
            </div>

            {/* Buttons */}
            <div className="flex gap-3 pt-2">
              {status === 'pending' && (
                <>
                  <Button 
                    onClick={onPaid}
                    className="flex-1 bg-gradient-to-r from-green-500 to-emerald-600 hover:from-green-600 hover:to-emerald-700 text-white shadow-lg shadow-green-500/30 hover:shadow-green-500/50 transition-all duration-200 font-semibold"
                    size="lg"
                  >
                    💵 Mark as Paid
                  </Button>
                  <Button 
                    onClick={onSent}
                    className="flex-1 bg-gradient-to-r from-blue-500 to-indigo-600 hover:from-blue-600 hover:to-indigo-700 text-white shadow-lg shadow-blue-500/30 hover:shadow-blue-500/50 transition-all duration-200 font-semibold"
                    size="lg"
                  >
                    🍽️ Mark as Sent
                  </Button>
                </>
              )}
              {status === 'paid' && (
                <Button 
                  onClick={onSent}
                  className="w-full bg-gradient-to-r from-blue-500 to-indigo-600 hover:from-blue-600 hover:to-indigo-700 text-white shadow-lg shadow-blue-500/30 hover:shadow-blue-500/50 transition-all duration-200 font-semibold"
                  size="lg"
                >
                  🍽️ Mark as Sent (Clear Table)
                </Button>
              )}
            </div>
          </div>
        ) : (
          <div className="text-center py-8">
            <div className="w-16 h-16 mx-auto mb-4 rounded-full bg-gray-100 flex items-center justify-center">
              <UtensilsCrossed className="h-8 w-8 text-gray-300" />
            </div>
            <p className="text-gray-400 font-medium">No active orders</p>
          </div>
        )}
      </CardContent>
    </Card>
  );
};
