#pragma once

#include <Arduino.h>
#include <Bluepad32.h>

#include "sumobot_cheatsheet.h"
#include "sumobot_motor.h"

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
  bool processCheatsheetButtons(uint16_t buttons);

  static SumobotApp* instance_;
  SumobotMotor motor_;
  SumobotCheatsheet cheatsheet_;
  ControllerPtr controllers_[BP32_MAX_GAMEPADS] = {};
  unsigned long lastBlink_ = 0;
  unsigned long lastControllerData_ = 0;
  bool turboMode_ = false;
  bool cheatComboWasDown_ = false;
};
