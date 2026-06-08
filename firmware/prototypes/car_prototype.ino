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
int leftTrigPin = 27;
int leftEchoPin = 14;
int rightTrigPin = 26;
int rightEchoPin = 25;

// IR Sensor Pins
int leftIR = 32;
int rightIR = 33;

// Constants
#define SLOW_SPEED 120         // Slow speed for precise movement (increased for motor torque)
#define LINE_FOLLOW_SPEED 140  // Speed for line following (increased)
#define TURN_SPEED 150         // Speed for 90-degree turns (increased for better turning)
#define OBSTACLE_DISTANCE 5    // cm - Detection distance
#define WALL_FOLLOW_DISTANCE 10 // cm - Distance to maintain from wall
#define TURN_90_DELAY 500      // ms - Time for 90-degree turn (adjust based on testing)
#define SENSOR_READ_DELAY 50   // ms - Fast sensor reading

// State variables
enum RobotState {
  LINE_FOLLOWING,
  OBSTACLE_DETECTED,
  TURNING_RIGHT,
  MOVING_PARALLEL,
  TURNING_FORWARD,
  MOVING_FORWARD,
  TURNING_LEFT,
  RETURNING,
  ALIGNING_TO_LINE
};

RobotState currentState = LINE_FOLLOWING;
long distanceTraveled = 0;  // Track distance during parallel movement
unsigned long lastSensorRead = 0;

void setupPins() {
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
  pinMode(leftTrigPin, OUTPUT);
  pinMode(leftEchoPin, INPUT);
  pinMode(rightTrigPin, OUTPUT);
  pinMode(rightEchoPin, INPUT);
  
  // IR sensor pins
  pinMode(leftIR, INPUT);
  pinMode(rightIR, INPUT);
  
  // Initialize motors stopped
  rotateMotor(0, 0);
}

long getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH);
  long distance = duration * 0.034 / 2;
  
  return distance;
}

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
  
  // Constrain speeds to valid PWM range 0-255 (assuming 8-bit PWM)  
  int rightSpeedPWM = abs(rightMotorSpeed);
  if (rightSpeedPWM > 255) rightSpeedPWM = 255;
  
  int leftSpeedPWM = abs(leftMotorSpeed);
  if (leftSpeedPWM > 255) leftSpeedPWM = 255;
  
  // Write PWM outputs
  analogWrite(ENA, rightSpeedPWM);
  analogWrite(ENB, leftSpeedPWM);
}

void stopMotors() {
  rotateMotor(0, 0);
}

// Turn 90 degrees to the right
void turn90Right() {
  Serial.println("Turning 90° RIGHT");
  rotateMotor(TURN_SPEED, -TURN_SPEED);  // Right motor forward, left motor backward
  delay(TURN_90_DELAY);
  stopMotors();
  delay(100);
}

// Turn 90 degrees to the left
void turn90Left() {
  Serial.println("Turning 90° LEFT");
  rotateMotor(-TURN_SPEED, TURN_SPEED);  // Right motor backward, left motor forward
  delay(TURN_90_DELAY);
  stopMotors();
  delay(100);
}

// Move forward slowly and return distance traveled in cm
long moveForwardSlow(int durationMs) {
  rotateMotor(SLOW_SPEED, SLOW_SPEED);
  delay(durationMs);
  stopMotors();
  
  // Approximate distance: assuming ~10cm per 500ms at SLOW_SPEED
  // Adjust this ratio based on your robot's actual speed
  return (durationMs * 10) / 500;
}

void lineFollow() {
  int leftIRValue = digitalRead(leftIR);
  int rightIRValue = digitalRead(rightIR);
  
  // Line following logic
  if (leftIRValue == LOW && rightIRValue == LOW) {
    // Both sensors on white - move forward
    rotateMotor(LINE_FOLLOW_SPEED, LINE_FOLLOW_SPEED);
  } else if (leftIRValue == HIGH && rightIRValue == LOW) {
    // Left sensor on black line - turn right
    rotateMotor(LINE_FOLLOW_SPEED, 80);
  } else if (leftIRValue == LOW && rightIRValue == HIGH) {
    // Right sensor on black line - turn left
    rotateMotor(80, LINE_FOLLOW_SPEED);
  } else {
    // Both sensors on black - on the line, move forward
    rotateMotor(LINE_FOLLOW_SPEED, LINE_FOLLOW_SPEED);
  }
}

