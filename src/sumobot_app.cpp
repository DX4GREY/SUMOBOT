#include "sumobot_app.h"

#include "sumobot_config.h"

SumobotApp* SumobotApp::instance_ = nullptr;

SumobotApp::SumobotApp() : cheatsheet_(motor_) {}

void SumobotApp::begin() {
  instance_ = this;
  Serial.begin(115200);
  Serial.println("\n===== SUMOBOT FRAMEWORK =====");

  pinMode(SumobotConfig::STATUS_LED, OUTPUT);
  digitalWrite(SumobotConfig::STATUS_LED, LOW);
  motor_.begin();

  String firmwareVersion = BP32.firmwareVersion();
  Serial.printf("[INIT] Bluepad32 Firmware: %s\n", firmwareVersion.c_str());
  const uint8_t* address = BP32.localBdAddress();
  Serial.printf("[INIT] BD Addr: %02X:%02X:%02X:%02X:%02X:%02X\n",
                address[0], address[1], address[2], address[3], address[4],
                address[5]);

  BP32.setup(&SumobotApp::onConnectedController,
             &SumobotApp::onDisconnectedController);
  BP32.enableVirtualDevice(false);
  BP32.forgetBluetoothKeys();
  Serial.println("[INIT] Waiting for controller...");
}

void SumobotApp::update() {
  if (BP32.update()) processControllers();

  if (hasConnectedController() &&
      millis() - lastControllerData_ > SumobotConfig::CONTROLLER_TIMEOUT_MS &&
      !cheatsheet_.isActive()) {
    motor_.stop();
  }

  cheatsheet_.update();

  if (millis() - lastBlink_ > 500) {
    lastBlink_ = millis();
    if (!hasConnectedController()) {
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
      Serial.printf("[BP32] Model=%s VID=0x%04x PID=0x%04x\n",
                    controller->getModelName().c_str(), properties.vendor_id,
                    properties.product_id);
      digitalWrite(SumobotConfig::STATUS_LED, HIGH);
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

bool SumobotApp::processCheatsheetButtons(uint16_t buttons) {
  const bool stopPressed = buttons & BUTTON_A;
  const bool comboPressed = (buttons & BUTTON_Y) && (buttons & BUTTON_B) &&
                            (buttons & BUTTON_X);

  if (stopPressed) {
    cheatComboWasDown_ = false;
    cheatsheet_.stop();
    return true;
  }

  if (comboPressed && !cheatComboWasDown_ && !cheatsheet_.isActive()) {
    cheatsheet_.start();
  }
  cheatComboWasDown_ = comboPressed;
  return cheatsheet_.isActive();
}

void SumobotApp::processGamepad(ControllerPtr controller) {
  const uint16_t buttons = controller->buttons();
  if (processCheatsheetButtons(buttons)) return;

  turboMode_ = buttons & BUTTON_SHOULDER_R;
  const int speedLimit = turboMode_ ? SumobotConfig::MAX_SPEED
                                    : SumobotConfig::NORMAL_SPEED;
  const int deadAnalog = 40;
  const int axisY = controller->axisY();
  const int axisX = controller->axisRX();
  const int ly = axisY == 0 ? 0 : axisY > deadAnalog ? 512 :
                 axisY < -deadAnalog ? -512 : 0;
  const int lx = axisX == 0 ? 0 : axisX > deadAnalog ? 512 :
                 axisX < -deadAnalog ? -512 : 0;

  const int throttle = map(-ly, -512, 512, -speedLimit, speedLimit);
  const int steering = map(-lx, -512, 512, -speedLimit, speedLimit);
  int leftMotor;
  int rightMotor;

  if (abs(throttle) < 20) {
    leftMotor = steering;
    rightMotor = -steering;
  } else {
    const float steer = static_cast<float>(steering) / speedLimit;
    constexpr float MIN_TURN_RATIO = 0.3f;
    if (steer > 0) {
      leftMotor = throttle;
      rightMotor = throttle * (1.0f - steer * (1.0f - MIN_TURN_RATIO));
    } else {
      rightMotor = throttle;
      leftMotor = throttle * (1.0f + steer * (1.0f - MIN_TURN_RATIO));
    }
  }

  leftMotor = constrain(leftMotor, -speedLimit, speedLimit);
  rightMotor = constrain(rightMotor, -speedLimit, speedLimit);
  if (abs(leftMotor) < 10) leftMotor = 0;
  if (abs(rightMotor) < 10) rightMotor = 0;

  motor_.drive(leftMotor, rightMotor);
  Serial.printf("[BP32] LX=%d LY=%d | Turbo=%s\n", lx, ly,
                turboMode_ ? "ON" : "OFF");
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
      Serial.println("[BP32] Unsupported controller");
    }
  }

  if (!hasConnectedController() && !cheatsheet_.isActive()) motor_.stop();
}
