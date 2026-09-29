#pragma once

#include <Arduino.h>

struct SumobotSettings {
  bool invertThrottle = false;
  bool invertSteering = false;
  uint8_t speedMode = 2;            // 1: Low, 2: Normal, 3: Fast
  bool accelerationEnabled = true;  // Anti-wheelie ramp
};

class SumobotStorage {
 public:
  static bool begin();
  static bool load(SumobotSettings& settings);
  static bool save(const SumobotSettings& settings);
  static void print(const SumobotSettings& settings);

 private:
  static constexpr const char* CONFIG_PATH = "/config.txt";
  static bool initialized_;
};
