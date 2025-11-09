import { getESP32Url, ESP32_CONFIG } from '@/config/esp32.config';

/**
 * Send table number notification to ESP32
 * @param tableNumber - The table number from which the order was placed
 * @returns Promise that resolves when notification is sent successfully
 */
export const sendTableNotificationToESP32 = async (tableNumber: number): Promise<void> => {
  const url = getESP32Url();
  
  try {
    console.log(`Sending table ${tableNumber} notification to ESP32 at ${url}`);
    
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), ESP32_CONFIG.timeout);
    
    const response = await fetch(url, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        tableNumber: tableNumber,
        timestamp: new Date().toISOString(),
      }),
      signal: controller.signal,
    });
    
    clearTimeout(timeoutId);
    
    if (!response.ok) {
      throw new Error(`ESP32 responded with status: ${response.status}`);
    }
    
    console.log(`Successfully notified ESP32 about table ${tableNumber}`);
  } catch (error) {
    if (error instanceof Error) {
      if (error.name === 'AbortError') {
        console.error('ESP32 request timed out');
        throw new Error('Failed to connect to ESP32: Request timed out');
      }
      console.error('Error sending notification to ESP32:', error.message);
      throw new Error(`Failed to notify ESP32: ${error.message}`);
    }
    throw new Error('Failed to notify ESP32: Unknown error');
  }
};

/**
 * Test connection to ESP32
 * @returns Promise that resolves to true if ESP32 is reachable
 */
export const testESP32Connection = async (): Promise<boolean> => {
  try {
    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), 3000);
    
    const response = await fetch(`http://${ESP32_CONFIG.ipAddress}:${ESP32_CONFIG.port}/ping`, {
      method: 'GET',
      signal: controller.signal,
    });
    
    clearTimeout(timeoutId);
    return response.ok;
  } catch (error) {
    console.error('ESP32 connection test failed:', error);
    return false;
  }
};
