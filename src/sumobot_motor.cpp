#include "sumobot_motor.h"

#include "sumobot_config.h"

namespace {

float pwmRatio(int command) {
  return static_cast<float>(abs(command)) / SumobotConfig::MAX_SPEED;
}

float lerp(float from, float to, float progress) {
  return from + (to - from) * progress;
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

  update();

  if (left == 0 && right == 0) {
    stop();
    return;
  }

  if (SumobotConfig::USE_ACELERATE && shouldAccelerate(left, right)) {
    if (!rampActive_ || rampTargetLeft_ != left || rampTargetRight_ != right) {
      rampStartLeft_ = appliedLeft_;
      rampStartRight_ = appliedRight_;
      rampTargetLeft_ = left;
      rampTargetRight_ = right;
      rampStartedAt_ = millis();
      rampActive_ = true;
    }
    refreshRamp();
  } else {
    rampActive_ = false;
    rampTargetLeft_ = left;
    rampTargetRight_ = right;
    applyOutputs(left, right);
  }
}

void SumobotMotor::update() {
  refreshRamp();
}

void SumobotMotor::applyOutputs(int left, int right) {
  appliedLeft_ = left;
  appliedRight_ = right;

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

void SumobotMotor::refreshRamp() {
  if (!rampActive_) return;

  const unsigned long elapsed = millis() - rampStartedAt_;
  const float progress = constrain(
      static_cast<float>(elapsed) / SumobotConfig::MOTOR_ACCELERATION_MS, 0.0f,
      1.0f);

  const int nextLeft = static_cast<int>(round(
      lerp(static_cast<float>(rampStartLeft_),
           static_cast<float>(rampTargetLeft_), progress)));
  const int nextRight = static_cast<int>(round(
      lerp(static_cast<float>(rampStartRight_),
           static_cast<float>(rampTargetRight_), progress)));

  applyOutputs(nextLeft, nextRight);

  if (progress >= 1.0f) {
    rampActive_ = false;
  }
}

bool SumobotMotor::shouldAccelerate(int left, int right) {
  if (left == 0 && right == 0) return false;
  if (left == 0 || right == 0) return true;
  return (left > 0 && right > 0) || (left < 0 && right < 0);
}

void SumobotMotor::stop() {
  if (stopped_) return;

  rampActive_ = false;
  rampStartLeft_ = 0;
  rampStartRight_ = 0;
  rampTargetLeft_ = 0;
  rampTargetRight_ = 0;
  leftMotor_.drive(0);
  rightMotor_.drive(0);
  appliedLeft_ = 0;
  appliedRight_ = 0;
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
