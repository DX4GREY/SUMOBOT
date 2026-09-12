#include "sumobot_motor.h"

#include "sumobot_config.h"

namespace {

float pwmRatio(int command) {
  return static_cast<float>(abs(command)) / SumobotConfig::MAX_SPEED;
}

}

SumobotMotor::SumobotMotor()
    : leftMotor_(SumobotConfig::LEFT_IN1, SumobotConfig::LEFT_IN2,
                 SumobotConfig::LEFT_PWM, 1, SumobotConfig::STANDBY),
      rightMotor_(SumobotConfig::RIGHT_IN1, SumobotConfig::RIGHT_IN2,
                  SumobotConfig::RIGHT_PWM, 1, SumobotConfig::STANDBY) {}

void SumobotMotor::begin() {
  pinMode(SumobotConfig::STANDBY, OUTPUT);
  digitalWrite(SumobotConfig::STANDBY, HIGH);
  stop();
}

void SumobotMotor::drive(int left, int right) {
  left = constrain(left, -SumobotConfig::MAX_SPEED, SumobotConfig::MAX_SPEED);
  right = constrain(right, -SumobotConfig::MAX_SPEED, SumobotConfig::MAX_SPEED);

  leftMotor_.drive(left);
  rightMotor_.drive(right);
  stopped_ = left == 0 && right == 0;

  telemetry_.leftCommand = left;
  telemetry_.rightCommand = right;
  telemetry_.leftRpm = estimateRpm(left);
  telemetry_.rightRpm = estimateRpm(right);
  telemetry_.leftTorqueNm = estimateTorqueNm(left);
  telemetry_.rightTorqueNm = estimateTorqueNm(right);

  Serial.printf("[MOTOR] LEFT=%d RIGHT=%d | RPM L=%.1f R=%.1f | TORQUE L=%.3f R=%.3f Nm\n",
                left, right, telemetry_.leftRpm, telemetry_.rightRpm,
                telemetry_.leftTorqueNm, telemetry_.rightTorqueNm);
}

void SumobotMotor::stop() {
  if (stopped_) return;

  leftMotor_.drive(0);
  rightMotor_.drive(0);
  stopped_ = true;
  telemetry_ = {};
  Serial.println("[MOTOR] STOP");
}

bool SumobotMotor::isStopped() const {
  return stopped_;
}

const SumobotMotorTelemetry& SumobotMotor::telemetry() const {
  return telemetry_;
}

float SumobotMotor::estimateRpm(int command) {
  const float voltageRatio = SumobotConfig::MOTOR_SUPPLY_VOLTAGE /
                             SumobotConfig::MOTOR_NOMINAL_VOLTAGE;
  return pwmRatio(command) * SumobotConfig::MOTOR_NOMINAL_RPM * voltageRatio;
}

float SumobotMotor::estimateTorqueNm(int command) {
  return pwmRatio(command) * SumobotConfig::MOTOR_STALL_TORQUE_NM;
}
