# SUMOBOT - TB6612FNG Motor Controller

A web-based robot motor control system using ESP32 and TB6612FNG dual motor driver. Control your robot via a responsive web interface with real-time speed adjustment and directional controls.

## Features

- **Web-based Control Interface**: Modern, responsive dashboard accessible from any device on the local network
- **Dual Motor Control**: Independent control of left and right motors using SparkFun TB6612FNG driver
- **Speed Adjustment**: Real-time PWM speed control (50-255) with visual feedback
- **Directional Controls**: Forward, backward, left turn, right turn, and stop commands
- **Debug Console**: Built-in hardware status checking and WiFi diagnostics
- **WiFi Access Point**: Creates standalone WiFi network without requiring router connectivity
- **Hardware Status Monitoring**: Periodic debug logs for motor and driver status

## Hardware Requirements

- **Microcontroller**: ESP32 Development Board
- **Motor Driver**: SparkFun TB6612FNG (or compatible)
- **Motors**: 2x DC Motors (compatible with TB6612FNG)
- **Power**: Separate power supply for motors (VM) and logic (VCC 3.3V-5V)
- **Connections**: Common ground between ESP32 and motor driver

## Pin Configuration

| Purpose | GPIO Pin | Notes |
|---------|----------|-------|
| Motor A Direction 1 | GPIO 4 (AIN1) | Motor A forward |
| Motor A Direction 2 | GPIO 14 (AIN2) | Motor A backward |
| Motor A Speed (PWM) | GPIO 5 (PWMA) | PWM frequency 5kHz |
| Motor B Direction 1 | GPIO 18 (BIN1) | Motor B forward |
| Motor B Direction 2 | GPIO 19 (BIN2) | Motor B backward |
| Motor B Speed (PWM) | GPIO 21 (PWMB) | PWM frequency 5kHz |
| Standby (Enable) | GPIO 33 (STBY) | Active HIGH to enable driver |
| Status LED | GPIO 2 | Built-in LED indicator |

## Setup & Installation

### 1. Hardware Wiring

Connect the TB6612FNG motor driver to your ESP32:

```
TB6612FNG      →  ESP32
─────────────────────────
AIN1           →  GPIO 4
AIN2           →  GPIO 14
PWMA           →  GPIO 5
BIN1           →  GPIO 18
BIN2           →  GPIO 19
PWMB           →  GPIO 21
STBY           →  GPIO 33
GND            →  GND (COMMON)
VM             →  Motor Power Supply (+)
GND (VM)       →  Motor Power Supply (-)
VCC            →  3.3V or 5V (Logic voltage)
```

### 2. Software Setup

**Prerequisites:**
- PlatformIO IDE installed (VS Code extension recommended)
- Python 3.x

**Steps:**

1. Clone/open this repository in PlatformIO

2. Install dependencies:
```bash
pio lib install "mbed-ateyercheese/Sparkfun_TB6612@0.0.0+sha.9d2787060b3e"
```

3. Build the project:
```bash
pio run --environment esp32dev
```

4. Upload to ESP32:
```bash
pio run --target upload --environment esp32dev
```

5. Monitor serial output:
```bash
pio device monitor --baud 115200
```

## Usage

### Connecting to the Robot

1. Power on the ESP32
2. On your device, connect to WiFi network: **`RobotController`**
3. WiFi Password: **`12345678`**
4. Open browser and navigate to: **`http://192.168.4.1`**

### Control Interface

The web dashboard provides:

- **▲ Forward Button**: Move robot forward (hold to continue)
- **▼ Backward Button**: Move robot backward (hold to continue)
- **◀ Left Turn Button**: Rotate robot left (hold to continue)
- **▶ Right Turn Button**: Rotate robot right (hold to continue)
- **■ Stop Button**: Emergency stop / brake
- **Speed Slider**: Adjust motor speed from 50% to 100% (PWM 50-255)
- **🔍 Debug Serial Button**: Display hardware and WiFi status

### Motor Movement Logic

| Command | Left Motor | Right Motor |
|---------|-----------|-------------|
| Forward | Forward | Forward |
| Backward | Backward | Backward |
| Left Turn | Backward | Forward |
| Right Turn | Forward | Backward |
| Stop | Brake | Brake |

## API Endpoints

The web server exposes the following endpoints:

