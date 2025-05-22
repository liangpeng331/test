#include "BluetoothSerial.h"

// BluetoothSerial Object for communication with the Android App
BluetoothSerial SerialBT;

// Pin Definitions for HR8833 Motor Driver
const int MA_SPEED_PIN = 5;  // ESP32 pin D5 (GPIO5) connected to Motor A Speed Control (PWM)
const int MA_DIR_PIN = 7;    // ESP32 pin D7 (GPIO7) connected to Motor A Direction Control
const int MB_SPEED_PIN = 6;  // ESP32 pin D6 (GPIO6) connected to Motor B Speed Control (PWM)
const int MB_DIR_PIN = 8;    // ESP32 pin D8 (GPIO8) connected to Motor B Direction Control

// PWM Channel Definitions for Motor Speed Control
const int MA_PWM_CHANNEL = 0; // ESP32 PWM channel for Motor A
const int MB_PWM_CHANNEL = 1; // ESP32 PWM channel for Motor B
const int PWM_FREQ = 5000;      // PWM frequency in Hz (5kHz is common for motors)
const int PWM_RESOLUTION = 8;   // PWM resolution in bits (8-bit = 0-255 duty cycle range)

// Default speed for movement commands received via Bluetooth
const int DEFAULT_SPEED = 150; // Speed value from 0-255 (approx 60% of max speed)

// Function to initialize motor control pins and PWM channels
void setupMotors() {
  // Initialize direction pins as OUTPUT to set motor rotation direction
  pinMode(MA_DIR_PIN, OUTPUT);
  pinMode(MB_DIR_PIN, OUTPUT);

  // Configure PWM channels
  ledcSetup(MA_PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcSetup(MB_PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);

  // Attach PWM channels to motor speed pins
  ledcAttachPin(MA_SPEED_PIN, MA_PWM_CHANNEL);
  ledcAttachPin(MB_SPEED_PIN, MB_PWM_CHANNEL);
}

// Function to set speed and direction for Motor A.
// Parameters:
//   speed: The desired speed for the motor (0-255). Values outside this range are constrained.
//   forward: true for forward motion, false for reverse. The actual direction depends on motor wiring.
void motorA_set(int speed, bool forward) {
  // Constrain speed to the valid PWM range (0-255 for 8-bit resolution)
  speed = constrain(speed, 0, 255);

  // Set motor direction pin (HIGH for forward, LOW for reverse - this might need to be flipped based on wiring)
  digitalWrite(MA_DIR_PIN, forward ? HIGH : LOW);

  // Set motor speed using PWM
  ledcWrite(MA_PWM_CHANNEL, speed);
}

// Function to set speed and direction for Motor B.
// Parameters:
//   speed: The desired speed for the motor (0-255). Values outside this range are constrained.
//   forward: true for forward motion, false for reverse. The actual direction depends on motor wiring.
void motorB_set(int speed, bool forward) {
  // Constrain speed to the valid PWM range (0-255 for 8-bit resolution)
  speed = constrain(speed, 0, 255);

  // Set motor direction pin (HIGH for forward, LOW for reverse - this might need to be flipped based on wiring)
  digitalWrite(MB_DIR_PIN, forward ? HIGH : LOW);

  // Set motor speed using PWM
  ledcWrite(MB_PWM_CHANNEL, speed);
}

// Function to move both motors forward at a given speed.
// Parameter: speed - The speed for the motors (0-255).
void moveForward(int speed) {
  Serial.println("Action: Moving Forward"); // USB Serial debug
  motorA_set(speed, true);
  motorB_set(speed, true);
}

// Function to move both motors backward at a given speed.
// Parameter: speed - The speed for the motors (0-255).
void moveBackward(int speed) {
  Serial.println("Action: Moving Backward"); // USB Serial debug
  motorA_set(speed, false);
  motorB_set(speed, false);
}

// Function to turn the car left (pivot turn: Motor A backward, Motor B forward).
// Parameter: speed - The speed for the motors (0-255).
void turnLeft(int speed) {
  Serial.println("Action: Turning Left"); // USB Serial debug
  motorA_set(speed, false); // Motor A backward
  motorB_set(speed, true);  // Motor B forward
}

// Function to turn the car right (pivot turn: Motor A forward, Motor B backward).
// Parameter: speed - The speed for the motors (0-255).
void turnRight(int speed) {
  Serial.println("Action: Turning Right"); // USB Serial debug
  motorA_set(speed, true);  // Motor A forward
  motorB_set(speed, false); // Motor B backward
}

// Function to stop both motors.
void stopMotors() {
  Serial.println("Action: Stopping Motors"); // USB Serial debug
  motorA_set(0, true); // Set speed to 0. Direction doesn't matter for stopping.
  motorB_set(0, true); // Set speed to 0.
}

// Setup function: This function runs once when the ESP32 starts or resets.
void setup() {
  // Initialize motor control pins and PWM settings
  setupMotors();

  // Initialize serial communication for debugging via USB monitor
  Serial.begin(115200); // Set baud rate for serial monitor
  Serial.println("ESP32 RC Car Controller Initializing...");

  // Initialize Bluetooth Serial communication with a specific device name
  SerialBT.begin("ESP32_RC_Car"); // This is the name the Android app will look for
  Serial.println("Bluetooth device active, name: ESP32_RC_Car. Ready to pair and connect."); 
}

// Loop function: This function runs repeatedly after setup() completes.
void loop() {
  // Check if there is any data available to read from Bluetooth Serial
  if (SerialBT.available()) {
    char cmd = SerialBT.read(); // Read the incoming byte (character command)
    
    // Print received command to USB Serial for debugging
    Serial.print("Received BT Command: '");
    Serial.print(cmd);
    Serial.println("'");

    // Process the received command using a switch statement
    switch (cmd) {
      case 'F': // Forward command
        moveForward(DEFAULT_SPEED);
        break;
      case 'B': // Backward command
        moveBackward(DEFAULT_SPEED);
        break;
      case 'L': // Turn Left command
        turnLeft(DEFAULT_SPEED);
        break;
      case 'R': // Turn Right command
        turnRight(DEFAULT_SPEED);
        break;
      case 'S': // Stop command
        stopMotors();
        break;
      default: // Unknown command
        Serial.println("Action: Unknown command received. No action taken.");
        break;
    }
  }
  // A small delay can be added here if needed, e.g., for power saving or to prevent loop spinning too fast,
  // but for responsive control, it's often omitted or kept very short.
  // delay(10); 
}
