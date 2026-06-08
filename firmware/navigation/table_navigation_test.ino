/*
 * testTABLEm3: Table Navigation Testing Module
 * 
 * Same as MODULE 3 with manual order testing capability
 * Uses MODULE 1B backward line following with BACK SENSORS
 * 
 * ALGORITHM:
 * - Single Order: Tables 1-5 → FORWARD, Tables 6-10 → BACKWARD
 * - Multiple Orders: Directional Grouping + 5F/4B rule
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

// IR Sensor Pins (Digital) - ACTIVE LOW (LOW = black line detected)
const int IR1 = 36;  // Staff station detector (Far Left) - DIRECTLY on line
const int IR2 = 33;  // Left line follower
const int IR3 = 35;  // Middle line follower (main)
const int IR4 = 32;  // Right line follower
const int IR5 = 39;  // Table counter (Far Right) - DIRECTLY on line

// BACK IR Sensors for backward movement (MODULE 1B)
const int BACK_IR_LEFT = 27;    // Back left sensor
const int BACK_IR_CENTER = 26;  // Back center sensor
const int BACK_IR_RIGHT = 25;   // Back right sensor

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

// Backward PID (lower to reduce zigzag)
const float KP_BACK = 45;          // ✅ Reduced from 45 for smoother backward
const float KD_BACK = 30;          // ✅ Reduced from 30

const int SENSORS_COUNT = 3;
const int SENSOR_PINS[SENSORS_COUNT] = {33, 35, 32};  // {Left, Middle, Right}
const int BACK_SENSOR_PINS[SENSORS_COUNT] = {27, 26, 25};  // {Left, Center, Right} for backward
const int ERRORS[2] = {0, 1};

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
        // ✅ MODULE1B: BACKWARD MOVEMENT - same formula as MODULE1B
        int left_speed = -(normal_speed + speed_difference);   // Negative = backward
        int right_speed = -(normal_speed - speed_difference);  // Negative = backward

        // ✅ Force backward direction directly (don't rely on get_direction)
        // Backward: LEFT(HIGH-LOW), RIGHT(LOW-HIGH)
        digitalWrite(left_in1, HIGH);
        digitalWrite(left_in2, LOW);
        analogWrite(left_en, normalize_speed(left_speed));
        
        digitalWrite(right_in1, LOW);
        digitalWrite(right_in2, HIGH);
        analogWrite(right_en, normalize_speed(right_speed));

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
bool initialSideChosen = false; // Track if initial side decision has been made
bool servingRightSide = true;   // Track which side we're currently serving

// Timing
unsigned long tableStopStartTime = 0;
unsigned long departureTime = 0;  // Time when robot departed from table (for IR5 ignore)

// MPU9250 object
MPU9250 mpu;

// PID, Sensors, Motors objects
Sensors sensors(SENSORS_COUNT, SENSOR_PINS, ERRORS);
Sensors backSensors(SENSORS_COUNT, BACK_SENSOR_PINS, ERRORS);  // MODULE 1B: Back sensors
Motors motors(
    NORMAL_SPEED, MIN_SPEED, MAX_SPEED,
    LEFT_IN1, LEFT_IN2, LEFT_EN,
    RIGHT_IN1, RIGHT_IN2, RIGHT_EN
);
PID pid(KP, KI, KD);
PID backPid(KP_BACK, KI, KD_BACK);  // MODULE 1B: Separate PID for backward with lower gains

// ============================================== DIRECTION DECISION /////////////////

struct RangeDecision {
    bool isForward;
    int targetTable;
};

RangeDecision decideDirection();
RangeDecision decideNextDirection();  // 5F/4B rule for step 4

// ============================================== SETUP /////////////////

void setup() {
    Serial.begin(115200);
    delay(500);
    
    Serial.println("\n========================================");
    Serial.println("   testTABLEm3: Testing Module");
    Serial.println("   (with MODULE 1B back sensors)");
    Serial.println("========================================\n");
    
    // Initialize IR sensors
    pinMode(IR1, INPUT);   // Staff station
    pinMode(IR5, INPUT);   // Table counter
    sensors.initialize();  // Initialize IR2, IR3, IR4 for line following
    backSensors.initialize();  // MODULE 1B: Initialize back sensors
    
    // Initialize motors
    motors.initialize();
    
    // Initialize I2C for MPU9250 (currently unused)
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(400000);
    
    Serial.println("🗺️  Restaurant Map:");
    Serial.println("  Staff ←→ T1 → T2 → T3 → T4 → T5 → T6 → T7 → T8 → T9 → T10 ←→ Staff");
    Serial.println("  RIGHT side (1-5): FORWARD | LEFT side (6-10): BACKWARD\n");
    
    Serial.println("📋 Navigation Logic (Directional Grouping + 5F/4B):");
    Serial.println("  SINGLE ORDER: Tables 1-5 → FORWARD, Tables 6-10 → BACKWARD");
    Serial.println("  MULTIPLE ORDERS:");
    Serial.println("    Step 1: Detect multi-order");
    Serial.println("    Step 2: Count RIGHT(1-5) vs LEFT(6-10), start with MORE (tie=LEFT)");
    Serial.println("    Step 3: Serve all tables in that range opportunistically");
    Serial.println("    Step 4: Use 5F/4B rule for remaining tables");
    Serial.println("    Step 5: Return based on last table (1-5→BWD, 6-10→FWD)\n");
    
    // ============================================
    // 🧪 TEST ORDERS - EDIT HERE TO ADD ORDERS
    // ============================================
    Serial.println("🧪 TESTING MODE - Add your test orders below:");
    Serial.println("   (Edit setup() function in testTABLEm3.ino)\n");
    
    // 👇 ADD YOUR TEST ORDERS HERE:
    addOrder(1);
    //addOrder(2);
    addOrder(3);
    addOrder(9);
    //addOrder(8);
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
    // ⚠️ ONLY count tables during NAVIGATING (NOT during RETURNING)
    // During RETURNING, only IR1 matters for detecting staff
    // ⚠️ Ignore IR5 for 1 second after departing from table
    bool canDetectIR5 = (millis() - departureTime) > 1000;
    if (ir5 && !lastIR5State && currentState == NAVIGATING && canDetectIR5) {
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
    cumulativeCounter = 0;  // Reset counter
    initialSideChosen = false;  // Reset for new navigation
    
    // Initialize served status
    for (int i = 0; i < 11; i++) {
        orderServed[i] = false;
    }
    
    // Decide INITIAL direction - only happens ONCE at staff
    RangeDecision decision = decideDirection();
    initialSideChosen = true;
    servingRightSide = decision.isForward;  // Track which side we started with
    
    movingForward = decision.isForward;
    targetTable = decision.targetTable;
    
    Serial.print("🎯 Target: Table ");
    Serial.println(targetTable);
    Serial.print("📍 Direction: ");
    Serial.println(movingForward ? "FORWARD (RIGHT side 1-5)" : "BACKWARD (LEFT side 6-10)");
    Serial.print("📊 Starting position: Table ");
    Serial.println(currentTable);
    Serial.println("   ⚡ Will serve ALL tables with orders along the way!");
}

void onTableMarkerDetected() {
    // IR5 detected a black square (table marker)
    
    Serial.print("🔔 IR5 triggered! State: ");
    Serial.print(currentState == NAVIGATING ? "NAVIGATING" : "RETURNING");
    Serial.print(", Direction: ");
    Serial.println(movingForward ? "FORWARD" : "BACKWARD");
    
    if (currentState == NAVIGATING) {
        if (movingForward) {
            // Forward navigation: increment table number with wrap-around
            currentTable++;
            if (currentTable > 10) {
                currentTable = 1;  // Wrap T10 → T1
            }
            cumulativeCounter = currentTable;
            
            Serial.print("📊 Forward → Table ");
            Serial.println(currentTable);
        } else {
            // Backward navigation: decrement table number OR wrap from staff
            if (currentTable == 0) {
                // Starting from staff going backward → first marker is T10
                currentTable = 10;
            } else if (currentTable > 1) {
                // Normal backward: T9→T8, T8→T7, etc.
                currentTable--;
            } else {
                // From T1 going backward wraps to T10 (if continuing the loop)
                currentTable = 10;
            }
            cumulativeCounter = currentTable;
            
            Serial.print("📊 Backward → Table ");
            Serial.println(currentTable);
        }
        
        // Check if this table has an order (opportunistic serving)
        checkAndServeTable();
        
    } else if (currentState == RETURNING) {
        if (movingForward) {
            // Returning forward: T9→T10→Staff
            if (currentTable >= 10) {
                currentTable = 0;  // Arrived at staff
                Serial.println("📊 Return forward → Arrived at Staff!");
            } else {
                currentTable++;
                Serial.print("📊 Return forward → Passing Table ");
                Serial.println(currentTable);
            }
        } else {
            // Returning backward: T3→T2→T1→Staff
            if (currentTable <= 1) {
                currentTable = 0;  // Arrived at staff
                Serial.println("📊 Return backward → Arrived at Staff!");
            } else {
                currentTable--;
                Serial.print("📊 Return backward → Passing Table ");
                Serial.println(currentTable);
            }
        }
        cumulativeCounter = currentTable;
    }
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
        // ==== STEP 5: Return based on last table position ====
        Serial.println("\n🎉 All orders served!");
        
        if (currentTable >= 1 && currentTable <= 5) {
            // Last table in RIGHT side (1-5) → return BACKWARD to staff
            movingForward = false;
            cumulativeCounter = currentTable;  // Will count down: T3→T2→T1→Staff
            Serial.print("🔙 Last table T");
            Serial.print(currentTable);
            Serial.println(" is RIGHT (1-5) → Returning BACKWARD");
            Serial.print("   Counter set to: ");
            Serial.println(cumulativeCounter);
        } else {
            // Last table in LEFT side (6-10) → return FORWARD to staff
            movingForward = true;
            cumulativeCounter = currentTable;  // Will count up: T9→T10→Staff
            Serial.print("🔙 Last table T");
            Serial.print(currentTable);
            Serial.println(" is LEFT (6-10) → Returning FORWARD");
            Serial.print("   Counter set to: ");
            Serial.println(cumulativeCounter);
        }
        
        targetTable = 0;  // Target is staff station
        currentState = RETURNING;
        Serial.println("   State: RETURNING");
        return;
    }
    
    // ==== STEP 3 & 4: Continue on current side, or switch to other side ====
    // Check if there are remaining orders on the CURRENT side
    int currentSideOrders = 0;
    int currentSideFarthest = 0;
    
    for (int i = 0; i < orderCount; i++) {
        int table = orderList[i];
        if (!orderServed[table]) {
            if (servingRightSide && table >= 1 && table <= 5) {
                // Still have RIGHT side orders
                currentSideOrders++;
                if (table > currentSideFarthest) currentSideFarthest = table;
            } else if (!servingRightSide && table >= 6 && table <= 10) {
                // Still have LEFT side orders
                currentSideOrders++;
                if (currentSideFarthest == 0 || table < currentSideFarthest) currentSideFarthest = table;
            }
        }
    }
    
    if (currentSideOrders > 0) {
        // ==== STEP 3: Still have orders on current side - continue in same direction ====
        targetTable = currentSideFarthest;
        // Keep same direction (movingForward doesn't change)
        Serial.print("\n🎯 Continue on ");
        Serial.print(servingRightSide ? "RIGHT" : "LEFT");
        Serial.print(" side: Target T");
        Serial.println(targetTable);
    } else {
        // ==== STEP 4: Finished current side - switch to OTHER side using 5F/4B ====
        Serial.print("\n✅ Finished ");
        Serial.print(servingRightSide ? "RIGHT" : "LEFT");
        Serial.println(" side! Switching to other side...");
        
        // Use 5F/4B to find closest table on other side
        RangeDecision decision = decideNextDirection();
        targetTable = decision.targetTable;
        
        // Update which side we're now serving
        servingRightSide = !servingRightSide;
        
        Serial.print("🔄 Now serving ");
        Serial.print(servingRightSide ? "RIGHT" : "LEFT");
        Serial.println(" side");
        Serial.print("📊 5F/4B chose: ");
        Serial.print(decision.isForward ? "FORWARD" : "BACKWARD");
        Serial.print(" to T");
        Serial.println(targetTable);
        
        // *** CRITICAL: Update direction based on 5F/4B decision ***
        bool wasForward = movingForward;
        movingForward = decision.isForward;
        
        // Handle direction switch
        if (!movingForward && wasForward) {
            cumulativeCounter = currentTable;
            Serial.println("   🔄 Switching direction: FORWARD → BACKWARD");
            Serial.print("   Counter reset to: ");
            Serial.println(cumulativeCounter);
        } else if (movingForward && !wasForward) {
            cumulativeCounter = currentTable;
            Serial.println("   🔄 Switching direction: BACKWARD → FORWARD");
            Serial.print("   Counter reset to: ");
            Serial.println(cumulativeCounter);
        }
        
        departureTime = millis();
        currentState = NAVIGATING;
        return;  // Early return after side switch
    }
    
    bool wasForward = movingForward;
    
    // Handle direction switch - reset counter based on current position
    if (!movingForward && wasForward) {
        // Switching from forward to backward: will count down from currentTable
        // Counter represents how many markers until staff (going backward)
        cumulativeCounter = currentTable;
        Serial.println("   🔄 Switching direction: FORWARD → BACKWARD");
        Serial.print("   Counter reset to: ");
        Serial.println(cumulativeCounter);
    } else if (movingForward && !wasForward) {
        // Switching from backward to forward: will count up from currentTable
        cumulativeCounter = currentTable;
        Serial.println("   🔄 Switching direction: BACKWARD → FORWARD");
        Serial.print("   Counter reset to: ");
        Serial.println(cumulativeCounter);
    }
    // If no direction change, counter continues as-is
    
    Serial.print("\n🎯 Next target: Table ");
    Serial.println(targetTable);
    Serial.print("   Direction: ");
    Serial.println(movingForward ? "FORWARD" : "BACKWARD");
    
    departureTime = millis();  // Start 1-second IR5 ignore period
    currentState = NAVIGATING;
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
    Serial.println("⚡ Directional Grouping + 5F/4B algorithm!");
}

// ============================================== DIRECTION DECISION ALGORITHM /////////////////

RangeDecision decideDirection() {
    unsigned long startMicros = micros();
    RangeDecision decision;
    
    // Get unserved tables
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
    
    // ==== SINGLE ORDER RULE ====
    if (unservedCount == 1) {
        int table = unservedTables[0];
        if (table >= 1 && table <= 5) {
            // RIGHT side (1-5) → FORWARD
            decision.isForward = true;
            decision.targetTable = table;
            Serial.println("📍 Single order: Table 1-5 → FORWARD (RIGHT side)");
        } else {
            // LEFT side (6-10) → BACKWARD
            decision.isForward = false;
            decision.targetTable = table;
            Serial.println("📍 Single order: Table 6-10 → BACKWARD (LEFT side)");
        }
    }
    // ==== MULTIPLE ORDER RULE ====
    else {
        // Step 2: Count RIGHT (1-5) vs LEFT (6-10)
        int rightCount = 0, leftCount = 0;
        int rightFarthest = 0, leftFarthest = 11;  // leftFarthest starts at 11 (higher than any table)
        
        for (int i = 0; i < unservedCount; i++) {
            int table = unservedTables[i];
            if (table >= 1 && table <= 5) {
                rightCount++;
                if (table > rightFarthest) rightFarthest = table;  // Highest number in RIGHT range
            } else if (table >= 6 && table <= 10) {
                leftCount++;
                if (table < leftFarthest) leftFarthest = table;  // Lowest number in LEFT range (farthest backward)
            }
        }
        
        Serial.print("📊 RIGHT (1-5): ");
        Serial.print(rightCount);
        Serial.print(" orders, LEFT (6-10): ");
        Serial.print(leftCount);
        Serial.println(" orders");
        
        // Step 2: Choose side with MORE orders (tie = LEFT)
        if (rightCount > leftCount) {
            // RIGHT has MORE → Go FORWARD to FARTHEST right table
            decision.isForward = true;
            decision.targetTable = rightFarthest;
            Serial.print("📍 Multi-order: RIGHT has MORE (");
            Serial.print(rightCount);
            Serial.print(" > ");
            Serial.print(leftCount);
            Serial.print(") → FORWARD to farthest T");
            Serial.println(rightFarthest);
        } else if (leftCount > rightCount) {
            // LEFT has MORE → Go BACKWARD to FARTHEST left table
            decision.isForward = false;
            decision.targetTable = leftFarthest;
            Serial.print("📍 Multi-order: LEFT has MORE (");
            Serial.print(leftCount);
            Serial.print(" > ");
            Serial.print(rightCount);
            Serial.print(") → BACKWARD to farthest T");
            Serial.println(leftFarthest);
        } else {
            // EQUAL: Default to LEFT (backward)
            decision.isForward = false;
            decision.targetTable = (leftFarthest <= 10) ? leftFarthest : rightFarthest;
            Serial.print("📍 Multi-order: EQUAL (");
            Serial.print(rightCount);
            Serial.print(" = ");
            Serial.print(leftCount);
            Serial.print(") → Default LEFT (BACKWARD) to T");
            Serial.println(decision.targetTable);
            
            // Fallback if no left tables
            if (leftFarthest > 10 && rightFarthest > 0) {
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

// 5F/4B Rule: For remaining tables after first side
RangeDecision decideNextDirection() {
    unsigned long startMicros = micros();
    RangeDecision decision;
    
    // Get unserved tables
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
    
    // Single remaining → use simple rule
    if (unservedCount == 1) {
        int table = unservedTables[0];
        decision.isForward = (table >= 1 && table <= 5);
        decision.targetTable = table;
        return decision;
    }
    
    // 5F/4B Rule: Calculate steps to each unserved table
    int bestForwardTable = -1, bestForwardSteps = 999;
    int bestBackwardTable = -1, bestBackwardSteps = 999;
    
    for (int i = 0; i < unservedCount; i++) {
        int table = unservedTables[i];
        
        // Forward steps (max 5)
        int forwardSteps;
        if (table > currentTable) {
            forwardSteps = table - currentTable;
        } else {
            forwardSteps = (10 - currentTable) + table;  // Wrap around
        }
        
        // Backward steps (max 4)
        int backwardSteps;
        if (table < currentTable) {
            backwardSteps = currentTable - table;
        } else {
            backwardSteps = currentTable + (10 - table);  // Wrap around
        }
        
        // Check if within reachability
        if (forwardSteps <= 5 && forwardSteps < bestForwardSteps) {
            bestForwardSteps = forwardSteps;
            bestForwardTable = table;
        }
        if (backwardSteps <= 4 && backwardSteps < bestBackwardSteps) {
            bestBackwardSteps = backwardSteps;
            bestBackwardTable = table;
        }
    }
    
    Serial.print("📊 5F/4B from T");
    Serial.print(currentTable);
    Serial.print(": Fwd→T");
    Serial.print(bestForwardTable);
    Serial.print("(");
    Serial.print(bestForwardSteps);
    Serial.print(" steps), Bwd→T");
    Serial.print(bestBackwardTable);
    Serial.print("(");
    Serial.print(bestBackwardSteps);
    Serial.println(" steps)");
    
    // Choose direction: prefer shorter path, then backward
    if (bestBackwardTable != -1 && (bestForwardTable == -1 || bestBackwardSteps <= bestForwardSteps)) {
        decision.isForward = false;
        decision.targetTable = bestBackwardTable;
    } else if (bestForwardTable != -1) {
        decision.isForward = true;
        decision.targetTable = bestForwardTable;
    } else {
        // Fallback: use simple rule
        decision.isForward = (unservedTables[0] >= 1 && unservedTables[0] <= 5);
        decision.targetTable = unservedTables[0];
    }
    
    unsigned long calcTime = micros() - startMicros;
    Serial.print("⚡ Calculation time: ");
    Serial.print(calcTime);
    Serial.println(" μs");
    
    return decision;
}

// ============================================== LINE FOLLOWING (MODULE 1B) /////////////////

void followLine() {
    int error, speed_difference;
    
    if (movingForward) {
        // FORWARD: Use front sensors
        error = sensors.calculate_error();
        speed_difference = pid.calculate_speed_difference(error);
        motors.drive(speed_difference, DEBUG);
    } else {
        // BACKWARD: Use back sensors (MODULE 1B)
        error = backSensors.calculate_error();
        speed_difference = backPid.calculate_speed_difference(error);
        motors.drive_backward(speed_difference, DEBUG);
    }
    
    if(DEBUG) {
        Serial.print("Direction: ");
        Serial.println(movingForward ? "FORWARD (front sensors)" : "BACKWARD (back sensors)");
        Serial.print("Error: ");
        Serial.print(error);
        Serial.print(", Speed diff: ");
        Serial.println(speed_difference);
    }
}

// ============================================== MOTOR CONTROL /////////////////

void stopMotors() {
    motors.stop();
}
