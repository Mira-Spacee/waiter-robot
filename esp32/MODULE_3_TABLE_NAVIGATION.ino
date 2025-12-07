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
    
    Serial.println("📋 Navigation Logic:");
    Serial.println("  SINGLE ORDER:");
    Serial.println("    Tables 1-5 (RIGHT) → FORWARD");
    Serial.println("    Tables 6-10 (LEFT) → BACKWARD");
    Serial.println("  MULTIPLE ORDERS:");
    Serial.println("    Step 1: Count orders per side (RIGHT=1-5, LEFT=6-10)");
    Serial.println("    Step 2: Start with side that has MORE orders (tie=LEFT)");
    Serial.println("    Step 3: Serve all tables in that range (opportunistic)");
    Serial.println("    Step 4: Use 5F/4B rule to reach remaining tables");
    Serial.println("    Step 5: Return via opposite direction of last table\n");
    
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
    Serial.println(movingForward ? "FORWARD (RIGHT side)" : "BACKWARD (LEFT side)");
    Serial.println("   ⚡ Opportunistic serving enabled!");
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
        // ========== STEP 5: Return to staff based on last table position ==========
        Serial.println("\n🎉 All orders served!");
        
        // If last table is in RIGHT range (1-5) → go BACKWARD to staff
        // If last table is in LEFT range (6-10) → go FORWARD to staff
        if (currentTable >= 1 && currentTable <= 5) {
            movingForward = false;  // BACKWARD to staff
            Serial.print("🔙 Last table T");
            Serial.print(currentTable);
            Serial.println(" is RIGHT (1-5) → Returning BACKWARD");
            cumulativeCounter = 10 + currentTable;
        } else {
            movingForward = true;   // FORWARD to staff
            Serial.print("🔙 Last table T");
            Serial.print(currentTable);
            Serial.println(" is LEFT (6-10) → Returning FORWARD");
        }
        
        targetTable = 0;  // Target is staff station
        currentState = RETURNING;
        return;
    }
    
    // ========== STEP 4: More orders remaining - use 5F/4B rule ==========
    if (orderCount > 0) {
        // Use decideNextDirection() which applies 5 forward / 4 backward rule
        RangeDecision decision = decideNextDirection();
        
        // Check if we need to switch direction
        bool wasForward = movingForward;
        movingForward = decision.isForward;
        targetTable = decision.targetTable;
        
        // Initialize cumulative counter if switching to backward
        if (!movingForward && wasForward) {
            cumulativeCounter = 10 + currentTable;
            Serial.println("   🔄 Switching to BACKWARD");
        } else if (movingForward && !wasForward) {
            // Switching from backward to forward - reset counter
            cumulativeCounter = currentTable;
            Serial.println("   🔄 Switching to FORWARD");
        }
        
        Serial.print("\n🎯 Next target: Table ");
        Serial.println(targetTable);
        Serial.print("   Direction: ");
        Serial.println(movingForward ? "FORWARD" : "BACKWARD");
        
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
    Serial.println("⚡ Directional Grouping + 5F/4B algorithm!");
}

// ============================================== DIRECTION DECISION ALGORITHM /////////////////

struct RangeDecision {
    bool isForward;     // true = forward (RIGHT side), false = backward (LEFT side)
    int targetTable;    // Farthest table in the chosen direction's range
};

RangeDecision decideDirection() {
    unsigned long startMicros = micros();
    
    RangeDecision decision;
    
    // Get list of unserved tables
    int unservedTables[10];
    int unservedCount = 0;
    
    for (int i = 0; i < orderCount; i++) {
        if (!orderServed[orderList[i]]) {
            unservedTables[unservedCount++] = orderList[i];
        }
    }
    
    if (unservedCount == 0) {
        decision.isForward = true;
        decision.targetTable = 0;
        return decision;
    }
    
    // ========== SINGLE ORDER: Tables 1-5 FORWARD, Tables 6-10 BACKWARD ==========
    if (unservedCount == 1) {
        int table = unservedTables[0];
        
        if (table >= 1 && table <= 5) {
            decision.isForward = true;
            decision.targetTable = table;
            Serial.println("📍 Single order: Table 1-5 → FORWARD (RIGHT)");
        } else {
            // Tables 6-10
            decision.isForward = false;
            decision.targetTable = table;
            Serial.println("📍 Single order: Table 6-10 → BACKWARD (LEFT)");
        }
    }
    // ========== MULTIPLE ORDERS: Directional Grouping + Bidirectional Sweep ==========
    else {
        // Step 2: Count orders in each side
        // RIGHT side (forward): Tables 1-5
        // LEFT side (backward): Tables 6-10
        
        int rightCount = 0;  // Tables 1-5
        int leftCount = 0;   // Tables 6-10
        int rightFarthest = 0;   // Highest table in RIGHT range (1-5)
        int leftFarthest = 0;    // Lowest table in LEFT range (6-10), which is farthest backward
        
        for (int i = 0; i < unservedCount; i++) {
            int table = unservedTables[i];
            
            if (table >= 1 && table <= 5) {
                // RIGHT side (forward)
                rightCount++;
                if (table > rightFarthest) rightFarthest = table;
            } else if (table >= 6 && table <= 10) {
                // LEFT side (backward)
                leftCount++;
                if (leftFarthest == 0 || table < leftFarthest) leftFarthest = table;
            }
        }
        
        Serial.print("📊 RIGHT (1-5): ");
        Serial.print(rightCount);
        Serial.print(" orders, farthest=");
        Serial.println(rightFarthest);
        Serial.print("📊 LEFT (6-10): ");
        Serial.print(leftCount);
        Serial.print(" orders, farthest=");
        Serial.println(leftFarthest);
        
        // Step 2 continued: Choose starting direction
        if (rightCount > leftCount) {
            // More orders on RIGHT → start FORWARD
            decision.isForward = true;
            decision.targetTable = rightFarthest;
            Serial.println("📍 Multi-order: RIGHT has more → FORWARD first");
        } else if (leftCount > rightCount) {
            // More orders on LEFT → start BACKWARD
            decision.isForward = false;
            decision.targetTable = leftFarthest;
            Serial.println("📍 Multi-order: LEFT has more → BACKWARD first");
        } else {
            // EQUAL: Default to LEFT (backward)
            decision.isForward = false;
            decision.targetTable = leftFarthest > 0 ? leftFarthest : rightFarthest;
            Serial.println("📍 Multi-order: EQUAL → Default BACKWARD (LEFT)");
            
            // If no LEFT orders, go RIGHT
            if (leftFarthest == 0) {
                decision.isForward = true;
                decision.targetTable = rightFarthest;
            }
        }
    }
    
    unsigned long calcTime = micros() - startMicros;
    Serial.print("⚡ Calculation time: ");
    Serial.print(calcTime);
    Serial.println(" μs");
    
    return decision;
}

