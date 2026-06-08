#include <Wire.h>
#include <MPU9250.h>
#include <MadgwickAHRS.h>  // Madgwick filter for sensor fusion

// ============================================== PIN DEFINITIONS /////////////////

// IR Sensor Pins (Digital)
const int IR1 = 36;  // Far Left
const int IR2 = 33;  // Left
const int IR3 = 35;  // Middle
const int IR4 = 32;  // Right
const int IR5 = 39;  // Far Right

// L298N Motor Driver Pins
const int LEFT_IN1 = 15;
const int LEFT_IN2 = 2;
const int LEFT_EN = 17;

const int RIGHT_IN1 = 18;
const int RIGHT_IN2 = 19;
const int RIGHT_EN = 23;

// MPU9250 I2C Pins (ESP32 default)
const int SDA_PIN = 21;
const int SCL_PIN = 22;

// ============================================== SETTINGS /////////////////

const int MOTOR_SPEED = 180;      // Normal speed for turning
const int TURN_SPEED = 120;       // Speed during 180° spin (OPTIMIZED)

const float TARGET_ANGLE = 160.0;    // Target 160° (compensates for ~20° overshoot)
const float ANGLE_TOLERANCE = 10.0;  // ±10° tolerance for turn
const int STOP_DURATION = 5000;     // Stop for 5 seconds after turn (ms)

// ============================================== GLOBAL OBJECTS /////////////////

MPU9250 mpu;
Madgwick filter;  // Madgwick AHRS filter for sensor fusion

// Sensor fusion settings
const float SAMPLE_FREQ = 100.0;  // 100Hz update rate (10ms loop)
const float BETA = 0.1;           // Madgwick filter gain (balanced)

// State tracking
bool turnInProgress = false;
bool ignoreTurnSensors = false;
unsigned long ignoreEndTime = 0;

float initialRoll = 0.0;  // Roll angle for 180° turn
float currentRoll = 0.0;
float targetRoll = 0.0;

// ============================================== SETUP /////////////////

void setup() {
    Serial.begin(115200);
    delay(500);
    
    Serial.println("\n========================================");
    Serial.println("   MODULE 2: 180° Turn (OPTIMIZED)");
    Serial.println("========================================\n");
    
    // Initialize IR sensors
    pinMode(IR1, INPUT);
    pinMode(IR2, INPUT);
    pinMode(IR3, INPUT);
    pinMode(IR4, INPUT);
    pinMode(IR5, INPUT);
    
    // Initialize motor pins
    pinMode(LEFT_IN1, OUTPUT);
    pinMode(LEFT_IN2, OUTPUT);
    pinMode(LEFT_EN, OUTPUT);
    pinMode(RIGHT_IN1, OUTPUT);
    pinMode(RIGHT_IN2, OUTPUT);
    pinMode(RIGHT_EN, OUTPUT);
    
    // Initialize I2C for MPU9250
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(400000);  // 400kHz I2C speed
    
    Serial.println("Initializing MPU9250...");
    
    // Initialize MPU9250
    if (!mpu.setup(0x68)) {  // MPU9250 I2C address
        Serial.println("❌ MPU9250 initialization failed!");
        Serial.println("Check I2C connections:");
        Serial.println("  SDA → GPIO 21");
        Serial.println("  SCL → GPIO 22");
        Serial.println("  VCC → 3.3V");
        Serial.println("  GND → GND");
        while (1) {
            delay(1000);
        }
    }
    
    Serial.println("✅ MPU9250 initialized successfully");
    
    // Calibrate gyroscope and magnetometer
    Serial.println("\n🔧 Calibrating sensors (5 seconds)...");
    Serial.println("   Keep robot STILL!");
    delay(2000);
    
    mpu.calibrateAccelGyro();
    mpu.calibrateMag();  // Calibrate magnetometer for accurate heading
    mpu.calibrateMag();  // Calibrate magnetometer for accurate heading
    
    Serial.println("✅ Calibration complete\n");
    
    // Initialize Madgwick filter
    filter.begin(SAMPLE_FREQ);
    Serial.print("🔬 Madgwick filter: ");
    Serial.print(SAMPLE_FREQ);
    Serial.println(" Hz\n");
    
    Serial.println("Hardware Configuration:");
    Serial.println("  IR1 (Far Left):  GPIO 36");
    Serial.println("  IR2 (Left):      GPIO 33");
    Serial.println("  IR3 (Middle):    GPIO 35");
    Serial.println("  IR4 (Right):     GPIO 32");
    Serial.println("  IR5 (Far Right): GPIO 39");
    Serial.println("  MPU9250 I2C:     SDA=21, SCL=22\n");
    
    Serial.println("🎯 180° Turn with Madgwick Fusion:");
    Serial.println("  - Full sensor calibration (gyro + mag)");
    Serial.println("  - Sample rate: 100Hz");
    Serial.println("  - Turn speed: 120 PWM");
    Serial.println("  - BETA: 0.1 (balanced)\n");
    Serial.println("  Trigger: IR1 AND IR5 detect black");
    Serial.println("  Action:  Spin 180° → Stop 5s\n");
    
    Serial.println("🚗 Starting operation...\n");
    delay(1000);
}

