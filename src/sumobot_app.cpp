#include "sumobot_app.h"

#include "sumobot_config.h"

namespace {

int normalizeAxis(int value) {
  value = constrain(value, -SumobotConfig::CONTROLLER_AXIS_LIMIT,
                    SumobotConfig::CONTROLLER_AXIS_LIMIT);
  const int magnitude = abs(value);
  if (magnitude <= SumobotConfig::DEADZONE) return 0;

  const int mapped = map(magnitude, SumobotConfig::DEADZONE,
                         SumobotConfig::CONTROLLER_AXIS_LIMIT, 0,
                         SumobotConfig::CONTROLLER_AXIS_LIMIT);

  // Subtle exponential response curve for precision control near center
  const float norm = static_cast<float>(mapped) / SumobotConfig::CONTROLLER_AXIS_LIMIT;
  const float curved = (1.0f - SumobotConfig::STICK_EXPO) * norm +
                       SumobotConfig::STICK_EXPO * (norm * norm * norm);
  const int finalMag = static_cast<int>(round(curved * SumobotConfig::CONTROLLER_AXIS_LIMIT));

  return value < 0 ? -finalMag : finalMag;
}

}  // namespace

SumobotApp* SumobotApp::instance_ = nullptr;

SumobotApp::SumobotApp() : cheatsheet_(motor_) {}

void SumobotApp::begin() {
  instance_ = this;
  Serial.begin(115200);
  delay(100);
  Serial.println("\n========================================");
  Serial.println("       SUMOBOT 500G RC FRAMEWORK        ");
  Serial.println("========================================");

  pinMode(SumobotConfig::STATUS_LED, OUTPUT);
  digitalWrite(SumobotConfig::STATUS_LED, LOW);
  motor_.begin();

  String firmwareVersion = BP32.firmwareVersion();
  Serial.printf("[INIT] Bluepad32 Firmware: %s\n", firmwareVersion.c_str());
  const uint8_t* address = BP32.localBdAddress();
  Serial.printf("[INIT] ESP32 Bluetooth BD Addr: %02X:%02X:%02X:%02X:%02X:%02X\n",
                address[0], address[1], address[2], address[3], address[4],
                address[5]);

  BP32.setup(&SumobotApp::onConnectedController,
             &SumobotApp::onDisconnectedController);
  BP32.enableVirtualDevice(false);

  if (SumobotConfig::FORGET_KEYS_ON_BOOT) {
    BP32.forgetBluetoothKeys();
    Serial.println("[INIT] Cleared Bluetooth keys (FORGET_KEYS_ON_BOOT=true)");
  } else {
    Serial.println("[INIT] Bluetooth bonding keys preserved for fast auto-reconnect");
  }

  // Muat konfigurasi tersimpan dari LittleFS
  SumobotSettings settings;
  settings.invertThrottle = SumobotConfig::INVERT_THROTTLE_DEFAULT;
  settings.invertSteering = SumobotConfig::INVERT_STEERING_DEFAULT;
  settings.speedMode = static_cast<uint8_t>(SumobotConfig::SPEED_NORMAL);
  settings.accelerationEnabled = SumobotConfig::USE_ACCELERATE_DEFAULT;

  if (SumobotStorage::load(settings)) {
    invertThrottle_ = settings.invertThrottle;
    invertSteering_ = settings.invertSteering;
    speedMode_ = static_cast<SpeedMode>(settings.speedMode);
    motor_.setAccelerationEnabled(settings.accelerationEnabled);
  }

  Serial.println("[INIT] System Ready. Waiting for controller...");
  Serial.println("[HINT] Klik L3 = Inversi Maju/Mundur | Klik R3 = Inversi Belok (Disimpan ke LittleFS)");
  Serial.println("[HINT] Tahan SELECT+START (Share+Options) 3s untuk reset pairing Bluetooth.");
}

void SumobotApp::saveCurrentSettings() {
  SumobotSettings settings;
  settings.invertThrottle = invertThrottle_;
  settings.invertSteering = invertSteering_;
  settings.speedMode = static_cast<uint8_t>(speedMode_);
  settings.accelerationEnabled = motor_.isAccelerationEnabled();
  SumobotStorage::save(settings);
}


