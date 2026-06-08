/*
 * ====================================================
 * ESP32 LINE FOLLOWER - BACKWARD WITH BACK SENSORS
 * ====================================================
 * MODULE1B - BACKWARD LINE FOLLOWING ✅ WORKING
 * 
 * Hardware:
 * - ESP32 Dev Module
 * - 3x BACK IR Sensors: Left(27), Middle(26), Right(25)
 * - L298N Motor Driver
 * - 4x DC Motors
 * 
 * Sensor Spacing: 1cm between each sensor
 * Total span: 4cm (left to right, including 2cm middle sensor)
 * 
 * Features:
 * - PID-based line following IN REVERSE
 * - Uses BACK sensors (near rear wheels)
 * - SAME logic as forward - no delay compensation needed!
 * ====================================================
 */

// ============================================== SETTINGS /////////////////

// For 4cm line width with 1cm sensor spacing (digital sensors):
// BACKWARD MODE: Same PID as forward because back sensors are near rear wheels!
const int NORMAL_SPEED = 70;       // ✅ Same speed as forward
const float KP = 45;               // ✅ Same as forward MODULE1
const float KI = 0;                // Keep at 0
const float KD = 30;               // ✅ Same as forward MODULE1

const int SENSORS_COUNT = 3;  // 3 BACK IR sensors
const int SENSOR_PINS[SENSORS_COUNT] = {27, 26, 25};  // {Back Left, Back Middle, Back Right} ✅ TESTED
const int ERRORS[2] = {0, 1};  // Only 2 error levels for 3 sensors

const int MAX_SPEED = 255;
const int MIN_SPEED = 0;

// L298N Motor Driver Pins (ESP32 GPIO)
const int LEFT_IN1 = 15;   // Left motor direction 1
const int LEFT_IN2 = 2;    // Left motor direction 2
const int LEFT_EN = 17;    // Left motor speed (PWM)

const int RIGHT_IN1 = 18;  // Right motor direction 1
const int RIGHT_IN2 = 19;  // Right motor direction 2
const int RIGHT_EN = 23;   // Right motor speed (PWM)

const bool DEBUG = false;  // Set to true for testing
const int DELAY_TIME = 200;  // Debug delay in milliseconds

// ==============================================  /////////////////


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
        // 3 BACK Sensors: Left(25), Middle(26), Right(27)
        // SAME logic as forward - back sensors are near rear wheels!
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
        int sensor_value = digitalRead(sensor_pin);  // ✅ Changed to digitalRead()
        return sensor_value == HIGH;  // ✅ BLACK line = HIGH
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
        // MODULE1B: ONLY BACKWARD - speeds are always negative
        // Backward: LEFT(HIGH-LOW), RIGHT(LOW-HIGH)
        if(is_left_motor) {
            dir1 = HIGH;
            dir2 = LOW;
        } else {
            dir1 = LOW;
            dir2 = HIGH;
        }
    }

    int normalize_speed(int speed) {
        return constrain(abs(speed), min_speed, max_speed);
    }

    void drive_backward(int speed_difference, bool debug) {
        // ⚠️ BACKWARD MOVEMENT: Just negate speeds, motor direction reversal handles steering
        int left_speed = -(normal_speed + speed_difference);   // Negative = backward
        int right_speed = -(normal_speed - speed_difference);  // Negative = backward

        set_motor(left_in1, left_in2, left_en, left_speed, true);   // true = left motor
        set_motor(right_in1, right_in2, right_en, right_speed, false); // false = right motor

        if(debug) {
            Serial.print("BACKWARD - left_speed: ");
            Serial.print(normalize_speed(left_speed));
            Serial.print(" - ");
            Serial.println(left_speed > 0 ? "F" : "B");

            Serial.print("BACKWARD - right_speed: ");
            Serial.print(normalize_speed(right_speed));
            Serial.print(" - ");
            Serial.println(right_speed > 0 ? "F" : "B");
        }
    }

};


// ============================================== GLOBAL OBJECTS /////////////////
Sensors sensors(SENSORS_COUNT, SENSOR_PINS, ERRORS);  // ✅ Removed THRESHOLD
Motors motors(
    NORMAL_SPEED, MIN_SPEED, MAX_SPEED,
    LEFT_IN1, LEFT_IN2, LEFT_EN,
    RIGHT_IN1, RIGHT_IN2, RIGHT_EN
);
PID pid(KP, KI, KD);


// ============================================== SETUP /////////////////
void setup() {
    Serial.begin(115200);
    Serial.println("\n========================================");
    Serial.println("   ESP32 Line Follower - BACK SENSORS");
    Serial.println("========================================");
    Serial.println("Hardware:");
    Serial.println("  - 3 BACK IR Sensors: L=25, M=26, R=27");
    Serial.println("  - Sensor spacing: 1cm between each");
    Serial.println("  - Total span: 4cm");
    Serial.println("  - L298N Motor Driver");
    Serial.println("  - 4 DC Motors\n");
    Serial.println("⚠️  BACKWARD MOVEMENT MODE");
    Serial.println("  - Using BACK sensors (near rear wheels)");
    Serial.println("  - Same PID as forward - no delay!\n");
    Serial.print("Normal Speed: ");
    Serial.println(NORMAL_SPEED);
    Serial.print("KP: ");
    Serial.println(KP);
    Serial.print("KD: ");
    Serial.println(KD);
    Serial.println();
    
    sensors.initialize();  // ✅ Initialize sensor pins
    motors.initialize();
    
    Serial.println("✅ Sensors initialized");
    Serial.println("✅ Motors initialized");
    Serial.println("🚗 Starting BACKWARD line following in 2 seconds...\n");
    delay(2000);
}


// ============================================== MAIN LOOP /////////////////
void loop() {
    int error = sensors.calculate_error();
    int speed_difference = pid.calculate_speed_difference(error);
    motors.drive_backward(speed_difference, DEBUG);  // ⚠️ Using drive_backward()

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
        delay(DELAY_TIME);
    }
}