// ============================================== MAIN LOOP /////////////////

void loop() {
    // OPTIMIZED: Always update sensors (no conditional check)
    mpu.update();
    updateMadgwickFilter();
    currentRoll = getMadgwickRoll();
    
    // Read all IR sensors
    bool ir1 = digitalRead(IR1) == HIGH;  // HIGH = black line
    bool ir2 = digitalRead(IR2) == HIGH;
    bool ir3 = digitalRead(IR3) == HIGH;
    bool ir4 = digitalRead(IR4) == HIGH;
    bool ir5 = digitalRead(IR5) == HIGH;
    
    // Check if we should ignore IR1/IR5 (during forward phase)
    if (ignoreTurnSensors && millis() >= ignoreEndTime) {
        ignoreTurnSensors = false;
    }
    
    // TRIGGER: Both IR1 and IR5 detect black line
    if (!turnInProgress && !ignoreTurnSensors && ir1 && ir5) {
        Serial.println("\n🎯 TRIGGER DETECTED!");
        Serial.println("   IR1 AND IR5 both on black line");
        Serial.println("   Initiating 180° turn...\n");
        
        perform180Turn();
    }
    
    delay(10);  // 100Hz loop
}

// ============================================== 180° TURN FUNCTION /////////////////

void perform180Turn() {
    turnInProgress = true;
    
    // Step 1: Record current roll from Madgwick filter (fused sensors)
    mpu.update();
    updateMadgwickFilter();
    initialRoll = getMadgwickRoll();
    targetRoll = normalizeAngle(initialRoll + TARGET_ANGLE);
    
    Serial.print("📐 Initial: ");
    Serial.print(initialRoll, 1);
    Serial.print("° | Target: ");
    Serial.print(targetRoll, 1);
    Serial.println("°\n");
    
    // Step 2: Spin in place until target angle achieved
    Serial.print("🔄 Spinning ");
    Serial.print(TARGET_ANGLE, 0);
    Serial.println("° (Madgwick fusion)...");
    
    unsigned long spinStartTime = millis();
    
    while (true) {
        // OPTIMIZED: Always update sensors (no conditional)
        mpu.update();
        updateMadgwickFilter();
        currentRoll = getMadgwickRoll();
        
        // Calculate how much we've rotated
        float rotated = getRotationAmount(initialRoll, currentRoll);
        
        // Check if we've reached target angle (±tolerance)
        if (rotated >= (TARGET_ANGLE - ANGLE_TOLERANCE)) {
            Serial.print("✅ ");
            Serial.print(TARGET_ANGLE, 0);
            Serial.println("° complete!");
            Serial.print("   Final: ");
            Serial.print(currentRoll, 1);
            Serial.print("° (Error: ");
            Serial.print(abs(rotated - TARGET_ANGLE), 1);
            Serial.println("°)");
            break;
        }
        
        // OPTIMIZED: Smoother deceleration curve (start slowing at 40° before target)
        float remaining = TARGET_ANGLE - rotated;
        int turnSpeed = TURN_SPEED;
        
        if (remaining < 40.0) {
            // Slow down in final 40° for precision
            turnSpeed = map(remaining, 0, 40, 70, TURN_SPEED);
            turnSpeed = constrain(turnSpeed, 70, TURN_SPEED);
        }
        
        // OPTIMIZED: Shorter timeout (10 seconds instead of 15)
        if (millis() - spinStartTime > 10000) {
            Serial.println("⚠️  Timeout at ");
            Serial.print(rotated, 1);
            Serial.println("°");
            break;
        }
        
        // Perform pivot turn with dynamic speed
        pivotTurnWithSpeed(turnSpeed);
        
        delay(10);  // 100Hz loop for stable fusion
    }
    
    // Step 3: Stop motors and wait 5 seconds
    stopMotors();
    
    Serial.println("🛑 Stopping for 5 seconds...");
    delay(STOP_DURATION);
    
    // Step 4: Resume normal operation
    turnInProgress = false;
    ignoreTurnSensors = false;
    
    Serial.println("✅ Turn complete!\n");
}

