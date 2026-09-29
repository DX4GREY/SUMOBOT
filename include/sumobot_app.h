#pragma once

#include <Arduino.h>
#include <Bluepad32.h>

#include "sumobot_cheatsheet.h"
#include "sumobot_config.h"
#include "sumobot_motor.h"
#include "sumobot_storage.h"

class SumobotApp {
 public:
  SumobotApp();

  void begin();
  void update();

 private:
  static void onConnectedController(ControllerPtr controller);
  static void onDisconnectedController(ControllerPtr controller);

  void connected(ControllerPtr controller);
  void disconnected(ControllerPtr controller);
  void processControllers();
  void processGamepad(ControllerPtr controller);
  bool hasConnectedController() const;
  bool checkCheatsheetCombos(ControllerPtr controller, uint16_t buttons, uint8_t dpad);
  void updateControllerFeedback(ControllerPtr controller);
  void updateControllerLed(ControllerPtr controller, uint8_t r, uint8_t g, uint8_t b);
  void playRumble(ControllerPtr controller, uint16_t durationMs, uint8_t weak, uint8_t strong);
  void setSpeedMode(int mode, ControllerPtr controller);
  void saveCurrentSettings();

  enum SpeedMode {
    SPEED_MODE_LOW = 1,
    SPEED_MODE_NORMAL = 2,
    SPEED_MODE_FAST = 3
  };

  static SumobotApp* instance_;
  SumobotMotor motor_;
  SumobotCheatsheet cheatsheet_;
  ControllerPtr controllers_[BP32_MAX_GAMEPADS] = {};
  unsigned long lastBlink_ = 0;
  unsigned long lastControllerData_ = 0;
  unsigned long pairResetPressStart_ = 0;

  SpeedMode speedMode_ = SPEED_MODE_NORMAL;
  bool invertThrottle_ = SumobotConfig::INVERT_THROTTLE_DEFAULT;
  bool invertSteering_ = SumobotConfig::INVERT_STEERING_DEFAULT;
  bool l1WasPressed_ = false;
  bool yWasPressed_ = false;
  bool l3WasPressed_ = false;
  bool r3WasPressed_ = false;
  bool attackMode_ = false;
  bool handbrakeActive_ = false;
  bool cheatComboWasDown_ = false;

  uint8_t lastLedR_ = 255;
  uint8_t lastLedG_ = 255;
  uint8_t lastLedB_ = 255;
};
