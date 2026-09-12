#pragma once

#include <Arduino.h>

#include "sumobot_motor.h"

class SumobotCheatsheet {
 public:
  explicit SumobotCheatsheet(SumobotMotor& motor);

  void start();
  void stop();
  void update();
  bool isActive() const;

 private:
  enum Step {
    IDLE,
    BACKWARD_5CM,
    RIGHT_45DEG,
    FORWARD_15CM,
    LEFT_135DEG,
    FORWARD_UNTIL_X
  };

  void advance(Step next, unsigned long duration, int left, int right,
               const char* label);

  SumobotMotor& motor_;
  Step step_ = IDLE;
  unsigned long stepStarted_ = 0;
};
