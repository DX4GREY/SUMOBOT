# SUMOBOT 500G - Professional ESP32 RC Mini Sumo Controller Firmware

An advanced, competition-grade firmware for 500g RC Sumobots (Mini Sumo class) powered by the **ESP32** microcontroller and the **TB6612FNG** dual H-Bridge motor driver. Features ultra-low latency Bluetooth gamepad connectivity via **Bluepad32**, active dynamic braking, anti-wheelie acceleration ramping, on-the-fly steering/throttle inversion, autonomous opening tactics with zero-latency operator override, full RGB/haptic rumble feedback, and non-volatile configuration storage powered by **LittleFS**.

---

## Table of Contents

1. [Key Features](#key-features)
2. [System Architecture](#system-architecture)
3. [Hardware Specifications & Pinout](#hardware-specifications--pinout)
4. [Gamepad Controller Mapping](#gamepad-controller-mapping)
5. [Driving Dynamics & Control Logic](#driving-dynamics--control-logic)
   - [Exponential Stick Response Curve](#exponential-stick-response-curve)
   - [Differential Arcade Drive Mixing](#differential-arcade-drive-mixing)
   - [Dynamic Active Short Braking](#dynamic-active-short-braking)
   - [Anti-Wheelie Asymmetric Acceleration Limiter](#anti-wheelie-asymmetric-acceleration-limiter)
   - [On-the-Fly Inversion (L3 & R3)](#on-the-fly-inversion-l3--r3)
6. [Autonomous Opening Tactics](#autonomous-opening-tactics)
   - [Tactic 1: Flank Right](#tactic-1-flank-right)
   - [Tactic 2: Flank Left](#tactic-2-flank-left)
   - [Tactic 3: Blitz Charge](#tactic-3-blitz-charge)
   - [Tactic 4: Juke & Strike](#tactic-4-juke--strike)
   - [Instant Operator Override & Failsafes](#instant-operator-override--failsafes)
7. [Haptic & Visual Feedback System](#haptic--visual-feedback-system)
8. [LittleFS Configuration Persistence](#littlefs-configuration-persistence)
9. [Configuration & Tuning Guide](#configuration--tuning-guide)
10. [Controller Pairing Instructions](#controller-pairing-instructions)
    - [DualShock 4 (PS4) / DualSense (PS5) / Xbox / Switch Pro](#dualshock-4-ps4--dualsense-ps5--xbox--switch-pro)
    - [DualShock 3 (PS3) via USB Pairing Script](#dualshock-3-ps3-via-usb-pairing-script)
    - [Pitstop Pairing Reset](#pitstop-pairing-reset)
11. [Build, Flash & Debug](#build-flash--debug)
12. [Wokwi Simulation](#wokwi-simulation)
13. [Project Structure & File Manifest](#project-structure--file-manifest)
14. [Troubleshooting & FAQ](#troubleshooting--faq)

---

## Key Features

- **Multi-Controller Bluetooth Engine**: Native compatibility with Sony DualShock 4 (PS4), DualSense (PS5), DualShock 3 (Sixaxis), Microsoft Xbox One/Series Wireless, Nintendo Switch Pro/Joy-Con, and standard HID gamepads using Bluepad32.
- **Precision Arcade Differential Drive**: Independent left/right motor mixing with stick deadzone filtering and subtle cubic exponential (*Expo*) curvature for surgical tracking at center stick and raw power at full deflection.
- **Dynamic Active Braking (*Short Brake*)**: Utilizes TB6612FNG low-side MOSFET shorting to halt motor back-EMF instantaneously, stopping the chassis on a dime at the dohyo white border line instead of coasting off the ring.
- **Anti-Wheelie (*Anti-Jengat*) Acceleration Limiter**: Asymmetric acceleration ramp ensures the robot's front wedge stays flush against the dohyo surface during explosive launches, while preserving razor-sharp deceleration and turning responsiveness. Toggleable on-the-fly via gamepad.
- **3 Speed Profiles + Instant Attack Turbo**:
  - **Mode 1 (Low - PWM 130)**: Precision maneuvering, tactical baiting, low-traction rings.
  - **Mode 2 (Normal - PWM 180)**: Balanced combat standard with optimized pushing torque.
  - **Mode 3 (Fast - PWM 220)**: High-speed flanking and perimeter sweeps.
  - **Attack Mode (Turbo - PWM 255)**: Holding R1 bypasses ramping instantly for 100% full-voltage battery dump into the motors.
- **On-the-Fly Inversion (L3 & R3)**:
  - **L3 (Left Stick Click)**: Inverts forward/backward throttle (`throttle = -throttle`). Ideal when driving reverse or using dual-blade sumobots.
  - **R3 (Right Stick Click)**: Inverts steering direction (`steer = -steer`). Ideal for reverse-perspective driving across the arena.
- **4 Autonomous Opening Tactics**: Pre-programmed opening gambits (Flank Right, Flank Left, Blitz Charge, Juke & Strike) launched via simple trigger combos.
- **Instant Operator Stick Override**: Touching either analog stick or pressing the Stop/Brake button instantly aborts autonomous routines with 0 ms latency, handing full manual authority back to the driver.
- **LittleFS Non-Volatile Persistence**: Automatically saves throttle inversion, steering inversion, active speed mode, and anti-wheelie state to flash memory (`/config.txt`). Settings survive power cycles.
- **RGB Lightbar & Dual Rumble Feedback**: Real-time status indication on supported gamepads (Green for Low, Blue for Normal, Yellow for Fast, Red for Attack Turbo, Purple for Tactics, Amber for Handbrake) paired with distinctive haptic rumble pulses.
- **Failsafe Watchdog & Fast Auto-Reconnect**: Emergency motor cutoff if controller packets drop for >500 ms. Stored NVS bonding keys enable sub-second reconnection upon switching on the robot's main power switch.

---

## System Architecture

```
                    +------------------------------------+
                    |        Bluetooth Gamepad           |
                    | (PS4 / PS3 / PS5 / Xbox / Switch)  |
                    +-----------------+------------------+
                                      | Bluetooth HID
                                      v
                    +------------------------------------+
                    |        ESP32 (Bluepad32)           |
                    |  - Stick Normalization & Expo      |
                    |  - Arcade Mixing & Direction Invert|
                    |  - Autonomous Tactics Engine       |
                    |  - LittleFS Config Persistence     |
                    +--------+------------------+--------+
                             |                  |
           PWM & Direction A |                  | PWM & Direction B
                             v                  v
                    +------------------------------------+
                    |       TB6612FNG Motor Driver       |
                    |  - Active Dynamic Short Braking    |
                    |  - VM: 7.4V - 11.1V / 12V LiPo     |
                    +--------+------------------+--------+
                             |                  |
                             v                  v
                      [ Motor Left ]      [ Motor Right ]
                      (Micro N20 Gear)    (Micro N20 Gear)
```

---

## Hardware Specifications & Pinout

### Recommended Components

| Component | Specification | Purpose |
|---|---|---|
| **Microcontroller** | ESP32 Dev Module (ESP-WROOM-32, 240MHz, Dual Core) | Main controller, Bluetooth stack, real-time PWM |
| **Motor Driver** | SparkFun TB6612FNG (or clone module) | Dual H-Bridge, 1.2A continuous / 3.2A peak per channel |
| **Motors** | 2x N20 Micro Metal Gearmotors (600 RPM @ 6V-12V) | High torque-to-weight ratio propulsion |
| **Wheels** | 42 mm Diameter Silicone / Soft Neoprene Tires | High coefficient of friction on dohyo surface |
| **Battery** | 2S LiPo (7.4V) or 3S LiPo (11.1V) (450 - 650 mAh, >30C) | High-discharge power source |
| **Logic Regulator**| LM1117-3.3V or Step-down Buck Converter (5V/3.3V) | Clean logic power for ESP32 and TB6612 VCC |
| **Chassis** | 500g Mini Sumo compliant (max 10 cm x 10 cm footprint) | Low center-of-gravity wedge chassis |

### Wiring Pinout Table

| TB6612FNG Pin | ESP32 Pin | Logic Signal | Functional Description |
|---|---|---|---|
| **AIN1** | GPIO 4 | Digital Output | Left Motor Direction Bit 1 |
| **AIN2** | GPIO 14 | Digital Output | Left Motor Direction Bit 2 |
| **PWMA** | GPIO 5 | LEDC PWM (5 kHz) | Left Motor Speed Control |
| **BIN1** | GPIO 18 | Digital Output | Right Motor Direction Bit 1 |
| **BIN2** | GPIO 19 | Digital Output | Right Motor Direction Bit 2 |
| **PWMB** | GPIO 21 | LEDC PWM (5 kHz) | Right Motor Speed Control |
| **STBY** | GPIO 33 | Digital Output | Standby Enable (Active HIGH) |
| **GND** | GND | Power Ground | Common system ground (ESP32, TB6612, Battery) |
| **VCC** | 3.3V or 5V | Logic Power | Driver internal logic rail |
| **VM** | LiPo (+) | Motor Power | Raw battery voltage rail (7.4V - 12V) |
| **AO1 / AO2** | Left Motor | Motor Output | Left motor terminals |
| **BO1 / BO2** | Right Motor | Motor Output | Right motor terminals |
| **STATUS LED** | GPIO 2 | Digital Output | Onboard status indicator LED |

> [!IMPORTANT]
> Always maintain a **single common ground (GND)** between the battery negative terminal, ESP32 ground pins, and TB6612 ground pins. Solder a 100nF ceramic capacitor across motor terminals to suppress brush noise that can disrupt Bluetooth reception.

---

## Gamepad Controller Mapping

```
                       [L1] Cycle Speed               [R1] Attack Mode (Turbo)
                       [L2] Tactic Modifier           [R2] Tactic Modifier
                              _=====_                  _=====_
                             / _____ \                / _____ \
                            +.-'_____'-.------------.-'_____'-.+
                           /   |     |  '.        .'  |  (Y) |   \
                          / ___| /^\ |___ \      / ___| (X) (B)|  \
                         / |      |      | ;    ; |            |   \
                        |  | <---   ---> | |    | |   (A)      |    |
                        |  |___   |   ___|/=======\|___________/    |
                        |      | \V/ |    |  PS   |                 |
                        |      |     |    |SELECT | START           |
                        |      '-----'    '-------'                 |
                        |             (L3)         (R3)             |
                        |           [Throttle]   [Steering]         |
                         \         [ Invert ]   [ Invert ]         /
                          \_______________________________________/
```

| Controller Input | Primary Action | Functional Details |
|---|---|---|
| **Left Stick Y (LY)** | **Throttle (Forward / Reverse)** | Pushing stick UP drives forward; pulling DOWN drives backward. Shaped with cubic expo curve. |
| **Right Stick RX (RX)** | **Steering (Turn Left / Right)** | Pushing stick RIGHT turns clockwise (right); pushing LEFT turns counter-clockwise (left). |
| **D-Pad (Up/Down/Left/Right)** | **Digital Driving Fallback** | Drives directly at the active speed limit if the analog sticks are centered. |
| **L1 (Left Bumper)** | **Cycle Speed Mode** | Toggles sequentially: **Mode 1 (Low)** → **Mode 2 (Normal)** → **Mode 3 (Fast)** → **Mode 1**. Saved to LittleFS. |
| **R1 (Right Bumper)** | **Attack Mode (Turbo)** | **Hold for instant 100% full-voltage power (PWM 255)**. Bypasses acceleration ramps for max ramming punch. |
| **L3 (Left Stick Click)** | **Toggle Throttle Inversion** | Flips forward and backward orientation. Back becomes front on dual-blade chassis. Saved to LittleFS. |
| **R3 (Right Stick Click)** | **Toggle Steering Inversion** | Flips left and right steering direction. Perfect for reverse-perspective driving. Saved to LittleFS. |
| **Circle / B Button** | **Emergency Handbrake** | Triggers instantaneous active dynamic short braking while held. Overrides current movement. |
| **Cross / A Button** | **Stop / Abort Tactic** | Immediately stops the robot or cancels any executing autonomous tactic. |
| **Triangle / Y Button** | **Toggle Anti-Wheelie Limiter** | Toggles acceleration ramp ON / OFF. Saved to LittleFS. |
| **L2 + R2** | **Launch Flank Right Tactic** | Default opening tactic (evade backward, angle 45°, flank opponent). |
| **L2 + R2 + D-Pad Right** | **Launch Flank Right Tactic** | Executes Flank Right gambit. |
| **L2 + R2 + D-Pad Left** | **Launch Flank Left Tactic** | Executes Flank Left gambit (mirrored). |
| **L2 + R2 + D-Pad Up** | **Launch Blitz Charge Tactic** | Full 100% power forward rush to knock opponent out before they pivot. |
| **L2 + R2 + D-Pad Down** | **Launch Juke & Strike Tactic** | Rapid 10cm retreat, 250ms pause to let opponent overcharge, 90° pivot strike. |
| **SELECT + START (3s)** | **Reset Bluetooth Pairing** | Hold Share + Options / Select + Start for 3 seconds in the pit to unpair and pair a new gamepad. |

---

## Driving Dynamics & Control Logic

### Exponential Stick Response Curve

Linear stick response can cause high-RPM gearmotors (600+ RPM) to feel hyper-sensitive and twitchy near the center deadzone. The firmware employs a cubic blending equation:

$$\text{Output} = (1.0 - \text{EXPO}) \cdot \text{Input} + \text{EXPO} \cdot \text{Input}^3$$

Where $\text{EXPO} = 0.20$. Small stick deflections yield soft, micro-fine aiming adjustments when lining up blade wedges against an opponent, while full deflection delivers 100% unfiltered maximum speed.

### Differential Arcade Drive Mixing

The firmware implements dual-axis arcade drive mixing with ratio preservation:

$$\text{Left Motor} = \text{Throttle} + \text{Steer}$$
$$\text{Right Motor} = \text{Throttle} - \text{Steer}$$

When steering at high throttle speeds where $\max(|\text{Left}|, |\text{Right}|) > \text{SpeedLimit}$, both channels are dynamically scaled:

$$\text{Left Motor} = \frac{\text{Left Motor} \cdot \text{SpeedLimit}}{\max(|\text{Left}|, |\text{Right}|)}, \quad \text{Right Motor} = \frac{\text{Right Motor} \cdot \text{SpeedLimit}}{\max(|\text{Left}|, |\text{Right}|)}$$

This ensures neither motor clips or saturates, maintaining the exact turning radius commanded by the operator.

### Dynamic Active Short Braking

The TB6612FNG driver truth table dictates that setting `IN1 = HIGH, IN2 = HIGH` shorts both motor terminals through the lower MOSFETs. When stopping or centering the stick:

- **Conventional libraries**: Call `drive(0)` which sets `IN1 = HIGH, IN2 = LOW, PWM = 0` (coasting/freewheeling). Momentum carries a 500g robot over the border line.
- **This firmware**: Calls `leftMotor_.brake()` and `rightMotor_.brake()`, producing immediate counter-electromotive braking force that halts rotation within milliseconds.

### Anti-Wheelie Asymmetric Acceleration Limiter

Sudden torque step-changes on high-traction silicone tires cause the sumobot front end to pitch up (*wheelie*), allowing the opponent's low wedge underneath:

- **Static Friction Kick (`START_MOTOR_ACCELERATE = 35`)**: Starts directly at the motor deadband threshold to prevent audible stall humming.
- **Acceleration Ramp (`MOTOR_ACCELERATION_MS = 140 ms`)**: Restricts throttle rate-of-change when accelerating from stop or speeding up.
- **Deceleration Ramp (`MOTOR_DECELERATION_MS = 80 ms`)**: Rapidly slows the chassis without forward pitching or bucking.
- **Attack Mode Bypass**: When R1 is held, all rate limiters are bypassed for instantaneous collision bursts.

### On-the-Fly Inversion (L3 & R3)

- **Throttle Inversion (L3)**: Inverts the sign of `throttle`. When the operator's robot is spun around with its tail facing the opponent, clicking L3 turns reverse into forward, allowing the rear scoop to charge immediately without rotating.
- **Steering Inversion (R3)**: Inverts the sign of `steer`. When watching the robot drive towards the driver from the opposite side of the ring, clicking R3 prevents cognitive disorientation by aligning left/right with the driver's perspective.

---

## Autonomous Opening Tactics

The firmware incorporates a non-blocking state machine in [`SumobotCheatsheet`](file:///home/dx4white/Documents/PlatformIO/Projects/SUMOBOT/include/sumobot_cheatsheet.h) to execute four tournament opening tactics:

```
[IDLE] --- Combo Triggered ---> [STEP_BACKWARD] ---> [STEP_TURN_1] ---> [STEP_DASH]
                                                                            |
[IDLE] <--- Safety Timeout / Stick Override <--- [STEP_FINAL_CHARGE] <--- [STEP_TURN_2]
```

### Tactic 1: Flank Right (`L2 + R2` or `L2 + R2 + D-Pad Right`)
1. **Reverse 5 cm**: Retreats at `CHEATSHEET_SPEED` to evade direct head-on collision.
2. **Turn Right 45°**: Rotates clockwise towards the flank.
3. **Dash Forward 15 cm**: Advances past the opponent's front line.
4. **Turn Left 135°**: Pivots inward to face the opponent's exposed flank or rear.
5. **Final Attack Charge**: Full speed rush for 1500 ms to push the opponent out.

### Tactic 2: Flank Left (`L2 + R2 + D-Pad Left`)
1. **Reverse 5 cm**: Evades straight frontal assault.
2. **Turn Left 45°**: Rotates counter-clockwise.
3. **Dash Forward 15 cm**: Advances along opponent's right flank.
4. **Turn Right 135°**: Pivots inward facing opponent's flank/rear.
5. **Final Attack Charge**: Full speed push.

### Tactic 3: Blitz Charge (`L2 + R2 + D-Pad Up`)
1. **Instant Full Power Rush**: Charges straight ahead at 100% PWM (`MAX_SPEED = 255`) for 1200 ms. Designed to bulldoze opponents before they can initiate turning or deployment maneuvers.

### Tactic 4: Juke & Strike (`L2 + R2 + D-Pad Down`)
1. **Fast Reverse 10 cm**: Rapidly pulls backward.
2. **Pause / Bait Hold (250 ms)**: Actively brakes and holds position, letting a charging opponent overshoot empty space.
3. **Pivot Right 90°**: Swings 90 degrees into the passing opponent's side.
4. **Flank Strike**: 100% power push for 1000 ms.

### Instant Operator Override & Failsafes

- **Stick Touch Takeover**: If the driver moves the left stick (`LY`) or right stick (`RX`) past the deadzone while any tactic is executing, the tactic is aborted instantly and manual driving authority is resumed with zero lag.
- **Emergency Button Abort**: Pressing **Cross (A)** or **Circle (B)** cancels any active tactic and activates active braking.
- **Final Charge Safety Timeout (`TACTIC_FINAL_CHARGE_MS = 1500 ms`)**: Guarantees that the final attack stage automatically terminates and halts the motors, preventing the robot from driving off the ring if the opponent dodges.

---

## Haptic & Visual Feedback System

### Controller Lightbar RGB LED

| Robot State | Lightbar Color | Hex / RGB | Functional Context |
|---|---|---|---|
| **Speed Mode 1 (Low)** | **Green** | `0, 255, 0` | Precision positioning / Low grip dohyo surface |
| **Speed Mode 2 (Normal)** | **Deep Sky Blue** | `0, 120, 255` | Standard combat mode |
| **Speed Mode 3 (Fast)** | **Gold / Yellow** | `255, 180, 0` | High-speed sweeping & flank navigation |
| **Attack Mode (Turbo)** | **Bright Red** | `255, 0, 0` | R1 held: 100% full voltage battery dump |
| **Autonomous Tactic Active** | **Purple / Magenta** | `200, 0, 255` | Pre-programmed gambit executing |
| **Handbrake Active** | **Amber / Orange** | `255, 80, 0` | Circle/B held: Active dynamic short brake |

### Dual Rumble Haptic Feedback

| Event | Rumble Pattern | Duration & Intensity |
|---|---|---|
| **Gamepad Connected** | Single Welcome Pulse | 250 ms @ 150 magnitude |
| **Mode 1 Selected** | Single Light Click | 100 ms @ Weak Motor |
| **Mode 2 Selected** | Double Medium Pulse | 180 ms @ Balanced Dual Motors |
| **Mode 3 Selected** | Triple Strong Pulse | 250 ms @ Heavy Dual Motors |
| **Throttle Invert (L3)** | Single Heavy Buzz | 250 ms @ Left Motor (Toggle ON) / 120 ms (Toggle OFF) |
| **Steering Invert (R3)** | Single Heavy Buzz | 250 ms @ Right Motor (Toggle ON) / 120 ms (Toggle OFF) |
| **Anti-Wheelie Toggled** | Confirmation Pulse | 150 ms (Enabled) / 300 ms (Disabled) |
| **Tactic Launched** | Tactical Rumble Burst | 200 ms @ Dual Motors |
| **Bluetooth Reset** | Long Warning Vibration | 600 ms @ Max Intensity |

### ESP32 Onboard LED (GPIO 2)

- **Slow Blinking (1 Hz - 500 ms)**: Waiting for Bluetooth gamepad connection.
- **Solid ON**: Gamepad connected, robot active and ready in manual mode.
- **Fast Blinking (10 Hz - 50 ms)**: Autonomous opening tactic in progress.

---

## LittleFS Configuration Persistence

All dynamic driver preferences modified via gamepad buttons are automatically committed to the onboard flash filesystem via **LittleFS** into [`/config.txt`](file:///home/dx4white/Documents/PlatformIO/Projects/SUMOBOT/include/sumobot_storage.h):

```text
# SUMOBOT 500G SETTINGS (LittleFS)
invert_throttle=1
invert_steering=0
speed_mode=2
accel_enabled=1
```

### Key Highlights
- **Instant Non-Volatile Commits**: When L3, R3, L1, or Y is pressed, the new state is saved within 5 ms.
- **Power-Cycle Immunity**: Before a match, you can test drive, set your desired speed mode and direction inversions, and switch the robot's power off. When powered on again inside the dohyo, all configurations are immediately restored.
- **Auto-Formatting**: If the partition is fresh or unformatted, LittleFS formats the 1.9 MB data partition automatically on boot.

---

## Configuration & Tuning Guide

All hardware and software operational parameters are centralized in [`include/sumobot_config.h`](file:///home/dx4white/Documents/PlatformIO/Projects/SUMOBOT/include/sumobot_config.h):

```cpp
namespace SumobotConfig {

// Motor Polarity (Set to -1 if gearbox or wiring is physically reversed)
constexpr int LEFT_MOTOR_DIR = 1;
constexpr int RIGHT_MOTOR_DIR = 1;

// Default Inversion States on startup
constexpr bool INVERT_THROTTLE_DEFAULT = false;
constexpr bool INVERT_STEERING_DEFAULT = false;

// Stick Filtering
constexpr int DEADZONE = 15;               // Analog deadband threshold
constexpr int CONTROLLER_AXIS_LIMIT = 512; // Bluepad32 axis max magnitude
constexpr float STICK_EXPO = 0.20f;        // 0.0 = linear, 0.20 = soft center

// Speed Profiles (PWM 0-255)
constexpr int SPEED_LOW = 130;             // Mode 1: Low
constexpr int SPEED_NORMAL = 180;          // Mode 2: Normal
constexpr int SPEED_FAST = 220;            // Mode 3: Fast
constexpr int MAX_SPEED = 255;             // Attack / Blitz Max

// Anti-Wheelie Acceleration Ramping
constexpr bool USE_ACCELERATE_DEFAULT = true;
constexpr int START_MOTOR_ACCELERATE = 35;        // Kick past static friction
constexpr unsigned long MOTOR_ACCELERATION_MS = 140; // Ramp-up duration (ms)
constexpr unsigned long MOTOR_DECELERATION_MS = 80;  // Ramp-down duration (ms)

// Autonomous Tactics Calibration (Floor friction & wheel diameter dependent)
constexpr unsigned long MS_PER_CM = 60;              // ms required to drive 1 cm
constexpr unsigned long MS_PER_DEGREE = 5;          // ms required to rotate 1 deg
constexpr unsigned long TACTIC_FINAL_CHARGE_MS = 1500; // Attack duration limit
}
```

### Tuning Steps for Competitions:
1. **Straight Line Drift**: If the robot veers slightly to one side, inspect wheel cleanliness and silicone tire diameter. Adjust PWM offsets if needed.
2. **Calibrating Distance (`MS_PER_CM`)**: Command a 20 cm movement. Measure actual travel with a ruler. Adjust `MS_PER_CM = (Measured_Time_ms) / 20`.
3. **Calibrating Turns (`MS_PER_DEGREE`)**: Command a 360° spin. If the robot turns 400°, decrease `MS_PER_DEGREE`. If it turns 320°, increase it.

---

## Controller Pairing Instructions

### DualShock 4 (PS4) / DualSense (PS5) / Xbox / Switch Pro

1. Power on the ESP32 sumobot.
2. Put your controller into pairing mode:
   - **PS4 DualShock 4**: Hold **SHARE + PS Button** until the lightbar double-flashes white rapidly.
   - **PS5 DualSense**: Hold **Create + PS Button** until the lightbar pulses blue/white rapidly.
   - **Xbox One / Series**: Press the **Xbox Button**, then hold the small **Sync Button** near the top port until the Xbox logo flashes fast.
   - **Nintendo Switch Pro**: Hold the small **Sync Button** next to the USB-C charging port.
3. The ESP32 running Bluepad32 will pair automatically within 2–5 seconds.
4. Upon connection, the controller will execute a confirmation rumble and the lightbar will turn **Deep Sky Blue**.
5. **Subsequent Reconnects**: Just tap the **PS / Xbox / Home** button once. The stored NVS keys will reconnect in <1 second!

### DualShock 3 (PS3) via USB Pairing Script

PS3 Sixaxis controllers require their internal master Bluetooth MAC address to be set to the ESP32's BD address:

1. Open PlatformIO Serial Monitor (`115200` baud) when booting the ESP32 to find the BD address:
   ```text
   [INIT] ESP32 Bluetooth BD Addr: 24:6F:28:B2:76:A2
   ```
2. Connect the PS3 controller to your computer using a Mini-USB cable.
3. Run the included pairing script:
   ```bash
   python3 sixaxispair.py --mac 24:6F:28:B2:76:A2
   ```
4. Unplug the USB cable and press the **PS Button**. The controller will connect directly to the robot.

### Pitstop Pairing Reset

If you need to switch controllers at a competition pitstop without reflashing the firmware:
- Hold **SELECT + START** (Share + Options on PS4) simultaneously for **3 seconds**.
- The controller will buzz for 600 ms, disconnect, and erase all stored Bluetooth bonding keys from NVS memory. The robot is now ready to pair with another controller.

---

## Build, Flash & Debug

### Prerequisites
- [PlatformIO IDE](https://platformio.org/) (VS Code Extension or standalone Core CLI)
- Python 3.x with USB drivers (CP210x or CH340 depending on your ESP32 board)

### Build and Upload Commands

```bash
# 1. Clone the repository
git clone https://github.com/your-username/SUMOBOT.git
cd SUMOBOT

# 2. Build the project firmware
~/.platformio/penv/bin/pio run

# 3. Flash firmware to ESP32 (Plug in USB cable)
~/.platformio/penv/bin/pio run --target upload

# 4. Open Serial Monitor (115200 Baud)
~/.platformio/penv/bin/pio device monitor
```

---

## Wokwi Simulation

The project includes Wokwi simulation configuration files ([`diagram.json`](file:///home/dx4white/Documents/PlatformIO/Projects/SUMOBOT/diagram.json) and [`wokwi.toml`](file:///home/dx4white/Documents/PlatformIO/Projects/SUMOBOT/wokwi.toml)):

1. Open the project in VS Code with the **Wokwi for VS Code** extension installed.
2. Build the firmware (`pio run`) to generate `.pio/build/esp32dev/firmware.elf`.
3. Open `diagram.json` and click **Start Simulation**.
4. The simulation visualizes the ESP32 DevKit, TB6612FNG custom chip, and dual DC motors with live PWM and direction responses.
> [!NOTE]
> Physical Bluetooth gamepads require real hardware testing on the ESP32, as virtual Bluetooth controllers are not supported in standard Wokwi.

---

## Project Structure & File Manifest

```
SUMOBOT/
├── platformio.ini              # PlatformIO environment, Bluepad32 framework, and flash settings
├── diagram.json                # Wokwi simulation wiring diagram (ESP32 + TB6612FNG + DC Motors)
├── wokwi.toml                  # Wokwi simulator ELF path and configuration
├── sixaxispair.py              # USB HID tool to pair DualShock 3 controllers to ESP32 MAC
├── include/
│   ├── sumobot_config.h        # Central pinout, speed limits, anti-wheelie & tactic calibration
│   ├── sumobot_motor.h         # Motor driver abstraction, active braking & telemetry
│   ├── sumobot_cheatsheet.h    # 4 opening tactics state machine engine
│   ├── sumobot_app.h           # Bluepad32 lifecycle, controller processing & feedback
│   └── sumobot_storage.h       # LittleFS configuration load/save interface
├── src/
│   ├── main.cpp                # Firmware entry point (setup and loop delegates to SumobotApp)
│   ├── sumobot_app.cpp         # Gamepad input normalization, mixing, and LED/rumble handlers
│   ├── sumobot_motor.cpp       # TB6612FNG control, dynamic braking, and acceleration ramps
│   ├── sumobot_cheatsheet.cpp  # State machine implementation for opening gambits
│   └── sumobot_storage.cpp     # LittleFS filesystem mounting and /config.txt parsing
├── lib/
│   └── SparkFun_TB6612FNG_Arduino_Library/ # Local Arduino TB6612 driver library
└── test/                       # Unit and functional testing directory
```

---

## Troubleshooting & FAQ

### 1. Motors hum but do not rotate at low speeds
- **Cause**: Static friction in the N20 gearbox is higher than the minimum commanded PWM.
- **Solution**: Increase `START_MOTOR_ACCELERATE` in [`sumobot_config.h`](file:///home/dx4white/Documents/PlatformIO/Projects/SUMOBOT/include/sumobot_config.h) from `35` to `45`–`50`.

### 2. Robot veers left when commanded to steer right
- **Cause**: Left/right motor wiring or direction polarity is inverted.
- **Solution**: Set `LEFT_MOTOR_DIR = -1` or `RIGHT_MOTOR_DIR = -1` in `sumobot_config.h`, or tap **R3** on your controller to invert steering.

### 3. ESP32 reboots during sudden acceleration or collisions
- **Cause**: Brownout detector triggered by motor stall current pulling battery voltage below 3.0V.
- **Solution**: Use a higher C-rating LiPo battery (e.g. 30C–50C), check battery charge, and add a 470µF electrolytic capacitor across the motor driver VM and GND pins.

### 4. Controller disconnects or will not pair
- **Cause**: Stale Bluetooth keys in NVS or controller battery is depleted.
- **Solution**: Charge the gamepad fully. Hold **SELECT + START** on the controller (or set `FORGET_KEYS_ON_BOOT = true` temporarily in `sumobot_config.h`) to clear pairing cache, then re-pair.

---

## License

This project is part of the **SUMOBOT 500G** competitive robotics platform. Released under the Apache 2.0 / MIT Open Source License.
