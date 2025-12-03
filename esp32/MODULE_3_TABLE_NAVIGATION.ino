/*
 * MODULE 3: Table Navigation System
 * 
 * Restaurant Layout:
 * - Tables 1-10 arranged along a path
 * - Staff station at start (both IR1 and IR5 detect black)
 * - Black squares at each table position (detected by IR5)
 * 
 * Navigation Logic:
 * 1. Going FORWARD (Staff → Table 10): Count UP when IR5 detects black
 * 2. Going BACKWARD (Table 10 → Staff): Count DOWN when IR5 detects black
 * 3. Order serving: Navigate to tables in order list, stop 5s at each
 * 4. Return to staff station when all orders complete
 */

#include <Wire.h>
#include <MPU9250.h>

// ============================================== PIN DEFINITIONS /////////////////

// IR Sensor Pins (Digital)
const int IR1 = 36;  // Far Left
const int IR2 = 33;  // Left
const int IR3 = 35;  // Middle (main line follower)
const int IR4 = 32;  // Right
const int IR5 = 39;  // Far Right (table counter)

// L298N Motor Driver Pins
const int LEFT_IN1 = 15;
const int LEFT_IN2 = 2;
const int LEFT_EN = 17;

const int RIGHT_IN1 = 18;
const int RIGHT_IN2 = 19;
const int RIGHT_EN = 23;

// MPU9250 I2C Pins
const int SDA_PIN = 21;
const int SCL_PIN = 22;

// ============================================== SETTINGS /////////////////

const int MOTOR_SPEED = 80;           // Forward speed
const int TURN_SPEED = 120;           // Turn speed
const int TABLE_STOP_DURATION = 5000; // Stop at table for 5 seconds

// PID Constants for line following
const float KP = 45.0;
const float KI = 0.0;
const float KD = 30.0;

// ============================================== GLOBAL VARIABLES /////////////////

// Navigation state
enum RobotState {
    IDLE,              // Waiting at staff station
    NAVIGATING,        // Moving to table
    AT_TABLE,          // Stopped at table
    RETURNING         // Returning to staff
};

RobotState currentState = IDLE;

// Table tracking
int currentTable = 0;        // Current table position (0 = staff station)
int cumulativeCounter = 0;   // Cumulative IR5 count (always increments)
bool movingForward = true;   // Direction: true = staff→tables, false = tables→staff
bool lastIR5State = false;   // Previous IR5 reading for edge detection

// Order management
const int MAX_ORDERS = 10;
int orderList[MAX_ORDERS];      // Original order list
bool orderServed[11];           // Track which tables have been served (0=staff, 1-10=tables)
int orderCount = 0;             // Number of orders in list
int servedCount = 0;            // Number of orders served
int targetTable = 0;            // Current target table (farthest unserved)

// PID Control
float lastError = 0;
float integral = 0;

// Timing
unsigned long tableStopStartTime = 0;

// MPU9250 object
MPU9250 mpu;

// ============================================== SETUP /////////////////

void setup() {
    Serial.begin(115200);
    delay(500);
    
    Serial.println("\n========================================");
    Serial.println("   MODULE 3: Table Navigation System");
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
    
    // Initialize I2C for MPU9250 (currently unused)
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(400000);
    
    Serial.println("🗺️  Restaurant Map:");
    Serial.println("  Tables: 1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → 9 → 10");
    Serial.println("  Staff Station: Start/End point (IR1 only)");
    Serial.println("  IR5: Counts tables (black squares)\n");
    
    Serial.println("📋 Navigation Logic (Reachability + Opportunistic):");
    Serial.println("  1. Check reachable tables from current position:");
    Serial.println("     FORWARD: +5 tables (current+1 to current+5)");
    Serial.println("     BACKWARD: +4 tables with wrap (1→10→9...)");
    Serial.println("  2. Choose direction with MORE reachable tables");
    Serial.println("  3. Go to farthest reachable, serve opportunistically");
    Serial.println("  4. IR5 counting: Cumulative (always ++), modulo to get table#\n");
    
    Serial.println("🚗 Robot ready at Staff Station!");
    Serial.println("   Waiting for orders...\n");
    
    delay(2000);
}

// ============================================== MAIN LOOP /////////////////

