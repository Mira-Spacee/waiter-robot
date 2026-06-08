/*
 * Restaurant Table Notification System - ESP32 Code
 * 
 * This code runs on ESP32 and receives table number notifications
 * from the restaurant ordering website.
 * 
 * Hardware Requirements:
 * - ESP32 development board
 * - Optional: LED, buzzer, or LCD display for notifications
 * 
 * Libraries Required:
 * - WiFi (included with ESP32)
 * - WebServer (included with ESP32)
 * - ArduinoJson (install from Library Manager)
 * - ESPmDNS (included with ESP32)
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>

// ====== CONFIGURATION ======
// Replace with your WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Optional: GPIO pins for notifications (uncomment if using)
// const int LED_PIN = 2;        // Built-in LED
// const int BUZZER_PIN = 4;     // Buzzer pin
// const int BUTTON_PIN = 5;     // Reset button

WebServer server(80);

// ====== HANDLERS ======

// Handle incoming order notifications
void handleOrder() {
  // Enable CORS (allows website to communicate with ESP32)
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  
  if (server.method() == HTTP_OPTIONS) {
    // Handle preflight CORS request
    server.send(204);
    return;
  }
  
  if (server.method() == HTTP_POST) {
    String body = server.arg("plain");
    
    // Parse JSON
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (!error) {
      int tableNumber = doc["tableNumber"];
      const char* timestamp = doc["timestamp"];
      
      // ====== NOTIFICATION LOGIC ======
      // Add your custom notification code here
      
      Serial.println("========================================");
      Serial.print("🔔 NEW ORDER from Table: ");
      Serial.println(tableNumber);
      Serial.print("Time: ");
      Serial.println(timestamp);
      Serial.println("========================================");
      
      // Example: Blink LED
      // digitalWrite(LED_PIN, HIGH);
      // delay(500);
      // digitalWrite(LED_PIN, LOW);
      
      // Example: Trigger buzzer
      // tone(BUZZER_PIN, 1000, 500); // 1000Hz for 500ms
      
      // Example: Display on LCD
      // lcd.clear();
      // lcd.print("Table ");
      // lcd.print(tableNumber);
      
      // Send success response
      StaticJsonDocument<100> response;
      response["status"] = "success";
      response["table"] = tableNumber;
      
      String responseStr;
      serializeJson(response, responseStr);
      
      server.send(200, "application/json", responseStr);
      
    } else {
      Serial.println("❌ Error parsing JSON");
      server.send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid JSON\"}");
    }
  } else {
    server.send(405, "application/json", "{\"status\":\"error\",\"message\":\"Method not allowed\"}");
  }
}

// Health check endpoint
void handlePing() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "OK");
}

// Handle 404 - Not Found
void handleNotFound() {
  server.send(404, "text/plain", "404 - Not Found");
}

// ====== SETUP ======
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n");
  Serial.println("========================================");
  Serial.println("Restaurant Table Notification System");
  Serial.println("========================================");
  
  // Optional: Initialize GPIO pins
  // pinMode(LED_PIN, OUTPUT);
  // pinMode(BUZZER_PIN, OUTPUT);
  // pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Connect to WiFi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ Connected to WiFi!");
    Serial.print("📡 IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("📶 Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.println("========================================");
    
    // Setup mDNS (makes ESP32 discoverable at restaurant-esp32.local)
    if (MDNS.begin("restaurant-esp32")) {
      Serial.println("✅ mDNS responder started!");
      Serial.println("🌐 You can now access ESP32 at:");
      Serial.println("   http://restaurant-esp32.local/order");
      Serial.println("   OR");
      Serial.print("   http://");
      Serial.print(WiFi.localIP());
      Serial.println("/order");
      Serial.println("========================================");
      Serial.println("⚠️  IMPORTANT: Update src/config/esp32.config.ts");
      Serial.println("   Change ipAddress to: 'restaurant-esp32.local'");
      Serial.println("   (No need to update IP when it changes!)");
      Serial.println("========================================");
    } else {
      Serial.println("❌ Error setting up mDNS responder!");
      Serial.println("⚠️  IMPORTANT: Copy the IP address above");
      Serial.println("   and paste it in src/config/esp32.config.ts");
      Serial.println("========================================");
    }
  } else {
    Serial.println("\n❌ Failed to connect to WiFi");
    Serial.println("Please check your credentials and try again");
    return;
  }
  
  // Setup HTTP server routes
  server.on("/order", HTTP_POST, handleOrder);
  server.on("/order", HTTP_OPTIONS, handleOrder); // Handle CORS preflight
  server.on("/ping", HTTP_GET, handlePing);
  server.onNotFound(handleNotFound);
  
  // Start server
  server.begin();
  Serial.println("🌐 HTTP server started");
  Serial.println("Listening for table notifications...\n");
}

// ====== MAIN LOOP ======
void loop() {
  // Handle incoming HTTP requests
  server.handleClient();
  
  // Keep mDNS service running
  MDNS.update();
  
  // Optional: Add button to clear notifications
  // if (digitalRead(BUTTON_PIN) == LOW) {
  //   digitalWrite(LED_PIN, LOW);
  //   delay(200); // Debounce
  // }
  
  // Optional: Add timeout to turn off LED after 5 seconds
  // static unsigned long lastNotification = 0;
  // if (millis() - lastNotification > 5000) {
  //   digitalWrite(LED_PIN, LOW);
  // }
}

// ====== ADDITIONAL FEATURES (Optional) ======

/*
// Example: Blink LED pattern for table number
void blinkTableNumber(int tableNumber) {
  for (int i = 0; i < tableNumber; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(300);
    digitalWrite(LED_PIN, LOW);
    delay(300);
  }
}

// Example: Play different tones for different tables
void playTableTone(int tableNumber) {
  int frequency = 500 + (tableNumber * 100);
  tone(BUZZER_PIN, frequency, 500);
}

// Example: LCD Display (requires LiquidCrystal_I2C library)
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

void displayOnLCD(int tableNumber) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("New Order!");
  lcd.setCursor(0, 1);
  lcd.print("Table ");
  lcd.print(tableNumber);
}
*/

// In your router settings:
WiFi Security: WPA3-Personal
Password: Strong 20+ character password
Hidden SSID: Optional
