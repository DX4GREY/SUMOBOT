#include "sumobot_cheatsheet.h"

#include "sumobot_config.h"

SumobotCheatsheet::SumobotCheatsheet(SumobotMotor& motor) : motor_(motor) {}

bool SumobotCheatsheet::isActive() const {
  return step_ != IDLE;
}

SumobotTactic SumobotCheatsheet::currentTactic() const {
  return tactic_;
}

const char* SumobotCheatsheet::currentTacticName() const {
  switch (tactic_) {
    case SumobotTactic::FLANK_RIGHT:
      return "Flank Right";
    case SumobotTactic::FLANK_LEFT:
      return "Flank Left";
    case SumobotTactic::BLITZ_CHARGE:
      return "Blitz Charge";
    case SumobotTactic::JUKE_BAIT:
      return "Juke & Strike";
    case SumobotTactic::NONE:
    default:
      return "None";
  }
}

void SumobotCheatsheet::start(SumobotTactic tactic) {
  tactic_ = tactic;
  Serial.printf("[TACTIC] >>> LAUNCHING: %s <<<\n", currentTacticName());

  switch (tactic_) {
    case SumobotTactic::FLANK_RIGHT:
    case SumobotTactic::FLANK_LEFT:
      advance(STEP_BACKWARD, 5 * SumobotConfig::MS_PER_CM,
              -SumobotConfig::CHEATSHEET_SPEED,
              -SumobotConfig::CHEATSHEET_SPEED, "Step 1: Reverse 5cm");
      break;

    case SumobotTactic::BLITZ_CHARGE:
      advance(STEP_FINAL_CHARGE, 1200, SumobotConfig::MAX_SPEED,
              SumobotConfig::MAX_SPEED, "Blitz: 100% Full Power Charge");
      break;

    case SumobotTactic::JUKE_BAIT:
      advance(STEP_BACKWARD, 10 * SumobotConfig::MS_PER_CM,
              -SumobotConfig::CHEATSHEET_SPEED,
              -SumobotConfig::CHEATSHEET_SPEED, "Step 1: Fast Reverse 10cm");
      break;

    case SumobotTactic::NONE:
    default:
      stop();
      break;
  }
}

void SumobotCheatsheet::stop() {
  if (isActive()) {
    Serial.printf("[TACTIC] Ended: %s\n", currentTacticName());
  }
  step_ = IDLE;
  tactic_ = SumobotTactic::NONE;
  stepStarted_ = 0;
  stepDuration_ = 0;
  motor_.stop();
}

void SumobotCheatsheet::advance(Step next, unsigned long duration, int left,
                                int right, const char* label) {
  step_ = next;
  stepStarted_ = millis();
  stepDuration_ = duration;
  motor_.drive(left, right, true /*bypassRamp for crisp tactical timing*/);
  Serial.printf("[TACTIC] %s (%lu ms) [L=%d R=%d]\n", label, duration, left, right);
}

void SumobotCheatsheet::update() {
  if (!isActive()) return;

  const unsigned long elapsed = millis() - stepStarted_;
  if (elapsed < stepDuration_) return;

  // Execute state transitions for the active tactic
  switch (tactic_) {
    case SumobotTactic::FLANK_RIGHT:
      switch (step_) {
        case STEP_BACKWARD:
          // Turn right 45 degrees
          advance(STEP_TURN_1, 45 * SumobotConfig::MS_PER_DEGREE,
                  SumobotConfig::CHEATSHEET_SPEED,
                  -SumobotConfig::CHEATSHEET_SPEED, "Step 2: Turn Right 45deg");
          break;

        case STEP_TURN_1:
          // Dash forward 15 cm
          advance(STEP_DASH, 15 * SumobotConfig::MS_PER_CM,
                  SumobotConfig::CHEATSHEET_SPEED,
                  SumobotConfig::CHEATSHEET_SPEED, "Step 3: Dash Forward 15cm");
          break;

        case STEP_DASH:
          // Turn left 135 degrees into opponent flank/rear
          advance(STEP_TURN_2, 135 * SumobotConfig::MS_PER_DEGREE,
                  -SumobotConfig::CHEATSHEET_SPEED,
                  SumobotConfig::CHEATSHEET_SPEED, "Step 4: Turn Left 135deg");
          break;

        case STEP_TURN_2:
          // Final attack charge with safety timeout
          advance(STEP_FINAL_CHARGE, SumobotConfig::TACTIC_FINAL_CHARGE_MS,
                  SumobotConfig::MAX_SPEED, SumobotConfig::MAX_SPEED,
                  "Step 5: Final Attack Charge");
          break;

        case STEP_FINAL_CHARGE:
        default:
          stop();
          break;
      }
      break;

    case SumobotTactic::FLANK_LEFT:
      switch (step_) {
        case STEP_BACKWARD:
          // Turn left 45 degrees
          advance(STEP_TURN_1, 45 * SumobotConfig::MS_PER_DEGREE,
                  -SumobotConfig::CHEATSHEET_SPEED,
                  SumobotConfig::CHEATSHEET_SPEED, "Step 2: Turn Left 45deg");
          break;

        case STEP_TURN_1:
          // Dash forward 15 cm
          advance(STEP_DASH, 15 * SumobotConfig::MS_PER_CM,
                  SumobotConfig::CHEATSHEET_SPEED,
                  SumobotConfig::CHEATSHEET_SPEED, "Step 3: Dash Forward 15cm");
          break;

        case STEP_DASH:
          // Turn right 135 degrees into opponent flank/rear
          advance(STEP_TURN_2, 135 * SumobotConfig::MS_PER_DEGREE,
                  SumobotConfig::CHEATSHEET_SPEED,
                  -SumobotConfig::CHEATSHEET_SPEED, "Step 4: Turn Right 135deg");
          break;

        case STEP_TURN_2:
          // Final attack charge with safety timeout
          advance(STEP_FINAL_CHARGE, SumobotConfig::TACTIC_FINAL_CHARGE_MS,
                  SumobotConfig::MAX_SPEED, SumobotConfig::MAX_SPEED,
                  "Step 5: Final Attack Charge");
          break;

        case STEP_FINAL_CHARGE:
        default:
          stop();
          break;
      }
      break;

    case SumobotTactic::BLITZ_CHARGE:
      // Single-step full power charge completed
      stop();
      break;

    case SumobotTactic::JUKE_BAIT:
      switch (step_) {
        case STEP_BACKWARD:
          // Pause / bait opponent to miss and charge into empty space
          advance(STEP_PAUSE, 250, 0, 0, "Step 2: Pause / Bait Hold");
          break;

        case STEP_PAUSE:
          // Pivot 90 degrees right into passing opponent
          advance(STEP_TURN_1, 90 * SumobotConfig::MS_PER_DEGREE,
                  SumobotConfig::CHEATSHEET_SPEED,
                  -SumobotConfig::CHEATSHEET_SPEED, "Step 3: Pivot 90deg");
          break;

        case STEP_TURN_1:
          // Strike flank
          advance(STEP_FINAL_CHARGE, 1000, SumobotConfig::MAX_SPEED,
                  SumobotConfig::MAX_SPEED, "Step 4: Flank Strike");
          break;

        case STEP_FINAL_CHARGE:
        default:
          stop();
          break;
      }
      break;

    case SumobotTactic::NONE:
    default:
      stop();
      break;
  }
}
