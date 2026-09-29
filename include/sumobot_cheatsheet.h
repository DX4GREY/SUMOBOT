#pragma once

#include <Arduino.h>

#include "sumobot_motor.h"

enum class SumobotTactic {
  NONE,
  FLANK_RIGHT,
  FLANK_LEFT,
  BLITZ_CHARGE,
  JUKE_BAIT
};

class SumobotCheatsheet {
 public:
  explicit SumobotCheatsheet(SumobotMotor& motor);

  void start(SumobotTactic tactic = SumobotTactic::FLANK_RIGHT);
  void stop();
  void update();
  bool isActive() const;
  SumobotTactic currentTactic() const;
  const char* currentTacticName() const;

 private:
  enum Step {
    IDLE,
    STEP_BACKWARD,
    STEP_TURN_1,
    STEP_DASH,
    STEP_TURN_2,
    STEP_FINAL_CHARGE,
    STEP_PAUSE
  };

  void advance(Step next, unsigned long duration, int left, int right,
               const char* label);

  SumobotMotor& motor_;
  SumobotTactic tactic_ = SumobotTactic::NONE;
  Step step_ = IDLE;
  unsigned long stepStarted_ = 0;
  unsigned long stepDuration_ = 0;
};
