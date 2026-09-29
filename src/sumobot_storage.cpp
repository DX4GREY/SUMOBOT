#include "sumobot_storage.h"

#include <LittleFS.h>

bool SumobotStorage::initialized_ = false;

bool SumobotStorage::begin() {
  if (initialized_) return true;

  // Mount LittleFS, format automatically if unformatted
  if (!LittleFS.begin(true)) {
    Serial.println("[STORAGE] Error: Failed to mount LittleFS!");
    return false;
  }

  initialized_ = true;
  Serial.printf("[STORAGE] LittleFS mounted (Total: %u KB, Used: %u KB)\n",
                static_cast<unsigned int>(LittleFS.totalBytes() / 1024),
                static_cast<unsigned int>(LittleFS.usedBytes() / 1024));
  return true;
}

bool SumobotStorage::load(SumobotSettings& settings) {
  if (!begin()) return false;

  if (!LittleFS.exists(CONFIG_PATH)) {
    Serial.println("[STORAGE] No existing config found. Creating defaults in LittleFS...");
    return save(settings);
  }

  File file = LittleFS.open(CONFIG_PATH, "r");
  if (!file) {
    Serial.println("[STORAGE] Error: Failed to open config file for reading");
    return false;
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0 || line.startsWith("#")) continue;

    int sep = line.indexOf('=');
    if (sep == -1) continue;

    String key = line.substring(0, sep);
    String val = line.substring(sep + 1);
    key.trim();
    val.trim();

    if (key.equalsIgnoreCase("invert_throttle")) {
      settings.invertThrottle = (val == "1" || val.equalsIgnoreCase("true"));
    } else if (key.equalsIgnoreCase("invert_steering")) {
      settings.invertSteering = (val == "1" || val.equalsIgnoreCase("true"));
    } else if (key.equalsIgnoreCase("speed_mode")) {
      int mode = val.toInt();
      if (mode >= 1 && mode <= 3) settings.speedMode = mode;
    } else if (key.equalsIgnoreCase("accel_enabled")) {
      settings.accelerationEnabled = (val == "1" || val.equalsIgnoreCase("true"));
    }
  }

  file.close();
  Serial.println("[STORAGE] Loaded settings from LittleFS:");
  print(settings);
  return true;
}

bool SumobotStorage::save(const SumobotSettings& settings) {
  if (!begin()) return false;

  File file = LittleFS.open(CONFIG_PATH, "w");
  if (!file) {
    Serial.println("[STORAGE] Error: Failed to open config file for writing");
    return false;
  }

  file.println("# SUMOBOT 500G SETTINGS (LittleFS)");
  file.printf("invert_throttle=%d\n", settings.invertThrottle ? 1 : 0);
  file.printf("invert_steering=%d\n", settings.invertSteering ? 1 : 0);
  file.printf("speed_mode=%u\n", settings.speedMode);
  file.printf("accel_enabled=%d\n", settings.accelerationEnabled ? 1 : 0);
  file.close();

  Serial.println("[STORAGE] Saved settings to LittleFS:");
  print(settings);
  return true;
}

void SumobotStorage::print(const SumobotSettings& settings) {
  Serial.printf("  - Invert Throttle (L3): %s\n", settings.invertThrottle ? "ON (Inverted)" : "OFF (Normal)");
  Serial.printf("  - Invert Steering (R3): %s\n", settings.invertSteering ? "ON (Inverted)" : "OFF (Normal)");
  Serial.printf("  - Speed Mode (L1):      Mode %u\n", settings.speedMode);
  Serial.printf("  - Anti-Wheelie Ramp(Y): %s\n", settings.accelerationEnabled ? "ENABLED" : "DISABLED");
}
