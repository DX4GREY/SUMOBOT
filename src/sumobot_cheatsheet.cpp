#include "sumobot_cheatsheet.h"

#include "sumobot_config.h"

SumobotCheatsheet::SumobotCheatsheet(SumobotMotor& motor) : motor_(motor) {}

bool SumobotCheatsheet::isActive() const {
  return step_ != IDLE;
}

void SumobotCheatsheet::start() {
  step_ = BACKWARD_5CM;
  stepStarted_ = millis();
  motor_.drive(-SumobotConfig::CHEATSHEET_SPEED,
               -SumobotConfig::CHEATSHEET_SPEED);
  Serial.println("[CHEATSHEET] Start: backward 5 cm");
}

void SumobotCheatsheet::stop() {
  if (isActive()) Serial.println("[CHEATSHEET] Stopped");
  step_ = IDLE;
  motor_.stop();
}

void SumobotCheatsheet::advance(Step next, unsigned long duration, int left,
                                int right, const char* label) {
  step_ = next;
  stepStarted_ = millis();
  motor_.drive(left, right);
  Serial.printf("[CHEATSHEET] %s (%lu ms)\n", label, duration);
}

void SumobotCheatsheet::update() {
  if (!isActive()) return;

  const unsigned long elapsed = millis() - stepStarted_;
  switch (step_) {
    case BACKWARD_5CM:
      if (elapsed >= 5 * SumobotConfig::MS_PER_CM) {
        advance(RIGHT_45DEG, 45 * SumobotConfig::MS_PER_DEGREE,
                SumobotConfig::CHEATSHEET_SPEED,
                -SumobotConfig::CHEATSHEET_SPEED, "right 45 degrees");
      }
      break;

    case RIGHT_45DEG:
      if (elapsed >= 45 * SumobotConfig::MS_PER_DEGREE) {
        advance(FORWARD_15CM, 15 * SumobotConfig::MS_PER_CM,
                SumobotConfig::CHEATSHEET_SPEED,
                SumobotConfig::CHEATSHEET_SPEED, "forward 15 cm");
      }
      break;

    case FORWARD_15CM:
      if (elapsed >= 15 * SumobotConfig::MS_PER_CM) {
        advance(LEFT_135DEG, 135 * SumobotConfig::MS_PER_DEGREE,
                -SumobotConfig::CHEATSHEET_SPEED,
                SumobotConfig::CHEATSHEET_SPEED, "left 135 degrees");
      }
      break;

    case LEFT_135DEG:
      if (elapsed >= 135 * SumobotConfig::MS_PER_DEGREE) {
        step_ = FORWARD_UNTIL_X;
        stepStarted_ = millis();
        motor_.drive(SumobotConfig::CHEATSHEET_SPEED,
                     SumobotConfig::CHEATSHEET_SPEED);
        Serial.println("[CHEATSHEET] Forward until X");
      }
      break;

    case FORWARD_UNTIL_X:
    case IDLE:
      break;
  }
}
