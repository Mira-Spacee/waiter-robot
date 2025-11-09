// Backend API Configuration
// This connects to your local Node.js server that stores orders

// Build API base URL with sensible defaults and optional .env overrides
// Priority: VITE_API_BASE_URL > (protocol://host:port + basePath)
const getBackendUrl = () => {
  // Allow full override via Vite env if provided (e.g., http://192.168.1.50:3001/api)
  const envBaseUrl = (import.meta as any)?.env?.VITE_API_BASE_URL as string | undefined;
  if (envBaseUrl) return envBaseUrl.replace(/\/$/, "");

  const protocol = ((import.meta as any)?.env?.VITE_API_PROTOCOL as string) || 'http';
  // If you're on localhost, use localhost; otherwise use the current page's hostname
  const derivedHost = window.location.hostname === 'localhost'
    ? 'localhost'
    : window.location.hostname;

  const host = ((import.meta as any)?.env?.VITE_API_HOST as string) || derivedHost;
  const port = ((import.meta as any)?.env?.VITE_API_PORT as string) || '3001';
  const basePath = ((import.meta as any)?.env?.VITE_API_BASE_PATH as string) || '/api';

  return `${protocol}://${host}:${port}${basePath}`;
};

export const API_BASE_URL = getBackendUrl();

export interface Order {
  id: string;
  tableNumber: number;
  items: any[];
  total: number;
  timestamp: string;
  status: 'pending' | 'sent' | 'paid';
}

// Fetch all orders
export const fetchOrders = async (): Promise<Order[]> => {
  try {
    const response = await fetch(`${API_BASE_URL}/orders`);
    if (!response.ok) throw new Error('Failed to fetch orders');
    return await response.json();
  } catch (error) {
    console.error('Error fetching orders:', error);
    return [];
  }
};

// Create a new order
export const createOrder = async (order: Order): Promise<boolean> => {
  try {
    const response = await fetch(`${API_BASE_URL}/orders`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(order),
    });
    return response.ok;
  } catch (error) {
    console.error('Error creating order:', error);
    return false;
  }
};

// Update order status
export const updateOrderStatus = async (
  orderId: string,
  status: 'sent' | 'paid'
): Promise<boolean> => {
  try {
    const response = await fetch(`${API_BASE_URL}/orders/${orderId}`, {
      method: 'PATCH',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ status }),
    });
    return response.ok;
  } catch (error) {
    console.error('Error updating order:', error);
    return false;
  }
};

// Clear all orders
export const clearAllOrders = async (): Promise<boolean> => {
  try {
    const response = await fetch(`${API_BASE_URL}/orders`, {
      method: 'DELETE',
    });
    return response.ok;
  } catch (error) {
    console.error('Error clearing orders:', error);
    return false;
  }
};

// Check if backend is reachable
export const checkBackendHealth = async (): Promise<boolean> => {
  try {
    const response = await fetch(`${API_BASE_URL}/health`);
    return response.ok;
  } catch (error) {
    console.error('Backend health check failed:', error);
    return false;
  }
};
