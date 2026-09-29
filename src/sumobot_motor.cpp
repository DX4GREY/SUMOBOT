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
      abs(target), max(0, SumobotConfig::START_MOTOR_ACELERATE));
  return target < 0 ? -initialMagnitude : initialMagnitude;
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

  if (left == 0 && right == 0) {
    stop();
    return;
  }

  if (SumobotConfig::USE_ACELERATE && shouldAccelerate(left, right)) {
    if (!rampActive_) {
      const bool startingFromStop = appliedLeft_ == 0 && appliedRight_ == 0;
      if (startingFromStop && SumobotConfig::START_MOTOR_ACELERATE > 0) {
        applyOutputs(initialRampOutput(left), initialRampOutput(right));
      }
      rampLastUpdatedAt_ = millis();
      rampActive_ = true;
    }
    // Update the target without restarting progress. Ignore tiny command
    // changes to prevent stick noise from making the chassis twitch.
    if (abs(left - rampTargetLeft_) >= SumobotConfig::MOTOR_TARGET_DEADBAND) {
      rampTargetLeft_ = left;
    }
    if (abs(right - rampTargetRight_) >=
        SumobotConfig::MOTOR_TARGET_DEADBAND) {
      rampTargetRight_ = right;
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
  if (left == appliedLeft_ && right == appliedRight_) return;

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

#if SUMOBOT_DEBUG_LOGGING
  Serial.printf("[MOTOR] LEFT=%d RIGHT=%d | RPM L=%.1f R=%.1f | TORQUE L=%.3f R=%.3f Nm\n",
                left, right, telemetry_.leftRpm, telemetry_.rightRpm,
                telemetry_.leftTorqueNm, telemetry_.rightTorqueNm);
#endif
}

void SumobotMotor::refreshRamp() {
  if (!rampActive_) return;

  const unsigned long now = millis();
  const unsigned long elapsed = now - rampLastUpdatedAt_;
  if (elapsed == 0) return;

  const int step = SumobotConfig::MOTOR_ACCELERATION_MS == 0
                       ? SumobotConfig::MAX_SPEED
                       : max(1, static_cast<int>(
                                    (static_cast<unsigned long>(
                                         SumobotConfig::MAX_SPEED) * elapsed) /
                                    SumobotConfig::MOTOR_ACCELERATION_MS));
  rampLastUpdatedAt_ = now;

  const int nextLeft = moveToward(appliedLeft_, rampTargetLeft_, step);
  const int nextRight = moveToward(appliedRight_, rampTargetRight_, step);

  applyOutputs(nextLeft, nextRight);

  if (nextLeft == rampTargetLeft_ && nextRight == rampTargetRight_) {
    rampActive_ = false;
  }
}

bool SumobotMotor::shouldAccelerate(int left, int right) {
  if (left == 0 && right == 0) return false;
  if (left == 0 || right == 0) return true;
  return (left > 0 && right > 0) || (left < 0 && right < 0);
}

void SumobotMotor::stop() {
  const bool alreadyStopped = stopped_ && !rampActive_ && appliedLeft_ == 0 &&
                              appliedRight_ == 0;

  rampActive_ = false;
  rampTargetLeft_ = 0;
  rampTargetRight_ = 0;
  if (!alreadyStopped) {
    leftMotor_.drive(0);
    rightMotor_.drive(0);
  }
  appliedLeft_ = 0;
  appliedRight_ = 0;
  stopped_ = true;
  telemetry_ = {};
  if (!alreadyStopped) {
#if SUMOBOT_DEBUG_LOGGING
    Serial.println("[MOTOR] STOP");
#endif
  }
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
