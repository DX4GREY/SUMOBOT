#pragma once

#include <Arduino.h>

// Keep real-time control paths quiet by default. Enable only while debugging
// on a bench, since Serial output can delay Bluetooth and PWM servicing.
#ifndef SUMOBOT_DEBUG_LOGGING
#define SUMOBOT_DEBUG_LOGGING 0
#endif

namespace SumobotConfig {

// ================= TB6612FNG PIN MAPPING =================
constexpr int LEFT_IN1 = 4;
constexpr int LEFT_IN2 = 14;
constexpr int LEFT_PWM = 5;
constexpr int RIGHT_IN1 = 18;
constexpr int RIGHT_IN2 = 19;
constexpr int RIGHT_PWM = 21;
constexpr int STANDBY = 33;
constexpr int STATUS_LED = 2;

// ================= MOTOR POLARITY =================
// Set to 1 for standard direction, -1 to invert if a motor is mounted backwards.
constexpr int LEFT_MOTOR_DIR = 1;
constexpr int RIGHT_MOTOR_DIR = 1;

// Default inversion states on startup (can be toggled on-the-fly via L3 and R3)
constexpr bool INVERT_THROTTLE_DEFAULT = false;
constexpr bool INVERT_STEERING_DEFAULT = false;

// ================= CONTROLLER THRESHOLDS =================
constexpr int DEADZONE = 15;
constexpr int CONTROLLER_AXIS_LIMIT = 512;
// Exponential response curve (0.0 = linear, 0.20 = soft center for fine aim)
constexpr float STICK_EXPO = 0.20f;

// ================= SPEED PROFILES (PWM 0-255) =================
constexpr int SPEED_LOW = 130;       // Mode 1: Rendah (Precision / Low Grip)
constexpr int SPEED_NORMAL = 180;    // Mode 2: Normal (Combat standard)
constexpr int SPEED_FAST = 220;      // Mode 3: Cepat (Fast search & flank)
constexpr int MAX_SPEED = 255;       // Mode Serang / Turbo (R1 held) / Blitz

// Legacy alias
constexpr int NORMAL_SPEED = SPEED_NORMAL;

// ================= SAFETY & TIMEOUTS =================
// Maximum time without controller data packet before emergency auto-stop (ms)
constexpr unsigned long CONTROLLER_TIMEOUT_MS = 500;

// Set to true to erase paired Bluetooth keys on every reboot.
// Default false allows paired gamepads (PS4/PS5/Xbox/Switch) to reconnect instantly on power-on!
constexpr bool FORGET_KEYS_ON_BOOT = false;

// ================= ACCELERATION & ANTI-WHEELIE =================
// Default state for acceleration ramping.
// Smooths sudden acceleration so the front scoop stays flat on the ring.
// Can be toggled on-the-fly via Gamepad (Triangle / Y button).
constexpr bool USE_ACCELERATE_DEFAULT = true;

// Initial PWM jump when starting from stop to break gearbox static friction
constexpr int START_MOTOR_ACCELERATE = 35;

// Acceleration ramp duration in milliseconds (anti-wheelie)
constexpr unsigned long MOTOR_ACCELERATION_MS = 140;

// Deceleration ramp duration in milliseconds (controlled quick stop)
constexpr unsigned long MOTOR_DECELERATION_MS = 80;

// ================= 500G SUMOBOT SPECIFICATIONS =================
constexpr float ROBOT_MASS_KG = 0.474f;
constexpr float WHEEL_DIAMETER_M = 0.042f;
constexpr float SHAFT_TO_FRONT_M = 0.075f;
constexpr float MOTOR_NOMINAL_VOLTAGE = 12.0f;
constexpr float MOTOR_NOMINAL_RPM = 600.0f;
constexpr float MOTOR_STALL_TORQUE_NM = 4.10f;
constexpr float MOTOR_SUPPLY_VOLTAGE = 12.0f;

// ================= AUTONOMOUS TACTICS CALIBRATION =================
// Time-based movement calibration (tune based on floor traction & battery voltage)
constexpr unsigned long MS_PER_CM = 60;
constexpr unsigned long MS_PER_DEGREE = 5;
constexpr int CHEATSHEET_SPEED = SPEED_NORMAL;
// Maximum duration of the final attack charge before automatic return to idle (ms)
constexpr unsigned long TACTIC_FINAL_CHARGE_MS = 1500;

}  // namespace SumobotConfig
