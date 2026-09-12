#pragma once

#include <Arduino.h>

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

constexpr int DEADZONE = 15;
constexpr int MAX_SPEED = 255;
constexpr int NORMAL_SPEED = 180;
constexpr unsigned long CONTROLLER_TIMEOUT_MS = 250;

// Motor specification. Replace these example values with the motor datasheet.
// RPM and torque must describe the gearbox output shaft if a gearbox is used.
constexpr float MOTOR_NOMINAL_VOLTAGE = 6.0f;
constexpr float MOTOR_NOMINAL_RPM = 300.0f;
constexpr float MOTOR_STALL_TORQUE_NM = 0.10f;
constexpr float MOTOR_SUPPLY_VOLTAGE = 6.0f;

// Time-based movement calibration. Use encoders later for exact distances/angles.
constexpr unsigned long MS_PER_CM = 60;
constexpr unsigned long MS_PER_DEGREE = 5;
constexpr int CHEATSHEET_SPEED = NORMAL_SPEED;

}