void SumobotApp::update() {
  if (BP32.update()) processControllers();

  motor_.update();

  // Failsafe: Stop motors if controller communication is lost
  if (hasConnectedController() &&
      millis() - lastControllerData_ > SumobotConfig::CONTROLLER_TIMEOUT_MS &&
      !cheatsheet_.isActive()) {
    motor_.stop();
  }

  cheatsheet_.update();

  // Status LED logic:
  // - Fast blinking (100ms) when autonomous tactic is executing
  // - Slow blinking (500ms) when waiting for controller connection
  // - Solid ON when connected and ready
  const unsigned long blinkInterval = cheatsheet_.isActive() ? 100 : 500;
  if (millis() - lastBlink_ > blinkInterval) {
    lastBlink_ = millis();
    if (!hasConnectedController()) {
      digitalWrite(SumobotConfig::STATUS_LED,
                   !digitalRead(SumobotConfig::STATUS_LED));
    } else if (cheatsheet_.isActive()) {
      digitalWrite(SumobotConfig::STATUS_LED,
                   !digitalRead(SumobotConfig::STATUS_LED));
    } else {
      digitalWrite(SumobotConfig::STATUS_LED, HIGH);
    }
  }

  delay(1);
}

void SumobotApp::onConnectedController(ControllerPtr controller) {
  if (instance_) instance_->connected(controller);
}

void SumobotApp::onDisconnectedController(ControllerPtr controller) {
  if (instance_) instance_->disconnected(controller);
}

void SumobotApp::connected(ControllerPtr controller) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (!controllers_[i]) {
      controllers_[i] = controller;
      Serial.printf("[BP32] Controller connected at slot [%d]\n", i);
      ControllerProperties properties = controller->getProperties();
      Serial.printf("[BP32] Model=%s VID=0x%04x PID=0x%04x Battery=%d\n",
                    controller->getModelName().c_str(), properties.vendor_id,
                    properties.product_id, controller->battery());
      digitalWrite(SumobotConfig::STATUS_LED, HIGH);

      // Welcome vibration
      playRumble(controller, 250, 150, 150);

      // Initialize LED feedback for default Normal mode
      updateControllerFeedback(controller);
      return;
    }
  }
  Serial.println("[BP32] Warning: Max controllers reached");
}

void SumobotApp::disconnected(ControllerPtr controller) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (controllers_[i] == controller) {
      controllers_[i] = nullptr;
      Serial.printf("[BP32] Controller disconnected from slot [%d]\n", i);
      digitalWrite(SumobotConfig::STATUS_LED, LOW);
      cheatsheet_.stop();
      motor_.stop();
      return;
    }
  }
}

bool SumobotApp::hasConnectedController() const {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    if (controllers_[i] && controllers_[i]->isConnected()) return true;
  }
  return false;
}

void SumobotApp::updateControllerLed(ControllerPtr controller, uint8_t r, uint8_t g, uint8_t b) {
  if (controller && (r != lastLedR_ || g != lastLedG_ || b != lastLedB_)) {
    lastLedR_ = r;
    lastLedG_ = g;
    lastLedB_ = b;
    controller->setColorLED(r, g, b);
  }
}

void SumobotApp::playRumble(ControllerPtr controller, uint16_t durationMs, uint8_t weak, uint8_t strong) {
  if (controller) {
    controller->playDualRumble(0, durationMs, weak, strong);
  }
}

void SumobotApp::setSpeedMode(int mode, ControllerPtr controller) {
  speedMode_ = static_cast<SpeedMode>(mode);
  switch (speedMode_) {
    case SPEED_MODE_LOW:
      Serial.printf("[SPEED] Mode 1: LOW (PWM %d) - Precision\n", SumobotConfig::SPEED_LOW);
      playRumble(controller, 100, 120, 0);
      break;
    case SPEED_MODE_NORMAL:
      Serial.printf("[SPEED] Mode 2: NORMAL (PWM %d) - Balanced Combat\n", SumobotConfig::SPEED_NORMAL);
      playRumble(controller, 180, 160, 160);
      break;
    case SPEED_MODE_FAST:
      Serial.printf("[SPEED] Mode 3: FAST (PWM %d) - High Speed Flank\n", SumobotConfig::SPEED_FAST);
      playRumble(controller, 250, 220, 220);
      break;
  }
  updateControllerFeedback(controller);
}

void SumobotApp::updateControllerFeedback(ControllerPtr controller) {
  if (!controller) return;

  if (cheatsheet_.isActive()) {
    // Magenta / Purple for autonomous tactical maneuvers
    updateControllerLed(controller, 200, 0, 255);
  } else if (handbrakeActive_) {
    // Amber / Warning Orange for handbrake
    updateControllerLed(controller, 255, 80, 0);
  } else if (attackMode_) {
    // Bright Red for Attack / Turbo Mode
    updateControllerLed(controller, 255, 0, 0);
  } else {
    // Standard speed mode colors
    switch (speedMode_) {
      case SPEED_MODE_LOW:
        // Green
        updateControllerLed(controller, 0, 255, 0);
        break;
      case SPEED_MODE_NORMAL:
        // Sky Blue
        updateControllerLed(controller, 0, 120, 255);
        break;
      case SPEED_MODE_FAST:
        // Gold / Yellow
        updateControllerLed(controller, 255, 180, 0);
        break;
    }
  }
}

