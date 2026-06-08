// ESP32 Configuration
// Update the IP address below to match your ESP32's IP address on your local network
export const ESP32_CONFIG = {
  // ✅ AUTOMATIC: Use mDNS hostname (works even when IP changes!)
  // No need to update this when ESP32 restarts or gets new IP
  ipAddress: 'restaurant-esp32.local',  // 🎯 Automatic discovery via mDNS
  
  // 📝 FALLBACK: If mDNS doesn't work, replace with the ESP32's actual LAN IP
  // ipAddress: '192.168.1.50',  // Manual IP (use only if mDNS fails)
  
  // Port where your ESP32 is listening (default: 80)
  port: 80,
  
  // Endpoints for different operations
  endpoints: {
    order: '/order',          // Send new order notification
    navigate: '/navigate',    // Navigate to table
    status: '/status',        // Check robot status
    returnHome: '/return-home', // Return to base
    emergency: '/emergency',  // Emergency notification from ESP32
    emergencyStatus: '/emergency/status' // ESP32 checks for staff response
  },
  
  // Timeout for requests (in milliseconds)
  timeout: 5000,
};

// Helper to get full ESP32 URL for specific endpoint
export const getESP32Url = (endpoint: keyof typeof ESP32_CONFIG.endpoints = 'order') => {
  return `http://${ESP32_CONFIG.ipAddress}:${ESP32_CONFIG.port}${ESP32_CONFIG.endpoints[endpoint]}`;
};

// Legacy support
export const getESP32BaseUrl = () => {
  return `http://${ESP32_CONFIG.ipAddress}:${ESP32_CONFIG.port}`;
};
