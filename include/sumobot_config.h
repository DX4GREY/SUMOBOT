#pragma once

#include <Arduino.h>

// Keep real-time control paths quiet by default. Enable only while debugging
// on a bench, since Serial output can delay Bluetooth and PWM servicing.
#ifndef SUMOBOT_DEBUG_LOGGING
#define SUMOBOT_DEBUG_LOGGING 0
#endif

namespace SumobotConfig {

// TB6612FNG pin mapping.
constexpr int LEFT_IN1 = 4;
constexpr int LEFT_IN2 = 14;
constexpr int LEFT_PWM = 5;
constexpr int RIGHT_IN1 = 18;
constexpr int RIGHT_IN2 = 19;
constexpr int RIGHT_PWM = 21;
constexpr int STANDBY = 33;
constexpr int STATUS_LED = 2;

constexpr int DEADZONE = 20;
constexpr int CONTROLLER_AXIS_LIMIT = 512;
constexpr int MOTOR_TARGET_DEADBAND = 3;
constexpr int MAX_SPEED = 255;
constexpr int NORMAL_SPEED = 180;
constexpr unsigned long CONTROLLER_TIMEOUT_MS = 250;
constexpr bool USE_ACELERATE = true;
// Initial PWM when acceleration starts from a complete stop. The value is
// limited to the requested target in code, so it is safe for small commands.
// Set to 0 for a fully gradual start.
constexpr int START_MOTOR_ACELERATE = 10;

// Motor specification. Replace these example values with the motor datasheet.
// RPM and torque must describe the gearbox output shaft if a gearbox is used.
constexpr float ROBOT_MASS_KG = 0.474f;
constexpr float WHEEL_DIAMETER_M = 0.042f;
constexpr float SHAFT_TO_FRONT_M = 0.075f;
constexpr float MOTOR_NOMINAL_VOLTAGE = 12.0f;
constexpr float MOTOR_NOMINAL_RPM = 600.0f;
constexpr float MOTOR_STALL_TORQUE_NM = 4.10f;
constexpr float MOTOR_SUPPLY_VOLTAGE = 12.0f;

// Target chassis acceleration. MOTOR_ACCELERATION_MS is derived from the
// wheel circumference and nominal motor speed, so changing wheel diameter or
// motor RPM automatically retunes the ramp. The mass and shaft offset above
// document the current chassis; tipping also depends on the actual center of
// mass and tire grip, which are not known from the current measurements.
constexpr float TARGET_LINEAR_ACCELERATION_MPS2 = 5.0f;
constexpr float WHEEL_PI = 3.14159265359f;
constexpr float MAX_LINEAR_SPEED_MPS =
    (MOTOR_NOMINAL_RPM / 60.0f) * WHEEL_PI * WHEEL_DIAMETER_M;
constexpr unsigned long MOTOR_ACCELERATION_MS = static_cast<unsigned long>(
    (MAX_LINEAR_SPEED_MPS / TARGET_LINEAR_ACCELERATION_MPS2) * 1000.0f + 0.5f);

// Time-based movement calibration. Use encoders later for exact distances/angles.
constexpr unsigned long MS_PER_CM = 60;
constexpr unsigned long MS_PER_DEGREE = 5;
constexpr int CHEATSHEET_SPEED = NORMAL_SPEED;

}
