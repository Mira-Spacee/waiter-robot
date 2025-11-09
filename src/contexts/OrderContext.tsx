import React, { createContext, useContext, useState, useEffect, ReactNode } from 'react';
import { sendTableNotificationToESP32 } from '@/services/esp32.service';
import { toast } from 'sonner';
import * as API from '@/services/api.service';

export interface MenuItem {
  id: string;
  name: string;
  price: number;
  image: string;
  category: 'meals' | 'drinks';
}

export interface BasketItem extends MenuItem {
  quantity: number;
}

export interface Order {
  id: string;
  tableNumber: number;
  items: BasketItem[];
  total: number;
  timestamp: Date;
  status: 'pending' | 'sent' | 'paid';
}

interface OrderContextType {
  basket: BasketItem[];
  orders: Order[];
  addToBasket: (item: MenuItem) => void;
  removeFromBasket: (itemId: string) => void;
  clearBasket: () => void;
  placeOrder: (tableNumber: number) => void;
  updateOrderStatus: (orderId: string, status: 'sent' | 'paid') => void;
  clearAllOrders: () => void;
}

const OrderContext = createContext<OrderContextType | undefined>(undefined);

export const OrderProvider = ({ children }: { children: ReactNode }) => {
  const [basket, setBasket] = useState<BasketItem[]>([]);
  const [orders, setOrders] = useState<Order[]>([]);

  // Load orders from backend on mount
  useEffect(() => {
    const loadOrders = async () => {
      const fetchedOrders = await API.fetchOrders();
      const ordersWithDates = fetchedOrders.map(order => ({
        ...order,
        timestamp: new Date(order.timestamp)
      }));
      setOrders(ordersWithDates);
    };
    loadOrders();
  }, []);

  // Poll for new orders every 3 seconds (real-time updates)
  useEffect(() => {
    const interval = setInterval(async () => {
      const fetchedOrders = await API.fetchOrders();
      const ordersWithDates = fetchedOrders.map(order => ({
        ...order,
        timestamp: new Date(order.timestamp)
      }));
      setOrders(ordersWithDates);
    }, 3000); // Check every 3 seconds

    return () => clearInterval(interval);
  }, []);

  const addToBasket = (item: MenuItem) => {
    setBasket(prev => {
      const existing = prev.find(i => i.id === item.id);
      if (existing) {
        return prev.map(i => 
          i.id === item.id ? { ...i, quantity: i.quantity + 1 } : i
        );
      }
      return [...prev, { ...item, quantity: 1 }];
    });
  };

  const removeFromBasket = (itemId: string) => {
    setBasket(prev => {
      const existing = prev.find(i => i.id === itemId);
      if (existing && existing.quantity > 1) {
        return prev.map(i => 
          i.id === itemId ? { ...i, quantity: i.quantity - 1 } : i
        );
      }
      return prev.filter(i => i.id !== itemId);
    });
  };

  const clearBasket = () => {
    setBasket([]);
  };

  const placeOrder = async (tableNumber: number) => {
    const total = basket.reduce((sum, item) => sum + (item.price * item.quantity), 0);
    const newOrder: Order = {
      id: Date.now().toString(),
      tableNumber,
      items: basket,
      total,
      timestamp: new Date(),
      status: 'pending'
    };
    
    // Send to backend API
    const orderForAPI = {
      ...newOrder,
      timestamp: newOrder.timestamp.toISOString()
    };
    
    const success = await API.createOrder(orderForAPI as any);
    
    if (!success) {
      toast.error('Failed to place order. Please check your connection.');
      return;
    }
    
    // Send table number to ESP32
    try {
      await sendTableNotificationToESP32(tableNumber);
      console.log(`Order sent to ESP32 for table ${tableNumber}`);
    } catch (error) {
      console.error('Failed to send notification to ESP32:', error);
      toast.warning(
        `Order placed but ESP32 notification failed. Please check ESP32 connection.`,
        {
          description: error instanceof Error ? error.message : 'Unknown error',
        }
      );
    }
    
    // Update local state
    setOrders(prev => [...prev, newOrder]);
    clearBasket();
  };

  const updateOrderStatus = async (orderId: string, status: 'sent' | 'paid') => {
    const success = await API.updateOrderStatus(orderId, status);
    
    if (success) {
      setOrders(prev =>
        prev.map(order =>
          order.id === orderId ? { ...order, status } : order
        )
      );
    } else {
      toast.error('Failed to update order status');
    }
  };

  const clearAllOrders = async () => {
    const success = await API.clearAllOrders();
    if (success) {
      setOrders([]);
    }
  };

  return (
    <OrderContext.Provider value={{
      basket,
      orders,
      addToBasket,
      removeFromBasket,
      clearBasket,
      placeOrder,
      updateOrderStatus,
      clearAllOrders
    }}>
      {children}
    </OrderContext.Provider>
  );
};

export const useOrders = () => {
  const context = useContext(OrderContext);
  if (!context) {
    throw new Error('useOrders must be used within OrderProvider');
  }
  return context;
};
