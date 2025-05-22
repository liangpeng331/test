# ESP32 Bluetooth RC Car

This project implements a remote-controlled car using an ESP32, an HR8833 motor driver (or similar 2-channel driver), and an Android application.

## Features
- Control car movement (Forward, Backward, Left, Right, Stop) via Bluetooth.
- Android app for sending commands.
- ESP32 firmware for motor control and Bluetooth communication.

## Hardware Requirements
- ESP32 development board
- HR8833 Motor Driver (or any 2-channel DC motor driver compatible with ESP32 logic levels)
- Two DC motors
- Chassis for the car
- Power source for ESP32 and motors
- Android device (for the controller app)

## ESP32 Pinout (as per `esp32_rc_car.ino`)
- MA Motor Speed Control (PWM): GPIO5 (D5)
- MA Motor Direction Control: GPIO7 (D7)
- MB Motor Speed Control (PWM): GPIO6 (D6)
- MB Motor Direction Control: GPIO8 (D8)

## ESP32 Firmware (`esp32_rc_car.ino`)
- Written in C/C++ for the Arduino IDE.
- Uses Bluetooth Serial for communication.
- Device Name: "ESP32_RC_Car"
- Commands:
  - 'F': Forward
  - 'B': Backward
  - 'L': Turn Left (pivot)
  - 'R': Turn Right (pivot)
  - 'S': Stop

## Android App (`RCCarController`)
- Written in Kotlin.
- Scans for Bluetooth devices, allows connection to "ESP32_RC_Car".
- Sends single-character commands for movement.
- Requires Bluetooth and Location permissions.

## Setup & Usage
1. **ESP32:**
   - Connect motors and ESP32 to the HR8833 driver as per the pinout.
   - Upload the `esp32_rc_car.ino` sketch using the Arduino IDE.
   - Power on the ESP32 setup.
2. **Android:**
   - Build and install the `RCCarController` app from Android Studio.
   - Enable Bluetooth on your Android device.
   - Open the app, tap "Connect to Car".
   - Select "ESP32_RC_Car" from the list to connect.
   - Use the on-screen buttons to control the car.

## Notes
- Motor direction might need to be inverted in the ESP32 code (`motorA_set` / `motorB_set` functions) depending on wiring.
- `DEFAULT_SPEED` in the ESP32 firmware can be adjusted (0-255).
- Ensure the HR8833 driver is correctly powered and can supply enough current for the motors.
- For Android 12 (API 31) and above, ensure `BLUETOOTH_SCAN` and `BLUETOOTH_CONNECT` permissions are granted. Location permission is also requested as it's good practice for Bluetooth scanning.
