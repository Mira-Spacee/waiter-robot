/*
 * testTABLEm3: Table Navigation Testing Module
 * 
 * Same as MODULE 3 with manual order testing capability
 * 
 * TO ADD TEST ORDERS: Edit the setup() function below
 * Example:
 *   addOrder(1);  // Add table 1
 *   addOrder(3);  // Add table 3
 *   addOrder(9);  // Add table 9
 */

#include <Wire.h>
#include <MPU9250.h>

// ============================================== PIN DEFINITIONS /////////////////

// IR Sensor Pins (Digital)
const int IR1 = 36;  // Staff station detector (Far Left)
const int IR2 = 33;  // Left line follower
const int IR3 = 35;  // Middle line follower (main)
const int IR4 = 32;  // Right line follower
const int IR5 = 39;  // Table counter (Far Right)

// L298N Motor Driver Pins (ESP32 GPIO)
const int LEFT_IN1 = 15;   // Left motor direction 1
const int LEFT_IN2 = 2;    // Left motor direction 2
const int LEFT_EN = 17;    // Left motor speed (PWM)

const int RIGHT_IN1 = 18;  // Right motor direction 1
const int RIGHT_IN2 = 19;  // Right motor direction 2
const int RIGHT_EN = 23;   // Right motor speed (PWM)

// MPU9250 I2C Pins
const int SDA_PIN = 21;
const int SCL_PIN = 22;

// ============================================== SETTINGS /////////////////

// For 4cm line width with 1cm sensor spacing (digital sensors):
const int NORMAL_SPEED = 70;       // ✅ Good speed for smooth 4cm line
const float KP = 45;               // ✅ Lower for 4cm line (was 55)
const float KI = 0;                // Keep at 0
const float KD = 30;               // ✅ Lower for smoother response

const int SENSORS_COUNT = 3;  // 3 IR sensors for line following
const int SENSOR_PINS[SENSORS_COUNT] = {33, 35, 32};  // {Left, Middle, Right}
const int ERRORS[2] = {0, 1};  // Only 2 error levels for 3 sensors

const int MAX_SPEED = 255;
const int MIN_SPEED = 0;

const int TABLE_STOP_DURATION = 5000; // Stop at table for 5 seconds
const bool DEBUG = false;  // Set to true for line following debug


// ============================================== PID CLASS /////////////////
class PID {
    public:

    float KP;
    float KI;
    float KD;

    float last_proportional, integral;

    PID (float KP, float KI, float KD) {
        this->KP = KP;
        this->KD = KD;
        this->KI = KI;
    }

    int calculate_speed_difference(int proportional) {
        float derivative = proportional - last_proportional;
        integral = integral + proportional;
        last_proportional = proportional;

        return proportional * KP + integral * KI + derivative * KD;
    }
};


// ============================================== SENSORS CLASS /////////////////
class Sensors {
    public:

    int sensors_count;
    const int *sensor_pins;
    const int *errors;
    int previous_error;

    Sensors (int sensors_count, const int *sensor_pins, const int *errors) {
        this->sensor_pins = sensor_pins;
        this->sensors_count = sensors_count;
        this->errors = errors;
        this->previous_error = 0;
    }

    void initialize() {
        // Set all sensor pins as INPUT
        for(int i = 0; i < sensors_count; i++) {
            pinMode(sensor_pins[i], INPUT);
        }
    }

    int calculate_error() {
        int error;
        // 3 Sensors: Left(33), Middle(35), Right(32)
        // Binary pattern: LMR (Left-Middle-Right)
        switch (get_line_position()) {
            // Right sensor only (line is to the RIGHT - turn RIGHT)
            case 0b001: error = -errors[1]; break;  // Turn right
            
            // Middle + Right (slight right)
            case 0b011: error = -errors[1]; break;  // Turn right

            // Middle sensor only (centered - no error)
            case 0b010: error =  errors[0]; break;  // Go straight (0)

            // Middle + Left (slight left)
            case 0b110: error =  errors[1]; break;  // Turn left
            
            // Left sensor only (line is to the LEFT - turn LEFT)
            case 0b100: error =  errors[1]; break;  // Turn left
            
            // All sensors on black (junction or wide line)
            case 0b111: error =  errors[0]; break;  // Go straight (0)
            
            // All sensors off (lost line - keep previous direction)
            case 0b000: error =  previous_error; break;
            
            // Any other combination
            default:    error =  previous_error; break;
        }
        previous_error = error;
        return error;
    }

    bool on_line(int sensor_pin) {
        // DIGITAL SENSOR: Returns HIGH (1) on black, LOW (0) on white
        int sensor_value = digitalRead(sensor_pin);
        return sensor_value == HIGH;  // BLACK line = HIGH
    }