bool SumobotApp::checkCheatsheetCombos(ControllerPtr controller, uint16_t buttons, uint8_t dpad) {
  // Opening tactic combo: L2 + R2 (Both triggers held)
  const bool comboPressed = (buttons & BUTTON_TRIGGER_L) && (buttons & BUTTON_TRIGGER_R);

  if (comboPressed && !cheatComboWasDown_ && !cheatsheet_.isActive()) {
    if (dpad & DPAD_UP) {
      cheatsheet_.start(SumobotTactic::BLITZ_CHARGE);
    } else if (dpad & DPAD_DOWN) {
      cheatsheet_.start(SumobotTactic::JUKE_BAIT);
    } else if (dpad & DPAD_LEFT) {
      cheatsheet_.start(SumobotTactic::FLANK_LEFT);
    } else if (dpad & DPAD_RIGHT) {
      cheatsheet_.start(SumobotTactic::FLANK_RIGHT);
    } else {
      cheatsheet_.start(SumobotTactic::FLANK_RIGHT);  // Default tactic
    }
    playRumble(controller, 200, 180, 180);
  }
  cheatComboWasDown_ = comboPressed;
  return cheatsheet_.isActive();
}

void SumobotApp::processGamepad(ControllerPtr controller) {
  const uint16_t buttons = controller->buttons();
  const uint8_t dpad = controller->dpad();

  // Utility: Tahan SELECT + START (Share + Options) selama 3 detik di pit untuk hapus pairing
  const bool pairResetPressed = (controller->miscSelect() && controller->miscStart());
  if (pairResetPressed) {
    if (pairResetPressStart_ == 0) {
      pairResetPressStart_ = millis();
    } else if (millis() - pairResetPressStart_ > 3000) {
      Serial.println("[BP32] Resetting Bluetooth bonding keys upon user request (SELECT+START)...");
      BP32.forgetBluetoothKeys();
      playRumble(controller, 600, 255, 255);
      controller->disconnect();
      pairResetPressStart_ = 0;
      return;
    }
  } else {
    pairResetPressStart_ = 0;
  }

  // Klik L3: Toggle Inversi Kemudi Maju / Mundur (Throttle Inversion)
  const bool l3Pressed = (buttons & BUTTON_THUMB_L);
  if (l3Pressed && !l3WasPressed_) {
    invertThrottle_ = !invertThrottle_;
    if (invertThrottle_) {
      Serial.println("[INVERT] Kemudi Maju/Mundur: INVERTED (Maju <-> Mundur Terbalik)");
      playRumble(controller, 250, 180, 0);
    } else {
      Serial.println("[INVERT] Kemudi Maju/Mundur: NORMAL");
      playRumble(controller, 120, 100, 0);
    }
    saveCurrentSettings();
  }
  l3WasPressed_ = l3Pressed;

  // Klik R3: Toggle Inversi Kemudi Belok (Steering Inversion)
  const bool r3Pressed = (buttons & BUTTON_THUMB_R);
  if (r3Pressed && !r3WasPressed_) {
    invertSteering_ = !invertSteering_;
    if (invertSteering_) {
      Serial.println("[INVERT] Kemudi Belok: INVERTED (Kiri <-> Kanan Terbalik)");
      playRumble(controller, 250, 0, 180);
    } else {
      Serial.println("[INVERT] Kemudi Belok: NORMAL");
      playRumble(controller, 120, 0, 100);
    }
    saveCurrentSettings();
  }
  r3WasPressed_ = r3Pressed;

  // Analog Stick readings
  // Bluepad32 axisY is negative when pushed UP; inverted here so UP is positive
  const int ly = normalizeAxis(-controller->axisY());
  // Bluepad32 axisRX is positive when pushed RIGHT
  const int lx = normalizeAxis(controller->axisRX());
  const bool stickActive = (ly != 0 || lx != 0);

  // Emergency Handbrake (Circle / B button) or Stop (Cross / A button)
  const bool stopButtonPressed = (buttons & BUTTON_A);
  const bool handbrakePressed = (buttons & BUTTON_B);

  if (handbrakePressed) {
    handbrakeActive_ = true;
    if (cheatsheet_.isActive()) cheatsheet_.stop();
    motor_.stop();
    updateControllerFeedback(controller);
    return;
  }
  handbrakeActive_ = false;

  // Tactical Cheatsheet / Autonomous Maneuvers
  if (cheatsheet_.isActive()) {
    // Instant seamless operator override: touching stick or pressing Stop cancels tactic
    if (stopButtonPressed || stickActive) {
      cheatsheet_.stop();
      cheatComboWasDown_ = false;
      Serial.println("[TACTIC] Operator resumed manual control");
      // Falls through to manual drive logic below with zero latency!
    } else {
      updateControllerFeedback(controller);
      return;
    }
  } else {
    if (checkCheatsheetCombos(controller, buttons, dpad)) {
      updateControllerFeedback(controller);
      return;
    }
  }

  // Toggle Anti-Wheelie Acceleration Ramp (Triangle / Y button)
  const bool yPressed = (buttons & BUTTON_Y);
  if (yPressed && !yWasPressed_) {
    const bool newState = !motor_.isAccelerationEnabled();
    motor_.setAccelerationEnabled(newState);
    if (newState) {
      Serial.println("[DRIVE] Anti-Wheelie Ramp: ON (Smooth Launch)");
      playRumble(controller, 150, 100, 100);
    } else {
      Serial.println("[DRIVE] Anti-Wheelie Ramp: OFF (Instant Raw Punch)");
      playRumble(controller, 300, 200, 200);
    }
    saveCurrentSettings();
  }
  yWasPressed_ = yPressed;

  // Cycle Speed Mode (L1 / BUTTON_SHOULDER_L)
  const bool l1Pressed = (buttons & BUTTON_SHOULDER_L);
  if (l1Pressed && !l1WasPressed_) {
    int nextMode = static_cast<int>(speedMode_) + 1;
    if (nextMode > SPEED_MODE_FAST) nextMode = SPEED_MODE_LOW;
    setSpeedMode(nextMode, controller);
    saveCurrentSettings();
  }
  l1WasPressed_ = l1Pressed;

  // Attack / Turbo Mode (R1 / BUTTON_SHOULDER_R held)
  attackMode_ = (buttons & BUTTON_SHOULDER_R);

  int baseSpeed = SumobotConfig::SPEED_NORMAL;
  switch (speedMode_) {
    case SPEED_MODE_LOW:
      baseSpeed = SumobotConfig::SPEED_LOW;
      break;
    case SPEED_MODE_FAST:
      baseSpeed = SumobotConfig::SPEED_FAST;
      break;
    case SPEED_MODE_NORMAL:
    default:
      baseSpeed = SumobotConfig::SPEED_NORMAL;
      break;
  }

  const int speedLimit = attackMode_ ? SumobotConfig::MAX_SPEED : baseSpeed;

  int throttle = 0;
  int steer = 0;

  if (stickActive) {
    // Proportional drive from analog sticks
    throttle = map(ly, -SumobotConfig::CONTROLLER_AXIS_LIMIT,
                   SumobotConfig::CONTROLLER_AXIS_LIMIT, -speedLimit, speedLimit);
    steer = map(lx, -SumobotConfig::CONTROLLER_AXIS_LIMIT,
                SumobotConfig::CONTROLLER_AXIS_LIMIT, -speedLimit, speedLimit);
  } else if (dpad != 0) {
    // Fallback D-Pad drive when sticks are centered
    if (dpad & DPAD_UP) throttle += speedLimit;
    if (dpad & DPAD_DOWN) throttle -= speedLimit;
    if (dpad & DPAD_RIGHT) steer += speedLimit;
    if (dpad & DPAD_LEFT) steer -= speedLimit;
  }

  // Terapkan inversi arah kemudi jika diaktifkan (via L3 / R3)
  if (invertThrottle_) {
    throttle = -throttle;
  }
  if (invertSteering_) {
    steer = -steer;
  }

  // Differential / Arcade Drive mixing:
  // Turning Right (steer > 0) -> Left motor increases, Right motor decreases (Clockwise)
  // Turning Left  (steer < 0) -> Left motor decreases, Right motor increases (Counter-Clockwise)
  int leftMotor = throttle + steer;
  int rightMotor = throttle - steer;

  // Normalize so neither motor exceeds speedLimit while preserving steering ratio
  const int maxMag = max(abs(leftMotor), abs(rightMotor));
  if (maxMag > speedLimit) {
    leftMotor = (leftMotor * speedLimit) / maxMag;
    rightMotor = (rightMotor * speedLimit) / maxMag;
  }

  // In Attack Mode, ramp is bypassed for explosive instantaneous torque
  motor_.drive(leftMotor, rightMotor, /*bypassRamp=*/attackMode_);

  updateControllerFeedback(controller);

#if SUMOBOT_DEBUG_LOGGING
  Serial.printf("[DRIVE] L=%d R=%d | SpeedLimit=%d Attack=%s\n", leftMotor, rightMotor,
                speedLimit, attackMode_ ? "ON" : "OFF");
#endif
}

void SumobotApp::processControllers() {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
    ControllerPtr controller = controllers_[i];
    if (controller && controller->isConnected() && controller->hasData()) {
      if (controller->isGamepad()) {
        processGamepad(controller);
        lastControllerData_ = millis();
        return;
      }
      Serial.println("[BP32] Unsupported controller type");
    }
  }

  if (!hasConnectedController() && !cheatsheet_.isActive()) {
    motor_.stop();
  }
}
