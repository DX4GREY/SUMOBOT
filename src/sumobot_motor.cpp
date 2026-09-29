#include "sumobot_motor.h"

#include "sumobot_config.h"

namespace {

float pwmRatio(int command) {
  return static_cast<float>(abs(command)) / SumobotConfig::MAX_SPEED;
}

int moveToward(int current, int target, int step) {
  if (current < target) return min(current + step, target);
  if (current > target) return max(current - step, target);
  return current;
}

int initialRampOutput(int target) {
  const int initialMagnitude = min(
      abs(target), max(0, SumobotConfig::START_MOTOR_ACCELERATE));
  return target < 0 ? -initialMagnitude : initialMagnitude;
}

}  // namespace

SumobotMotor::SumobotMotor()
    : leftMotor_(SumobotConfig::LEFT_IN1, SumobotConfig::LEFT_IN2,
                 SumobotConfig::LEFT_PWM, SumobotConfig::LEFT_MOTOR_DIR,
                 SumobotConfig::STANDBY),
      rightMotor_(SumobotConfig::RIGHT_IN1, SumobotConfig::RIGHT_IN2,
                  SumobotConfig::RIGHT_PWM, SumobotConfig::RIGHT_MOTOR_DIR,
                  SumobotConfig::STANDBY),
      accelerationEnabled_(SumobotConfig::USE_ACCELERATE_DEFAULT) {}

void SumobotMotor::begin() {
  pinMode(SumobotConfig::STANDBY, OUTPUT);
  digitalWrite(SumobotConfig::STANDBY, HIGH);
  brake();
}

void SumobotMotor::drive(int left, int right, bool bypassRamp) {
  left = constrain(left, -SumobotConfig::MAX_SPEED, SumobotConfig::MAX_SPEED);
  right = constrain(right, -SumobotConfig::MAX_SPEED, SumobotConfig::MAX_SPEED);

  if (!accelerationEnabled_ || bypassRamp) {
    rampActive_ = false;
    rampTargetLeft_ = left;
    rampTargetRight_ = right;
    if (left == 0 && right == 0) {
      brake();
    } else {
      applyOutputs(left, right);
    }
    return;
  }

  // Already stopped and requested target is stop
  if (left == 0 && right == 0 && appliedLeft_ == 0 && appliedRight_ == 0) {
    brake();
    return;
  }

  const bool startingFromStop = (appliedLeft_ == 0 && appliedRight_ == 0);

  rampTargetLeft_ = left;
  rampTargetRight_ = right;

  if (startingFromStop && (left != 0 || right != 0)) {
    // Jump straight past the static gearbox friction deadband into immediate motion
    const int initLeft = (left != 0 && SumobotConfig::START_MOTOR_ACCELERATE > 0)
                             ? initialRampOutput(left)
                             : 0;
    const int initRight = (right != 0 && SumobotConfig::START_MOTOR_ACCELERATE > 0)
                              ? initialRampOutput(right)
                              : 0;
    applyOutputs(initLeft, initRight);
    rampLastUpdatedAt_ = millis();
    rampActive_ = true;
  } else if (!rampActive_) {
    rampLastUpdatedAt_ = millis();
    rampActive_ = true;
  }

  refreshRamp();
}

void SumobotMotor::update() {
  refreshRamp();
}