void loop() {
    // Read IR sensors
    bool ir1 = digitalRead(IR1) == HIGH;  // HIGH = black
    bool ir2 = digitalRead(IR2) == HIGH;
    bool ir3 = digitalRead(IR3) == HIGH;
    bool ir4 = digitalRead(IR4) == HIGH;
    bool ir5 = digitalRead(IR5) == HIGH;
    
    // Detect IR5 edge (black square passing)
    // Count tables during NAVIGATING and RETURNING states
    if (ir5 && !lastIR5State && (currentState == NAVIGATING || currentState == RETURNING)) {
        // Rising edge: IR5 just detected black square
        onTableMarkerDetected();
    }
    lastIR5State = ir5;
    
    // State machine
    switch (currentState) {
        case IDLE:
            // At staff station, check if orders exist
            stopMotors();
            
            if (orderCount > 0 && servedCount == 0) {
                Serial.println("\n📦 New orders received!");
                printOrders();
                startNavigation();
            }
            break;
            
        case NAVIGATING:
            // Moving toward target table
            followLine();
            
            // Note: Table serving is handled in onTableMarkerDetected()
            // via checkAndServeTable() - opportunistic serving!
            
            break;
            
        case AT_TABLE:
            // Stopped at table
            stopMotors();
            
            if (millis() - tableStopStartTime >= TABLE_STOP_DURATION) {
                departFromTable();
            }
            break;
            
        case RETURNING:
            // Returning to staff (count tables until currentTable == 0)
            followLine();
            
            // Check if reached staff station (currentTable == 0)
            if (currentTable == 0) {
                returnToStaff();
            }
            break;
    }
    
    delay(10);  // 100Hz loop
}

// ============================================== NAVIGATION FUNCTIONS /////////////////

void startNavigation() {
    currentState = NAVIGATING;
    currentTable = 0;  // Reset at staff station
    servedCount = 0;
    
    // Initialize served status
    for (int i = 0; i < 11; i++) {
        orderServed[i] = false;
    }
    
    // Decide direction using range algorithm
    RangeDecision decision = decideDirection();
    
    movingForward = decision.isForward;
    targetTable = decision.targetTable;
    
    // Initialize cumulative counter based on direction
    if (!movingForward) {
        // Backward: m = 10 + currentTable
        cumulativeCounter = 10 + currentTable;
    }
    // Forward: cumulativeCounter already at currentTable from previous movement
    
    Serial.print("🎯 Target: Table ");
    Serial.println(targetTable);
    Serial.print("📍 Direction: ");
    Serial.println(movingForward ? "FORWARD (+5 reach)" : "BACKWARD (+4 reach, wrap)");
    Serial.println("   ⚡ Will serve ALL tables with orders along the way!");
}

void onTableMarkerDetected() {
    // IR5 detected a black square (table marker)
    
    if (movingForward) {
        // Forward: simple increment
        cumulativeCounter++;
        currentTable = cumulativeCounter;
        
        Serial.print("📊 Count: ");
        Serial.print(cumulativeCounter);
        Serial.print(" → Table ");
        Serial.print(currentTable);
        Serial.println(" (forward)");
    } else {
        // Backward: decrement from m (where m=10+x when backward started)
        cumulativeCounter--;
        
        // Convert: 13→T3, 12→T2, 11→T1, 10→T10, 9→T9, etc.
        if (cumulativeCounter > 10) {
            currentTable = cumulativeCounter - 10;  // 13→3, 12→2, 11→1
        } else {
            currentTable = cumulativeCounter;        // 10→10, 9→9, 8→8, etc.
        }
        
        Serial.print("📊 Count: ");
        Serial.print(cumulativeCounter);
        Serial.print(" → Table ");
        Serial.print(currentTable);
        Serial.println(" (backward)");
    }
    
    // Check if this table has an order (opportunistic serving)
    checkAndServeTable();
}

void checkAndServeTable() {
    // Check if current table has an unserved order
    bool hasOrder = false;
    
    for (int i = 0; i < orderCount; i++) {
        if (orderList[i] == currentTable && !orderServed[currentTable]) {
            hasOrder = true;
            break;
        }
    }
    
    if (hasOrder) {
        // This table has an order! Stop and serve
        Serial.println("   🎯 This table has an order!");
        arriveAtTable();
    }
}

void arriveAtTable() {
    currentState = AT_TABLE;
    stopMotors();
    tableStopStartTime = millis();
    
    Serial.print("\n✅ Arrived at Table ");
    Serial.println(currentTable);
    Serial.println("🍽️  Serving order... (5 seconds)");
}