// Main obstacle avoidance procedure with precise navigation
void preciseObstacleAvoidance() {
  Serial.println("\n=== OBSTACLE DETECTED - Starting Avoidance Procedure ===");
  
  // Step 1: Stop for 2 seconds
  Serial.println("Step 1: Stopping for 2 seconds");
  stopMotors();
  delay(2000);
  
  // Step 2: Turn 90° RIGHT
  Serial.println("Step 2: Turning 90° RIGHT");
  turn90Right();
  delay(500);
  
  // Step 3: Move forward while counting distance with RIGHT ultrasonic
  Serial.println("Step 3: Moving parallel to obstacle, tracking distance");
  distanceTraveled = 0;
  unsigned long moveStartTime = millis();
  unsigned long lastDistanceUpdate = millis();
  
  while (true) {
    // Read RIGHT ultrasonic sensor (now facing the obstacle)
    long rightDistance = getDistance(rightTrigPin, rightEchoPin);
    
    // Move forward slowly
    rotateMotor(SLOW_SPEED, SLOW_SPEED);
    
    // Update distance traveled every 100ms
    if (millis() - lastDistanceUpdate >= 100) {
      // Approximate: 120 speed ~ 12cm per 100ms (calibrate this!)
      distanceTraveled += 12;
      lastDistanceUpdate = millis();
      
      Serial.print("Distance traveled: ");
      Serial.print(distanceTraveled);
      Serial.print(" cm, Right sensor: ");
      Serial.println(rightDistance);
    }
    
    // Check if obstacle ended (right sensor stops reading or reads far)
    if (rightDistance > 30 || rightDistance == 0) {
      Serial.println("Obstacle ended!");
      stopMotors();
      delay(100);
      break;
    }
    
    delay(SENSOR_READ_DELAY);
  }
  
  Serial.print("Total distance traveled parallel: ");
  Serial.print(distanceTraveled);
  Serial.println(" cm");
  
  // Step 4: Stop for 2 seconds
  Serial.println("Step 4: Stopping for 2 seconds");
  delay(2000);
  
  // Step 5: Turn -90° (LEFT) to face forward
  Serial.println("Step 5: Turning 90° LEFT to face forward");
  turn90Left();
  delay(500);
  
  // Step 6: Move forward the same distance
  Serial.println("Step 6: Moving forward same distance");
  unsigned long moveDuration = (distanceTraveled * 500) / 10;  // Convert cm to ms
  Serial.print("Moving for ");
  Serial.print(moveDuration);
  Serial.println(" ms");
  
  rotateMotor(SLOW_SPEED, SLOW_SPEED);
  delay(moveDuration);
  stopMotors();
  delay(500);
  
  // Step 7: Stop for 2 seconds
  Serial.println("Step 7: Stopping for 2 seconds");
  delay(2000);
  
  // Step 8: Turn -90° (LEFT) again to face the line
  Serial.println("Step 8: Turning 90° LEFT to face the line");
  turn90Left();
  delay(500);
  
  // Step 9: Move back the same distance to reach the line
  Serial.println("Step 9: Returning to line - moving same distance");
  rotateMotor(SLOW_SPEED, SLOW_SPEED);
  delay(moveDuration);
  stopMotors();
  delay(500);
  
  // Step 10: Check if both IR sensors detect black line
  Serial.println("Step 10: Checking if back on line");
  int leftIRValue = digitalRead(leftIR);
  int rightIRValue = digitalRead(rightIR);
  
  if (leftIRValue == HIGH && rightIRValue == HIGH) {
    Serial.println("SUCCESS! Both sensors on black line");
    delay(2000);
    
    // Step 11: Turn 90° RIGHT to face forward on the line
    Serial.println("Step 11: Turning 90° RIGHT to face forward");
    turn90Right();
    delay(500);
    
    Serial.println("=== Obstacle Avoidance Complete - Resuming Line Following ===\n");
    currentState = LINE_FOLLOWING;
  } else {
    Serial.println("WARNING: Not perfectly aligned on line, but continuing");
    turn90Right();
    delay(500);
    currentState = LINE_FOLLOWING;
  }
}

void obstacleAvoid() {
  long frontDistance = getDistance(frontTrigPin, frontEchoPin);
  
  // Fast sensor reading
  if (millis() - lastSensorRead < SENSOR_READ_DELAY) {
    return;
  }
  lastSensorRead = millis();
  
  if (frontDistance < OBSTACLE_DISTANCE && frontDistance > 0) {
    // Obstacle detected within 5cm
    Serial.print("OBSTACLE! Front Distance: ");
    Serial.println(frontDistance);
    
    currentState = OBSTACLE_DETECTED;
    preciseObstacleAvoidance();
  } else {
    // No obstacle - continue line following
    if (currentState == LINE_FOLLOWING) {
      lineFollow();
    }
  }
}

void setup() {
  Serial.begin(115200);
  setupPins();
  
  Serial.println("\n========================================");
  Serial.println("Robot Started - Precise Obstacle Avoidance");
  Serial.println("========================================");
  Serial.println("Motor Setup: ENA/IN1/IN2 for Right, ENB/IN3/IN4 for Left");
  Serial.println("Obstacle Detection: 5cm");
  Serial.println("Speed: SLOW for precision");
  Serial.println("========================================\n");
  
  delay(2000);  // Give time to place robot on line
  currentState = LINE_FOLLOWING;
}

void loop() {
  obstacleAvoid();
  delay(20);  // Fast loop for quick sensor response
}