void SumobotMotor::applyOutputs(int left, int right) {
  if (left == appliedLeft_ && right == appliedRight_ && !stopped_) return;

  appliedLeft_ = left;
  appliedRight_ = right;

  // Active electrical dynamic braking when output is 0, drive when non-zero
  if (left == 0) {
    leftMotor_.brake();
  } else {
    leftMotor_.drive(left);
  }

  if (right == 0) {
    rightMotor_.brake();
  } else {
    rightMotor_.drive(right);
  }

  stopped_ = (left == 0 && right == 0);

  telemetry_.leftCommand = left;
  telemetry_.rightCommand = right;
  telemetry_.leftRpm = estimateRpm(left);
  telemetry_.rightRpm = estimateRpm(right);
  telemetry_.leftTorqueNm = estimateTorqueNm(left);
  telemetry_.rightTorqueNm = estimateTorqueNm(right);

#if SUMOBOT_DEBUG_LOGGING
  Serial.printf("[MOTOR] LEFT=%d RIGHT=%d | RPM L=%.1f R=%.1f | TORQUE L=%.3f R=%.3f Nm\n",
                left, right, telemetry_.leftRpm, telemetry_.rightRpm,
                telemetry_.leftTorqueNm, telemetry_.rightTorqueNm);
#endif
}

int SumobotMotor::calculateStep(int current, int target, unsigned long elapsed) {
  if (elapsed == 0) return 0;

  // Decelerating if target magnitude is less than current magnitude,
  // or if target has opposite sign (must decelerate towards 0 first).
  const bool isDecelerating = (abs(target) < abs(current)) ||
                              ((current > 0 && target < 0) || (current < 0 && target > 0));

  const unsigned long rampMs = isDecelerating
                                   ? SumobotConfig::MOTOR_DECELERATION_MS
                                   : SumobotConfig::MOTOR_ACCELERATION_MS;

  if (rampMs == 0) return SumobotConfig::MAX_SPEED;

  return max(1, static_cast<int>(
                    (static_cast<unsigned long>(SumobotConfig::MAX_SPEED) * elapsed) /
                    rampMs));
}

void SumobotMotor::refreshRamp() {
  if (!rampActive_) return;

  const unsigned long now = millis();
  const unsigned long elapsed = now - rampLastUpdatedAt_;
  if (elapsed == 0) return;
  rampLastUpdatedAt_ = now;

  const int stepLeft = calculateStep(appliedLeft_, rampTargetLeft_, elapsed);
  const int stepRight = calculateStep(appliedRight_, rampTargetRight_, elapsed);

  const int nextLeft = moveToward(appliedLeft_, rampTargetLeft_, stepLeft);
  const int nextRight = moveToward(appliedRight_, rampTargetRight_, stepRight);

  applyOutputs(nextLeft, nextRight);

  if (nextLeft == rampTargetLeft_ && nextRight == rampTargetRight_) {
    rampActive_ = false;
    if (nextLeft == 0 && nextRight == 0) {
      brake();
    }
  }
}

void SumobotMotor::stop() {
  brake();
}

void SumobotMotor::brake() {
  rampActive_ = false;
  rampTargetLeft_ = 0;
  rampTargetRight_ = 0;
  leftMotor_.brake();
  rightMotor_.brake();
  appliedLeft_ = 0;
  appliedRight_ = 0;
  stopped_ = true;
  telemetry_ = {};

#if SUMOBOT_DEBUG_LOGGING
  Serial.println("[MOTOR] ACTIVE BRAKE");
#endif
}

bool SumobotMotor::isStopped() const {
  return stopped_;
}

const SumobotMotorTelemetry& SumobotMotor::telemetry() const {
  return telemetry_;
}

void SumobotMotor::setAccelerationEnabled(bool enabled) {
  accelerationEnabled_ = enabled;
  if (!accelerationEnabled_) {
    rampActive_ = false;
  }
}

bool SumobotMotor::isAccelerationEnabled() const {
  return accelerationEnabled_;
}

float SumobotMotor::estimateRpm(int command) {
  const float voltageRatio = SumobotConfig::MOTOR_SUPPLY_VOLTAGE /
                             SumobotConfig::MOTOR_NOMINAL_VOLTAGE;
  return pwmRatio(command) * SumobotConfig::MOTOR_NOMINAL_RPM * voltageRatio;
}

float SumobotMotor::estimateTorqueNm(int command) {
  return pwmRatio(command) * SumobotConfig::MOTOR_STALL_TORQUE_NM;
}