// ============================================== MOTOR CONTROL /////////////////

void moveForward() {
    analogWrite(LEFT_EN, MOTOR_SPEED);
    analogWrite(RIGHT_EN, MOTOR_SPEED);
    
    // Forward: LEFT(LOW-HIGH), RIGHT(HIGH-LOW)
    digitalWrite(LEFT_IN1, LOW);
    digitalWrite(LEFT_IN2, HIGH);
    digitalWrite(RIGHT_IN1, HIGH);
    digitalWrite(RIGHT_IN2, LOW);
}

void pivotTurn() {
    pivotTurnWithSpeed(TURN_SPEED);
}

void pivotTurnWithSpeed(int speed) {
    // Pivot turn: Left FORWARD, Right BACKWARD (clockwise spin)
    analogWrite(LEFT_EN, speed);
    analogWrite(RIGHT_EN, speed);
    
    // Left motor FORWARD: LOW-HIGH
    digitalWrite(LEFT_IN1, LOW);
    digitalWrite(LEFT_IN2, HIGH);
    
    // Right motor BACKWARD: LOW-HIGH (opposite of forward HIGH-LOW)
    digitalWrite(RIGHT_IN1, LOW);
    digitalWrite(RIGHT_IN2, HIGH);
}

void stopMotors() {
    analogWrite(LEFT_EN, 0);
    analogWrite(RIGHT_EN, 0);
    digitalWrite(LEFT_IN1, LOW);
    digitalWrite(LEFT_IN2, LOW);
    digitalWrite(RIGHT_IN1, LOW);
    digitalWrite(RIGHT_IN2, LOW);
}

// ============================================== HELPER FUNCTIONS /////////////////

// Update Madgwick filter with sensor data (9-axis fusion)
void updateMadgwickFilter() {
    // Get raw sensor data (in correct units)
    float gx = mpu.getGyroX();  // deg/s
    float gy = mpu.getGyroY();
    float gz = mpu.getGyroZ();
    
    float ax = mpu.getAccX();   // g
    float ay = mpu.getAccY();
    float az = mpu.getAccZ();
    
    float mx = mpu.getMagX();   // uT (microTesla)
    float my = mpu.getMagY();
    float mz = mpu.getMagZ();
    
    // Update Madgwick filter (gyro, accel, mag)
    // Note: Madgwick expects gyro in radians/sec, so convert
    filter.update(gx, gy, gz, ax, ay, az, mx, my, mz);
}

// Get roll angle from Madgwick filter (0-360°)
float getMadgwickRoll() {
    // Get roll angle from Madgwick filter
    float roll = filter.getRoll();  // Returns roll in degrees
    
    // Normalize to 0-360°
    while (roll < 0) roll += 360.0;
    while (roll >= 360.0) roll -= 360.0;
    
    return roll;
}

// Normalize angle to -180 to +180 range
float normalizeAngle(float angle) {
    while (angle > 360.0) angle -= 360.0;
    while (angle < 0.0) angle += 360.0;
    return angle;
}

// Calculate rotation amount from start to current angle
float getRotationAmount(float startAngle, float currentAngle) {
    float diff = currentAngle - startAngle;
    
    // Handle wraparound (e.g., 350° to 10° = 20° rotation, not -340°)
    if (diff > 180.0) diff -= 360.0;
    if (diff < -180.0) diff += 360.0;
    
    return abs(diff);
}