void departFromTable() {
    Serial.print("✔️  Order served at Table ");
    Serial.println(currentTable);
    
    // Mark this table as served
    orderServed[currentTable] = true;
    servedCount++;
    
    // Remove this table from orderList
    for (int i = 0; i < orderCount; i++) {
        if (orderList[i] == currentTable) {
            // Shift remaining orders down
            for (int j = i; j < orderCount - 1; j++) {
                orderList[j] = orderList[j + 1];
            }
            orderCount--;  // Decrease order count
            break;
        }
    }
    
    Serial.print("   Progress: ");
    Serial.print(servedCount);
    Serial.print(" served, ");
    Serial.print(orderCount);
    Serial.println(" remaining");
    
    // Check if orderList is empty
    if (orderCount == 0) {
        // All orders served! Calculate optimal direction to return to staff
        Serial.println("\n🎉 All orders served!");
        
        // Calculate steps needed in each direction to reach Staff (position 0)
        int forwardSteps;
        int backwardSteps;
        
        // FORWARD to staff: currentTable→10, then 10→0 (wrap)
        // Total: (10 - currentTable) + 1
        forwardSteps = (10 - currentTable) + 1;
        
        // BACKWARD to staff: currentTable→currentTable-1→...→1→0
        // Total: currentTable
        backwardSteps = currentTable;
        
        // Choose direction with fewer steps
        if (forwardSteps <= backwardSteps) {
            movingForward = true;
            Serial.print("🔙 Returning FORWARD (needs ");
            Serial.print(forwardSteps);
            Serial.println(" steps)");
            // Don't reset cumulative counter - continue from current
        } else {
            movingForward = false;
            Serial.print("🔙 Returning BACKWARD (needs ");
            Serial.print(backwardSteps);
            Serial.println(" steps)");
            // Set cumulative counter for backward
            cumulativeCounter = 10 + currentTable;
        }
        
        targetTable = 0;  // Target is staff station
        currentState = RETURNING;
        return;
    }
    
    // More orders remaining
    if (orderCount > 0) {
        // Recalculate direction for remaining orders
        RangeDecision decision = decideDirection();
        
        movingForward = decision.isForward;
        targetTable = decision.targetTable;
        
        // Initialize cumulative counter if switching to backward
        if (!movingForward) {
            cumulativeCounter = 10 + currentTable;
        }
        
        Serial.print("\n🎯 Next target: Table ");
        Serial.println(targetTable);
        Serial.print("   Direction: ");
        Serial.println(movingForward ? "FORWARD (+5 reach)" : "BACKWARD (+4 reach, wrap)");
        
        currentState = NAVIGATING;
    }
}

void returnToStaff() {
    currentState = IDLE;
    stopMotors();
    currentTable = 0;
    
    Serial.println("\n🏁 Arrived at Staff Station!");
    Serial.println("   All orders complete.");
    Serial.println("   Waiting for new orders...\n");
    
    // Clear order list and served status
    orderCount = 0;
    servedCount = 0;
    targetTable = 0;
}

// ============================================== ORDER MANAGEMENT /////////////////

void addOrder(int tableNumber) {
    if (orderCount < MAX_ORDERS && tableNumber >= 1 && tableNumber <= 10) {
        orderList[orderCount] = tableNumber;
        orderCount++;
        
        Serial.print("➕ Order added: Table ");
        Serial.println(tableNumber);
    } else {
        Serial.print("❌ Invalid table number or order list full: ");
        Serial.println(tableNumber);
    }
}

void printOrders() {
    Serial.print("📋 Orders (");
    Serial.print(orderCount);
    Serial.print(" tables): ");
    
    for (int i = 0; i < orderCount; i++) {
        Serial.print(orderList[i]);
        if (i < orderCount - 1) Serial.print(", ");
    }
    Serial.println();
    Serial.println("⚡ Reachability + Opportunistic algorithm!");
}

// ============================================== RANGE-BASED F/B ALGORITHM /////////////////

struct RangeDecision {
    bool isForward;     // true = forward, false = backward
    int targetTable;    // Farthest reachable table in the chosen direction
};