// Step 4: After serving one side, calculate next direction using 5F/4B rule
RangeDecision decideNextDirection() {
    unsigned long startMicros = micros();
    
    RangeDecision decision;
    
    // Get remaining unserved tables
    int unservedTables[10];
    int unservedCount = 0;
    
    for (int i = 0; i < orderCount; i++) {
        if (!orderServed[orderList[i]]) {
            unservedTables[unservedCount++] = orderList[i];
        }
    }
    
    if (unservedCount == 0) {
        decision.isForward = true;
        decision.targetTable = 0;
        return decision;
    }
    
    // If only 1 remaining, use single order logic
    if (unservedCount == 1) {
        int table = unservedTables[0];
        if (table >= 1 && table <= 5) {
            decision.isForward = true;
            decision.targetTable = table;
        } else {
            decision.isForward = false;
            decision.targetTable = table;
        }
        return decision;
    }
    
    // Step 4: Use 5 forward / 4 backward rule from current position
    // Find which table is reachable with fewer steps
    
    int bestForwardTable = -1;
    int bestForwardSteps = 999;
    int bestBackwardTable = -1;
    int bestBackwardSteps = 999;
    
    for (int i = 0; i < unservedCount; i++) {
        int table = unservedTables[i];
        
        // Calculate FORWARD steps (currentTable → table going 1→2→3...→10→1...)
        int forwardSteps;
        if (table > currentTable) {
            forwardSteps = table - currentTable;
        } else {
            forwardSteps = (10 - currentTable) + table;  // Wrap around
        }
        
        // Calculate BACKWARD steps (currentTable → table going 10→9→8...→1→10...)
        int backwardSteps;
        if (table < currentTable) {
            backwardSteps = currentTable - table;
        } else {
            backwardSteps = currentTable + (10 - table);  // Wrap around
        }
        
        // Check if reachable within limits (5 forward, 4 backward)
        if (forwardSteps <= 5 && forwardSteps < bestForwardSteps) {
            bestForwardSteps = forwardSteps;
            bestForwardTable = table;
        }
        if (backwardSteps <= 4 && backwardSteps < bestBackwardSteps) {
            bestBackwardSteps = backwardSteps;
            bestBackwardTable = table;
        }
    }
    
    Serial.print("📊 From T");
    Serial.print(currentTable);
    Serial.print(": Forward best=T");
    Serial.print(bestForwardTable);
    Serial.print("(");
    Serial.print(bestForwardSteps);
    Serial.print(" steps), Backward best=T");
    Serial.print(bestBackwardTable);
    Serial.print("(");
    Serial.print(bestBackwardSteps);
    Serial.println(" steps)");
    
    // Choose direction with fewer steps
    if (bestBackwardTable != -1 && (bestForwardTable == -1 || bestBackwardSteps <= bestForwardSteps)) {
        decision.isForward = false;
        decision.targetTable = bestBackwardTable;
        Serial.print("📍 Next: BACKWARD to T");
        Serial.print(bestBackwardTable);
        Serial.print(" (");
        Serial.print(bestBackwardSteps);
        Serial.println(" steps)");
    } else if (bestForwardTable != -1) {
        decision.isForward = true;
        decision.targetTable = bestForwardTable;
        Serial.print("📍 Next: FORWARD to T");
        Serial.print(bestForwardTable);
        Serial.print(" (");
        Serial.print(bestForwardSteps);
        Serial.println(" steps)");
    } else {
        // Fallback: go to closest table regardless of limit
        int closest = unservedTables[0];
        decision.isForward = (closest >= 1 && closest <= 5);
        decision.targetTable = closest;
        Serial.println("📍 Next: Fallback to closest");
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