    int get_line_position() {
        int line_position = 0;
        // Build binary pattern: bit 0 = LEFT, bit 1 = MIDDLE, bit 2 = RIGHT
        for(int i = 0; i < sensors_count; i++) {
            if(on_line(sensor_pins[i])) {
                line_position += (1 << i);
            }
        }
        return line_position;
    }
    
};


// ============================================== MOTORS CLASS /////////////////
class Motors {
    public:

    int normal_speed;
    int min_speed;
    int max_speed;

    int left_in1;
    int left_in2;
    int left_en;
    
    int right_in1;
    int right_in2;
    int right_en;

    Motors (int normal_speed, int min_speed, int max_speed, int left_in1, int left_in2, int left_en, int right_in1, int right_in2, int right_en) {
        this->normal_speed = normal_speed;
        this->min_speed = min_speed;
        this->max_speed = max_speed;

        this->left_in1 = left_in1;
        this->left_in2 = left_in2;
        this->left_en = left_en;
        
        this->right_in1 = right_in1;
        this->right_in2 = right_in2;
        this->right_en = right_en;
    }

    void initialize() {
        pinMode(left_en, OUTPUT);
        pinMode(left_in1, OUTPUT);
        pinMode(left_in2, OUTPUT);

        pinMode(right_en, OUTPUT);
        pinMode(right_in1, OUTPUT);
        pinMode(right_in2, OUTPUT);
    }

    void set_motor(int in1, int in2, int en, int speed, bool is_left_motor) {
        int dir1, dir2;
        get_direction(speed, dir1, dir2, is_left_motor);

        digitalWrite(in1, dir1);
        digitalWrite(in2, dir2);
        analogWrite(en, normalize_speed(speed));
    }

    void get_direction(int speed, int &dir1, int &dir2, bool is_left_motor) {
        // Based on your working code:
        // Forward: LEFT(LOW-HIGH), RIGHT(HIGH-LOW)
        if(speed > 0) {
            // Forward direction
            if(is_left_motor) {
                dir1 = LOW;
                dir2 = HIGH;
            } else {
                dir1 = HIGH;
                dir2 = LOW;
            }
        } else {
            // Backward direction (reverse of forward)
            if(is_left_motor) {
                dir1 = HIGH;
                dir2 = LOW;
            } else {
                dir1 = LOW;
                dir2 = HIGH;
            }
        }
    }

    int normalize_speed(int speed) {
        return constrain(abs(speed), min_speed, max_speed);
    }

    void drive(int speed_difference, bool debug) {
        int left_speed = normal_speed + speed_difference;
        int right_speed = normal_speed - speed_difference;

        set_motor(left_in1, left_in2, left_en, left_speed, true);   // true = left motor
        set_motor(right_in1, right_in2, right_en, right_speed, false); // false = right motor

        if(debug) {
            Serial.print("left_speed: ");
            Serial.print(normalize_speed(left_speed));
            Serial.print(" - ");
            Serial.println(left_speed > 0 ? "F" : "B");

            Serial.print("right_speed: ");
            Serial.print(normalize_speed(right_speed));
            Serial.print(" - ");
            Serial.println(right_speed > 0 ? "F" : "B");
        }
    }
    
    void drive_backward(int speed_difference, bool debug) {
        // Backward movement: negate speeds AND reverse PID correction
        // When going backward, left/right steering is reversed
        int left_speed = -(normal_speed - speed_difference);   // Swap the correction
        int right_speed = -(normal_speed + speed_difference);  // Swap the correction

        set_motor(left_in1, left_in2, left_en, left_speed, true);   // true = left motor
        set_motor(right_in1, right_in2, right_en, right_speed, false); // false = right motor

        if(debug) {
            Serial.print("BACKWARD - left_speed: ");
            Serial.print(normalize_speed(left_speed));
            Serial.print(", right_speed: ");
            Serial.println(normalize_speed(right_speed));
        }
    }

    void stop() {
        analogWrite(left_en, 0);
        analogWrite(right_en, 0);
        digitalWrite(left_in1, LOW);
        digitalWrite(left_in2, LOW);
        digitalWrite(right_in1, LOW);
        digitalWrite(right_in2, LOW);
    }

};


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

// Timing
unsigned long tableStopStartTime = 0;

// MPU9250 object
MPU9250 mpu;