RangeDecision decideDirection() {
    unsigned long startMicros = micros();
    
    // Check which unserved tables are reachable in each direction
    // FORWARD: Can reach up to currentTable+5
    // BACKWARD: Can reach up to 4 tables back (with wrapping)
    
    int forwardReachable[10];  // Tables reachable going forward
    int backwardReachable[10]; // Tables reachable going backward
    int forwardCount = 0;
    int backwardCount = 0;
    int forwardFarthest = -1;
    int backwardFarthest = -1;
    
    // Check FORWARD reachability (currentTable + 1 to currentTable + 5)
    for (int i = 0; i < orderCount; i++) {
        int table = orderList[i];
        if (!orderServed[table]) {
            // Forward: simple increment
            if (table > currentTable && table <= currentTable + 5) {
                forwardReachable[forwardCount++] = table;
                if (table > forwardFarthest) forwardFarthest = table;
            }
        }
    }
    
    // Check BACKWARD reachability (4 steps back with wrapping)
    for (int i = 0; i < orderCount; i++) {
        int table = orderList[i];
        if (!orderServed[table]) {
            // Calculate backward distance with wrapping
            int backwardSteps;
            if (table < currentTable) {
                backwardSteps = currentTable - table;
            } else {
                backwardSteps = currentTable + (10 - table);
            }
            
            if (backwardSteps > 0 && backwardSteps <= 4) {
                backwardReachable[backwardCount++] = table;
                // Farthest backward = the one with most backward steps
                if (backwardFarthest == -1 || backwardSteps > (backwardFarthest < currentTable ? currentTable - backwardFarthest : currentTable + (10 - backwardFarthest))) {
                    backwardFarthest = table;
                }
            }
        }
    }
    
    RangeDecision decision;
    
    // Choose direction with more reachable tables
    if (forwardCount > backwardCount) {
        decision.isForward = true;
        decision.targetTable = forwardFarthest;
    } else if (backwardCount > forwardCount) {
        decision.isForward = false;
        decision.targetTable = backwardFarthest;
    } else if (forwardCount > 0) {
        // Equal: prefer forward
        decision.isForward = true;
        decision.targetTable = forwardFarthest;
    } else {
        // No reachable tables (shouldn't happen)
        decision.isForward = true;
        decision.targetTable = currentTable + 1;
    }
    
    unsigned long calcTime = micros() - startMicros;
    Serial.print("⚡ Calculation time: ");
    Serial.print(calcTime);
    Serial.println(" μs");
    
    return decision;
}

// ============================================== LINE FOLLOWING /////////////////

void followLine() {
    // Read IR sensors for line following
    bool ir2 = digitalRead(IR2) == HIGH;
    bool ir3 = digitalRead(IR3) == HIGH;
    bool ir4 = digitalRead(IR4) == HIGH;
    
    // Calculate error based on IR2, IR3, IR4
    float error = 0;
    
    if (!ir2 && ir3 && !ir4) {
        error = 0;      // On line (centered)
    } else if (ir2 && ir3 && !ir4) {
        error = -1;     // Slightly left
    } else if (ir2 && !ir3 && !ir4) {
        error = -2;     // More left
    } else if (!ir2 && ir3 && ir4) {
        error = 1;      // Slightly right
    } else if (!ir2 && !ir3 && ir4) {
        error = 2;      // More right
    } else if (ir2 && ir3 && ir4) {
        error = 0;      // All on black (intersection)
    } else if (!ir2 && !ir3 && !ir4) {
        error = lastError;  // Lost line, use last error
    }
    
    // PID calculation
    float P = KP * error;
    integral += error;
    float I = KI * integral;
    float D = KD * (error - lastError);
    
    float correction = P + I + D;
    lastError = error;
    
    // Apply correction to motors
    int leftSpeed = MOTOR_SPEED - correction;
    int rightSpeed = MOTOR_SPEED + correction;
    
    // Constrain speeds
    leftSpeed = constrain(leftSpeed, 0, 255);
    rightSpeed = constrain(rightSpeed, 0, 255);
    
    // Move forward with correction
    if (movingForward) {
        moveForwardWithSpeed(leftSpeed, rightSpeed);
    } else {
        moveBackwardWithSpeed(leftSpeed, rightSpeed);
    }
}

// ============================================== MOTOR CONTROL /////////////////

void moveForwardWithSpeed(int leftSpeed, int rightSpeed) {
    analogWrite(LEFT_EN, leftSpeed);
    analogWrite(RIGHT_EN, rightSpeed);
    
    // Forward: LEFT(LOW-HIGH), RIGHT(HIGH-LOW)
    digitalWrite(LEFT_IN1, LOW);
    digitalWrite(LEFT_IN2, HIGH);
    digitalWrite(RIGHT_IN1, HIGH);
    digitalWrite(RIGHT_IN2, LOW);
}

void moveBackwardWithSpeed(int leftSpeed, int rightSpeed) {
    analogWrite(LEFT_EN, leftSpeed);
    analogWrite(RIGHT_EN, rightSpeed);
    
    // Backward: LEFT(HIGH-LOW), RIGHT(LOW-HIGH)
    digitalWrite(LEFT_IN1, HIGH);
    digitalWrite(LEFT_IN2, LOW);
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
