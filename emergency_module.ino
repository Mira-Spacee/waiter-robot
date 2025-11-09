/*
 * AVIV Robot - Emergency Recovery Module
 * 
 * Purpose: Emergency maneuver to return to the black line
 * 
 * Operation:
 * 1. Turn RIGHT 90 degrees
 * 2. Move forward until BOTH IR sensors detect black line
 * 3. Stop for 2 seconds
 * 4. Turn LEFT 90 degrees to align with line (line between both IR sensors)
 * 5. Resume normal operation
 */

// ============================================
// PIN DEFINITIONS
// ============================================

// Motor Pins - Corrected for ENA/ENB
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

// ============================================
// CONSTANTS
// ============================================

#define MOTOR_SPEED 70         // Motor speed for emergency maneuver
#define TURN_SPEED 150          // Speed for turning
#define TURN_90_DELAY 500       // Time to turn 90 degrees (ms) - CALIBRATE THIS!
#define SEARCH_SPEED 100        // Speed while searching for line

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
  
  // Initialize motors stopped
  stopMotors();
  
  Serial.println("\n========================================");
  Serial.println("🚨 AVIV Robot - Emergency Module");
  Serial.println("========================================");
  Serial.println("⚠️  This module performs emergency recovery");
  Serial.println("📍 Operation:");
  Serial.println("   1. Turn RIGHT 90°");
  Serial.println("   2. Search for black line");
  Serial.println("   3. Stop when both IR detect line");
  Serial.println("   4. Turn LEFT 90° to align");
  Serial.println("========================================\n");
  
  delay(3000);  // 3 second delay before starting
  
  Serial.println("🚀 Starting emergency procedure...\n");
}

// ============================================
// MOTOR CONTROL FUNCTIONS
// ============================================

void rotateMotor(int rightMotorSpeed, int leftMotorSpeed) {
  // Right motor direction (IN1, IN2)
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
  
  // Left motor direction (IN3, IN4)
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
  
  // Set PWM speeds
  int rightSpeedPWM = abs(rightMotorSpeed);
  if (rightSpeedPWM > 255) rightSpeedPWM = 255;
  
  int leftSpeedPWM = abs(leftMotorSpeed);
  if (leftSpeedPWM > 255) leftSpeedPWM = 255;
  
  analogWrite(ENA, rightSpeedPWM);
  analogWrite(ENB, leftSpeedPWM);
}

void stopMotors() {
  rotateMotor(0, 0);
  Serial.println("⏸️  Motors STOPPED");
}

void turn90Right() {
  Serial.println("↪️  Turning 90° RIGHT...");
  rotateMotor(TURN_SPEED, -TURN_SPEED);  // Right forward, left backward
  delay(TURN_90_DELAY);
  stopMotors();
  delay(200);
  Serial.println("✅ Turn complete");
}

void turn90Left() {
  Serial.println("↩️  Turning 90° LEFT...");
  rotateMotor(-TURN_SPEED, TURN_SPEED);  // Right backward, left forward
  delay(TURN_90_DELAY);
  stopMotors();
  delay(200);
  Serial.println("✅ Turn complete");
}

void moveForward() {
  rotateMotor(SEARCH_SPEED, SEARCH_SPEED);
}

// ============================================
// SENSOR FUNCTIONS
// ============================================

bool checkBothIROnLine() {
  int leftValue = digitalRead(leftIR);
  int rightValue = digitalRead(rightIR);
  
  // Both sensors detect black line (assuming HIGH = black)
  // If your sensors are inverted (LOW = black), change to: leftValue == LOW && rightValue == LOW
  return (leftValue == HIGH && rightValue == HIGH);
}

void printIRStatus() {
  int leftValue = digitalRead(leftIR);
  int rightValue = digitalRead(rightIR);
  
  Serial.print("IR Sensors - Left: ");
  Serial.print(leftValue == HIGH ? "BLACK" : "WHITE");
  Serial.print(" | Right: ");
  Serial.println(rightValue == HIGH ? "BLACK" : "WHITE");
}

// ============================================
// EMERGENCY PROCEDURE
// ============================================

void emergencyRecovery() {
  Serial.println("\n🚨 EMERGENCY RECOVERY INITIATED");
  Serial.println("========================================\n");
  
  // Step 1: Turn RIGHT 90 degrees
  Serial.println("📍 Step 1: Turning RIGHT 90°");
  turn90Right();
  delay(500);
  
  // Step 2: Move forward and search for black line
  Serial.println("\n📍 Step 2: Searching for black line...");
  Serial.println("   Moving forward until both IR sensors detect line");
  
  unsigned long searchStartTime = millis();
  bool lineFound = false;
  
  while (!lineFound) {
    moveForward();
    
    // Check IR sensors every 100ms
    if (millis() % 100 == 0) {
      printIRStatus();
    }
    
    // Check if both sensors detect line
    if (checkBothIROnLine()) {
      lineFound = true;
      stopMotors();
      
      Serial.println("\n✅ BLACK LINE DETECTED!");
      Serial.println("   Both IR sensors on the line");
    }
    
    // Safety timeout after 10 seconds
    if (millis() - searchStartTime > 10000) {
      stopMotors();
      Serial.println("\n⚠️  TIMEOUT: Line not found after 10 seconds");
      Serial.println("   Please check robot position and IR sensors");
      return;
    }
    
    delay(50);  // Small delay for sensor reading
  }
  
  // Step 3: Stop for 2 seconds
  Serial.println("\n📍 Step 3: Stopping for 2 seconds");
  delay(2000);
  
  // Step 4: Turn LEFT 90 degrees to align with line
  Serial.println("\n📍 Step 4: Aligning with line (Turn LEFT 90°)");
  turn90Left();
  delay(500);
  
  // Final verification
  Serial.println("\n📍 Step 5: Final alignment check");
  printIRStatus();
  
  Serial.println("\n========================================");
  Serial.println("✅ EMERGENCY RECOVERY COMPLETE!");
  Serial.println("========================================");
  Serial.println("🤖 Robot should now be aligned with the line");
  Serial.println("   Line should be BETWEEN the two IR sensors\n");
}

// ============================================
// MAIN LOOP
// ============================================

void loop() {
  // Run emergency procedure once
  emergencyRecovery();
  
  // Stop and wait
  Serial.println("\n⏸️  Emergency procedure complete. Robot stopped.");
  Serial.println("💡 Reset ESP32 to run again\n");
  
  // Infinite loop - do nothing
  while(true) {
    delay(1000);
  }
}
