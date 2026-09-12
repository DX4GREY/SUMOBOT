#include "sumobot_motor.h"

#include "sumobot_config.h"

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
  Serial.printf("[MOTOR] LEFT=%d RIGHT=%d\n", left, right);
}

void SumobotMotor::stop() {
  if (stopped_) return;

  leftMotor_.drive(0);
  rightMotor_.drive(0);
  stopped_ = true;
  Serial.println("[MOTOR] STOP");
}

bool SumobotMotor::isStopped() const {
  return stopped_;
}