### GET /
Returns the HTML control interface.

### GET /motor?cmd=<COMMAND>&val=<VALUE>

**Commands:**
- `maju` - Move forward
- `mundur` - Move backward
- `kiri` - Turn left
- `kanan` - Turn right
- `stop` - Stop and brake
- `speed&val=<50-255>` - Set motor speed
- `debug` - Print debug information to serial

**Examples:**
```
http://192.168.4.1/motor?cmd=maju
http://192.168.4.1/motor?cmd=speed&val=200
http://192.168.4.1/motor?cmd=stop
http://192.168.4.1/motor?cmd=debug
```

## Troubleshooting

### Robot not responding

1. **Check Serial Connection**: Verify USB connection and correct COM port
2. **Check WiFi**: Confirm device is connected to "RobotController" network
3. **Check Power**: Ensure motor power supply is connected and turned on
4. **Click Debug Button**: View hardware and WiFi status in serial monitor

### Motors not moving

1. **Verify Ground Connection**: Ensure common ground between ESP32 and motor driver
2. **Check Voltage**: Motor power supply (VM) should match motor specifications
3. **Check STBY Pin**: Serial monitor should show "STBY set HIGH"
4. **Test with Serial Commands**: Use debug mode to verify PWM output

### Erratic Motor Behavior

1. **Motor Power Interference**: Add capacitors near motor power input
2. **Poor Wiring**: Re-secure all connections, especially power lines
3. **Conflicting Pin Usage**: Verify no pins are used elsewhere

## Serial Debug Output

Monitor the serial port at 115200 baud to see:

```
======== BOOTING ========
ESP32 TB6612FNG Debug Mode
[INIT] STBY set HIGH -> Driver enabled
===== CHECK HARDWARE =====
STBY Pin (33) State: HIGH
AIN1: 4, AIN2: 14, PWMA: 5
BIN1: 18, BIN2: 19, PWMB: 21
...
[WIFI] Access Point SSID: RobotController
[WIFI] IP Address: 192.168.4.1
[WIFI] Client count: 1
[SERVER] Web server dimulai pada port 80
```

## Performance Specifications

- **PWM Frequency**: 5 kHz
- **Speed Range**: 50-255 (PWM values)
- **Response Time**: <50ms for commands
- **Concurrent Clients**: Up to ~10 simultaneous WiFi connections
- **Baud Rate**: 115200 bps (serial debugging)

## Project Structure

```
SUMOBOT/
├── src/
│   └── main.cpp              # Main program with motor control and web server
├── include/                  # Header files (if needed)
├── lib/                      # External libraries
├── test/                     # Test files
├── platformio.ini           # PlatformIO configuration
└── README.md                # This file
```

## Dependencies

- **SparkFun_TB6612**: Motor driver library
- **WiFi.h**: Built-in WiFi functionality
- **WebServer.h**: Built-in web server functionality
- **Arduino.h**: Standard Arduino framework (via PlatformIO)

## Configuration

Edit the following in `src/main.cpp` to customize:

```cpp
// WiFi Settings
const char* ssid = "RobotController";        // WiFi network name
const char* password = "12345678";           // WiFi password

// Default Speed
int speedValue = 200;                        // Initial motor speed (50-255)

// GPIO Pins
#define AIN1 4                               // Motor A direction 1
#define AIN2 14                              // Motor A direction 2
#define PWMA 5                               // Motor A PWM
#define BIN1 18                              // Motor B direction 1
#define BIN2 19                              // Motor B direction 2
#define PWMB 21                              // Motor B PWM
#define STBY 33                              // Standby/Enable pin
#define LED_PIN 2                            // Status LED
```

## Development Notes

- Motor offset value is set to 1 for positive logic direction
- Built-in LED (GPIO 2) blinks to indicate server operation
- Periodic debug output every 10 seconds shows system status
- All motor commands include serial logging for debugging
- HTML/CSS is embedded directly in the firmware for minimal storage usage

## License

This project is part of the SUMOBOT robot platform.

## Support

For issues or questions:
1. Check the serial monitor for debug output
2. Verify hardware connections match pin configuration
3. Ensure proper power supply for motors and logic circuits
4. Review troubleshooting section above

---

**Last Updated**: 2026-05-14  
**Platform**: ESP32 (espressif32)  
**Framework**: Arduino
