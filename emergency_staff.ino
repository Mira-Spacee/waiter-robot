/*
 * AVIV Robot - Emergency Staff Module
 * 
 * Obstacle Detection with Staff Notification System
 * 
 * Features:
 * 1. Detects obstacle in front
 * 2. Waits 20 seconds with audio warnings every 5 seconds
 * 3. Sends notification to staff page
 * 4. Waits for staff response (Accept/Deny)
 * 5. Accept: Wait until obstacle cleared, then continue
 * 6. Deny: Run emergency recovery module
 * 
 * Hardware Requirements:
 * - Motors, Ultrasonic sensors, IR sensors (same as emergency module)
 * - DFPlayer Mini MP3 module for audio
 * - WiFi connection for staff communication
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "DFRobotDFPlayerMini.h"

// ============================================
// PIN DEFINITIONS
// ============================================

// Motor Pins
int ENA = 22;  // Right motor speed (PWM)
int IN1 = 15;
int IN2 = 2;

int ENB = 23;  // Left motor speed (PWM)
int IN3 = 18;
int IN4 = 19;

// Ultrasonic Sensor Pins
int frontTrigPin = 13;
int frontEchoPin = 12;

// IR Sensor Pins
int leftIR = 32;
int rightIR = 33;

// DFPlayer Mini Pins (Software Serial)
#define RX_PIN 16  // Connect to DFPlayer TX
#define TX_PIN 17  // Connect to DFPlayer RX

// ============================================
// WIFI & SERVER CONFIGURATION
// ============================================

const char* ssid = "YOUR_WIFI_SSID";          // Change this
const char* password = "YOUR_WIFI_PASSWORD";   // Change this
const char* serverUrl = "http://192.168.1.100:3001/api/emergency";  // Change to your server IP

// ============================================
// CONSTANTS
// ============================================

#define MOTOR_SPEED 120
#define TURN_SPEED 150
#define TURN_90_DELAY 500
#define SEARCH_SPEED 100
#define OBSTACLE_DISTANCE 10      // cm - Distance to detect obstacle
#define WARNING_INTERVAL 5000     // 5 seconds between warnings
#define TOTAL_WAIT_TIME 20000     // 20 seconds total wait time

// ============================================
// GLOBAL VARIABLES
// ============================================

HardwareSerial mySerial(1);  // Use UART1 for DFPlayer
DFRobotDFPlayerMini myDFPlayer;

enum RobotState {
  NORMAL_OPERATION,
  OBSTACLE_DETECTED,
  WAITING_FOR_CLEARANCE,
  WAITING_FOR_STAFF_RESPONSE,
  STAFF_ACCEPTED,
  STAFF_DENIED,
  EMERGENCY_RECOVERY
};

RobotState currentState = NORMAL_OPERATION;
unsigned long obstacleDetectedTime = 0;
unsigned long lastWarningTime = 0;
int warningCount = 0;

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(115200);
  
  // Motor pins
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  // Ultrasonic pins
  pinMode(frontTrigPin, OUTPUT);
  pinMode(frontEchoPin, INPUT);
  
  // IR sensor pins
  pinMode(leftIR, INPUT);
  pinMode(rightIR, INPUT);
  
  // Initialize DFPlayer
  mySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  
  Serial.println("\n========================================");
  Serial.println("🚨 AVIV Robot - Emergency Staff Module");
  Serial.println("========================================");
  
  if (!myDFPlayer.begin(mySerial)) {
    Serial.println("⚠️  DFPlayer Mini not detected!");
    Serial.println("   Please check:");
    Serial.println("   1. Connections");
    Serial.println("   2. SD card inserted");
    Serial.println("   3. Audio files on SD card");
  } else {
    Serial.println("✅ DFPlayer Mini initialized");
    myDFPlayer.volume(25);  // Set volume (0-30)
  }
  
  // Connect to WiFi
  connectWiFi();
  
  stopMotors();
  
  Serial.println("\n✅ System ready!");
  Serial.println("========================================\n");
}

// ============================================
// WIFI FUNCTIONS
// ============================================

void connectWiFi() {
  Serial.print("📡 Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" ✅ Connected!");
    Serial.print("   IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(" ❌ Failed to connect");
  }
}

// ============================================
// MOTOR CONTROL FUNCTIONS
// ============================================

void rotateMotor(int rightMotorSpeed, int leftMotorSpeed) {
  if (rightMotorSpeed < 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  } else if (rightMotorSpeed > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }
  
  if (leftMotorSpeed < 0) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  } else if (leftMotorSpeed > 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }
  
  analogWrite(ENA, abs(rightMotorSpeed) > 255 ? 255 : abs(rightMotorSpeed));
  analogWrite(ENB, abs(leftMotorSpeed) > 255 ? 255 : abs(leftMotorSpeed));
}

void stopMotors() {
  rotateMotor(0, 0);
}

// ============================================
// SENSOR FUNCTIONS
// ============================================

long getDistance() {
  digitalWrite(frontTrigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(frontTrigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(frontTrigPin, LOW);
  
  long duration = pulseIn(frontEchoPin, HIGH);
  long distance = duration * 0.034 / 2;
  
  return distance;
}

bool checkBothIROnLine() {
  int leftValue = digitalRead(leftIR);
  int rightValue = digitalRead(rightIR);
  return (leftValue == HIGH && rightValue == HIGH);
}

// ============================================
// AUDIO FUNCTIONS
// ============================================

void playWarning() {
  Serial.println("🔊 Playing audio: 'Please clear the black line'");
  myDFPlayer.play(1);  // Play track 001.mp3 from SD card
  delay(100);
}

// ============================================
// HTTP COMMUNICATION FUNCTIONS
// ============================================

void sendStaffNotification(String message, String type) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected!");
    return;
  }
  
  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");
  
  StaticJsonDocument<200> doc;
  doc["type"] = type;
  doc["message"] = message;
  doc["timestamp"] = millis();
  
  String jsonString;
  serializeJson(doc, jsonString);
  
  Serial.println("📤 Sending notification to staff...");
  Serial.println("   " + jsonString);
  
  int httpResponseCode = http.POST(jsonString);
  
  if (httpResponseCode > 0) {
    Serial.print("✅ Response code: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("❌ Error code: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
}

String checkStaffResponse() {
  if (WiFi.status() != WL_CONNECTED) {
    return "none";
  }
  
  HTTPClient http;
  http.begin(String(serverUrl) + "/status");
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    String payload = http.getString();
    
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      String response = doc["response"].as<String>();
      http.end();
      return response;  // "accept", "deny", or "none"
    }
  }
  
  http.end();
  return "none";
}

// ============================================
// EMERGENCY RECOVERY (from previous module)
// ============================================

void turn90Right() {
  Serial.println("↪️  Turning 90° RIGHT...");
  rotateMotor(TURN_SPEED, -TURN_SPEED);
  delay(TURN_90_DELAY);
  stopMotors();
  delay(200);
}

void turn90Left() {
  Serial.println("↩️  Turning 90° LEFT...");
  rotateMotor(-TURN_SPEED, TURN_SPEED);
  delay(TURN_90_DELAY);
  stopMotors();
  delay(200);
}

void emergencyRecovery() {
  Serial.println("\n🚨 STARTING EMERGENCY RECOVERY MODULE");
  Serial.println("========================================\n");
  
  currentState = EMERGENCY_RECOVERY;
  
  // Step 1: Turn RIGHT 90°
  Serial.println("Step 1: Turn RIGHT 90°");
  turn90Right();
  delay(500);
  
  // Step 2: Search for black line
  Serial.println("Step 2: Searching for black line...");
  unsigned long searchStart = millis();
  bool lineFound = false;
  
  while (!lineFound && millis() - searchStart < 10000) {
    rotateMotor(SEARCH_SPEED, SEARCH_SPEED);
    
    if (checkBothIROnLine()) {
      lineFound = true;
      stopMotors();
      Serial.println("✅ Line found!");
    }
    
    delay(50);
  }
  
  if (!lineFound) {
    stopMotors();
    Serial.println("⚠️  Line not found! Manual intervention needed.");
    return;
  }
  
  // Step 3: Stop for 2 seconds
  Serial.println("Step 3: Stopping for 2 seconds");
  delay(2000);
  
  // Step 4: Turn LEFT 90° to align
  Serial.println("Step 4: Turn LEFT 90° to align");
  turn90Left();
  delay(500);
  
  Serial.println("✅ EMERGENCY RECOVERY COMPLETE!\n");
  currentState = NORMAL_OPERATION;
}

// ============================================
// OBSTACLE HANDLING STATE MACHINE
// ============================================

void handleObstacle() {
  long distance = getDistance();
  
  switch (currentState) {
    case NORMAL_OPERATION:
      if (distance < OBSTACLE_DISTANCE && distance > 0) {
        Serial.println("\n⚠️  OBSTACLE DETECTED!");
        Serial.print("   Distance: ");
        Serial.print(distance);
        Serial.println(" cm");
        
        stopMotors();
        obstacleDetectedTime = millis();
        lastWarningTime = millis();
        warningCount = 0;
        currentState = OBSTACLE_DETECTED;
        
        playWarning();
        warningCount++;
      } else {
        // Normal operation - move forward
        rotateMotor(MOTOR_SPEED, MOTOR_SPEED);
      }
      break;
      
    case OBSTACLE_DETECTED:
      {
        unsigned long elapsedTime = millis() - obstacleDetectedTime;
        
        // Play warning every 5 seconds
        if (millis() - lastWarningTime >= WARNING_INTERVAL && warningCount < 4) {
          playWarning();
          warningCount++;
          lastWarningTime = millis();
          Serial.print("🔊 Warning #");
          Serial.print(warningCount);
          Serial.println(" of 4");
        }
        
        // Check if obstacle cleared
        distance = getDistance();
        if (distance >= OBSTACLE_DISTANCE) {
          Serial.println("✅ Obstacle cleared during wait period!");
          currentState = NORMAL_OPERATION;
          break;
        }
        
        // After 20 seconds, notify staff
        if (elapsedTime >= TOTAL_WAIT_TIME) {
          Serial.println("\n⏰ 20 seconds elapsed - Sending staff notification");
          sendStaffNotification("Come help me! Obstacle blocking path.", "help_request");
          currentState = WAITING_FOR_STAFF_RESPONSE;
        }
      }
      break;
      
    case WAITING_FOR_STAFF_RESPONSE:
      {
        String response = checkStaffResponse();
        
        if (response == "accept") {
          Serial.println("✅ Staff ACCEPTED - Waiting for clearance");
          currentState = STAFF_ACCEPTED;
        } else if (response == "deny") {
          Serial.println("❌ Staff DENIED - Running emergency recovery");
          currentState = STAFF_DENIED;
          emergencyRecovery();
        } else {
          // Still waiting - check every 2 seconds
          Serial.println("⏳ Waiting for staff response...");
          delay(2000);
        }
      }
      break;
      
    case STAFF_ACCEPTED:
      {
        distance = getDistance();
        
        if (distance >= OBSTACLE_DISTANCE) {
          Serial.println("✅ Obstacle cleared! Resuming operation");
          delay(1000);
          currentState = NORMAL_OPERATION;
        } else {
          Serial.println("⏳ Waiting for obstacle to be cleared...");
          delay(1000);
        }
      }
      break;
      
    case STAFF_DENIED:
      // Emergency recovery already executed, go back to normal
      currentState = NORMAL_OPERATION;
      break;
  }
}

// ============================================
// MAIN LOOP
// ============================================

void loop() {
  handleObstacle();
  delay(100);  // Small delay for loop stability
}