// PID, Sensors, Motors objects
Sensors sensors(SENSORS_COUNT, SENSOR_PINS, ERRORS);
Motors motors(
    NORMAL_SPEED, MIN_SPEED, MAX_SPEED,
    LEFT_IN1, LEFT_IN2, LEFT_EN,
    RIGHT_IN1, RIGHT_IN2, RIGHT_EN
);
PID pid(KP, KI, KD);

// ============================================== RANGE-BASED F/B ALGORITHM /////////////////

struct RangeDecision {
    bool isForward;     // true = forward, false = backward
    int targetTable;    // Farthest reachable table in the chosen direction
};

// Function declarations
RangeDecision decideDirection();

// ============================================== SETUP /////////////////

void setup() {
    Serial.begin(115200);
    delay(500);
    
    Serial.println("\n========================================");
    Serial.println("   testTABLEm3: Testing Module");
    Serial.println("========================================\n");
    
    // Initialize IR sensors
    pinMode(IR1, INPUT);   // Staff station
    pinMode(IR5, INPUT);   // Table counter
    sensors.initialize();  // Initialize IR2, IR3, IR4 for line following
    
    // Initialize motors
    motors.initialize();
    
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
    
    // ============================================
    // 🧪 TEST ORDERS - EDIT HERE TO ADD ORDERS
    // ============================================
    Serial.println("🧪 TESTING MODE - Add your test orders below:");
    Serial.println("   (Edit setup() function in testTABLEm3.ino)\n");
    
    // 👇 ADD YOUR TEST ORDERS HERE:
    addOrder(1);
    addOrder(3);
    addOrder(5);
    // addOrder(5);
    // addOrder(7);
    // ============================================
    
    Serial.println("\n🚗 Robot ready at Staff Station!");
    Serial.println("   Test orders loaded. Ready to start!\n");
    
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
            
            // Check if reached staff station via IR1 OR currentTable == 0
            if (ir1 || currentTable == 0) {
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
    
    Serial.print("🔔 IR5 triggered! State: ");
    Serial.print(currentState == NAVIGATING ? "NAVIGATING" : "RETURNING");
    Serial.print(", Direction: ");
    Serial.println(movingForward ? "FORWARD" : "BACKWARD");
    
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
        } else if (cumulativeCounter == 10) {
            currentTable = 10;  // T10
        } else if (cumulativeCounter > 0) {
            currentTable = cumulativeCounter;  // 9→9, 8→8, ..., 1→1
        } else {
            currentTable = 0;  // Reached staff
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
        
        Serial.print("   📐 From T");
        Serial.print(currentTable);
        Serial.println(" to Staff:");
        Serial.print("      Forward steps: ");
        Serial.println(forwardSteps);
        Serial.print("      Backward steps: ");
        Serial.println(backwardSteps);
        
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
            // Set cumulative counter for backward movement
            cumulativeCounter = 10 + currentTable;
            Serial.print("   Counter set to: ");
            Serial.println(cumulativeCounter);
            Serial.print("   movingForward flag: ");
            Serial.println(movingForward ? "TRUE (FORWARD)" : "FALSE (BACKWARD)");
        }
        
        targetTable = 0;  // Target is staff station
        currentState = RETURNING;
        Serial.print("   State changed to: RETURNING\n");
        Serial.print("   Direction: ");
        Serial.println(movingForward ? "FORWARD" : "BACKWARD");
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
        
        Serial.print("   ➕ Order added: Table ");
        Serial.println(tableNumber);
    } else {
        Serial.print("   ❌ Invalid table number or order list full: ");
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

// ============================================== RANGE-BASED F/B ALGORITHM (IMPLEMENTATION) /////////////////

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
    // Use MODULE 1's PID-based line following
    int error = sensors.calculate_error();
    int speed_difference = pid.calculate_speed_difference(error);
    
    // Use appropriate motor direction based on movingForward flag
    if (movingForward) {
        motors.drive(speed_difference, DEBUG);
    } else {
        motors.drive_backward(speed_difference, DEBUG);
    }
    
    if(DEBUG) {
        // Print sensor states
        Serial.print("Sensors [L M R]: [");
        Serial.print(sensors.on_line(SENSOR_PINS[0]) ? "■" : "□");
        Serial.print(" ");
        Serial.print(sensors.on_line(SENSOR_PINS[1]) ? "■" : "□");
        Serial.print(" ");
        Serial.print(sensors.on_line(SENSOR_PINS[2]) ? "■" : "□");
        Serial.println("]");
        
        Serial.print("error: ");
        Serial.println(error);
        Serial.print("speed_difference: ");
        Serial.println(speed_difference);
        Serial.println("--------------------------------------");
    }
}

// ============================================== MOTOR CONTROL /////////////////

void stopMotors() {
    motors.stop();
}
